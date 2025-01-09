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
#include <mc_mgr/mc_mgr_intf.h>
#include "traffic_mgr/common/tm_ctx.h"

/**
 * Set port limit. When buffer usage accounted on port basis crosses the
 * limit, traffic is not admitted into traffic manager.
 *
 * Default: Set to 100% buffer usage. (286,720 cells for Tofino, 393,216 cells
 * for Tofino2).
 *
 * Related APIs: bf_tm_port_ingress_drop_limit_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Limits in terms of cells. The lowest 3 bits will be
 *                   lost, so the limit should be the module of 8 for the
 *                   correctness
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_limit_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_wac_drop_limit(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set port limit in QAC. When buffer usage accounted on port basis crosses the
 * limit, traffic Will be dropped on QAC stage.
 *
 * Default: Set to 100% buffer usage. (286,720 cells for Tofino, 393,216 cells
 * for Tofino2).
 *
 * Related APIs: bf_tm_port_egress_drop_limit_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Limits in terms of cells.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_limit_set(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_qac_drop_limit(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear port drop state register in WAC.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_wac_drop_state_clear(bf_dev_id_t dev,
                                            bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_wac_drop_state(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear port drop state register in QAC.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_qac_drop_state_clear(bf_dev_id_t dev,
                                            bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_qac_drop_state(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear port drop limit register in QAC.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_qac_drop_limit_clear(bf_dev_id_t dev,
                                            bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clr_qac_drop_limit(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set port hysteresis limits. When usage of cells goes below
 * hysteresis value  port pause or drop condition will be cleared.
 *
 * Default : 32 cells.
 *
 * Related APIs: bf_tm_port_ingress_drop_limit_set(),
 *               bf_tm_port_ingress_hysteresis_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Offset Limit. The lowest 3 bits will be
 *                   lost, so the limit should be the module of 8 for the
 *                   correctness
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_ingress_hysteresis_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  if (!TM_IS_8CELL_UNITS(cells)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        cells,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(cells)));
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_wac_hyst(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set port hysteresis limits. When usage of cells goes below
 * hysteresis value  port drop condition will be cleared.
 *
 * Default : 32 cells.
 *
 * Related APIs: bf_tm_port_egress_drop_limit_set(),
 *               bf_tm_port_egress_hysteresis_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param cells      Offset Limit. The lowest 3 bits will be
 *                   lost, so the limit should be the module of 8 for the
 *                   correctness
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_egress_hysteresis_set(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  if (!TM_IS_8CELL_UNITS(cells)) {
    LOG_WARN(
        "The requested value %d is not a multiple of 8 and will be set to HW "
        "rounded down value of %d",
        cells,
        TM_8CELL_UNITS_TO_CELLS(TM_CELLS_TO_8CELL_UNITS(cells)));
  }

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_qac_hyst(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * This API can be used to set cut through buffer size on per port basis.
 * The specified size is set aside for unicast traffic in cut through mode.
 *
 * Default : Set according to absorb TM processing cycle time.
 *
 * Related APIs: bf_tm_port_uc_cut_through_limit_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param cells      Size in terms of cells (upto 16). Valid value [1..15]
 *                   If size is set to zero, then cut through is disabled.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_uc_cut_through_limit_set(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                uint8_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  if (cells > g_tm_ctx[dev]->tm_cfg.uc_ct_max_cells) {
    return (BF_INVALID_ARG);
  }
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_uc_cut_through_limit(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * This API can be used to set type of Pause desired. When pause type is
 * BF_TM_PAUSE_PFC, then pfc is asserted. If type set to BF_TM_PAUSE_PORT
 * port pause is asserted. When pause is set to BF_TM_PAUSE_NONE, then
 * no pause or pfc is asserted.
 *
 * Related APIs: bf_tm_port_pfc_cos_mapping_set()
 *             : bf_tm_port_flowcontrol_rx_set()
 *
 * Default : No Pause or flow control assertion.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param type       Pause type.
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_port_flowcontrol_mode_set(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_flow_ctrl_type_t type)

{
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return BF_SUCCESS;
  }

  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_flowcontrol_mode(dev, p, type);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * This API can be used to set how port should react to pause assertion
 * from peer port. When pause type is BF_TM_PAUSE_PFC, then pfc is
 * asserted. If type set to BF_TM_PAUSE_PORT port will react to pause.
 * When pause is set to BF_TM_PAUSE_NONE, then pause or pfc is ignored
 * and traffic is still pushed to epipe and egress port.
 *
 * Related APIs: bf_tm_port_flowcontrol_mode_set()
 *
 * Default : Ignore pause and pfc.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port handle.
 * @param type       Pause type.
 * @return           Status of API call.
 *
 */
bf_status_t bf_tm_port_flowcontrol_rx_set(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_flow_ctrl_type_t type)

{
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_flowcontrol_rx(dev, p, type);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * When PFC level pause is desired, it is required to map internal
 * iCoS(iCoS = ig_intr_md.ingress_cos) to packet CoS. This API can
 * be used to set up the CoS mapping.
 *
 * Default: No PFC
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_set()
 *
 * @param dev         ASIC device identifier.
 * @param port        Port handle
 * @param cos_to_icos Array of 8 uint8_t values.
 *                    Array index is CoS and array value is iCoS.
 * @return            Status of API call.
 *
 */
bf_status_t bf_tm_port_pfc_cos_mapping_set(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           uint8_t *cos_to_icos) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(NULL == cos_to_icos);
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_pfc_cos_map(dev, p, cos_to_icos);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Set PFC state register for Port
 *
 * @param[in] dev     ASIC device identifier.
 * @param[in] port    Port id
 * @param[in] icos    iCOS (priority) PFC state bit number (0..7)
 * @param[in] state   iCOS (priority) PFC state bit value (true - 1, false - 0)
 * @return            Status of API call.
 *                    BF_SUCCESS on success
 *                    Non-Zero on error
 */
bf_status_t bf_tm_port_pfc_state_set(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_icos_t icos,
                                     bool state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *port_ptr;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_ICOS_INVALID(icos));
  rc = bf_tm_port_get_descriptor(dev, port, &port_ptr);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_pfc_state(dev, port_ptr, icos, state);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Clear PFC state register for Port
 *
 * @param[in] dev     ASIC device identifier.
 * @param[in] port    Port id
 * @return            Status of API call.
 *                    BF_SUCCESS on success
 *                    Non-Zero on error
 */
bf_status_t bf_tm_port_pfc_state_clear(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *port_ptr;
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &port_ptr);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_clear_pfc_state(dev, port_ptr);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * This API can be used to specify which one of the ports on a pipe is connected
 * to CPU. Copy to CPU packet uses the specified port. This function overrides
 * if a cpu port was already set.
 *
 * Default: No CPU port
 *
 * Related APIs: bf_tm_port_cpuport_reset()
 *
 * @param dev         ASIC device identifier.
 * @param port        Port handle
 * @return            Status of API call.
 *
 */
bf_status_t bf_tm_port_cpuport_set(bf_dev_id_t dev, bf_dev_port_t port) {
  // Mc-mgr implements copy to cpu port. Use that API here.
  // Application will see TM API to set copy-to-cpu port.
  return (bf_mc_set_copy_to_cpu(dev, true, port));
}

/**
 * This API can be used to specify no CPU port. Application can use this API
 * to indicate no cpu port or clear any previously set cpu port.
 *
 * Default: No CPU port
 *
 * Related APIs: bf_tm_port_cpuport_set()
 *
 * @param dev         ASIC device identifier.
 * @return            Status of API call.
 *
 */
bf_status_t bf_tm_port_cpuport_reset(bf_dev_id_t dev) {
  return (bf_mc_set_copy_to_cpu(dev, false, 0));
}

/**
 * Enable cut-through switching for a port in TM
 *
 * Default : Cut-through is disabled.
 *
 *NOTE:
 *    This API shouldn't be called directly as enabling/disabling
 *    cut-through is a two-step process (enable/disable in TM and
 *    enable/disable in ParDe EBUF register).
 *    This should be called through BF_PAL API only.
 *
 * Related APIs: bf_tm_port_cut_through_disable()
 *               bf_tm_port_cut_through_enable_status_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_cut_through_enable(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_enable_cut_through(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Disable cut-through switching for a port in TM
 *
 * Default : Cut-through is disabled.
 *
 *NOTE:
 *    This API shouldn't be called directly as enabling/disabling
 *    cut-through is a two-step process (enable/disable in TM and
 *    enable/disable in ParDe EBUF register).
 *    This should be called through BF_PAL API only.
 *
 * Related APIs: bf_tm_port_cut_through_enable()
 *               bf_tm_port_cut_through_enable_status_get()
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_cut_through_disable(bf_dev_id_t dev,
                                           bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_disable_cut_through(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/**
 * Use this API to flush all queues assigned to a port.
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @return           Status of API call.
 */

bf_status_t bf_tm_port_all_queues_flush(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_flush_all_queues(dev, p);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

/*
 * Set port skid limits. Cannot be increased beyond the size of
 * skid pool. If set to zero, in transit  traffic
 * from all lossless PPGs of the port will be dropped once pause/pfc
 * is asserted.
 *
 * Default : Skid limits are set to zero.
 *
 * Related APIs: bf_tm_ppg_skid_limit_get()
 *
 * param dev               ASIC device identifier.
 * param port              Port Identifier
 * param cells             Limits in terms of number of cells.
 * return                  Status of API call.
 */
bf_status_t bf_tm_port_skid_limit_set(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  if (!g_tm_ctx[dev]) {
    return rc;
  }
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  BF_TM_INVALID_ARG(TM_IS_THRES_INVALID(cells, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_skid_limit(dev, p, cells);
  TM_UNLOCK_AND_FLUSH(dev);

  return (rc);
}

/**
 * Set the qac rx state (enable or disable) for a port in TM
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier
 * @param state      Enable (true) or disable (false)
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_set_qac_rx(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bool state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(
      TM_IS_PORT_INVALID(DEV_PORT_TO_LOCAL_PORT(port), g_tm_ctx[dev]));
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  rc = bf_tm_port_set_qac_rx_state(dev, p, state);
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}
