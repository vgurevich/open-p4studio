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


/*
 *    This file contains all data strcutures and parameters
 *    related to port
 */

#ifndef __TM_PORT_H__
#define __TM_PORT_H__

#include <stdint.h>
#include <stdbool.h>
#include "tm_ig_ppg.h"
#include "tm_ctx.h"

/* Section 1 : Asic Type/version independent TM data structure  */

typedef struct _bf_tm_port {
  // offset_idx (hyst value index into table)
  uint8_t p_pipe;      // Physical pipe ID (logical-pipe is mapped
                       // to physical pipe# and stored here)
  uint8_t l_pipe;      // logical pipe
  uint8_t d_pipe;      // Die Local pipe_id for Multi-Die CB
  uint8_t port;        // TM Pipe local port
  uint8_t uport;       // API level port which could be same as port or diferent
                       // based on the
                       // Tofino Chip Family
  uint8_t pg;          // Port Group
  bool mirror_port;    // TODO: not initialized !
  bool offline;        // when port is deleted/deconfigured/removed,
                       // offline is TRUE
  bool status;         // TRUE when port is up, FALSE when port is down
  bool admin_state;    // TRUE when port is enabled, otherwise FALSE
  bool qac_rx_enable;  // TRUE when qac_rx can be enabled, otherwise FALSE
  bool recirc_enable;  // TRUE when recirculation is enabled on the Port

  uint8_t ppg_count;  // Count of PPGs allocated to the port
  bf_tm_ppg_t *ppg_list[BF_TM_MAX_PFC_LEVELS];  // Keeps track of ppgs
                                                // allocated for the port.
  bf_tm_ppg_t *ppgs[BF_TM_MAX_PFC_LEVELS];      // From allocated PPG list
                                                // this array maintains
                                                // mapping of ppg to icos.
  bf_tm_thres_t wac_drop_limit;                 // Drop limit in cells (WAC)
  bf_tm_thres_t qac_drop_limit;                 // Drop limit in cells (QAC)
  bf_tm_thres_t wac_resume_limit;               // Hysteresis in cells (WAC)
  bf_tm_thres_t qac_resume_limit;               // Hysteresis in cells (QAC)
  uint8_t wac_hyst_index;              // Index that maps to WAC offset-profile.
  uint8_t qac_hyst_index;              // Index that maps to QAC offset-profile.
  bf_tm_thres_t uc_cut_through_limit;  // cells for CT in cells
  bool ct_enabled;                     // Enable/Disable CT
  bf_tm_thres_t skid_limit;            // Skid limit in cells

  uint8_t qid_profile;  // Queue's profile index. UINT8_MAX if not assigned.

  uint32_t burst_size;  // in bytes or pps
  uint32_t port_rate;   // in kbps or pps
  bool pps;
  bool sch_enabled;
  bool max_rate_enabled;  // Enable/Disable shaping
  bool tdm;               // needed ??
  uint8_t credit;

  bf_port_speeds_t speed;         // Port current speed for scheduling
  bf_port_speeds_t speed_on_add;  // Port initial speed set on port_add
  bool has_mac;

  uint8_t cos_to_icos[BF_TM_MAX_COS_LEVELS];  // CoS to iCoS mapping
  uint8_t icos_to_cos_mask;        // Reverse mapping used for PFC generation.
  bf_tm_flow_ctrl_type_t fc_type;  // Pause, PFC, or None
                                   // (Assertion to upstream)
  bf_tm_flow_ctrl_type_t fc_rx_type;  // honor PFC, or not.

  bool cfg_restored;  // true if port config was built from HW

  // Cached Counters per Port.
  tm_cache_counter_node_list_t *counter_state_list;
  bf_tm_sched_shaper_prov_type_t sch_prov_type;

  uint32_t credits;  // Current credits left
} bf_tm_port_t;

bf_status_t bf_tm_port_get_descriptor(bf_dev_id_t,
                                      bf_dev_port_t,
                                      bf_tm_port_t **);
void bf_tm_port_set_q_profile(bf_dev_id_t, bf_dev_port_t, int);
void bf_tm_port_get_q_profile(bf_dev_id_t, bf_dev_port_t, int *);
void bf_tm_port_set_icos_ppg_mapping(bf_dev_id_t,
                                     bf_dev_port_t,
                                     uint16_t,
                                     bf_tm_ppg_t *);
void bf_tm_port_remove_icos_mapped_to_default_ppg(bf_dev_id_t,
                                                  bf_dev_port_t,
                                                  uint16_t);
bf_tm_status_t bf_tm_port_set_wac_drop_limit(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             bf_tm_thres_t cells);
bf_tm_status_t bf_tm_port_set_qac_drop_limit(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             bf_tm_thres_t cells);
bf_tm_status_t bf_tm_port_get_wac_drop_limit(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             uint32_t *,
                                             uint32_t *);
bf_tm_status_t bf_tm_port_get_qac_drop_limit(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             uint32_t *,
                                             uint32_t *);
bf_tm_status_t bf_tm_port_get_qac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t *sw_hyst,
                                       uint32_t *hw_hyst);
bf_tm_status_t bf_tm_port_get_wac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t *sw_hyst,
                                       uint32_t *hw_hyst);
bf_tm_status_t bf_tm_port_set_wac_hyst(bf_dev_id_t,
                                       bf_tm_port_t *p,
                                       bf_tm_thres_t cells);
bf_tm_status_t bf_tm_port_set_qac_hyst(bf_dev_id_t,
                                       bf_tm_port_t *p,
                                       bf_tm_thres_t cells);
bf_tm_status_t bf_tm_port_get_skid_limit(bf_dev_id_t,
                                         bf_tm_port_t *p,
                                         bf_tm_thres_t *,
                                         bf_tm_thres_t *);
bf_tm_status_t bf_tm_port_get_wac_hyst_index(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             uint8_t *,
                                             uint8_t *);
bf_tm_status_t bf_tm_port_get_qac_hyst_index(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             uint8_t *,
                                             uint8_t *);
bf_tm_status_t bf_tm_port_set_uc_cut_through_limit(bf_dev_id_t,
                                                   bf_tm_port_t *p,
                                                   uint8_t cells);
bf_tm_status_t bf_tm_port_get_uc_cut_through_limit(bf_dev_id_t,
                                                   bf_tm_port_t *p,
                                                   bf_tm_thres_t *,
                                                   bf_tm_thres_t *);
bf_tm_status_t bf_tm_port_set_flowcontrol_mode(bf_dev_id_t,
                                               bf_tm_port_t *p,
                                               bf_tm_flow_ctrl_type_t type);
bf_tm_status_t bf_tm_port_get_flowcontrol_mode(bf_dev_id_t,
                                               bf_tm_port_t *p,
                                               bf_tm_flow_ctrl_type_t *,
                                               bf_tm_flow_ctrl_type_t *);
bf_tm_status_t bf_tm_port_set_flowcontrol_rx(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             bf_tm_flow_ctrl_type_t type);
bf_tm_status_t bf_tm_port_get_flowcontrol_rx(bf_dev_id_t,
                                             bf_tm_port_t *p,
                                             bf_tm_flow_ctrl_type_t *,
                                             bf_tm_flow_ctrl_type_t *);
bf_tm_status_t bf_tm_port_set_pfc_cos_map(bf_dev_id_t,
                                          bf_tm_port_t *p,
                                          uint8_t *cos_to_icos);
bf_tm_status_t bf_tm_port_get_pfc_cos_map(bf_dev_id_t,
                                          bf_tm_port_t *p,
                                          uint8_t *sw_cos_to_icos);
bf_tm_status_t bf_tm_port_get_pfc_cos_mask(bf_dev_id_t,
                                           bf_tm_port_t *p,
                                           uint8_t *sw_icos_to_cos_mask,
                                           uint8_t *hw_icos_to_cos_mask);
bf_status_t bf_tm_port_enable_cut_through(bf_dev_id_t dev, bf_tm_port_t *p);
bf_status_t bf_tm_port_disable_cut_through(bf_dev_id_t dev, bf_tm_port_t *p);
bf_status_t bf_tm_port_get_cut_through_enable_status(bf_dev_id_t dev,
                                                     bf_tm_port_t *p,
                                                     bool *sw_ct_enabled,
                                                     bool *hw_ct_enabled);
bf_status_t bf_tm_port_set_qac_rx_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        bool enable);

bf_tm_status_t bf_tm_add_new_port(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_port_speeds_t speed);
bf_tm_status_t bf_tm_delete_port(bf_dev_id_t dev, bf_dev_port_t port);
bf_tm_status_t bf_tm_update_port_status(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bool state);
bf_tm_status_t bf_tm_update_port_admin_state(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bool enable);
bf_tm_status_t bf_tm_port_set_cpu_port(bf_dev_id_t, bf_tm_port_t *p);
bf_tm_status_t bf_tm_port_get_ingress_drop_counter(bf_dev_id_t,
                                                   bf_tm_port_t *,
                                                   uint64_t *);
bf_tm_status_t bf_tm_port_get_wac_drop_state(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             bool *state);
bf_tm_status_t bf_tm_port_get_wac_drop_state_ext(bf_dev_id_t,
                                                 bf_tm_port_t *,
                                                 bool *,
                                                 bool *);
bf_tm_status_t bf_tm_port_clear_wac_drop_state(bf_dev_id_t, bf_tm_port_t *);
bf_tm_status_t bf_tm_port_get_qac_drop_state(bf_dev_id_t,
                                             bf_tm_port_t *,
                                             bool *);
bf_tm_status_t bf_tm_port_get_egress_drop_counter(bf_dev_id_t,
                                                  bf_tm_port_t *,
                                                  uint64_t *);
bf_tm_status_t bf_tm_port_clear_ingress_drop_counter(bf_dev_id_t,
                                                     bf_tm_port_t *);
bf_tm_status_t bf_tm_port_clear_egress_drop_counter(bf_dev_id_t,
                                                    bf_tm_port_t *);
bf_tm_status_t bf_tm_port_get_egress_drop_color_counter(bf_dev_id_t,
                                                        bf_tm_port_t *,
                                                        bf_tm_color_t,
                                                        uint64_t *);
bf_tm_status_t bf_tm_port_get_ingress_usage_counter(bf_dev_id_t,
                                                    bf_tm_port_t *,
                                                    uint32_t *);
bf_tm_status_t bf_tm_port_clear_ingress_usage_counter(bf_dev_id_t,
                                                      bf_tm_port_t *);
bf_tm_status_t bf_tm_port_get_egress_usage_counter(bf_dev_id_t,
                                                   bf_tm_port_t *,
                                                   uint32_t *);
bf_tm_status_t bf_tm_port_clear_egress_usage_counter(bf_dev_id_t dev,
                                                     bf_tm_port_t *p);
bf_tm_status_t bf_tm_port_get_ingress_water_mark(bf_dev_id_t,
                                                 bf_tm_port_t *,
                                                 uint32_t *);
bf_tm_status_t bf_tm_port_get_egress_water_mark(bf_dev_id_t,
                                                bf_tm_port_t *,
                                                uint32_t *);
bf_tm_status_t bf_tm_port_clear_ingress_water_mark(bf_dev_id_t, bf_tm_port_t *);
bf_tm_status_t bf_tm_port_clear_egress_water_mark(bf_dev_id_t, bf_tm_port_t *);
bf_tm_status_t bf_tm_port_flush_all_queues(bf_dev_id_t, bf_tm_port_t *p);
bf_tm_status_t bf_tm_port_set_skid_limit(bf_dev_id_t,
                                         bf_tm_port_t *p,
                                         bf_tm_thres_t cells);
bf_tm_status_t bf_tm_port_set_cache_counters(bf_dev_id_t dev,
                                             bf_dev_port_t port);

bf_tm_status_t bf_tm_port_get_credits(bf_dev_id_t dev, bf_tm_port_t *p);

bf_tm_status_t bf_tm_port_get_pre_port_mask(bf_dev_id_t dev,
                                            uint32_t *mask_array,
                                            uint32_t size);
bf_tm_status_t bf_tm_port_clear_pre_port_mask(bf_dev_id_t dev);
bf_tm_status_t bf_tm_port_get_pre_port_down_mask(bf_dev_id_t dev,
                                                 uint32_t *mask_array,
                                                 uint32_t size);
bf_tm_status_t bf_tm_port_clear_pre_port_down_mask(bf_dev_id_t dev);
bf_tm_status_t bf_tm_port_clear_qac_drop_state(bf_dev_id_t dev,
                                               bf_tm_port_t *p);
bf_tm_status_t bf_tm_port_clr_qac_drop_limit(bf_dev_id_t dev, bf_tm_port_t *p);

bf_tm_status_t bf_tm_port_get_pfc_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        uint8_t *state);
bf_tm_status_t bf_tm_port_set_pfc_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        uint8_t icos,
                                        bool state);
bf_tm_status_t bf_tm_port_get_pfc_state_ext(bf_dev_id_t dev,
                                            bf_tm_port_t *p_dscr,
                                            uint8_t *port_ppg_state,
                                            uint8_t *rm_pfc_state,
                                            uint8_t *mac_pfc_out,
                                            bool *mac_pause_out);
bf_tm_status_t bf_tm_port_clear_pfc_state(bf_dev_id_t dev, bf_tm_port_t *p);
bf_tm_status_t bf_tm_port_get_defaults(bf_dev_id_t,
                                       bf_tm_port_t *,
                                       bf_tm_port_defaults_t *);
bf_tm_status_t bf_tm_port_get_pfc_enabled_cos(bf_dev_id_t dev,
                                              bf_tm_port_t *p,
                                              uint8_t cos,
                                              bool *sw_enabled,
                                              bool *hw_enabled);
/* Section 2 */

/*
 * Function pointers to program HW/ASIC.
 * Depending on ASIC version/type populate correct read/write fptr.
 */

typedef bf_tm_status_t (*bf_tm_port_wr_fptr)(bf_dev_id_t, bf_tm_port_t *);
typedef bf_tm_status_t (*bf_tm_port_rd_fptr)(bf_dev_id_t, bf_tm_port_t *);

typedef bf_tm_status_t (*bf_tm_port_wr_pfc_state_fptr)(bf_dev_id_t,
                                                       bf_tm_port_t *,
                                                       uint8_t,
                                                       bool);

typedef bf_tm_status_t (*bf_tm_port_cntr_fptr)(bf_dev_id_t,
                                               bf_tm_port_t *,
                                               uint64_t *);
typedef bf_tm_status_t (*bf_tm_port_cntr_fptr2)(bf_dev_id_t,
                                                bf_tm_port_t *,
                                                bf_tm_color_t,
                                                uint64_t *);
typedef bf_tm_status_t (*bf_tm_port_cntr_fptr3)(bf_dev_id_t,
                                                bf_tm_port_t *,
                                                uint8_t *);
typedef bf_tm_status_t (*bf_tm_port_cntr_fptr4)(
    bf_dev_id_t, bf_tm_port_t *, uint8_t *, uint8_t *, uint8_t *, bool *);

typedef bf_tm_status_t (*bf_tm_port_state_fptr)(bf_dev_id_t,
                                                bf_tm_port_t *,
                                                bool *);

typedef bf_tm_status_t (*bf_tm_port_state_fptr2)(bf_dev_id_t,
                                                 bf_tm_port_t *,
                                                 bool *,
                                                 bool *);
typedef bf_tm_status_t (*bf_tm_port_pre_mask_rd_fptr)(bf_dev_id_t,
                                                      uint32_t *,
                                                      uint32_t);
typedef bf_tm_status_t (*bf_tm_port_pre_mask_wr_fptr)(bf_dev_id_t);

typedef bf_tm_status_t (*bf_tm_port_defaults_rd_fptr)(bf_dev_id_t,
                                                      bf_tm_port_t *,
                                                      bf_tm_port_defaults_t *);

typedef struct _bf_tm_port_hw_funcs {
  bf_tm_port_rd_fptr port_wac_drop_limit_rd_fptr;
  bf_tm_port_wr_fptr port_wac_drop_limit_wr_fptr;
  bf_tm_port_rd_fptr port_qac_drop_limit_rd_fptr;
  bf_tm_port_wr_fptr port_qac_drop_limit_wr_fptr;
  bf_tm_port_wr_fptr clr_qac_drop_lmt_fptr;
  bf_tm_port_rd_fptr port_wac_hyst_rd_fptr;
  bf_tm_port_wr_fptr port_wac_hyst_wr_fptr;
  bf_tm_port_rd_fptr port_qac_hyst_rd_fptr;
  bf_tm_port_wr_fptr port_qac_hyst_wr_fptr;
  bf_tm_port_wr_fptr port_uc_ct_limit_wr_fptr;
  bf_tm_port_rd_fptr port_uc_ct_limit_rd_fptr;
  bf_tm_port_wr_fptr port_flowcontrol_mode_wr_fptr;
  bf_tm_port_rd_fptr port_flowcontrol_mode_rd_fptr;
  bf_tm_port_wr_fptr port_flowcontrol_rx_wr_fptr;
  bf_tm_port_rd_fptr port_flowcontrol_rx_rd_fptr;
  bf_tm_port_wr_fptr port_pfc_cos_map_wr_fptr;
  bf_tm_port_rd_fptr port_pfc_cos_bvec_rd_fptr;
  bf_tm_port_wr_fptr port_add;
  bf_tm_port_wr_fptr port_remove;
  bf_tm_port_wr_fptr cpu_port_wr_fptr;
  bf_tm_port_wr_fptr port_cut_through_wr_fptr;
  bf_tm_port_rd_fptr port_cut_through_rd_fptr;
  bf_tm_port_wr_fptr port_qac_rx_wr_fptr;
  bf_tm_port_wr_fptr port_flush_queues_fptr;
  bf_tm_port_wr_fptr port_skid_limit_wr_fptr;
  bf_tm_port_rd_fptr port_skid_limit_rd_fptr;
  bf_tm_port_rd_fptr port_credits_rd_fptr;

  bf_tm_port_cntr_fptr ingress_drop_cntr_fptr;
  bf_tm_port_cntr_fptr egress_drop_cntr_fptr;
  bf_tm_port_cntr_fptr2 egress_drop_color_cntr_fptr;
  bf_tm_port_state_fptr2 wac_drop_state_ext_get_fptr;
  bf_tm_port_wr_fptr clr_wac_drop_state_fptr;
  bf_tm_port_state_fptr qac_drop_state_get_fptr;
  bf_tm_port_state_fptr wac_drop_state_get_fptr;
  bf_tm_port_wr_fptr clr_qac_drop_state_fptr;
  bf_tm_port_wr_fptr ingress_drop_cntr_clear_fptr;
  bf_tm_port_wr_fptr egress_drop_cntr_clear_fptr;
  bf_tm_port_cntr_fptr ingress_usage_cntr_fptr;
  bf_tm_port_wr_fptr ingress_usage_cntr_wr_fptr;
  bf_tm_port_wr_fptr egress_usage_cntr_wr_fptr;
  bf_tm_port_cntr_fptr egress_usage_cntr_fptr;
  bf_tm_port_cntr_fptr ingress_wm_cntr_fptr;
  bf_tm_port_cntr_fptr egress_wm_cntr_fptr;
  bf_tm_port_wr_fptr ingress_wm_clear_fptr;
  bf_tm_port_wr_fptr egress_wm_clear_fptr;

  bf_tm_port_pre_mask_rd_fptr pre_port_mask_rd_fptr;
  bf_tm_port_pre_mask_wr_fptr pre_port_mask_wr_fptr;
  bf_tm_port_pre_mask_rd_fptr pre_port_down_mask_rd_fptr;
  bf_tm_port_pre_mask_wr_fptr pre_port_down_mask_wr_fptr;

  bf_tm_port_cntr_fptr3 port_pfc_state_fptr;
  bf_tm_port_cntr_fptr4 port_pfc_state_ext_fptr;
  bf_tm_port_wr_pfc_state_fptr set_port_pfc_state_fptr;
  bf_tm_port_wr_fptr clr_port_pfc_state_fptr;

  bf_tm_port_defaults_rd_fptr port_get_defaults_fptr;
  bf_tm_port_cntr_fptr3 port_pfc_enabled_cos_rd_fptr;
} bf_tm_port_hw_funcs_tbl;

#endif
