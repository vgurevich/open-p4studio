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


#include <traffic_mgr/traffic_mgr_types.h>
#include "traffic_mgr/common/tm_ctx.h"

/**
 * Get per port drop count.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_get(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_dev_port_t port,
                                uint64_t *ig_count,
                                uint64_t *eg_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_port_get_ingress_drop_counter(dev, p, ig_count);
    if (BF_TM_IS_OK(rc)) {
      rc = bf_tm_port_get_egress_drop_counter(dev, p, eg_count);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get per port ingress drop count.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_get(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bf_dev_port_t port,
                                        uint64_t *ig_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_port_get_ingress_drop_counter(dev, p, ig_count);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get per port egress drop count.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_get(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       bf_dev_port_t port,
                                       uint64_t *eg_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_port_get_egress_drop_counter(dev, p, eg_count);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get per port egress drop count.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param color         Color
 * @param count         Per port Packet drops from Egress TM perspective
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_egress_color_drop_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_color_t color,
                                             uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_port_get_egress_drop_color_counter(dev, p, color, count);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set per port drop count cache.
 * This data will not be pushed to hardware.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_cache_set(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_dev_port_t port,
                                      uint64_t ig_count,
                                      uint64_t eg_count) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  if (pipe != DEV_PORT_TO_PIPE(port)) {
    LOG_ERROR("Port=%d is on a different pipe than pipe %d", port, pipe);
    return BF_INVALID_ARG;
  }

  rc = bf_tm_port_drop_ingress_cache_set(dev, port, ig_count);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Failed to set ingress drop count for port=%d", port);
    return rc;
  }

  rc = bf_tm_port_drop_egress_cache_set(dev, port, eg_count);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Failed to set egress drop count for port=%d", port);
    return rc;
  }

  return BF_SUCCESS;
}

/**
 * Set ingress per port drop count cache.
 * This data will not be pushed to hardware.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_ingress_cache_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint64_t drop_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    if (p->counter_state_list != NULL && p->counter_state_list->is_valid) {
      uint32_t ctr_idx =
          TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_INGRESS);
      p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val = drop_count;

      // Clear the previous and hw current counter cache value
      p->counter_state_list->counter_val[ctr_idx].prev_reg_ctr_val = 0;
      p->counter_state_list->counter_val[ctr_idx].hw_cur_reg_ctr_val = 0;
    }
    // Also clear the HW counter
    rc = bf_tm_port_clear_ingress_drop_counter(dev, p);
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}

/**
 * Set egress per port drop count cache.
 * This data will not be pushed to hardware.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_egress_cache_set(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint64_t drop_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc == BF_SUCCESS) {
    if (p->counter_state_list != NULL && p->counter_state_list->is_valid) {
      uint32_t ctr_idx =
          TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_EGRESS);
      p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val = drop_count;

      // Clear the previous and hw current counter cache value
      p->counter_state_list->counter_val[ctr_idx].prev_reg_ctr_val = 0;
      p->counter_state_list->counter_val[ctr_idx].hw_cur_reg_ctr_val = 0;
    }
    // Also clear the HW counter
    rc = bf_tm_port_clear_egress_drop_counter(dev, p);
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}

/**
 * Get per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_cache_get(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_dev_port_t port,
                                      uint64_t *ig_count,
                                      uint64_t *eg_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  BF_TM_INVALID_ARG((NULL == ig_count));
  BF_TM_INVALID_ARG((NULL == eg_count));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);

  if (rc == BF_SUCCESS && p->counter_state_list != NULL &&
      p->counter_state_list->is_valid) {
    uint32_t ctr_idx =
        TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_INGRESS);

    *ig_count = p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
    ctr_idx = TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_EGRESS);
    *eg_count = p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
  } else {
    rc = BF_UNEXPECTED;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get ingress per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ingress_port_drop_cache_get(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              bf_dev_port_t port,
                                              uint64_t *drop_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  BF_TM_INVALID_ARG((NULL == drop_count));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);

  if (rc == BF_SUCCESS && p->counter_state_list != NULL &&
      p->counter_state_list->is_valid) {
    uint32_t ctr_idx =
        TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_INGRESS);
    *drop_count = p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
  } else {
    rc = BF_UNEXPECTED;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get egress per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_egress_port_drop_cache_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bf_dev_port_t port,
                                             uint64_t *drop_count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  BF_TM_INVALID_ARG((NULL == drop_count));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_descriptor(dev, port, &p);

  if (rc == BF_SUCCESS && p->counter_state_list != NULL &&
      p->counter_state_list->is_valid) {
    uint32_t ctr_idx =
        TM_CACHE_CTR_PORT_REL_INDEX(TOTAL_PKTS_DROPPED_ON_PORT_EGRESS);
    *drop_count = p->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
  } else {
    rc = BF_UNEXPECTED;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear per port drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_count_clear(bf_dev_id_t dev, bf_dev_port_t port) {
  return bf_tm_port_drop_cache_set(dev, DEV_PORT_TO_PIPE(port), port, 0, 0);
}

/**
 * Clear per port ingress drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_count_clear(bf_dev_id_t dev,
                                                bf_dev_port_t port) {
  return bf_tm_port_drop_ingress_cache_set(dev, port, 0);
}

/**
 * Clear per port egress drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_count_clear(bf_dev_id_t dev,
                                               bf_dev_port_t port) {
  return bf_tm_port_drop_egress_cache_set(dev, port, 0);
}

/**
 * Get per ppg drop count.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_get(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               bf_tm_ppg_hdl ppg,
                               uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  if ((uint8_t)_pipe != pipe) {
    // pipe specified and PPG passed do not belong to same pipe
    return (BF_INVALID_ARG);
  }
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_get_drop_counter(dev, _ppg, count);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Set per ppg drop count cache.
 * This data will not be pushed to hardware.
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_cache_set(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_tm_ppg_hdl ppg,
                                     uint64_t count) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  if ((uint8_t)_pipe != pipe) {
    // pipe specified and PPG passed do belong to same pipe
    return (BF_INVALID_ARG);
  }
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (_ppg != NULL) {
    if (_ppg->counter_state_list != NULL &&
        _ppg->counter_state_list->is_valid) {
      uint32_t ctr_idx = TM_CACHE_CTR_PPG_REL_INDEX(TOTAL_PKTS_DROPPED_PER_PPG);
      _ppg->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val = count;

      // Clear the previous and hw current counter cache value
      _ppg->counter_state_list->counter_val[ctr_idx].prev_reg_ctr_val = 0;
      _ppg->counter_state_list->counter_val[ctr_idx].hw_cur_reg_ctr_val = 0;
    }

    // Also clear the HW counter
    rc = bf_tm_ppg_clear_drop_counter(dev, _ppg);
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return rc;
}

/**
 * Get per ppg drop count cached 64 bit counter to account for the
 * counter wrap.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_cache_get(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_tm_ppg_hdl ppg,
                                     uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == count));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  if ((uint8_t)_pipe != pipe) {
    // pipe specified and PPG passed do belong to same pipe
    return (BF_INVALID_ARG);
  }
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (_ppg != NULL && _ppg->counter_state_list != NULL &&
      _ppg->counter_state_list->is_valid) {
    uint32_t ctr_idx = TM_CACHE_CTR_PPG_REL_INDEX(TOTAL_PKTS_DROPPED_PER_PPG);
    *count = _ppg->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
  } else {
    rc = BF_UNEXPECTED;
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return rc;
}
/**
 * Clear per ppg drop count.
 *
 * @param dev           ASIC device identifier.
 * @param ppg           PPG identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_count_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  return bf_tm_ppg_drop_cache_set(dev, TM_PPG_PIPE(ppg), ppg, 0);
}

/**
 * Get per queue drop count
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_get(bf_dev_id_t dev,
                             bf_dev_pipe_t pipe,
                             bf_dev_port_t port,
                             bf_tm_queue_t queue,
                             uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) rc = bf_tm_q_get_drop_counter(dev, q, count);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get per queue drop count per TM Subdevice TF3 aand later
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_get_ext(bf_dev_id_t dev,
                                 bf_subdev_id_t die_id,
                                 bf_dev_pipe_t pipe,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_SUBDEV_INVALID(die_id));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS)
    rc = bf_tm_q_get_drop_counter_ext(dev, die_id, q, count);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set per queue drop count cache.
 * This data will not be pushed to hardware.
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_cache_set(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint64_t count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    if (q->counter_state_list != NULL && q->counter_state_list->is_valid) {
      uint32_t ctr_idx = TM_CACHE_CTR_Q_REL_INDEX(TOTAL_PKTS_DROPPED_PER_Q);
      q->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val = count;

      // Clear the previous and hw current counter cache value
      q->counter_state_list->counter_val[ctr_idx].prev_reg_ctr_val = 0;
      q->counter_state_list->counter_val[ctr_idx].hw_cur_reg_ctr_val = 0;
    }

    // Also clear the HW counter
    rc = bf_tm_q_clear_drop_counter(dev, q);
  }

  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}

/**
 * Get per queue drop count cached 64 bit counter to account for the
 * counter wrap.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_cache_get(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  BF_TM_INVALID_ARG((NULL == count));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS && q->counter_state_list != NULL &&
      q->counter_state_list->is_valid) {
    uint32_t ctr_idx = TM_CACHE_CTR_Q_REL_INDEX(TOTAL_PKTS_DROPPED_PER_Q);
    *count = q->counter_state_list->counter_val[ctr_idx].cur_reg_ctr_val;
  } else {
    rc = BF_UNEXPECTED;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear per queue drop count
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_count_clear(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue) {
  return bf_tm_q_drop_cache_set(dev, DEV_PORT_TO_PIPE(port), port, queue, 0);
}

/**
 * Get Buffer full drop  count
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_pipe_buffer_full_drop_get(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_wac_pipe_get_buffer_full_drop_counter(dev, pipe, count);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear Buffer full drop count
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_pipe_buffer_full_drop_clear(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_wac_pipe_clear_buffer_full_drop_counter(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get port current usage count in units of cells.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param port       port identifier.
 * @param ig_count   Port usage count in cells from Ingress TM perspective.
 * @param eg_count   Port usage count in cells from Egress TM perspective.
 * @param ig_wm      Watermark of port in units of cell from
 *                   Ingress TM view point.
 * @param eg_wm      Watermark of port in units of cell from
 *                   Egress TM view point.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_usage_get(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 bf_dev_port_t port,
                                 uint32_t *ig_count,
                                 uint32_t *eg_count,
                                 uint32_t *ig_wm,
                                 uint32_t *eg_wm) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_ingress_usage_counter(dev, p, ig_count);
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_port_get_egress_usage_counter(dev, p, eg_count);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_port_get_ingress_water_mark(dev, p, ig_wm);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_port_get_egress_water_mark(dev, p, eg_wm);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear port watermark counter.
 *
 * @param dev        ASIC device identifier.
 * @param port       port identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_watermark_clear(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_ingress_water_mark(dev, p);

  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_port_clear_egress_water_mark(dev, p);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear port ingress watermark counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_ingress_watermark_clear(bf_dev_id_t dev,
                                               bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_ingress_water_mark(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear port ingress usage counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_ingress_usage_count_clear(bf_dev_id_t dev,
                                                 bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_ingress_usage_counter(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear port egress usage counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_egress_usage_count_clear(bf_dev_id_t dev,
                                                bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_egress_usage_counter(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear port egress watermark counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_egress_watermark_clear(bf_dev_id_t dev,
                                              bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_egress_water_mark(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get Pool usage counters. Valid pools are application/shared pool,
 * negative mirror pool and skid pool. There is no water mark support
 * for negative mirror pool. In negative mirror pool case water mark
 * value will be zero all the time.
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev        ASIC device identifier.
 * @param pool       pool identifier.
 * @param count      Pool usage count in cells.
 * @param wm         Water mark value in units of cell.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_usage_get(bf_dev_id_t dev,
                                 bf_tm_app_pool_t pool,
                                 uint32_t *count,
                                 uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (dir == BF_TM_DIR_INGRESS) {
    bf_tm_ig_spool_get_pool_usage(dev, id, count);
    bf_tm_ig_spool_get_pool_wm(dev, id, wm);
  } else {
    bf_tm_eg_spool_get_pool_usage(dev, id, count);
    bf_tm_eg_spool_get_pool_wm(dev, id, wm);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear shared AP pool watermark counter
 *
 * @param dev        ASIC device identifier.
 * @param pool       pool identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_watermark_clear(bf_dev_id_t dev, bf_tm_app_pool_t pool) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  if (dir == BF_TM_DIR_INGRESS) {
    rc = bf_tm_ig_spool_clear_pool_wm(dev, id);
  } else {
    rc = bf_tm_eg_spool_clear_pool_wm(dev, id);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get PPG usage count.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param ppg        PPG identifier.
 * @param gmin_count   Cell inuse from gmin pool.
 * @param shared_count Cell inuse from shared pool.
 * @param skid_count   Cell inuse from skid pool.
 * @param wm         Water mark value in units of cell.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_usage_get(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_tm_ppg_hdl ppg,
                                uint32_t *gmin_count,
                                uint32_t *shared_count,
                                uint32_t *skid_count,
                                uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], _pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_get_gmin_usage_counter(dev, _ppg, gmin_count);
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_get_shared_usage_counter(dev, _ppg, shared_count);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_get_skid_usage_counter(dev, _ppg, skid_count);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_get_wm_counter(dev, _ppg, wm);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear PPG usage count. [gmin, shared, skid, watermark]
 *
 * @param dev        ASIC device identifier.
 * @param ppg        PPG identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_usage_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], _pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, _pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_clear_gmin_usage_counter(dev, _ppg);
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_clear_shared_usage_counter(dev, _ppg);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_clear_skid_usage_counter(dev, _ppg);
  }
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_ppg_clear_watermark(dev, _ppg);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear PPG watermark counter.
 *
 * @param dev        ASIC device identifier.
 * @param ppg        PPG identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_watermark_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, port, _pipe;
  bf_tm_ppg_t *_ppg;
  bool defppg;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &_pipe, &port);
  _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], _pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, _pipe, dev);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_ppg_clear_watermark(dev, _ppg);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get queue usage count
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Cell inuse .
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_usage_get(bf_dev_id_t dev,
                              bf_dev_pipe_t pipe,
                              bf_dev_port_t port,
                              bf_tm_queue_t queue,
                              uint32_t *count,
                              uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_get_usage_count(dev, q, count);
    if (BF_TM_IS_OK(rc)) {
      rc = bf_tm_q_get_wm_count(dev, q, wm);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get queue usage count per TM Subdevice TF3 aand later.
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Cell inuse .
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_usage_get_ext(bf_dev_id_t dev,
                                  bf_subdev_id_t die_id,
                                  bf_dev_pipe_t pipe,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  uint32_t *count,
                                  uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_SUBDEV_INVALID(die_id));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pipe != DEV_PORT_TO_PIPE(port));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_get_usage_count_ext(dev, die_id, q, count);
    if (BF_TM_IS_OK(rc)) {
      rc = bf_tm_q_get_wm_count_ext(dev, die_id, q, wm);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear queue watermark counter.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_watermark_clear(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_clear_watermark(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear queue usage counter.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_usage_clear(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_queue_t queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_clear_usage_count(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get discard queue current usage counter.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param count         Cell inuse.
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_discard_usage_get(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t *count,
                                      uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  bf_dev_port_t port = 0;
  bf_tm_queue_t queue = 0;
  bf_dev_port_t devport = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_port_mirror_on_drop_dest_get(dev, pipe, &port, &queue);
  if (rc != BF_SUCCESS) return (rc);
  devport = MAKE_DEV_PORT(pipe, port);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, devport, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_get_usage_count(dev, q, count);
    if (BF_TM_IS_OK(rc)) {
      rc = bf_tm_q_get_wm_count(dev, q, wm);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get discard queue current usage counter per TM Subdevice TF3 aand later.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param count         Cell inuse.
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_discard_usage_get_ext(bf_dev_id_t dev,
                                          bf_subdev_id_t die_id,
                                          bf_dev_pipe_t pipe,
                                          uint32_t *count,
                                          uint32_t *wm) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  bf_dev_port_t port = 0;
  bf_tm_queue_t queue = 0;
  bf_dev_port_t devport = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_SUBDEV_INVALID(die_id));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_port_mirror_on_drop_dest_get(dev, pipe, &port, &queue);
  if (rc != BF_SUCCESS) return (rc);
  devport = MAKE_DEV_PORT(pipe, port);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, devport, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_get_usage_count_ext(dev, die_id, q, count);
    if (BF_TM_IS_OK(rc)) {
      rc = bf_tm_q_get_wm_count_ext(dev, die_id, q, wm);
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear discard queue watermark counter.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_q_discard_watermark_clear(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  bf_dev_port_t port = 0;
  bf_tm_queue_t queue = 0;
  bf_dev_port_t devport = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_port_mirror_on_drop_dest_get(dev, pipe, &port, &queue);
  if (rc != BF_SUCCESS) return (rc);
  devport = MAKE_DEV_PORT(pipe, port);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, devport, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_q_clear_watermark(dev, q);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get Pipe counters. Total number of cells, packets received by a pipe.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param cell_count Total Cell count through the pipe.
 * @param pkt_count  Total Cell count through the pipe.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_counters_get(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint64_t *cell_count,
                                    uint64_t *pkt_count) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_get_total_in_cell_count(dev, pipe, cell_count);
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_pipe_get_total_in_pkt_count(dev, pipe, pkt_count);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear Pipe cell counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_cell_counter(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_clear_total_in_cell_count(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear Pipe packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_packet_counter(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_clear_total_in_packet_count(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get Unicast and Multicast cut through packet count through a pipe.
 * Counters are incremented after packet is admitted into TM in egress
 * direction.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param uc_count   Total Unicast cut through packets through the pipe.
 * @param mc_count   Total Multicast cut through packets through the pipe.
 * @return           Status of API call.
 */
bf_status_t bf_tm_cut_through_counters_get(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint64_t *uc_count,
                                           uint64_t *mc_count) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_get_uc_ct_count(dev, pipe, uc_count);
  if (BF_TM_IS_OK(rc)) {
    rc = bf_tm_pipe_get_mc_ct_count(dev, pipe, mc_count);
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear unicast cut-through packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_uc_ct_packet_counter(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_clear_uc_ct_count(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear multicast cut-through packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_mc_ct_packet_counter(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_clear_mc_ct_count(dev, pipe);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Clear blocklevel drop counters for TM.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @param[in] clear_mask    A mask of counters to be cleared
 * @return                  Status of API call.
 */
bf_status_t bf_tm_blklvl_drop_clear(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint32_t clear_mask) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_blklvl_clr_drop_cntrs(dev, pipe, clear_mask);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get blocklevel drop counters for TM.
 * Blocklevel Drops, Error, or Discard Counters
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param blk_cntrs     Block Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_blklvl_drop_get(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  bf_tm_blklvl_cntrs_t *blk_cntrs) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_blklvl_get_drop_cntrs(dev, pipe, blk_cntrs);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get PRE FIFO drop counters for TM.
 * PRE-FIFO Drops, Error, or Discard Counters
 *
 * @param dev           ASIC device identifier.
 * @param fifo_cntrs    PRE FIFO Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_pre_fifo_drop_get(bf_dev_id_t dev,
                                    bf_tm_pre_fifo_cntrs_t *fifo_cntrs) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pre_fifo_get_drop_cntrs(dev, fifo_cntrs);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear PRE FIFO drop counters for TM.
 *
 * @param dev           ASIC device identifier.
 * @param fifo_cntrs    PRE FIFO Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_pre_fifo_drop_clear(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t fifo) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PRE_FIFO_INVALID(fifo))

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pre_fifo_clr_drop_cntrs(dev, pipe, fifo);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*

**
 * Get per port number of packets
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier.
 * @param cell_count Total Cell count through the pipe.
 * @param pkt_count  Total Cell count through the pipe.
 * @return           Status of API call.
 *
bf_status_t
bf_tm_port_stats_get (
  bf_dev_id_t         dev,
  bf_dev_port_t       port,
  uint32_t            *ig_pkt_count,
  uint32_t            *eg_pkt_count)
{


}

*/
