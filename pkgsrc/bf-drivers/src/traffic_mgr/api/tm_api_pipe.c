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
 * Set egress pipe limit.  Default value of the pipe limit
 * is set to maximum buffering capability of the traffic manager.
 * When admitting packet into Traffic manager, apart from other
 * checks, the packet has to also pass usage check on per egress pipe
 * usage limit. A packet destined to egress pipe whose usage limit
 * has crossed, will not be admitted.
 *
 * Default: Set to maximum TM buffer.
 *
 * Related APIs: bf_tm_pipe_egress_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier
 * @param cells      Limit in terms of number of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_egress_limit_set(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_set_limit(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set pipe hysteresis limit.
 * When usage of cells goes below the hysteresis limit, pipe level
 * drop condition will be cleared. Single hysteresis value is used
 * for all pipes of the ASIC.
 *
 * Default : Hysteresis is set to zero or no hysteresis.
 *
 * Related APIs: bf_tm_pipe_egress_limit_set(),
 *               bf_tm_pipe_egress_hysteresis_get()
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier
 * @param cells      New threshold limit
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_egress_hysteresis_set(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_pipe_set_hyst(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set Egress Pipe Queue Stats Reporting (QSTAT) mode
 * Only QSTAT visible queues at the pipe are participating in reporting.
 * False: Trigger QSTAT reporting on Q color threshold crosses.
 * True: Trigger QSTAT reporting on any Q depth updates.
 *
 * Default: 0
 *
 * Related APIs: bf_tm_q_visible_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @param[in] mode       Queue statistics reporting mode.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_qstat_report_mode_set(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bool mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_set_qstat_report_mode(dev, p, mode);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/**
 * Get Egress Pipe Queue Stats Reporting (QSTAT) default mode
 * Only QSTAT visible queues at the pipe are participating in reporting.
 * False: Trigger QSTAT reporting of Q color threshold crosses.
 * True: Trigger QSTAT reporting of any Q depth updates.
 *
 * Related APIs: bf_tm_qstat_report_mode_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @param[out] mode      Queue statistics default reporting mode.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_qstat_report_mode_get_default(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bool *mode) {
  (void)pipe;
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == mode))
  if (NULL == g_tm_ctx[dev]) {
    return (BF_UNEXPECTED);
  }

  bf_tm_pipe_defaults_t def;
  rc = bf_tm_pipe_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc) {
    if (def.qstat_report_mode_is_valid) {
      *mode = def.qstat_report_mode;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }

  return rc;
}

/**
 * Get Egress Pipe Queue Stats Reporting (QSTAT) mode
 * Only QSTAT visible queues at the pipe are participating in reporting.
 * False: Trigger QSTAT reporting of Q color threshold crosses.
 * True: Trigger QSTAT reporting of any Q depth updates.
 *
 * Default: 0
 *
 * Related APIs: bf_tm_q_visible_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @param[out] mode      Queue statistics reporting mode.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_qstat_report_mode_get(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bool *mode) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == mode))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_get_qstat_report_mode(dev, p, mode);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/**
 * Get egress Pipe deflection port enable mode
 *
 * Default: true
 *
 * Related APIs: bf_tm_pipe_deflection_port_enable_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] pipe        Pipe Identifier.
 * @param[out] enable     Deflection mode enable status.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_deflection_port_enable_get(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bool *enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_get_deflection_port_enable(dev, p, enable);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}

/**
 * Get egress Pipe deflection port default enable mode.
 *
 * Default: true
 *
 * Related APIs: bf_tm_pipe_deflection_port_enable_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] pipe        Pipe Identifier.
 * @param[out] enable     Deflection mode default enable status.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_deflection_port_enable_get_default(bf_dev_id_t dev,
                                                          bf_dev_pipe_t pipe,
                                                          bool *enable) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG((NULL == enable))
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  bf_tm_pipe_defaults_t def;
  rc = bf_tm_pipe_get_defaults(dev, NULL, &def);
  if (BF_SUCCESS == rc) {
    if (def.mirror_drop_enable_is_valid) {
      *enable = def.mirror_drop_enable;
    } else {
      rc = BF_NOT_SUPPORTED;
    }
  }

  return rc;
}

/**
 * Set egress Pipe deflection port enable mode
 *
 * Default: true
 *
 * Related APIs: bf_tm_pipe_deflection_port_enable_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @param[in] enable     Deflection mode enable status.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_deflection_port_enable_set(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe,
                                                  bool enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_eg_pipe_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  rc = bf_tm_pipe_get_descriptor(dev, pipe, &p);
  if (rc != BF_SUCCESS) return (rc);
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_set_deflection_port_enable(dev, p, enable);
  TM_UNLOCK_AND_FLUSH(dev);
  return rc;
}
