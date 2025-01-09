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


// This file implements abstracted (from Tofino/Tofino-lite/..) APIs
// to set scheduling and shaping on egress queues

#include <string.h>
#include "tm_ctx.h"
#include "tm_sch.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"





static bf_tm_sch_hw_funcs_tbl g_sch_hw_fptr_tbl;

bf_status_t bf_tm_sch_set_q_sched_prio(bf_dev_id_t dev,
                                       bf_tm_eg_q_t *q,
                                       bf_tm_sched_prio_t prio,
                                       bool r_bw) {
  bf_status_t rc = BF_SUCCESS;

  if (r_bw) {
    if (TM_HITLESS_IS_CFG_MATCH(
            prio, q->q_sch_cfg.max_rate_sch_prio, g_tm_ctx[dev])) {
      return (rc);
    }
    q->q_sch_cfg.max_rate_sch_prio = prio;
  } else {
    if (TM_HITLESS_IS_CFG_MATCH(
            prio, q->q_sch_cfg.min_rate_sch_prio, g_tm_ctx[dev])) {
      return (rc);
    }
    q->q_sch_cfg.min_rate_sch_prio = prio;
  }

  if (g_sch_hw_fptr_tbl.q_sch_prio_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_sch_prio_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_sched_prio(bf_dev_id_t dev,
                                       bf_tm_eg_q_t *q,
                                       bool r_bw,
                                       bf_tm_sched_prio_t *sw_prio,
                                       bf_tm_sched_prio_t *hw_prio) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if (r_bw) {
    *sw_prio = q->q_sch_cfg.max_rate_sch_prio;
  } else {
    *sw_prio = q->q_sch_cfg.min_rate_sch_prio;
  }

  if (TM_IS_TARGET_ASIC(dev) && hw_prio &&
      g_sch_hw_fptr_tbl.q_sch_prio_wr_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_sch_prio_rd_fptr(dev, &out_q);
    if (r_bw) {
      *hw_prio = out_q.q_sch_cfg.max_rate_sch_prio;
    } else {
      *hw_prio = out_q.q_sch_cfg.min_rate_sch_prio;
    }
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_dwrr_wt(bf_dev_id_t dev,
                                    bf_tm_eg_q_t *q,
                                    uint16_t wt) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(wt, q->q_sch_cfg.dwrr_wt, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.dwrr_wt = wt;

  if (g_sch_hw_fptr_tbl.q_dwrr_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_dwrr_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_dwrr_wt(bf_dev_id_t dev,
                                    bf_tm_eg_q_t *q,
                                    uint16_t *sw_wt,
                                    uint16_t *hw_wt) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  *sw_wt = q->q_sch_cfg.dwrr_wt;

  if (TM_IS_TARGET_ASIC(dev) && hw_wt && g_sch_hw_fptr_tbl.q_dwrr_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_dwrr_rd_fptr(dev, &out_q);
    *hw_wt = out_q.q_sch_cfg.dwrr_wt;
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_speed(bf_dev_id_t dev,
                                  bf_tm_eg_q_t *q,
                                  bf_port_speeds_t *speed) {
  bf_status_t rc = BF_SUCCESS;

  if (g_sch_hw_fptr_tbl.q_speed_rd_fptr) {
    rc = g_sch_hw_fptr_tbl.q_speed_rd_fptr(dev, q, speed);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_pfc_prio(bf_dev_id_t dev,
                                     bf_tm_eg_q_t *q,
                                     uint8_t pfc_prio) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pfc_prio, q->q_sch_cfg.pfc_prio, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.pfc_prio = pfc_prio;

  if (g_sch_hw_fptr_tbl.q_pfc_prio_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_pfc_prio_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_pfc_prio(bf_dev_id_t dev,
                                     bf_tm_eg_q_t *q,
                                     uint8_t *sw_pfc_prio,
                                     uint8_t *hw_pfc_prio) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  *sw_pfc_prio = q->q_sch_cfg.pfc_prio;

  if (TM_IS_TARGET_ASIC(dev) && hw_pfc_prio &&
      g_sch_hw_fptr_tbl.q_pfc_prio_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_pfc_prio_rd_fptr(dev, &out_q);
    *hw_pfc_prio = out_q.q_sch_cfg.pfc_prio;
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_max_rate(bf_dev_id_t dev,
                                     bf_tm_eg_q_t *q,
                                     bool pps,
                                     uint32_t burst_size,
                                     uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pps, q->q_sch_cfg.pps, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(
          burst_size, q->q_sch_cfg.max_burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(rate, q->q_sch_cfg.max_rate, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.max_rate = rate;
  q->q_sch_cfg.max_burst_size = burst_size;
  q->q_sch_cfg.pps = pps;
  q->q_sch_cfg.sch_prov_type = BF_TM_SCH_RATE_UPPER;

  if (g_sch_hw_fptr_tbl.q_shaper_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_shaper_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_max_rate_provisioning(
    bf_dev_id_t dev,
    bf_tm_eg_q_t *q,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pps, q->q_sch_cfg.pps, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(
          burst_size, q->q_sch_cfg.max_burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(rate, q->q_sch_cfg.max_rate, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.max_rate = rate;
  q->q_sch_cfg.max_burst_size = burst_size;
  q->q_sch_cfg.pps = pps;
  q->q_sch_cfg.sch_prov_type = prov_type;

  if (g_sch_hw_fptr_tbl.q_shaper_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_shaper_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_max_rate(bf_dev_id_t dev,
                                     bf_tm_eg_q_t *q,
                                     bool *pps,
                                     uint32_t *sw_burst_size,
                                     uint32_t *hw_burst_size,
                                     uint32_t *sw_rate,
                                     uint32_t *hw_rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if (sw_rate) *sw_rate = q->q_sch_cfg.max_rate;
  if (sw_burst_size) *sw_burst_size = q->q_sch_cfg.max_burst_size;
  if (pps) *pps = q->q_sch_cfg.pps;

  if (TM_IS_TARGET_ASIC(dev) && pps && hw_burst_size && hw_rate &&
      g_sch_hw_fptr_tbl.q_shaper_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_shaper_rd_fptr(dev, &out_q);
    *hw_rate = out_q.q_sch_cfg.max_rate;
    *hw_burst_size = out_q.q_sch_cfg.max_burst_size;
    *pps = out_q.q_sch_cfg.pps;
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_max_rate_provisioning(
    bf_dev_id_t dev,
    bf_tm_eg_q_t *q,
    bool *pps,
    uint32_t *sw_burst_size,
    uint32_t *hw_burst_size,
    uint32_t *sw_rate,
    uint32_t *hw_rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if ((q == NULL) || (pps == NULL) || (sw_burst_size == NULL) ||
      (hw_burst_size == NULL) || (sw_rate == NULL) || (hw_rate == NULL)) {
    return BF_INVALID_ARG;
  }

  *sw_rate = q->q_sch_cfg.max_rate;
  *sw_burst_size = q->q_sch_cfg.max_burst_size;
  *pps = q->q_sch_cfg.pps;
  if (prov_type) {
    *prov_type = q->q_sch_cfg.sch_prov_type;
  }

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.q_shaper_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_shaper_rd_fptr(dev, &out_q);
    *hw_rate = out_q.q_sch_cfg.max_rate;
    *hw_burst_size = out_q.q_sch_cfg.max_burst_size;
    *pps = out_q.q_sch_cfg.pps;
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_q_max_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          true, q->q_sch_cfg.max_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.max_rate_enable = true;
  if (g_sch_hw_fptr_tbl.q_max_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_max_rate_enable_status_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_q_max_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          false, q->q_sch_cfg.max_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.max_rate_enable = false;
  if (g_sch_hw_fptr_tbl.q_max_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_max_rate_enable_status_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_q_max_rate_status_get(bf_dev_id_t dev,
                                            bf_tm_eg_q_t *q,
                                            bool *status) {
  bf_status_t rc = BF_SUCCESS;

  if (g_sch_hw_fptr_tbl.q_max_rate_enable_status_rd_fptr) {
    rc = g_sch_hw_fptr_tbl.q_max_rate_enable_status_rd_fptr(dev, q);
    if (rc == BF_SUCCESS) {
      *status = q->q_sch_cfg.max_rate_enable;
    }
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_gmin_rate(bf_dev_id_t dev,
                                      bf_tm_eg_q_t *q,
                                      bool pps,
                                      uint32_t burst_size,
                                      uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pps, q->q_sch_cfg.pps, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(
          burst_size, q->q_sch_cfg.min_burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(rate, q->q_sch_cfg.min_rate, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.min_rate = rate;
  q->q_sch_cfg.min_burst_size = burst_size;
  q->q_sch_cfg.pps = pps;

  if (g_sch_hw_fptr_tbl.q_gmin_shaper_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_gmin_shaper_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_gmin_rate(bf_dev_id_t dev,
                                      bf_tm_eg_q_t *q,
                                      bool *pps,
                                      uint32_t *sw_burst_size,
                                      uint32_t *hw_burst_size,
                                      uint32_t *sw_rate,
                                      uint32_t *hw_rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  *sw_rate = q->q_sch_cfg.min_rate;
  *sw_burst_size = q->q_sch_cfg.min_burst_size;
  *pps = q->q_sch_cfg.pps;

  if (TM_IS_TARGET_ASIC(dev) && hw_burst_size && hw_rate &&
      g_sch_hw_fptr_tbl.q_gmin_shaper_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_gmin_shaper_rd_fptr(dev, &out_q);
    *hw_rate = out_q.q_sch_cfg.min_rate;
    *hw_burst_size = out_q.q_sch_cfg.min_burst_size;
    *pps = out_q.q_sch_cfg.pps;
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_q_min_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          true, q->q_sch_cfg.min_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.min_rate_enable = true;
  if (g_sch_hw_fptr_tbl.q_min_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_min_rate_enable_status_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_q_min_rate(bf_dev_id_t dev, bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          false, q->q_sch_cfg.min_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.min_rate_enable = false;
  if (g_sch_hw_fptr_tbl.q_min_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_min_rate_enable_status_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_q_min_rate_status_get(bf_dev_id_t dev,
                                            bf_tm_eg_q_t *q,
                                            bool *status) {
  bf_status_t rc = BF_SUCCESS;

  if (g_sch_hw_fptr_tbl.q_min_rate_enable_status_rd_fptr) {
    rc = g_sch_hw_fptr_tbl.q_min_rate_enable_status_rd_fptr(dev, q);
    if (rc == BF_SUCCESS) {
      *status = q->q_sch_cfg.min_rate_enable;
    }
  }
  return (rc);
}

bf_status_t bf_tm_sch_l1_get_descriptor(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t dev_l1,
                                        bf_tm_eg_l1_t **l1) {
  if (!l1 || dev_l1 >= g_tm_ctx[dev]->tm_cfg.l1_per_pg) {
    return BF_INVALID_ARG;
  }
  *l1 = BF_TM_FIRST_L1_PTR_IN_PG(g_tm_ctx[dev], port) + dev_l1;
  if (dev_l1 != (*l1)->logical_l1) {
    LOG_ERROR(
        "Mismatch between logical l1 lookup and l1 records. Device = %d "
        "pipe = %d "
        "dev_l1 = %d "
        "struct_l1 = %d",
        dev,
        (*l1)->p_pipe,
        dev_l1,
        (*l1)->logical_l1);
  }
  return BF_SUCCESS;
}

bf_status_t bf_tm_sch_l1_enable(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_eg_l1_t *l1) {
  if (NULL == l1) {
    return BF_INVALID_ARG;
  }
  bf_dev_port_t l1_dev_port =
      MAKE_DEV_PORT(((bf_dev_port_t)(l1->l_pipe)), (l1->uport));
  if (l1->in_use && l1_dev_port != port) {
    LOG_ERROR(
        "l1 already in use by port %d. "
        "Device = %d "
        "pipe = %d "
        "pg = %d "
        "l1 = %d",
        l1_dev_port,
        dev,
        l1->l_pipe,
        l1->pg,
        l1->logical_l1);
    return BF_INVALID_ARG;
  }

  if (l1->l_pipe != (DEV_PORT_TO_PIPE(port))) {
    LOG_ERROR("port=%d is on different pipe than l1 node %d of pg=%d pipe=%d",
              port,
              l1->logical_l1,
              l1->pg,
              l1->l_pipe);
    return BF_INVALID_ARG;
  }

  l1->in_use = true;
  l1->uport = DEV_PORT_TO_LOCAL_PORT(port);
  l1->port = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_user_to_device(dev, port));

  l1->l1_sch_cfg.cid =
      DEV_PORT_TO_LOCAL_PORT(l1->port) % g_tm_ctx[dev]->tm_cfg.ports_per_pg;
  l1->l1_sch_cfg.sch_enabled = true;

  bf_status_t rc = BF_SUCCESS;
  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return rc;
}

bf_status_t bf_tm_sch_l1_disable(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  if (!l1->in_use) {
    LOG_ERROR(
        "Cannot disable unallocated l1. Device = %d "
        "pipe = %d "
        "l1 = %d",
        dev,
        l1->p_pipe,
        l1->logical_l1);
    return BF_INVALID_ARG;
  }

  l1->l1_sch_cfg.sch_enabled = false;

  bf_status_t rc = BF_SUCCESS;
  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return rc;
}

static bf_status_t l1_has_queues(bf_dev_id_t dev,
                                 bf_tm_eg_l1_t *l1,
                                 bool *has_queues) {
  if (NULL == l1 || NULL == has_queues) {
    return BF_INVALID_ARG;
  }

  bf_status_t rc = BF_SUCCESS;
  *has_queues = false;

  bf_dev_port_t port =
      MAKE_DEV_PORT(((bf_dev_port_t)(l1->l_pipe)), (l1->uport));

  bf_tm_q_profile_t *q_prof;
  int q_profile_index;
  uint8_t q_count;

  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof = g_tm_ctx[dev]->q_profile + q_profile_index;
  q_count = q_prof->q_count;

  bf_tm_queue_t q;
  bf_tm_eg_q_t *queue;
  for (q = 0; q < q_count; q++) {
    rc |= bf_tm_q_get_descriptor(dev, port, q, &queue);
    if (rc != BF_TM_EOK) {
      return rc;
    }
    if (queue->q_sch_cfg.sch_enabled &&
        queue->q_sch_cfg.cid == l1->logical_l1) {
      *has_queues = true;
      return rc;
    }
  }
  return rc;
}

bf_status_t bf_tm_sch_l1_free(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;
  bool has_queues;
  if (!l1->in_use) {
    LOG_ERROR(
        "Cannot disable unallocated l1. Device = %d "
        "pipe = %d "
        "l1 = %d",
        dev,
        l1->p_pipe,
        l1->logical_l1);
    return BF_INVALID_ARG;
  }
  rc = l1_has_queues(dev, l1, &has_queues);
  if (rc != BF_SUCCESS) {
    return rc;
  }
  if (has_queues) {
    LOG_ERROR(
        "Cannot disable l1 while queues are still assigned. Device = %d "
        "pipe = %d "
        "l1 = %d",
        dev,
        l1->p_pipe,
        l1->logical_l1);
    return BF_INVALID_ARG;
  }

  l1->in_use = false;
  l1->l1_sch_cfg.sch_enabled = false;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return rc;
}

bf_status_t bf_tm_sch_set_l1_sched_prio(bf_dev_id_t dev,
                                        bf_tm_eg_l1_t *l1,
                                        bf_tm_sched_prio_t prio,
                                        bool r_bw) {
  bf_status_t rc = BF_SUCCESS;

  if (r_bw) {
    if (TM_HITLESS_IS_CFG_MATCH(
            prio, l1->l1_sch_cfg.max_rate_sch_prio, g_tm_ctx[dev])) {
      return (rc);
    }
    l1->l1_sch_cfg.max_rate_sch_prio = prio;
  } else {
    if (TM_HITLESS_IS_CFG_MATCH(
            prio, l1->l1_sch_cfg.min_rate_sch_prio, g_tm_ctx[dev])) {
      return (rc);
    }
    l1->l1_sch_cfg.min_rate_sch_prio = prio;
  }

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_l1_dwrr_wt(bf_dev_id_t dev,
                                     bf_tm_eg_l1_t *l1,
                                     uint16_t wt) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(wt, l1->l1_sch_cfg.dwrr_wt, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.dwrr_wt = wt;

  if (g_sch_hw_fptr_tbl.l1_dwrr_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_dwrr_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_l1_dwrr_wt(bf_dev_id_t dev,
                                     bf_tm_eg_l1_t *l1,
                                     uint16_t *sw_wt,
                                     uint16_t *hw_wt) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_wt) {
    *sw_wt = l1->l1_sch_cfg.dwrr_wt;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_wt && TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.l1_dwrr_rd_fptr) {
    if (l1) {
      memcpy(&l1_out, l1, sizeof(l1_out));
      rc = g_sch_hw_fptr_tbl.l1_dwrr_rd_fptr(dev, &l1_out);
      *hw_wt = l1_out.l1_sch_cfg.dwrr_wt;
    }
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_enable(bf_dev_id_t dev,
                                    bf_tm_eg_l1_t *l1,
                                    bf_dev_port_t *sw_port,
                                    bf_dev_port_t *hw_port,
                                    bool *sw_enable,
                                    bool *hw_enable) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_port) {
    *sw_port = MAKE_DEV_PORT(l1->l_pipe, l1->uport);
  }

  if (sw_enable) {
    *sw_enable = l1->l1_sch_cfg.sch_enabled;
  }

  bf_tm_eg_l1_t l1_out;
  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);

    if (hw_port) {
      *hw_port = lld_sku_map_devport_from_device_to_user(
          dev,
          MAKE_DEV_PORT(l1_out.l_pipe,
                        (g_tm_ctx[dev]->tm_cfg.ports_per_pg * l1_out.pg) +
                            l1_out.l1_sch_cfg.cid));
    }

    if (hw_enable) {
      *hw_enable = l1_out.l1_sch_cfg.sch_enabled;
    }
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_guaranteed_rate_enable(bf_dev_id_t dev,
                                                    bf_tm_eg_l1_t *l1,
                                                    bool *sw_enable,
                                                    bool *hw_enable) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_enable) {
    *sw_enable = l1->l1_sch_cfg.min_rate_enable;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_enable && TM_IS_TARGET_ASIC(dev) &&
      g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);
    *hw_enable = l1_out.l1_sch_cfg.min_rate_enable;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_max_shaping_rate_enable(bf_dev_id_t dev,
                                                     bf_tm_eg_l1_t *l1,
                                                     bool *sw_enable,
                                                     bool *hw_enable) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_enable) {
    *sw_enable = l1->l1_sch_cfg.max_rate_enable;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_enable && TM_IS_TARGET_ASIC(dev) &&
      g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);
    *hw_enable = l1_out.l1_sch_cfg.max_rate_enable;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_priority_prop_enable(bf_dev_id_t dev,
                                                  bf_tm_eg_l1_t *l1,
                                                  bool *sw_enable,
                                                  bool *hw_enable) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_enable) {
    *sw_enable = l1->l1_sch_cfg.pri_prop;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_enable && TM_IS_TARGET_ASIC(dev) &&
      g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);
    *hw_enable = l1_out.l1_sch_cfg.pri_prop;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_priority(bf_dev_id_t dev,
                                      bf_tm_eg_l1_t *l1,
                                      bf_tm_sched_prio_t *sw_prio,
                                      bf_tm_sched_prio_t *hw_prio) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_prio) {
    *sw_prio = l1->l1_sch_cfg.min_rate_sch_prio;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_prio && TM_IS_TARGET_ASIC(dev) &&
      g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);
    *hw_prio = l1_out.l1_sch_cfg.min_rate_sch_prio;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_sched_prio(bf_dev_id_t dev,
                                        bf_tm_eg_l1_t *l1,
                                        bf_tm_sched_prio_t *sw_prio,
                                        bf_tm_sched_prio_t *hw_prio) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_prio) {
    *sw_prio = l1->l1_sch_cfg.max_rate_sch_prio;
  }

  bf_tm_eg_l1_t l1_out;
  if (hw_prio && TM_IS_TARGET_ASIC(dev) &&
      g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &l1_out);
    *hw_prio = l1_out.l1_sch_cfg.max_rate_sch_prio;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_guaranteed_rate(bf_dev_id_t dev,
                                             bf_tm_eg_l1_t *l1,
                                             bool *sw_pps,
                                             bool *hw_pps,
                                             uint32_t *sw_burst_size,
                                             uint32_t *hw_burst_size,
                                             uint32_t *sw_rate,
                                             uint32_t *hw_rate) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_pps) {
    *sw_pps = l1->l1_sch_cfg.pps;
  }

  if (sw_burst_size) {
    *sw_burst_size = l1->l1_sch_cfg.min_burst_size;
  }

  if (sw_rate) {
    *sw_rate = l1->l1_sch_cfg.min_rate;
  }

  bf_tm_eg_l1_t l1_out;
  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.l1_gmin_shaper_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_gmin_shaper_rd_fptr(dev, &l1_out);

    if (hw_pps) {
      *hw_pps = l1_out.l1_sch_cfg.pps;
    }

    if (hw_burst_size) {
      *hw_burst_size = l1_out.l1_sch_cfg.min_burst_size;
    }

    if (hw_rate) {
      *hw_rate = l1_out.l1_sch_cfg.min_rate;
    }
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_shaping_rate(bf_dev_id_t dev,
                                          bf_tm_eg_l1_t *l1,
                                          bool *sw_pps,
                                          bool *hw_pps,
                                          uint32_t *sw_burst_size,
                                          uint32_t *hw_burst_size,
                                          uint32_t *sw_rate,
                                          uint32_t *hw_rate) {
  bf_status_t rc = BF_SUCCESS;

  if (sw_pps) {
    *sw_pps = l1->l1_sch_cfg.pps;
  }

  if (sw_burst_size) {
    *sw_burst_size = l1->l1_sch_cfg.max_burst_size;
  }

  if (sw_rate) {
    *sw_rate = l1->l1_sch_cfg.max_rate;
  }

  bf_tm_eg_l1_t l1_out;
  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.l1_shaper_rd_fptr) {
    memcpy(&l1_out, l1, sizeof(l1_out));
    rc = g_sch_hw_fptr_tbl.l1_shaper_rd_fptr(dev, &l1_out);

    if (hw_pps) {
      *hw_pps = l1_out.l1_sch_cfg.pps;
    }

    if (hw_burst_size) {
      *hw_burst_size = l1_out.l1_sch_cfg.max_burst_size;
    }

    if (hw_rate) {
      *hw_rate = l1_out.l1_sch_cfg.max_rate;
    }
  }

  return (rc);
}

bf_status_t bf_tm_sch_set_l1_max_rate(bf_dev_id_t dev,
                                      bf_tm_eg_l1_t *l1,
                                      bool pps,
                                      uint32_t burst_size,
                                      uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pps, l1->l1_sch_cfg.pps, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(
          burst_size, l1->l1_sch_cfg.max_burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(rate, l1->l1_sch_cfg.max_rate, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.max_rate = rate;
  l1->l1_sch_cfg.max_burst_size = burst_size;
  l1->l1_sch_cfg.pps = pps;

  if (g_sch_hw_fptr_tbl.l1_set_sch_rate_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_rate_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_l1_max_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          true, l1->l1_sch_cfg.max_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.max_rate_enable = true;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_l1_max_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          false, l1->l1_sch_cfg.max_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.max_rate_enable = false;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_l1_gmin_rate(bf_dev_id_t dev,
                                       bf_tm_eg_l1_t *l1,
                                       bool pps,
                                       uint32_t burst_size,
                                       uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(pps, l1->l1_sch_cfg.pps, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(
          burst_size, l1->l1_sch_cfg.min_burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(rate, l1->l1_sch_cfg.min_rate, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.min_rate = rate;
  l1->l1_sch_cfg.min_burst_size = burst_size;
  l1->l1_sch_cfg.pps = pps;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_min_rate_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_l1_min_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          true, l1->l1_sch_cfg.min_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.min_rate_enable = true;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_l1_min_rate(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(
          false, l1->l1_sch_cfg.min_rate_enable, g_tm_ctx[dev])) {
    return (rc);
  }
  l1->l1_sch_cfg.min_rate_enable = false;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_l1_pri_prop(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  l1->l1_sch_cfg.pri_prop = true;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_l1_pri_prop(bf_dev_id_t dev, bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;

  l1->l1_sch_cfg.pri_prop = false;

  if (g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }
  return (rc);
}

bf_status_t bf_tm_sch_enable_port_max_rate(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(true, p->max_rate_enabled, g_tm_ctx[dev])) {
    return (rc);
  }
  p->max_rate_enabled = true;

  if (g_sch_hw_fptr_tbl.port_max_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.port_max_rate_enable_status_wr_fptr(dev, p);
  }
  return (rc);
}

bf_status_t bf_tm_sch_disable_port_max_rate(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(false, p->max_rate_enabled, g_tm_ctx[dev])) {
    return (rc);
  }
  p->max_rate_enabled = false;

  if (g_sch_hw_fptr_tbl.port_max_rate_enable_status_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.port_max_rate_enable_status_wr_fptr(dev, p);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_port_max_rate_enable_status(bf_dev_id_t dev,
                                                      bf_tm_port_t *port,
                                                      bool *sw_status,
                                                      bool *hw_status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_status = port->max_rate_enabled;

  if (TM_IS_TARGET_ASIC(dev) && hw_status &&
      g_sch_hw_fptr_tbl.port_max_rate_enable_status_rd_fptr) {
    memcpy(&out_p, port, sizeof(out_p));
    rc = g_sch_hw_fptr_tbl.port_max_rate_enable_status_rd_fptr(dev, &out_p);
    *hw_status = out_p.max_rate_enabled;
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_port_max_rate(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        bool pps,
                                        uint32_t burst_size,
                                        uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(rate, p->port_rate, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(burst_size, p->burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(pps, p->pps, g_tm_ctx[dev])) {
    return (rc);
  }
  p->port_rate = rate;
  p->burst_size = burst_size;
  p->pps = pps;
  p->sch_prov_type = BF_TM_SCH_RATE_UPPER;

  if (g_sch_hw_fptr_tbl.port_shaper_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.port_shaper_wr_fptr(dev, p);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_port_max_rate_provisioning(
    bf_dev_id_t dev,
    bf_tm_port_t *p,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(rate, p->port_rate, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(burst_size, p->burst_size, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(pps, p->pps, g_tm_ctx[dev])) {
    return (rc);
  }
  p->port_rate = rate;
  p->burst_size = burst_size;
  p->pps = pps;
  p->sch_prov_type = prov_type;

  if (g_sch_hw_fptr_tbl.port_shaper_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.port_shaper_wr_fptr(dev, p);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_port_max_rate(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        bool *pps,
                                        uint32_t *sw_burst_size,
                                        uint32_t *hw_burst_size,
                                        uint32_t *sw_rate,
                                        uint32_t *hw_rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_rate = p->port_rate;
  *sw_burst_size = p->burst_size;
  *pps = p->pps;

  if (TM_IS_TARGET_ASIC(dev) && hw_rate &&
      g_sch_hw_fptr_tbl.port_shaper_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_sch_hw_fptr_tbl.port_shaper_rd_fptr(dev, &out_p);
    *hw_rate = out_p.port_rate;
    *hw_burst_size = out_p.burst_size;
    *pps = out_p.pps;
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_port_max_rate_provisioning(
    bf_dev_id_t dev,
    bf_tm_port_t *p,
    bool *pps,
    uint32_t *sw_burst_size,
    uint32_t *hw_burst_size,
    uint32_t *sw_rate,
    uint32_t *hw_rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_rate = p->port_rate;
  *sw_burst_size = p->burst_size;
  *pps = p->pps;

  if (prov_type) {
    *prov_type = p->sch_prov_type;
  }

  if (TM_IS_TARGET_ASIC(dev) && hw_rate &&
      g_sch_hw_fptr_tbl.port_shaper_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_sch_hw_fptr_tbl.port_shaper_rd_fptr(dev, &out_p);
    *hw_rate = out_p.port_rate;
    *hw_burst_size = out_p.burst_size;
    *pps = out_p.pps;
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_pkt_ifg_compensation(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  if (g_sch_hw_fptr_tbl.pkt_ifg_comp_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.pkt_ifg_comp_wr_fptr(
        dev, (void *)&(g_tm_ctx[dev]->pipes[pipe]));
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_pkt_ifg_compensation(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               uint8_t *sw_ifg,
                                               uint8_t *hw_ifg) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t eg_pipe;

  *sw_ifg = g_tm_ctx[dev]->pipes[pipe].ifg_compensation;

  if (TM_IS_TARGET_ASIC(dev) && hw_ifg &&
      g_sch_hw_fptr_tbl.pkt_ifg_comp_rd_fptr) {
    memcpy(&eg_pipe, &(g_tm_ctx[dev]->pipes[pipe]), sizeof(eg_pipe));
    rc = g_sch_hw_fptr_tbl.pkt_ifg_comp_rd_fptr(dev, (void *)&eg_pipe);
    *hw_ifg = eg_pipe.ifg_compensation;
  }
  return (rc);
}

bf_status_t bf_tm_sch_l1_get_port_nth(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_l1_node_t port_l1,
                                      bf_tm_eg_l1_t **l1) {
  if (!(DEV_PORT_VALIDATE(port))) {
    LOG_ERROR("Invalid dev=%d dev_port=%d", dev, port);
    return BF_INVALID_ARG;
  }

  bf_tm_eg_l1_t *cur_l1 = BF_TM_FIRST_L1_PTR_IN_PG(g_tm_ctx[dev], port);
  bf_dev_port_t l_port = DEV_PORT_TO_LOCAL_PORT(port);
  uint8_t l_pipe = DEV_PORT_TO_PIPE(port);
  bf_tm_l1_node_t l1_idx = 0;

  for (int i = 0; i < g_tm_ctx[dev]->tm_cfg.l1_per_pg; i++) {
    if (cur_l1->l_pipe != l_pipe) {
      LOG_ERROR(
          "Inconsistent %d-th l1 with pipe=%d pg=%d l1=%d for dev=%d "
          "dev_port=%d",
          i,
          cur_l1->l_pipe,
          cur_l1->pg,
          cur_l1->logical_l1,
          dev,
          port);
      return BF_UNEXPECTED;
    }
    if (cur_l1->in_use && cur_l1->uport == l_port && l1_idx == port_l1) {
      *l1 = cur_l1;
      return BF_SUCCESS;
    } else if (cur_l1->in_use && cur_l1->uport == l_port) {
      l1_idx++;
    }
    cur_l1++;
  }
  return BF_OBJECT_NOT_FOUND;
}

bf_status_t bf_tm_sch_l1_set_port_default(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_eg_l1_t **l1) {
  if (!BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type) &&
      !BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    return BF_SUCCESS;
  }

  if (!(DEV_PORT_VALIDATE(port))) {
    LOG_ERROR("Invalid dev=%d dev_port=%d", dev, port);
    return BF_INVALID_ARG;
  }

  bf_tm_eg_l1_t *cur_l1 = BF_TM_FIRST_L1_PTR_IN_PG(g_tm_ctx[dev], port);
  bf_dev_port_t l_port = DEV_PORT_TO_LOCAL_PORT(port);
  uint8_t l_pipe = DEV_PORT_TO_PIPE(port);

  for (int i = 0; i < g_tm_ctx[dev]->tm_cfg.l1_per_pg; i++) {
    if (cur_l1->l_pipe != l_pipe) {
      LOG_ERROR(
          "Inconsistent %d-th l1 with pipe=%d pg=%d l1=%d for dev=%d "
          "dev_port=%d",
          i,
          cur_l1->l_pipe,
          cur_l1->pg,
          cur_l1->logical_l1,
          dev,
          port);
      return BF_UNEXPECTED;
    }
    if (cur_l1->in_use && cur_l1->uport == l_port) {
      if (l1) *l1 = cur_l1;
      return BF_SUCCESS;
    }
    cur_l1++;
  }

  // there's nothing assigned to this port
  bf_tm_l1_node_t l_l1 =
      DEV_PORT_TO_LOCAL_PORT(
          lld_sku_map_devport_from_user_to_device(dev, port)) %
      g_tm_ctx[dev]->tm_cfg.ports_per_pg;
  bf_status_t rc = bf_tm_sch_l1_get_descriptor(dev, port, l_l1, &cur_l1);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  if (cur_l1->in_use) return BF_NO_SYS_RESOURCES;

  rc = bf_tm_sch_l1_enable(dev, port, cur_l1);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  if (l1) *l1 = cur_l1;
  return rc;
}

bf_status_t bf_tm_sch_set_l1_stats_from_q(bf_dev_id_t dev,
                                          bf_tm_eg_q_t *q,
                                          bf_tm_eg_l1_t *l1) {
  if (!BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type) &&
      !BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    return BF_SUCCESS;
  }
  bool delta = false;
  if (q->q_sch_cfg.dwrr_wt > l1->l1_sch_cfg.dwrr_wt) {
    l1->l1_sch_cfg.dwrr_wt = q->q_sch_cfg.dwrr_wt;
    delta = true;
  }
  if (q->q_sch_cfg.pps > l1->l1_sch_cfg.pps) {
    l1->l1_sch_cfg.pps = q->q_sch_cfg.pps;
    delta = true;
  }
  if (q->q_sch_cfg.max_rate_sch_prio > l1->l1_sch_cfg.max_rate_sch_prio) {
    l1->l1_sch_cfg.max_rate_sch_prio = q->q_sch_cfg.max_rate_sch_prio;
    delta = true;
  }
  if (q->q_sch_cfg.min_rate_sch_prio > l1->l1_sch_cfg.min_rate_sch_prio) {
    l1->l1_sch_cfg.min_rate_sch_prio = q->q_sch_cfg.min_rate_sch_prio;
    delta = true;
  }
  if (q->q_sch_cfg.max_rate > l1->l1_sch_cfg.max_rate) {
    l1->l1_sch_cfg.max_rate = q->q_sch_cfg.max_rate;
    delta = true;
  }
  if (q->q_sch_cfg.min_rate > l1->l1_sch_cfg.min_rate) {
    l1->l1_sch_cfg.min_rate = q->q_sch_cfg.min_rate;
    delta = true;
  }
  if (q->q_sch_cfg.min_rate_enable != l1->l1_sch_cfg.min_rate_enable) {
    l1->l1_sch_cfg.min_rate_enable = q->q_sch_cfg.min_rate_enable;
    delta = true;
  }
  if (q->q_sch_cfg.max_rate_enable != l1->l1_sch_cfg.max_rate_enable) {
    l1->l1_sch_cfg.max_rate_enable = q->q_sch_cfg.max_rate_enable;
    delta = true;
  }
  if (q->q_sch_cfg.max_burst_size > l1->l1_sch_cfg.max_burst_size) {
    l1->l1_sch_cfg.max_burst_size = q->q_sch_cfg.max_burst_size;
    delta = true;
  }
  if (q->q_sch_cfg.min_burst_size > l1->l1_sch_cfg.min_burst_size) {
    l1->l1_sch_cfg.min_burst_size = q->q_sch_cfg.min_burst_size;
    delta = true;
  }

  bf_status_t rc = BF_SUCCESS;
  if (delta && g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr(dev, l1);
  }

  return rc;
}

bf_status_t bf_tm_sch_set_q_sched(bf_dev_id_t dev,
                                  bf_tm_eg_q_t *q,
                                  bool enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1 = q->l1;

  if (TM_HITLESS_IS_CFG_MATCH(
          enable, q->q_sch_cfg.sch_enabled, g_tm_ctx[dev])) {
    return (rc);
  }
  q->q_sch_cfg.sch_enabled = enable;

  if (!BF_TM_IS_TOFINO(g_tm_ctx[dev]->asic_type) && enable) {
    if (!l1) {
      bf_dev_port_t q_port = MAKE_DEV_PORT((q->l_pipe), (q->uport));
      rc = bf_tm_sch_l1_get_port_nth(dev, q_port, 0, &l1);
      if (rc == BF_OBJECT_NOT_FOUND) {
        // an L1 hasnt been assigned to the queue's port
        rc = bf_tm_sch_l1_set_port_default(dev, q_port, &l1);
      }
    }
    if (rc != BF_SUCCESS) return rc;

    q->l1 = l1;
    q->q_sch_cfg.cid = l1->logical_l1;

    rc = bf_tm_sch_set_l1_stats_from_q(dev, q, l1);
    if (rc != BF_SUCCESS) return rc;
  }

  if (g_sch_hw_fptr_tbl.q_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_set_sch_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_sched(bf_dev_id_t dev,
                                  bf_tm_eg_q_t *q,
                                  bool *sw_status,
                                  bool *hw_status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  *sw_status = q->q_sch_cfg.sch_enabled;

  if (TM_IS_TARGET_ASIC(dev) && hw_status &&
      g_sch_hw_fptr_tbl.q_get_sch_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_get_sch_rd_fptr(dev, &out_q);
    *hw_status = out_q.q_sch_cfg.sch_enabled;
  }
  return (rc);
}

bf_status_t bf_tm_sch_q_l1_get(bf_dev_id_t dev,
                               bf_tm_eg_q_t *q,
                               bf_tm_l1_node_t *sw_l1_node,
                               bf_tm_l1_node_t *hw_l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if (sw_l1_node) {
    *sw_l1_node = q->q_sch_cfg.cid;
  }

  if (TM_IS_TARGET_ASIC(dev) && hw_l1_node &&
      g_sch_hw_fptr_tbl.q_get_sch_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_get_sch_rd_fptr(dev, &out_q);
    *hw_l1_node = out_q.q_sch_cfg.cid;
  }
  return (rc);
}

bf_status_t bf_tm_sch_q_l1_set(bf_dev_id_t dev,
                               bf_tm_eg_q_t *q,
                               bf_tm_eg_l1_t *l1) {
  bf_status_t rc = BF_SUCCESS;
  if (!l1->in_use) {
    LOG_ERROR(
        "Unable to set queue to l1 node. l1 is not assigned to a port."
        "Device = %d "
        "Queue port = %d ",
        dev,
        q->uport);
    return BF_INVALID_ARG;
  }
  if (q->uport != l1->uport) {
    LOG_ERROR(
        "Unable to set queue to l1 node. q is assigned to differnt port."
        "Device = %d "
        "Queue port = %d "
        "L1 port = %d",
        dev,
        q->uport,
        l1->uport);
    return BF_INVALID_ARG;
  }

  q->l1 = l1;
  q->q_sch_cfg.cid = l1->logical_l1;

  if (g_sch_hw_fptr_tbl.q_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_set_sch_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_q_l1_reset(bf_dev_id_t dev, bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  bf_dev_port_t q_port = MAKE_DEV_PORT((q->l_pipe), (q->uport));
  rc = bf_tm_sch_l1_get_port_nth(dev, q_port, 0, &l1);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  q->l1 = l1;
  q->q_sch_cfg.cid = l1->logical_l1;

  if (g_sch_hw_fptr_tbl.q_set_sch_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_set_sch_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_port_sched(bf_dev_id_t dev,
                                     bf_tm_port_t *port,
                                     bf_port_speeds_t speed,
                                     bool enable) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(enable, port->sch_enabled, g_tm_ctx[dev]) &&
      TM_HITLESS_IS_CFG_MATCH(speed, port->speed, g_tm_ctx[dev])) {
    return (rc);
  }

  int num_ch = 1;
  bf_port_speeds_t ch_select_speed = speed;
  if (speed == BF_SPEED_NONE) {
    // If we're removing this port, update all sub-channels in mac.
    ch_select_speed = port->speed;
  }

  if (BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    switch (ch_select_speed) {
      case BF_SPEED_400G:
        if (port->port % 4 != 0) {
          rc = BF_IN_USE;
          break;
        }
        num_ch = 4;
        break;

      case BF_SPEED_200G:
        if (port->port % 2 != 0) {
          rc = BF_IN_USE;
          break;
        }
        num_ch = 2;
        break;

      default:
        break;
    }

    if (BF_SUCCESS != rc && BF_SPEED_NONE == speed && !enable) {
      // Don't error on disable if called for a sub-channel alone,
      // with incorrect speed set in memory if the port is not added yet.
      num_ch = 1;
      rc = BF_SUCCESS;
    }

  } else {
    switch (ch_select_speed) {
      case BF_SPEED_400G:
        if (port->port % 8 != 0) {
          rc = BF_IN_USE;
          break;
        }
        num_ch = 8;
        break;

      case BF_SPEED_200G:
        if (port->port % 4 != 0) {
          rc = BF_IN_USE;
          break;
        }
        num_ch = 4;
        break;

      case BF_SPEED_100G:
        if (port->port % 2 != 0) {
          rc = BF_IN_USE;
          break;
        }
        num_ch = 2;
        break;

      default:
        break;
    }
  }

  LOG_DBG(
      "%s:%d dev=%d l_pipe=%d uport=%d speed=%d(%d on add) set %d channels to "
      "speed=%d, rc=%d",
      __func__,
      __LINE__,
      dev,
      port->l_pipe,
      port->uport,
      speed,
      port->speed_on_add,
      num_ch,
      ch_select_speed,
      rc);

  if (BF_SUCCESS != rc) {
    return (rc);
  }

  for (int i = 0; i < num_ch; i++) {
    port->speed = speed;
    port->sch_enabled = enable;
    if (g_sch_hw_fptr_tbl.port_set_sch_wr_fptr) {
      rc |= g_sch_hw_fptr_tbl.port_set_sch_wr_fptr(dev, port);
    }
    port++;
  }

  return ((BF_SUCCESS != rc) ? BF_INTERNAL_ERROR : BF_SUCCESS);
}

bf_status_t bf_tm_sch_get_port_sched(bf_dev_id_t dev,
                                     bf_tm_port_t *port,
                                     bool *sw_status,
                                     bool *hw_status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_status = port->sch_enabled;

  if (TM_IS_TARGET_ASIC(dev) && hw_status &&
      g_sch_hw_fptr_tbl.port_get_sch_rd_fptr) {
    memcpy(&out_p, port, sizeof(out_p));
    rc = g_sch_hw_fptr_tbl.port_get_sch_rd_fptr(dev, &out_p);
    *hw_status = out_p.sch_enabled;
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_port_egress_pfc_status(bf_dev_id_t dev,
                                                 bf_tm_port_t *port,
                                                 uint8_t *sw_status,
                                                 uint8_t *hw_status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  if (TM_IS_TARGET_ASIC(dev) && hw_status &&
      g_sch_hw_fptr_tbl.eg_port_pfc_status_fptr) {
    memcpy(&out_p, port, sizeof(out_p));
    rc = g_sch_hw_fptr_tbl.eg_port_pfc_status_fptr(dev, &out_p, hw_status);
  }
  if (rc) {
    *sw_status = *hw_status;
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_egress_pfc_status(bf_dev_id_t dev,
                                              bf_tm_eg_q_t *q,
                                              bool *sw_status,
                                              bool *hw_status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if (TM_IS_TARGET_ASIC(dev) && hw_status &&
      g_sch_hw_fptr_tbl.eg_q_pfc_status_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.eg_q_pfc_status_fptr(dev, &out_q, hw_status);
  }
  if (rc) {
    *sw_status = *hw_status;
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_egress_pfc_status(bf_dev_id_t dev,
                                              bf_tm_eg_q_t *q,
                                              bool status) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.eg_q_pfc_set_status_fptr) {
    rc = g_sch_hw_fptr_tbl.eg_q_pfc_set_status_fptr(dev, q, status);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_sch_clear_q_egress_pfc_status(bf_dev_id_t dev,
                                                   bf_tm_eg_q_t *q) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.eg_q_pfc_clr_status_fptr) {
    rc = g_sch_hw_fptr_tbl.eg_q_pfc_clr_status_fptr(dev, q);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bool bf_tm_sch_cfg_verify_burst_size(bf_dev_id_t dev,
                                     uint32_t restore_burst_size,
                                     uint32_t orig_burst_size) {
  bool ret_val = true;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.burst_size_verify_fptr) {
    ret_val = g_sch_hw_fptr_tbl.burst_size_verify_fptr(
        dev, restore_burst_size, orig_burst_size);
  }

  return (ret_val);
}

bool bf_tm_sch_cfg_verify_rate(bf_dev_id_t dev,
                               uint32_t restore_rate,
                               uint32_t orig_rate,
                               bool pps) {
  bool ret_val = true;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.rate_verify_fptr) {
    ret_val =
        g_sch_hw_fptr_tbl.rate_verify_fptr(dev, restore_rate, orig_rate, pps);
  }

  return (ret_val);
}

bf_status_t bf_tm_sch_force_disable_port_sched(bf_dev_id_t dev,
                                               bf_tm_port_t *port) {
  bf_status_t rc = BF_SUCCESS;

  /* Do not update SW cache, disable SCH directly in HW */
  if (g_sch_hw_fptr_tbl.port_force_disable_sch_fptr) {
    rc = g_sch_hw_fptr_tbl.port_force_disable_sch_fptr(dev, port);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_adv_fc_mode_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool enable) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.adv_fc_mode_enable_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.adv_fc_mode_enable_wr_fptr(dev, pipe, enable);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_adv_fc_mode_enable(bf_dev_id_t dev,
                                             bf_tm_eg_pipe_t *pipe,
                                             bool *enable) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.adv_fc_mode_enable_rd_fptr) {
    rc = g_sch_hw_fptr_tbl.adv_fc_mode_enable_rd_fptr(dev, pipe, enable);
  }
  return (rc);
}

bf_status_t bf_tm_sch_set_q_adv_fc_mode(bf_dev_id_t dev,
                                        bf_tm_eg_q_t *q,
                                        bf_tm_sched_adv_fc_mode_t mode) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(mode, q->q_sch_cfg.adv_fc_mode, g_tm_ctx[dev])) {
    return (rc);
  }

  q->q_sch_cfg.adv_fc_mode = mode;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.q_adv_fc_mode_wr_fptr) {
    rc = g_sch_hw_fptr_tbl.q_adv_fc_mode_wr_fptr(dev, q);
  }
  return (rc);
}

bf_status_t bf_tm_sch_get_q_adv_fc_mode(
    bf_dev_id_t dev,
    bf_tm_eg_q_t *q,
    bf_tm_sched_adv_fc_mode_t *sw_adv_fc_mode,
    bf_tm_sched_adv_fc_mode_t *hw_adv_fc_mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t out_q;

  if (sw_adv_fc_mode) {
    *sw_adv_fc_mode = q->q_sch_cfg.adv_fc_mode;
  }

  if (TM_IS_TARGET_ASIC(dev) && hw_adv_fc_mode &&
      g_sch_hw_fptr_tbl.q_adv_fc_mode_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_sch_hw_fptr_tbl.q_adv_fc_mode_rd_fptr(dev, &out_q);
    *hw_adv_fc_mode = out_q.q_sch_cfg.adv_fc_mode;
  }

  return (rc);
}

bf_status_t bf_tm_sch_get_l1_sch(bf_dev_id_t dev, bf_tm_eg_l1_t *l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t out_l1;

  if (TM_IS_TARGET_ASIC(dev) && g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr) {
    memcpy(&out_l1, l1_node, sizeof(out_l1));
    rc = g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr(dev, &out_l1);
    memcpy(l1_node, &out_l1, sizeof(bf_tm_eg_l1_t));
  }
  return (rc);
}

bf_tm_status_t bf_tm_sch_q_get_defaults(bf_dev_id_t devid,
                                        bf_tm_port_t *p,
                                        bf_tm_eg_q_t *q,
                                        bf_tm_sch_q_defaults_t *def) {
  BF_TM_INVALID_ARG(def == NULL);
  if (g_sch_hw_fptr_tbl.sch_q_defaults_rd_fptr) {
    return g_sch_hw_fptr_tbl.sch_q_defaults_rd_fptr(devid, p, q, def);
  }
  return BF_NOT_SUPPORTED;
}

bf_tm_status_t bf_tm_sch_port_get_defaults(bf_dev_id_t devid,
                                           bf_tm_port_t *p,
                                           bf_tm_sch_port_defaults_t *def) {
  BF_TM_INVALID_ARG(def == NULL);
  if (g_sch_hw_fptr_tbl.sch_port_defaults_rd_fptr) {
    return g_sch_hw_fptr_tbl.sch_port_defaults_rd_fptr(devid, p, def);
  }
  return BF_NOT_SUPPORTED;
}

bf_tm_status_t bf_tm_sch_l1_get_defaults(bf_dev_id_t devid,
                                         bf_tm_port_t *p,
                                         bf_tm_eg_l1_t *l1,
                                         bf_tm_sch_l1_defaults_t *def) {
  BF_TM_INVALID_ARG(def == NULL);
  if (g_sch_hw_fptr_tbl.sch_l1_defaults_rd_fptr) {
    return g_sch_hw_fptr_tbl.sch_l1_defaults_rd_fptr(devid, p, l1, def);
  }
  return BF_NOT_SUPPORTED;
}

#define BF_TM_SCH_HW_FTBL_WR_FUNCS(q_prio,                                    \
                                   q_wt,                                      \
                                   q_pfc_prio,                                \
                                   q_rate,                                    \
                                   q_min_rate,                                \
                                   q_max_rate_enable_status,                  \
                                   q_min_rate_enable_status,                  \
                                   port_max_rate_enable_status,               \
                                   port_rate,                                 \
                                   pkt_ifg,                                   \
                                   q_sched,                                   \
                                   l1_sched,                                  \
                                   l1_sched_wt,                               \
                                   l1_rate,                                   \
                                   l1_min_rate,                               \
                                   port_sched,                                \
                                   port_force_disable_sched,                  \
                                   q_adv_fc_mode,                             \
                                   sch_adv_fc_mode_enable,                    \
                                   eg_q_pfc_set_status,                       \
                                   eg_q_pfc_clr_status)                       \
  {                                                                           \
    g_sch_hw_fptr_tbl.q_sch_prio_wr_fptr = q_prio;                            \
    g_sch_hw_fptr_tbl.q_dwrr_wr_fptr = q_wt;                                  \
    g_sch_hw_fptr_tbl.q_pfc_prio_wr_fptr = q_pfc_prio;                        \
    g_sch_hw_fptr_tbl.q_shaper_wr_fptr = q_rate;                              \
    g_sch_hw_fptr_tbl.q_gmin_shaper_wr_fptr = q_min_rate;                     \
    g_sch_hw_fptr_tbl.q_max_rate_enable_status_wr_fptr =                      \
        q_max_rate_enable_status;                                             \
    g_sch_hw_fptr_tbl.q_min_rate_enable_status_wr_fptr =                      \
        q_min_rate_enable_status;                                             \
    g_sch_hw_fptr_tbl.port_max_rate_enable_status_wr_fptr =                   \
        port_max_rate_enable_status;                                          \
    g_sch_hw_fptr_tbl.port_shaper_wr_fptr = port_rate;                        \
    g_sch_hw_fptr_tbl.pkt_ifg_comp_wr_fptr = pkt_ifg;                         \
    g_sch_hw_fptr_tbl.q_set_sch_wr_fptr = q_sched;                            \
    g_sch_hw_fptr_tbl.l1_set_sch_wr_fptr = l1_sched;                          \
    g_sch_hw_fptr_tbl.l1_dwrr_wr_fptr = l1_sched_wt;                          \
    g_sch_hw_fptr_tbl.l1_set_sch_rate_wr_fptr = l1_rate;                      \
    g_sch_hw_fptr_tbl.l1_set_sch_min_rate_wr_fptr = l1_min_rate;              \
    g_sch_hw_fptr_tbl.port_set_sch_wr_fptr = port_sched;                      \
    g_sch_hw_fptr_tbl.port_force_disable_sch_fptr = port_force_disable_sched; \
    g_sch_hw_fptr_tbl.q_adv_fc_mode_wr_fptr = q_adv_fc_mode;                  \
    g_sch_hw_fptr_tbl.adv_fc_mode_enable_wr_fptr = sch_adv_fc_mode_enable;    \
    g_sch_hw_fptr_tbl.eg_q_pfc_set_status_fptr = eg_q_pfc_set_status;         \
    g_sch_hw_fptr_tbl.eg_q_pfc_clr_status_fptr = eg_q_pfc_clr_status;         \
  }

#define BF_TM_SCH_HW_FTBL_RD_FUNCS(q_prio,                                 \
                                   q_wt,                                   \
                                   q_pfc_prio,                             \
                                   q_rate,                                 \
                                   q_min_rate,                             \
                                   q_max_rate_enable_status,               \
                                   q_min_rate_enable_status,               \
                                   port_max_rate_enable_status,            \
                                   port_rate,                              \
                                   pkt_ifg,                                \
                                   q_sched,                                \
                                   port_sched,                             \
                                   eg_port_pfc_status,                     \
                                   eg_q_pfc_status,                        \
                                   burst_size_verify,                      \
                                   rate_verify,                            \
                                   q_adv_fc_mode,                          \
                                   sch_adv_fc_mode_enable,                 \
                                   l1_wt,                                  \
                                   l1_sched,                               \
                                   l1_rate,                                \
                                   l1_min_rate,                            \
                                   q_speed,                                \
                                   q_def_vals,                             \
                                   port_def_vals,                          \
                                   l1_def_vals)                            \
  {                                                                        \
    g_sch_hw_fptr_tbl.q_sch_prio_rd_fptr = q_prio;                         \
    g_sch_hw_fptr_tbl.q_dwrr_rd_fptr = q_wt;                               \
    g_sch_hw_fptr_tbl.q_pfc_prio_rd_fptr = q_pfc_prio;                     \
    g_sch_hw_fptr_tbl.q_shaper_rd_fptr = q_rate;                           \
    g_sch_hw_fptr_tbl.q_gmin_shaper_rd_fptr = q_min_rate;                  \
    g_sch_hw_fptr_tbl.q_min_rate_enable_status_rd_fptr =                   \
        q_max_rate_enable_status;                                          \
    g_sch_hw_fptr_tbl.q_max_rate_enable_status_rd_fptr =                   \
        q_min_rate_enable_status;                                          \
    g_sch_hw_fptr_tbl.port_max_rate_enable_status_rd_fptr =                \
        port_max_rate_enable_status;                                       \
    g_sch_hw_fptr_tbl.port_shaper_rd_fptr = port_rate;                     \
    g_sch_hw_fptr_tbl.pkt_ifg_comp_rd_fptr = pkt_ifg;                      \
    g_sch_hw_fptr_tbl.q_get_sch_rd_fptr = q_sched;                         \
    g_sch_hw_fptr_tbl.port_get_sch_rd_fptr = port_sched;                   \
    g_sch_hw_fptr_tbl.eg_port_pfc_status_fptr = eg_port_pfc_status;        \
    g_sch_hw_fptr_tbl.eg_q_pfc_status_fptr = eg_q_pfc_status;              \
    g_sch_hw_fptr_tbl.burst_size_verify_fptr = burst_size_verify;          \
    g_sch_hw_fptr_tbl.rate_verify_fptr = rate_verify;                      \
    g_sch_hw_fptr_tbl.q_adv_fc_mode_rd_fptr = q_adv_fc_mode;               \
    g_sch_hw_fptr_tbl.adv_fc_mode_enable_rd_fptr = sch_adv_fc_mode_enable; \
    g_sch_hw_fptr_tbl.l1_dwrr_rd_fptr = l1_wt;                             \
    g_sch_hw_fptr_tbl.l1_get_sch_rd_fptr = l1_sched;                       \
    g_sch_hw_fptr_tbl.l1_shaper_rd_fptr = l1_rate;                         \
    g_sch_hw_fptr_tbl.l1_gmin_shaper_rd_fptr = l1_min_rate;                \
    g_sch_hw_fptr_tbl.q_speed_rd_fptr = q_speed;                           \
    g_sch_hw_fptr_tbl.sch_q_defaults_rd_fptr = q_def_vals;                 \
    g_sch_hw_fptr_tbl.sch_port_defaults_rd_fptr = port_def_vals;           \
    g_sch_hw_fptr_tbl.sch_l1_defaults_rd_fptr = l1_def_vals;               \
  }

void bf_tm_sch_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_SCH_HW_FTBL_WR_FUNCS(NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
  BF_TM_SCH_HW_FTBL_RD_FUNCS(NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
}

static void bf_tm_sch_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    // Set both q-sched normal prio and
    // remaining-bw prio
    BF_TM_SCH_HW_FTBL_WR_FUNCS(bf_tm_tofino_sch_set_q_priority,
                               bf_tm_tofino_sch_set_q_wt,
                               bf_tm_tofino_sch_set_q_pfc_prio,
                               bf_tm_tofino_sch_set_q_rate,
                               bf_tm_tofino_sch_set_q_min_rate,
                               bf_tm_tofino_sch_set_q_max_rate_enable_status,
                               bf_tm_tofino_sch_set_q_min_rate_enable_status,
                               bf_tm_tofino_sch_set_port_max_rate_enable_status,
                               bf_tm_tofino_sch_set_port_rate,
                               bf_tm_tofino_sch_set_pkt_ifg,
                               bf_tm_tofino_sch_set_q_sched,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               bf_tm_tofino_sch_set_port_sched,
                               bf_tm_tofino_sch_force_disable_port_sched,
                               NULL,
                               NULL,
                               bf_tm_tofino_sch_set_q_egress_pfc_status,
                               bf_tm_tofino_sch_clear_q_egress_pfc_status);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    // Set both q-sched normal prio and
    // remaining-bw prio
    BF_TM_SCH_HW_FTBL_WR_FUNCS(bf_tm_tof2_sch_set_q_priority,
                               bf_tm_tof2_sch_set_q_wt,
                               bf_tm_tof2_sch_set_q_pfc_prio,
                               bf_tm_tof2_sch_set_q_rate,
                               bf_tm_tof2_sch_set_q_min_rate,
                               bf_tm_tof2_sch_set_q_max_rate_enable_status,
                               bf_tm_tof2_sch_set_q_min_rate_enable_status,
                               bf_tm_tof2_sch_set_port_sched,
                               bf_tm_tof2_sch_set_port_rate,
                               bf_tm_tof2_sch_set_pkt_ifg,
                               bf_tm_tof2_sch_set_q_sched,
                               bf_tm_tof2_sch_set_l1_sched,
                               bf_tm_tof2_sch_set_l1_wt,
                               bf_tm_tof2_sch_set_l1_rate,
                               bf_tm_tof2_sch_set_l1_min_rate,
                               bf_tm_tof2_sch_set_port_sched,
                               NULL,
                               bf_tm_tof2_sch_set_q_adv_fc_mode,
                               bf_tm_tof2_sch_set_adv_fc_mode_enable,
                               bf_tm_tof2_sch_set_q_egress_pfc_status,
                               bf_tm_tof2_sch_clear_q_egress_pfc_status);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    // Set both q-sched normal prio and
    // remaining-bw prio
    BF_TM_SCH_HW_FTBL_WR_FUNCS(bf_tm_tof3_sch_set_q_priority,
                               bf_tm_tof3_sch_set_q_wt,
                               bf_tm_tof3_sch_set_q_pfc_prio,
                               bf_tm_tof3_sch_set_q_rate,
                               bf_tm_tof3_sch_set_q_min_rate,
                               bf_tm_tof3_sch_set_q_max_rate_enable_status,
                               bf_tm_tof3_sch_set_q_min_rate_enable_status,
                               bf_tm_tof3_sch_set_port_sched,
                               bf_tm_tof3_sch_set_port_rate,
                               bf_tm_tof3_sch_set_pkt_ifg,
                               bf_tm_tof3_sch_set_q_sched,
                               bf_tm_tof3_sch_set_l1_sched,
                               bf_tm_tof3_sch_set_l1_wt,
                               bf_tm_tof3_sch_set_l1_rate,
                               bf_tm_tof3_sch_set_l1_min_rate,
                               bf_tm_tof3_sch_set_port_sched,
                               NULL,
                               bf_tm_tof3_sch_set_q_adv_fc_mode,
                               bf_tm_tof3_sch_set_adv_fc_mode_enable,
                               bf_tm_tof3_sch_set_q_egress_pfc_status,
                               bf_tm_tof3_sch_clear_q_egress_pfc_status);
  }

























}

static void bf_tm_sch_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_SCH_HW_FTBL_RD_FUNCS(bf_tm_tofino_sch_get_q_priority,
                               bf_tm_tofino_sch_get_q_wt,
                               bf_tm_tofino_sch_get_q_pfc_prio,
                               bf_tm_tofino_sch_get_q_rate,
                               bf_tm_tofino_sch_get_q_min_rate,
                               bf_tm_tofino_sch_get_q_max_rate_enable_status,
                               bf_tm_tofino_sch_get_q_min_rate_enable_status,
                               bf_tm_tofino_sch_get_port_sched,
                               bf_tm_tofino_sch_get_port_rate,
                               bf_tm_tofino_sch_get_pkt_ifg,
                               bf_tm_tofino_sch_get_q_sched,
                               bf_tm_tofino_sch_get_port_sched,
                               bf_tm_tofino_sch_get_port_egress_pfc_status,
                               bf_tm_tofino_sch_get_q_egress_pfc_status,
                               bf_tm_tofino_sch_verify_burst_size,
                               bf_tm_tofino_sch_verify_rate,
                               NULL /*adv_fc_mode*/,
                               NULL /*adv_fc_mode_en*/,
                               NULL /*l1_wt*/,
                               NULL /*l1_sched*/,
                               NULL /*l1_rate*/,
                               NULL /*l1_min_rate*/,
                               NULL /*q_speed*/,
                               bf_tm_tofino_sch_q_get_defaults,
                               bf_tm_tofino_sch_port_get_defaults,
                               NULL /*sch_l1_get_defaults*/);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_SCH_HW_FTBL_RD_FUNCS(bf_tm_tof2_sch_get_q_priority,
                               bf_tm_tof2_sch_get_q_wt,
                               bf_tm_tof2_sch_get_q_pfc_prio,
                               bf_tm_tof2_sch_get_q_rate,
                               bf_tm_tof2_sch_get_q_min_rate,
                               bf_tm_tof2_sch_get_q_max_rate_enable_status,
                               bf_tm_tof2_sch_get_q_min_rate_enable_status,
                               bf_tm_tof2_sch_get_port_sched,
                               bf_tm_tof2_sch_get_port_rate,
                               bf_tm_tof2_sch_get_pkt_ifg,
                               bf_tm_tof2_sch_get_q_sched,
                               bf_tm_tof2_sch_get_port_sched,
                               bf_tm_tof2_sch_get_port_egress_pfc_status,
                               bf_tm_tof2_sch_get_q_egress_pfc_status,
                               bf_tm_tof2_sch_verify_burst_size,
                               bf_tm_tof2_sch_verify_rate,
                               bf_tm_tof2_sch_get_q_adv_fc_mode,
                               bf_tm_tof2_sch_get_adv_fc_mode_enable,
                               bf_tm_tof2_sch_get_l1_wt,
                               bf_tm_tof2_sch_get_l1_sched,
                               bf_tm_tof2_sch_get_l1_rate,
                               bf_tm_tof2_sch_get_l1_min_rate,
                               bf_tm_tof2_sch_get_q_speed,
                               bf_tm_tof2_sch_q_get_defaults,
                               bf_tm_tof2_sch_port_get_defaults,
                               bf_tm_tof2_sch_l1_get_defaults);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_SCH_HW_FTBL_RD_FUNCS(bf_tm_tof3_sch_get_q_priority,
                               bf_tm_tof3_sch_get_q_wt,
                               bf_tm_tof3_sch_get_q_pfc_prio,
                               bf_tm_tof3_sch_get_q_rate,
                               bf_tm_tof3_sch_get_q_min_rate,
                               bf_tm_tof3_sch_get_q_max_rate_enable_status,
                               bf_tm_tof3_sch_get_q_min_rate_enable_status,
                               bf_tm_tof3_sch_get_port_sched,
                               bf_tm_tof3_sch_get_port_rate,
                               bf_tm_tof3_sch_get_pkt_ifg,
                               bf_tm_tof3_sch_get_q_sched,
                               bf_tm_tof3_sch_get_port_sched,
                               bf_tm_tof3_sch_get_port_egress_pfc_status,
                               bf_tm_tof3_sch_get_q_egress_pfc_status,
                               bf_tm_tof3_sch_verify_burst_size,
                               bf_tm_tof3_sch_verify_rate,
                               bf_tm_tof3_sch_get_q_adv_fc_mode,
                               bf_tm_tof3_sch_get_adv_fc_mode_enable,
                               bf_tm_tof3_sch_get_l1_wt,
                               bf_tm_tof3_sch_get_l1_sched,
                               bf_tm_tof3_sch_get_l1_rate,
                               bf_tm_tof3_sch_get_l1_min_rate,
                               bf_tm_tof3_sch_get_q_speed,
                               bf_tm_tof3_sch_q_get_defaults,
                               bf_tm_tof3_sch_port_get_defaults,
                               bf_tm_tof3_sch_l1_get_defaults);
  }






























}

void bf_tm_sch_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_sch_set_hw_ftbl_rd_funcs(tm_ctx);
  bf_tm_sch_set_hw_ftbl_wr_funcs(tm_ctx);
}

void bf_tm_sch_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_sch_null_hw_ftbl(tm_ctx);
  bf_tm_sch_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_sch_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->eg_l1) {
    bf_sys_free(tm_ctx->eg_l1);
    tm_ctx->eg_l1 = NULL;
  }
}

bf_tm_status_t bf_tm_init_sch(bf_tm_dev_ctx_t *tm_ctx) {
  bf_dev_pipe_t pipe = 0;
  uint32_t num_pipes = 0;
  uint16_t physical_l1;
  bf_tm_eg_l1_t *l1;

  if (tm_ctx->eg_l1) {
    bf_sys_free(tm_ctx->eg_l1);
  }

  lld_sku_get_num_active_pipes(tm_ctx->devid, &num_pipes);

  if (BF_TM_IS_TOF2(tm_ctx->asic_type) || BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    tm_ctx->eg_l1 = bf_sys_calloc(
        sizeof(bf_tm_eg_l1_t),
        num_pipes * tm_ctx->tm_cfg.pg_per_pipe * tm_ctx->tm_cfg.l1_per_pg);

    l1 = tm_ctx->eg_l1;
    for (int i = 0; i < (int)num_pipes; i++) {
      if (lld_sku_map_pipe_id_to_phy_pipe_id(tm_ctx->devid, i, &pipe) !=
          LLD_OK) {
        LOG_ERROR(
            "Unable to map logical pipe to "
            "physical pipe id. Device = %d "
            "Logical "
            "pipe = %d",
            tm_ctx->devid,
            i);
      }
      bf_sys_assert(pipe < BF_TM_MAX_MAU_PIPES);
      physical_l1 = 0;
      for (int k = 0; k < tm_ctx->tm_cfg.pg_per_pipe; k++) {
        for (int j = 0; j < tm_ctx->tm_cfg.l1_per_pg; j++) {
          l1->l_pipe = i;
          l1->p_pipe = pipe;
          l1->logical_l1 = j;
          l1->physical_l1 = physical_l1++;
          l1->in_use = false;
          l1->l1_sch_cfg.pri_prop = false;
          l1->pg = k;
          l1->l1_sch_cfg.sch_enabled = true;
          // Disable shaping by default
          l1->l1_sch_cfg.max_rate_enable = false;
          l1->l1_sch_cfg.min_rate_enable = false;
          l1++;
        }
      }
    }
  } else {
    tm_ctx->eg_l1 = NULL;
  }

  bf_tm_sch_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}
