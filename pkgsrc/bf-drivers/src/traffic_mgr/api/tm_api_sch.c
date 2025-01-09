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
 * This file implements egress TM Q scheduling, shaping
 */

#include <traffic_mgr/traffic_mgr_types.h>
#include "traffic_mgr/common/tm_ctx.h"

/*
 * Set queue scheduling priority. Scheduling priority level used when
 * serving guaranteed bandwidth. Higher the number, higher the  priority to
 * select the queue for scheduling.
 *
 * Default: Queue scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_q_remaining_bw_priority_set ()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param queue                 queue
 * @param priority              Scheduling priority of queue.
 */
bf_status_t bf_tm_sched_q_priority_set(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bf_tm_sched_prio_t priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_sched_prio(dev, q, priority, false);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue DWRR weights. These weights are used by queues at same
 * priority level. Across prioirty these weights serve as ratio to
 * share excess or remaining bandwidth.
 *
 * Default: Queue scheduling weights set to 1023
 *
 * Related APIs: bf_tm_sched_q_priority_set(),
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param weight          Weight value. Supported range [ 0.. 1023 ]
 *                        Weight 0  is used to disable the DWRR especially when
 *                        Max Rate Leakybucket is used.
 */
bf_status_t bf_tm_sched_q_dwrr_weight_set(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint16_t weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_dwrr_wt(dev, q, weight);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue shaping rate in units of kbps or pps.
 *
 * Default: Queue shaping rate set to match port bandwidth.
 *          Burst size set to 16384 bytes / approx 10 packets
 *          (assuming 1.5K packet)
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param pps             If set to true, values are in terms of pps
 *                        and packets else in terms of kbps and bytes.
 * @param burst_size      Burst size in packets or bytes.
 * @param rate            Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_q_shaping_rate_set(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool pps,
                                           uint32_t burst_size,
                                           uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_max_rate(dev, q, pps, burst_size, rate);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue shaping rate in units of kbps or pps using provisioning type.
 *
 * Default: Queue shaping rate set to match port bandwidth.
 *          Burst size set to 16384 bytes / approx 10 packets
 *          (assuming 1.5K packet)
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param pps             If set to true, values are in terms of pps
 *                        and packets else in terms of kbps and bytes.
 * @param burst_size      Burst size in packets or bytes.
 * @param rate            Shaper value in pps or kbps.
 * @param prov_type       Shaper provisioning type {UPPER, LOWER, MIN_ERR}
 */
bf_status_t bf_tm_sched_q_shaping_rate_set_provisioning(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_max_rate_provisioning(
        dev, q, pps, burst_size, rate, prov_type);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable token bucket that assures queue shaping rate (pps or bps)
 *
 * Default: Queue shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_q_max_shaping_rate_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_max_shaping_rate_enable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_enable_q_max_rate(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable token bucket that assures queue shaping rate (pps or bps)
 *
 * Default: Queue shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_q_max_shaping_rate_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_max_shaping_rate_disable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_disable_q_max_rate(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue guaranteed rate in terms of pps or kbps.
 *
 * Default: Queue shaping rate set to match port bandwidth.
 *          Burst size set to 18K bytes/12 packets
 *          (assuming 1.5K packet)
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_get()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param pps             If set to true, values are in terms of pps
 *                        and packets, else in terms of kbps and bytes.
 * @param burst_size      Burst size in packets or bytes.
 * @param rate            Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_q_guaranteed_rate_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue,
                                              bool pps,
                                              uint32_t burst_size,
                                              uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_gmin_rate(dev, q, pps, burst_size, rate);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable token bucket that assures queue guaranteed rate (pps or bps)
 *
 * Default: Queue guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_guaranteed_rate_enable(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_enable_q_min_rate(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable token bucket that assures queue guaranteed rate (pps or bps)
 *
 * Default: Queue guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_guaranteed_rate_disable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_disable_q_min_rate(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the queue for
 * scheduling.
 *
 * Default: Queue scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_q_remaining_bw_priority_get ()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param priority        Scheduling priority of queue.
 */
bf_status_t bf_tm_sched_q_remaining_bw_priority_set(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_sched_prio(dev, q, priority, true);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Associate queue with l1 node for further scheduling.
 *
 * Default: By deafult, queue is set to schedule with the default l1 node
 * for a port.
 *
 * Related APIs: bf_tm_sched_q_l1_reset
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_l1_set(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_l1_node_t l1_node,
                                 bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
    if (rc == BF_SUCCESS) {
      rc = bf_tm_sch_q_l1_set(dev, q, l1);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set queue to default l1 node for its port. Note that the port must have at
 * least 1 l1 node assigned to it.
 *
 * Related APIs: bf_tm_sched_q_l1_set
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_l1_reset(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    bf_tm_sch_q_l1_reset(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set l1 node scheduling priority. Scheduling priority level used when
 * serving guaranteed bandwidth. Higher the number, higher the  priority to
 * select the l1 node for scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_remaining_bw_priority_set ()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param l1_node               l1 node
 * @param priority              Scheduling priority of queue.
 */
bf_status_t bf_tm_sched_l1_priority_set(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bf_tm_sched_prio_t priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_l1_sched_prio(dev, l1, priority, false);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Set l1 node DWRR weights. These weights are used by l1 nodes at same
 * priority level. Across priority these weights serve as ratio to
 * share excess or remaining bandwidth.
 *
 * Default: l1 node scheduling weights set to 1023
 *
 * Related APIs: bf_tm_sched_l1_priority_set(),
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1 node         l1 node
 * @param weight          Weight value. Supported range [ 0.. 1023 ]
 *                        Weight 0  is used to disable the DWRR especially when
 *                        Max Rate Leakybucket is used.
 */
bf_status_t bf_tm_sched_l1_dwrr_weight_set(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node,
                                           uint16_t weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_l1_dwrr_wt(dev, l1, weight);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set l1 node shaping rate in units of kbps or pps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 * @param pps             If set to true, values are applied in terms of pps
 *                        and packets, else in terms of kbps and bytes.
 * @param burst_size      Burst size in packets or bytes.
 * @param rate            Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_shaping_rate_set(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node,
                                            bool pps,
                                            uint32_t burst_size,
                                            uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_l1_max_rate(dev, l1, pps, burst_size, rate);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable token bucket that assures l1 node shaping rate (pps or bps)
 *
 * Default: l1 node shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_l1_max_shaping_rate_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_max_shaping_rate_enable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_enable_l1_max_rate(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable token bucket that assures l1 node shaping rate (pps or bps)
 *
 * Default: l1 node shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_l1_max_shaping_rate_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_max_shaping_rate_disable(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_disable_l1_max_rate(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable priority propagation from child queues
 *
 * Default: priority propagation is disabled
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_l1_priority_prop_enable(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_enable_l1_pri_prop(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable priority propagation from child queues
 *
 * Default: priority propagation is disabled
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */

bf_status_t bf_tm_sched_l1_priority_prop_disable(bf_dev_id_t dev,
                                                 bf_dev_port_t port,
                                                 bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_disable_l1_pri_prop(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set l1 node guaranteed rate in terms of pps or kbps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_get()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 * @param pps             If set to true, values are in terms of pps
 *                        and packets, else in terms of kbps and bytes.
 * @param burst_size      Burst size in packets or bytes.
 * @param rate            Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_set(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_l1_node_t l1_node,
                                               bool pps,
                                               uint32_t burst_size,
                                               uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_l1_gmin_rate(dev, l1, pps, burst_size, rate);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the l1 node for
 * scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_remaining_bw_priority_get ()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 * @param priority        Scheduling priority of l1 node.
 */
bf_status_t bf_tm_sched_l1_remaining_bw_priority_set(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_l1_sched_prio(dev, l1, priority, true);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable token bucket that assures l1 node guaranteed rate (pps or bps)
 *
 * Default: l1 node guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_enable(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_enable_l1_min_rate(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable token bucket that assures l1 node guaranteed rate (pps or bps)
 *
 * Default: l1 node guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_disable(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_disable_l1_min_rate(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable l1 node scheduling. If l1 is not associated with a port, the l1 node
 * will be allocated to the port. l1 node cannot already be associated with
 * another port.
 *
 * Default: By deafult, each port receives an l1 one when a queue is first
 * allocated to it.
 *
 * Related APIs: bf_tm_sched_l1_disable
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_enable(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_l1_enable(dev, port, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable l1 node scheduling. If disabled, l1 node will not participate in
 * scheduling.
 *
 * Default: By deafult, each port receives an l1 one when a queue is first
 * allocated to it.
 *
 * Related APIs: bf_tm_sched_l1_enable
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_disable(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_l1_disable(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable l1 node and deallocate node from port. All queues must have been
 * dissocated from this l1 node before calling this function.
 *
 * Default: By deafult, each port receives an l1 one when a queue is first
 * allocated to it.
 *
 * Related APIs: bf_tm_sched_l1_enable
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param l1_node         l1 node
 */
bf_status_t bf_tm_sched_l1_free(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_l1_node_t l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_l1_free(dev, l1);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Enable token bucket that assures port shaping rate (pps or kbps)
 *
 * Default: Port shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_port_shaping_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 */
bf_status_t bf_tm_sched_port_shaping_enable(bf_dev_id_t dev,
                                            bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_enable_port_max_rate(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Disable token bucket that assures port shaping rate (pps or kbps)
 *
 * Default: Port shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_port_shaping_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 */
bf_status_t bf_tm_sched_port_shaping_disable(bf_dev_id_t dev,
                                             bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_disable_port_max_rate(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * @brief Set port shaping rate in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_port_shaping_rate_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] pps             If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[in] burst_size      Burst size in packets or bytes.
 * @param[in] rate            Shaper value in pps or kbps.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_rate_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bool pps,
                                              uint32_t burst_size,
                                              uint32_t rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_port_max_rate(dev, p, pps, burst_size, rate);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * @brief Set port shaping rate in units of kbps or pps using
 * provisioning type.
 *
 * Related APIs: bf_tm_sched_port_shaping_rate_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] pps             If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[in] burst_size      Burst size in packets or bytes.
 * @param[in] rate            Shaper value in pps or kbps.
 * @param[in] prov_type       Shaper provisioning type {UPPER, LOWER, MIN_ERR}
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_rate_set_provisioning(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool pps,
    uint32_t burst_size,
    uint32_t rate,
    bf_tm_sched_shaper_prov_type_t prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_port_max_rate_provisioning(
      dev, p, pps, burst_size, rate, prov_type);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Set number of bytes added per packet to packet length by shaper
 *
 * Default: Zero bytes are added to packet length.
 *
 * Related APIs: bf_tm_sched_pkt_ifg_compensation_get
 *
 * @param dev             ASIC device identifier.
 * @param pipe            Pipe identifier.
 * @param adjustment      Byte adjustment done on every packet.
 */
bf_status_t bf_tm_sched_pkt_ifg_compensation_set(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint8_t adjustment) {
  bf_status_t rc = BF_SUCCESS;
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  g_tm_ctx[dev]->pipes[pipe].ifg_compensation = adjustment;
  rc = bf_tm_sch_set_pkt_ifg_compensation(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Enable queue scheduling. If disabled, queue will not participate in
 * scheduling.
 *
 * Default: By deafult, queue is enabled to schedule its traffic
 *          towards egress pipe/MAC.
 *
 * Related APIs: bf_tm_disable_q
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_enable(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_sched(dev, q, true);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Disable queue scheduling. If disabled, queue will not participate in
 * scheduling.
 *
 * Default: By deafult, queue is enabled to schedule its traffic
 *          towards egress pipe/MAC.
 *
 * Related APIs: bf_tm_enable_q
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 */
bf_status_t bf_tm_sched_q_disable(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_sched(dev, q, false);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Enable port into scheduler. If disabled, the port will not participate
 * in scheduling.
 * The scheduling speed will be set on all channels (TM Ports)
 * starting from the port given depending on how many channels
 * are needed to participate.
 *
 * Default: By deafult, port is enabled to schedule its traffic
 *          towards egress pipe/MAC.
 *
 * Related APIs: bf_tm_sched_port_disable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param speed           Port Speed (10G/25G/40G/50G/100G)
 */
bf_status_t bf_tm_sched_port_enable(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_port_speeds_t speed) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_port_sched(dev, p, speed, true);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Disable port from scheduler. If disabled, the port will not participate
 * in scheduling.
 *
 * Default: By deafult, port is enabled to schedule its traffic
 *          towards egress pipe/MAC.
 *
 * Related APIs: bf_tm_sched_port_enable()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 */
bf_status_t bf_tm_sched_port_disable(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_port_sched(
      dev, p, BF_SPEED_NONE, false); /* Speed doesn't matter when disabling */
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue scheduler advanced flow control mode.
 * Scheduler Advanced Flow Control Mechanism, 0 = Credit 1 = Xoff
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_q_adv_fc_mode_get()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param queue                 queue
 * @param mode                  Scheduler Advanced Flow Control Mode
 *                              0 = credit 1 = xoff.
 */
bf_status_t bf_tm_sched_q_adv_fc_mode_set(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_sched_adv_fc_mode_t mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_adv_fc_mode(dev, q, mode);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Sets scheduler advanced flow control mode enable/disable mode.
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_adv_fc_mode_enable_get()
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Logical PipeId index
 * @param enable                Scheduler Advanced Flow Control Mode
 *                              Enable/Disable
 */

bf_status_t bf_tm_sched_adv_fc_mode_enable_set(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bool enable) {
  bf_status_t rc;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_set_adv_fc_mode_enable(dev, p, enable);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set queue PFC status
 *
 * Related APIs: bf_tm_sched_q_egress_pfc_status_get(),
 *               bf_tm_sched_q_egress_pfc_status_clear()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      port
 * @param[in] queue     queue
 * @param[in] status    Egress Queue PFC Status - True or False value of
 *                      pfc_pause
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_egress_pfc_status_set(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_set_q_egress_pfc_status(dev, q, status);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Clear queue PFC status
 *
 * Related APIs: bf_tm_sched_q_egress_pfc_status_get(),
 *               bf_tm_sched_q_egress_pfc_status_set()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      port
 * @param[in] queue     queue
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_egress_pfc_status_clear(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_clear_q_egress_pfc_status(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}
