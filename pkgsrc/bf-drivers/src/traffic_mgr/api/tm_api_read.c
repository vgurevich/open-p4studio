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
 * This file contains APIs for client application to fetch
 * Traffic Manager configurations.
 */

#include <bf_types/bf_types.h>
#include <traffic_mgr/traffic_mgr_types.h>

#include "traffic_mgr/common/tm_ctx.h"

/**
 * Get TM device-specific settings.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[out] cfg            Config.
 * @return                    Status of API call.
 *
 */
bf_status_t bf_tm_dev_config_get(bf_dev_id_t dev, bf_tm_dev_cfg_t *cfg) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cfg));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  cfg->pipe_cnt = g_tm_ctx[dev]->tm_cfg.pipe_cnt;
  cfg->pg_per_pipe = g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
  cfg->q_per_pg = g_tm_ctx[dev]->tm_cfg.q_per_pg;
  cfg->ports_per_pg = g_tm_ctx[dev]->tm_cfg.ports_per_pg;
  cfg->pfc_ppg_per_pipe = g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe;
  cfg->total_ppg_per_pipe = g_tm_ctx[dev]->tm_cfg.total_ppg_per_pipe;
  cfg->pre_fifo_per_pipe = g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
  cfg->l1_per_pg = g_tm_ctx[dev]->tm_cfg.l1_per_pg;
  cfg->l1_per_pipe = g_tm_ctx[dev]->tm_cfg.l1_per_pipe;
  return (BF_SUCCESS);
}

/**
 * Get number of active pipes.
 *
 * @param[in]  dev       ASIC device identifier.
 * @param[out] count     Number of active pipes.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_get_count(bf_dev_id_t dev, uint8_t *count) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == count));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  *count = g_tm_ctx[dev]->tm_cfg.pipe_cnt;
  return (BF_SUCCESS);
}

/**
 * Get first logical pipe.
 *
 * @param[in]  dev              ASIC device identifier.
 * @param[out] first_l_pipe     First logical pipe.
 * @return                      Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_get_first(bf_dev_id_t dev, bf_dev_pipe_t *first_l_pipe) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == first_l_pipe));
  *first_l_pipe = 0;
  return BF_SUCCESS;
}

/**
 * Get next logical pipe.
 *
 * @param[in]  dev              ASIC device identifier.
 * @param[in]  current_l_pipe   Current logical pipe.
 * @param[out] next_l_pipe      Next logical pipe.
 *                              Might be NULL to only check for the next pipe.
 *
 * @return                      Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_get_next(bf_dev_id_t dev,
                                bf_dev_pipe_t current_l_pipe,
                                bf_dev_pipe_t *next_l_pipe) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(current_l_pipe, g_tm_ctx[dev]));

  if (TM_IS_PIPE_INVALID((current_l_pipe + 1), g_tm_ctx[dev])) {
    return BF_OBJECT_NOT_FOUND;
  }

  if (next_l_pipe) {
    *next_l_pipe = current_l_pipe + 1;
  }
  return BF_SUCCESS;
}

/**
 * Get number of ports per pipe.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier
 *                                  For PIPE_ALL get port count for all pipes.
 * @param[in]  regular_ports_only   Get number of regular ports or all ports
 *                                  including mirror ports.
 * @param[out] count                Number of ports.
 * @return                          Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_port_get_count(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bool regular_ports_only,
                                      uint16_t *count) {
  uint16_t count_ = 0;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == count));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  if (BF_DEV_PIPE_ALL == pipe) {
    count_ = g_tm_ctx[dev]->tm_cfg.pipe_cnt;
  } else {
    count_ = 1;
    BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  }

  // TF1, TF2, TF3 have same number of ports per each pipe.

  if (regular_ports_only) {
    count_ *= (g_tm_ctx[dev]->tm_cfg.ports_per_pg) *
              (g_tm_ctx[dev]->tm_cfg.pg_per_pipe);
  } else {
    count_ *= (g_tm_ctx[dev]->tm_cfg.ports_per_pg) *
                  (g_tm_ctx[dev]->tm_cfg.pg_per_pipe) +
              (g_tm_ctx[dev]->tm_cfg.mirror_port_cnt);
  }
  *count = count_;
  return (BF_SUCCESS);
}

/**
 * Get first devport in a pipe.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier.
 *                                  For PIPE_ALL get the very first port from
 *                                  the first pipe on the device.
 * @param[in]  regular_ports_only   Get only regular port or any (regular or
 *                                  mirror)
 * @param[out] port_id              Device port identifier (pipe id and port
 *                                  number).
 *
 * @return BF_SUCCESS          :    First port in a pipe found
 * @return BF_UNEXPECTED       :    TM context is invalid
 */
bf_status_t bf_tm_pipe_port_get_first(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bool regular_ports_only,
                                      bf_dev_port_t *port) {
  bf_status_t rc = BF_UNEXPECTED;
  // This parameter was introduced for cases when a mirror port can be placed at
  // the beginning of all ports (e.g. port 0). For now, for TOF1, TOF2, TOF3
  // mirror port is placed at the end of ports (port 72) so this parameter has
  // no matter. Reserved for future TOF architectures.
  (void)regular_ports_only;
  BF_TM_INVALID_ARG((NULL == port));
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  if (BF_DEV_PIPE_ALL == pipe) {
    rc = bf_tm_pipe_get_first(dev, &pipe);
    if (BF_SUCCESS != rc) {
      return rc;
    }
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  *port = MAKE_DEV_PORT(pipe, 0);
  return BF_SUCCESS;
}

/**
 * Check the pipe is valid.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  pipe                Device pipe to validate.
 *                                 BF_DEV_PIPE_ALL is not a valid value.
 *
 * @return BF_SUCCESS          :   If the pipe is valid for the device.
 *
 */
bf_status_t bf_tm_pipe_is_valid(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  return BF_SUCCESS;
}

/**
 * Check the port is valid.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  dev_port            Device port to validate.
 *
 * @return BF_SUCCESS          :   If the port is valid for the device.
 *
 */
bf_status_t bf_tm_port_is_valid(bf_dev_id_t dev, bf_dev_port_t dev_port) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev] || NULL == g_tm_ctx[dev]->ports) {
    return (BF_UNEXPECTED);
  }

  // Validate the dev port value (checks for out of range values)
  BF_TM_INVALID_ARG(dev_port !=
                    MAKE_DEV_PORT(DEV_PORT_TO_PIPE(dev_port),
                                  DEV_PORT_TO_LOCAL_PORT(dev_port)));

  // Validate the pipe value
  BF_TM_INVALID_ARG(
      TM_IS_PIPE_INVALID(DEV_PORT_TO_PIPE(dev_port), g_tm_ctx[dev]));

  // Validate the local port value
  uint32_t local_ports_cnt = BF_TM_PORTS_PER_PIPE(g_tm_ctx[dev]);
  bf_dev_port_t lld_port = local_ports_cnt;
  lld_err_t lld_err =
      lld_sku_map_devport_from_user_to_device_safe(dev, dev_port, &lld_port);

  if (LLD_OK != lld_err ||
      DEV_PORT_TO_LOCAL_PORT(lld_port) >= local_ports_cnt) {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/**
 * Get next devport in a pipe.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  current_port        Current device port identifier.
 * @param[in]  regular_ports_only  Get only regular next port (0..71), exclude
 *                                 special (mirror ports). Set to true to get
 *                                 regular ports iterator, set to false to get
 *                                 all ports iterator.
 * @param[out] next_port           Next device port identifier.
 *                                 Might be NULL if only check for the next
 *                                 port.
 *
 * @return BF_SUCCESS          :   Next port found
 * @return BF_OBJECT_NOT_FOUND :   Next port not found
 * @return BF_UNEXPECTED       :   TM context is invalid
 * @return BF_NOT_IMPLEMENTED  :   This functionality is not implemented for
 *                                 current Tofino version
 *
 */
bf_status_t bf_tm_pipe_port_get_next(bf_dev_id_t dev,
                                     bf_dev_port_t current_port,
                                     bool regular_ports_only,
                                     bf_dev_port_t *next_port) {
  bf_status_t status = bf_tm_port_is_valid(dev, current_port);
  if (BF_SUCCESS != status) {
    return status;
  }

  uint8_t cur_lport = DEV_PORT_TO_LOCAL_PORT(current_port);
  bf_dev_pipe_t cur_pipe = DEV_PORT_TO_PIPE(current_port);
  uint8_t next_lport = 0;
  if (BF_TM_IS_TOFINO(g_tm_ctx[dev]->asic_type) ||
      BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type) ||
      BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    next_lport = cur_lport + g_tm_ctx[dev]->tm_cfg.chnl_mult;
  } else {
    return BF_NOT_IMPLEMENTED;
  }

  if (TM_IS_PORT_INVALID(next_lport, g_tm_ctx[dev])) {
    return BF_OBJECT_NOT_FOUND;
  }

  // here some additional checks
  bf_dev_port_t next_devport = MAKE_DEV_PORT(cur_pipe, next_lport);
  bf_tm_port_t *n_port = BF_TM_PORT_PTR(g_tm_ctx[dev], next_devport);
  if (cur_pipe != n_port->l_pipe || next_lport != n_port->uport) {
    LOG_ERROR(
        "%s:%d port %d - next port %d check failed: "
        "l_pipe(%d != %d) or l_port(%d != %d)",
        __func__,
        __LINE__,
        current_port,
        next_devport,
        cur_pipe,
        n_port->l_pipe,
        next_lport,
        n_port->uport);
    return BF_UNEXPECTED;
  }

  if (regular_ports_only &&
      n_port->port >= g_tm_ctx[dev]->tm_cfg.mirror_port_start) {
    return BF_OBJECT_NOT_FOUND;
  }

  if (next_port) {
    *next_port = next_devport;
  }
  return BF_SUCCESS;
}

/**
 * Get number of supported iCoS.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[in]  port           Port Identifier
 * @param[out] icos_count     Number of supported iCoS.
 * @return                    Status of API call.
 *
 */
bf_status_t bf_tm_port_icos_count_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint8_t *icos_count) {
  (void)port;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == icos_count));

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  *icos_count = g_tm_ctx[dev]->tm_cfg.icos_count;

  return (BF_SUCCESS);
}

/**
 * Get first dev_port in a port group.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier.
 * @param[in]  pg_id                Device port group (a.k.a. 'quad').
 * @param[out] port_id              Device port identifier (pipe id and port
 *                                  number).
 *
 * @return BF_SUCCESS          :    First port in a pipe found
 * @return BF_UNEXPECTED       :    TM context is invalid
 */
bf_status_t bf_tm_pipe_port_group_get_first_port(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_tm_pg_t pg_id,
                                                 bf_dev_port_t *port_id) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pg_id >= (g_tm_ctx[dev]->tm_cfg.pg_per_pipe));
  BF_TM_INVALID_ARG((NULL == port_id));

  bf_dev_port_t port_ = BF_TM_PG_BASE_DEV_PORT(g_tm_ctx[dev], pipe, pg_id);
  if (!DEV_PORT_VALIDATE(port_)) {
    return BF_UNEXPECTED;
  }
  *port_id = port_;
  return BF_SUCCESS;
}

/**
 * Get Port Group of a port.
 *
 * @param[in]   dev         ASIC device identifier.
 * @param[in]   port_id     Device port identifier (pipe id and port number).
 * @param[out]  pg_id       Device port group (a.k.a. 'quad').
 * @param[out]  pg_port_nr  Port number in its group.
 * @return                  Status of API call.
 *
 */
bf_status_t bf_tm_port_group_get(bf_dev_id_t dev,
                                 bf_dev_port_t port_id,
                                 bf_tm_pg_t *pg_id,
                                 uint8_t *pg_port_nr) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  // Only regular ports with port group
  BF_TM_INVALID_ARG(!DEV_PORT_VALIDATE(port_id));

  BF_TM_INVALID_ARG(
      TM_IS_PIPE_INVALID(DEV_PORT_TO_PIPE(port_id), g_tm_ctx[dev]));

  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port_id, g_tm_ctx[dev]));

  bf_tm_port_t *port_p = BF_TM_PORT_PTR(g_tm_ctx[dev], port_id);

  if (NULL == port_p || 0 == g_tm_ctx[dev]->tm_cfg.ports_per_pg) {
    return (BF_UNEXPECTED);
  }

  if (NULL != pg_id) {
    *pg_id = port_p->pg;
  }

  if (NULL != pg_port_nr) {
    *pg_port_nr = (port_p->port) % (g_tm_ctx[dev]->tm_cfg.ports_per_pg);
  }

  return (BF_SUCCESS);
}

/**
 * Get base Port Group queue for a Port.
 *
 * Related APIs: bf_tm_q_get_descriptor()
 *               bf_tm_pg_port_queue_get()
 *
 * @param[in]  dev        ASIC device identifier.
 * @param[in]  port_id    Device port identifier (pipe id and port number).
 * @param[out] pg_id      Device port group (a.k.a. 'quad') of the port.
 * @param[out] pg_queue   Port group queue number (physical queue) which
 *                        corresponds to the very first queue of the port.
 * @param[out] is_mapped  True if the physical queue (identified by pg_id and
 *                        pg_queue) is currently mapped as the very first queue
 *                        of the port_id, otherwise it is either not carved or
 *                        it belongs to the next port in the same port group.
 * @return                Status of the API call.
 */
bf_status_t bf_tm_port_base_queue_get(bf_dev_id_t dev,
                                      bf_dev_port_t port_id,
                                      bf_tm_pg_t *pg_id,
                                      uint8_t *pg_queue,
                                      bool *is_mapped) {
  bf_tm_q_profile_t *q_prof;
  int q_profile_index = 0;
  int q_nr = 0;

  BF_TM_INVALID_ARG(pg_id == NULL);
  BF_TM_INVALID_ARG(pg_queue == NULL);
  BF_TM_INVALID_ARG(is_mapped == NULL);

  bf_status_t status = bf_tm_port_group_get(dev, port_id, pg_id, NULL);
  if (BF_SUCCESS != status) return (status);

  // Get device port TM queue mapping profile.
  bf_tm_port_get_q_profile(dev, port_id, &q_profile_index);

  if (q_profile_index >= g_tm_ctx[dev]->tm_cfg.q_prof_cnt) {
    LOG_ERROR("dev=%d dev_port=%d incorrect queue profile index %d",
              dev,
              port_id,
              q_profile_index);
    return (BF_UNEXPECTED);
  }
  q_prof = g_tm_ctx[dev]->q_profile + q_profile_index;

  // Queue number at the port's current carving scope.
  q_nr = q_prof->base_q;

  if (q_nr < 0 || q_nr >= (g_tm_ctx[dev]->tm_cfg.q_per_pg)) {
    LOG_ERROR("dev=%d dev_port=%d incorrect queue %d", dev, port_id, q_nr);
    return (BF_UNEXPECTED);  // Consistency check failed
  }

  *pg_queue = *((uint8_t *)(&q_nr));
  *is_mapped = (q_prof->q_count > 0);

  return (BF_SUCCESS);
}

/**
 * Get Port Queue number by its Port Group physical queue.
 *
 * Related APIs: bf_tm_q_get_descriptor()
 *
 *
 * @param[in]  dev_tgt    Pipeline on an ASIC device.
 * @param[in]  pg_id      Device port group (a.k.a. 'quad').
 * @param[in]  pg_queue   Port group queue number (physical queue).
 * @param[out] port_id    Device port identifier (pipe id and port number).
 * @param[out] queue_nr   Port queue number to use with bf_tm_q_* APIs.
 *                        If the Port group queue is currently not mapped to
 *                        any port queue, then queue_nr is equal to pg_queue,
 *                        port_id is set to the first port in the port group,
 *                        and is_mapped equals to false.
 * @param[out] is_mapped  True if the physical queue (identified by pg_id and
 *                        pg_queue) is currently mapped to the queue_nr at
 *                        port_id, otherwise the port_id is set to the
 *                        port's group first port and queue_nr is equal to
 *                        the pg_queue.
 * @return                Status of the API call.
 */
bf_status_t bf_tm_pg_port_queue_get(bf_dev_target_t *dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue,
                                    bf_dev_port_t *port_id,
                                    bf_tm_queue_t *queue_nr,
                                    bool *is_mapped) {
  bf_tm_q_profile_t *q_prof;
  int q_profile_index = 0;
  int q_nr = 0;

  BF_TM_INVALID_ARG(dev_tgt == NULL);

  bf_dev_id_t dev_id = dev_tgt->device_id;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev_id));
  if (NULL == g_tm_ctx[dev_id]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(dev_tgt->dev_pipe_id, g_tm_ctx[dev_id]));
  BF_TM_INVALID_ARG(port_id == NULL);
  BF_TM_INVALID_ARG(queue_nr == NULL);
  BF_TM_INVALID_ARG(is_mapped == NULL);
  BF_TM_INVALID_ARG(pg_id >= (g_tm_ctx[dev_id]->tm_cfg.pg_per_pipe));
  BF_TM_INVALID_ARG(pg_queue >= (g_tm_ctx[dev_id]->tm_cfg.q_per_pg));

  /* 1) Get device port from pg_id. */
  bf_dev_port_t pg_base_port = 0;
  bf_status_t rc = bf_tm_pipe_port_group_get_first_port(
      dev_id, dev_tgt->dev_pipe_id, pg_id, &pg_base_port);
  if (rc != BF_SUCCESS) return (rc);

  bf_tm_eg_q_t *pg_queues =
      BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[dev_id], pg_base_port);

  if (pg_queues[pg_queue].pg != pg_id) {
    return (BF_UNEXPECTED);  // Consistency check failed
  }
  if (pg_queues[pg_queue].logical_q != pg_queue) {
    return (BF_UNEXPECTED);  // Consistency check failed
  }
  if (pg_queues[pg_queue].l_pipe != dev_tgt->dev_pipe_id) {
    return (BF_UNEXPECTED);  // Consistency check failed
  }
  if (!pg_queues[pg_queue].in_use) {
    return (BF_NOT_READY);
  }

  /* Extract from the Queue info which port is currently carved to it. */
  *port_id =
      MAKE_DEV_PORT(pg_queues[pg_queue].l_pipe, pg_queues[pg_queue].uport);

  /* 2) Get device port TM queue mapping profile. */
  bf_tm_port_get_q_profile(dev_id, *port_id, &q_profile_index);
  if (q_profile_index >= g_tm_ctx[dev_id]->tm_cfg.q_prof_cnt) {
    LOG_ERROR("dev=%d dev_port=%d incorrect queue profile index %d",
              dev_id,
              *port_id,
              q_profile_index);
    return (BF_UNEXPECTED);
  }
  q_prof = g_tm_ctx[dev_id]->q_profile + q_profile_index;

  /* Queue number at the port's current carving scope. */
  q_nr = (int)pg_queue - q_prof->base_q;

  if (q_nr < 0 || q_nr >= (g_tm_ctx[dev_id]->tm_cfg.q_per_pg)) {
    LOG_ERROR(
        "dev=%d dev_port=%d pg_id=%d (base queue %d) "
        "profile %d is incorrect for pg_queue=%d",
        dev_id,
        *port_id,
        pg_id,
        q_prof->base_q,
        q_profile_index,
        pg_queue);
    return (BF_UNEXPECTED);  // Consistency check failed
  }

  if (q_nr < q_prof->q_count) {
    *queue_nr = (bf_tm_queue_t)q_nr;
    *is_mapped = true;
  } else {
    /* This port group queue is currently not mapped to a port queue. */
    *queue_nr = pg_queue;
    *port_id = pg_base_port;
    *is_mapped = false;
  }

  return (BF_SUCCESS);
}

/*
 * When egress queues need to honour received PFC from downstream,
 * by mapping cos to queue using the API below, queues
 * will not participate in scheduling until PFC gets cleared.
 *
 * Default: All queues are mapping CoS zero.
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_set()
 *
 * param[in]  dev    ASIC device identifier.
 * param[in]  port   Device port id.
 * param[in]  queu   Port Queue number.
 * param[out] cos    PFC CoS associated with the queue.
 * return            Status of the API call.
 */
bf_status_t bf_tm_q_pfc_cos_mapping_get(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_queue_t queue,
                                        uint8_t *cos) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(cos == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) {
    return (rc);
  }
  // Get HW pfc_pri if possible from SCH.
  rc = bf_tm_sch_get_q_pfc_prio(dev, q, cos, cos);
  return (rc);
}

/*
 * PFC default CoS value.
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_get()
 *
 * param[in]  dev    ASIC device identifier.
 * param[out] cos    PFC CoS default value.
 * return            Status of the API call.
 */
bf_status_t bf_tm_q_pfc_cos_mapping_get_default(bf_dev_id_t dev, uint8_t *cos) {
  BF_TM_INVALID_ARG(cos == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  bf_status_t rc = BF_SUCCESS;
  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);
  if (BF_SUCCESS == rc && def.q_cos_is_valid) {
    *cos = def.q_cos;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get pipe limit.  Default value of the pipe limit
 * is set to maximum buffering capability of the traffic manager.
 *
 * When admitting packet into Traffic manager, apart from other
 * checks, the packet has to also pass usage check on per egress pipe
 * usage limit. A packet destined to egress pipe whose limit  has
 * crossed, will not be admitted.
 *
 * Related API: bf_tm_pipe_egress_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier
 * @param cells      Limits in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_egress_limit_get(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint32_t *cells) {
  bf_status_t rc;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == cells))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  return (bf_tm_pipe_get_limit(dev, p, cells, cells));
}

/*
 * Get pipe default limit.
 * Default value of the pipe limit is set to maximum buffering
 * capability of the traffic manager.
 *
 * When admitting packet into Traffic manager, apart from other
 * checks, the packet has to also pass usage check on per egress pipe
 * usage limit. A packet destined to egress pipe whose limit  has
 * crossed, will not be admitted.
 *
 * Related API: bf_tm_pipe_egress_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier
 * @param cells      Default limit in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_egress_limit_get_default(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t *cells) {
  (void)pipe;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cells))

  bf_tm_pipe_defaults_t def;
  rc = bf_tm_pipe_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc) {
    if (def.egress_limit_cells_is_valid) {
      *cells = def.egress_limit_cells;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  return (rc);
}

/*
 * Get pipe hysteresis limit. When usage of cells goes below the hysteresis
 * limit, pipe level drop condition  will be cleared.
 *
 * Related API: bf_tm_pipe_egress_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier
 * @param cells      Limits in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_egress_hysteresis_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint32_t *cells) {
  bf_status_t rc;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  return (bf_tm_pipe_get_hyst(dev, p, cells, cells));
}

/*
 * Get pipe default hysteresis limit.
 * When usage of cells goes below the hysteresis limit,
 * pipe level drop condition  will be cleared.
 *
 * Related API: bf_tm_pipe_egress_hysteresis_get()
 *
 * @param dev[in]     ASIC device identifier.
 * @param pipe[in]    Pipe Identifier
 * @param cells[out]  Limits in terms of number of cells.
 * @return            Status of API call.
 */
bf_status_t bf_tm_pipe_egress_hysteresis_get_default(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cells))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  bf_tm_pipe_defaults_t def;
  rc = bf_tm_pipe_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc) {
    if (def.egress_hysteresis_cells_is_valid) {
      *cells = def.egress_hysteresis_cells;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  return rc;
}

/*
 * This API can be used to get queue count and mapping of a port.
 *
 * Related APIs: bf_tm_port_q_mapping_set ()
 *
 * @param dev        ASIC device identifier.
 * @param port       port handle.
 * @param q_count    number of queues used for the port.
 * @param q_mapping  Array of integer values specifying queue mapping
 *                   Mapping is indexed by ig_intr_md.qid.
 *                   Value q_mapping[ig_intr_md.qid] is port's QID
 *                   :
 *                   Caller has to provide array of size 32.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_q_mapping_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint8_t *q_count,
                                     uint8_t *q_mapping) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == q_count))
  BF_TM_INVALID_ARG((NULL == q_mapping))

  bf_tm_q_profile_t *q_prof;
  int q_profile_index = 0, i;

  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof = g_tm_ctx[dev]->q_profile + q_profile_index;
  *q_count = q_prof->q_count;
  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.q_per_pg; i++) {
    q_mapping[i] = q_prof->q_mapping[i];
  }
  return (BF_SUCCESS);
}

/*
 * This API can be used to get physical queue for particular port and ingress
 * qid.
 *
 * @param dev         ASIC device identifier.
 * @param port        port handle.
 * @param ingress_qid ingress qid.
 * @param log_pipe    logical pipe ID.
 * @param phys_q      physical queue ID.
 *
 * @return            Status of API call.
 */
bf_status_t bf_tm_port_pipe_physical_queue_get(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               uint32_t ingress_qid,
                                               bf_dev_pipe_t *log_pipe,
                                               uint32_t *phys_q) {
  BF_TM_INVALID_ARG(log_pipe == NULL);
  BF_TM_INVALID_ARG(phys_q == NULL);

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(NULL == g_tm_ctx[dev]);

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(DEV_PORT_TO_PIPE(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  *log_pipe = DEV_PORT_TO_PIPE(port);
  /*
  commented HW state read func and use SW state
  bf_status_t rc =
      bf_tm_q_get_pipe_physical_queue(dev, port, ingress_qid, log_pipe, phys_q);
  */

  // Get device port TM queue mapping profile.
  bf_tm_q_profile_t *q_prof;
  int q_profile_index = 0;
  int q_nr = 0;
  bf_tm_port_get_q_profile(dev, port, &q_profile_index);
  q_prof = g_tm_ctx[dev]->q_profile + q_profile_index;

  // Queue number at the port's current carving scope.
  q_nr = q_prof->base_q;

  if (q_nr < 0 || q_nr >= (g_tm_ctx[dev]->tm_cfg.q_per_pg)) {
    return (BF_UNEXPECTED);  // Consistency check failed
  }
  bool is_mapped = (q_prof->q_count > 0);
  if (is_mapped) {
    bf_tm_pg_t pg_id =
        DEV_PORT_TO_LOCAL_PORT(port) /
        (g_tm_ctx[dev]->tm_cfg.ports_per_pg * g_tm_ctx[dev]->tm_cfg.chnl_mult);

    *phys_q = (q_nr + q_prof->q_mapping[ingress_qid]) +
              pg_id * (g_tm_ctx[dev]->tm_cfg.q_per_pg);
  } else {
    *phys_q = q_nr;
  }
  return (BF_SUCCESS);
}

/*
 * This API can be used to get port and ingress qid list for
 * particular physical queue
 *
 * @param dev              ASIC device identifier.
 * @param log_pipe         Logical pipe ID.
 * @param pipe_queue       Physical queue ID.
 * @param port             Port handle {logical pipe id, port id}.
 * @param qid_count        Number of ingress qids (first qid_count
 *                         in array is significant).
 * @param qid_list         Ingress qids list.
 *                         :
 *                         Caller has to provide array of size 32 for TOF or
 *                         128 for TOF2
 *                         if physical queue is not currently mapped, port is
 *                         set to the port's group first port and
 *                         qid_count is set to the 0
 *
 * @return                 Status of API call.
 */
bf_status_t bf_tm_pipe_queue_qid_list_get(bf_dev_id_t dev,
                                          bf_dev_pipe_t log_pipe,
                                          bf_tm_queue_t pipe_queue,
                                          bf_dev_port_t *port,
                                          uint32_t *qid_count,
                                          bf_tm_queue_t *qid_list) {
  BF_TM_INVALID_ARG(port == NULL);
  BF_TM_INVALID_ARG(qid_count == NULL);
  BF_TM_INVALID_ARG(qid_list == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(log_pipe, g_tm_ctx[dev]));

  bf_status_t rc = BF_SUCCESS;
  bf_dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = log_pipe;

  bf_tm_pg_t pg_id = pipe_queue / g_tm_ctx[dev]->tm_cfg.q_per_pg;
  uint8_t pg_queue = pipe_queue % g_tm_ctx[dev]->tm_cfg.q_per_pg;
  bf_dev_port_t devport;
  bf_tm_queue_t queue_nr;
  bool is_mapped;

  rc = bf_tm_pg_port_queue_get(
      &dev_tgt, pg_id, pg_queue, &devport, &queue_nr, &is_mapped);

  if (rc != BF_SUCCESS) return rc;
  if (!is_mapped) {
    *port = devport;
    *qid_count = 0;
    return rc;
  }
  *port = devport;

  uint8_t q_map[BF_TM_MAX_QUEUE_PER_PG];
  uint8_t count = 0;
  rc = bf_tm_port_q_mapping_get(dev, devport, &count, q_map);
  if (rc != BF_SUCCESS) return rc;

  int j = 0;
  for (int i = 0; i < count; ++i) {
    if (q_map[i] == queue_nr) {
      qid_list[j] = i;
      j++;
    }
  }
  *qid_count = j;
  return (rc);
}

/*
 * A queue can be optionally assigned to any application pool.
 * When assigned to application pool, get static or dynamic shared limit
 *
 * Related APIs: bf_tm_q_app_pool_usage_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            port handle.
 * @param queue           queue identifier. Valid range [ 0..31 ]
 * @param pool            Application pool to which queue is assigned to.
 *                        Valid values are BF_TM_EG_POOL0..3.
 * @param base_use_limit  Limit to which PPG can grow inside application
 *                        pool. Once this limit is crossed, if queue burst
 *                        absroption factor (BAF) is non zero, depending
 *                        availability of buffer, queue is allowed to
 *                        use buffer upto BAF limit. If BAF limit is zero,
 *                        queue is treated as static and no dynamic
 *                        thresholding.
 * @param dynamic_baf     One of the values listed in bf_tm_queue_baf_t
 *                        When BF_TM_QUEUE_BAF_DISABLE is used, queue uses
 *                        static limit.
 * @param hysteresis      Hysteresis value of queue.
 * @return                Status of API call.
 */

bf_status_t bf_tm_q_app_pool_usage_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bf_tm_app_pool_t *pool,
                                       uint32_t *base_use_limit,
                                       bf_tm_queue_baf_t *dynamic_baf,
                                       uint32_t *hysteresis) {
  bf_status_t rc = BF_UNEXPECTED;
  bf_tm_eg_q_t *q = NULL;
  bool is_dyn = false;
  uint8_t pid = 0;
  uint8_t baf = 0;

  BF_TM_INVALID_ARG(pool == NULL);
  BF_TM_INVALID_ARG(base_use_limit == NULL);
  BF_TM_INVALID_ARG(dynamic_baf == NULL);
  BF_TM_INVALID_ARG(hysteresis == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }

  // Read from SW only where the value should be already set.
  rc = bf_tm_q_get_app_limit(dev, q, base_use_limit, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }

  rc = bf_tm_q_get_is_dynamic(dev, q, &is_dyn, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }

  if (is_dyn) {
    rc = bf_tm_q_get_baf(dev, q, &baf, NULL);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      return (rc);
    }
    *dynamic_baf = baf;
  } else {
    *dynamic_baf = BF_TM_Q_BAF_DISABLE;
  }

  rc = bf_tm_q_get_app_hyst(dev, q, hysteresis, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }

  rc = bf_tm_q_get_app_poolid(dev, q, &pid, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  *pool = pid + BF_TM_EG_APP_POOL_0;  // Logical numbering of egress pools is
                                      // after all ingress pools. However in
                                      // HW egress pools are numbered from 0;
  return (rc);
}

/**
 * A queue can be optionally assigned to any application pool.
 * When assigned to application pool, get pool ID
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            port handle.
 * @param[in] queue           queue identifier. Valid range [ 0..31 ] for
 *                            Tofino and [0..127] for Tofino2
 * @param[out] pool           Application pool to which queue is assigned to.
 *                            Valid values are BF_TM_EG_POOL0..3.
 *                            pool set to BF_TM_APP_POOL_LAST if queue is not
 *                            assigned to AP
 * @return                    Status of API call.
 */

bf_status_t bf_tm_q_app_poolid_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bf_tm_app_pool_t *pool) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  uint8_t pid = 0;
  BF_TM_INVALID_ARG(pool == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  *pool = BF_TM_APP_POOL_LAST;
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_q_get_app_poolid(dev, q, &pid, &pid);
  *pool = pid + BF_TM_EG_APP_POOL_0;  // Logical numbering of egress pools is
                                      // after all ingress pools. However in
                                      // HW egress pools are numbered from 0;
  return (rc);
}

/**
 * @brief Get queue fast recovery mode.
 *
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[in]  port            Port handle.
 * @param[in]  queue           Queue identifier. Valid range [0..31] for
 *                             Tofino and [0..127] for Tofino2
 * @param[out] fast_recovery   Queue fast recovery mode.
 * @return                     Status of the API call.
 */
bf_status_t bf_tm_q_fast_recovery_mode_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool *fast_recovery) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(fast_recovery == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }

  rc = bf_tm_q_get_fast_recovery_mode(dev, q, fast_recovery, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/*
 * Default shared pool settings for a queue.
 *
 * Related APIs: bf_tm_q_app_pool_usage_get()
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[out] pool            Default application pool to which a queue
 *                             is assigned.
 * @param[out] base_use_limit  Default limit to which a queue can grow inside
 *                             an application pool.
 * @param[out] dynamic_baf     Default BAF setting for a queue.
 * @param[out] hysteresis      Default Hysteresis value of a queue.
 * @return                     Status of API call.
 */
bf_status_t bf_tm_q_app_pool_usage_get_default(bf_dev_id_t dev,
                                               bf_tm_app_pool_t *pool,
                                               uint32_t *base_use_limit,
                                               bf_tm_queue_baf_t *dynamic_baf,
                                               uint32_t *hysteresis) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((pool == NULL));
  BF_TM_INVALID_ARG((base_use_limit == NULL));
  BF_TM_INVALID_ARG((dynamic_baf == NULL));
  BF_TM_INVALID_ARG((hysteresis == NULL));

  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);
  if (BF_SUCCESS == rc && def.q_app_pool_is_valid &&
      def.q_base_use_limit_is_valid && def.q_dynamic_baf_is_valid &&
      def.q_qac_hysteresis_is_valid) {
    *pool = def.q_app_pool;
    *base_use_limit = def.q_base_use_limit;
    *dynamic_baf = def.q_dynamic_baf;
    *hysteresis = def.q_qac_hysteresis;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get queue hysteresis in terms of buffer cells.
 *
 * Related APIs: bf_tm_q_hysteresis_set()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue identifier. Valid range [ 0..31 ]
 * @param[out] cells  Queue hysteresis specified in cell count.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_hysteresis_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(cells == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  // Read from SW only where the value should be already set.
  rc = bf_tm_q_get_app_hyst(dev, q, cells, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/*
 * Get queue limit set at a shared application pool.
 * The queue does not use a shared pool if the cells value is zero.
 *
 * Related APIs: bf_tm_q_app_pool_usage_get()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue identifier. Valid range [ 0..31 ]
 * @param[out] cells  Queue limit set at a shared pool in cell count.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_app_pool_limit_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(cells == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  // Read from SW only where the value should be already set.
  rc = bf_tm_q_get_app_limit(dev, q, cells, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/*
 * Get queue min limits. Returned limits are accounted in terms of cells.
 *
 * Related APIs: bf_tm_q_guaranteed_min_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       port handle.
 * @param queue      queue identifier. Valid range [ 0..31 ]
 * @param cells      Queue limits specified in cell count
 */
bf_status_t bf_tm_q_guaranteed_min_limit_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  // Read from SW only where the value should be already set.
  rc = bf_tm_q_get_min_limit(dev, q, cells, NULL);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/**
 * Get default minmum buffer limit for a queue.
 *
 * Related APIs: bf_tm_q_guaranteed_min_limit_get()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[out] cells  Default minimum buffer limit for a queue in cell count.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_guaranteed_min_limit_get_default(bf_dev_id_t dev,
                                                     uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(cells == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  /* TODO: extend TM API to keep defaults used at init time. */

  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);
  if (BF_SUCCESS == rc && def.q_gmin_limit_is_valid) {
    *cells = def.q_gmin_limit;  // TF1: BF_TM_Q_GMIN_LMT
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/**
 * Get queue tail drop enable mode.
 *
 * Related APIs: bf_tm_q_tail_drop_enable ()
 *               bf_tm_q_tail_drop_disable ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose tail drop mode has to be set.
 * @param[out] mode   Tail drop mode current setting.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_tail_drop_get(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bool *mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(mode == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  return bf_tm_q_get_tail_drop_en(dev, q, mode, mode);
}

/**
 * Get queue color drop enable mode.
 *
 * Related APIs: bf_tm_q_color_drop_enable ()
 *               bf_tm_q_color_drop_disable ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose color drop mode has to be set.
 * @param[out] mode   Color drop mode current setting.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_tail_drop_get_default(bf_dev_id_t dev, bool *mode) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(mode == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);
  if (BF_SUCCESS == rc && def.q_tail_drop_is_valid) {
    *mode = def.q_tail_drop;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/**
 * Get queue color drop enable mode.
 *
 * Related APIs: bf_tm_q_color_drop_enable ()
 *               bf_tm_q_color_drop_disable ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose color drop mode has to be set.
 * @param[out] mode   Color drop mode current setting.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_color_drop_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bool *mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(mode == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  return bf_tm_q_get_color_drop_en(dev, q, mode, mode);
}

/**
 * Get queue drop state.
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose color drop state has to be get.
 * @param[in]  color  Color
 * @param[out] state  Color drop state.
 * @return            Status of the API call.
 */
bf_status_t bf_tm_q_drop_state_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bf_tm_color_t color,
                                   bool *state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) {
    return (rc);
  }

  return bf_tm_q_get_egress_drop_state(dev, q, color, state);
}

/**
 * Get queue color drop default mode.
 *
 * Related APIs: bf_tm_q_color_drop_enable ()
 *               bf_tm_q_color_drop_disable ()
 *               bf_tm_q_color_drop_get ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[out] mode  Color drop mode default setting.
 * @return           Status of the API call.
 */
bf_status_t bf_tm_q_color_drop_get_default(bf_dev_id_t dev, bool *mode) {
  BF_TM_INVALID_ARG(mode == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  bf_status_t rc = BF_SUCCESS;
  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);
  if (BF_SUCCESS == rc && def.q_color_drop_mode_is_valid) {
    *mode = def.q_color_drop_mode;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/**
 * Get color drop limits for queue.
 *
 * Related APIs: bf_tm_q_color_limit_set ()
 *
 * @param dev        ASIC device identifier.
 * @param port       port handle.
 * @param queue      queue whose dynamic limits has to be adjusted.
 * @param color      Color (RED, YELLOW)
 * @param limit      Color Limit is specified in percentage of guaranteed queue
 *                   limit.
 *                   Green Color limit is equal to queue limit.
 *                   Yellow, Red limit obtained is percentage of overall
 *                   queue share limit. Once queue usage reaches the limit,
 *                   appropriate colored packets are tail dropped.
 *                   To get GREEN Color limit use
 *                   bf_tm_q_guaranteed_min_limit_get()
 * @return           Status of API call.
 */
bf_status_t bf_tm_q_color_limit_get(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bf_tm_color_t color,
                                    bf_tm_queue_color_limit_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;
  uint8_t lt = 0;

  BF_TM_INVALID_ARG(limit == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  switch (color) {
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_q_get_yel_limit_pcent(dev, q, &lt, &lt);
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_q_get_red_limit_pcent(dev, q, &lt, &lt);
      break;
    case BF_TM_COLOR_GREEN:
    default:
      break;
  }
  *limit = lt;
  return (rc);
}

/**
 * Get color drop limit defaults for queues.
 *
 * Related APIs: bf_tm_q_color_limit_get ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  color  Color (RED, YELLOW)
 * @param[out] limit  HW default color limit.
 * @return            Status of API call.
 */
bf_status_t bf_tm_q_color_limit_get_default(bf_dev_id_t dev,
                                            bf_tm_color_t color,
                                            bf_tm_queue_color_limit_t *limit) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((limit == NULL));

  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);

  if (BF_SUCCESS == rc) {
    switch (color) {
      case BF_TM_COLOR_YELLOW:
        if (def.q_color_yellow_limit_is_valid) {
          *limit = def.q_color_yellow_limit;
        } else {
          rc = BF_NOT_SUPPORTED;
        }
        break;
      case BF_TM_COLOR_RED:
        if (def.q_color_red_limit_is_valid) {
          *limit = def.q_color_red_limit;
        } else {
          rc = BF_NOT_SUPPORTED;
        }
        break;
      case BF_TM_COLOR_GREEN:
      default:
        rc = BF_INVALID_ARG;
        break;
    }
  }

  return (rc);
}

/**
 * Get drop state shadow for queue.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param queue      Queue whose shadow register has to be read.
 * @param state      32-bit state:
 *                   per queue 4 bits states (green, yellow, red, min)
 *                   [3:0] -- Qi in PIPE0,
 *                   [7:4] -- Qi in PIPE 1,
 *                   [11:8] -- Qi in PIPE2,
 *                   [15:12] -- Qi in PIPE3
 * @return           Status of API call.
 */
bf_status_t bf_tm_q_drop_state_shadow_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint32_t *state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(state == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return rc;
  rc = bf_tm_q_get_drop_state_shadow(dev, q, state);
  return (rc);
}

/**
 * Get color hysteresis for queue.
 *
 * Related APIs: bf_tm_q_color_hysteresis_set ()
 *
 * @param dev        ASIC device identifier.
 * @param port       port handle.
 * @param queue      queue whose dynamic limits has to be adjusted.
 * @param color      Color (RED, YELLOW, GREEN)
 * @param cells      Number of cells queue usage drops to
 *                   when drop condition is cleared.
 * @return           Status of API call.
 */
bf_status_t bf_tm_q_color_hysteresis_get(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_color_t color,
                                         bf_tm_thres_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(cells == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  switch (color) {
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_q_get_yel_hyst(dev, q, cells, cells);
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_q_get_red_hyst(dev, q, cells, cells);
      break;
    default:
      break;
  }
  return (rc);
}

/**
 * Get color hysteresis defaults for queues.
 *
 * Related APIs: bf_tm_q_color_hysteresis_get ()
 *
 * @param[in]  dev       ASIC device identifier.
 * @param[in]  color     Color (RED, YELLOW)
 * @param[out] cells     HW default color hysteresis.
 * @return               Status of API call.
 */
bf_status_t bf_tm_q_color_hysteresis_get_default(bf_dev_id_t dev,
                                                 bf_tm_color_t color,
                                                 bf_tm_thres_t *cells) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((cells == NULL));

  bf_tm_q_defaults_t def;
  rc = bf_tm_q_get_defaults(dev, &def);

  if (BF_SUCCESS == rc) {
    switch (color) {
      case BF_TM_COLOR_YELLOW:
        if (def.q_color_yellow_hysteresis_is_valid) {
          *cells = def.q_color_yellow_hysteresis;
        } else {
          rc = BF_NOT_SUPPORTED;
        }
        break;
      case BF_TM_COLOR_RED:
        if (def.q_color_red_hysteresis_is_valid) {
          *cells = def.q_color_red_hysteresis;
        } else {
          rc = BF_NOT_SUPPORTED;
        }
        break;
      case BF_TM_COLOR_GREEN:
      default:
        rc = BF_INVALID_ARG;
        break;
    }
  }
  return (rc);
}

/*
 * This API can be used to get cut through buffer size on per port basis.
 * The specified size is set aside for unicast traffic in cut through mode.
 *
 * Related APIs: bf_tm_port_uc_cut_through_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param size       Size in terms of cells (upto 16). Valid value [1..15]
 *                   If size is set to zero, then cut through get disabled.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_uc_cut_through_limit_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                uint8_t *size) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == size));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  bf_tm_thres_t sz = 0;
  rc = bf_tm_port_get_uc_cut_through_limit(dev, p, &sz, &sz);
  *size = sz;
  return (rc);
}

/*
 * Get total number of supported PPGs.
 *
 * Related APIs: bf_tm_ppg_unusedppg_get()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe identifier.
 * @param total_ppg  Pointer to unsigned integer location where total
 *                   supported PPG count will be stored.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_totalppg_get(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint32_t *total_ppg) {
  (void)pipe;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == total_ppg));

  *total_ppg = g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe;
  return (BF_SUCCESS);
}

/*
 * Get total number of unused PPGs.
 *
 * Related APIs: bf_tm_ppg_totalppg_get()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe identifier.
 * @param total_ppg  Pointer to unsigned integer location where
 *                    current unused PPG count will be stored.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_unusedppg_get(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint32_t *unused_ppg) {
  bf_tm_ppg_t *_ppg;
  int i, count = 0;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == unused_ppg));

  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe; i++) {
    _ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, i);
    BF_TM_PPG_CHECK(_ppg, i, pipe, dev);

    if (_ppg && !_ppg->in_use) {
      count++;
    }
  }
  *unused_ppg = count;

  return (BF_SUCCESS);
}

/**
 * @brief Default TM buffering settings for a PPG.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  ppg_hdl             ppg handle.
 * @param[out] min_limit_cells     The PPG default guaranteed min limit in
 *                                 cells.
 * @param[out] hysteresis_cells    The PPG default hysteresis limit in cells.
 * @return                         Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_buffer_get_default(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg_hdl,
                                         uint32_t *min_limit_cells,
                                         uint32_t *hysteresis_cells) {
  bf_status_t rc = BF_SUCCESS;
  bool is_dpg = false;
  int lport = 0;
  int ppg_n = 0;
  int pipe = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg_hdl, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((min_limit_cells == NULL));
  BF_TM_INVALID_ARG((hysteresis_cells == NULL));

  bf_tm_api_hlp_get_ppg_details(dev, ppg_hdl, &is_dpg, &ppg_n, &pipe, &lport);
  bf_tm_ppg_t *ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(ppg, ppg_n, pipe, dev);

  bf_tm_ppg_defaults_t def;
  rc = bf_tm_ppg_get_defaults(dev, ppg, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }

  if (def.min_limit_cells_is_valid && def.hysteresis_cells_is_valid) {
    *min_limit_cells = def.min_limit_cells;
    *hysteresis_cells = def.hysteresis_cells;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * @brief Default shared pool settings for a PPG.
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_get()
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[in]  ppg_hdl         ppg handle.
 * @param[out] pool            Default application pool to which the PPG
 *                             is assigned.
 * @param[out] pool_max_cells  Default limit to which the PPg can grow inside
 *                             an application pool.
 * @param[out] dynamic_baf     Default BAF setting for the PPG.
 * @return                     Status of API call.
 */
bf_status_t bf_tm_ppg_app_pool_usage_get_default(bf_dev_id_t dev,
                                                 bf_tm_ppg_hdl ppg_hdl,
                                                 bf_tm_app_pool_t *pool,
                                                 uint32_t *pool_max_cells,
                                                 bf_tm_ppg_baf_t *dynamic_baf) {
  bf_status_t rc = BF_SUCCESS;
  bool is_dpg = false;
  int lport = 0;
  int ppg_n = 0;
  int pipe = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg_hdl, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((pool == NULL));
  BF_TM_INVALID_ARG((pool_max_cells == NULL));
  BF_TM_INVALID_ARG((dynamic_baf == NULL));

  bf_tm_api_hlp_get_ppg_details(dev, ppg_hdl, &is_dpg, &ppg_n, &pipe, &lport);
  bf_tm_ppg_t *ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(ppg, ppg_n, pipe, dev);

  bf_tm_ppg_defaults_t def;
  rc = bf_tm_ppg_get_defaults(dev, ppg, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.pool_is_valid && def.pool_max_cells_is_valid &&
      def.dynamic_baf_is_valid) {
    *pool = def.pool;
    *pool_max_cells = def.pool_max_cells;
    *dynamic_baf = def.dynamic_baf;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/**
 * @brief Get default iCoS DPG map for the Port.
 *
 * Related APIs: bf_tm_ppg_defaultppg_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port_id     Device port identifier (pipe id and port number).
 * @param[out] icos_mask  Default iCoS bitmask with DPG assignments: 1 is set
 *                        if the iCoS is attached to the Port's DPG.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 *
 */
bf_status_t bf_tm_port_icos_map_get_default(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint8_t *icos_mask) {
  (void)port;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == icos_mask));
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_defaults_t def;
  rc = bf_tm_port_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc && def.port_icos_mask_is_valid) {
    *icos_mask = def.port_icos_mask;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/**
 * @brief Get PPG identifiers for a Port iCoS map.
 *
 * Related APIs: bf_tm_ppg_icos_mapping_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port_id     Device port identifier (pipe id and port number).
 * @param[out] ppg_mask   iCoS bitmask of PPG assignments: 1 is set if the iCoS
 *                        is detached from Port's DPG and attached to a PPG.
 * @param[out] ppg_hdlrs  Map of the port's iCoS to PPG handlers.
 *                        Index is iCoS (0..BF_TM_MAX_PFC_LEVELS-1).
 *                        Each item is a PPG hahdler assigned to the iCoS
 *                        or zero if not. In the latter case the appropriate
 *                        ppg_mask bit is also zero. Items are not unique as
 *                        several iCoS might be assigned to the same PPG.
 * @return                Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_port_ppg_map_get(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   uint8_t *ppg_mask,
                                   bf_tm_ppg_hdl *ppg_hdlrs) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p_dscr = NULL;
  bf_tm_ppg_hdl dpg_hndl = 0;
  uint8_t res_mask = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(NULL == ppg_mask);
  BF_TM_INVALID_ARG(NULL == ppg_hdlrs);

  rc = bf_tm_port_get_descriptor(dev, port_id, &p_dscr);
  if (rc != BF_SUCCESS) return (rc);

  rc = bf_tm_ppg_defaultppg_get(dev, port_id, &dpg_hndl);
  if (BF_SUCCESS != rc) return (rc);

  rc = bf_tm_ppg_icos_mapping_get(dev, dpg_hndl, &res_mask);
  if (BF_SUCCESS != rc) return (rc);

  res_mask = ~res_mask;  // from DPG iCoS inverse to PPG mapping.

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  uint8_t w_mask = 0x01;

  for (uint8_t i = 0; w_mask && i < BF_TM_MAX_PFC_LEVELS; i++, w_mask <<= 1) {
    ppg_hdlrs[i] = 0;  // clear with 0
    if (w_mask & res_mask) {
      // Check the PPG attached to this iCoS
      bf_tm_ppg_t *ppg_d = p_dscr->ppgs[i];
      if (NULL != ppg_d && (ppg_d->in_use) &&
          ((MAKE_DEV_PORT(ppg_d->l_pipe, ppg_d->uport)) == port_id)) {
        ppg_hdlrs[i] =
            BF_TM_PPG_HANDLE(ppg_d->l_pipe, ppg_d->uport, ppg_d->ppg);
      } else {
        // iCoS DPG mask shouldn't have zero bit at detached PPG or none.
        LOG_DBG("%s:%d port %d DPG mask 0x%x iCoS:%d with detached PPG",
                __func__,
                __LINE__,
                port_id,
                ~(res_mask),
                i);
      }
    } else {
      ppg_hdlrs[i] = dpg_hndl;
    }
  }  // for all iCoS

  TM_UNLOCK_AND_FLUSH(dev);

  *ppg_mask = res_mask;

  return (BF_SUCCESS);
}

/**
 * @brief Get device port from PPG handle.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] ppg_hdl   PPG handle used for TM API calls.
 * @param[out] port_id  Device port identifier (pipe id and port number)
 *                      which was used to allocate the PPG handle.
 *                      It's current allocation status might be obsolete.
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_port_get(bf_dev_id_t dev,
                               bf_tm_ppg_hdl ppg_hdl,
                               bf_dev_port_t *port_id) {
  bool def_ppg = false;
  int lport = 0;
  int ppg_n = 0;
  int pipe = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg_hdl, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == port_id));

  bf_tm_api_hlp_get_ppg_details(dev, ppg_hdl, &def_ppg, &ppg_n, &pipe, &lport);

  *port_id = MAKE_DEV_PORT(pipe, lport);

  return (BF_SUCCESS);
}

/**
 * @brief Get ppg number from PPG handle.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] ppg_hdl   PPG handle used for TM API calls.
 * @param[out] ppg_nr   PPG number
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_nr_get(bf_dev_id_t dev,
                             bf_tm_ppg_hdl ppg_hdl,
                             bf_tm_ppg_id_t *ppg_nr) {
  bool def_ppg = false;
  int lport = 0;
  int ppg_n = 0;
  int pipe = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg_hdl, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == ppg_nr));

  bf_tm_api_hlp_get_ppg_details(dev, ppg_hdl, &def_ppg, &ppg_n, &pipe, &lport);

  *ppg_nr = (uint8_t)ppg_n;

  return (BF_SUCCESS);
}

/**
 * @brief Get TM Mirror Port default PG's handle.
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] pipe_id   Pipe identifier.
 * @param[out] ppg_hdl  Default PG handle of the Mirror port.
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_mirror_port_handle_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe_id,
                                             bf_tm_ppg_hdl *ppg_hdl) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe_id, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == ppg_hdl));

  int l_port = g_tm_ctx[dev]->tm_cfg.mirror_port_start;
  int dpg_mirror = (g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe) + l_port;

  bf_dev_port_t u_port = DEV_PORT_TO_LOCAL_PORT(
      lld_sku_map_devport_from_device_to_user(dev, l_port));

  *ppg_hdl = BF_TM_PPG_HANDLE(pipe_id, u_port, dpg_mirror);

  return (BF_SUCCESS);
}

/*
 * A non deafult PPG can be optionally assigned to any application pool.
 * Get the application pool id of the ppg
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_set()
 *
 * @param dev             ASIC device identifier.
 * @param ppg             ppg handle.
 * @param pool            Application pool to which PPG is assigned to.
 * @return status of API call
 */
bf_status_t bf_tm_ppg_app_pool_id_get(bf_dev_id_t dev,
                                      bf_tm_ppg_hdl ppg,
                                      uint32_t *pool) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_app_poolid(dev, _ppg, pool, pool);
  TM_UNLOCK_AND_FLUSH(dev);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/**
 * @brief Get Fast recovery mode for PPG
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] ppg             ppg handle.
 * @param[out] fast_recover   Fast recovery mode
 * @return                    status of API call
 */
bf_status_t bf_tm_ppg_fast_recovery_mode_get(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bool *fast_recover) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(fast_recover == NULL);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_fast_recovery_mode(dev, _ppg, fast_recover, fast_recover);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Per ppg skid usage counter
 *
 * Related APIs: bf_tm_ppg_get_skid_usage_counter()
 *
 * @param dev             ASIC device identifier.
 * @param ppg             ppg handle.
 * @param counter         Counter value.
 * @return status of API call
 */
bf_status_t bf_tm_ppg_skid_usage_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     uint32_t *counter) {
  bf_status_t rc = BF_SUCCESS;
  int ppg_n, pipe;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  bf_tm_api_hlp_get_ppg_details(dev, ppg, NULL, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_skid_usage_counter(dev, _ppg, counter);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * A non deafult PPG can be optionally assigned to any application pool.
 * When assigned to application pool, get static or dynamic shared limit
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_set()
 *
 * @param dev             ASIC device identifier.
 * @param ppg             ppg handle.
 * @param pool            Application pool to which PPG is assigned to.
 * @param base_use_limit  Limit to which PPG can grow inside application
 *                        pool. Once this limit is crossed, if PPG burst
 *                        absroption factor (BAF) is non zero, depending
 *                        availability of buffer, PPG is allowed to
 *                        use buffer upto BAF limit. If BAF limit is zero,
 *                        PPG is treated as static and no dynamic thresholding.
 * @param dynamic_baf     One of the values listed in bf_tm_ppg_baf_t
 *                        When BF_TM_PPG_BAF_DISABLE is used, PPG uses
 *                        static limit.
 * @param hysteresis      Hysteresis value.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_app_pool_usage_get(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg,
                                         bf_tm_app_pool_t pool,
                                         uint32_t *base_use_limit,
                                         bf_tm_ppg_baf_t *dynamic_baf,
                                         uint32_t *hysteresis) {
  (void)pool;
  bf_status_t rc = BF_SUCCESS;
  bool defppg, is_dyn = false;
  int ppg_n, pipe;
  uint8_t baf;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == base_use_limit));
  BF_TM_INVALID_ARG((NULL == dynamic_baf));
  BF_TM_INVALID_ARG((NULL == hysteresis));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_app_limit(dev, _ppg, base_use_limit, base_use_limit);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  rc = bf_tm_ppg_get_is_dynamic(dev, _ppg, &is_dyn, &is_dyn);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
    return (rc);
  }
  if (is_dyn) {
    rc = bf_tm_ppg_get_baf(dev, _ppg, &baf, &baf);
    if (BF_SUCCESS != rc) {
      LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
      return (rc);
    }
    *dynamic_baf = baf;
  } else {
    *dynamic_baf = BF_TM_PPG_BAF_DISABLE;
  }
  rc = bf_tm_ppg_get_ppg_hyst(dev, _ppg, hysteresis, hysteresis);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d rc=%d(%s)", __func__, __LINE__, rc, bf_err_str(rc));
  }
  return (rc);
}

/*
 * Get PPG miminum limits. Returned limits are accounted in terms of cells.
 *
 * Related APIs: bf_tm_ppg_guaranteed_min_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose limits has to be adjusted.
 * @param cells      Number of cells set as minimum limit
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_guaranteed_min_limit_get(bf_dev_id_t dev,
                                               bf_tm_ppg_hdl ppg,
                                               uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == cells));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_min_limit(dev, _ppg, cells, cells);
  return (rc);
}

/*
 * Get PFC(Lossless) enable flag value
 *
 * Related APIs: bf_tm_ppg_lossless_treatment_enable()
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose lossless flag has to be fetched.
 * @param pfc        value of the pfc flag
 * @return status of API call;
 */
bf_status_t bf_tm_ppg_lossless_treatment_get(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bool *pfc_flag) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == pfc_flag));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_pfc_treatment(dev, _ppg, pfc_flag, pfc_flag);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/*
 * Get Port PFC(Lossless) state
 *
 * Related APIs: bf_tm_port_pfc_state_set(), bf_tm_port_clear_pfc_state()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port whose lossless bitmask has to be fetched.
 * @param state      value of the pfc reg
 * @return status of API call;
 */
bf_status_t bf_tm_port_pfc_state_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint8_t *state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == state));
  bf_tm_port_t *p_dscr;

  rc = bf_tm_port_get_descriptor(dev, port, &p_dscr);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_pfc_state(dev, p_dscr, state);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/*
 * Get Port PFC(Lossless) CoS enabled status
 *
 * @param dev        ASIC device identifier.
 * @param port       Port whose lossless enable bit has to be fetched.
 * @param cos        CoS (0-7)
 * @param enabled    CoS enabled
 * @return status of API call;
 */
bf_status_t bf_tm_port_pfc_enable_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint8_t cos,
                                      bool *enabled) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == enabled));
  BF_TM_INVALID_ARG((cos >= BF_TM_MAX_PFC_LEVELS));
  bf_tm_port_t *p_dscr;

  rc = bf_tm_port_get_descriptor(dev, port, &p_dscr);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_pfc_enabled_cos(dev, p_dscr, cos, enabled, enabled);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/*
 * GGet Port PFC(Lossless) state (TOF2, TOF3) including final and
 * remote/internal pfc status)
 *
 * Related APIs: bf_tm_port_clear_pfc_state()
 *
 * @param dev              ASIC device identifier.
 * @param port             Port whose PFC extended state has to be get.
 * @param port_ppg_state   Port PPG PFC state bitmap for each of 8 iCoS
 * @param rm_pfc_state     Remote PFC state bitmap for each of 8 iCoS
 * @param mac_pfc_out      MAC PFC state bitmap for each of 8 iCoS
 * @param mac_pause_out    MAC pause state
 *
 * @return status of API call;
 */
bf_status_t bf_tm_port_pfc_state_get_ext(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint8_t *port_ppg_state,
                                         uint8_t *rm_pfc_state,
                                         uint8_t *mac_pfc_out,
                                         bool *mac_pause_out) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  bf_tm_port_t *p_dscr;

  rc = bf_tm_port_get_descriptor(dev, port, &p_dscr);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_pfc_state_ext(
      dev, p_dscr, port_ppg_state, rm_pfc_state, mac_pfc_out, mac_pause_out);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/**
 * @brief Get ppg allocation config.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] ppg           ppg whose icos mask has to be fetched.
 * @param[out] ppgid        ppg id
 * @param[out] poolid       Application Pool id
 * @param[out] default_ppg  Is ppg default
 * @return                  Status of API call.
 */
bf_status_t bf_tm_ppg_allocation_cfg_get(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg,
                                         uint32_t *ppgid,
                                         uint32_t *poolid,
                                         bool *default_ppg) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == ppgid));
  BF_TM_INVALID_ARG((NULL == poolid));
  BF_TM_INVALID_ARG((NULL == default_ppg));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_allocation(dev, _ppg);
  *ppgid = _ppg->ppg;
  *poolid = _ppg->ppg_cfg.app_poolid;
  *default_ppg = _ppg->is_default_ppg;
  return (rc);
}

/*
 * Get ppg skid limits.
 *
 * Related APIs: bf_tm_ppg_skid_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose skid limits has to be fetched.
 * @param cells      Limits in terms of number of cells
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_skid_limit_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == cells));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_skid_limit(dev, _ppg, cells, cells);

  return (rc);
}

/*
 * Get ppg resume limit.
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose skid limits has to be fetched.
 * @param limit      Limits in terms of number of cells
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_resume_limit_get(bf_dev_id_t dev,
                                       bf_tm_ppg_hdl ppg,
                                       uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == limit));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_resume_limit(dev, _ppg, limit, limit);
  return (rc);
}

/*
 * Get ppg drop state
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose drop state has to be fetched.
 * @param state      State
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_drop_state_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     bool *state) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == state));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_drop_state(dev, _ppg, state, state);

  return (rc);
}

/*
 * Get ppg hysteresis limits. Same hysteresis limits are applied to
 * PPGs limits inside MIN pool and PPGs mapped to Skid Pool.
 * Hysterisis limits are numbers of cells the ppg usage should fall by
 * from its limit value. Once usage limits are below hysteresis, appropriate
 * condition is cleared. Example when PPG's skid usage limit falls
 * below its allowed limits limit by hysteresis value, drop condition is
 * cleared.
 *
 * Related APIs: bf_tm_ppg_skid_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg whose hysteresis limits has to be fetched.
 * @param cells      Limits in terms of number of cells
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_guaranteed_min_skid_hysteresis_get(bf_dev_id_t dev,
                                                         bf_tm_ppg_hdl ppg,
                                                         uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == cells));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_ppg_hyst(dev, _ppg, cells, cells);
  return (rc);
}

/**
 * Get all iCoS (iCoS = ig_intr_md.ingress_cos) traffic is attached to PPG
 *
 *
 * Related APIs: bf_tm_ppg_icos_mapping_set()
 *
 * @param dev        ASIC device identifier.
 * @param ppg        ppg handle
 * @param icos_bmap  Bit map of iCoS (iCoS = ig_intr_md.ingress_cos).
 *                   Bit 7 is interpreted as iCoS 7 that is attached.
 *                   ppg handle.
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_ppg_icos_mapping_get(bf_dev_id_t dev,
                                       bf_tm_ppg_hdl ppg,
                                       uint8_t *icos_bmap) {
  bool defppg;
  int ppg_n, pipe;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PPG_INVALID(ppg, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == icos_bmap));

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);
  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  *icos_bmap = _ppg->ppg_cfg.icos_mask;

  return (BF_SUCCESS);
}

/*
 * Get application pool size. Size in units of cell set aside for
 * application pool.
 *
 * Related APIs: bf_tm_pool_size_set()
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool identifier.
 * @param cells           Size of pool in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_size_get(bf_dev_id_t dev,
                                bf_tm_app_pool_t pool,
                                uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  if (dir == BF_TM_DIR_INGRESS) {
    rc = bf_tm_ig_spool_get_green_limit(
        dev, id, g_tm_ctx[dev]->ig_pool, cells, cells);
  } else {
    rc = bf_tm_eg_spool_get_green_limit(
        dev, id, g_tm_ctx[dev]->eg_pool, cells, cells);
  }
  return (rc);
}

/*
 * Get Application pool color drop limits.
 *
 * Related APIs: bf_tm_pool_color_drop_limit_set()
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool handle.
 * @param color           Color (Green, Yellow, Red)
 * @param limit           Limits in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_limit_get(bf_dev_id_t dev,
                                            bf_tm_app_pool_t pool,
                                            bf_tm_color_t color,
                                            uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_get_green_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit, limit)
               : bf_tm_eg_spool_get_green_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
    case BF_TM_COLOR_YELLOW:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_get_yel_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit, limit)
               : bf_tm_eg_spool_get_yel_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
    case BF_TM_COLOR_RED:
      rc = (dir == BF_TM_DIR_INGRESS)
               ? bf_tm_ig_spool_get_red_limit(
                     dev, id, g_tm_ctx[dev]->ig_pool, limit, limit)
               : bf_tm_eg_spool_get_red_limit(
                     dev, id, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
  }
  return (rc);
}
/*
 * Get color drop state for application pool
 *
 * @param dev         ASIC device identifier.
 * @param entry       Color.
 * @param state       4-bit drop state for 4 pools
 * @return            Status of API call.
 */
bf_status_t bf_tm_pools_color_drop_state_get(bf_dev_id_t dev,
                                             bf_tm_color_t color,
                                             uint32_t *state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_ig_spool_color_drop_state_get(dev, color, state);
  return (rc);
}

/*
 * Clear color drop state for application pool
 *
 * @param dev         ASIC device identifier.
 * @param entry       Color.
 * @return            Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_state_clear(bf_dev_id_t dev,
                                              bf_tm_color_t color) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_ig_spool_color_drop_state_clear(dev, color);
  return (rc);
}

/*
 * Get Color drop hysteresis. The same hysteresis value is applied on
 * all application pools. Resume condition is triggered when pool usage drops
 * by hysteresis value.
 *
 * Related APIs: bf_tm_pool_color_drop_hysteresis_set()
 *
 * @param dev             ASIC device identifier.
 * @param color           Color (Green, Yellow, Red)
 * @param limit           Limits in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_hysteresis_get(bf_dev_id_t dev,
                                                 bf_tm_color_t color,
                                                 uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  switch (color) {
    case BF_TM_COLOR_GREEN:
      rc = bf_tm_ig_spool_get_green_hyst(
          dev, g_tm_ctx[dev]->ig_pool, limit, limit);
      rc |= bf_tm_eg_spool_get_green_hyst(
          dev, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
    case BF_TM_COLOR_YELLOW:
      rc = bf_tm_ig_spool_get_yel_hyst(
          dev, g_tm_ctx[dev]->ig_pool, limit, limit);
      rc |= bf_tm_eg_spool_get_yel_hyst(
          dev, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
    case BF_TM_COLOR_RED:
      rc = bf_tm_ig_spool_get_red_hyst(
          dev, g_tm_ctx[dev]->ig_pool, limit, limit);
      rc |= bf_tm_eg_spool_get_red_hyst(
          dev, g_tm_ctx[dev]->eg_pool, limit, limit);
      break;
  }
  return (rc);
}

/*
 * Get default color drop hysteresis for all the application pools.
 *
 * Related APIs: bf_tm_pool_color_drop_hysteresis_set()
 *
 * @param dev             ASIC device identifier.
 * @param color           Color (Green, Yellow, Red)
 * @param limit           Limits in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_hysteresis_get_default(bf_dev_id_t dev,
                                                         bf_tm_color_t color,
                                                         uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(limit == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  (void)color;
  bf_tm_pool_defaults_t def;
  rc = bf_tm_pool_defaults_get(dev, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.color_drop_hysteresis_is_valid) {
    *limit = def.color_drop_hysteresis;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get per PFC level limit values. PFC level limits are configurable on per
 * application pool basis. When usage numbers hit pfc limits, PAUSE is triggered
 * for lossless traffic or PFC enabled traffc.
 *
 * Related APIs: bf_tm_pool_pfc_limit_set()
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool handle for which limits are configured.
 * @param icos            Internal CoS (iCoS = ig_intr_md.ingress_cos) level
 *                        on which limits are applied.
 * @param limit           Limit value in terms of cell count.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_pfc_limit_get(bf_dev_id_t dev,
                                     bf_tm_app_pool_t pool,
                                     bf_tm_icos_t icos,
                                     uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  int dir;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(TM_IS_ICOS_INVALID(icos));

  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));
  if (dir == BF_TM_DIR_EGRESS) {
    return (BF_INVALID_ARG);
  }

  rc = bf_tm_ig_spool_get_pfc_limit(
      dev, id, icos, g_tm_ctx[dev]->ig_pool, limit, limit);
  return (rc);
}

/*
 * Get per PFC level default limit values.
 *
 * Related APIs: bf_tm_pool_pfc_limit_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            pool handle for which limits are configured.
 * @param[in] icos            Internal CoS (iCoS = ig_intr_md.ingress_cos) level
 *                            on which limits are applied.
 * @param[out] limit          Limit value in terms of cell count.
 * @return                    Status of API call.
 */
bf_status_t bf_tm_pool_pfc_limit_get_default(bf_dev_id_t dev,
                                             bf_tm_app_pool_t pool,
                                             bf_tm_icos_t icos,
                                             uint32_t *limit) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_ICOS_INVALID(icos));
  BF_TM_INVALID_ARG(limit == NULL);

  int dir;
  int id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(id, g_tm_ctx[dev]));

  bf_tm_pool_defaults_t def;
  rc = bf_tm_pool_defaults_get(dev, &pool, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.pfc_limit_is_valid) {
    *limit = def.pfc_limit;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get Application pool's color drop policy. Default policy is to
 * trigger drops based on color.
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool identifier.
 * @param drop_state      Drop or no drop condition
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_color_drop_state_get(bf_dev_id_t dev,
                                            bf_tm_app_pool_t pool,
                                            bool *drop_state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  int dir;
  int pool_id = bf_tm_api_hlp_get_pool_details(pool, &dir);
  BF_TM_INVALID_ARG(TM_IS_POOL_INVALID(pool_id, g_tm_ctx[dev]));
  if (drop_state != NULL) {
    if (dir == BF_TM_DIR_INGRESS) {
      rc = bf_tm_ig_spool_get_color_drop(
          dev, pool_id, g_tm_ctx[dev]->ig_pool, drop_state, drop_state);
    } else if (dir == BF_TM_DIR_EGRESS) {
      rc = bf_tm_eg_spool_get_color_drop(
          dev, pool_id, g_tm_ctx[dev]->eg_pool, drop_state, drop_state);
    } else {
      rc = (BF_UNEXPECTED);
    }
  }
  return (rc);
}

/*
 * Get Application pool's drop state egress
 *
 * @param dev             ASIC device identifier.
 * @param drop_type       Register entry [0..7]:
 *                        0 - Per Egress PIPE Buffer drop state (4 PIPEs)
 *                        1 - Global Buffer AP Green drop state (4 Pools)
 *                        2 - Global Buffer AP Yellow drop state(4 Pools)
 *                        3 - Global Buffer AP Red drop state (4 Pools)
 *                        4 - PIPE0 PRE FIFO drop state (4 FIFOs)
 *                        5 - PIPE1 PRE FIFO drop state (4 FIFOs)
 *                        6 - PIPE2 PRE FIFO drop state (4 FIFOs)
 *                        7 - PIPE3 PRE FIFO drop state (4 FIFOs)
 *                        Next entries will be available for TOF3 only:
 *                        8 - PIPE4 PRE FIFO drop state (4 FIFOs)
 *                        9 - PIPE5 PRE FIFO drop state (4 FIFOs)
 *                        10 - PIPE6 PRE FIFO drop state (4 FIFOs)
 *                        11 - PIPE7 PRE FIFO drop state (4 FIFOs)
 * @param state           drop state value
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_egress_buffer_drop_state_get(
    bf_dev_id_t dev, bf_tm_eg_buffer_drop_state_en drop_type, uint32_t *state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_eg_spool_get_buffer_drop_state(dev, drop_type, state);
  return (rc);
}

/*
 * Clear Application pool's drop state egress
 *
 * @param dev             ASIC device identifier.
 * @param drop_type       Register entry [0..7]:
 *                        0 - Per Egress PIPE Buffer drop state (4 PIPEs)
 *                        1 - Global Buffer AP Green drop state (4 Pools)
 *                        2 - Global Buffer AP Yellow drop state(4 Pools)
 *                        3 - Global Buffer AP Red drop state (4 Pools)
 *                        4 - PIPE0 PRE FIFO drop state (4 FIFOs)
 *                        5 - PIPE1 PRE FIFO drop state (4 FIFOs)
 *                        6 - PIPE2 PRE FIFO drop state (4 FIFOs)
 *                        7 - PIPE3 PRE FIFO drop state (4 FIFOs)
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_egress_buffer_drop_state_clear(
    bf_dev_id_t dev, bf_tm_eg_buffer_drop_state_en drop_type) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_eg_spool_clear_buffer_drop_state(dev, drop_type);
  return (rc);
}

/*
 * Get skid pool size.
 *
 * Related APIs: bf_tm_pool_skid_size_set()
 *
 * @param dev             ASIC device identifier.
 * @param cells           Size of pool in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_skid_size_get(bf_dev_id_t dev, uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  rc = bf_tm_ig_gpool_get_skid_limit(dev, g_tm_ctx[dev]->ig_pool, cells, cells);
  return (rc);
}

/*
 * Get global skid pool hysteresis.
 *
 * Related APIs: bf_tm_pool_skid_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param cells      Number of cells set as skid pool hysteresis.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_skid_hysteresis_get(bf_dev_id_t dev, uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  rc = bf_tm_ig_gpool_get_skid_hyst(dev, g_tm_ctx[dev]->ig_pool, cells, cells);
  return (rc);
}

/*
 * Get default global skid pool hysteresis.
 *
 * Related APIs: bf_tm_pool_skid_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param cells      Number of cells set as skid pool hysteresis.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_skid_hysteresis_get_default(bf_dev_id_t dev,
                                                   uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cells));

  bf_tm_pool_defaults_t def;
  rc = bf_tm_pool_defaults_get(dev, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.skid_hysteresis_is_valid) {
    *cells = def.skid_hysteresis;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get negative mirror pool limit. Returned limit are accounted
 * in terms of cells.
 *
 * Related APIs: bf_tm_pool_mirror_on_drop_size_set()
 *
 * @param dev        ASIC device identifier.
 * @param cells      Size of pool.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_mirror_on_drop_size_get(bf_dev_id_t dev,
                                               uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  // Same size is set on both ingress and egress TM.
  // Add another API to independently set .
  rc = bf_tm_ig_gpool_get_dod_limit(dev, g_tm_ctx[dev]->ig_pool, cells, cells);
  rc |= bf_tm_eg_gpool_get_dod_limit(dev, g_tm_ctx[dev]->eg_pool, cells, cells);
  return (rc);
}

/*
 * Get per pipe per FIFO limit. Returned limit are accounted
 * in terms of cells.
 *
 * Related APIs: bf_tm_pre_fifo_limit_set()
 *
 * @param[in]   dev     ASIC device identifier.
 * @param[in]   pipe    Pipe identifier.
 * @param[in]   fifo    FIFO identifier.
 * @param[out]  cells   Size of pool.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pre_fifo_limit_get(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     uint8_t fifo,
                                     uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  rc = bf_tm_eg_gpool_get_fifo_limit(
      dev, g_tm_ctx[dev]->eg_pool, pipe, fifo, cells, cells);
  return (rc);
}

/**
 * Get global min limit. Returned limit are accounted
 * in terms of cells.
 *
 * Default : 0x6000
 *
 *
 * param dev        ASIC device identifier.
 * param cells      New size of pool.
 * return           Status of API call.
 */
bf_status_t bf_tm_global_min_limit_get(bf_dev_id_t dev, uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_ig_gpool_get_glb_min_limit(
      dev, g_tm_ctx[dev]->ig_pool, cells, cells);
  return (rc);
}

/*
 * Get cut through pool size for unicast traffic.
 *
 * Related APIs:  bf_tm_uc_cut_through_size_set()
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool identifier.
 * @param cells           Size of pool in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_uc_cut_through_size_get(bf_dev_id_t dev,
                                               uint32_t *cells) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  bf_tm_uc_ct_size_get(dev, cells, cells);
  return (BF_SUCCESS);
}

/*
 * Get cut through pool size for Multicast traffic. This size determines total
 * buffer set aside for multicast cut through traffic.
 *
 * Related APIs:  bf_tm_mc_cut_through_size_set()
 *
 * @param dev             ASIC device identifier.
 * @param pool            pool identifier.
 * @param cells           Size of pool in terms of cells.
 * @return                Status of API call.
 */
bf_status_t bf_tm_pool_mc_cut_through_size_get(bf_dev_id_t dev,
                                               uint32_t *cells) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  bf_tm_mc_ct_size_get(dev, cells, cells);
  return (BF_SUCCESS);
}

/*
 * Get port limit. When buffer usage accounted on port basis
 * crosses the limit, traffic is not admitted into traffic manager.
 *
 * Related APIs: bf_tm_port_ingress_drop_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Limit in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_limit_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == cells));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_wac_drop_limit(dev, p, cells, cells);
  return (rc);
}

/*
 * Get egress port limit. When buffer usage accounted on port basis
 * crosses the limit, traffic Will be dropped on QAC stage.
 *
 * Related APIs: bf_tm_port_egress_drop_limit_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Limit in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_limit_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cells));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_qac_drop_limit(dev, p, cells, cells);
  return (rc);
}

/*
 * Get port drop state in WAC. (TOF1 only)
 *
 * Related APIs: bf_tm_port_get_wac_drop_state()
 *
 * @param dev                ASIC device identifier.
 * @param port               Port Identifier
 * @param state              Drop state
 * @return                   Status of API call.
 */
bf_status_t bf_tm_port_wac_drop_state_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == state));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_wac_drop_state(dev, p, state);
  return (rc);
}

/*
 * Get port drop state in WAC. (TOF2, TOF3 ...)
 *
 * Related APIs: bf_tm_port_get_wac_drop_state_ext()
 *
 * @param dev                ASIC device identifier.
 * @param port               Port Identifier
 * @param shr_lmt_state      Drop state caused by shared limit.
 * @param hdr_lmt_state      Drop state caused by headroom limit.
 * @return                   Status of API call.
 */
bf_status_t bf_tm_port_wac_drop_state_get_ext(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bool *shr_lmt_state,
                                              bool *hdr_lmt_state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == shr_lmt_state));
  BF_TM_INVALID_ARG((NULL == hdr_lmt_state));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_wac_drop_state_ext(dev, p, shr_lmt_state, hdr_lmt_state);
  return (rc);
}

/*
 * Get port drop state in QAC.
 *
 * Related APIs: bf_tm_port_get_qac_drop_state()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param state      Drop state bitmask.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_qac_drop_state_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == state));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_qac_drop_state(dev, p, state);
  return (rc);
}

/*
 * Get port hysteresis limits.
 * When usage of cells goes below hysteresis value  port pause or drop
 * condition  will be cleared.
 *
 * Related APIs: bf_tm_port_ingress_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Offset Limit
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_ingress_hysteresis_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == cells));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_wac_hyst(dev, p, cells, cells);
  return (rc);
}

/*
 * Get egress port hysteresis limits.
 * When usage of cells goes below hysteresis value  port drop
 * condition  will be cleared.
 *
 * Related APIs: bf_tm_port_egress_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Offset Limit
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_egress_hysteresis_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == cells));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_qac_hyst(dev, p, cells, cells);
  return (rc);
}

/*
 * Get port skid limit.
 *
 * Related APIs: bf_tm_port_skid_limit_set()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port Identifier
 * @param[out] cells  Limit in terms of number of cells.
 * @return            Status of API call.
 */
bf_status_t bf_tm_port_skid_limit_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == cells));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_skid_limit(dev, p, cells, cells);
  return (rc);
}

/*
 * Default TM buffering settings for a port.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  port                Port Identifier
 * @param[out] ct_enable           The port default cut-through mode.
 * @param[out] uc_ct_limit_cells   The port default cut-through buffer size.
 * @param[out] ig_limit_cells      The port default ingress limit in cells.
 * @param[out] ig_hysteresis_cells The port default ingress hysteresis limit in
 *                                 cells.
 * @param[out] eg_limit_cells      The port default egress limit in cells.
 * @param[out] eg_hysteresis_cells The port default egress hysteresis limit in
 *                                 cells.
 * @param[out] skid_limit_cells    The port default skid limit in cells.
 * @return                         Status of API call.
 */
bf_status_t bf_tm_port_buffer_get_default(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *ct_enable,
                                          uint8_t *uc_ct_limit_cells,
                                          uint32_t *ig_limit_cells,
                                          uint32_t *ig_hysteresis_cells,
                                          uint32_t *eg_limit_cells,
                                          uint32_t *eg_hysteresis_cells,
                                          uint32_t *skid_limit_cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  bf_tm_port_defaults_t def;
  rc = bf_tm_port_get_defaults(dev, p, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }

  if (ct_enable) {
    if (def.port_ct_enable_is_valid) {
      *ct_enable = def.port_ct_enable;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (uc_ct_limit_cells) {
    if (def.port_uc_ct_limit_is_valid) {
      *uc_ct_limit_cells = def.port_uc_ct_limit;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (skid_limit_cells) {
    if (def.port_skid_limit_is_valid) {
      *skid_limit_cells = def.port_skid_limit;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (ig_limit_cells) {
    if (def.port_ig_limit_is_valid) {
      *ig_limit_cells = def.port_ig_limit;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (ig_hysteresis_cells) {
    if (def.port_ig_hysteresis_is_valid) {
      *ig_hysteresis_cells = def.port_ig_hysteresis;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (eg_limit_cells) {
    if (def.port_eg_limit_is_valid) {
      *eg_limit_cells = def.port_eg_limit;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  if (eg_hysteresis_cells) {
    if (def.port_eg_hysteresis_is_valid) {
      *eg_hysteresis_cells = def.port_eg_hysteresis;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }

  return (rc);
}

/*
 * Default flow control settings for a port.
 *
 * Related APIs: bf_tm_port_flowcontrol_mode_get()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  port     Port Identifier
 * @param[out] mode_tx  Default flow control mode on the port.
 * @param[out] mode_rx  Default react on flow control frame
 *                      received at the port.
 * @param[out] cos_map  Default CoS to iCoS map.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_flowcontrol_get_default(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_flow_ctrl_type_t *mode_tx,
                                               bf_tm_flow_ctrl_type_t *mode_rx,
                                               uint8_t *cos_map) {
  bf_status_t rc = BF_SUCCESS;

  (void)port;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == mode_tx));
  BF_TM_INVALID_ARG((NULL == mode_rx));
  BF_TM_INVALID_ARG((NULL == cos_map));

  for (int i = 0; i < BF_TM_MAX_COS_LEVELS; i++) {
    cos_map[i] = i;
  }

  bf_tm_port_defaults_t def;
  rc = bf_tm_port_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc && def.port_mode_tx_is_valid &&
      def.port_mode_rx_is_valid) {
    *mode_tx = def.port_mode_tx;
    *mode_rx = def.port_mode_rx;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * This API can be used to get pause type set on port.
 *
 * Related APIs: bf_tm_port_flowcontrol_mode_set()
 *
 * Default : No Pause or flow control.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param type       Pause type.
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_port_flowcontrol_mode_get(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_flow_ctrl_type_t *type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == type));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  return (bf_tm_port_get_flowcontrol_mode(dev, p, type, type));
}

/*
 * This API can be used to get pause type set on port to react when
 * port sees pause or pfc from peer.
 *
 * Related APIs: bf_tm_port_flowcontrol_rx_set()
 *               bf_tm_port_flowcontrol_mode_set()
 *
 * Default : Ignore Pause, PFC.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param type       Pause type.
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_port_flowcontrol_rx_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_flow_ctrl_type_t *type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == type));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  return (bf_tm_port_get_flowcontrol_rx(dev, p, type, type));
}

/*
 * Get iCoS(iCoS = ig_intr_md.ingress_cos) to packet CoS.
 *
 * Default: No PFC
 *
 * Related APIs: bf_tm_port_pfc_cos_mapping_set()
 *
 * @param dev         ASIC device identifier.
 * @param port        Port handle
 * @param cos_to_icos Array of 8 uint8_t values.
 *                    Array index is CoS and array value is iCoS.
 * @return            Status of API call.
 *
 */
bf_status_t bf_tm_port_pfc_cos_mapping_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           uint8_t *cos_to_icos) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == cos_to_icos));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  return (bf_tm_port_get_pfc_cos_map(dev, p, cos_to_icos));
}

/**
 * This API can be used to get cut-through enable status of a port.
 *
 * Related APIs: bf_tm_port_cut_through_enable()
 *               bf_tm_port_cut_through_disable()
 *
 * Default : Cut-through is disabled.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param sw_ct_enabled      Cut-through enable status in TM SW cache
 * @param hw_ct_enabled      Cut-through enable status in TM HW register
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_port_cut_through_enable_status_get(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bool *sw_ct_enabled,
                                                     bool *hw_ct_enabled) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == sw_ct_enabled));
  /* hw_ct_enabled might be NULL if only SW status is needed. */

  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_get_cut_through_enable_status(
      dev, p, sw_ct_enabled, hw_ct_enabled);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Get queue scheduling priority.
 * Scheduling priority level when serving guaranteed bandwidth. Higher the
 * number, higher the  priority to select the queue for scheduling.
 *
 * Related APIs: bf_tm_sched_q_priority_set()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param queue                 queue
 * @param priority              Scheduling priority of queue.
 */
bf_status_t bf_tm_sched_q_priority_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bf_tm_sched_prio_t *priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == priority));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_sch_get_q_sched_prio(dev, q, false, priority, priority);

  return (rc);
}

/*
 * Get queue scheduling speed capability.
 * When a queue is carved on its port configured at some line rate
 * and port scheduling speed, then the queue becomes assigned to
 * appropriate HW scheduling capacity not worse than the port's
 * scheduling speed.
 *
 * Related APIs: bf_tm_sched_port_enable(),
 *               bf_tm_port_q_mapping_set()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port
 * @param[in]  queue  Queue
 * @param[out] speed  Current scheduling speed capability of the queue.
 *                    BF_SPEED_NONE if not assigned to HW resources.
 * @return            Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_speed_get(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bf_port_speeds_t *speed) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == speed));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_sch_get_q_speed(dev, q, speed);
  return (rc);
}

/*
 * Get queue DWRR weights. These weights are used when queues at same
 * priority level are scheduled during excess bandwidth sharing.
 *
 * Related APIs: bf_tm_sched_q_dwrr_weight_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param weight          Weight value. Supported range [ 0.. 1023 ]
 */
bf_status_t bf_tm_sched_q_dwrr_weight_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint16_t *weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == weight));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_sch_get_q_dwrr_wt(dev, q, weight, weight);
  return (rc);
}

/*
 * Default queue DWRR weights. These weights are used when queues at same
 * priority level are scheduled during excess bandwidth sharing.
 *
 * Related APIs: bf_tm_sched_q_dwrr_weight_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param weight          Weight value. Supported range [ 0.. 1023 ]
 */
bf_status_t bf_tm_sched_q_dwrr_weight_get_default(bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bf_tm_queue_t queue,
                                                  uint16_t *weight) {
  (void)port;
  (void)queue;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == weight));

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_dwrr_weight_is_valid) {
    *weight = def.sch_q_dwrr_weight;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get queue shaping guaranteed rate enable status.
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_enable()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      Port
 * @param[in] queue     queue
 * @param[out] enable   True if the queue guaranteed shaping rate
 *                      is enabled.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_guaranteed_enable_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    *enable = q->q_sch_cfg.min_rate_enable;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get queue scheduling enable status.
 *
 * Related APIs: bf_tm_sched_q_enable()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      Port
 * @param[in] queue     queue
 * @param[out] enable   True if the queue scheduling
 *                      is enabled.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_enable_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue,
                                     bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    *enable = q->q_sch_cfg.sch_enabled;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get queue PFC status
 *
 * Related APIs: bf_tm_sch_get_q_egress_pfc_status(),
 *               bf_tm_sched_q_egress_pfc_status_set(),
 *               bf_tm_sched_q_egress_pfc_status_clear()
 *
 * @param[in]   dev       ASIC device identifier.
 * @param[in]   port      port
 * @param[in]   queue     queue
 * @param[out]  status    Egress Queue PFC Status - True or False value of
 *                        pfc_pause
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_egress_pfc_status_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool *status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_get_q_egress_pfc_status(dev, q, status, status);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 *Get queue scheduling default status.
 *
 * Related APIs : bf_tm_sched_q_enable()
 *
 * @param[in] dev ASIC device identifier.
 * @param[in] port Port
 * @param[in] queue queue
 * @param[out] enable True if the queue scheduling
 *             is enabled by default.
 * @ return Status of API call.
 *   BF_SUCCESS on success
 *   Non-Zero on error
 */
bf_status_t bf_tm_sched_q_enable_get_default(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             bool *enable) {
  (void)port;
  (void)queue;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_enable_is_valid) {
    *enable = def.sch_q_enable;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get queue shaping max rate enable status.
 *
 * Related APIs: bf_tm_sched_q_shaping_rate_enable()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      Port
 * @param[in] queue     queue
 * @param[out] enable   True if the queue max shaping rate
 *                      is enabled.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_shaping_enable_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    *enable = q->q_sch_cfg.max_rate_enable;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get queue shaping max rate defaults.
 *
 * Related APIs: bf_tm_sched_q_shaping_rate_enable()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] queue      queue
 * @param[out] enable    True if the queue max shaping rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority of queue.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_shaping_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  (void)port;
  (void)queue;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));
  BF_TM_INVALID_ARG((NULL == priority));

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_shaping_enable_is_valid && def.sch_q_priority_is_valid) {
    *enable = def.sch_q_shaping_enable;
    *priority = def.sch_q_priority;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get queue guaranteed min rate defaults.
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_enable()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] queue      queue
 * @param[out] enable    True if the queue guaranteed rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority of queue.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_guaranteed_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  (void)port;
  (void)queue;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));
  BF_TM_INVALID_ARG((NULL == priority));

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_guaranteed_enable_is_valid && def.sch_q_priority_is_valid) {
    *enable = def.sch_q_guaranteed_enable;
    *priority = def.sch_q_priority;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get queue shaping rate. Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_q_shaping_rate_set()
 *
 * @param[in]  dev           ASIC device identifier.
 * @param[in]  port          Port
 * @param[in]  queue         queue
 * @param[out] pps           If set to true, values are in terms of pps
 *                           and packets, else in terms of kbps and bytes.
 * @param[out] burst_size    Burst size in packets or bytes.
 * @param[out] rate          Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_q_shaping_rate_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool *pps,
                                           uint32_t *burst_size,
                                           uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_get_q_max_rate(
        dev, q, pps, burst_size, burst_size, rate, rate);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get queue shaping rate. Rate is in units of kbps or pps. Also get the
 * rate provisioning type.
 *
 * Related APIs: bf_tm_sched_q_shaping_rate_set()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] queue         queue
 * @param[out] pps          If set to true, rates are applied in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 * @param[out] prov_type    The rate provisioning type (OVER, UNDER,
 *                          MIN_ERROR)
 */
bf_status_t bf_tm_sched_q_shaping_rate_get_provisioning(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_get_q_max_rate_provisioning(
        dev, q, pps, burst_size, burst_size, rate, rate, prov_type);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Gets queue default shaping rate.
 * Rate is in units of kbps or pps. Also get the rate provisioning type.
 *
 * Related APIs: bf_tm_sched_q_shaping_rate_get_provisioning()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] queue         queue
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 * @param[out] prov_type    The rate provisioning type (OVER, UNDER,
 *                          MIN_ERROR)
 */
bf_status_t bf_tm_sched_q_shaping_rate_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  (void)queue;

  // Queue Shaping settings are _almost_ the same as its Port Shaping settings

  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == pps))
  BF_TM_INVALID_ARG((NULL == burst_size))
  BF_TM_INVALID_ARG((NULL == rate))
  BF_TM_INVALID_ARG((NULL == prov_type))

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, p, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_shaping_rate_pps_is_valid &&
      def.sch_q_shaping_rate_prov_type_is_valid &&
      def.sch_q_shaping_rate_burst_size_is_valid &&
      def.sch_q_shaping_rate_is_valid) {
    *pps = def.sch_q_shaping_rate_pps;
    *prov_type = def.sch_q_shaping_rate_prov_type;
    *burst_size = def.sch_q_shaping_rate_burst_size;
    *rate = def.sch_q_shaping_rate;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get queue guaranteed rate. Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] queue           Queue
 * @param[out] pps            If set to true, values are in terms of
 *                            pps and packets, else in kbps and bytes.
 *                            else in terms of kbps.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_guaranteed_rate_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bf_tm_queue_t queue,
                                              bool *pps,
                                              uint32_t *burst_size,
                                              uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == pps))
  BF_TM_INVALID_ARG((NULL == burst_size))
  BF_TM_INVALID_ARG((NULL == rate))

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_sch_get_q_gmin_rate(
      dev, q, pps, burst_size, burst_size, rate, rate);
  return (rc);
}

/*
 * Default queue guaranteed rate. Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_q_guaranteed_rate_get()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] queue         Queue
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 * @param[out] prov_type    The rate provisioning type (OVER, UNDER,
 *                          MIN_ERROR)
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_guaranteed_rate_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  (void)queue;
  bf_tm_port_t *p;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == pps))
  BF_TM_INVALID_ARG((NULL == burst_size))
  BF_TM_INVALID_ARG((NULL == rate))
  BF_TM_INVALID_ARG((NULL == prov_type))

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, p, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_guaranteed_rate_pps_is_valid &&
      def.sch_q_guaranteed_rate_prov_type_is_valid &&
      def.sch_q_guaranteed_rate_burst_size_is_valid &&
      def.sch_q_guaranteed_rate_is_valid) {
    *pps = def.sch_q_guaranteed_rate_pps;
    *prov_type = def.sch_q_guaranteed_rate_prov_type;
    *burst_size = def.sch_q_guaranteed_rate_burst_size;
    *rate = def.sch_q_guaranteed_rate;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the queue for scheduling.
 *
 * Related APIs: bf_tm_sched_q_remaining_bw_priority_set()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param queue           queue
 * @param priority        Scheduling priority of queue.
 */
bf_status_t bf_tm_sched_q_remaining_bw_priority_get(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_prio_t *priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == priority));

  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc != BF_SUCCESS) return (rc);

  rc = bf_tm_sch_get_q_sched_prio(dev, q, true, priority, priority);
  return (rc);
}

/*
 * Get port shaping rate. Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_port_shaping_rate_set()
 *
 * @param[in] dev          ASIC device identifier.
 * @param[in] port         Port
 * @param[out] pps         If set to true, values are in terms of pps
 *                         and packets else in terms of kbps and bytes.
 * @param[out] burst_size  Burst size packets or bytes.
 * @param[out] rate        Shaper value in pps or kbps.
 * @return                 Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_rate_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bool *pps,
                                              uint32_t *burst_size,
                                              uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == rate));
  BF_TM_INVALID_ARG((NULL == pps));
  BF_TM_INVALID_ARG((NULL == burst_size));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_port_max_rate(
      dev, p, pps, burst_size, burst_size, rate, rate);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get port PFC status.
 *
 * Related APIs: bf_tm_sch_get_port_egress_pfc_status()
 *
 * @param dev             ASIC device identifier.
 * @param port            Port
 * @param status          PFC status.
 */
bf_status_t bf_tm_sched_port_egress_pfc_status_get(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   uint8_t *status) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_port_egress_pfc_status(dev, p, status, status);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get port shaping rate. Rate is in units of kbps or pps.  Also get the
 * rate provisioning type.
 *
 * Related APIs: bf_tm_sched_port_shaping_rate_set()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 * @param[out] prov_type    The rate provisioning type (OVER, UNDER,
 *MIN_ERROR)
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_rate_get_provisioning(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == pps))
  BF_TM_INVALID_ARG((NULL == burst_size))
  BF_TM_INVALID_ARG((NULL == rate))

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_port_max_rate_provisioning(
      dev, p, pps, burst_size, burst_size, rate, rate, prov_type);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get Port shaping max rate enable default status.
 *
 * Related APIs: bf_tm_sched_port_shaping_enable()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[out] enable    True if the port max shaping rate
 *                       is enabled by default.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_enable_get_default(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        bool *enable) {
  (void)port;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  bf_tm_sch_port_defaults_t def;
  rc = bf_tm_sch_port_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_port_shaping_enable_is_valid) {
    *enable = def.sch_port_shaping_enable;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get Port shaping max rate enable status.
 *
 * Related APIs: bf_tm_sched_port_shaping_enable()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] port      Port
 * @param[out] enable   True if the port max shaping rate
 *                      is enabled.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_enable_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  rc = bf_tm_sch_get_port_max_rate_enable_status(dev, p, enable, enable);

  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Default port shaping rate.
 * Also gets the rate provisioning type.
 * The default rate depends on the current port scheduling speed,
 * and if it is disabled, then the maximum port speed is used.
 *
 * Related APIs: bf_tm_sched_port_shaping_rate_get_provisioning()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 * @param[out] prov_type    The rate provisioning type (OVER, UNDER,
 *                          MIN_ERROR)
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_shaping_rate_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bool *pps,
    uint32_t *burst_size,
    uint32_t *rate,
    bf_tm_sched_shaper_prov_type_t *prov_type) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == pps))
  BF_TM_INVALID_ARG((NULL == burst_size))
  BF_TM_INVALID_ARG((NULL == rate))
  BF_TM_INVALID_ARG((NULL == prov_type))

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  bf_tm_sch_port_defaults_t def;
  rc = bf_tm_sch_port_get_defaults(dev, p, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_port_shaping_rate_pps_is_valid &&
      def.sch_port_shaping_rate_prov_type_is_valid &&
      def.sch_port_shaping_rate_burst_size_is_valid &&
      def.sch_port_shaping_rate_is_valid) {
    *pps = def.sch_port_shaping_rate_pps;
    *prov_type = def.sch_port_shaping_rate_prov_type;
    *burst_size = def.sch_port_shaping_rate_burst_size;
    *rate = def.sch_port_shaping_rate;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get TM Port current status.
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[out] is_offline     Port is currently offline.
 * @param[out] is_enabled     Port is currently enabled.
 * @param[out] qac_rx_enabled QAC Rx current status.
 * @param[out] recirc_enabled Recirculation status.
 * @param[out] has_mac        Port has MAC.
 * @return             Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_status_get(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bool *is_offline,
                                  bool *is_enabled,
                                  bool *qac_rx_enable,
                                  bool *recirc_enable,
                                  bool *has_mac) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  if (is_offline) {
    *is_offline = p->offline;
  }
  if (is_enabled) {
    *is_enabled = p->admin_state;
  }
  if (qac_rx_enable) {
    *qac_rx_enable = p->qac_rx_enable;
  }
  if (recirc_enable) {
    *recirc_enable = p->recirc_enable;
  }
  if (has_mac) {
    *has_mac = p->has_mac;
  }

  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get port scheduling speed reset value.
 *
 * Related APIs: bf_tm_sched_port_speed_get()
 *
 * @param[in] dev      ASIC device identifier.
 * @param[in] port     Port
 * @param[out] speed   Port scheduling speed when it was added.
 * @return             Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_speed_get_reset(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_port_speeds_t *speed) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == speed));
  if (NULL == g_tm_ctx[dev]) {
    return BF_UNEXPECTED;
  }

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  *speed = p->speed_on_add;
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Get port current scheduling speed.
 *
 * Related APIs: bf_tm_sched_port_enable()
 *
 * @param[in] dev      ASIC device identifier.
 * @param[in] port     Port
 * @param[out] speed   Current scheduling speed on the port.
 * @return             Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_speed_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_port_speeds_t *speed) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == speed));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  *speed = p->speed;
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/*
 * Get per packet byte adjustment default value
 *
 * Related APIs: bf_tm_sched_pkt_ifg_compensation_get ()
 *
 * @param[in] dev          ASIC device identifier.
 * @param[in] pipe         Pipe identifier.
 * @param[out] adjustment  Default byte adjustment done on every packet.
 */
bf_status_t bf_tm_sched_pkt_ifg_compensation_get_default(bf_dev_id_t dev,
                                                         bf_dev_pipe_t pipe,
                                                         uint8_t *adjustment) {
  (void)pipe;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == adjustment))

  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  bf_tm_pipe_defaults_t def;
  rc = bf_tm_pipe_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc) {
    if (def.pkt_ifg_compensation_is_valid) {
      *adjustment = def.pkt_ifg_compensation;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  return (rc);
}

/*
 * Get per packet byte adjustment value
 *
 * Related APIs: bf_tm_sched_pkt_ifg_compensation_set ()
 *
 * @param[in] dev          ASIC device identifier.
 * @param[in] pipe         Pipe identifier.
 * @param[out] adjustment  Byte adjustment done on every packet.
 */
bf_status_t bf_tm_sched_pkt_ifg_compensation_get(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 uint8_t *adjustment) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == adjustment))
  rc = bf_tm_sch_get_pkt_ifg_compensation(dev, pipe, adjustment, adjustment);
  return (rc);
}

/**
 * Get number of global timestamp bits that is to be right shifted.
 *
 * @param[in] dev            ASIC device identifier.
 * @param[out] shift         Number of Global timestamp bits that are right
 *                           shifted.
 *                           Up to 16 bits can be right shifted. Any shift
 *value
 *                           greater than 16 is capped to 16.
 * @return                   Status of API call.
 */
bf_status_t bf_tm_timestamp_shift_get(bf_dev_id_t dev, uint8_t *shift) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == shift))
  rc = bf_tm_get_timestamp_shift(dev, shift);
  return (rc);
}

/**
 * Get the default number of global timestamp bits that is to be right
 *shifted.
 *
 * @param[in] dev            ASIC device identifier.
 * @param[out] shift         Number of Global timestamp bits that are right
 *                           shifted.
 *                           Upto 16 bits can be right shifted. Any shift
 *value
 *                           greater than 16 is capped to 16.
 * @return                   Status of API call.
 */
bf_status_t bf_tm_timestamp_shift_get_default(bf_dev_id_t dev, uint8_t *shift) {
  BF_TM_INVALID_ARG(shift == NULL);
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));

  *shift = 0;

  return (BF_SUCCESS);
}

/**
 * Use this API to get (port, queue) used for egressing out
 * negative mirror traffic in a pipe.
 * Port queue is get according to the port's current queue map.
 *
 * param dev        ASIC device identifier.
 * param pipe       Pipe Identifier.
 * param port       Negative Mirror device port.
 * param queue      Port Queue for negative mirror traffic.
 * return           Status of API call.
 */
bf_status_t bf_tm_port_mirror_on_drop_dest_get(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_dev_port_t *port,
                                               bf_tm_queue_t *queue) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q = NULL;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == port));
  BF_TM_INVALID_ARG((NULL == queue));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_mirror_on_drop_destination(dev, pipe, &q, &q);
  TM_UNLOCK_AND_FLUSH(dev);
  if (BF_SUCCESS != rc) {
    return (rc);
  }
  if (NULL == q) {
    // Not initialized yet
    return bf_tm_port_mirror_on_drop_dest_get_default(dev, pipe, port, queue);
  }

  *port = MAKE_DEV_PORT(q->l_pipe, q->uport);
  rc = bf_tm_q_get_port_queue_nr(dev, q, queue);
  return (rc);
}

/**
 * Use this API to get (port, queue) used for egressing out
 * negative mirror traffic in a pipe by default.
 *
 * Related APIs: bf_tm_port_mirrot_on_drop_dest_get()
 *
 * param dev[in]     ASIC device identifier.
 * param pipe[in]    Pipe Identifier.
 * param port[out]   Default negative Mirror port.
 * param queue[out]  Default queue where negative mirror traffic is enqueued.
 * return            Status of API call.
 */
bf_status_t bf_tm_port_mirror_on_drop_dest_get_default(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bf_dev_port_t *port,
                                                       bf_tm_queue_t *queue) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return BF_UNEXPECTED;
  }
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG((NULL == port));
  BF_TM_INVALID_ARG((NULL == queue));

  bf_tm_pipe_defaults_t def;
  bf_tm_eg_pipe_t *p;
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  rc = bf_tm_pipe_get_defaults(dev, p, &def);
  if (BF_SUCCESS == rc) {
    if (def.port_mirror_on_drop_dest_is_valid &&
        def.queue_mirror_on_drop_dest_is_valid) {
      *port = def.port_mirror_on_drop_dest;
      *queue = def.queue_mirror_on_drop_dest;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }
  return (rc);
}

/*
 * Get queue scheduler advanced flow control mode.
 * Scheduler Advanced Flow Control Mechanism, 0 = Credit 1 = Xoff
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_q_adv_fc_mode_set()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param queue                 queue
 * @param mode                  Scheduler Advanced Flow Control Mode
 *                              0 = credit 1 = xoff.
 */
bf_status_t bf_tm_sched_q_adv_fc_mode_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          bf_tm_sched_adv_fc_mode_t *mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_get_q_adv_fc_mode(dev, q, mode, mode);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Default queue scheduler advanced flow control mode.
 * Scheduler Advanced Flow Control Mechanism, 0 = Credit 1 = Xoff
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_q_adv_fc_mode_set()
 *
 * @param dev                   ASIC device identifier.
 * @param port                  Port
 * @param queue                 queue
 * @param mode                  Scheduler Advanced Flow Control Mode
 *                              0 = credit 1 = xoff.
 */
bf_status_t bf_tm_sched_q_adv_fc_mode_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_queue_t queue,
    bf_tm_sched_adv_fc_mode_t *mode) {
  (void)port;
  (void)queue;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == mode));

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_q_adv_fc_mode_is_valid) {
    *mode = def.sch_q_adv_fc_mode;
  } else {
    rc = BF_NOT_SUPPORTED;
  }
  return (rc);
}

/*
 * Get scheduler advanced flow control mode enable/disable mode.
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_adv_fc_mode_enable_set()
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Logical PipeId index
 * @param enable                Scheduler Advanced Flow Control Mode
 *                              Enable/Disable
 */

bf_status_t bf_tm_sched_adv_fc_mode_enable_get(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bool *enable) {
  bf_status_t rc;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_adv_fc_mode_enable(dev, p, enable);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Get scheduler advanced flow control default mode.
 * used for Ghost Thread Implementation
 *
 * Related APIs: bf_tm_sched_adv_fc_mode_enable_get()
 *
 * @param dev[in]      ASIC device identifier.
 * @param pipe[in]     Logical PipeId index
 * @param enable[out]  Scheduler Advanced Flow Control Mode
 *                     default mode.
 * @return             Status of API call.
 */

bf_status_t bf_tm_sched_adv_fc_mode_enable_get_default(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bool *enable) {
  (void)pipe;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable))
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  bf_tm_sch_q_defaults_t def;
  rc = bf_tm_sch_q_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_adv_fc_mode_enable_is_valid) {
    *enable = def.sch_adv_fc_mode_enable;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get current credits for 100G ports (thread safe).
 *
 * Related APIs: bf_tm_port_credits_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier.
 * @param[out] credits    Number of credits.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_credits_get_safe(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        uint32_t *credits) {
  bf_status_t rc = BF_SUCCESS;

  TM_MUTEX_LOCK(&g_tm_timer_lock[dev]);
  rc = bf_tm_port_credits_get(dev, port, credits);
  TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev]);

  return (rc);
}

/*
 * Get current credits for 100G ports.
 *
 * Related APIs: bf_tm_port_get_credits()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier.
 * @param credits    Number of credits.
 * @return           Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_credits_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   uint32_t *credits) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);
  rc = bf_tm_port_get_credits(dev, p);
  if (credits != NULL) {
    *credits = p->credits;
  }
  return (rc);
}

/*
 * Get total PFC levels
 *
 * Related APIs: bf_tm_pool_pfc_limit_get()
 *
 * @param dev[in]        ASIC device identifier.
 * @param levels[out]    Number of PFC levels.
 * @return               Status of API call.
 */
bf_status_t bf_tm_max_pfc_levels_get(bf_dev_id_t dev, uint32_t *levels) {
  BF_TM_INVALID_ARG(levels == NULL);
  (void)dev;

  *levels = BF_TM_MAX_PFC_LEVELS;

  return BF_SUCCESS;
}

/**
 * Get L1 Node the Queue is associated to.
 *
 * Related APIs: bf_tm_sched_q_l1_set()
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  port        Device port id.
 * @param[in]  queue       Port Queue number.
 * @param[out] l1_node     L1 Node number (in the same port group).
 * @return                 Status of API call.
 */
bf_status_t bf_tm_sched_q_l1_get(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 bf_tm_l1_node_t *l1_node) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_q_t *q;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == l1_node))

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_q_get_descriptor(dev, port, queue, &q);
  if (rc == BF_SUCCESS) {
    rc = bf_tm_sch_q_l1_get(dev, q, l1_node, l1_node);
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Get L1 Node the Queue is associated by default.
 *
 * Related APIs: bf_tm_sched_q_l1_get()
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  port        Device port id.
 * @param[in]  queue       Port Queue number.
 * @param[out] l1_node     Default L1 Node number in the port group.
 * @return                 Status of API call.
 */
bf_status_t bf_tm_sched_q_l1_get_default(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_l1_node_t *l1_node) {
  (void)queue;  // TF2: default L1 Node is the same for all queues on the port.

  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1 = NULL;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == l1_node))

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  rc = bf_tm_sch_l1_get_port_nth(dev, port, 0, &l1);
  if (BF_SUCCESS == rc) {
    if (l1 == NULL || port != MAKE_DEV_PORT((l1->l_pipe), (l1->uport))) {
      rc = BF_UNEXPECTED;
    } else {
      *l1_node = l1->logical_l1;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get L1 Node port asssignment status.
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  pipe        Pipe Identifier.
 * @param[in]  pg_id       Device port group (a.k.a. 'quad').
 * @param[in]  l1_node     L1 Node number (in the port group).
 * @param[out] in_use      True if L1 Node is associated to some port.
 * @param[out] pg_port_nr  Associated port number in the port group.
 * @param[out] port_id     Device port identifier (pipe id and port number)
 *                         of the associated port.
 * @return                 Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_port_assignment_get(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_tm_pg_t pg_id,
                                               bf_tm_l1_node_t l1_node,
                                               bool *in_use,
                                               uint8_t *pg_port_nr,
                                               bf_dev_port_t *port_id) {
  bf_dev_port_t pg_base_port = 0;

  bf_status_t rc =
      bf_tm_pipe_port_group_get_first_port(dev, pipe, pg_id, &pg_base_port);
  if (rc != BF_SUCCESS) return (rc);

  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev]->tm_cfg.l1_per_pg));

  bf_tm_eg_l1_t *l1 = NULL;

  rc = bf_tm_sch_l1_get_descriptor(dev, pg_base_port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);
  if (NULL == l1) {
    return (BF_UNEXPECTED);
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  if (NULL != in_use) {
    *in_use = l1->in_use;
  }

  uint8_t pg_port_nr_ =
      (DEV_PORT_TO_LOCAL_PORT(l1->port)) % (g_tm_ctx[dev]->tm_cfg.ports_per_pg);

  if ((pg_id != l1->pg) || (pg_port_nr_ != l1->l1_sch_cfg.cid)) {
    LOG_ERROR(
        "%s:%d pipe=%d pg=%d pg_l1_node=%d inconsistent pg_port and channel "
        "(%d!=%d)",
        __func__,
        __LINE__,
        pipe,
        pg_id,
        l1_node,
        pg_port_nr_,
        l1->l1_sch_cfg.cid);
    rc = BF_UNEXPECTED;
  }

  TM_UNLOCK_AND_FLUSH(dev);

  if (NULL != port_id) {
    // Can't use l1->uport here as it is initially 0 for non assigned nodes
    bf_dev_port_t l1_port =
        lld_sku_map_devport_from_user_to_device(dev, pg_base_port) +
        pg_port_nr_;
    *port_id = MAKE_DEV_PORT(
        l1->l_pipe, lld_sku_map_devport_from_device_to_user(dev, l1_port));
  }

  if (NULL != pg_port_nr) {
    *pg_port_nr = pg_port_nr_;
  }

  return (rc);
}

/**
 * Get L1 Node default port asssignment status.
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  pipe        Pipe Identifier.
 * @param[in]  pg_id       Device port group (a.k.a. 'quad').
 * @param[in]  l1_node     L1 Node number (in the port group).
 * @param[out] in_use      True if L1 Node is associated to some port by
 * default.
 * @param[out] pg_port_nr  Associated default port number in the port group.
 * @param[out] port_id     Device port identifier (pipe id and port number)
 *                         of the port associated by default.
 * @return                 Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_port_assignment_get_default(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bf_tm_pg_t pg_id,
                                                       bf_tm_l1_node_t l1_node,
                                                       bool *in_use,
                                                       uint8_t *pg_port_nr,
                                                       bf_dev_port_t *port_id) {
  bf_dev_port_t pg_base_port = 0;

  bf_status_t rc =
      bf_tm_pipe_port_group_get_first_port(dev, pipe, pg_id, &pg_base_port);
  if (rc != BF_SUCCESS) return (rc);

  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev]->tm_cfg.l1_per_pg));

  bf_dev_port_t pg_port_nr_ =
      ((g_tm_ctx[dev]->tm_cfg.ports_per_pg) > l1_node) ? l1_node : 0;

  if (NULL != port_id) {
    bf_dev_port_t l1_port =
        lld_sku_map_devport_from_user_to_device(dev, pg_base_port) +
        pg_port_nr_;
    *port_id = MAKE_DEV_PORT(
        pipe, lld_sku_map_devport_from_device_to_user(dev, l1_port));
  }

  if (NULL != pg_port_nr) {
    *pg_port_nr = pg_port_nr_;
  }

  if (NULL != in_use) {
    *in_use = ((g_tm_ctx[dev]->tm_cfg.ports_per_pg) > l1_node);
  }

  return (rc);
}

/**
 * Get Port to L1 Nodes assignment status.
 *
 * @param[in]  dev           ASIC device identifier.
 * @param[in]  port_id       Device port identifier (pipe id and port number).
 * @param[in]  in_use_only   Count only active L1 Nodes.
 * @param[out] l1_nodes_cnt  Number of items in l1_nodes.
 * @param[out] l1_nodes      Array of Port group L1 Node ids associated
 *                           with the port_id. Caller has to provide memory
 *                           for 32 max items (BF_TM_TOF2_SCH_L1_PER_PG).
 * @return                   Status of API call.
 *
 */
bf_status_t bf_tm_port_l1_node_assignment_get(bf_dev_id_t dev,
                                              bf_dev_port_t port_id,
                                              bool in_use_only,
                                              uint8_t *l1_nodes_cnt,
                                              bf_tm_l1_node_t *l1_nodes) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(!DEV_PORT_VALIDATE(port_id));  // Only regular ports
  BF_TM_INVALID_ARG(
      TM_IS_PIPE_INVALID(DEV_PORT_TO_PIPE(port_id), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port_id, g_tm_ctx[dev]));

  BF_TM_INVALID_ARG(l1_nodes_cnt == NULL);
  BF_TM_INVALID_ARG(l1_nodes == NULL);

  bf_tm_eg_l1_t *pg_l1_nodes = g_tm_ctx[dev]->eg_l1;
  if (NULL == pg_l1_nodes) {
    return (BF_UNEXPECTED);
  }
  pg_l1_nodes = BF_TM_FIRST_L1_PTR_IN_PG((g_tm_ctx[dev]), port_id);

  uint8_t res_l1_cnt = 0;
  *l1_nodes_cnt = 0;

  uint8_t pipe_id = DEV_PORT_TO_PIPE(port_id);
  bf_tm_pg_t pg_id = 0;
  bf_tm_pg_t pg_port_nr = 0;

  rc = bf_tm_port_group_get(dev, port_id, &pg_id, &pg_port_nr);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("%s:%d dev=%d port_id=%d port group get rc=%d",
              __func__,
              __LINE__,
              dev,
              port_id,
              rc);
    return rc;
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  for (uint8_t pg_l1 = 0; pg_l1 < (g_tm_ctx[dev]->tm_cfg.l1_per_pg); pg_l1++) {
    if (in_use_only && !(pg_l1_nodes[pg_l1].in_use)) {
      continue;  // skip free nodes.
    }

    //-- L1 Node sanity check
    if (((pg_l1_nodes[pg_l1].l_pipe) != pipe_id) ||
        ((pg_l1_nodes[pg_l1].pg) != pg_id) ||
        ((pg_l1_nodes[pg_l1].logical_l1) != pg_l1)) {
      LOG_ERROR("%s:%d pipe=%d pg_id=%d pg_l1_node=%d SW memory inconsistent",
                __func__,
                __LINE__,
                pipe_id,
                pg_id,
                pg_l1);
      rc = BF_UNEXPECTED;
      break;
    }

    if ((pg_l1_nodes[pg_l1].uport) == DEV_PORT_TO_LOCAL_PORT(port_id)) {
      // L1 Node to Port sanity check
      if ((pg_l1_nodes[pg_l1].l1_sch_cfg.cid) != pg_port_nr) {
        LOG_ERROR(
            "%s:%d pipe=%d pg=%d pg_l1_node=%d inconsistent pg_port and "
            "channel (%d!=%d)",
            __func__,
            __LINE__,
            pipe_id,
            pg_id,
            pg_l1,
            pg_port_nr,
            pg_l1_nodes[pg_l1].l1_sch_cfg.cid);
        rc = BF_UNEXPECTED;
        break;
      }

      l1_nodes[res_l1_cnt++] = pg_l1;
    }
  }

  TM_UNLOCK_AND_FLUSH(dev);

  *l1_nodes_cnt = res_l1_cnt;
  return (rc);
}

/**
 * Get L1 Node queue asssignments.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[in]  pipe           Pipe Identifier.
 * @param[in]  pg_id          Device port group (a.k.a. 'quad').
 * @param[in]  l1_node        L1 Node number (in the port group).
 * @param[out] l1_queues_cnt  Number of items in l1_queues for port group
 *                            queue ids associated with the L1 Node.
 * @param[out] l1_queues      Array of Port group queue ids associated
 *                            with the L1 Node. Caller has to provide memory
 *                            for 128 max items (BF_TM_TOF2_QUEUES_PER_PG).
 * @return                    Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_queue_assignment_get(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bf_tm_pg_t pg_id,
                                                bf_tm_l1_node_t l1_node,
                                                uint8_t *l1_queues_cnt,
                                                bf_tm_queue_t *l1_queues) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  BF_TM_INVALID_ARG(l1_queues_cnt == NULL);
  BF_TM_INVALID_ARG(l1_queues == NULL);

  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(pg_id >= (g_tm_ctx[dev]->tm_cfg.pg_per_pipe));
  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev]->tm_cfg.l1_per_pg));

  if (NULL == (g_tm_ctx[dev]->eg_q)) {
    return (BF_UNEXPECTED);
  }

  int pg_queue_idx = ((int)(g_tm_ctx[dev]->tm_cfg.q_per_pipe)) * pipe;
  pg_queue_idx += (int)((g_tm_ctx[dev]->tm_cfg.q_per_pg)) * (pg_id);

  bf_tm_eg_q_t *pg_queues = &((g_tm_ctx[dev]->eg_q)[pg_queue_idx]);

  uint8_t res_q_cnt = 0;
  *l1_queues_cnt = 0;

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  for (uint8_t pg_q = 0; pg_q < (g_tm_ctx[dev]->tm_cfg.q_per_pg); pg_q++) {
    if (NULL == (pg_queues[pg_q].l1)) {
      continue;  // this queue has no L1 Node assigned.
    }

    //-- Queue sanity check
    if (((pg_queues[pg_q].l_pipe) != pipe) || ((pg_queues[pg_q].pg) != pg_id) ||
        ((pg_queues[pg_q].logical_q) != pg_q)) {
      LOG_ERROR(
          "%s:%d pipe=%d pg=%d pg_queue=%d SW memory inconsistent (pipe=%d "
          "pg=%d pg_queue=%d)",
          __func__,
          __LINE__,
          pipe,
          pg_id,
          pg_q,
          pg_queues[pg_q].l_pipe,
          pg_queues[pg_q].pg,
          pg_queues[pg_q].logical_q);
      rc = BF_UNEXPECTED;
      break;
    }

    //-- Queue to L1 Node link sanity check
    if (((pg_queues[pg_q].l1->l_pipe) != pipe) ||
        ((pg_queues[pg_q].l1->pg) != pg_id)) {
      LOG_ERROR(
          "%s:%d pipe=%d pg=%d pg_queue=%d L1 Node memory inconsistent "
          "(p_pipe=%d l_pipe=%d, pg=%d)",
          __func__,
          __LINE__,
          pipe,
          pg_id,
          pg_q,
          pg_queues[pg_q].l1->p_pipe,
          pg_queues[pg_q].l1->l_pipe,
          pg_queues[pg_q].l1->pg);
      rc = BF_UNEXPECTED;
      break;
    }

    if ((pg_queues[pg_q].l1->logical_l1) == l1_node) {
      // Queue is assigned to the L1 Node requested
      if (DEV_PORT_TO_LOCAL_PORT((pg_queues[pg_q].l1->port)) !=
          DEV_PORT_TO_LOCAL_PORT((pg_queues[pg_q].port))) {
        LOG_WARN(
            "%s:%d pipe=%d pg=%d pg_queue=%d mapping to pipe_port=%d "
            "mismatch with l1_node=%d to dev_port=%d assignment",
            __func__,
            __LINE__,
            pipe,
            pg_id,
            pg_q,
            pg_queues[pg_q].port,
            l1_node,
            pg_queues[pg_q].l1->port);
      }
      l1_queues[res_q_cnt++] = pg_q;
    }
  }

  TM_UNLOCK_AND_FLUSH(dev);

  *l1_queues_cnt = res_q_cnt;
  return (rc);
}

/**
 * Get l1 node DWRR weight.
 * These weights are used by l1 nodes at same priority level.
 * Across priority these weights serve as ratio to
 * share excess or remaining bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_dwrr_weight_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] weight         Weight value
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_dwrr_weight_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node,
                                           uint16_t *weight) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(weight == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_dwrr_wt(dev, l1, weight, weight);
    } else {
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get l1 scheduler enable status.
 *
 * Related APIs: bf_tm_sched_l1_enable()
 *               bf_tm_sched_l1_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_enable_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_l1_node_t l1_node,
                                      bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t sw_port = 0;
  bf_dev_port_t hw_port = 0;
  bool sw_enable = 0;
  bool hw_enable = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(enable == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &sw_port, &hw_port, &sw_enable, &hw_enable);
  TM_UNLOCK_AND_FLUSH(dev);

  if (rc != BF_SUCCESS) return (rc);

  //-- SW vs. HW sanity check
  //   The SW Model has L1 Node scheduling and port not set.
  if ((TM_IS_TARGET_ASIC(dev)) &&
      ((sw_port != hw_port) || (sw_enable != hw_enable))) {
    LOG_WARN(
        "%s:%d dev=%d port=%d l1_node=%d "
        "(sw_port=%d, sw_enable=%d) != (hw_port=%d, hw_enable=%d)",
        __func__,
        __LINE__,
        dev,
        port,
        l1_node,
        sw_port,
        sw_enable,
        hw_port,
        hw_enable);
    // TODO: TM driver to handle all cases with HW and SW mismatch.
    // return (BF_UNEXPECTED);
  }

  // Check if the l1 node is associated to the same port
  if (sw_port != port && l1->in_use) {
    sw_enable = false;
    rc = BF_IN_USE;
  }

  *enable = sw_enable;

  return (rc);
}

/**
 * Get enable status for token bucket that assures l1 node guaranteed rate
 * (pps or bps)
 *
 * Default: l1 node guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable()
 *               bf_tm_sched_l1_guaranteed_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_enable_get(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(enable == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_guaranteed_rate_enable(dev, l1, enable, enable);
    } else {
      if (sched_enable) {
        *enable = false;
      }
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get enable status for token bucket that assures l1 node shaping rate
 * (pps or bps)
 *
 * Default: l1 node shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_l1_max_shaping_rate_enable()
 *               bf_tm_sched_l1_max_shaping_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_shaping_rate_enable_get(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(enable == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_max_shaping_rate_enable(dev, l1, enable, enable);
    } else {
      if (sched_enable) {
        *enable = false;
      }
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get enable status for priority propagation for children queues
 *
 * Default: priority propagation is disabled
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *               bf_tm_sched_l1_priority_prop_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_prop_enable_get(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(enable == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_priority_prop_enable(dev, l1, enable, enable);
    } else {
      if (sched_enable) {
        *enable = false;
      }
      LOG_ERROR("%s:%d l1_node=%d inconsistent port(%d,%d,%d) %sin use.",
                __func__,
                __LINE__,
                l1_node,
                port,
                port_out,
                l1->uport,
                (l1->in_use) ? "" : "or not ");
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get l1 node scheduling priority. Scheduling priority level used when
 * serving guaranteed bandwidth. Higher the number, higher the  priority to
 * select the l1 node for scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_priority_set()
 *
 * @param[in] dev                   ASIC device identifier
 * @param[in] port                  Port
 * @param[in] l1_node               l1 node
 * @param[out] priority             Scheduling priority of l1 node
 * @return                          Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_get(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bf_tm_sched_prio_t *priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(priority == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_priority(dev, l1, priority, priority);
    } else {
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the l1 node for
 * scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_remaining_bw_priority_set()
 *
 * @param[in] dev             ASIC device identifier
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] priority       Scheduling priority of l1 node
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_remaining_bw_priority_get(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t *priority) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(priority == NULL);

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_sched_prio(dev, l1, priority, priority);
    } else {
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get l1 node guaranteed rate in terms of pps or kbps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_get(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_l1_node_t l1_node,
                                               bool *pps,
                                               uint32_t *burst_size,
                                               uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_guaranteed_rate(
          dev, l1, pps, pps, burst_size, burst_size, rate, rate);
    } else {
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get l1 node shaping rate in units of kbps or pps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_shaping_rate_get(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node,
                                            bool *pps,
                                            uint32_t *burst_size,
                                            uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_l1_t *l1;
  bf_dev_port_t port_out;
  bool sched_enable;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));

  rc = bf_tm_sch_l1_get_descriptor(dev, port, l1_node, &l1);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_sch_get_l1_enable(
      dev, l1, &port_out, &port_out, &sched_enable, &sched_enable);

  // Check if the l1 node is associated to the same port
  if (rc == BF_SUCCESS) {
    if (port_out == port || !(l1->in_use)) {
      rc = bf_tm_sch_get_l1_shaping_rate(
          dev, l1, pps, pps, burst_size, burst_size, rate, rate);
    } else {
      rc = BF_IN_USE;
    }
  }
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get L1 Node DWRR weight default.
 *
 * Related APIs: bf_tm_sched_l1_dwrr_weight_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         L1 Node
 * @param[out] weight         Default weight value
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_dwrr_weight_get_default(bf_dev_id_t dev_id,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   uint16_t *weight) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev_id));
  if (!g_tm_ctx[dev_id]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev_id));
  if (NULL == g_tm_ctx[dev_id]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev_id]->tm_cfg.l1_per_pg));
  BF_TM_INVALID_ARG(weight == NULL);

  bf_tm_sch_l1_defaults_t def;
  bf_tm_eg_l1_t *l1;
  rc = bf_tm_sch_l1_get_descriptor(dev_id, port, l1_node, &l1);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  rc = bf_tm_sch_l1_get_defaults(dev_id, NULL, l1, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_l1_dwrr_weight_is_valid) {
    *weight = def.sch_l1_dwrr_weight;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get L1 Node guaranteed min rate defaults.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] l1_node    L1 Node
 * @param[out] enable    True if the L1 Node guaranteed rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_l1_guaranteed_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  (void)port;
  (void)l1_node;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == enable));
  BF_TM_INVALID_ARG((NULL == priority));

  bf_tm_sch_l1_defaults_t def;
  rc = bf_tm_sch_l1_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_l1_guaranteed_enable_is_valid &&
      def.sch_l1_guaranteed_priority_is_valid) {
    *enable = def.sch_l1_guaranteed_enable;
    *priority = def.sch_l1_guaranteed_priority;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get L1 Node shaping max rate defaults.
 *
 * Related APIs: bf_tm_sched_l1_shaping_rate_enable_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] l1_node    L1 Node
 * @param[out] enable    True if the L1 Node max shaping rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_l1_shaping_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority) {
  (void)port;
  (void)l1_node;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG((NULL == enable));
  BF_TM_INVALID_ARG((NULL == priority));

  bf_tm_sch_l1_defaults_t def;
  rc = bf_tm_sch_l1_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_l1_shaping_enable_is_valid &&
      def.sch_l1_shaping_priority_is_valid) {
    *enable = def.sch_l1_shaping_enable;
    *priority = def.sch_l1_shaping_priority;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/**
 * Get default status for priority propagation for children queues
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *               bf_tm_sched_l1_priority_prop_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_prop_get_default(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_l1_node_t l1_node,
                                                     bool *enable) {
  (void)port;
  (void)l1_node;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(enable == NULL);

  bf_tm_sch_l1_defaults_t def;
  rc = bf_tm_sch_l1_get_defaults(dev, NULL, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }
  if (def.sch_l1_priority_prop_enable_is_valid) {
    *enable = def.sch_l1_priority_prop_enable;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/*
 * Get L1 Node default guaranteed rate.
 * Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_get()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] l1_node       l1 node
 * @param[out] pps          If set to true, values are applied in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_get_default(bf_dev_id_t dev,
                                                       bf_dev_port_t port,
                                                       bf_tm_l1_node_t l1_node,
                                                       bool *pps,
                                                       uint32_t *burst_size,
                                                       uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev]->tm_cfg.l1_per_pg));

  if (!(TM_IS_TARGET_ASIC(dev)) &&
      (g_tm_ctx[dev]->tm_cfg.ports_per_pg) > l1_node) {
    // on SW Model 'default' L1 Nodes use settings from child queues.
    bf_tm_sched_shaper_prov_type_t q_prov;
    rc = bf_tm_sched_q_guaranteed_rate_get_default(
        dev, port, 0, pps, burst_size, rate, &q_prov);
  } else {
    bf_tm_sch_l1_defaults_t def;
    rc = bf_tm_sch_l1_get_defaults(dev, NULL, NULL, &def);
    if (BF_SUCCESS != rc) {
      return rc;
    }
    if (def.sch_l1_guaranteed_rate_pps_is_valid &&
        def.sch_l1_guaranteed_rate_burst_size_is_valid &&
        def.sch_l1_guaranteed_rate_is_valid) {
      *pps = def.sch_l1_guaranteed_rate_pps;
      *burst_size = def.sch_l1_guaranteed_rate_burst_size;
      *rate = def.sch_l1_guaranteed_rate;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }

  return (rc);
}

/*
 * Get L1 Node default shaping rate.
 * Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_l1_shaping_rate_get()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] l1_node       l1 node
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_shaping_rate_get_default(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *pps,
                                                    uint32_t *burst_size,
                                                    uint32_t *rate) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }
  BF_TM_INVALID_ARG(l1_node >= (g_tm_ctx[dev]->tm_cfg.l1_per_pg));

  if (!(TM_IS_TARGET_ASIC(dev)) &&
      (g_tm_ctx[dev]->tm_cfg.ports_per_pg) > l1_node) {
    // on SW Model 'default' L1 Nodes use settings from child queues.
    bf_tm_sched_shaper_prov_type_t q_prov;
    rc = bf_tm_sched_q_shaping_rate_get_default(
        dev, port, 0, pps, burst_size, rate, &q_prov);
  } else {
    bf_tm_sch_l1_defaults_t def;
    rc = bf_tm_sch_l1_get_defaults(dev, NULL, NULL, &def);
    if (BF_SUCCESS != rc) {
      return rc;
    }
    if (NULL != rate) {
      if (def.sch_l1_shaping_rate_is_valid) {
        *rate = def.sch_l1_shaping_rate;
      } else {
        return (BF_NOT_SUPPORTED);
      }
    }
    if (NULL != pps) {
      if (def.sch_l1_shaping_rate_pps_is_valid) {
        *pps = def.sch_l1_shaping_rate_pps;
      } else {
        return (BF_NOT_SUPPORTED);
      }
    }
    if (NULL != burst_size) {
      if (def.sch_l1_shaping_rate_burst_size_is_valid) {
        *burst_size = def.sch_l1_shaping_rate_burst_size;
      } else {
        return (BF_NOT_SUPPORTED);
      }
    }
  }

  return (rc);
}

/**
 * Get Ingress global max cell limit threshold. Returned limit are accounted
 * in terms of cells.
 *
 * Default : 256000
 *
 *
 * @param[in] dev         ASIC device identifier.
 * @param[out] cells      Ingress global limit in cells.
 * @return                BF_SUCCESS on success
 *                        Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_get(bf_dev_id_t dev, uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(cells == NULL);

  rc = bf_tm_ig_gpool_get_glb_cell_limit(
      dev, g_tm_ctx[dev]->ig_pool, cells, cells);
  return (rc);
}

/**
 * Get Default value of Ingress global max cell limit. Returned limit are
 * accounted in terms of cells.
 *
 * Default : 256000
 *
 *
 * @param[in] dev         ASIC device identifier.
 * @param[out] cells      Ingress global limit in cells.
 * @return                BF_SUCCESS on success
 *                        Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_get_default(bf_dev_id_t dev,
                                               uint32_t *cells) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(cells == NULL);

  bf_tm_pool_defaults_t def;
  rc = bf_tm_pool_defaults_get(dev, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }

  if (def.glb_max_cell_limit_is_valid) {
    *cells = def.glb_max_cell_limit;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}

/**
 * Get Ingress global max cell limit state. Returned state indicates
 * the Ingress global cell limit is enabled or disabled
 *
 * Default : Global cell limit is enabled by default
 *
 *
 * @param[in] dev         ASIC device identifier.
 * @param[out] state      Ingress global limit state.
 * @return                BF_SUCCESS on success
 *                        Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_state_get(bf_dev_id_t dev, bool *state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(state == NULL);

  rc = bf_tm_ig_gpool_get_glb_cell_limit_state(
      dev, g_tm_ctx[dev]->ig_pool, state, state);
  return (rc);
}

/**
 * Get default value of Ingress global max cell limit state.
 *
 * Default : Global cell limit is enabled by default
 *
 *
 * @param[in] dev         ASIC device identifier.
 * @param[out] state      Ingress global limit state.
 * @return                BF_SUCCESS on success
 *                        Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_state_get_default(bf_dev_id_t dev,
                                                     bool *state) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(state == NULL);

  bf_tm_pool_defaults_t def;
  rc = bf_tm_pool_defaults_get(dev, NULL, &def);
  if (BF_SUCCESS != rc) {
    return rc;
  }

  if (def.glb_max_cell_limit_en_is_valid) {
    *state = def.glb_max_cell_limit_en;
  } else {
    rc = BF_NOT_SUPPORTED;
  }

  return (rc);
}
