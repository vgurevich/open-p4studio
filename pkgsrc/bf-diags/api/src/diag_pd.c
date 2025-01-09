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


/*!
 * @file diag_pd.c
 * @date
 *
 * Contains invocation of PD commands from diag
 *
 */

#include <stdio.h>
#include <string.h>
#include "diag_common.h"
#include "diag_pd.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_mc.h>
#include "diag_vlan.h"
#include "diag_util.h"
#include "diag_create_pkt.h"
#include "diag_pkt_database.h"

#define DIAG_DEF_MC_GRP_RID 0x8000
#define DIAG_TCAM_ENTRIES_MAX 900
#define DIAG_EXM_ENTRIES_MAX 200

#if defined(DIAG_PATTERN_SHIFT_ENABLE)
/* program pattern shift table */
bf_status_t diag_pd_add_pattern_shift(bf_dev_id_t dev_id,
                                      uint32_t size_index,
                                      uint32_t pkt_length,
                                      bool is_right_shift) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_tbl_pattern_shift_match_spec_t match_spec;
  p4_pd_status_t status = 0;

  memset(&match_spec, 0, sizeof(match_spec));

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.eg_intr_md_pkt_length = pkt_length;
  match_spec.egress_md_size_index = size_index;

  if (is_right_shift) {
    /* Make the PD call */
    status = p4_pd_diag_tbl_pattern_shift_table_add_with_increment_pkt_size(
        sess_hdl, dev_tgt, &match_spec, &entry_hdl);
  } else {
    status = p4_pd_diag_tbl_pattern_shift_table_add_with_restore_pkt_size(
        sess_hdl, dev_tgt, &match_spec, &entry_hdl);
  }
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
}

#define ORIG_PKT_SIZE 2194
bf_status_t diag_pd_read_counter(bf_dev_id_t dev_id) {
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  (void)dev_tgt;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;
  dev_tgt.dev_pipe_id = 0;

  uint32_t value = 0;
  int value_cnt = 0;
  status = p4_pd_diag_register_read_packet_counter(
      sess_hdl, dev_tgt, 0, 1, &value, &value_cnt);
  DIAG_PRINT("packet counter value is hex %x, decimal %u \n", value, value);

  for (int idx = ORIG_PKT_SIZE - 1; idx <= ORIG_PKT_SIZE + 32; idx = idx + 1) {
    status = p4_pd_diag_register_read_size_counter(
        sess_hdl, dev_tgt, idx, 1, &value, &value_cnt);
    DIAG_PRINT("size counter idx, value is %d, %u \n", idx, value);
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
}
bf_status_t diag_pd_reset_shift_counter(bf_dev_id_t dev_id) {
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  (void)dev_tgt;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  uint32_t value = 0;
  int value_cnt = 0;
  status = p4_pd_diag_register_read_packet_counter(
      sess_hdl, dev_tgt, 0, 1, &value, &value_cnt);
  DIAG_PRINT("packet counter value is %x \n", value);
  status = p4_pd_diag_register_reset_all_packet_counter(sess_hdl, dev_tgt);
  status = p4_pd_diag_register_reset_all_size_counter(sess_hdl, dev_tgt);

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
}

#endif  // DIAG_PATTERN_SHIFT_ENABLE

/* Add default entries */
bf_status_t diag_pd_add_default_entries(bf_dev_id_t dev_id) {
#if !defined(DIAG_PHV_FLOP_TEST)
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_status_t status = 0;
  p4_pd_diag_set_ing_port_prop_action_spec_t ing_prop_action_spec;
  void *cb_fn_cookie = NULL;

  (void)dev_tgt;
  (void)cb_fn_cookie;

  DIAG_DEV_INFO(dev_id)->mac_aging_ttl = DIAG_DEF_TTL;
  DIAG_DEV_INFO(dev_id)->lrn_timeout = DIAG_LEARN_TIMEOUT;
  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  /* Default entry for ing_port_prop */
  memset(&ing_prop_action_spec, 0, sizeof(ing_prop_action_spec));
  ing_prop_action_spec.action_exclusion_id = 0;
#ifdef SKIP_DEF_ENTRY
  p4_pd_diag_ing_port_prop_set_default_action_set_ing_port_prop(
      sess_hdl, dev_tgt, &ing_prop_action_spec, &entry_hdl);
#endif

#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  // if model, pkt_length w/0 ETH CRC
  // if hw, pkt_length contains ETH CRC
  int max_shift = 31;
  diag_pd_add_pattern_shift(dev_id, 0, ORIG_PKT_SIZE + max_shift, false);
  for (int idx = 1; idx <= max_shift; idx++) {
    diag_pd_add_pattern_shift(dev_id, idx, ORIG_PKT_SIZE + idx - 1, true);
  }
#endif

#if defined(DIAG_ADVANCED_FEATURES)
  /* Default entry for port_vlan_mapping */
  p4_pd_diag_port_vlan_mapping_set_default_action_port_vlan_miss(
      sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for def_vlan_mapping */
  p4_pd_diag_def_vlan_mapping_set_default_action_def_vlan_miss(
      sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for smac */
  p4_pd_diag_smac_set_default_action_smac_miss(sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for dmac */
  p4_pd_diag_dmac_set_default_action_dmac_miss(sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for bd-flood */
  p4_pd_diag_bd_flood_set_default_action_nop(sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for learn-notify */
  p4_pd_diag_learn_notify_set_default_action_nop(sess_hdl, dev_tgt, &entry_hdl);

  /* Default entry for vlan decap */
  p4_pd_diag_vlan_decap_set_default_action_nop(sess_hdl, dev_tgt, &entry_hdl);
  /* Add default match entry */
  diag_pd_add_vlan_decap(dev_id, &entry_hdl);

  /* Default entry for vlan encap */
  p4_pd_diag_vlan_encap_set_default_action_set_packet_vlan_untagged(
      sess_hdl, dev_tgt, &entry_hdl);

  /* Action entries for stage0-18 */

  int stage_pri = 0;
  uint16_t tbl_stage_cntr = 0;

  p4_pd_diag_tbl_stage0_match_spec_t match_spec_0;
  match_spec_0.ethernet_valid = 0x1;
  p4_pd_diag_tbl_stage0_table_add_with_set_meta0(
      sess_hdl, dev_tgt, &match_spec_0, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage1_match_spec_t match_spec_1;
  memset(&match_spec_1, 0, sizeof(match_spec_1));
  match_spec_1.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_1.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage1_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_1, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

#if 0
  p4_pd_diag_tbl_stage3_match_spec_t match_spec_3;
  memset(&match_spec_3, 0, sizeof(match_spec_3));
  match_spec_3.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_3.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage3_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_3, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage4_match_spec_t match_spec_4;
  memset(&match_spec_4, 0, sizeof(match_spec_4));
  match_spec_4.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage4_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_4, &entry_hdl);
  tbl_stage_cntr++;
#endif

  p4_pd_diag_tbl_stage5_match_spec_t match_spec_5;
  memset(&match_spec_5, 0, sizeof(match_spec_5));
  match_spec_5.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_5.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage5_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_5, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

#if !defined(TOFINO2H)
  p4_pd_diag_tbl_stage6_match_spec_t match_spec_6;
  memset(&match_spec_6, 0, sizeof(match_spec_6));
  match_spec_6.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage6_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_6, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage7_match_spec_t match_spec_7;
  memset(&match_spec_7, 0, sizeof(match_spec_7));
  match_spec_7.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_7.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage7_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_7, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage8_match_spec_t match_spec_8;
  memset(&match_spec_8, 0, sizeof(match_spec_8));
  match_spec_8.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage8_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_8, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage9_match_spec_t match_spec_9;
  memset(&match_spec_9, 0, sizeof(match_spec_9));
  match_spec_9.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_9.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage9_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_9, stage_pri, &entry_hdl);
  tbl_stage_cntr++;
#endif

#if !defined(TOFINO2H)
  p4_pd_diag_tbl_stage10_match_spec_t match_spec_10;
  memset(&match_spec_10, 0, sizeof(match_spec_10));
  match_spec_10.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage10_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_10, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage11_match_spec_t match_spec_11;
  memset(&match_spec_11, 0, sizeof(match_spec_11));
  match_spec_11.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_11.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage11_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_11, stage_pri, &entry_hdl);
  tbl_stage_cntr++;
#endif

#if !defined(TOFINO1) && !defined(TOFINO2M) && !defined(TOFINO2H)
  p4_pd_diag_tbl_stage12_match_spec_t match_spec_12;
  memset(&match_spec_12, 0, sizeof(match_spec_12));
  match_spec_12.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage12_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_12, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage13_match_spec_t match_spec_13;
  memset(&match_spec_13, 0, sizeof(match_spec_13));
  match_spec_13.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_13.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage13_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_13, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage14_match_spec_t match_spec_14;
  memset(&match_spec_14, 0, sizeof(match_spec_14));
  match_spec_14.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage14_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_14, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage15_match_spec_t match_spec_15;
  memset(&match_spec_15, 0, sizeof(match_spec_15));
  match_spec_15.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_15.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage15_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_15, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage16_match_spec_t match_spec_16;
  memset(&match_spec_16, 0, sizeof(match_spec_16));
  match_spec_16.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage16_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_16, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage17_match_spec_t match_spec_17;
  memset(&match_spec_17, 0, sizeof(match_spec_17));
  match_spec_17.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_17.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage17_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_17, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage18_match_spec_t match_spec_18;
  memset(&match_spec_18, 0, sizeof(match_spec_18));
  match_spec_18.l2_metadata_inter_stage = tbl_stage_cntr;
  p4_pd_diag_tbl_stage18_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_18, &entry_hdl);
  tbl_stage_cntr++;

  p4_pd_diag_tbl_stage19_match_spec_t match_spec_19;
  memset(&match_spec_19, 0, sizeof(match_spec_19));
  match_spec_19.l2_metadata_inter_stage = tbl_stage_cntr;
  match_spec_19.l2_metadata_inter_stage_mask = 0xff;
  p4_pd_diag_tbl_stage19_table_add_with_set_meta(
      sess_hdl, dev_tgt, &match_spec_19, stage_pri, &entry_hdl);
  tbl_stage_cntr++;

#endif

  /* End of default entries */

  /* Store learn session handle as it is needed for acking the learns */
  status = p4_pd_client_init(&(DIAG_DEV_INFO(dev_id)->learn_sess_hdl));
  if (status != 0) {
    DIAG_PRINT("Client init failed for learn session");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }
  /* Register for mac learn notification */
  p4_pd_diag_SwitchIngressDeparser_digest_register(
      DIAG_DEV_INFO(dev_id)->learn_sess_hdl,
      dev_id,
      diag_mac_learn_digest_notify_cb,
      cb_fn_cookie);
  /* Set learning timeout */
  diag_pd_learning_timeout_set(dev_id, DIAG_DEV_INFO(dev_id)->lrn_timeout);
  /* Enable idle timeout on dmac table */
  diag_pd_idle_tmo_en_dmac(dev_id);
  /* Enable idle timeout on smac table */
  diag_pd_idle_tmo_en_smac(dev_id);

  /* Setup yid prune table */
  diag_pd_add_yid(dev_id);
#endif  // DIAG_ADVANCED_FEATURES

  /* Setup default port properties */
  diag_pd_add_ing_port_prop(dev_id);

  /* Add entry for learning notification */
  diag_pd_add_learn_notify(dev_id, &entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);
#else  // DIAG_PHV_FLOP_TEST
  (void)dev_id;
#endif

  return BF_SUCCESS;
}

/* Set default port properties */
bf_status_t diag_pd_add_ing_port_prop(bf_dev_id_t dev_id) {
#if !defined(DIAG_PHV_FLOP_TEST)
  bf_dev_port_t dev_port = 0;
  unsigned int pipe, port;
  uint32_t num_pipes = 0;
  p4_pd_sess_hdl_t sess_hdl;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_ing_port_prop_match_spec_t match_spec;
  p4_pd_diag_set_ing_port_prop_action_spec_t action_spec;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_status_t status = 0;

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  num_pipes = DIAG_DEV_INFO(dev_id)->num_active_pipes;

  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port = 0; port < BF_PIPE_PORT_COUNT; port += 1) {
      dev_port = MAKE_DEV_PORT(pipe, port);
      /* Create match spec */
      match_spec.ig_intr_md_ingress_port = dev_port;
      /* Create Action spec */
      action_spec.action_exclusion_id =
          DIAG_MAKE_72_PIPE_PORT(pipe, port); /* yid */
      action_spec.action_port = dev_port;
      /* Make the PD call */
      p4_pd_diag_ing_port_prop_table_add_with_set_ing_port_prop(
          sess_hdl, dev_tgt, &match_spec, &action_spec, &entry_hdl);
    }
  }
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);
#else  // DIAG_PHV_FLOP_TEST
  (void)dev_id;
#endif

  return BF_SUCCESS;
}

/* Set dst override */
bf_status_t diag_pd_add_dst_override(bf_dev_id_t dev_id,
                                     bf_dev_port_t ig_port,
                                     bf_dev_port_t dst_port,
                                     uint32_t tcp_dstPort_start,
                                     uint32_t tcp_dstPort_end,
                                     int priority,
                                     p4_pd_entry_hdl_t *entry_hdl) {
#if !defined(DIAG_PHV_FLOP_CONFIG_3)
  p4_pd_sess_hdl_t sess_hdl;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_dst_override_match_spec_t match_spec;
  p4_pd_status_t status = 0;

  DIAG_PRINT(
      "dst-override-add: ig_port %d, dst-port %d, tcp (%d-%d), pri %d \n",
      ig_port,
      dst_port,
      tcp_dstPort_start,
      tcp_dstPort_end,
      priority);

  memset(&match_spec, 0, sizeof(match_spec));

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ig_intr_md_ingress_port = ig_port;
#if defined(DIAG_PHV_STRESS_ENABLE)
  match_spec.phv_stress_hdr_f0_start = tcp_dstPort_start;
  match_spec.phv_stress_hdr_f0_end = tcp_dstPort_end;
#elif defined(DIAG_PHV_FLOP_TEST)
  match_spec.testdata_pkt_ctrl_start = tcp_dstPort_start;
  match_spec.testdata_pkt_ctrl_end = tcp_dstPort_end;
#else
  match_spec.tcp_dstPort_start = tcp_dstPort_start;
  match_spec.tcp_dstPort_end = tcp_dstPort_end;
#endif

  /* For pkts destined to CPU, encode the ingress port in the tcp srcport */
  if (dst_port == diag_cpu_port_get(dev_id, ig_port)) {
    /* Create action spec */
    p4_pd_diag_override_eg_port_to_cpu_action_spec_t action_spec_to_cpu;
    memset(&action_spec_to_cpu, 0, sizeof(action_spec_to_cpu));
    action_spec_to_cpu.action_port = dst_port;

    /* Make the PD call */
    status = p4_pd_diag_dst_override_table_add_with_override_eg_port_to_cpu(
        sess_hdl,
        dev_tgt,
        &match_spec,
        priority,
        &action_spec_to_cpu,
        entry_hdl);
  } else {
    /* Create action spec */
    p4_pd_diag_override_eg_port_action_spec_t action_spec;
    memset(&action_spec, 0, sizeof(action_spec));
    action_spec.action_port = dst_port;

    /* Make the PD call */
    status = p4_pd_diag_dst_override_table_add_with_override_eg_port(
        sess_hdl, dev_tgt, &match_spec, priority, &action_spec, entry_hdl);
  }
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Set Multicast dst override */
bf_status_t diag_pd_mc_add_dst_override(bf_dev_id_t dev_id,
                                        bf_dev_port_t ig_port,
                                        bf_mc_grp_id_t mc_grp_id_a,
                                        bf_mc_grp_id_t mc_grp_id_b,
                                        uint32_t tcp_dstPort_start,
                                        uint32_t tcp_dstPort_end,
                                        int priority,
                                        p4_pd_entry_hdl_t *entry_hdl) {
#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
  p4_pd_sess_hdl_t sess_hdl;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_dst_override_match_spec_t match_spec;
  p4_pd_status_t status = 0;
  /*
   For unknown unicast, vlan-id is used as the rid.
   For multicast using dst-override, set rid to a value greater than 4K,
   so that the vlan pruning logic does not kick in.
  */
  bf_mc_rid_t rid = DIAG_DEF_MC_GRP_RID;

  DIAG_PRINT(
      "mc-dst-override-add: ig_port %d, mc-grp-id-a %d, mc-grp-id-b %d, tcp "
      "(%d-%d), pri %d \n",
      ig_port,
      mc_grp_id_a,
      mc_grp_id_b,
      tcp_dstPort_start,
      tcp_dstPort_end,
      priority);

  memset(&match_spec, 0, sizeof(match_spec));

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ig_intr_md_ingress_port = ig_port;
  match_spec.tcp_dstPort_start = tcp_dstPort_start;
  match_spec.tcp_dstPort_end = tcp_dstPort_end;

  /* Create action spec */
  p4_pd_diag_override_mc_eg_port_action_spec_t action_spec;
  memset(&action_spec, 0, sizeof(action_spec));
  action_spec.action_mgid_a = mc_grp_id_a;
  action_spec.action_mgid_b = mc_grp_id_b;
  action_spec.action_rid = rid;

  /* Make the PD call */
  status = p4_pd_diag_dst_override_table_add_with_override_mc_eg_port(
      sess_hdl, dev_tgt, &match_spec, priority, &action_spec, entry_hdl);

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Remove dst override */
bf_status_t diag_pd_del_dst_override(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t entry_hdl) {
#if !defined(DIAG_PHV_FLOP_CONFIG_3)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("dst-override-del: handle %d \n", entry_hdl);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_dst_override_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Remove dst override by match spec */
bf_status_t diag_pd_del_dst_override_by_match_spec(bf_dev_id_t dev_id,
                                                   bf_dev_port_t ig_port,
                                                   uint32_t tcp_dstPort_start,
                                                   uint32_t tcp_dstPort_end,
                                                   int priority) {
#if !defined(DIAG_PHV_FLOP_CONFIG_3)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_dst_override_match_spec_t match_spec;

  DIAG_PRINT(
      "dst-override-del-by-match-spec: ig_port %d, tcp (%d-%d), pri %d \n",
      ig_port,
      tcp_dstPort_start,
      tcp_dstPort_end,
      priority);

  memset(&match_spec, 0, sizeof(match_spec));

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ig_intr_md_ingress_port = ig_port;
#if defined(DIAG_PHV_STRESS_ENABLE)
  match_spec.phv_stress_hdr_f0_start = tcp_dstPort_start;
  match_spec.phv_stress_hdr_f0_end = tcp_dstPort_end;
#elif defined(DIAG_PHV_FLOP_TEST)
  match_spec.testdata_pkt_ctrl_start = tcp_dstPort_start;
  match_spec.testdata_pkt_ctrl_end = tcp_dstPort_end;
#else
  match_spec.tcp_dstPort_start = tcp_dstPort_start;
  match_spec.tcp_dstPort_end = tcp_dstPort_end;
#endif

  /* Make the PD call */
  status = p4_pd_diag_dst_override_table_delete_by_match_spec(
      sess_hdl, dev_tgt, &match_spec, priority);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Program yid prune tbl */
bf_status_t diag_pd_add_yid(bf_dev_id_t dev_id) {
#if defined(DIAG_ADVANCED_FEATURES)
  int index = 0, yid = 0;
  unsigned int pipe, port;
  p4_pd_sess_hdl_t sess_hdl = 0;
  bf_mc_port_map_t port_map;
  p4_pd_status_t status = 0;
  uint32_t num_pipes = 0;

  memset(&port_map, 0, sizeof(port_map));

  p4_pd_mc_create_session(&sess_hdl);
  num_pipes = DIAG_DEV_INFO(dev_id)->num_active_pipes;

  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port = 0; port < BF_PIPE_PORT_COUNT; port += 1) {
      memset(&port_map, 0, sizeof(port_map));
      index = DIAG_MAKE_72_PIPE_PORT(pipe, port);
      yid = index;
      /* Max yid is 288 */
      if (yid >= (DIAG_SUBDEV_PIPE_COUNT * BF_PIPE_PORT_COUNT)) {
        break;
      }
      port_map[index / 8] = (port_map[index / 8] | (1 << (index % 8))) & 0xFF;
      status |=
          p4_pd_mc_update_port_prune_table(sess_hdl, dev_id, yid, &port_map[0]);
    }
  }

  p4_pd_mc_complete_operations(sess_hdl);
  p4_pd_mc_destroy_session(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Set default vlan */
bf_status_t diag_pd_add_default_vlan(bf_dev_id_t dev_id,
                                     int vlan_id,
                                     bf_mc_rid_t rid,
                                     bf_dev_port_t dev_port,
                                     p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_def_vlan_mapping_match_spec_t match_spec;
  p4_pd_diag_set_def_vlan_action_spec_t action_spec;
  p4_pd_status_t status = 0;

  DIAG_PRINT("def-vlan-add: vlan %d, port %d \n", vlan_id, dev_port);

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));
  *entry_hdl = 0;

  dev_tgt.device_id = dev_id;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;
  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ig_intr_md_ingress_port = dev_port;

  /* Create Action spec */
  action_spec.action_vid = vlan_id;
  action_spec.action_ingress_rid = rid;

  /* Make the PD call */
  status = p4_pd_diag_def_vlan_mapping_table_add_with_set_def_vlan(
      sess_hdl, dev_tgt, &match_spec, &action_spec, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Remove default vlan */
bf_status_t diag_pd_del_default_vlan(bf_dev_id_t dev_id,
                                     int vlan_id,
                                     bf_dev_port_t dev_port,
                                     p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("def-vlan-del: vlan %d, port %d \n", vlan_id, dev_port);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status =
      p4_pd_diag_def_vlan_mapping_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Add to port-vlan mapping table */
bf_status_t diag_pd_add_port_vlan_mapping(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          bf_mc_rid_t rid,
                                          bf_dev_port_t dev_port,
                                          p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_port_vlan_mapping_match_spec_t match_spec;
  p4_pd_diag_set_ing_vlan_action_spec_t action_spec;
  p4_pd_status_t status = 0;

  DIAG_PRINT("port-vlan-add: vlan %d, port %d \n", vlan_id, dev_port);

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));
  *entry_hdl = 0;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ig_intr_md_ingress_port = dev_port;
  match_spec.vlan_tag_valid = 1;
  match_spec.vlan_tag_vlan_id = vlan_id;

  /* Create action spec */
  action_spec.action_vid = vlan_id;
  action_spec.action_ingress_rid = rid;

  /* Make the PD call */
  status = p4_pd_diag_port_vlan_mapping_table_add_with_set_ing_vlan(
      sess_hdl, dev_tgt, &match_spec, &action_spec, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del from port-vlan mapping table */
bf_status_t diag_pd_del_port_vlan_mapping(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          bf_dev_port_t dev_port,
                                          p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("port-vlan-del: vlan %d, port %d \n", vlan_id, dev_port);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status =
      p4_pd_diag_port_vlan_mapping_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Add mac,vlan to dmac table */
bf_status_t diag_pd_add_dmac(bf_dev_id_t dev_id,
                             int vlan_id,
                             bf_dev_port_t dev_port,
                             uint8_t *dmac,
                             int ttl,
                             p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_dmac_match_spec_t match_spec;
  p4_pd_diag_dmac_hit_action_spec_t action_spec;
  p4_pd_status_t status = 0;
  p4_pd_entry_hdl_t exis_entry_hdl = 0;

  DIAG_PRINT("vlan-dmac-add: vlan %d, port %d \n", vlan_id, dev_port);

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));
  *entry_hdl = 0;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ingress_metadata_vlan_id = vlan_id;
  memcpy(&match_spec.ethernet_dstAddr, dmac, DIAG_MAC_SIZE);

  /* Check for any existing entry */
  status = p4_pd_diag_dmac_match_spec_to_entry_hdl(
      sess_hdl, dev_tgt, &match_spec, &exis_entry_hdl);
  if (status == BF_SUCCESS) {
    DIAG_PRINT("dmac entry exists: Deleting existing entry with hdl %d \n",
               exis_entry_hdl);
    diag_pd_del_dmac(dev_id, exis_entry_hdl);
  }

  /* Create action spec */
  action_spec.action_port = dev_port;

  /* Make the PD call for pkts from CPU */
  status = p4_pd_diag_dmac_table_add_with_dmac_hit(
      sess_hdl, dev_tgt, &match_spec, &action_spec, ttl, entry_hdl);
  DIAG_PRINT("Added entry with entry-hdl %d\n", *entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del mac,vlan to dmac table */
bf_status_t diag_pd_del_dmac(bf_dev_id_t dev_id, p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("vlan-dmac-del: hdl %d \n", entry_hdl);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_dmac_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Enable idle time for dmac table */
bf_status_t diag_pd_idle_tmo_en_dmac(bf_dev_id_t dev_id) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_idle_time_params_t idle_params;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  memset(&idle_params, 0, sizeof(idle_params));
  idle_params.mode = PD_NOTIFY_MODE;
  idle_params.params.notify.ttl_query_interval = 25000;
  idle_params.params.notify.max_ttl = DIAG_MAX_TTL;
  idle_params.params.notify.min_ttl = DIAG_MIN_TTL;
  idle_params.params.notify.callback_fn = diag_dmac_idle_tmo_expiry_cb;

  /* Make the PD call */
  status = p4_pd_diag_dmac_idle_tmo_enable(sess_hdl, dev_id, idle_params);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Add mac to smac table */
bf_status_t diag_pd_add_smac(bf_dev_id_t dev_id,
                             int vlan_id,
                             bf_dev_port_t dev_port,
                             uint8_t *smac,
                             int ttl,
                             p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_smac_match_spec_t match_spec;
  p4_pd_status_t status = 0;
  p4_pd_entry_hdl_t exis_entry_hdl = 0;

  DIAG_PRINT("vlan-smac-add: vlan %d, port %d \n", vlan_id, dev_port);

  memset(&match_spec, 0, sizeof(match_spec));
  *entry_hdl = 0;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.ingress_metadata_vlan_id = vlan_id;
  memcpy(&match_spec.ethernet_srcAddr[0], smac, DIAG_MAC_SIZE);

  /* Check for any existing entry */
  status = p4_pd_diag_smac_match_spec_to_entry_hdl(
      sess_hdl, dev_tgt, &match_spec, &exis_entry_hdl);
  if (status == BF_SUCCESS) {
    DIAG_PRINT("smac entry exists with hdl %d, skipping add \n",
               exis_entry_hdl);
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_smac_table_add_with_smac_hit(
      sess_hdl, dev_tgt, &match_spec, ttl, entry_hdl);
  DIAG_PRINT("Added entry with entry-hdl %d\n", *entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del mac from smac table */
bf_status_t diag_pd_del_smac(bf_dev_id_t dev_id, p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("vlan-smac-del: hdl %d \n", entry_hdl);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_smac_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Enable idle time for smac table */
bf_status_t diag_pd_idle_tmo_en_smac(bf_dev_id_t dev_id) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_idle_time_params_t idle_params;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  memset(&idle_params, 0, sizeof(idle_params));
  idle_params.mode = PD_NOTIFY_MODE;
  idle_params.params.notify.ttl_query_interval = 25000;
  idle_params.params.notify.max_ttl = DIAG_MAX_TTL;
  idle_params.params.notify.min_ttl = 5000;
  idle_params.params.notify.callback_fn = diag_smac_idle_tmo_expiry_cb;

  /* Make the PD call */
  status = p4_pd_diag_smac_idle_tmo_enable(sess_hdl, dev_id, idle_params);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Set Flood MC-index for vlan */
bf_status_t diag_pd_add_bd_flood(bf_dev_id_t dev_id,
                                 int vlan_id,
                                 int mc_index,
                                 p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_bd_flood_match_spec_t match_spec;
  p4_pd_diag_set_bd_flood_mc_index_action_spec_t action_spec;
  p4_pd_status_t status = 0;

  DIAG_PRINT("mc-index-add: vlan %d, mc-index %d \n", vlan_id, mc_index);

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  /* Create match spec */
  match_spec.ingress_metadata_vlan_id = vlan_id;

  /* Create Action spec */
  action_spec.action_mc_index = mc_index;

  /* Make the PD call */
  status = p4_pd_diag_bd_flood_table_add_with_set_bd_flood_mc_index(
      sess_hdl, dev_tgt, &match_spec, &action_spec, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Set Flood MC-index for vlan */
bf_status_t diag_pd_del_bd_flood(bf_dev_id_t dev_id,
                                 int vlan_id,
                                 int mc_index,
                                 p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("mc-index-del: vlan %d, mc-index %d \n", vlan_id, mc_index);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_bd_flood_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Set flood ports of vlan */
bf_status_t diag_pd_add_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         bf_mc_rid_t rid,
                                         uint8_t *port_map,
                                         uint8_t *lag_map,
                                         p4_pd_entry_hdl_t *mc_grp_hdl,
                                         p4_pd_entry_hdl_t *mc_node_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_status_t status = 0;
  p4_pd_sess_hdl_t sess_hdl = 0;
  uint16_t xid = 0;
  bool xid_valid = false;

  DIAG_PRINT(
      "vlan-flood-ports-add: vlan %d, mc-index %d \n", vlan_id, mc_index);

  p4_pd_mc_create_session(&sess_hdl);

  p4_pd_mc_mgrp_create(sess_hdl, dev_id, mc_index, mc_grp_hdl);
  p4_pd_mc_node_create(sess_hdl, dev_id, rid, port_map, lag_map, mc_node_hdl);
  status = p4_pd_mc_associate_node(
      sess_hdl, dev_id, *mc_grp_hdl, *mc_node_hdl, xid, xid_valid);

  p4_pd_mc_complete_operations(sess_hdl);
  p4_pd_mc_destroy_session(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Update flood ports of vlan */
bf_status_t diag_pd_upd_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         uint8_t *port_map,
                                         uint8_t *lag_map,
                                         p4_pd_entry_hdl_t mc_node_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT(
      "vlan-flood-ports-upd: vlan %d, mc-index %d \n", vlan_id, mc_index);

  p4_pd_mc_create_session(&sess_hdl);

  status =
      p4_pd_mc_node_update(sess_hdl, dev_id, mc_node_hdl, port_map, lag_map);

  p4_pd_mc_complete_operations(sess_hdl);
  p4_pd_mc_destroy_session(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Remove all flood ports of vlan */
bf_status_t diag_pd_del_vlan_flood_ports(bf_dev_id_t dev_id,
                                         int vlan_id,
                                         int mc_index,
                                         p4_pd_entry_hdl_t mc_grp_hdl,
                                         p4_pd_entry_hdl_t mc_node_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT(
      "vlan-flood-ports-del: vlan %d, mc-index %d \n", vlan_id, mc_index);

  p4_pd_mc_create_session(&sess_hdl);

  if (mc_grp_hdl && mc_node_hdl) {
    status =
        p4_pd_mc_dissociate_node(sess_hdl, dev_id, mc_grp_hdl, mc_node_hdl);
    p4_pd_mc_node_destroy(sess_hdl, dev_id, mc_node_hdl);
    p4_pd_mc_mgrp_destroy(sess_hdl, dev_id, mc_grp_hdl);
  }
  p4_pd_mc_complete_operations(sess_hdl);
  p4_pd_mc_destroy_session(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Add learn-notify entry */
bf_status_t diag_pd_add_learn_notify(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_learn_notify_match_spec_t match_spec;
  p4_pd_status_t status = 0;

  memset(&match_spec, 0, sizeof(match_spec));

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  /* Create match spec */
  match_spec.l2_metadata_l2_src_miss = 1;

  /* Make the PD call */
  status = p4_pd_diag_learn_notify_table_add_with_generate_learn_notify(
      sess_hdl, dev_tgt, &match_spec, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del learn-notify entry */
bf_status_t diag_pd_del_learn_notify(bf_dev_id_t dev_id,
                                     p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("learn-notify-del \n");

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_learn_notify_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

#if defined(DIAG_ADVANCED_FEATURES)
p4_pd_status_t diag_mac_learn_digest_notify_cb(
    p4_pd_sess_hdl_t input_sess_hdl,
    p4_pd_diag_SwitchIngressDeparser_digest_digest_msg_t *msg,
    void *callback_fn_cookie) {
  int vlan_id = 0, i = 0, pipe = 0;
  uint8_t srcAddr[DIAG_MAC_SIZE];
  bf_dev_port_t dev_port = 0;
  bf_dev_id_t dev_id = 0;
  p4_pd_diag_SwitchIngressDeparser_digest_digest_entry_t *entry = NULL;
  p4_pd_entry_hdl_t entry_hdl = 0;

  (void)input_sess_hdl;
  (void)callback_fn_cookie;
  memset(&srcAddr[0], 0, sizeof(srcAddr));

  if (!msg) {
    return BF_INVALID_ARG;
  }
  dev_id = msg->dev_tgt.device_id;
  pipe = msg->dev_tgt.dev_pipe_id;
  (void)pipe;
#if 0
  DIAG_PRINT("Received %d learns on dev %d, pipe %d \n",
             msg->num_entries,
             dev_id,
             pipe);
#endif
  for (i = 0; i < msg->num_entries; i++) {
    entry = &(msg->entries[i]);
    dev_port = entry->ig_md_ingress_metadata_ingress_port;
    vlan_id = entry->ig_md_ingress_metadata_vlan_id;
    if (vlan_id == 0) {
      continue;
    }
    memcpy(&srcAddr[0], &(entry->hdr_ethernet_srcAddr[0]), DIAG_MAC_SIZE);
    DIAG_PRINT("Received learn on vlan %d, port %d \n", vlan_id, dev_port);
    /* Add smac entry */
    diag_pd_add_smac(dev_id,
                     vlan_id,
                     dev_port,
                     &srcAddr[0],
                     DIAG_DEV_INFO(dev_id)->mac_aging_ttl,
                     &entry_hdl);
    /* Add dmac entry */
    diag_pd_add_dmac(dev_id,
                     vlan_id,
                     dev_port,
                     &srcAddr[0],
                     DIAG_DEV_INFO(dev_id)->mac_aging_ttl,
                     &entry_hdl);
  }
  /* Ack the notificaton */
  p4_pd_diag_SwitchIngressDeparser_digest_notify_ack(
      DIAG_DEV_INFO(dev_id)->learn_sess_hdl, msg);
  return BF_SUCCESS;
}
#endif

void diag_dmac_idle_tmo_expiry_cb(bf_dev_id_t dev_id,
                                  p4_pd_entry_hdl_t entry_hdl,
                                  p4_pd_idle_time_hit_state_e hs,
                                  void *cookie) {
#if defined(DIAG_ADVANCED_FEATURES)
  if (hs == ENTRY_IDLE) {
    DIAG_PRINT("Received dmac timeout for entry_hdl %d \n", entry_hdl);
    diag_pd_del_dmac(dev_id, entry_hdl);
  } else {
    DIAG_PRINT("Received dmac activation for entry_hdl %d \n", entry_hdl);
  }
#endif

  return;
}

void diag_smac_idle_tmo_expiry_cb(bf_dev_id_t dev_id,
                                  p4_pd_entry_hdl_t entry_hdl,
                                  p4_pd_idle_time_hit_state_e hs,
                                  void *cookie) {
#if defined(DIAG_ADVANCED_FEATURES)
  if (hs == ENTRY_IDLE) {
    DIAG_PRINT("Received smac timeout for entry_hdl %d \n", entry_hdl);
    diag_pd_del_smac(dev_id, entry_hdl);
  } else {
    DIAG_PRINT("Received smac activation for entry_hdl %d \n", entry_hdl);
  }
#endif
  return;
}

/* Add entry to vlan encap */
bf_status_t diag_pd_add_vlan_encap(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int vlan_id,
                                   p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_vlan_encap_match_spec_t match_spec;
  p4_pd_diag_set_packet_vlan_tagged_action_spec_t action_spec;
  p4_pd_status_t status = 0;

  DIAG_PRINT("vlan-encap-add: port %d, vlan %d \n", dev_port, vlan_id);

  memset(&match_spec, 0, sizeof(match_spec));
  memset(&action_spec, 0, sizeof(action_spec));
  *entry_hdl = 0;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.eg_intr_md_egress_port = dev_port;
  match_spec.egress_metadata_vlan_id = vlan_id;

  /* Create aciton spec */
  action_spec.action_vlan_id = vlan_id;

  /* Vlan encap */
  status = p4_pd_diag_vlan_encap_table_add_with_set_packet_vlan_tagged(
      sess_hdl, dev_tgt, &match_spec, &action_spec, entry_hdl);
  DIAG_PRINT("Added entry with entry-hdl %d\n", *entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del entry from vlan encap */
bf_status_t diag_pd_del_vlan_encap(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("vlan-encap-del: port %d \n", dev_port);

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_vlan_encap_table_delete(sess_hdl, dev_id, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del entry from vlan encap using match spec */
bf_status_t diag_pd_del_vlan_encap_by_match_spec(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 int vlan_id) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_vlan_encap_match_spec_t match_spec;

  DIAG_PRINT(
      "vlan-encap-del-by-match-spec: port %d, vlan %d\n", dev_port, vlan_id);

  memset(&match_spec, 0, sizeof(match_spec));

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  /* Create match spec */
  match_spec.eg_intr_md_egress_port = dev_port;
  match_spec.egress_metadata_vlan_id = vlan_id;

  /* Make the PD call */
  status = p4_pd_diag_vlan_encap_table_delete_by_match_spec(
      sess_hdl, dev_tgt, &match_spec);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}
/* Add entry to vlan decap */
bf_status_t diag_pd_add_vlan_decap(bf_dev_id_t dev_id,
                                   p4_pd_entry_hdl_t *entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_diag_vlan_decap_match_spec_t match_spec;
  p4_pd_status_t status = 0;

  memset(&match_spec, 0, sizeof(match_spec));
  *entry_hdl = 0;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Create match spec */
  match_spec.vlan_tag_valid = 1;

  /* Vlan encap */
  status = p4_pd_diag_vlan_decap_table_add_with_remove_vlan_single_tagged(
      sess_hdl, dev_tgt, &match_spec, entry_hdl);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Del entry from vlan decap */
bf_status_t diag_pd_del_vlan_decap(bf_dev_id_t dev_id,
                                   p4_pd_entry_hdl_t entry_hdl) {
#if defined(DIAG_ADVANCED_FEATURES)
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  DIAG_PRINT("vlan-decap-del: \n");

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Make the PD call */
  status = p4_pd_diag_vlan_decap_table_delete(sess_hdl, dev_id, entry_hdl);

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Populate entries in tables to increase power usage */
bf_status_t diag_pd_power_populate_entries_in_tables(bf_dev_id_t dev_id) {
#ifdef DIAG_POWER_ENABLE
  int i = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_status_t status = 0;
  bool add_exm_entries = true, add_tcam_entries = true;
  bool check_size = true;

  DIAG_PRINT("DIAG: Started populating tables with entries \n");

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  if (add_exm_entries) {
    DIAG_PRINT("DIAG: Adding exm entries to existing tables\n");

    /* Start new session */
    status = p4_pd_client_init(&sess_hdl);
    if (status != 0) {
      DIAG_PRINT("Client init failed ");
      return status;
    }

    p4_pd_begin_batch(sess_hdl);
    /* Make sure key sizes are same before doing memcpy */
    check_size = true;

    for (i = 1; i <= DIAG_EXM_ENTRIES_MAX; i++) {
      p4_pd_diag_tbl_stage6_match_spec_t match_tbl_stage;
      match_tbl_stage.l2_metadata_inter_stage = i;
      match_tbl_stage.l2_metadata_inter_stage_dummy = 0xff;

      p4_pd_diag_tbl_stage6_match_spec_t match_tbl_stage6;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_tbl_stage6) == sizeof(match_tbl_stage));
      }
      memcpy(&match_tbl_stage6, &match_tbl_stage, sizeof(match_tbl_stage6));
      p4_pd_diag_tbl_stage6_table_add_with_nop(
          sess_hdl, dev_tgt, &match_tbl_stage6, &entry_hdl);

      p4_pd_diag_tbl_stage8_match_spec_t match_tbl_stage8;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_tbl_stage8) == sizeof(match_tbl_stage));
      }
      memcpy(&match_tbl_stage8, &match_tbl_stage, sizeof(match_tbl_stage8));
      p4_pd_diag_tbl_stage8_table_add_with_nop(
          sess_hdl, dev_tgt, &match_tbl_stage8, &entry_hdl);

      p4_pd_diag_tbl_stage10_match_spec_t match_tbl_stage10;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_tbl_stage10) == sizeof(match_tbl_stage));
      }
      memcpy(&match_tbl_stage10, &match_tbl_stage, sizeof(match_tbl_stage10));
      p4_pd_diag_tbl_stage10_table_add_with_nop(
          sess_hdl, dev_tgt, &match_tbl_stage10, &entry_hdl);

      check_size = false;
    }
    p4_pd_end_batch(sess_hdl, false);
    p4_pd_complete_operations(sess_hdl);

    p4_pd_client_cleanup(sess_hdl);

    DIAG_PRINT("DIAG: Adding exm entries to extra tables\n");

    /* Start new session */
    status = p4_pd_client_init(&sess_hdl);
    if (status != 0) {
      DIAG_PRINT("Client init failed ");
      return status;
    }
    p4_pd_begin_batch(sess_hdl);
    /* Make sure key sizes are same before doing memcpy */
    check_size = true;

    for (i = 1; i <= DIAG_EXM_ENTRIES_MAX; i++) {
#ifdef DIAG_POWER_ENABLE
      p4_pd_diag_exm_table6_0_match_spec_t match_spec;
      memset(&match_spec, 0xff, sizeof(match_spec));
      match_spec.ethernet_etherType = i;
      if (match_spec.ethernet_etherType == 0x800) {
        match_spec.ethernet_etherType = 10;
      }
      match_spec.ethernet_dstAddr[0] = i & 0xff;
      match_spec.ethernet_dstAddr[1] = (i >> 8) & 0xff;
      match_spec.ethernet_dstAddr[2] = (i >> 16) & 0xff;
      match_spec.ethernet_dstAddr[3] = (i >> 24) & 0xff;
      match_spec.ethernet_srcAddr[0] = i & 0xff;
      match_spec.l2_metadata_dummy_exm_key[0] = i & 0xff;
      match_spec.l2_metadata_dummy_exm_key[1] = (i >> 8) & 0xff;

      /* populate all exm tables, match specs in all tables have same fields
         and size
      */
      p4_pd_diag_exm_table6_0_match_spec_t match_spec6_0;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec6_0));
      }
      memcpy(&match_spec6_0, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table6_0_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec6_0, &entry_hdl);

      p4_pd_diag_exm_table7_0_match_spec_t match_spec7_0;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec7_0));
      }
      memcpy(&match_spec7_0, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table7_0_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec7_0, &entry_hdl);

      p4_pd_diag_exm_table8_0_match_spec_t match_spec8_0;
      if (check_size) {
      }
      DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec8_0));
      memcpy(&match_spec8_0, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table8_0_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec8_0, &entry_hdl);

      p4_pd_diag_exm_table9_0_match_spec_t match_spec9_0;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec9_0));
      }
      memcpy(&match_spec9_0, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table9_0_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec9_0, &entry_hdl);

      p4_pd_diag_exm_table10_0_match_spec_t match_spec10_0;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec10_0));
      }
      memcpy(&match_spec10_0, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table10_0_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec10_0, &entry_hdl);

#ifdef DIAG_POWER_MAX_ENABLE
      p4_pd_diag_exm_table4_1_match_spec_t match_spec4_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec4_1));
      }
      memcpy(&match_spec4_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table4_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec4_1, &entry_hdl);

      p4_pd_diag_exm_table5_1_match_spec_t match_spec5_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec5_1));
      }
      memcpy(&match_spec5_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table5_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec5_1, &entry_hdl);

      p4_pd_diag_exm_table6_1_match_spec_t match_spec6_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec6_1));
      }
      memcpy(&match_spec6_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table6_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec6_1, &entry_hdl);

      p4_pd_diag_exm_table7_1_match_spec_t match_spec7_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec7_1));
      }
      memcpy(&match_spec7_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table7_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec7_1, &entry_hdl);

      p4_pd_diag_exm_table8_1_match_spec_t match_spec8_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec8_1));
      }
      memcpy(&match_spec8_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table8_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec8_1, &entry_hdl);

      p4_pd_diag_exm_table9_1_match_spec_t match_spec9_1;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec9_1));
      }
      memcpy(&match_spec9_1, &match_spec, sizeof(match_spec));
      p4_pd_diag_exm_table9_1_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec9_1, &entry_hdl);
#endif
#endif
      check_size = false;
    }
    p4_pd_end_batch(sess_hdl, false);
    p4_pd_complete_operations(sess_hdl);

    p4_pd_client_cleanup(sess_hdl);
  }

  if (add_tcam_entries) {
    int stage_pri = 200;

    DIAG_PRINT("DIAG: Adding tcam entries \n");

    /* Start new session */
    status = p4_pd_client_init(&sess_hdl);
    if (status != 0) {
      DIAG_PRINT("Client init failed ");
      return status;
    }
    p4_pd_begin_batch(sess_hdl);
    check_size = true;

    for (i = 1; i <= DIAG_TCAM_ENTRIES_MAX; i++) {
      p4_pd_diag_tbl_stage1_match_spec_t match_spec;
      memset(&match_spec, 0, sizeof(match_spec));

      match_spec.l2_metadata_inter_stage = i + 50;
      match_spec.l2_metadata_inter_stage_mask = 0xffffffff;
      match_spec.l2_metadata_inter_stage_dummy = i % 200;
      match_spec.l2_metadata_inter_stage_dummy_mask = 0xff;

      p4_pd_diag_tbl_stage1_table_add_with_set_meta(
          sess_hdl, dev_tgt, &match_spec, stage_pri, &entry_hdl);

#if 0
      p4_pd_diag_tbl_stage3_match_spec_t match_spec_3;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec_3));
      }
      memcpy(&match_spec_3, &match_spec, sizeof(match_spec));
      p4_pd_diag_tbl_stage3_table_add_with_set_meta(
          sess_hdl, dev_tgt, &match_spec_3, stage_pri, &entry_hdl);
#endif

      p4_pd_diag_tbl_stage5_match_spec_t match_spec_5;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec_5));
      }
      memcpy(&match_spec_5, &match_spec, sizeof(match_spec));
      p4_pd_diag_tbl_stage5_table_add_with_set_meta(
          sess_hdl, dev_tgt, &match_spec_5, stage_pri, &entry_hdl);

      p4_pd_diag_tbl_stage7_match_spec_t match_spec_7;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec_7));
      }
      memcpy(&match_spec_7, &match_spec, sizeof(match_spec));
      p4_pd_diag_tbl_stage7_table_add_with_set_meta(
          sess_hdl, dev_tgt, &match_spec_7, stage_pri, &entry_hdl);

      p4_pd_diag_tbl_stage9_match_spec_t match_spec_9;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec) == sizeof(match_spec_9));
      }
      memcpy(&match_spec_9, &match_spec, sizeof(match_spec));
      p4_pd_diag_tbl_stage9_table_add_with_set_meta(
          sess_hdl, dev_tgt, &match_spec_9, stage_pri, &entry_hdl);

#ifdef DIAG_POWER_ENABLE
      p4_pd_diag_tbl_tcam_5_ig_match_spec_t match_spec_tcam;
      memset(&match_spec_tcam, 0, sizeof(match_spec_tcam));
      match_spec_tcam.l2_metadata_dummy_tcam_key[0] = i & 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key[1] = (i >> 8) & 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key[2] = (i >> 16) & 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key[3] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key[4] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key_mask[0] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key_mask[1] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key_mask[2] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key_mask[3] = 0xff;
      match_spec_tcam.l2_metadata_dummy_tcam_key_mask[4] = 0xff;

      /* Extra tcam tables */
      p4_pd_diag_tbl_tcam_5_ig_match_spec_t match_spec_tcam_5;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_5));
      }
      memcpy(&match_spec_tcam_5, &match_spec_tcam, sizeof(match_spec_tcam));
      p4_pd_diag_tbl_tcam_5_ig_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec_tcam_5, stage_pri, &entry_hdl);

      p4_pd_diag_tbl_tcam_7_ig_match_spec_t match_spec_tcam_7;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_7));
      }
      memcpy(&match_spec_tcam_7, &match_spec_tcam, sizeof(match_spec_tcam));
      p4_pd_diag_tbl_tcam_7_ig_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec_tcam_7, stage_pri, &entry_hdl);

      p4_pd_diag_tbl_tcam_9_ig_match_spec_t match_spec_tcam_9;
      if (check_size) {
        DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_9));
      }
      memcpy(&match_spec_tcam_9, &match_spec_tcam, sizeof(match_spec_tcam));
      p4_pd_diag_tbl_tcam_9_ig_table_add_with_do_nothing(
          sess_hdl, dev_tgt, &match_spec_tcam_9, stage_pri, &entry_hdl);

#endif
      check_size = false;
    }
    p4_pd_end_batch(sess_hdl, false);
    p4_pd_complete_operations(sess_hdl);

    p4_pd_client_cleanup(sess_hdl);
  }

  DIAG_PRINT("DIAG: Finished populating tables with entries \n");
  return status;
#else
  return BF_SUCCESS;
#endif
}

bf_status_t diag_pd_phv_power_populate_entries_in_tables(bf_dev_id_t dev_id) {
#ifdef DIAG_PARDE_STRESS_POWER
  int i = 0;
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_status_t status = 0;
  bool check_size = true;

  DIAG_PRINT("DIAG: Started populating tables with entries \n");

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;
  int stage_pri = 200;
  DIAG_PRINT("DIAG: Adding tcam entries \n");

  /* Start new session */
  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  p4_pd_begin_batch(sess_hdl);
  check_size = true;

  for (i = 1; i <= DIAG_TCAM_ENTRIES_MAX; i++) {
    p4_pd_diag_tbl_tcam_2_eg_match_spec_t match_spec_tcam;
    memset(&match_spec_tcam, 0, sizeof(match_spec_tcam));
    match_spec_tcam.phv_stress_hdr_g4 = i & 0xffffffff;
    match_spec_tcam.phv_stress_hdr_g4_mask = 0xffffffff;

    /* Extra tcam tables */
    p4_pd_diag_tbl_tcam_2_eg_match_spec_t match_spec_tcam_2;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_2));
    }
    memcpy(&match_spec_tcam_2, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_2_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_2, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_3_eg_match_spec_t match_spec_tcam_3;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_3));
    }
    memcpy(&match_spec_tcam_3, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_3_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_3, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_4_eg_match_spec_t match_spec_tcam_4;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_4));
    }
    memcpy(&match_spec_tcam_4, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_4_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_4, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_5_eg_match_spec_t match_spec_tcam_5;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_5));
    }
    memcpy(&match_spec_tcam_5, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_5_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_5, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_6_eg_match_spec_t match_spec_tcam_6;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_6));
    }
    memcpy(&match_spec_tcam_6, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_6_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_6, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_7_eg_match_spec_t match_spec_tcam_7;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_7));
    }
    memcpy(&match_spec_tcam_7, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_7_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_7, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_8_eg_match_spec_t match_spec_tcam_8;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_8));
    }
    memcpy(&match_spec_tcam_8, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_8_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_8, stage_pri, &entry_hdl);

    p4_pd_diag_tbl_tcam_9_eg_match_spec_t match_spec_tcam_9;
    if (check_size) {
      DIAG_ASSERT(sizeof(match_spec_tcam) == sizeof(match_spec_tcam_9));
    }
    memcpy(&match_spec_tcam_9, &match_spec_tcam, sizeof(match_spec_tcam));
    p4_pd_diag_tbl_tcam_9_eg_table_add_with_do_nothing(
        sess_hdl, dev_tgt, &match_spec_tcam_9, stage_pri, &entry_hdl);

    check_size = false;
  }
  p4_pd_end_batch(sess_hdl, false);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);
  DIAG_PRINT("DIAG: Finished populating tables with entries \n");
  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Populate entries in tables to stress mau */
bf_status_t diag_pd_mau_bus_stress_populate_entries_in_tables(
    bf_dev_id_t dev_id) {
#ifdef DIAG_MAU_BUS_STRESS_ENABLE
  p4_pd_dev_target_t dev_tgt;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_entry_hdl_t entry_hdl;
  p4_pd_status_t status = 0;
  int stage_pri = 200;
  bool ingress = true, egress = true;

  DIAG_PRINT("DIAG: Started populating mau stress tables with entries \n");

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL;

  DIAG_PRINT("DIAG: Adding exm entries to existing tables\n");

  /* Start new session */
  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  p4_pd_begin_batch(sess_hdl);

  if (ingress) {
    p4_pd_diag_set_mau_exm_key_ig_action_spec_t action_exm;
    p4_pd_diag_set_mau_tcam_key_ig_action_spec_t action_tcam;
    uint16_t exm_cntr = 0, tcam_cntr = 0;

    p4_pd_diag_mau_exm_table1_ig_match_spec_t match1_e;
    memset(&match1_e, 0, sizeof(match1_e));
    match1_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table1_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match1_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table1_ig_match_spec_t match1_t;
    memset(&match1_t, 0, sizeof(match1_t));
    match1_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match1_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table1_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match1_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table2_ig_match_spec_t match2_e;
    memset(&match2_e, 0xff, sizeof(match2_e));
    match2_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table2_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match2_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table2_ig_match_spec_t match2_t;
    memset(&match2_t, 0xff, sizeof(match2_t));
    match2_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match2_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table2_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match2_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table3_ig_match_spec_t match3_e;
    memset(&match3_e, 0, sizeof(match3_e));
    match3_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table3_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match3_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table3_ig_match_spec_t match3_t;
    memset(&match3_t, 0, sizeof(match3_t));
    match3_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match3_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table3_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match3_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table4_ig_match_spec_t match4_e;
    memset(&match4_e, 0xff, sizeof(match4_e));
    match4_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table4_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match4_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table4_ig_match_spec_t match4_t;
    memset(&match4_t, 0xff, sizeof(match4_t));
    match4_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match4_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table4_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match4_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

#if defined(TOFINO2H)
    p4_pd_diag_mau_exm_table5_ig_match_spec_t match5_e;
    memset(&match5_e, 0, sizeof(match5_e));
    match5_e.l2_metadata_mau_exm_cntr = exm_cntr;
    match5_e.l2_metadata_mau_tcam_cntr = tcam_cntr;
    p4_pd_diag_mau_exm_table5_ig_table_add_with_mau_bus_stress_cntr_inc_ig(
        sess_hdl, dev_tgt, &match5_e, &entry_hdl);
    exm_cntr++;
    tcam_cntr++;

#else
    p4_pd_diag_mau_exm_table5_ig_match_spec_t match5_e;
    memset(&match5_e, 0, sizeof(match5_e));
    match5_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table5_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match5_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table5_ig_match_spec_t match5_t;
    memset(&match5_t, 0, sizeof(match5_t));
    match5_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match5_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table5_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match5_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table6_ig_match_spec_t match6_e;
    memset(&match6_e, 0xff, sizeof(match6_e));
    match6_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table6_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match6_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table6_ig_match_spec_t match6_t;
    memset(&match6_t, 0xff, sizeof(match6_t));
    match6_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match6_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table6_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match6_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table7_ig_match_spec_t match7_e;
    memset(&match7_e, 0, sizeof(match7_e));
    match7_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table7_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match7_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table7_ig_match_spec_t match7_t;
    memset(&match7_t, 0, sizeof(match7_t));
    match7_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match7_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table7_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match7_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table8_ig_match_spec_t match8_e;
    memset(&match8_e, 0xff, sizeof(match8_e));
    match8_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table8_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match8_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table8_ig_match_spec_t match8_t;
    memset(&match8_t, 0xff, sizeof(match8_t));
    match8_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match8_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table8_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match8_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table9_ig_match_spec_t match9_e;
    memset(&match9_e, 0, sizeof(match9_e));
    match9_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table9_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match9_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table9_ig_match_spec_t match9_t;
    memset(&match9_t, 0, sizeof(match9_t));
    match9_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match9_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table9_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match9_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table10_ig_match_spec_t match10_e;
    memset(&match10_e, 0xff, sizeof(match10_e));
    match10_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table10_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match10_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table10_ig_match_spec_t match10_t;
    memset(&match10_t, 0xff, sizeof(match10_t));
    match10_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match10_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table10_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match10_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

#if defined(TOFINO1) || defined(TOFINO2M)
    /* tofino1 and tofino2m have 12 stages */
    p4_pd_diag_mau_exm_table11_ig_match_spec_t match11_e;
    memset(&match11_e, 0, sizeof(match11_e));
    match11_e.l2_metadata_mau_exm_cntr = exm_cntr;
    match11_e.l2_metadata_mau_tcam_cntr = tcam_cntr;
    p4_pd_diag_mau_exm_table11_ig_table_add_with_mau_bus_stress_cntr_inc_ig(
        sess_hdl, dev_tgt, &match11_e, &entry_hdl);
    exm_cntr++;
    tcam_cntr++;
#else

    p4_pd_diag_mau_exm_table11_ig_match_spec_t match11_e;
    memset(&match11_e, 0, sizeof(match11_e));
    match11_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table11_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match11_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table11_ig_match_spec_t match11_t;
    memset(&match11_t, 0, sizeof(match11_t));
    match11_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match11_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table11_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match11_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table12_ig_match_spec_t match12_e;
    memset(&match12_e, 0xff, sizeof(match12_e));
    match12_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table12_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match12_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table12_ig_match_spec_t match12_t;
    memset(&match12_t, 0xff, sizeof(match12_t));
    match12_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match12_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table12_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match12_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table13_ig_match_spec_t match13_e;
    memset(&match13_e, 0, sizeof(match13_e));
    match13_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table13_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match13_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table13_ig_match_spec_t match13_t;
    memset(&match13_t, 0, sizeof(match13_t));
    match13_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match13_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table13_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match13_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table14_ig_match_spec_t match14_e;
    memset(&match14_e, 0xff, sizeof(match14_e));
    match14_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table14_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match14_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table14_ig_match_spec_t match14_t;
    memset(&match14_t, 0xff, sizeof(match14_t));
    match14_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match14_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table14_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match14_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table15_ig_match_spec_t match15_e;
    memset(&match15_e, 0, sizeof(match15_e));
    match15_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table15_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match15_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table15_ig_match_spec_t match15_t;
    memset(&match15_t, 0, sizeof(match15_t));
    match15_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match15_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table15_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match15_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table16_ig_match_spec_t match16_e;
    memset(&match16_e, 0xff, sizeof(match16_e));
    match16_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table16_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match16_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table16_ig_match_spec_t match16_t;
    memset(&match16_t, 0xff, sizeof(match16_t));
    match16_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match16_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table16_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match16_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table17_ig_match_spec_t match17_e;
    memset(&match17_e, 0, sizeof(match17_e));
    match17_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table17_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match17_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table17_ig_match_spec_t match17_t;
    memset(&match17_t, 0, sizeof(match17_t));
    match17_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match17_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table17_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match17_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table18_ig_match_spec_t match18_e;
    memset(&match18_e, 0xff, sizeof(match18_e));
    match18_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table18_ig_table_add_with_set_mau_exm_key_ig(
        sess_hdl, dev_tgt, &match18_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table18_ig_match_spec_t match18_t;
    memset(&match18_t, 0xff, sizeof(match18_t));
    match18_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match18_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table18_ig_table_add_with_set_mau_tcam_key_ig(
        sess_hdl, dev_tgt, &match18_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table19_ig_match_spec_t match19_e;
    memset(&match19_e, 0, sizeof(match19_e));
    match19_e.l2_metadata_mau_exm_cntr = exm_cntr;
    match19_e.l2_metadata_mau_tcam_cntr = tcam_cntr;
    p4_pd_diag_mau_exm_table19_ig_table_add_with_mau_bus_stress_cntr_inc_ig(
        sess_hdl, dev_tgt, &match19_e, &entry_hdl);
    exm_cntr++;
    tcam_cntr++;

#endif  // tofino1 or tofino2m
#endif  // tofino2h
  }

  if (egress) {
    p4_pd_diag_set_mau_exm_key_eg_action_spec_t action_exm;
    p4_pd_diag_set_mau_tcam_key_eg_action_spec_t action_tcam;
    uint16_t exm_cntr = 0, tcam_cntr = 0;

    p4_pd_diag_mau_exm_table1_eg_match_spec_t match1_e;
    memset(&match1_e, 0, sizeof(match1_e));
    match1_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table1_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match1_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table1_eg_match_spec_t match1_t;
    memset(&match1_t, 0, sizeof(match1_t));
    match1_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match1_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table1_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match1_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table2_eg_match_spec_t match2_e;
    memset(&match2_e, 0xff, sizeof(match2_e));
    match2_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table2_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match2_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table2_eg_match_spec_t match2_t;
    memset(&match2_t, 0xff, sizeof(match2_t));
    match2_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match2_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table2_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match2_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table3_eg_match_spec_t match3_e;
    memset(&match3_e, 0, sizeof(match3_e));
    match3_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table3_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match3_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table3_eg_match_spec_t match3_t;
    memset(&match3_t, 0, sizeof(match3_t));
    match3_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match3_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table3_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match3_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table4_eg_match_spec_t match4_e;
    memset(&match4_e, 0xff, sizeof(match4_e));
    match4_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table4_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match4_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table4_eg_match_spec_t match4_t;
    memset(&match4_t, 0xff, sizeof(match4_t));
    match4_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match4_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table4_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match4_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

#if defined(TOFINO2H) || defined(TOFINO1)
    p4_pd_diag_mau_exm_table5_eg_match_spec_t match5_e;
    memset(&match5_e, 0, sizeof(match5_e));
    match5_e.l2_metadata_mau_exm_cntr = exm_cntr;
    match5_e.l2_metadata_mau_tcam_cntr = tcam_cntr;
    p4_pd_diag_mau_exm_table5_eg_table_add_with_mau_bus_stress_cntr_inc_eg(
        sess_hdl, dev_tgt, &match5_e, &entry_hdl);
    exm_cntr++;
    tcam_cntr++;

#else
    p4_pd_diag_mau_exm_table5_eg_match_spec_t match5_e;
    memset(&match5_e, 0, sizeof(match5_e));
    match5_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table5_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match5_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table5_eg_match_spec_t match5_t;
    memset(&match5_t, 0, sizeof(match5_t));
    match5_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match5_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table5_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match5_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table6_eg_match_spec_t match6_e;
    memset(&match6_e, 0xff, sizeof(match6_e));
    match6_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table6_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match6_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table6_eg_match_spec_t match6_t;
    memset(&match6_t, 0xff, sizeof(match6_t));
    match6_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match6_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table6_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match6_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table7_eg_match_spec_t match7_e;
    memset(&match7_e, 0, sizeof(match7_e));
    match7_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0xff, sizeof(action_exm));
    p4_pd_diag_mau_exm_table7_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match7_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table7_eg_match_spec_t match7_t;
    memset(&match7_t, 0, sizeof(match7_t));
    match7_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match7_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0xff, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table7_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match7_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table8_eg_match_spec_t match8_e;
    memset(&match8_e, 0xff, sizeof(match8_e));
    match8_e.l2_metadata_mau_exm_cntr = exm_cntr;
    memset(&action_exm, 0, sizeof(action_exm));
    p4_pd_diag_mau_exm_table8_eg_table_add_with_set_mau_exm_key_eg(
        sess_hdl, dev_tgt, &match8_e, &action_exm, &entry_hdl);
    exm_cntr++;

    p4_pd_diag_mau_tcam_table8_eg_match_spec_t match8_t;
    memset(&match8_t, 0xff, sizeof(match8_t));
    match8_t.l2_metadata_mau_tcam_cntr = tcam_cntr;
    match8_t.l2_metadata_mau_tcam_cntr_mask = tcam_cntr;
    memset(&action_tcam, 0, sizeof(action_tcam));
    p4_pd_diag_mau_tcam_table8_eg_table_add_with_set_mau_tcam_key_eg(
        sess_hdl, dev_tgt, &match8_t, stage_pri, &action_tcam, &entry_hdl);
    tcam_cntr++;

    p4_pd_diag_mau_exm_table9_eg_match_spec_t match9_e;
    memset(&match9_e, 0, sizeof(match9_e));
    match9_e.l2_metadata_mau_exm_cntr = exm_cntr;
    match9_e.l2_metadata_mau_tcam_cntr = tcam_cntr;
    p4_pd_diag_mau_exm_table9_eg_table_add_with_mau_bus_stress_cntr_inc_eg(
        sess_hdl, dev_tgt, &match9_e, &entry_hdl);
    exm_cntr++;
    tcam_cntr++;
#endif  // tofino2h
  }

  p4_pd_end_batch(sess_hdl, false);
  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  DIAG_PRINT("DIAG: Finished populating mau stress tables with entries \n");
  return status;
#else
  return BF_SUCCESS;
#endif
}

bf_status_t diag_pd_learning_timeout_set(bf_dev_id_t dev_id,
                                         uint32_t timeout_usec) {
#if defined(DIAG_ADVANCED_FEATURES)
  bf_status_t status = BF_SUCCESS;
  p4_pd_sess_hdl_t sess_hdl = 0;

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  DIAG_DEV_INFO(dev_id)->lrn_timeout = timeout_usec;
  /* Set learning timeout */
  status = p4_pd_diag_set_learning_timeout(
      sess_hdl, dev_id, DIAG_DEV_INFO(dev_id)->lrn_timeout);
  p4_pd_complete_operations(sess_hdl);
  p4_pd_client_cleanup(sess_hdl);
  return status;
#else
  return BF_SUCCESS;
#endif
}

/* Create Multicast group */
bf_status_t diag_pd_mc_mgrp_create(bf_dev_id_t dev_id,
                                   bf_mc_grp_id_t grp_id,
                                   bf_mc_mgrp_hdl_t *mgrp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_mgrp_create(sess_hdl, dev_id, grp_id, mgrp_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mgrp create failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Delete multicast group */
bf_status_t diag_pd_mc_mgrp_destroy(bf_dev_id_t dev_id,
                                    bf_mc_mgrp_hdl_t mgrp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_mgrp_destroy(sess_hdl, dev_id, mgrp_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mgrp destroy failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Add ports to multicast group */
bf_status_t diag_pd_mc_mgrp_ports_add(bf_dev_id_t dev_id,
                                      bf_mc_port_map_t port_map,
                                      bf_mc_lag_map_t lag_map,
                                      bf_mc_rid_t rid,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_node_hdl_t *node_hdl) {
  bf_mc_session_hdl_t sess_hdl = 0;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status =
      bf_mc_node_create(sess_hdl, dev_id, rid, port_map, lag_map, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node create failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status =
      bf_mc_associate_node(sess_hdl, dev_id, mgrp_hdl, *node_hdl, false, 0);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node associate failed\n");
    bf_mc_node_destroy(sess_hdl, dev_id, *node_hdl);
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Delete ports from multicast group */
bf_status_t diag_pd_mc_mgrp_ports_del(bf_dev_id_t dev_id,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_node_hdl_t node_hdl) {
  bf_mc_session_hdl_t sess_hdl = 0;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_dissociate_node(sess_hdl, dev_id, mgrp_hdl, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node Dissociate failed\n");
  }

  status = bf_mc_node_destroy(sess_hdl, dev_id, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node destroy failed\n");
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
  }

  return status;
}

/* Create Multicast ecmp */
bf_status_t diag_pd_mc_ecmp_create(bf_dev_id_t dev_id,
                                   bf_mc_ecmp_hdl_t *ecmp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_ecmp_create(sess_hdl, dev_id, ecmp_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("ecmp create failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Destroy Multicast ecmp group */
bf_status_t diag_pd_mc_ecmp_destory(bf_dev_id_t dev_id,
                                    bf_mc_ecmp_hdl_t ecmp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_ecmp_destroy(sess_hdl, dev_id, ecmp_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("ecmp destroy failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Associate ecmp to multicast-group */
bf_status_t diag_pd_mc_associate_ecmp(bf_dev_id_t dev_id,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_ecmp_hdl_t ecmp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_associate_ecmp(sess_hdl, dev_id, mgrp_hdl, ecmp_hdl, false, 0);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Associate ecmp failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Dissociate ecmp from multicast-group */
bf_status_t diag_pd_mc_dissociate_ecmp(bf_dev_id_t dev_id,
                                       bf_mc_mgrp_hdl_t mgrp_hdl,
                                       bf_mc_ecmp_hdl_t ecmp_hdl) {
  bf_mc_session_hdl_t sess_hdl;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_dissociate_ecmp(sess_hdl, dev_id, mgrp_hdl, ecmp_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Dissociate ecmp failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Add ports to ecmp group */
bf_status_t diag_pd_mc_ecmp_ports_add(bf_dev_id_t dev_id,
                                      bf_mc_port_map_t port_map,
                                      bf_mc_lag_map_t lag_map,
                                      bf_mc_rid_t rid,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bf_mc_node_hdl_t *node_hdl) {
  bf_mc_session_hdl_t sess_hdl = 0;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status =
      bf_mc_node_create(sess_hdl, dev_id, rid, port_map, lag_map, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node create failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_ecmp_mbr_add(sess_hdl, dev_id, ecmp_hdl, *node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc ecmp mbr add failed\n");
    bf_mc_node_destroy(sess_hdl, dev_id, *node_hdl);
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Delete ports from ecmp */
bf_status_t diag_pd_mc_ecmp_ports_del(bf_dev_id_t dev_id,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bf_mc_node_hdl_t node_hdl) {
  bf_mc_session_hdl_t sess_hdl = 0;
  bf_status_t status = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_ecmp_mbr_rem(sess_hdl, dev_id, ecmp_hdl, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc ecmp mbr remove failed\n");
  }

  status = bf_mc_node_destroy(sess_hdl, dev_id, node_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc node destroy failed\n");
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
  }

  return status;
}

/* Add ports to LAG */
bf_status_t diag_pd_mc_set_lag_membership(bf_dev_id_t dev_id,
                                          uint32_t lag_id,
                                          bf_mc_port_map_t port_map) {
  bf_mc_session_hdl_t sess_hdl = 0;
  bf_status_t status = 0;
  bf_mc_rid_t rid = 0;

  status = bf_mc_create_session(&sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Create session failed\n");
    return status;
  }

  status = bf_mc_set_lag_membership(sess_hdl, dev_id, lag_id, port_map);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Mc lag memebrship set failed\n");
    bf_mc_destroy_session(sess_hdl);
    return status;
  }

  status = bf_mc_complete_operations(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("MC complete operations failed\n");
  }

  status = bf_mc_destroy_session(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Destroy session failed\n");
    return status;
  }

  return status;
}

/* Stream setup */
bf_status_t diag_pd_stream_setup(bf_dev_id_t dev_id,
                                 bf_dev_pipe_t pipe,
                                 uint32_t app_id,
                                 uint32_t pkt_size,
                                 uint32_t pkt_buf_offset,
                                 bf_dev_port_t pktgen_port,
                                 bf_dev_port_t src_port,
                                 uint32_t num_pkts,
                                 uint32_t timer_nsec) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;
  uint8_t pkt_buf[DIAG_MAX_PKT_SIZE];

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  status = p4_pd_pktgen_enable(sess_hdl, dev_id, pktgen_port);
  if (status != 0) {
    DIAG_PRINT("Pgen enable failed for port %d", pktgen_port);
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  status = p4_pd_pktgen_app_disable(sess_hdl, dev_tgt, app_id);
  if (status != 0) {
    DIAG_PRINT("Pgen app disable failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  if (diag_is_chip_family_tofino1(dev_id)) {
    p4_pd_pktgen_app_cfg pgen_cfg;
    memset(&pgen_cfg, 0, sizeof(pgen_cfg));

    pgen_cfg.trigger_type = PD_PKTGEN_TRIGGER_TIMER_PERIODIC;
    pgen_cfg.batch_count = 0;
    pgen_cfg.packets_per_batch = num_pkts;
    pgen_cfg.pattern_value = 0;
    pgen_cfg.pattern_mask = 0;
    pgen_cfg.timer_nanosec = timer_nsec;
    pgen_cfg.ibg = 0;
    pgen_cfg.ibg_jitter = 0;
    pgen_cfg.ipg = 0;
    pgen_cfg.ipg_jitter = 0;
    pgen_cfg.source_port = src_port;
    pgen_cfg.increment_source_port = false;
    pgen_cfg.pkt_buffer_offset = pkt_buf_offset;
    pgen_cfg.length = pkt_size;

    status = p4_pd_pktgen_cfg_app(sess_hdl, dev_tgt, app_id, pgen_cfg);
  } else {
    p4_pd_pktgen_app_cfg_tof2 pgen_cfg;
    memset(&pgen_cfg, 0, sizeof(pgen_cfg));

    pgen_cfg.trigger_type = PD_PKTGEN_TRIGGER_TIMER_PERIODIC;
    pgen_cfg.batch_count = 0;
    pgen_cfg.packets_per_batch = num_pkts;
    pgen_cfg.timer_nanosec = timer_nsec;
    pgen_cfg.source_port = src_port;
    /* Channel-id is in range 0 - 7 */
    pgen_cfg.assigned_chnl_id = pktgen_port % 8;
    pgen_cfg.increment_source_port = false;
    pgen_cfg.pkt_buffer_offset = pkt_buf_offset;
    pgen_cfg.length = pkt_size;

    status = p4_pd_pktgen_cfg_app_tof2(sess_hdl, dev_tgt, app_id, pgen_cfg);
  }
  if (status != 0) {
    DIAG_PRINT("Pgen app cfg failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  memset(pkt_buf, 0, sizeof(pkt_buf));
  status = p4_pd_pktgen_write_pkt_buffer(
      sess_hdl, dev_tgt, 0, pkt_size, &pkt_buf[0]);
  if (status != 0) {
    DIAG_PRINT("Pgen write buffer failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  status = p4_pd_pktgen_set_pkt_counter(sess_hdl, dev_tgt, app_id, 0);
  if (status != 0) {
    DIAG_PRINT("Pgen set counter failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Stream start */
bf_status_t diag_pd_stream_start(bf_dev_id_t dev_id,
                                 bf_dev_pipe_t pipe,
                                 uint32_t app_id,
                                 uint32_t pkt_size,
                                 uint8_t *pkt_buf,
                                 uint32_t pkt_buf_offset) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  status = p4_pd_pktgen_write_pkt_buffer(
      sess_hdl, dev_tgt, pkt_buf_offset, pkt_size, pkt_buf);
  if (status != 0) {
    DIAG_PRINT("Pgen write buffer failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  status = p4_pd_pktgen_app_enable(sess_hdl, dev_tgt, app_id);
  if (status != 0) {
    DIAG_PRINT("Pgen app enable failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Stream adjust */
bf_status_t diag_pd_stream_adjust(bf_dev_id_t dev_id,
                                  bf_dev_pipe_t pipe,
                                  uint32_t app_id,
                                  uint32_t pkt_size,
                                  uint32_t pkt_buf_offset,
                                  bf_dev_port_t pktgen_port,
                                  bf_dev_port_t src_port,
                                  uint32_t num_pkts,
                                  uint32_t timer_nsec,
                                  uint8_t *pkt_buf,
                                  bool enabled) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  status = p4_pd_pktgen_app_disable(sess_hdl, dev_tgt, app_id);
  if (status != 0) {
    DIAG_PRINT("Pgen app disable failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  /* Sleep for sometime for packets to clear */
  bf_sys_usleep(2000000);

  if (diag_is_chip_family_tofino1(dev_id)) {
    p4_pd_pktgen_app_cfg pgen_cfg;
    memset(&pgen_cfg, 0, sizeof(pgen_cfg));

    pgen_cfg.trigger_type = PD_PKTGEN_TRIGGER_TIMER_PERIODIC;
    pgen_cfg.batch_count = 0;
    pgen_cfg.packets_per_batch = num_pkts;
    pgen_cfg.pattern_value = 0;
    pgen_cfg.pattern_mask = 0;
    pgen_cfg.timer_nanosec = timer_nsec;
    pgen_cfg.ibg = 0;
    pgen_cfg.ibg_jitter = 0;
    pgen_cfg.ipg = 0;
    pgen_cfg.ipg_jitter = 0;
    pgen_cfg.source_port = src_port;
    pgen_cfg.increment_source_port = false;
    pgen_cfg.pkt_buffer_offset = pkt_buf_offset;
    pgen_cfg.length = pkt_size;

    status = p4_pd_pktgen_cfg_app(sess_hdl, dev_tgt, app_id, pgen_cfg);
  } else {
    p4_pd_pktgen_app_cfg_tof2 pgen_cfg;
    memset(&pgen_cfg, 0, sizeof(pgen_cfg));

    pgen_cfg.trigger_type = PD_PKTGEN_TRIGGER_TIMER_PERIODIC;
    pgen_cfg.batch_count = 0;
    pgen_cfg.packets_per_batch = num_pkts;
    pgen_cfg.timer_nanosec = timer_nsec;
    pgen_cfg.source_port = src_port;
    /* Channel-id is in range 0 - 7 */
    pgen_cfg.assigned_chnl_id = pktgen_port % 8;
    pgen_cfg.increment_source_port = false;
    pgen_cfg.pkt_buffer_offset = pkt_buf_offset;
    pgen_cfg.length = pkt_size;

    status = p4_pd_pktgen_cfg_app_tof2(sess_hdl, dev_tgt, app_id, pgen_cfg);
  }
  if (status != 0) {
    DIAG_PRINT("Pgen app cfg failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  status = p4_pd_pktgen_write_pkt_buffer(
      sess_hdl, dev_tgt, pkt_buf_offset, pkt_size, pkt_buf);
  if (status != 0) {
    DIAG_PRINT("Pgen write buffer failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  /* Enable stream again if it was enabled before */
  if (enabled) {
    status = p4_pd_pktgen_app_enable(sess_hdl, dev_tgt, app_id);
    if (status != 0) {
      DIAG_PRINT("Pgen app enable failed ");
      p4_pd_client_cleanup(sess_hdl);
      return status;
    }
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Stream stop */
bf_status_t diag_pd_stream_stop(bf_dev_id_t dev_id,
                                bf_dev_pipe_t pipe,
                                uint32_t app_id) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  status = p4_pd_pktgen_app_disable(sess_hdl, dev_tgt, app_id);
  if (status != 0) {
    DIAG_PRINT("Pgen app disable failed ");
    p4_pd_client_cleanup(sess_hdl);
    return status;
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Stream counter get */
bf_status_t diag_pd_stream_counter_get(bf_dev_id_t dev_id,
                                       bf_dev_pipe_t pipe,
                                       uint32_t app_id,
                                       uint64_t *cntr_val) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;
  p4_pd_dev_target_t dev_tgt;

  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  status = p4_pd_pktgen_get_pkt_counter(sess_hdl, dev_tgt, app_id, cntr_val);
  if (status != 0) {
    DIAG_PRINT("Pgen get counter failed ");
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Stream cleanup */
bf_status_t diag_pd_stream_cleanup(bf_dev_id_t dev_id,
                                   bf_dev_port_t pktgen_port,
                                   bool disable_port) {
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_status_t status = 0;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }

  /* Do not disable as other sessions might still exist */
  if (disable_port) {
    status = p4_pd_pktgen_disable(sess_hdl, dev_id, pktgen_port);
    if (status != 0) {
      DIAG_PRINT("Pgen disable failed ");
      p4_pd_client_cleanup(sess_hdl);
      return status;
    }
  }

  p4_pd_complete_operations(sess_hdl);

  p4_pd_client_cleanup(sess_hdl);

  return BF_SUCCESS;
}

/* Program entire GFM with a test pattern. */
bf_status_t diag_pd_gfm_pattern(bf_dev_id_t dev_id,
                                int num_patterns,
                                uint64_t *row_patterns,
                                uint64_t *bad_parity_rows) {
  p4_pd_status_t status = 0;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt = {.device_id = dev_id,
                                .dev_pipe_id = BF_DEV_PIPE_ALL};
  uint32_t api_flags = PIPE_FLAG_SYNC_REQ;
  bf_dev_direction_t gress = BF_DEV_DIR_EGRESS;
  dev_stage_t stage = 0xFF;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  status = p4_pd_gfm_test_pattern_set(sess_hdl,
                                      dev_tgt,
                                      api_flags,
                                      gress,
                                      stage,
                                      num_patterns,
                                      row_patterns,
                                      bad_parity_rows);
  p4_pd_complete_operations(sess_hdl);
  p4_pd_client_cleanup(sess_hdl);
  return status;
}

/* Program a single GFM column with a test pattern. */
bf_status_t diag_pd_gfm_col(bf_dev_id_t dev_id,
                            int column,
                            uint16_t col_data[64]) {
  p4_pd_status_t status = 0;
  p4_pd_sess_hdl_t sess_hdl = 0;
  p4_pd_dev_target_t dev_tgt = {.device_id = dev_id,
                                .dev_pipe_id = BF_DEV_PIPE_ALL};
  uint32_t api_flags = PIPE_FLAG_SYNC_REQ;
  bf_dev_direction_t gress = BF_DEV_DIR_EGRESS;
  dev_stage_t stage = 0xFF;

  status = p4_pd_client_init(&sess_hdl);
  if (status != 0) {
    DIAG_PRINT("Client init failed ");
    return status;
  }
  status = p4_pd_gfm_test_col_set(
      sess_hdl, dev_tgt, api_flags, gress, stage, column, col_data);
  p4_pd_complete_operations(sess_hdl);
  p4_pd_client_cleanup(sess_hdl);
  return status;
}
