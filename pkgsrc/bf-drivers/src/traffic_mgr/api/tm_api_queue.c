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
 * Description of APIs for Traffic Manager application to manage
 * egress buffer and queues meant to implement part of QoS behaviour
 * based on traffic properties.
 */

#include <traffic_mgr/traffic_mgr.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "tm_api_helper.h"

/* q_mapping : array of 32 integers specifying queue numbering for
 * finer queue allocation to ports. Return type should be checked.
 * Depending upon hardware resources available at the time of API invocation,
 * special queue carving may or may not be possible to accomodate.
 *
 * Related APIs :
 *    dvm_add_port_with_queues()
 *    bf_tm_allocate_deafult_queues()
 *    bf_tm_allocate_queues()
 *
 */
/**
 * This API can be used to allocate flexible number of queues behind a port.
 * By default straight mapping is established. If flexible queue mapping is
 * desired, this API can be used.
 * Default mapping or when q_mapping is NULL, queue mapping is according to
 * following calculation.
 *    queue# = Ig_intr_md.qid % q_count.
 *    Example Queue mapping in default mode:
 *                   Ig_intr_md.qid 0 --> port qid 0
 *                   Ig_intr_md.qid 1 --> port qid 1
 *                         :
 *                   Ig_intr_md.qid 7 --> port qid 7
 *                         :
 *
 * NOTE: If this API gets called for changing the queue carving (queue count),
 * then application must consider the following:
 *   - always call queue mapping APIs strictly in increasing order of ports
 *     within a port group.
 *   - if queue count gets changed for a port/channel in a port group,
 *     then application must call the queue mapping APIs
 *     for rest of the ports after it (if present) in that port group in
 *     increasing order.
 *   - when several TM ports are configured for a single PM port as 'channels'
 *     with the same scheduling speed (e.g. 2 TM ports for 100Gb port,
 *     or 4 TM ports for 200Gb etc.), then only the first of TM ports will
 *     get HW resources allocated for its queues. Other TM ports may
 *     have empty map with zero q_count.
 *   - port scheduling speed affects amount of hardware resources which is
 *     needed per each queue to serve at the scheduling speed, so it might
 *     happen that the total number of queues requested is too large to
 *     allocate from the port's group common HW resources.
 *   - when a PM port is added, then its related TM ports (channels) become
 *     enabled for TM scheduling and their queue carving is applied to
 *     assign TM hardware resources to queues, so it is recommended to add
 *     PM ports following ascending order in each port group; PM port delete
 *     releases HW queue resources in the port group scope.
 *   - if port scheduling speed, or queue count gets changed, then calling
 *     this API while traffic is running on any port within the port group
 *     may cause traffic disrupted for a short period of time and buffer
 *     accounting would be inconsistent.
 *
 * Related APIs: bf_tm_port_q_mapping_get()
 *               bf_tm_sched_port_enable()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param q_count    number of queues being mapped.
 * param q_mapping  Array of integer values specifying queue mapping
 *                   Mapping is indexed by ig_intr_md.qid.
                     Value q_mapping[ig_intr_md.qid] is port's QID
 *                   :
 * return           Status of API call.
 */
bf_status_t bf_tm_port_q_mapping_set(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint8_t q_count,
                                     uint8_t *q_mapping) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_carve_queues(dev, port, q_count, q_mapping);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * A queue can be optionally assigned to any application pool.
 * When assigned to application pool, static or dynamic shared limit
 * can be set. This API aids to achieve that. If it is desired to not
 * assign queue to any pool, then this API need not be invoked.
 * Advantage of belonging to a pool of queues is that queue can share
 * resources. Hence can grow to dynamic limit depending on burst absorption
 * factor and pool usage number. Dynamic limit will be higher than its own
 * queue limit. If statis limit is used, then  queue can usage upto its
 * own limit before tail drop condition occurs.
 *
 * Default: Queues are not assigned to any application pool.
 *
 * Related APIs: bf_tm_q_app_pool_usage_get()
 *
 * param dev             ASIC device identifier.
 * param port            port handle.
 * param queue           queue identifier. Valid range [ 0..31 ]
 * param pool            Application pool to which queue is assigned to.
 *                       Valid values are BF_TM_EG_POOL0..3.
 * param base_use_limit  Limit to which PPG can grow inside application
 *                       pool. Limit is specified in cell count.
 *                       Once this limit is crossed, if queue burst
 *                       absroption factor (BAF) is non zero, depending
 *                       availability of buffer, queue is allowed to
 *                       use buffer upto BAF limit. If BAF limit is zero,
 *                       queue is treated as static and no dynamic
 *                       buffering is possible.
 * param dynamic_baf     One of the values listed in bf_tm_queue_baf_t
 *                       When BF_TM_QUEUE_BAF_DISABLE is used, queue uses
 *                       static limit.
 * param hysteresis      Hysteresis value of queue in cell count.
 * return                Status of API call.
 */

bf_status_t bf_tm_q_app_pool_usage_set(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bf_tm_app_pool_t pool,
                                       uint32_t base_use_limit,
                                       bf_tm_queue_baf_t dynamic_baf,
                                       uint32_t hysteresis) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  (void)bf_tm_api_hlp_get_pool_details(pool, &dir);
  if (dir == BF_TM_DIR_INGRESS) {
    return (BF_INVALID_ARG);
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    bool is_dyn = bf_tm_api_hlp_is_baf_dynamic(dynamic_baf);
    rc = bf_tm_q_set_app_limit(dev, q, base_use_limit);
    rc |= bf_tm_q_set_is_dynamic(dev, q, is_dyn);
    if (is_dyn) rc |= bf_tm_q_set_baf(dev, q, dynamic_baf);
    rc |= bf_tm_q_set_app_hyst(dev, q, hysteresis);
    // Egress poolid starts from 4 in SW maintained enums. However
    // poolid in HW is between 0 and 3
    rc |= bf_tm_q_set_app_poolid(dev, q, pool - BF_TM_EG_APP_POOL_0);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * This API can be used to disable queue participation in application
 * pool. In such case, once q's gmin limits are used, queue tail drop
 * can occur.
 *
 * Related APIs: bf_tm_q_app_pool_usage_set()
 *
 * param dev        ASIC device identifier.
 * param port       Port handle.
 * param queue      queue identifier
 * return           Status of API call.
 *
 */

bf_status_t bf_tm_q_app_pool_usage_disable(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    // Q is always assigned to some pool. But app pool base limit is set to
    // zero.
    rc = bf_tm_q_set_app_limit(dev, q, 0);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set queue min limits. Inorder to increase min limits, MIN pool should
 * have that many free cells unused. If not available, limits cannot be
 * increased and API fails.When queue limits are decreased, in the absence
 * of shared/application pools, decreased number of cells will be unused
 * and earmarked for future use. These unused cells can be used in future
 * to increase queue limits. If application pools are carved out, then the
 * decreased number of cells are equally distributed to all application pools
 * by raising their pool limits.
 *
 * Default: TM buffer is equally distribted to all queues assuming all queues
 *          are active.
 *
 * Related APIs: bf_tm_q_guaranteed_min_limit_get()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue identifier. Valid range [ 0..31 ]
 * param cells      Queue limits specified in cell count
 */
bf_status_t bf_tm_q_guaranteed_min_limit_set(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_min_limit(dev, q, cells);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set color drop limits for queue. Color drop limits for red should
 * be less than color drop limits of yellow, which inturn is less than
 * color drop limits of green. Green limit is always 100%
 *
 * Default: Color drop limits for yellow and red are set to 75% of gmin size of
 *queue.
 *
 * Related APIs: bf_tm_q_color_drop_enable(), bf_tm_q_color_limit_get()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue whose color drop limit to be set.
 * param color      Color (RED, YELLOW)
 * param limit      Number of cells queue usage/growth can tolerate before
 *                  before appropriate colored packets are dropped.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_color_limit_set(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bf_tm_color_t color,
                                    bf_tm_queue_color_limit_t limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    switch (color) {
      case BF_TM_COLOR_YELLOW:
        rc = bf_tm_q_set_yel_limit_pcent(dev, q, limit);
        break;
      case BF_TM_COLOR_RED:
        rc = bf_tm_q_set_red_limit_pcent(dev, q, limit);
        break;
      case BF_TM_COLOR_GREEN:
      default:
        break;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set queue color hysteresis. When queue's usage for RED/YELLOW colored
 * packets falls below by hysteresis value, tail drop condition is cleared
 * for corresponding colored packets.
 *
 * Default: Set to no hysteresis.
 *
 * Related APIs: bf_tm_q_color_hysteresis_get(),
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue whose color drop limit to be set.
 * param color      Color (RED, YELLOW, GREEN)
 * param cells      Number of cells queue usage should drop to
 *                  before clearing drop condition.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_color_hysteresis_set(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_color_t color,
                                         bf_tm_thres_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  if (!TM_IS_8CELL_UNITS(cells)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        cells,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(cells)));
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    switch (color) {
      case BF_TM_COLOR_YELLOW:
        rc = bf_tm_q_set_yel_hyst(dev, q, cells);
        break;
      case BF_TM_COLOR_RED:
        rc = bf_tm_q_set_red_hyst(dev, q, cells);
        break;
      case BF_TM_COLOR_GREEN:
      default:
        break;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable queue tail drop condition. When queue
 * threshold limits (guranteed min limit + shared limit)
 * are reached, packets are dropped.
 *
 * Default : Trigger drops when queue threshold limits are reached.
 *
 * Related APIs: bf_tm_q_tail_drop_disable()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue for which tail drop has to be enabled.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_tail_drop_enable(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_tail_drop_en(dev, q, true);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable queue tail drop condition. When queue
 * threshold limits (guranteed min limit + shared limit)
 * are reached, packets are not dropped in Egress. This
 * will lead to Ingress drops eventually.
 *
 * Default : Trigger drops when queue threshold limits are reached.
 *
 * Related APIs: bf_tm_q_tail_drop_enable()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue for which tail drop has to be disabled.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_tail_drop_disable(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_tail_drop_en(dev, q, false);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set Queue Stats reporting visibility.
 * "Queue Stats Reporting" (QSTAT) feature: Visible queues report its depth
 * changes to ingress MAU depending on what reporting mode is set at the queue's
 * egress pipe.
 *
 * Default : False
 *
 * Related APIs: bf_tm_q_visible_get()
 *               bf_tm_qstat_report_mode_get()
 *
 * param[in] dev        ASIC device identifier.
 * param[in] port       Port handle.
 * param[in] queue      Queue for port.
 * param[in] visible    QSTAT reporting visibility of the queue.
 * return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_visible_set(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_queue_t queue,
                                bool visible) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_visible(dev, q, visible);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get Queue Stats reporting visibility.
 * "Queue Stats Reporting" (QSTAT) feature: Visible queues report its depth
 * changes to ingress MAU depending on what reporting mode is set at the queue's
 * egress pipe.
 *
 * Default : False
 *
 * Related APIs: bf_tm_q_visible_set()
 *               bf_tm_qstat_report_mode_set()
 *
 * param[in] dev            ASIC device identifier.
 * param[in] port           Port handle.
 * param[in] queue          Queue for port.
 * param[out] visible_sw    QSTAT reporting visibility of the queue in sW.
 * param[out] visible_hw    QSTAT reporting visibility of the queue in HW.
 * return                   Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_visible_get(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_queue_t queue,
                                bool *visible_sw,
                                bool *visible_hw) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_get_visible(dev, q, visible_sw, visible_hw);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get Queue Stats default reporting visibility.
 * "Queue Stats Reporting" (QSTAT) feature: Visible queues report its depth
 * changes to ingress MAU depending on what reporting mode is set at the queue's
 * egress pipe.
 *
 * Default : False
 *
 * Related APIs: bf_tm_q_visible_get()
 *
 * param[in] dev            ASIC device identifier.
 * param[out] visible       QSTAT default reporting visibility of a queue.
 * return                   Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_visible_get_default(bf_dev_id_t dev, bool *visible) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(visible == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  switch (g_tm_ctx[dev]->asic_type) {
    case BF_TM_ASIC_TOFINO:
      rc = BF_NOT_SUPPORTED;
      break;
    case BF_TM_ASIC_TOF2:
      *visible = !(TM_IS_TARGET_ASIC(dev));
      break;
    case BF_TM_ASIC_TOF3:
      *visible = !(TM_IS_TARGET_ASIC(dev));
      break;
    default:
      rc = BF_NOT_SUPPORTED;
      break;
  }
  return (rc);
}

/**
 * Enable queue color drop condition. Based on packet color, when queue
 * color threshold limit are reached, packets are dropped.
 * When color drop is not enabled, packets do not get any treatment
 * based on their color.
 *
 * Default : Trigger drops based on color.
 *
 * Related APIs: bf_tm_q_color_drop_disable()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue whose color drop is to set.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_color_drop_enable(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_color_drop_en(dev, q, true);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable queue color drop condition. Based on packet color, when queue
 * color threshold limit are reached, packets are dropped. When color drop
 * is not enabled, packets do not get any treatment based on their color.
 *
 * Related APIs: bf_tm_q_color_drop_enable()
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue whose color drop is to set.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_color_drop_disable(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_color_drop_en(dev, q, false);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear egress q color dropstate register
 *
 * param dev        ASIC device identifier.
 * param port       port handle.
 * param queue      queue whose color drop is to clear.
 * param color      color id
 * return           Status of API call.
 */
bf_status_t bf_tm_q_color_drop_state_clear(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bf_tm_color_t color) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_clear_egress_drop_state(dev, q, color);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  if (BF_SUCCESS == rc || BF_UNEXPECTED == rc) {
    return rc;
  } else {
    return BF_INTERNAL_ERROR;
  }
}

/**
 * Clear Ingress Buffer Per Queue State Shadow Copy Register.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port handle.
 * @param[in] queue      queue whose dropstate shadow is to clear.
 * @return               Status of API call.
 */
bf_status_t bf_tm_q_shadow_drop_state_clear(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_clear_drop_state_shadow(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Use this API to set (port, queue) used for egressing out
 * negative mirror traffic. Its possible to set one such
 * (port,queue) value for each pipe.
 * Destination mirror on drop port has to be one of the ports
 * in the same pipe where drops are occuring.
 *
 * param dev        ASIC device identifier.
 * param pipe       Pipe Identifier.
 * param port       Negative Mirror port .
 * param queue      Queue where negative mirror traffic is enqueued.
 * return           Status of API call.
 */
bf_status_t bf_tm_port_mirror_on_drop_dest_set(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_dev_port_t port,
                                               bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  bf_dev_port_t l_p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  if (pipe != DEV_PORT_TO_PIPE(port)) {
    // Negative Mirror Destination pipe is not same as
    // pipe from where drops are deflected to.
    return (BF_INVALID_ARG);
  }
  l_p = MAKE_DEV_PORT(pipe, DEV_PORT_TO_LOCAL_PORT(port));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, l_p, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_mirror_on_drop_destination(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * When egress queues need to honour received PFC from downstream,
 * by mapping cos to queue using the API below, queues
 * will not participate in scheduling until PFC gets cleared.
 *
 * Default: All queues are mapping CoS zero.
 *
 * Related APIs: bf_tm_ppg_icos_mapping_set()
 *
 * param dev         ASIC device identifier.
 * param port        Port handle
 * param queue       queue to which CoS is mapped.
 * param cos         CoS associated with the queue.
 * return            Status of API call.
 */
bf_status_t bf_tm_q_pfc_cos_mapping_set(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_queue_t queue,
                                        uint8_t cos) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    // Program queue_config:pfc_pri in sch block
    bf_tm_sch_set_q_pfc_prio(dev, q, cos);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Use this API to set hysteresis value of (port, queue)
 *
 * param dev        ASIC device identifier.
 * param port       port identifier
 * param queue      Queue identifier
 * param hysteresis Hysteresis value of queue in cell count.
 * return           Status of API call.
 */
bf_status_t bf_tm_q_hysteresis_set(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint32_t hysteresis) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  if (!TM_IS_8CELL_UNITS(hysteresis)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        hysteresis,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(hysteresis)));
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_set_app_hyst(dev, q, hysteresis);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}
