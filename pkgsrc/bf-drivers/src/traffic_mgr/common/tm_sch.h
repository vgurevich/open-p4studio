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
 *    related to Scheduler and shaper in TM
 */

#ifndef __TM_SCH_H__
#define __TM_SCH_H__

#include <traffic_mgr/traffic_mgr_types.h>

/*
 *
 * Section 1: Scheduler Accessor functions exported for use by other functions
 *
 *  - First section has all data structures  related to sched.
 *  - Accessor functions are provided in its corresponding .c file to
 *    read/write to data elements.
 *
 * Section 2: Sched HW programming functions
 *
 *  - For every scheduler field/attribute that has presence in HW, implement
 *    corresponding HW read/write funciton.
 *  - Setup a table of funtion pointers so that accessor functions
 *    will use these functions to program/read from HW
 *  - These HW access function will be invoked any time accessor functions
 *    modifies fields that has presence in HW or attempts to read (with option
 *    to also read from HW)
 */

/* Section 1 */

// Prototypes of accessor functions

bf_status_t bf_tm_sch_l1_get_descriptor(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t dev_l1,
                                        bf_tm_eg_l1_t **l1);

bf_status_t bf_tm_sch_set_q_sched_prio(bf_dev_id_t,
                                       bf_tm_eg_q_t *,
                                       bf_tm_sched_prio_t,
                                       bool);
bf_status_t bf_tm_sch_get_q_sched_prio(bf_dev_id_t,
                                       bf_tm_eg_q_t *,
                                       bool,
                                       bf_tm_sched_prio_t *,
                                       bf_tm_sched_prio_t *);

#define BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(field, ...) \
  bf_tm_status_t bf_tm_sch_set_##field(bf_dev_id_t, bf_tm_eg_q_t *, __VA_ARGS__)

#define BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(field, ...) \
  bf_tm_status_t bf_tm_sch_get_##field(bf_dev_id_t, bf_tm_eg_q_t *, __VA_ARGS__)

#define BF_TM_SCH_CLR_ACCESSOR_FUNC_PROTO(field) \
  bf_tm_status_t bf_tm_sch_clear_##field(bf_dev_id_t, bf_tm_eg_q_t *)

BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_dwrr_wt, uint16_t);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_dwrr_wt, uint16_t *, uint16_t *);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_max_rate, bool, uint32_t, uint32_t);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_max_rate_provisioning,
                                 bool,
                                 uint32_t,
                                 uint32_t,
                                 bf_tm_sched_shaper_prov_type_t);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(
    q_max_rate, bool *, uint32_t *, uint32_t *, uint32_t *, uint32_t *);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_max_rate_provisioning,
                                 bool *,
                                 uint32_t *,
                                 uint32_t *,
                                 uint32_t *,
                                 uint32_t *,
                                 bf_tm_sched_shaper_prov_type_t *);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_gmin_rate, bool, uint32_t, uint32_t);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(
    q_gmin_rate, bool *, uint32_t *, uint32_t *, uint32_t *, uint32_t *);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_sched, bool);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_sched, bool *, bool *);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_pfc_prio, uint8_t);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_pfc_prio, uint8_t *, uint8_t *);

BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_adv_fc_mode, bf_tm_sched_adv_fc_mode_t);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_adv_fc_mode,
                                 bf_tm_sched_adv_fc_mode_t *,
                                 bf_tm_sched_adv_fc_mode_t *);

BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_speed, bf_port_speeds_t *);

#define BF_TM_PORT_SCH_WR_ACCESSOR_FUNC_PROTO(field, ...) \
  bf_tm_status_t bf_tm_sch_set_port_##field(              \
      bf_dev_id_t, bf_tm_port_t *, __VA_ARGS__)

#define BF_TM_PORT_SCH_RD_ACCESSOR_FUNC_PROTO(field, ...) \
  bf_tm_status_t bf_tm_sch_get_port_##field(              \
      bf_dev_id_t, bf_tm_port_t *, __VA_ARGS__)

BF_TM_PORT_SCH_WR_ACCESSOR_FUNC_PROTO(max_rate, bool, uint32_t, uint32_t);
BF_TM_PORT_SCH_WR_ACCESSOR_FUNC_PROTO(max_rate_provisioning,
                                      bool,
                                      uint32_t,
                                      uint32_t,
                                      bf_tm_sched_shaper_prov_type_t);
BF_TM_PORT_SCH_RD_ACCESSOR_FUNC_PROTO(
    max_rate, bool *, uint32_t *, uint32_t *, uint32_t *, uint32_t *);
BF_TM_PORT_SCH_RD_ACCESSOR_FUNC_PROTO(max_rate_provisioning,
                                      bool *,
                                      uint32_t *,
                                      uint32_t *,
                                      uint32_t *,
                                      uint32_t *,
                                      bf_tm_sched_shaper_prov_type_t *);
BF_TM_PORT_SCH_WR_ACCESSOR_FUNC_PROTO(sched, bf_port_speeds_t, bool);
BF_TM_PORT_SCH_RD_ACCESSOR_FUNC_PROTO(sched, bool *, bool *);

bf_status_t bf_tm_sch_set_pkt_ifg_compensation(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe);

bf_status_t bf_tm_sch_get_pkt_ifg_compensation(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint8_t *sw_ifg,
                                               uint8_t *hw_ifg);

bf_status_t bf_tm_sch_enable_q_max_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q);
bf_status_t bf_tm_sch_disable_q_max_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q);
bf_status_t bf_tm_sch_enable_q_min_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q);
bf_status_t bf_tm_sch_disable_q_min_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q);
bf_status_t bf_tm_sch_q_max_rate_status_get(bf_dev_id_t dev,
                                            bf_tm_eg_q_t *q,
                                            bool *status);
bf_status_t bf_tm_sch_q_min_rate_status_get(bf_dev_id_t dev,
                                            bf_tm_eg_q_t *q,
                                            bool *status);

bf_status_t bf_tm_sch_enable_port_max_rate(bf_dev_id_t dev, bf_tm_port_t *p);
bf_status_t bf_tm_sch_disable_port_max_rate(bf_dev_id_t dev, bf_tm_port_t *p);
bf_status_t bf_tm_sch_get_port_max_rate_enable_status(bf_dev_id_t dev,
                                                      bf_tm_port_t *p,
                                                      bool *sw_status,
                                                      bool *hw_status);

bf_status_t bf_tm_sch_l1_enable(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_l1_disable(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_l1_free(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_q_l1_get(bf_dev_id_t dev,
                               bf_tm_eg_q_t *q,
                               bf_tm_l1_node_t *sw_l1_node,
                               bf_tm_l1_node_t *hw_l1_node);
bf_status_t bf_tm_sch_q_l1_set(bf_dev_id_t dev,
                               bf_tm_eg_q_t *q,
                               bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_q_l1_reset(bf_dev_id_t dev, bf_tm_eg_q_t *q);
bf_status_t bf_tm_sch_set_l1_sched_prio(bf_dev_id_t dev,
                                        bf_tm_eg_l1_t *l1,
                                        bf_tm_sched_prio_t prio,
                                        bool r_bw);
bf_status_t bf_tm_sch_l1_set_port_default(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_eg_l1_t **l1);
bf_status_t bf_tm_sch_l1_get_port_nth(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_l1_node_t port_l1,
                                      bf_tm_eg_l1_t **l1);
bf_status_t bf_tm_sch_set_l1_dwrr_wt(bf_dev_id_t dev,
                                     bf_tm_eg_l1_t *l1,
                                     uint16_t wt);
bf_status_t bf_tm_sch_get_l1_dwrr_wt(bf_dev_id_t dev,
                                     bf_tm_eg_l1_t *l1,
                                     uint16_t *sw_wt,
                                     uint16_t *hw_wt);
bf_status_t bf_tm_sch_get_l1_enable(bf_dev_id_t dev,
                                    bf_tm_eg_l1_t *l1,
                                    bf_dev_port_t *sw_port,
                                    bf_dev_port_t *hw_port,
                                    bool *sw_enable,
                                    bool *hw_enable);
bf_status_t bf_tm_sch_get_l1_guaranteed_rate_enable(bf_dev_id_t dev,
                                                    bf_tm_eg_l1_t *l1,
                                                    bool *sw_enable,
                                                    bool *hw_enable);
bf_status_t bf_tm_sch_get_l1_max_shaping_rate_enable(bf_dev_id_t dev,
                                                     bf_tm_eg_l1_t *l1,
                                                     bool *sw_enable,
                                                     bool *hw_enable);
bf_status_t bf_tm_sch_get_l1_priority_prop_enable(bf_dev_id_t dev,
                                                  bf_tm_eg_l1_t *l1,
                                                  bool *sw_enable,
                                                  bool *hw_enable);
bf_status_t bf_tm_sch_get_l1_priority(bf_dev_id_t dev,
                                      bf_tm_eg_l1_t *l1,
                                      bf_tm_sched_prio_t *sw_prio,
                                      bf_tm_sched_prio_t *hw_prio);
bf_status_t bf_tm_sch_get_l1_sched_prio(bf_dev_id_t dev,
                                        bf_tm_eg_l1_t *l1,
                                        bf_tm_sched_prio_t *sw_prio,
                                        bf_tm_sched_prio_t *hw_prio);
bf_status_t bf_tm_sch_get_l1_guaranteed_rate(bf_dev_id_t dev,
                                             bf_tm_eg_l1_t *l1,
                                             bool *sw_pps,
                                             bool *hw_pps,
                                             uint32_t *sw_burst_size,
                                             uint32_t *hw_burst_size,
                                             uint32_t *sw_rate,
                                             uint32_t *hw_rate);
bf_status_t bf_tm_sch_get_l1_shaping_rate(bf_dev_id_t dev,
                                          bf_tm_eg_l1_t *l1,
                                          bool *sw_pps,
                                          bool *hw_pps,
                                          uint32_t *sw_burst_size,
                                          uint32_t *hw_burst_size,
                                          uint32_t *sw_rate,
                                          uint32_t *hw_rate);
bf_status_t bf_tm_sch_set_l1_max_rate(bf_dev_id_t dev,
                                      bf_tm_eg_l1_t *l1,
                                      bool pps,
                                      uint32_t burst_size,
                                      uint32_t rate);
bf_status_t bf_tm_sch_enable_l1_max_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_disable_l1_max_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_set_l1_gmin_rate(bf_dev_id_t dev,
                                       bf_tm_eg_l1_t *l1,
                                       bool pps,
                                       uint32_t burst_size,
                                       uint32_t rate);
bf_status_t bf_tm_sch_enable_l1_min_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_disable_l1_min_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_enable_l1_pri_prop(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);
bf_status_t bf_tm_sch_disable_l1_pri_prop(bf_dev_id_t dev, bf_tm_eg_l1_t *l1);

bf_status_t bf_tm_sch_force_disable_port_sched(bf_dev_id_t dev,
                                               bf_tm_port_t *port);

bf_status_t bf_tm_sch_set_adv_fc_mode_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool enable);
bf_status_t bf_tm_sch_get_adv_fc_mode_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool *enable);
bf_status_t bf_tm_sch_get_l1_sch(bf_dev_id_t dev, bf_tm_eg_l1_t *l1_node);
bf_tm_status_t bf_tm_sch_q_get_defaults(bf_dev_id_t devid,
                                        bf_tm_port_t *p,
                                        bf_tm_eg_q_t *q,
                                        bf_tm_sch_q_defaults_t *def);
bf_tm_status_t bf_tm_sch_port_get_defaults(bf_dev_id_t devid,
                                           bf_tm_port_t *p,
                                           bf_tm_sch_port_defaults_t *def);
bf_tm_status_t bf_tm_sch_l1_get_defaults(bf_dev_id_t devid,
                                         bf_tm_port_t *p,
                                         bf_tm_eg_l1_t *l1,
                                         bf_tm_sch_l1_defaults_t *def);
/* Section 2 */
/*
 * Function Pointers to program HW
 */

typedef bf_tm_status_t (*bf_tm_sch_q_wr_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_sch_q_wr_fptr1)(bf_dev_id_t,
                                               bf_tm_eg_q_t *,
                                               bool);
typedef bf_tm_status_t (*bf_tm_sch_q_rd_fptr)(bf_dev_id_t,
                                              bf_tm_eg_q_t *,
                                              bool *);
typedef bf_tm_status_t (*bf_tm_sch_q_speed_rd_fptr)(bf_dev_id_t,
                                                    bf_tm_eg_q_t *,
                                                    bf_port_speeds_t *);
typedef bf_tm_status_t (*bf_tm_sch_l1_wr_fptr)(bf_dev_id_t, bf_tm_eg_l1_t *);
typedef bf_tm_status_t (*bf_tm_sch_wr_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_sch_rd_fptr)(bf_dev_id_t, bf_tm_eg_q_t *);
typedef bf_tm_status_t (*bf_tm_sch_port_wr_fptr)(bf_dev_id_t, bf_tm_port_t *);
typedef bf_tm_status_t (*bf_tm_sch_port_rd_fptr)(bf_dev_id_t, bf_tm_port_t *);
typedef bf_tm_status_t (*bf_tm_sch_port_rd_fptr2)(bf_dev_id_t,
                                                  bf_tm_port_t *,
                                                  uint8_t *);

typedef bf_tm_status_t (*bf_tm_sch_ifg_wr_fptr)(bf_dev_id_t, void *);
typedef bf_tm_status_t (*bf_tm_sch_ifg_rd_fptr)(bf_dev_id_t, void *);

typedef bf_tm_status_t (*bf_tm_sch_adv_fc_mode_enable_wr_fptr)(
    bf_dev_id_t, bf_tm_eg_pipe_t *pipe, bool);
typedef bf_tm_status_t (*bf_tm_sch_adv_fc_mode_enable_rd_fptr)(
    bf_dev_id_t, bf_tm_eg_pipe_t *pipe, bool *);

typedef bf_tm_status_t (*bf_tm_sch_l1_rd_fptr)(bf_dev_id_t, bf_tm_eg_l1_t *);
typedef bf_tm_status_t (*bf_tm_sch_q_defaults_rd_fptr)(
    bf_dev_id_t, bf_tm_port_t *, bf_tm_eg_q_t *, bf_tm_sch_q_defaults_t *);
typedef bf_tm_status_t (*bf_tm_sch_port_defaults_rd_fptr)(
    bf_dev_id_t, bf_tm_port_t *, bf_tm_sch_port_defaults_t *);
typedef bf_tm_status_t (*bf_tm_sch_l1_defaults_rd_fptr)(
    bf_dev_id_t, bf_tm_port_t *, bf_tm_eg_l1_t *, bf_tm_sch_l1_defaults_t *);
/*
 * Funtion pointers to verify shaping restore config
 * for HA unit test
 */
typedef bool (*bf_tm_sch_burst_size_verify_fptr)(bf_dev_id_t,
                                                 uint32_t,
                                                 uint32_t);
typedef bool (*bf_tm_sch_rate_verify_fptr)(bf_dev_id_t,
                                           uint32_t,
                                           uint32_t,
                                           bool);

typedef struct _bf_tm_sch_hw_funcs {
  bf_tm_sch_wr_fptr q_sch_prio_wr_fptr;
  bf_tm_sch_rd_fptr q_sch_prio_rd_fptr;
  bf_tm_sch_wr_fptr q_dwrr_wr_fptr;
  bf_tm_sch_rd_fptr q_dwrr_rd_fptr;
  bf_tm_sch_wr_fptr q_shaper_wr_fptr;
  bf_tm_sch_rd_fptr q_shaper_rd_fptr;
  bf_tm_sch_wr_fptr q_gmin_shaper_wr_fptr;
  bf_tm_sch_rd_fptr q_gmin_shaper_rd_fptr;
  bf_tm_sch_wr_fptr q_max_rate_enable_status_wr_fptr;
  bf_tm_sch_rd_fptr q_max_rate_enable_status_rd_fptr;
  bf_tm_sch_wr_fptr q_min_rate_enable_status_wr_fptr;
  bf_tm_sch_rd_fptr q_min_rate_enable_status_rd_fptr;
  bf_tm_sch_wr_fptr q_pfc_prio_wr_fptr;
  bf_tm_sch_rd_fptr q_pfc_prio_rd_fptr;
  bf_tm_sch_port_wr_fptr port_max_rate_enable_status_wr_fptr;
  bf_tm_sch_port_rd_fptr port_max_rate_enable_status_rd_fptr;
  bf_tm_sch_port_wr_fptr port_shaper_wr_fptr;
  bf_tm_sch_port_rd_fptr port_shaper_rd_fptr;
  bf_tm_sch_ifg_wr_fptr pkt_ifg_comp_wr_fptr;
  bf_tm_sch_ifg_rd_fptr pkt_ifg_comp_rd_fptr;
  bf_tm_sch_q_wr_fptr q_set_sch_wr_fptr;
  bf_tm_sch_l1_wr_fptr l1_set_sch_wr_fptr;
  bf_tm_sch_l1_wr_fptr l1_dwrr_wr_fptr;
  bf_tm_sch_l1_wr_fptr l1_set_sch_rate_wr_fptr;
  bf_tm_sch_l1_wr_fptr l1_set_sch_min_rate_wr_fptr;
  bf_tm_sch_rd_fptr q_get_sch_rd_fptr;
  bf_tm_sch_port_wr_fptr port_set_sch_wr_fptr;
  bf_tm_sch_port_rd_fptr port_get_sch_rd_fptr;
  bf_tm_sch_port_rd_fptr2 eg_port_pfc_status_fptr;
  bf_tm_sch_q_rd_fptr eg_q_pfc_status_fptr;
  bf_tm_sch_burst_size_verify_fptr burst_size_verify_fptr;
  bf_tm_sch_rate_verify_fptr rate_verify_fptr;
  bf_tm_sch_port_wr_fptr port_force_disable_sch_fptr;
  bf_tm_sch_wr_fptr q_adv_fc_mode_wr_fptr;
  bf_tm_sch_rd_fptr q_adv_fc_mode_rd_fptr;
  bf_tm_sch_adv_fc_mode_enable_wr_fptr adv_fc_mode_enable_wr_fptr;
  bf_tm_sch_adv_fc_mode_enable_rd_fptr adv_fc_mode_enable_rd_fptr;
  bf_tm_sch_l1_rd_fptr l1_dwrr_rd_fptr;
  bf_tm_sch_l1_rd_fptr l1_get_sch_rd_fptr;
  bf_tm_sch_l1_rd_fptr l1_shaper_rd_fptr;
  bf_tm_sch_l1_rd_fptr l1_gmin_shaper_rd_fptr;
  bf_tm_sch_q_speed_rd_fptr q_speed_rd_fptr;
  bf_tm_sch_q_defaults_rd_fptr sch_q_defaults_rd_fptr;
  bf_tm_sch_port_defaults_rd_fptr sch_port_defaults_rd_fptr;
  bf_tm_sch_l1_defaults_rd_fptr sch_l1_defaults_rd_fptr;
  bf_tm_sch_q_wr_fptr1 eg_q_pfc_set_status_fptr;
  bf_tm_sch_q_wr_fptr eg_q_pfc_clr_status_fptr;
} bf_tm_sch_hw_funcs_tbl;

/*
 * Function to verify shaping restore config for HA unit test
 */
bool bf_tm_sch_cfg_verify_burst_size(bf_dev_id_t, uint32_t, uint32_t);
bool bf_tm_sch_cfg_verify_rate(bf_dev_id_t, uint32_t, uint32_t, bool);

BF_TM_PORT_SCH_RD_ACCESSOR_FUNC_PROTO(egress_pfc_status, uint8_t *, uint8_t *);
BF_TM_SCH_RD_ACCESSOR_FUNC_PROTO(q_egress_pfc_status, bool *, bool *);
BF_TM_SCH_WR_ACCESSOR_FUNC_PROTO(q_egress_pfc_status, bool);
BF_TM_SCH_CLR_ACCESSOR_FUNC_PROTO(q_egress_pfc_status);

#endif
