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

#include <traffic_mgr/traffic_mgr.h>

#include "traffic_mgr/common/tm_ctx.h"

/**
 * Set the input FIFO arbitration mode to strict priority or weighted round
 * robin.  Note that if strict priority is enabled on a FIFO, all FIFOs higher
 * will also be enabled for strict priority.  For example, to set FIFO 1 as
 * strict priority, 2 and 3 must also be strict priority.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe_bmap         Pipe bit mask. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO to configure, must be 0, 1, 2 or 3.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[in] use_strict_pri    If @c true, use strict priority.  If @c false,
 *                              use weighted round robin.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_arb_mode_set(bf_dev_id_t dev,
                                       uint8_t pipe_bmap,
                                       int fifo,
                                       bool use_strict_pri) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPEMASK_INVALID(pipe_bmap));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_set_fifo_arb_mode(dev, pipe_bmap, fifo, use_strict_pri);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Set the input FIFO arbitration weights used by the weighted round robin
 * arbitration mode.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe_bmap         Pipe bit mask. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO to configure, must be 0, 1, 2 or 3.
 *                              Check ASIC capabilites to find maximum number of
 *                              fifos.
 * @param[in] weight            The weight assigned to FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_wrr_weight_set(bf_dev_id_t dev,
                                         uint8_t pipe_bmap,
                                         int fifo,
                                         uint8_t weight) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPEMASK_INVALID(pipe_bmap));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_set_fifo_wrr_weight(dev, pipe_bmap, fifo, weight);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Set multicast fifo to iCoS mapping.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe_bmap         Pipe bit mask. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO to configure, must be 0, 1, 2 or 3.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[in] icos_bmap         iCoS bit map.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_icos_mapping_set(bf_dev_id_t dev,
                                           uint8_t pipe_bmap,
                                           int fifo,
                                           uint8_t icos_bmap) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPEMASK_INVALID(pipe_bmap));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_set_fifo_icos_mapping(dev, pipe_bmap, fifo, icos_bmap);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get multicast fifo to iCoS mapping.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe
 * @param[out] icos_bmap        iCoS bit map.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_icos_mapping_get(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           int fifo,
                                           uint8_t *icos_bmap) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(NULL == icos_bmap);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_get_fifo_icos_mapping(dev, pipe, fifo, icos_bmap, icos_bmap);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get default multicast fifo to iCoS mapping.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe
 * @param[out] icos_bmap        iCoS bit map.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_icos_mapping_get_default(bf_dev_id_t dev,
                                                   bf_dev_pipe_t pipe,
                                                   int fifo,
                                                   uint8_t *icos_bmap) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(NULL == icos_bmap);

  if (fifo == 0) {
    *icos_bmap = 0xff;
  } else {
    *icos_bmap = 0;
  }

  return (rc);
}

/**
 * Set the input FIFO depth.  Sum of all four sizes cannot exceed 8192,
 * additionally, each size must be a multiple of 8.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe_bmap         Pipe bit mask. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO to configure, must be 0, 1, 2 or 3.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[in] size              The size assigned to FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_depth_set(bf_dev_id_t dev,
                                    uint8_t pipe_bmap,
                                    int fifo,
                                    int size) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPEMASK_INVALID(pipe_bmap));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_set_fifo_depth(dev, pipe_bmap, fifo, size);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get the input FIFO arbitration mode to strict priority or weighted round
 * robin.  Note that if strict priority is enabled on a FIFO, all FIFOs higher
 * must also be enabled for strict priority.  For example, to set FIFO 1 as
 * strict priority, 2 and 3 must also be strict priority.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] use_strict_pri   If @c true, arbitration mode is strict priority.
 *                              If @c false, arbitration is weighted round
 *                              robin.
 * @return Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_arb_mode_get(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       int fifo,
                                       bool *use_strict_pri) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_get_fifo_arb_mode(dev, pipe, fifo, use_strict_pri, NULL);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get the default input FIFO arbitration mode to strict priority or weighted
 * round robin.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] use_strict_pri   If @c true, arbitration mode is strict priority.
 *                              If @c false, arbitration is weighted round
 *                              robin.
 * @return Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_arb_mode_get_default(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               int fifo,
                                               bool *use_strict_pri) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  *use_strict_pri = false;

  return (rc);
}

/**
 * Get the input FIFO arbitration weights used by the weighted round robin
 * arbitration mode.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] weight           The weight assigned to FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_wrr_weight_get(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         int fifo,
                                         uint8_t *weight) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_get_fifo_wrr_weight(dev, pipe, fifo, weight, NULL);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get the default input FIFO arbitration weights used by the weighted round
 * robin arbitration mode.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] weight           The weight assigned to FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_wrr_weight_get_default(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 int fifo,
                                                 uint8_t *weight) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  *weight = 0;

  return (rc);
}

/**
 * Get the input FIFO depth.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] size             The size assigned to specified FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_depth_get(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    int fifo,
                                    int *size) {
  bf_status_t rc = BF_SUCCESS;
  uint16_t depth;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_mcast_get_fifo_depth(dev, pipe, fifo, &depth, NULL);
  *size = depth;
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Get the default input FIFO depth.
 *
 * @param[in] dev               The ASIC id.
 * @param[in] pipe              Pipe number. Check ASIC manual to find maximum
 *                              of pipes.
 * @param[in] fifo              The FIFO id.
 *                              Check ASIC manual to find maximum number of
 *                              fifos per pipe.
 * @param[out] size             The size assigned to specified FIFO.
 * @return                      Status of the API call.
 */
bf_status_t bf_tm_mc_fifo_depth_get_default(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            int fifo,
                                            int *size) {
  bf_status_t rc = BF_SUCCESS;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_MCFIFO_INVALID(fifo, g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_PIPE_INVALID(pipe, g_tm_ctx[dev]));

  *size = 0;

  return (rc);
}
