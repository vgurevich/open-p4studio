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
// to maange Queues in Egress TM

#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

static bf_tm_q_thres_hw_funcs_tbl g_q_thres_hw_fptr_tbl;
static bf_tm_q_cfg_hw_funcs_tbl g_q_cfg_hw_fptr_tbl;

bf_tm_status_t bf_tm_q_get_base_queue(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      int port_ch,
                                      int *base_q) {
  bf_tm_q_profile_t *q_prof;
  int prev_port_q_profile;

  if (port_ch == 0) {
    /* For channel 0, base queue is always 0 */
    *base_q = 0;
    return BF_TM_EOK;
  }

  /*
   * For rest of the channels, calculate base_queue based on the previous
   * channel/port's base queue and queue count in the same port group
   *
   * NOTE: Since the base queue is calculated based on the prev port's base
   * queue and queue count in the same port group, application should -
   *   - always call queue mapping APIs strictly in increasing order of ports
   *      within a port group
   *   - if queue count gets changed for a port/channel in a port group,
   *      then application should call the queue mapping APIs
   *      for rest of the ports after it (if present) in that port group in
   *      increasing order
   *   - if queue count gets changed, then calling this API while traffic
   *      running on any port within the port group is not recommended as
   *      traffic would be disrupted for a short period of time and buffer
   *      accounting would be inconsistent
   * The above restrictions are not applicable if just queue mapping gets
   * changed
   *
   */
  bf_tm_port_get_q_profile(
      dev, port - (1 * g_tm_ctx[dev]->tm_cfg.chnl_mult), &prev_port_q_profile);
  if (prev_port_q_profile < 0 ||
      prev_port_q_profile >= g_tm_ctx[dev]->tm_cfg.q_prof_cnt) {
    /* Should never hit this case for valid inputs */
    LOG_ERROR(
        "TM: invalid queue profile index for dev %d, "
        "port %d",
        dev,
        port - 1);

    return BF_INVALID_ARG;
  }
  q_prof = g_tm_ctx[dev]->q_profile + prev_port_q_profile;
  *base_q = q_prof->base_q + q_prof->q_count;

  return BF_TM_EOK;
}
static bf_tm_status_t bf_tm_q_create_q_profile(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               int q_count,
                                               uint8_t *q_map,
                                               uint8_t *prof_index) {
  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev]->q_profile;
  bf_tm_status_t sts;
  int i, port_ch, j;
  int base_q = 0;
  bool matched = false;

  // Get the channel number for this port
  port_ch = DEV_PORT_TO_LOCAL_PORT(
                lld_sku_map_devport_from_user_to_device(dev, port)) %
            g_tm_ctx[dev]->tm_cfg.ports_per_pg;

  // Get what base queue should be used for this port
  sts = bf_tm_q_get_base_queue(dev, port, port_ch, &base_q);
  if (sts != BF_TM_EOK) {
    return sts;
  }

  // Validate if queue carving config is within the limit
  if (base_q + q_count > g_tm_ctx[dev]->tm_cfg.q_per_pg) {
    LOG_ERROR(
        "TM: queue config exceeds total port group queue limit for dev %d, "
        "port %d",
        dev,
        port);
    return BF_INVALID_ARG;  // Should we return NO_RESOURCES???
  }

  // Check if any existing profile matches
  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.q_prof_cnt && !matched; i++, q_prof++) {
    if (!q_prof->in_use) {
      continue;
    }

    /*
     * Skip the queue profile if queue count or channel or base queue
     * is not matching
     */
    if (q_prof->q_count != q_count || q_prof->ch_in_pg != port_ch ||
        q_prof->base_q != base_q) {
      continue;
    }
    // Check if q-mapping match
    for (j = 0; j < g_tm_ctx[dev]->tm_cfg.q_per_pg; j++) {
      if (q_map[j] != q_prof->q_mapping[j]) {
        break;
      } else {
        continue;
      }
    }
    if (j >= g_tm_ctx[dev]->tm_cfg.q_per_pg) {
      // match
      matched = true;
      *prof_index = i;
    }
  }
  if (!matched) {
    // we have to create a new one
    if (g_tm_ctx[dev]->q_profile_use_cnt >= g_tm_ctx[dev]->tm_cfg.q_prof_cnt) {
      return (BF_NO_SYS_RESOURCES);
    }
    q_prof = g_tm_ctx[dev]->q_profile;
    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.q_prof_cnt; i++, q_prof++) {
      if (!q_prof->in_use) {
        break;
      }
    }
    if (i >= g_tm_ctx[dev]->tm_cfg.q_prof_cnt) {
      // Shouldn't hit this case.
    }
    g_tm_ctx[dev]->q_profile_use_cnt++;
    *prof_index = i;
    q_prof->in_use = true;
    q_prof->q_count = q_count;
    q_prof->ch_in_pg = port_ch;

    q_prof->base_q = base_q;
    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.q_per_pg; i++) {
      if (q_map) {
        q_prof->q_mapping[i] = q_map[i];
      } else {
        // should never reach here.. caller should setup q-map
        q_prof->q_mapping[i] = i % q_count;
      }
    }
  }
  return (BF_SUCCESS);
}

bf_status_t bf_tm_q_get_descriptor(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t port_q,
                                   bf_tm_eg_q_t **q) {
  bf_status_t rc = BF_SUCCESS;
  int q_profile_index;

  if (!g_tm_ctx[dev]) {
    return rc;
  }
  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev]->q_profile;

  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof += q_profile_index;
  *q = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[dev], port) + q_prof->base_q + port_q;
  return (rc);
}

bf_status_t bf_tm_q_get_qid_descriptor(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       uint8_t qid,
                                       bf_tm_eg_q_t **q) {
  int q_profile_index;

  if (TM_IS_DEV_INVALID(dev) || qid >= BF_TM_MAX_QUEUE_PER_PG) {
    return (BF_INVALID_ARG);
  }

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev]->q_profile;

  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof += q_profile_index;

  bf_tm_queue_t port_q = q_prof->q_mapping[qid];
  if (port_q >= BF_TM_MAX_QUEUE_PER_PG) {
    LOG_ERROR("dev=%d port=%d qid=%d invalid port map to profile=%d queue=%d",
              dev,
              port,
              qid,
              q_profile_index,
              port_q);
    return (BF_UNEXPECTED);
  }

  *q = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[dev], port) + q_prof->base_q + port_q;
  return (BF_SUCCESS);
}

/* Get from the current queue mapping its port-relative queue number.
 */
bf_status_t bf_tm_q_get_port_queue_nr(bf_dev_id_t dev,
                                      bf_tm_eg_q_t *q,
                                      bf_tm_queue_t *queue_nr) {
  int q_profile_index;

  if (TM_IS_DEV_INVALID(dev) || NULL == q || NULL == queue_nr) {
    return (BF_INVALID_ARG);
  }

  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev]->q_profile;

  if (NULL == g_tm_ctx[dev] || NULL == q_prof) {
    return (BF_UNEXPECTED);
  }

  bf_tm_port_get_q_profile(dev, q->uport, &q_profile_index);
  q_prof += q_profile_index;

  if (q_prof->base_q > q->logical_q) {
    LOG_ERROR("dev=%d port=%d l_queue=%d invalid profile=%d base queue=%d",
              dev,
              q->uport,
              q->logical_q,
              q_profile_index,
              q_prof->base_q);
    return (BF_UNEXPECTED);
  }

  bf_tm_queue_t port_q = q->logical_q - q_prof->base_q;

  if ((int)port_q >= q_prof->q_count) {
    LOG_ERROR(
        "dev=%d port=%d l_queue=%d port_q=%d invalid profile=%d num queues=%d",
        dev,
        q->uport,
        q->logical_q,
        port_q,
        q_profile_index,
        q_prof->q_count);
    return (BF_UNEXPECTED);
  }

  *queue_nr = port_q;

  return (BF_SUCCESS);
}

bf_status_t bf_tm_q_carve_queues(bf_dev_id_t dev_id,
                                 bf_dev_port_t port,
                                 uint8_t q_count,
                                 uint8_t *q_mapping)

{
  bf_status_t rc = BF_SUCCESS;
  bf_tm_q_profile_t *q_prof = g_tm_ctx[dev_id]->q_profile;
  uint8_t prof_index = 0;
  uint8_t q_map[g_tm_ctx[dev_id]->tm_cfg.q_per_pg];
  int i;
  bf_tm_eg_q_t *q;
  bf_tm_port_t *p;
  int port_ch;

  if (!q_mapping) {
    // Set default mapping.
    if (q_count != 0) {
      for (i = 0; i < g_tm_ctx[dev_id]->tm_cfg.q_per_pg; i++) {
        q_map[i] = i % q_count;
      }
    }
  }

  // Keep old profile to release its queues later.
  int prof_old = 0;
  bf_tm_port_get_q_profile(dev_id, port, &prof_old);

  rc = bf_tm_q_create_q_profile(
      dev_id, port, q_count, (q_mapping) ? q_mapping : q_map, &prof_index);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  bf_tm_port_set_q_profile(dev_id, port, prof_index);

  port_ch = DEV_PORT_TO_LOCAL_PORT(
                lld_sku_map_devport_from_user_to_device(dev_id, port)) %
            g_tm_ctx[dev_id]->tm_cfg.ports_per_pg;
  for (i = 0; i < q_count; i++) {
    rc = bf_tm_q_get_descriptor(dev_id, port, i, &q);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "%s: TM: Failed to get queue descriptor for dev %d, port %d, queue "
          "%d",
          __func__,
          dev_id,
          port,
          i);
      return (rc);
    }

    rc = bf_tm_q_set_cache_counters(dev_id, port, i);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: bf_tm_q_set_cache_counters failed for dev %d, port %d, queue "
          "%d, rc "
          "%s (%d)",
          dev_id,
          port,
          q->physical_q,
          bf_err_str(rc),
          rc);
      return (rc);
    }

    q->in_use = true;
    /*
     * Update the port/channel id ownership for the queue in SW copy and
     * program the HW if ownership has changed
     */
    q->uport = DEV_PORT_TO_LOCAL_PORT(port);
    q->port = DEV_PORT_TO_LOCAL_PORT(
        lld_sku_map_devport_from_user_to_device(dev_id, port));

    if (port_ch != q->q_sch_cfg.cid) {
      /*
       * Note that in tof2 this is actually the L1 but we guarentee the l1
       * with the same channel ID as the port is assigned to the port.
       */
      q->q_sch_cfg.cid = port_ch;
      /* Program the new channel id for this queue in HW */
      rc = bf_tm_sch_set_q_sched(dev_id, q, q->q_sch_cfg.sch_enabled);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: setting queue config failed for dev %d, port %d, queue %d, rc "
            "%s (%d)",
            dev_id,
            port,
            q->physical_q,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }

  rc = bf_tm_port_get_descriptor(dev_id, port, &p);
  if (BF_SUCCESS != rc) {
    return (rc);
  }

  if ((prof_index != prof_old) &&
      (prof_old < g_tm_ctx[dev_id]->tm_cfg.q_prof_cnt) &&
      (q_prof[prof_old].in_use) && (g_q_cfg_hw_fptr_tbl.q_release_fptr)) {
    LOG_TRACE(
        "Carve HQ resources on dev:%d, dev_port:%d, from "
        "q_profile:%d(ch:%d,base:%d,cnt:%d) to "
        "q_profile:%d(ch:%d,base:%d,cnt:%d)",
        dev_id,
        port,
        prof_old,
        q_prof[prof_old].ch_in_pg,
        q_prof[prof_old].base_q,
        q_prof[prof_old].q_count,
        prof_index,
        q_prof[prof_index].ch_in_pg,
        q_prof[prof_index].base_q,
        q_prof[prof_index].q_count);

    rc = g_q_cfg_hw_fptr_tbl.q_release_fptr(
        dev_id, port, p->p_pipe, prof_old, q_prof + prof_old);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to release HQ resources on dev:%d, dev_port:%d, "
          "q_profile:%d, rc=%d(%s)",
          dev_id,
          port,
          prof_old,
          rc,
          bf_err_str(rc));
      return (rc);
    }
  }

  if (g_q_cfg_hw_fptr_tbl.q_carve_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_carve_fptr(
        dev_id, port, p->p_pipe, prof_index, q_prof + prof_index);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to carve HQ resources on dev:%d, dev_port:%d, "
          "q_profile:%d, rc=%d(%s)",
          dev_id,
          port,
          prof_index,
          rc,
          bf_err_str(rc));
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_set_min_limit(bf_dev_id_t dev_id,
                                     bf_tm_eg_q_t *q,
                                     bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.min_limit, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.min_limit = limit;

  if (g_q_thres_hw_fptr_tbl.min_limit_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.min_limit_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_min_limit(bf_dev_id_t dev_id,
                                     bf_tm_eg_q_t *q,
                                     bf_tm_thres_t *sw_limit,
                                     bf_tm_thres_t *hw_limit) {
  bf_tm_eg_q_t out_q;
  bf_tm_status_t rc = BF_TM_EOK;
  *sw_limit = q->thresholds.min_limit;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.min_limit_rd_fptr) {
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_thres_hw_fptr_tbl.min_limit_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.min_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_app_limit(bf_dev_id_t dev_id,
                                     bf_tm_eg_q_t *q,
                                     bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.app_limit, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.app_limit = limit;
  if (g_q_thres_hw_fptr_tbl.app_limit_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.app_limit_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_app_limit(bf_dev_id_t dev_id,
                                     bf_tm_eg_q_t *q,
                                     bf_tm_thres_t *sw_limit,
                                     bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.app_limit;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.app_limit_rd_fptr) {
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_thres_hw_fptr_tbl.app_limit_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.app_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_app_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.app_hyst, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.app_hyst = limit;

  if (g_q_thres_hw_fptr_tbl.app_hyst_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.app_hyst_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_app_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t *sw_limit,
                                    bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.app_hyst;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.app_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    *hw_limit = BF_TM_INVALID_THRES_LIMIT;
    rc = g_q_thres_hw_fptr_tbl.app_hyst_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.app_hyst;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_app_hyst_index(bf_dev_id_t dev_id,
                                          bf_tm_eg_q_t *q,
                                          uint8_t *sw_hyst_index,
                                          uint8_t *hw_hyst_index) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_hyst_index = q->thresholds.app_hyst_index;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_hyst_index &&
      g_q_thres_hw_fptr_tbl.app_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_thres_hw_fptr_tbl.app_hyst_rd_fptr(dev_id, &out_q);
    *hw_hyst_index = out_q.thresholds.app_hyst_index;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_yel_limit_pcent(bf_dev_id_t dev_id,
                                           bf_tm_eg_q_t *q,
                                           uint8_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  q->thresholds.yel_limit_pcent = limit;

  if (g_q_thres_hw_fptr_tbl.yel_limit_pcent_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.yel_limit_pcent_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_yel_limit_pcent(bf_dev_id_t dev_id,
                                           bf_tm_eg_q_t *q,
                                           uint8_t *sw_limit,
                                           uint8_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.yel_limit_pcent;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.yel_limit_pcent_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    *hw_limit = 0;
    rc = g_q_thres_hw_fptr_tbl.yel_limit_pcent_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.yel_limit_pcent;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_red_limit_pcent(bf_dev_id_t dev_id,
                                           bf_tm_eg_q_t *q,
                                           uint8_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.red_limit_pcent, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.red_limit_pcent = limit;

  if (g_q_thres_hw_fptr_tbl.red_limit_pcent_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.red_limit_pcent_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_red_limit_pcent(bf_dev_id_t dev_id,
                                           bf_tm_eg_q_t *q,
                                           uint8_t *sw_limit,
                                           uint8_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.red_limit_pcent;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.red_limit_pcent_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    *hw_limit = 0;
    rc = g_q_thres_hw_fptr_tbl.red_limit_pcent_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.red_limit_pcent;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_yel_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.yel_hyst, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.yel_hyst = limit;

  if (g_q_thres_hw_fptr_tbl.yel_hyst_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.yel_hyst_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_yel_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t *sw_limit,
                                    bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.yel_hyst;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.yel_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    *hw_limit = 0;
    rc = g_q_thres_hw_fptr_tbl.yel_hyst_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.yel_hyst;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_yel_hyst_index(bf_dev_id_t dev_id,
                                          bf_tm_eg_q_t *q,
                                          uint8_t *sw_hyst_index,
                                          uint8_t *hw_hyst_index) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_hyst_index = q->thresholds.yel_hyst_index;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_hyst_index &&
      g_q_thres_hw_fptr_tbl.yel_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_thres_hw_fptr_tbl.yel_hyst_rd_fptr(dev_id, &out_q);
    *hw_hyst_index = out_q.thresholds.yel_hyst_index;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_red_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          limit, q->thresholds.red_hyst, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->thresholds.red_hyst = limit;

  if (g_q_thres_hw_fptr_tbl.red_hyst_wr_fptr) {
    rc = g_q_thres_hw_fptr_tbl.red_hyst_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_red_hyst(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    bf_tm_thres_t *sw_limit,
                                    bf_tm_thres_t *hw_limit) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_limit = q->thresholds.red_hyst;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_limit &&
      g_q_thres_hw_fptr_tbl.red_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    *hw_limit = 0;
    rc = g_q_thres_hw_fptr_tbl.red_hyst_rd_fptr(dev_id, &out_q);
    *hw_limit = out_q.thresholds.red_hyst;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_red_hyst_index(bf_dev_id_t dev_id,
                                          bf_tm_eg_q_t *q,
                                          uint8_t *sw_hyst_index,
                                          uint8_t *hw_hyst_index) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;
  *sw_hyst_index = q->thresholds.red_hyst_index;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_hyst_index &&
      g_q_thres_hw_fptr_tbl.red_hyst_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_thres_hw_fptr_tbl.red_hyst_rd_fptr(dev_id, &out_q);
    *hw_hyst_index = out_q.thresholds.red_hyst_index;
  }
  return (rc);
}

bf_tm_status_t bf_tm_restore_qac_offset_profile(bf_dev_id_t dev_id) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_thres_hw_fptr_tbl.restore_qac_offset_profile_fptr) {
    g_q_thres_hw_fptr_tbl.restore_qac_offset_profile_fptr(dev_id);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_set_app_poolid(bf_dev_id_t dev_id,
                                      bf_tm_eg_q_t *q,
                                      uint8_t pool) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(pool, q->q_cfg.app_poolid, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->q_cfg.app_poolid = pool;

  if (g_q_cfg_hw_fptr_tbl.app_poolid_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.app_poolid_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_app_poolid(bf_dev_id_t dev_id,
                                      bf_tm_eg_q_t *q,
                                      uint8_t *sw_pool,
                                      uint8_t *hw_pool) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_pool = q->q_cfg.app_poolid;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_pool &&
      g_q_cfg_hw_fptr_tbl.app_poolid_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.app_poolid_rd_fptr(dev_id, &out_q);
    *hw_pool = out_q.q_cfg.app_poolid;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_is_dynamic(bf_dev_id_t dev_id,
                                      bf_tm_eg_q_t *q,
                                      bool mode) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(mode, q->q_cfg.is_dynamic, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->q_cfg.is_dynamic = mode;

  if (g_q_cfg_hw_fptr_tbl.is_dynamic_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.is_dynamic_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_is_dynamic(bf_dev_id_t dev_id,
                                      bf_tm_eg_q_t *q,
                                      bool *sw_q_mode,
                                      bool *hw_q_mode) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_q_mode = q->q_cfg.is_dynamic;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_q_mode &&
      g_q_cfg_hw_fptr_tbl.is_dynamic_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.is_dynamic_rd_fptr(dev_id, &out_q);
    *hw_q_mode = out_q.q_cfg.is_dynamic;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_baf(bf_dev_id_t dev_id,
                               bf_tm_eg_q_t *q,
                               uint8_t baf) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(baf, q->q_cfg.baf, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->q_cfg.baf = baf;
  if (g_q_cfg_hw_fptr_tbl.baf_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.baf_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_baf(bf_dev_id_t dev_id,
                               bf_tm_eg_q_t *q,
                               uint8_t *sw_baf,
                               uint8_t *hw_baf) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_baf = q->q_cfg.baf;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_baf && g_q_cfg_hw_fptr_tbl.baf_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.baf_rd_fptr(dev_id, &out_q);
    *hw_baf = out_q.q_cfg.baf;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_qac_buffer(bf_dev_id_t dev_id, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(true, true, g_tm_ctx[dev_id])) {
    return (rc);
  }
  if (g_q_cfg_hw_fptr_tbl.qac_buffer_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.qac_buffer_wr_fptr(dev_id, q);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_wac_buffer(bf_dev_id_t dev_id, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(true, true, g_tm_ctx[dev_id])) {
    return (rc);
  }
  if (g_q_cfg_hw_fptr_tbl.wac_buffer_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.wac_buffer_wr_fptr(dev_id, q);
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_color_drop_en(bf_dev_id_t dev_id,
                                         bf_tm_eg_q_t *q,
                                         bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          enable, q->q_cfg.color_drop_en, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->q_cfg.color_drop_en = enable;
  if (g_q_cfg_hw_fptr_tbl.color_drop_en_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.color_drop_en_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_color_drop_en(bf_dev_id_t dev_id,
                                         bf_tm_eg_q_t *q,
                                         bool *sw_enable,
                                         bool *hw_enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_enable = q->q_cfg.color_drop_en;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_enable &&
      g_q_cfg_hw_fptr_tbl.color_drop_en_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.color_drop_en_rd_fptr(dev_id, &out_q);
    *hw_enable = out_q.q_cfg.color_drop_en;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_tail_drop_en(bf_dev_id_t dev_id,
                                        bf_tm_eg_q_t *q,
                                        bool enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  if (TM_HITLESS_IS_CFG_MATCH(
          enable, q->q_cfg.tail_drop_en, g_tm_ctx[dev_id])) {
    return (rc);
  }
  q->q_cfg.tail_drop_en = enable;
  if (g_q_cfg_hw_fptr_tbl.tail_drop_en_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.tail_drop_en_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_tail_drop_en(bf_dev_id_t dev_id,
                                        bf_tm_eg_q_t *q,
                                        bool *sw_enable,
                                        bool *hw_enable) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_enable = q->q_cfg.tail_drop_en;
  if (TM_IS_TARGET_ASIC(dev_id) && hw_enable &&
      g_q_cfg_hw_fptr_tbl.tail_drop_en_rd_fptr) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.tail_drop_en_rd_fptr(dev_id, &out_q);
    *hw_enable = out_q.q_cfg.tail_drop_en;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_mirror_on_drop_destination(bf_dev_id_t dev_id,
                                                      bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  (g_tm_ctx[dev_id]->pipes + q->l_pipe)->neg_mirror_dest = q;

  if (g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_mirror_on_drop_destination(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe,
    bf_tm_eg_q_t **sw_neg_mirror_dest,
    bf_tm_eg_q_t **hw_neg_mirror_dest) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t q;

  *sw_neg_mirror_dest = (g_tm_ctx[dev_id]->pipes + pipe)->neg_mirror_dest;

  if (TM_IS_TARGET_ASIC(dev_id) && hw_neg_mirror_dest &&
      g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_rd_fptr) {
    lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, pipe, &q.p_pipe);
    q.l_pipe = pipe;
    rc = g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_rd_fptr(dev_id, &q);
    if (rc == BF_SUCCESS) {
      *hw_neg_mirror_dest =
          BF_TM_FIRST_Q_PTR_IN_PIPE(g_tm_ctx[dev_id], pipe) + q.physical_q;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_set_visible(bf_dev_id_t dev_id,
                                   bf_tm_eg_q_t *q,
                                   bool visible) {
  bf_tm_status_t rc = BF_TM_EOK;

  q->q_cfg.visible = visible;

  if (g_q_cfg_hw_fptr_tbl.q_visible_wr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_visible_wr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_visible(bf_dev_id_t dev_id,
                                   bf_tm_eg_q_t *q,
                                   bool *visible_sw,
                                   bool *visible_hw) {
  bf_tm_status_t rc = BF_TM_EOK;
  *visible_sw = q->q_cfg.visible;

  if (g_q_cfg_hw_fptr_tbl.q_visible_rd_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_visible_rd_fptr(dev_id, q);
  }
  if (rc == BF_SUCCESS) {
    *visible_hw = q->q_cfg.visible;
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_get_drop_counter(bf_dev_id_t dev_id,
                                        bf_tm_eg_q_t *q,
                                        uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr(dev_id, q, count);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_get_drop_counter_ext(bf_dev_id_t dev_id,
                                            bf_subdev_id_t die_id,
                                            bf_tm_eg_q_t *q,
                                            uint64_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr_ext) {
    rc = g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr_ext(dev_id, die_id, q, count);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_get_egress_drop_state(bf_dev_id_t dev_id,
                                             bf_tm_eg_q_t *q,
                                             bf_tm_color_t color,
                                             bool *state) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_dropstate_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_dropstate_fptr(dev_id, q, color, state);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_clear_egress_drop_state(bf_dev_id_t dev_id,
                                               bf_tm_eg_q_t *q,
                                               bf_tm_color_t color) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_dropstate_clr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_dropstate_clr_fptr(dev_id, q, color);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_clear_drop_counter(bf_dev_id_t dev_id, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_drop_cntr_clr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_drop_cntr_clr_fptr(dev_id, q);
  }

  return (rc);
}

bf_tm_status_t bf_tm_q_get_usage_count(bf_dev_id_t dev_id,
                                       bf_tm_eg_q_t *q,
                                       uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t cnt;

  if (g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr(dev_id, q, &cnt);
    *count = (uint32_t)cnt;  // 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_usage_count_ext(bf_dev_id_t dev_id,
                                           bf_subdev_id_t die_id,
                                           bf_tm_eg_q_t *q,
                                           uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t cnt;

  if (g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr_ext) {
    rc = g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr_ext(dev_id, die_id, q, &cnt);
    *count = (uint32_t)cnt;  // 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_clear_usage_count(bf_dev_id_t dev_id, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_usage_cntr_clr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_usage_cntr_clr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_wm_count(bf_dev_id_t dev_id,
                                    bf_tm_eg_q_t *q,
                                    uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t cnt;

  if (g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr(dev_id, q, &cnt);
    *count = (uint32_t)cnt;  // 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_wm_count_ext(bf_dev_id_t dev_id,
                                        bf_subdev_id_t die_id,
                                        bf_tm_eg_q_t *q,
                                        uint32_t *count) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint64_t cnt;

  if (g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr_ext) {
    rc = g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr_ext(dev_id, die_id, q, &cnt);
    *count = (uint32_t)cnt;  // 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_clear_watermark(bf_dev_id_t dev_id, bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_wm_clr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_wm_clr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_profiles_mapping(bf_dev_id_t dev_id,
                                            bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_profiles_qid_map_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_profiles_qid_map_fptr(dev_id, q_profile);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_profile_mapping(bf_dev_id_t dev_id,
                                           int p_pipe,
                                           int profile_index,
                                           bf_tm_q_profile_t *q_profile) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_profile_qid_map_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_profile_qid_map_fptr(
        dev_id, p_pipe, profile_index, q_profile);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_port_q_profile(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          int p_pipe,
                                          uint32_t *profile_index) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_profile_port_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_profile_port_fptr(
        dev_id, port, p_pipe, profile_index);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_pipe_physical_queue(bf_dev_id_t dev_id,
                                               bf_dev_port_t port,
                                               uint32_t ing_q,
                                               bf_dev_pipe_t *log_pipe,
                                               bf_tm_queue_t *phys_q) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_ing_qid_to_phys_q) {
    rc = g_q_cfg_hw_fptr_tbl.q_ing_qid_to_phys_q(
        dev_id, port, ing_q, log_pipe, phys_q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_drop_state_shadow(bf_dev_id_t dev_id,
                                             bf_tm_eg_q_t *q,
                                             uint32_t *state) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_get_dropstate_shadow) {
    rc = g_q_cfg_hw_fptr_tbl.q_get_dropstate_shadow(dev_id, q, state);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_clear_drop_state_shadow(bf_dev_id_t dev_id,
                                               bf_tm_eg_q_t *q) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_q_cfg_hw_fptr_tbl.q_dropst_shadow_clr_fptr) {
    rc = g_q_cfg_hw_fptr_tbl.q_dropst_shadow_clr_fptr(dev_id, q);
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_fast_recovery_mode(bf_dev_id_t dev_id,
                                              bf_tm_eg_q_t *q,
                                              bool *sw_fast_recovery,
                                              bool *hw_fast_recovery) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t out_q;

  *sw_fast_recovery = q->q_cfg.fast_recover_mode;

  if (hw_fast_recovery && g_q_cfg_hw_fptr_tbl.q_get_fast_recovery) {
    memcpy(&out_q, q, sizeof(out_q));
    rc = g_q_cfg_hw_fptr_tbl.q_get_fast_recovery(dev_id, &out_q);
    *hw_fast_recovery = out_q.q_cfg.fast_recover_mode;
  }
  return (rc);
}

bf_tm_status_t bf_tm_q_get_defaults(bf_dev_id_t devid,
                                    bf_tm_q_defaults_t *def) {
  BF_TM_INVALID_ARG(def == NULL);
  if (g_q_cfg_hw_fptr_tbl.q_get_defaults_fptr) {
    return g_q_cfg_hw_fptr_tbl.q_get_defaults_fptr(devid, def);
  }
  return BF_NOT_SUPPORTED;
}

#define BF_TM_Q_HW_FTBL_SET_LIMIT_FUNCS(min_limit_fptr,                   \
                                        app_limit_fptr,                   \
                                        app_hyst_fptr,                    \
                                        red_limit_pcent_fptr,             \
                                        yel_limit_pcent_fptr,             \
                                        red_hyst_fptr,                    \
                                        yel_hyst_fptr)                    \
  {                                                                       \
    g_q_thres_hw_fptr_tbl.min_limit_wr_fptr = min_limit_fptr;             \
    g_q_thres_hw_fptr_tbl.app_limit_wr_fptr = app_limit_fptr;             \
    g_q_thres_hw_fptr_tbl.app_hyst_wr_fptr = app_hyst_fptr;               \
    g_q_thres_hw_fptr_tbl.red_limit_pcent_wr_fptr = red_limit_pcent_fptr; \
    g_q_thres_hw_fptr_tbl.yel_limit_pcent_wr_fptr = yel_limit_pcent_fptr; \
    g_q_thres_hw_fptr_tbl.red_hyst_wr_fptr = red_hyst_fptr;               \
    g_q_thres_hw_fptr_tbl.yel_hyst_wr_fptr = yel_hyst_fptr;               \
  }

#define BF_TM_Q_HW_FTBL_GET_LIMIT_FUNCS(min_limit_fptr,                   \
                                        app_limit_fptr,                   \
                                        app_hyst_fptr,                    \
                                        red_limit_pcent_fptr,             \
                                        yel_limit_pcent_fptr,             \
                                        red_hyst_fptr,                    \
                                        yel_hyst_fptr,                    \
                                        qac_offset_profile_fptr)          \
  {                                                                       \
    g_q_thres_hw_fptr_tbl.min_limit_rd_fptr = min_limit_fptr;             \
    g_q_thres_hw_fptr_tbl.app_limit_rd_fptr = app_limit_fptr;             \
    g_q_thres_hw_fptr_tbl.app_hyst_rd_fptr = app_hyst_fptr;               \
    g_q_thres_hw_fptr_tbl.red_limit_pcent_rd_fptr = red_limit_pcent_fptr; \
    g_q_thres_hw_fptr_tbl.yel_limit_pcent_rd_fptr = yel_limit_pcent_fptr; \
    g_q_thres_hw_fptr_tbl.red_hyst_rd_fptr = red_hyst_fptr;               \
    g_q_thres_hw_fptr_tbl.yel_hyst_rd_fptr = yel_hyst_fptr;               \
    g_q_thres_hw_fptr_tbl.restore_qac_offset_profile_fptr =               \
        qac_offset_profile_fptr;                                          \
  }

#define BF_TM_Q_HW_FTBL_SET_FUNCS(app_poolid_fptr,                             \
                                  is_dyn_fptr,                                 \
                                  baf_fptr,                                    \
                                  color_drop_en_fptr,                          \
                                  tail_drop_en_fptr,                           \
                                  neg_mir_dest_fptr,                           \
                                  visible_fptr,                                \
                                  carve_q_fptr,                                \
                                  release_q_fptr,                              \
                                  q_drop_cntr_clear_fptr,                      \
                                  q_watermark_clear_fptr,                      \
                                  q_dropst_shadow_clear_fptr,                  \
                                  q_dropstate_clear_fptr,                      \
                                  q_usage_cntr_clear_fptr,                     \
                                  qac_buffer_fptr,                             \
                                  wac_buffer_fptr)                             \
  {                                                                            \
    g_q_cfg_hw_fptr_tbl.app_poolid_wr_fptr = app_poolid_fptr;                  \
    g_q_cfg_hw_fptr_tbl.is_dynamic_wr_fptr = is_dyn_fptr;                      \
    g_q_cfg_hw_fptr_tbl.baf_wr_fptr = baf_fptr;                                \
    g_q_cfg_hw_fptr_tbl.color_drop_en_wr_fptr = color_drop_en_fptr;            \
    g_q_cfg_hw_fptr_tbl.tail_drop_en_wr_fptr = tail_drop_en_fptr;              \
    g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_wr_fptr = neg_mir_dest_fptr;         \
    g_q_cfg_hw_fptr_tbl.q_visible_wr_fptr = visible_fptr;                      \
    g_q_cfg_hw_fptr_tbl.q_carve_fptr = carve_q_fptr;                           \
    g_q_cfg_hw_fptr_tbl.q_release_fptr = release_q_fptr;                       \
    g_q_cfg_hw_fptr_tbl.q_drop_cntr_clr_fptr = q_drop_cntr_clear_fptr;         \
    g_q_cfg_hw_fptr_tbl.q_wm_clr_fptr = q_watermark_clear_fptr;                \
    g_q_cfg_hw_fptr_tbl.q_dropst_shadow_clr_fptr = q_dropst_shadow_clear_fptr; \
    g_q_cfg_hw_fptr_tbl.q_dropstate_clr_fptr = q_dropstate_clear_fptr;         \
    g_q_cfg_hw_fptr_tbl.q_usage_cntr_clr_fptr = q_usage_cntr_clear_fptr;       \
    g_q_cfg_hw_fptr_tbl.qac_buffer_wr_fptr = qac_buffer_fptr;                  \
    g_q_cfg_hw_fptr_tbl.wac_buffer_wr_fptr = wac_buffer_fptr;                  \
  }

#define BF_TM_Q_HW_FTBL_GET_FUNCS(app_poolid_fptr,                        \
                                  is_dyn_fptr,                            \
                                  baf_fptr,                               \
                                  color_drop_en_fptr,                     \
                                  tail_drop_en_fptr,                      \
                                  drop_cntr_fptr,                         \
                                  dropstate_fptr,                         \
                                  usage_cntr_fptr,                        \
                                  wm_cntr_fptr,                           \
                                  qid_profile_map_fptr,                   \
                                  qid_profiles_map_fptr,                  \
                                  port_qprofile_fptr,                     \
                                  neg_mirror_dest_fptr,                   \
                                  visible_fptr,                           \
                                  ing_qid_to_phys_qid,                    \
                                  dropst_shadow_fptr,                     \
                                  fast_recover,                           \
                                  q_defaults_fptr,                        \
                                  drop_cntr_fptr_ext,                     \
                                  usage_cntr_fptr_ext,                    \
                                  wm_cntr_fptr_ext)                       \
  {                                                                       \
    g_q_cfg_hw_fptr_tbl.app_poolid_rd_fptr = app_poolid_fptr;             \
    g_q_cfg_hw_fptr_tbl.is_dynamic_rd_fptr = is_dyn_fptr;                 \
    g_q_cfg_hw_fptr_tbl.baf_rd_fptr = baf_fptr;                           \
    g_q_cfg_hw_fptr_tbl.color_drop_en_rd_fptr = color_drop_en_fptr;       \
    g_q_cfg_hw_fptr_tbl.tail_drop_en_rd_fptr = tail_drop_en_fptr;         \
    g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr = drop_cntr_fptr;                \
    g_q_cfg_hw_fptr_tbl.q_dropstate_fptr = dropstate_fptr;                \
    g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr = usage_cntr_fptr;              \
    g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr = wm_cntr_fptr;                    \
    g_q_cfg_hw_fptr_tbl.q_profile_qid_map_fptr = qid_profile_map_fptr;    \
    g_q_cfg_hw_fptr_tbl.q_profiles_qid_map_fptr = qid_profiles_map_fptr;  \
    g_q_cfg_hw_fptr_tbl.q_profile_port_fptr = port_qprofile_fptr;         \
    g_q_cfg_hw_fptr_tbl.q_neg_mirror_dest_rd_fptr = neg_mirror_dest_fptr; \
    g_q_cfg_hw_fptr_tbl.q_visible_rd_fptr = visible_fptr;                 \
    g_q_cfg_hw_fptr_tbl.q_ing_qid_to_phys_q = ing_qid_to_phys_qid;        \
    g_q_cfg_hw_fptr_tbl.q_get_dropstate_shadow = dropst_shadow_fptr;      \
    g_q_cfg_hw_fptr_tbl.q_get_fast_recovery = fast_recover;               \
    g_q_cfg_hw_fptr_tbl.q_get_defaults_fptr = q_defaults_fptr;            \
    g_q_cfg_hw_fptr_tbl.q_drop_cntr_fptr_ext = drop_cntr_fptr_ext;        \
    g_q_cfg_hw_fptr_tbl.q_usage_cntr_fptr_ext = usage_cntr_fptr_ext;      \
    g_q_cfg_hw_fptr_tbl.q_wm_cntr_fptr_ext = wm_cntr_fptr_ext;            \
  }

void bf_tm_q_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_Q_HW_FTBL_SET_LIMIT_FUNCS(NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  BF_TM_Q_HW_FTBL_GET_LIMIT_FUNCS(
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  BF_TM_Q_HW_FTBL_SET_FUNCS(NULL,
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
  BF_TM_Q_HW_FTBL_GET_FUNCS(NULL,
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

static void bf_tm_q_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  /* Depending on HW version (Tofino, Tofino-lite, ...) setup this
   * function table with correct function pointers
   */
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    // Tofino HW APIs
    BF_TM_Q_HW_FTBL_SET_LIMIT_FUNCS(bf_tm_tofino_q_set_min_limit,
                                    bf_tm_tofino_q_set_app_limit,
                                    bf_tm_tofino_q_set_app_hyst,
                                    bf_tm_tofino_q_set_red_limit_pcent,
                                    bf_tm_tofino_q_set_yel_limit_pcent,
                                    bf_tm_tofino_q_set_red_hyst,
                                    bf_tm_tofino_q_set_yel_hyst);
    BF_TM_Q_HW_FTBL_SET_FUNCS(bf_tm_tofino_q_set_app_poolid,
                              bf_tm_tofino_q_set_is_dynamic,
                              bf_tm_tofino_q_set_baf,
                              bf_tm_tofino_q_set_color_drop_en,
                              bf_tm_tofino_q_set_tail_drop_en,
                              bf_tm_tofino_q_set_neg_mir_dest,
                              NULL,
                              bf_tm_tofino_q_carve_queues,
                              NULL,
                              bf_tm_tofino_q_clear_drop_counter,
                              bf_tm_tofino_q_clear_watermark,
                              bf_tm_tofino_q_clear_skidpool_drop_state_shadow,
                              bf_tm_tofino_q_clear_egress_drop_state,
                              bf_tm_tofino_q_clear_usage_counter,
                              NULL,
                              NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    // Tof2 HW APIs
    BF_TM_Q_HW_FTBL_SET_LIMIT_FUNCS(bf_tm_tof2_q_set_min_limit,
                                    bf_tm_tof2_q_set_app_limit,
                                    bf_tm_tof2_q_set_app_hyst,
                                    bf_tm_tof2_q_set_red_limit_pcent,
                                    bf_tm_tof2_q_set_yel_limit_pcent,
                                    bf_tm_tof2_q_set_red_hyst,
                                    bf_tm_tof2_q_set_yel_hyst);
    BF_TM_Q_HW_FTBL_SET_FUNCS(bf_tm_tof2_q_set_app_poolid,
                              bf_tm_tof2_q_set_is_dynamic,
                              bf_tm_tof2_q_set_baf,
                              bf_tm_tof2_q_set_color_drop_en,
                              bf_tm_tof2_q_set_tail_drop_en,
                              bf_tm_tof2_q_set_neg_mir_dest,
                              bf_tm_tof2_q_set_visible,
                              bf_tm_tof2_q_carve_queues,
                              bf_tm_tof2_q_release_queues,
                              bf_tm_tof2_q_clear_drop_counter,
                              bf_tm_tof2_q_clear_wm_counter,
                              NULL /*dropst shadow*/,
                              bf_tm_tof2_q_clear_egress_drop_state,
                              bf_tm_tof2_q_clear_usage_counter,
                              NULL,
                              NULL);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    // Tof3 HW APIs
    BF_TM_Q_HW_FTBL_SET_LIMIT_FUNCS(bf_tm_tof3_q_set_min_limit,
                                    bf_tm_tof3_q_set_app_limit,
                                    bf_tm_tof3_q_set_app_hyst,
                                    bf_tm_tof3_q_set_red_limit_pcent,
                                    bf_tm_tof3_q_set_yel_limit_pcent,
                                    bf_tm_tof3_q_set_red_hyst,
                                    bf_tm_tof3_q_set_yel_hyst);
    BF_TM_Q_HW_FTBL_SET_FUNCS(bf_tm_tof3_q_set_app_poolid,
                              bf_tm_tof3_q_set_is_dynamic,
                              bf_tm_tof3_q_set_baf,
                              bf_tm_tof3_q_set_color_drop_en,
                              bf_tm_tof3_q_set_tail_drop_en,
                              bf_tm_tof3_q_set_neg_mir_dest,
                              bf_tm_tof3_q_set_visible,
                              bf_tm_tof3_q_carve_queues,
                              bf_tm_tof3_q_release_queues,
                              bf_tm_tof3_q_clear_drop_counter,
                              bf_tm_tof3_q_clear_wm_counter,
                              NULL /*dropst shadow*/,
                              bf_tm_tof3_q_clear_egress_drop_state,
                              bf_tm_tof3_q_clear_usage_counter,
                              bf_tm_tof3_q_set_qac_buffer,
                              bf_tm_tof3_q_set_wac_buffer);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition.
  }
}

static void bf_tm_q_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  /* Depending on HW version (Tofino, Tofino-lite, ...) setup this
   * function table with correct function pointers
   */
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_Q_HW_FTBL_GET_LIMIT_FUNCS(bf_tm_tofino_q_get_min_limit,
                                    bf_tm_tofino_q_get_app_limit,
                                    bf_tm_tofino_q_get_app_hyst,
                                    bf_tm_tofino_q_get_red_limit_pcent,
                                    bf_tm_tofino_q_get_yel_limit_pcent,
                                    bf_tm_tofino_q_get_red_hyst,
                                    bf_tm_tofino_q_get_yel_hyst,
                                    bf_tm_tofino_restore_qac_offset_profile);
    BF_TM_Q_HW_FTBL_GET_FUNCS(bf_tm_tofino_q_get_app_poolid,
                              bf_tm_tofino_q_get_is_dynamic,
                              bf_tm_tofino_q_get_baf,
                              bf_tm_tofino_q_get_color_drop_en,
                              bf_tm_tofino_q_get_tail_drop_en,
                              bf_tm_tofino_q_get_drop_counter,
                              bf_tm_tofino_q_get_egress_drop_state,
                              bf_tm_tofino_q_get_usage_counter,
                              bf_tm_tofino_q_get_wm_counter,
                              bf_tm_tofino_get_q_profile_mapping,
                              bf_tm_tofino_get_q_profiles_mapping,
                              bf_tm_tofino_get_port_q_profile,
                              bf_tm_tofino_q_get_neg_mir_dest,
                              NULL,
                              bf_tm_tofino_get_phys_q,
                              bf_tm_tofino_q_get_skidpool_drop_state_shadow,
                              bf_tm_tofino_q_get_fast_recover_mode,
                              bf_tm_tofino_q_get_defaults,
                              NULL,
                              NULL,
                              NULL);
  } else if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_Q_HW_FTBL_GET_LIMIT_FUNCS(bf_tm_tof2_q_get_min_limit,
                                    bf_tm_tof2_q_get_app_limit,
                                    bf_tm_tof2_q_get_app_hyst,
                                    bf_tm_tof2_q_get_red_limit_pcent,
                                    bf_tm_tof2_q_get_yel_limit_pcent,
                                    bf_tm_tof2_q_get_red_hyst,
                                    bf_tm_tof2_q_get_yel_hyst,
                                    bf_tm_tof2_restore_qac_offset_profile);
    BF_TM_Q_HW_FTBL_GET_FUNCS(bf_tm_tof2_q_get_app_poolid,
                              bf_tm_tof2_q_get_is_dynamic,
                              bf_tm_tof2_q_get_baf,
                              bf_tm_tof2_q_get_color_drop_en,
                              bf_tm_tof2_q_get_tail_drop_en,
                              bf_tm_tof2_q_get_drop_counter,
                              bf_tm_tof2_q_get_egress_drop_state,
                              bf_tm_tof2_q_get_usage_counter,
                              bf_tm_tof2_q_get_wm_counter,
                              bf_tm_tof2_get_q_profile_mapping,
                              bf_tm_tof2_get_q_profiles_mapping,
                              bf_tm_tof2_get_port_q_profile,
                              bf_tm_tof2_q_get_neg_mir_dest,
                              bf_tm_tof2_q_get_visible,
                              bf_tm_tof2_get_physical_q,
                              NULL,
                              bf_tm_tof2_q_get_fast_recover_mode,
                              bf_tm_tof2_q_get_defaults,
                              NULL,
                              NULL,
                              NULL);
  } else if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_Q_HW_FTBL_GET_LIMIT_FUNCS(bf_tm_tof3_q_get_min_limit,
                                    bf_tm_tof3_q_get_app_limit,
                                    bf_tm_tof3_q_get_app_hyst,
                                    bf_tm_tof3_q_get_red_limit_pcent,
                                    bf_tm_tof3_q_get_yel_limit_pcent,
                                    bf_tm_tof3_q_get_red_hyst,
                                    bf_tm_tof3_q_get_yel_hyst,
                                    bf_tm_tof3_restore_qac_offset_profile);
    BF_TM_Q_HW_FTBL_GET_FUNCS(bf_tm_tof3_q_get_app_poolid,
                              bf_tm_tof3_q_get_is_dynamic,
                              bf_tm_tof3_q_get_baf,
                              bf_tm_tof3_q_get_color_drop_en,
                              bf_tm_tof3_q_get_tail_drop_en,
                              bf_tm_tof3_q_get_drop_counter,
                              bf_tm_tof3_q_get_egress_drop_state,
                              bf_tm_tof3_q_get_usage_counter,
                              bf_tm_tof3_q_get_wm_counter,
                              bf_tm_tof3_get_q_profile_mapping,
                              bf_tm_tof3_get_q_profiles_mapping,
                              bf_tm_tof3_get_port_q_profile,
                              bf_tm_tof3_q_get_neg_mir_dest,
                              bf_tm_tof3_q_get_visible,
                              bf_tm_tof3_get_physical_q,
                              NULL,
                              bf_tm_tof3_q_get_fast_recover_mode,
                              bf_tm_tof3_q_get_defaults,
                              bf_tm_tof3_q_get_drop_counter_ext,
                              bf_tm_tof3_q_get_usage_counter_ext,
                              bf_tm_tof3_q_get_wm_counter_ext);
  }
}

void bf_tm_q_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_q_set_hw_ftbl_rd_funcs(tm_ctx);
  bf_tm_q_set_hw_ftbl_wr_funcs(tm_ctx);
}

void bf_tm_q_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_q_null_hw_ftbl(tm_ctx);
  bf_tm_q_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_q_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->eg_q) bf_sys_free(tm_ctx->eg_q);
  if (tm_ctx->q_profile) bf_sys_free(tm_ctx->q_profile);
}

bf_tm_status_t bf_tm_init_q(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_status_t rc = BF_TM_EOK;
  bf_tm_eg_q_t *q;
  bf_dev_pipe_t pipe = 0;
  int i, j, k;
  uint16_t physical_q;

  if (tm_ctx->eg_q) {
    bf_tm_q_delete(tm_ctx);
  }

  tm_ctx->eg_q =
      bf_sys_calloc(tm_ctx->tm_cfg.pipe_cnt * tm_ctx->tm_cfg.q_per_pipe,
                    sizeof(bf_tm_eg_q_t));
  if (!tm_ctx->eg_q) {
    return (BF_NO_SYS_RESOURCES);
  }

  // allocate q-profile
  tm_ctx->q_profile =
      bf_sys_calloc(tm_ctx->tm_cfg.q_prof_cnt, sizeof(bf_tm_q_profile_t));
  if (!tm_ctx->q_profile) {
    bf_tm_q_delete(tm_ctx);
    return (BF_NO_SYS_RESOURCES);
  }

  q = tm_ctx->eg_q;
  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(tm_ctx->devid, &num_pipes);
  for (i = 0; i < (int)num_pipes; i++) {
    if (lld_sku_map_pipe_id_to_phy_pipe_id(tm_ctx->devid, i, &pipe) != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          tm_ctx->devid,
          i);
    }
    bf_sys_assert(pipe < BF_TM_MAX_MAU_PIPES);
    physical_q = 0;
    for (k = 0; k < tm_ctx->tm_cfg.pg_per_pipe; k++) {
      for (j = 0; j < tm_ctx->tm_cfg.q_per_pg; j++) {
        q->l_pipe = i;
        q->p_pipe = pipe;
        q->logical_q = j;
        q->physical_q = physical_q++;
        q->hq_base = q->physical_q;  // to be changed on carving
        q->hq_per_vq = 0;            // to be set on carving
        q->in_use = false;
        q->pg = k;
        // q->q_cfg.app_poolid = tm_ctx->tm_cfg.gmin_pool;
        q->q_cfg.app_poolid = 0;
        q->q_sch_cfg.sch_enabled = true;
        q->q_sch_cfg.sch_pfc_enabled =
            true;  // Per HW team, this has to be set always
        // Disable shaping by default
        q->q_sch_cfg.max_rate_enable = false;
        q->q_sch_cfg.min_rate_enable = false;
        // Advanced Flow Control Mode (credit = 0 ; xoff = 1)
        q->q_sch_cfg.adv_fc_mode = BF_TM_SCH_ADV_FC_MODE_CRE;
        q++;
      }
    }
  }
  bf_tm_q_set_hw_ftbl(tm_ctx);

  return (rc);
}

/*
 * Setup the Cache Counters for Queue.
 * per port
 *
 * Related APIs:
 *
 * param dev        ASIC device identifier.
 * param port       port handle
 * param q          port_q handle relative to the port
 * return           Status of API call.
 */
bf_status_t bf_tm_q_set_cache_counters(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t port_q) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_q_get_descriptor(dev, port, port_q, &q);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "get q descriptor failed for port (0x%x) and q (0x%x)", port, port_q);
    return (rc);
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  if (q->counter_state_list != NULL) {
    tm_free_cache_counter_node(TM_QUEUE, q->counter_state_list, g_tm_ctx[dev]);
    q->counter_state_list = NULL;
  }

  // Add the Cached Counter Node for < 64 bit counters wrap case.
  tm_counter_node_id_t node_id;
  tm_cache_counter_node_list_t *node_ptr = NULL;
  TRAFFIC_MGR_MEMSET(&node_id, 0, sizeof(tm_counter_node_id_t));
  node_id.port = port;
  node_id.pipe = DEV_PORT_TO_PIPE(port);
  node_id.q = port_q;
  rc = tm_allocate_and_init_cache_counter_node(
      TM_QUEUE, g_tm_ctx[dev], &node_id, &node_ptr);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to add Cached Counters for TM port (0x%x) and q (0x%x)",
              port,
              port_q);
    q->counter_state_list = NULL;
    // Not sure if we can call the free here
    // bf_tm_ppg_free(dev, *ppg);
  } else {
    q->counter_state_list = node_ptr;
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}
