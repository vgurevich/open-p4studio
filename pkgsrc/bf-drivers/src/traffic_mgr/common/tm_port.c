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


#include "tm_ctx.h"
#include "traffic_mgr/hw_intf/tm_tofino_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof2_hw_intf.h"
#include "traffic_mgr/hw_intf/tm_tof3_hw_intf.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <pipe_mgr/pktgen_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

static bf_tm_port_hw_funcs_tbl g_port_hw_fptr_tbl;

bf_status_t bf_tm_port_get_descriptor(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_port_t **p) {
  bf_status_t rc = BF_SUCCESS;
  *p = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
  return (rc);
}

void bf_tm_port_set_q_profile(bf_dev_id_t dev,
                              bf_dev_port_t port,
                              int q_profile) {
  (BF_TM_PORT_PTR(g_tm_ctx[dev], port))->qid_profile = q_profile;
}

void bf_tm_port_get_q_profile(bf_dev_id_t dev,
                              bf_dev_port_t port,
                              int *q_profile) {
  *q_profile = (BF_TM_PORT_PTR(g_tm_ctx[dev], port))->qid_profile;
}

void bf_tm_ports_delete(bf_tm_dev_ctx_t *tm_ctx) {
  if (tm_ctx->ports) {
    bf_sys_free(tm_ctx->ports);
  }
}

// Set PPGs that belong to the port. When non default PPGs
// are in use, they map traffic of certain icos/s. Such PPGs are
// identified by icos_bmap. Multiple icos traffic can map to single PPG.
void bf_tm_port_set_icos_ppg_mapping(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint16_t icos_bmap,
                                     bf_tm_ppg_t *ppg) {
  bf_tm_port_t *p;
  int i;

  p = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.icos_count; i++) {
    if (icos_bmap & (1 << i)) {
      p->ppgs[i] = ppg;
    }
  }
}

void bf_tm_port_remove_icos_mapped_to_default_ppg(
    bf_dev_id_t dev, bf_dev_port_t port, uint16_t default_ppg_icos_bmap) {
  bf_tm_port_t *p;
  int i;

  p = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.icos_count; i++) {
    if (default_ppg_icos_bmap & (1 << i)) {
      if (p->ppgs[i] && !(p->ppgs[i]->is_default_ppg)) {
        // A non deafult PPG is mapped to iCoS that is currently
        // being remapped to default PPG.

        // remove iCoS mask from the PPG
        p->ppgs[i]->ppg_cfg.icos_mask &= (~(1 << i));
        p->ppgs[i] = NULL;  // For the iCoS that is mapped default PPG, set PPG
                            // pointer to NULL.
      }
    }
  }
}

bf_tm_status_t bf_tm_port_set_wac_drop_limit(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->wac_drop_limit, g_tm_ctx[dev])) {
    return (rc);
  }
  p->wac_drop_limit = cells;
  if (g_port_hw_fptr_tbl.port_wac_drop_limit_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_wac_drop_limit_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_qac_drop_limit(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->qac_drop_limit, g_tm_ctx[dev])) {
    return (rc);
  }
  p->qac_drop_limit = cells;
  if (g_port_hw_fptr_tbl.port_qac_drop_limit_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_qac_drop_limit_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_wac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (!g_port_hw_fptr_tbl.port_wac_hyst_wr_fptr) {
    return BF_NOT_SUPPORTED;
  }

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->wac_resume_limit, g_tm_ctx[dev])) {
    return (rc);
  }

  p->wac_resume_limit = cells;
  rc = g_port_hw_fptr_tbl.port_wac_hyst_wr_fptr(dev, p);
  // p->wac_hyst_index is also updated on success

  return (rc);
}

bf_tm_status_t bf_tm_port_set_qac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->qac_resume_limit, g_tm_ctx[dev])) {
    return (rc);
  }
  p->qac_resume_limit = cells;
  if (g_port_hw_fptr_tbl.port_qac_hyst_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_qac_hyst_wr_fptr(dev, p);
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_set_uc_cut_through_limit(bf_dev_id_t dev,
                                                   bf_tm_port_t *p,
                                                   uint8_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->uc_cut_through_limit, g_tm_ctx[dev])) {
    return (rc);
  }
  p->uc_cut_through_limit = cells;
  if (g_port_hw_fptr_tbl.port_uc_ct_limit_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_uc_ct_limit_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_flowcontrol_mode(bf_dev_id_t dev,
                                               bf_tm_port_t *p,
                                               bf_tm_flow_ctrl_type_t type) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(type, p->fc_type, g_tm_ctx[dev])) {
    return (rc);
  }
  p->fc_type = type;
  if (g_port_hw_fptr_tbl.port_flowcontrol_mode_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_flowcontrol_mode_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_flowcontrol_rx(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             bf_tm_flow_ctrl_type_t type) {
  bf_status_t rc = BF_SUCCESS;

  p->fc_rx_type = type;
  if (g_port_hw_fptr_tbl.port_flowcontrol_rx_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_flowcontrol_rx_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_pfc_cos_map(bf_dev_id_t dev,
                                          bf_tm_port_t *p,
                                          uint8_t *cos_to_icos) {
  bf_status_t rc = BF_SUCCESS;
  int i;
  bool match = true;

  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.icos_count && match; i++) {
    match = TM_HITLESS_IS_CFG_MATCH(
        cos_to_icos[i], p->cos_to_icos[i], g_tm_ctx[dev]);
  }
  if (match) {
    return (rc);
  }
  if (g_port_hw_fptr_tbl.port_pfc_cos_map_wr_fptr) {
    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.icos_count; i++) {
      p->cos_to_icos[i] = cos_to_icos[i];
    }
    rc = g_port_hw_fptr_tbl.port_pfc_cos_map_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_cpu_port(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  rc = g_port_hw_fptr_tbl.cpu_port_wr_fptr(dev, p);

  return (rc);
}

bf_tm_status_t bf_tm_port_set_skid_limit(bf_dev_id_t dev,
                                         bf_tm_port_t *p,
                                         uint32_t cells) {
  bf_status_t rc = BF_SUCCESS;

  if (TM_HITLESS_IS_CFG_MATCH(cells, p->skid_limit, g_tm_ctx[dev])) {
    return (rc);
  }
  p->skid_limit = cells;
  if (g_port_hw_fptr_tbl.port_skid_limit_wr_fptr) {
    rc = g_port_hw_fptr_tbl.port_skid_limit_wr_fptr(dev, p);
  }
  return (rc);
}

bf_status_t bf_tm_port_enable_cut_through(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t mac_blk, channel;
  lld_err_t lld_err;
  bf_dev_port_t port = MAKE_DEV_PORT(p->l_pipe, p->uport);

  /* This API shouldn't be called for internal ports */
  lld_err = lld_sku_map_dev_port_id_to_mac_ch(dev, port, &mac_blk, &channel);
  if (lld_err != LLD_OK) {
    /*
     * This means internal ports as these ports don't have MAC,
     * return error.
     */
    return (BF_INVALID_ARG);
  }

  if (TM_HITLESS_IS_CFG_MATCH(p->ct_enabled, true, g_tm_ctx[dev])) {
    return (rc);
  }

  if (g_port_hw_fptr_tbl.port_cut_through_wr_fptr) {
    p->ct_enabled = true;
    rc = g_port_hw_fptr_tbl.port_cut_through_wr_fptr(dev, p);
  }

  return (rc);
}

bf_status_t bf_tm_port_disable_cut_through(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t mac_blk, channel;
  lld_err_t lld_err;
  bf_dev_port_t port = MAKE_DEV_PORT(p->l_pipe, p->uport);

  /* This API shouldn't be called for internal ports */
  lld_err = lld_sku_map_dev_port_id_to_mac_ch(dev, port, &mac_blk, &channel);
  if (lld_err != LLD_OK) {
    /*
     * This means internal ports as these ports don't have MAC,
     * return error.
     */
    return (BF_INVALID_ARG);
  }

  if (TM_HITLESS_IS_CFG_MATCH(p->ct_enabled, false, g_tm_ctx[dev])) {
    return (rc);
  }

  if (g_port_hw_fptr_tbl.port_cut_through_wr_fptr) {
    p->ct_enabled = false;
    rc = g_port_hw_fptr_tbl.port_cut_through_wr_fptr(dev, p);
  }

  return (rc);
}

bf_status_t bf_tm_port_set_qac_rx_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        bool enable) {
  bf_status_t rc = BF_SUCCESS;
  uint32_t mac_blk, channel;
  lld_err_t lld_err;
  bf_dev_port_t port = MAKE_DEV_PORT(p->l_pipe, p->uport);

  /* This API shouldn't be called for internal ports */
  lld_err = lld_sku_map_dev_port_id_to_mac_ch(dev, port, &mac_blk, &channel);
  if (lld_err != LLD_OK) {
    /* This means internal ports as these ports don't have MAC */
    return (BF_SUCCESS);
  }

  if (TM_HITLESS_IS_CFG_MATCH(p->qac_rx_enable, enable, g_tm_ctx[dev])) {
    return (rc);
  }

  if (g_port_hw_fptr_tbl.port_qac_rx_wr_fptr) {
    p->qac_rx_enable = enable;
    rc = g_port_hw_fptr_tbl.port_qac_rx_wr_fptr(dev, p);
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_wac_drop_limit(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint32_t *sw_limit,
                                             uint32_t *hw_limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_limit = p->wac_drop_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_limit &&
      g_port_hw_fptr_tbl.port_wac_drop_limit_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_wac_drop_limit_rd_fptr(dev, &out_p);
    *hw_limit = out_p.wac_drop_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_qac_drop_limit(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint32_t *sw_limit,
                                             uint32_t *hw_limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_limit = p->qac_drop_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_limit &&
      g_port_hw_fptr_tbl.port_qac_drop_limit_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_qac_drop_limit_rd_fptr(dev, &out_p);
    *hw_limit = out_p.qac_drop_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clr_qac_drop_limit(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  if (TM_IS_TARGET_ASIC(dev) && g_port_hw_fptr_tbl.clr_qac_drop_lmt_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.clr_qac_drop_lmt_fptr(dev, &out_p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_wac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t *sw_hyst,
                                       uint32_t *hw_hyst) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  if (!g_port_hw_fptr_tbl.port_wac_hyst_rd_fptr) {
    return BF_NOT_SUPPORTED;
  }

  *sw_hyst = p->wac_resume_limit;

  if (TM_IS_TARGET_ASIC(dev) && hw_hyst) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_wac_hyst_rd_fptr(dev, &out_p);
    *hw_hyst = out_p.wac_resume_limit;
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_qac_hyst(bf_dev_id_t dev,
                                       bf_tm_port_t *p,
                                       uint32_t *sw_hyst,
                                       uint32_t *hw_hyst) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_hyst = p->qac_resume_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_hyst &&
      g_port_hw_fptr_tbl.port_qac_hyst_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_qac_hyst_rd_fptr(dev, &out_p);
    *hw_hyst = out_p.qac_resume_limit;
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_skid_limit(bf_dev_id_t dev,
                                         bf_tm_port_t *p,
                                         uint32_t *sw_limit,
                                         uint32_t *hw_limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  BF_TM_INVALID_ARG((NULL == p));
  BF_TM_INVALID_ARG((NULL == sw_limit));

  *sw_limit = p->skid_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_limit &&
      g_port_hw_fptr_tbl.port_skid_limit_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_skid_limit_rd_fptr(dev, &out_p);
    *hw_limit = out_p.skid_limit;
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_wac_hyst_index(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint8_t *sw_hyst_index,
                                             uint8_t *hw_hyst_index) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  if (!g_port_hw_fptr_tbl.port_wac_hyst_rd_fptr) {
    return BF_NOT_SUPPORTED;
  }

  *sw_hyst_index = p->wac_hyst_index;

  if (TM_IS_TARGET_ASIC(dev) && hw_hyst_index) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_wac_hyst_rd_fptr(dev, &out_p);
    *hw_hyst_index = out_p.wac_hyst_index;
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_qac_hyst_index(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             uint8_t *sw_hyst_index,
                                             uint8_t *hw_hyst_index) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_hyst_index = p->qac_hyst_index;
  if (TM_IS_TARGET_ASIC(dev) && hw_hyst_index &&
      g_port_hw_fptr_tbl.port_qac_hyst_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_qac_hyst_rd_fptr(dev, &out_p);
    *hw_hyst_index = out_p.qac_hyst_index;
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_uc_cut_through_limit(bf_dev_id_t dev,
                                                   bf_tm_port_t *p,
                                                   bf_tm_thres_t *sw_limit,
                                                   bf_tm_thres_t *hw_limit) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_limit = p->uc_cut_through_limit;
  if (TM_IS_TARGET_ASIC(dev) && hw_limit &&
      g_port_hw_fptr_tbl.port_uc_ct_limit_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_uc_ct_limit_rd_fptr(dev, &out_p);
    *hw_limit = out_p.uc_cut_through_limit;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_flowcontrol_mode(bf_dev_id_t dev,
                                               bf_tm_port_t *p,
                                               bf_tm_flow_ctrl_type_t *swtype,
                                               bf_tm_flow_ctrl_type_t *hwtype) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t _p;

  *swtype = p->fc_type;
  if (hwtype && g_port_hw_fptr_tbl.port_flowcontrol_mode_rd_fptr) {
    memcpy(&_p, p, sizeof(_p));
    rc = g_port_hw_fptr_tbl.port_flowcontrol_mode_rd_fptr(dev, &_p);
    *hwtype = _p.fc_type;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_flowcontrol_rx(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             bf_tm_flow_ctrl_type_t *swtype,
                                             bf_tm_flow_ctrl_type_t *hwtype) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  BF_TM_INVALID_ARG((NULL == p));
  BF_TM_INVALID_ARG((NULL == swtype));

  *swtype = p->fc_rx_type;
  if (TM_IS_TARGET_ASIC(dev) && hwtype &&
      g_port_hw_fptr_tbl.port_flowcontrol_rx_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_flowcontrol_rx_rd_fptr(dev, &out_p);
    *hwtype = out_p.fc_rx_type;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pfc_cos_map(bf_dev_id_t dev,
                                          bf_tm_port_t *p,
                                          uint8_t *sw_cos_to_icos) {
  bf_status_t rc = BF_SUCCESS;
  int i;

  for (i = 0; i < g_tm_ctx[dev]->tm_cfg.icos_count; i++) {
    sw_cos_to_icos[i] = p->cos_to_icos[i];
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_pfc_cos_mask(bf_dev_id_t dev,
                                           bf_tm_port_t *p,
                                           uint8_t *sw_icos_to_cos_mask,
                                           uint8_t *hw_icos_to_cos_mask) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_icos_to_cos_mask = p->icos_to_cos_mask;

  if (hw_icos_to_cos_mask && g_port_hw_fptr_tbl.port_pfc_cos_bvec_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    rc = g_port_hw_fptr_tbl.port_pfc_cos_bvec_rd_fptr(dev, &out_p);
    if (rc == BF_SUCCESS) {
      *hw_icos_to_cos_mask = out_p.icos_to_cos_mask;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pfc_enabled_cos(bf_dev_id_t dev,
                                              bf_tm_port_t *p,
                                              uint8_t cos,
                                              bool *sw_enabled,
                                              bool *hw_enabled) {
  bf_status_t rc = BF_SUCCESS;
  BF_TM_INVALID_ARG((NULL == p));
  BF_TM_INVALID_ARG((NULL == sw_enabled));
  BF_TM_INVALID_ARG((NULL == hw_enabled));

  bf_tm_port_t out_p;

  *sw_enabled = (bool)((p->icos_to_cos_mask) & (1 << cos));
  if (g_port_hw_fptr_tbl.port_pfc_enabled_cos_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));
    uint8_t en_mask = 0;
    rc = g_port_hw_fptr_tbl.port_pfc_enabled_cos_rd_fptr(dev, &out_p, &en_mask);
    if (rc == BF_SUCCESS) {
      *hw_enabled = (bool)(en_mask & (1 << cos));
    }
  } else {
    return BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_status_t bf_tm_port_get_cut_through_enable_status(bf_dev_id_t dev,
                                                     bf_tm_port_t *p,
                                                     bool *sw_ct_enabled,
                                                     bool *hw_ct_enabled) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t out_p;

  *sw_ct_enabled = p->ct_enabled;
  if (TM_IS_TARGET_ASIC(dev) && hw_ct_enabled &&
      g_port_hw_fptr_tbl.port_cut_through_rd_fptr) {
    memcpy(&out_p, p, sizeof(out_p));

    rc = g_port_hw_fptr_tbl.port_cut_through_rd_fptr(dev, &out_p);
    if (rc == BF_SUCCESS) {
      *hw_ct_enabled = out_p.ct_enabled;
    }
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_ingress_drop_counter(bf_dev_id_t dev,
                                                   bf_tm_port_t *p,
                                                   uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_drop_cntr_fptr) {
    rc = g_port_hw_fptr_tbl.ingress_drop_cntr_fptr(dev, p, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_wac_drop_state(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             bool *state) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.wac_drop_state_get_fptr) {
    rc = g_port_hw_fptr_tbl.wac_drop_state_get_fptr(dev, p, state);
  } else {
    return (BF_NOT_SUPPORTED);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_wac_drop_state_ext(bf_dev_id_t dev,
                                                 bf_tm_port_t *p,
                                                 bool *shr_lmt_state,
                                                 bool *hdr_lmt_state) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.wac_drop_state_get_fptr) {
    rc = g_port_hw_fptr_tbl.wac_drop_state_ext_get_fptr(
        dev, p, shr_lmt_state, hdr_lmt_state);
  } else {
    return (BF_NOT_SUPPORTED);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_wac_drop_state(bf_dev_id_t dev,
                                               bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.clr_wac_drop_state_fptr) {
    rc = g_port_hw_fptr_tbl.clr_wac_drop_state_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_qac_drop_state(bf_dev_id_t dev,
                                             bf_tm_port_t *p,
                                             bool *state) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.qac_drop_state_get_fptr) {
    rc = g_port_hw_fptr_tbl.qac_drop_state_get_fptr(dev, p, state);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_qac_drop_state(bf_dev_id_t dev,
                                               bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.clr_qac_drop_state_fptr) {
    rc = g_port_hw_fptr_tbl.clr_qac_drop_state_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_egress_drop_counter(bf_dev_id_t dev,
                                                  bf_tm_port_t *p,
                                                  uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.egress_drop_cntr_fptr) {
    rc = g_port_hw_fptr_tbl.egress_drop_cntr_fptr(dev, p, count);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_ingress_drop_counter(bf_dev_id_t dev,
                                                     bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_drop_cntr_clear_fptr) {
    rc = g_port_hw_fptr_tbl.ingress_drop_cntr_clear_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_egress_drop_counter(bf_dev_id_t dev,
                                                    bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.egress_drop_cntr_clear_fptr) {
    rc = g_port_hw_fptr_tbl.egress_drop_cntr_clear_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_ingress_usage_counter(bf_dev_id_t dev,
                                                    bf_tm_port_t *p,
                                                    uint32_t *count) {
  uint64_t cnt;
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_usage_cntr_fptr) {
    rc = g_port_hw_fptr_tbl.ingress_usage_cntr_fptr(dev, p, &cnt);
    *count = (uint32_t)cnt;  // Upper 32bits will not be relevant. 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_ingress_usage_counter(bf_dev_id_t dev,
                                                      bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_usage_cntr_wr_fptr) {
    rc = g_port_hw_fptr_tbl.ingress_usage_cntr_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_egress_usage_counter(bf_dev_id_t dev,
                                                   bf_tm_port_t *p,
                                                   uint32_t *count) {
  uint64_t cnt;
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.egress_usage_cntr_fptr) {
    g_port_hw_fptr_tbl.egress_usage_cntr_fptr(dev, p, &cnt);
    *count = (uint32_t)cnt;  // Upper 32bits will not be relevant. 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_egress_usage_counter(bf_dev_id_t dev,
                                                     bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.egress_usage_cntr_wr_fptr) {
    rc = g_port_hw_fptr_tbl.egress_usage_cntr_wr_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_ingress_water_mark(bf_dev_id_t dev,
                                                 bf_tm_port_t *p,
                                                 uint32_t *count) {
  uint64_t cnt;
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_wm_cntr_fptr) {
    g_port_hw_fptr_tbl.ingress_wm_cntr_fptr(dev, p, &cnt);
    *count = (uint32_t)cnt;  // Upper 32bits will not be relevant. 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_egress_water_mark(bf_dev_id_t dev,
                                                bf_tm_port_t *p,
                                                uint32_t *count) {
  uint64_t cnt;
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.egress_wm_cntr_fptr) {
    g_port_hw_fptr_tbl.egress_wm_cntr_fptr(dev, p, &cnt);
    *count = (uint32_t)cnt;  // Upper 32bits will not be relevant. 32b counter
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_ingress_water_mark(bf_dev_id_t dev,
                                                   bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.ingress_wm_clear_fptr) {
    rc = g_port_hw_fptr_tbl.ingress_wm_clear_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_egress_water_mark(bf_dev_id_t dev,
                                                  bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;

  if (g_port_hw_fptr_tbl.egress_wm_clear_fptr) {
    rc = g_port_hw_fptr_tbl.egress_wm_clear_fptr(dev, p);
  }
  return (rc);
}

bf_tm_status_t bf_tm_add_new_port(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_port_speeds_t speed) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  uint32_t mac_blk, channel;
  lld_err_t lld_err;

  // Port will be added in hitless HA case.
  // 1. Check if config is restored. Do not use scratch reg as read operation
  // will hamper
  //    cold boot/fast-reconfig. Ideally if we know driver is in warm_init mode,
  //    then
  //    consider reading scratch register (bit indicating TM was previously
  //    cfged successfully).
  // 2. If cfg restored, compare restored config value and new port value
  // 3. If change in port cfg, push the change to asic.
  // 4. make port online regardless. Its not possible to restore online status
  //    as it is not stored as TM cfg in chip. Same goes for ct_enabled, speed.
  //    honor new value for ct_enabled, speed.
  // 5. For port rate, honor new rate value if restored rate is not same as new
  // rate.

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  if (g_port_hw_fptr_tbl.port_add) {
    TM_LOCK(dev, g_tm_ctx[dev]->lock);
    p->offline = false;
    // Disable cut-through by default
    p->ct_enabled = false;
    p->speed = speed;
    p->speed_on_add = speed;  // Keep this value for reset purposes.

    /*
     * For ports that don't have MAC (CPU PCIe port and special internal
     * ports - recirculation ports) and for ports that have MAC but
     * recirculation enabled, qac rx gets enabled during
     * port-add (before port-enable) and qac rx gets disabled during
     * port-delete or port-disable.
     *
     * For ports that have MAC (front panel ports, CPU MAC ports and
     * internal ports on 32Q) and have recirculation disabled, qac rx
     * have enabled only during port-enable as enabling qac rx
     * during port-add and traffic already running will
     * cause queue stuck due to MAC not auto draining. Also, qac_rx gets
     * disabled during port-disable to avoid buffer usage for disabled ports.
     *
     */
    lld_err = lld_sku_map_dev_port_id_to_mac_ch(dev, port, &mac_blk, &channel);
    if (lld_err != LLD_OK) {
      /* Port doesn't have MAC, enable port and qac_rx */
      LOG_TRACE(
          "Enabling port and QAC rx during port-add for dev %d, dev_port %d",
          dev,
          port);
      p->admin_state = true;
      p->qac_rx_enable = true;
      p->has_mac = false;
    } else {
      p->has_mac = true;
      /* Port has MAC, check recirculation enable status */
      rc = bf_recirculation_get(dev, port, &(p->recirc_enable));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "%s: Not able to get recirc enable status for dev %d, port %d, "
            "rc = %s (%d)",
            __func__,
            dev,
            port,
            bf_err_str(rc),
            rc);
        TM_UNLOCK_AND_FLUSH(dev);
        return (rc);
      }

      /* Enable qac_rx if recirculation is enabled */
      if (p->recirc_enable) {
        LOG_TRACE(
            "Enabling QAC rx during port-add for recirculation enabled port, "
            "dev %d, dev_port %d",
            dev,
            port);
        p->qac_rx_enable = true;
      }
    }

    rc = bf_tm_port_set_cache_counters(dev, port);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "%s: Not able to allocate Cache Counters for dev %d, port %d, "
          "rc = %s (%d)",
          __func__,
          dev,
          port,
          bf_err_str(rc),
          rc);
      TM_UNLOCK_AND_FLUSH(dev);
      return (rc);
    }

    if (TM_HITLESS_WARM_INIT_IN_PROGRESS(dev)) {
      /* For hitless HA, return */
      TM_UNLOCK_AND_FLUSH(dev);
      return (rc);
    }

    switch (speed) {
      case BF_SPEED_1G:
        p->port_rate = 1000000;  // In kilo
        p->uc_cut_through_limit = 4;
        break;
      case BF_SPEED_10G:
        p->port_rate = 10000000;
        p->uc_cut_through_limit = 6;  // ct limit as per TOFLAB-36
        break;
      case BF_SPEED_25G:
        p->port_rate = 25000000;
        p->uc_cut_through_limit = 0xc;  // ct limit as per TOFLAB-36
        break;
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
        p->port_rate = 40000000;
        p->uc_cut_through_limit = 0xc;  // ct limit as per TOFLAB-36
        break;
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        p->port_rate = 50000000;
        p->uc_cut_through_limit = 0xc;  // ct limit as per TOFLAB-36
        break;
      case BF_SPEED_100G:
        if (!p->cfg_restored) {
          p->port_rate = 100000000;
          p->uc_cut_through_limit = 0xf;  // ct limit as per TOFLAB-36
        }
        break;
      case BF_SPEED_400G:
        if (!p->cfg_restored) {
          p->port_rate = 400000000;
          p->uc_cut_through_limit = 0xf;  // ct limit as per TOFLAB-36
        }
        break;
      default:
        p->port_rate = 100000000;
        p->uc_cut_through_limit = 0xf;
        break;
    }
    p->burst_size = 9216;  // Set deafult burst size to 9216 Bytes
    p->pps = false;  // Shaper by deafult is set to shape on bytes not packets.
    // When port is added, disable generation and honoring of PFC.
    p->fc_type = BF_TM_PAUSE_NONE;
    p->fc_rx_type = BF_TM_PAUSE_NONE;
    // need to set fc mode in the HW
    bf_tm_port_set_flowcontrol_mode(dev, p, p->fc_type);
    // When ever port is added, enable port max rate
    bf_tm_sch_l1_set_port_default(dev, port, NULL);
    rc = g_port_hw_fptr_tbl.port_add(dev, p);
    TM_UNLOCK_AND_FLUSH(dev);
  }
  return (rc);
}

bf_tm_status_t bf_tm_delete_port(bf_dev_id_t dev, bf_dev_port_t port) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  if (g_port_hw_fptr_tbl.port_remove) {
    TM_LOCK(dev, g_tm_ctx[dev]->lock);
    p->offline = true;
    p->ct_enabled = false;
    p->speed_on_add = BF_SPEED_NONE;
    /* Set admin state to false and disable qac_rx */
    p->admin_state = false;
    p->qac_rx_enable = false;

    LOG_TRACE("Delete dev:%d port:%d", dev, port);

    rc = g_port_hw_fptr_tbl.port_remove(dev, p);
    TM_UNLOCK_AND_FLUSH(dev);
  }

  if (p->counter_state_list != NULL) {
    rc = tm_free_cache_counter_node(
        TM_PORT, p->counter_state_list, g_tm_ctx[dev]);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to delete Cached Counters for TM port (%d) ", port);
    }
    p->counter_state_list = NULL;
  }

  return (rc);
}

static bf_tm_status_t bf_tm_update_port_qac_rx(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_port_t *p,
                                               bool enable) {
  bf_status_t rc = BF_SUCCESS;

  /*
   * For ports that don't have MAC (CPU PCIe port and special internal
   * ports - recirculation ports) and for ports that have MAC but
   * recirculation enabled, qac rx gets enabled during
   * port-add (before port-enable) and qac rx gets disabled during
   * port-delete or port-disable.
   *
   * For ports that have MAC (front panel ports, CPU MAC ports and
   * internal ports on 32Q) and have recirculation disabled, qac rx
   * have enabled only during port-enable as enabling qac rx
   * during port-add and traffic already running will
   * cause queue stuck due to MAC not auto draining. Also, qac_rx gets
   * disabled during port-disable to avoid buffer usage for disabled ports.
   *
   */
  rc = bf_recirculation_get(dev, port, &(p->recirc_enable));
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Not able to get recirc enable status for dev %d, port %d, "
        "rc = %s (%d)",
        __func__,
        dev,
        port,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  if (p->recirc_enable && enable) {
    LOG_TRACE(
        "%s: recirculation enabled for dev %d, port %d, skip updating qac_rx",
        __func__,
        dev,
        port);
    return BF_SUCCESS;
  }

  if (g_port_hw_fptr_tbl.port_qac_rx_wr_fptr) {
    TM_LOCK(dev, g_tm_ctx[dev]->lock);

    if (enable) {
      LOG_TRACE("Enabling QAC rx during port-enable for dev %d, dev_port %d",
                dev,
                port);
      p->qac_rx_enable = true;
    } else {
      LOG_TRACE("Disabling QAC rx during port-disable for dev %d, dev_port %d",
                dev,
                port);
      p->qac_rx_enable = false;
    }

    /* For hitless HA case, do not touch HW */
    if (TM_HITLESS_WARM_INIT_IN_PROGRESS(dev)) {
      TM_UNLOCK_AND_FLUSH(dev);
      return (rc);
    }

    rc = g_port_hw_fptr_tbl.port_qac_rx_wr_fptr(dev, p);
    TM_UNLOCK_AND_FLUSH(dev);
  }

  return (rc);
}

bf_tm_status_t bf_tm_update_port_status(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bool state) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  // Update the SW cache for port status
  p->status = state;

  // Update QAC RX mode
  rc = bf_tm_update_port_qac_rx(dev, port, p, state);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Failed to %s QAC RX for dev %d, port %d, "
        "rc = %s (%d)",
        __func__,
        (state ? "enable" : "disable"),
        dev,
        port,
        bf_err_str(rc),
        rc);
  }

  return (rc);
}

bf_tm_status_t bf_tm_update_port_admin_state(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bool enable) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  // Update the SW cache for admin state
  p->admin_state = enable;

  if (p->has_mac && !p->recirc_enable && !tm_is_device_locked(dev)) {
    // If this is a MAC port and is not a recirc port then the
    // the actual link up happens in port_status callback so we can
    // return Success here.
    // If the device is locked (we are in a HA replay) then go ahead and update
    // the QAC Rx state based on the admin enable state.  Once the HA completes
    // we will get link up/down notifications and can reevaluate the QAC Rx
    // state.  However, to avoid momentary traffic loss we start off assuming
    // the port is up.  If the port is actually down and there is traffic
    // forwarding to it, this may briefly allow those packets to be enqueued and
    // reach the egress pipeline where they are dropped at the MAC.
    LOG_TRACE(
        "%s: MAC port (Non Recirc) %s QAC RX for dev %d, port %d "
        "SUCCESS",
        __func__,
        (enable ? "enable" : "disable"),
        dev,
        port);
    return rc;
  }

  // Update QAC RX mode
  rc = bf_tm_update_port_qac_rx(dev, port, p, enable);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Failed to %s QAC RX for dev %d, port %d, "
        "rc = %s (%d)",
        __func__,
        (enable ? "enable" : "disable"),
        dev,
        port,
        bf_err_str(rc),
        rc);
  }

  return (rc);
}

bf_tm_status_t bf_tm_port_get_egress_drop_color_counter(bf_dev_id_t dev,
                                                        bf_tm_port_t *p,
                                                        bf_tm_color_t color,
                                                        uint64_t *count) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.egress_drop_color_cntr_fptr && count) {
    rc = g_port_hw_fptr_tbl.egress_drop_color_cntr_fptr(dev, p, color, count);
  } else {
    rc = BF_INVALID_ARG;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pre_port_mask(bf_dev_id_t dev,
                                            uint32_t *mask_array,
                                            uint32_t size) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.pre_port_mask_rd_fptr && mask_array) {
    rc = g_port_hw_fptr_tbl.pre_port_mask_rd_fptr(dev, mask_array, size);
  } else {
    rc = BF_INVALID_ARG;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_pre_port_mask(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.pre_port_mask_wr_fptr) {
    rc = g_port_hw_fptr_tbl.pre_port_mask_wr_fptr(dev);
  } else {
    rc = BF_INVALID_ARG;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pre_port_down_mask(bf_dev_id_t dev,
                                                 uint32_t *mask_array,
                                                 uint32_t size) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.pre_port_down_mask_rd_fptr && mask_array) {
    rc = g_port_hw_fptr_tbl.pre_port_down_mask_rd_fptr(dev, mask_array, size);
  } else {
    rc = BF_INVALID_ARG;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_pre_port_down_mask(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.pre_port_down_mask_wr_fptr) {
    rc = g_port_hw_fptr_tbl.pre_port_down_mask_wr_fptr(dev);
  } else {
    rc = BF_INVALID_ARG;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pfc_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        uint8_t *state) {
  bf_status_t rc = BF_SUCCESS;
  if (state) {
    *state = 0;
  } else {
    return BF_INVALID_ARG;
  }
  if (g_port_hw_fptr_tbl.port_pfc_state_fptr) {
    rc = g_port_hw_fptr_tbl.port_pfc_state_fptr(dev, p, state);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_set_pfc_state(bf_dev_id_t dev,
                                        bf_tm_port_t *p,
                                        uint8_t icos,
                                        bool state) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.set_port_pfc_state_fptr) {
    rc = g_port_hw_fptr_tbl.set_port_pfc_state_fptr(dev, p, icos, state);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_pfc_state_ext(bf_dev_id_t dev,
                                            bf_tm_port_t *p_dscr,
                                            uint8_t *port_ppg_state,
                                            uint8_t *rm_pfc_state,
                                            uint8_t *mac_pfc_out,
                                            bool *mac_pause_out) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.port_pfc_state_ext_fptr) {
    rc = g_port_hw_fptr_tbl.port_pfc_state_ext_fptr(
        dev, p_dscr, port_ppg_state, rm_pfc_state, mac_pfc_out, mac_pause_out);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_clear_pfc_state(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.clr_port_pfc_state_fptr) {
    rc = g_port_hw_fptr_tbl.clr_port_pfc_state_fptr(dev, p);
  } else {
    rc = BF_NOT_IMPLEMENTED;
  }
  return (rc);
}

bf_tm_status_t bf_tm_port_get_defaults(bf_dev_id_t devid,
                                       bf_tm_port_t *p,
                                       bf_tm_port_defaults_t *def) {
  BF_TM_INVALID_ARG(def == NULL);
  if (g_port_hw_fptr_tbl.port_get_defaults_fptr) {
    return g_port_hw_fptr_tbl.port_get_defaults_fptr(devid, p, def);
  }
  return BF_NOT_SUPPORTED;
}

#define BF_TM_PORT_HW_FTBL_SET(uc_ct_limit_fptr,                               \
                               fc_mode_fptr,                                   \
                               fc_rx_fptr,                                     \
                               pfc_cos_map_fptr,                               \
                               port_add_fptr,                                  \
                               port_delete_fptr,                               \
                               set_cpu_port_fptr,                              \
                               cut_through_fptr,                               \
                               qac_rx_fptr,                                    \
                               qac_flush_fptr,                                 \
                               ig_wm_clear_fptr,                               \
                               eg_wm_clear_fptr,                               \
                               ig_drop_cntr_clear_fptr,                        \
                               eg_drop_cntr_clear_fptr,                        \
                               skid_limit_fptr,                                \
                               pre_port_mask,                                  \
                               pre_port_down_mask,                             \
                               ingress_usage_cntr,                             \
                               clr_wac_drop_st,                                \
                               egress_usage_cntr,                              \
                               clr_qac_dropstate,                              \
                               clr_qac_drop_lmt,                               \
                               set_pfc_state,                                  \
                               clr_pfc_state,                                  \
                               wac_drop_limit,                                 \
                               qac_drop_limit,                                 \
                               wac_hyst_limit,                                 \
                               qac_hyst_limit)                                 \
  {                                                                            \
    g_port_hw_fptr_tbl.port_uc_ct_limit_wr_fptr = uc_ct_limit_fptr;            \
    g_port_hw_fptr_tbl.port_flowcontrol_mode_wr_fptr = fc_mode_fptr;           \
    g_port_hw_fptr_tbl.port_flowcontrol_rx_wr_fptr = fc_rx_fptr;               \
    g_port_hw_fptr_tbl.port_pfc_cos_map_wr_fptr = pfc_cos_map_fptr;            \
    g_port_hw_fptr_tbl.port_add = port_add_fptr;                               \
    g_port_hw_fptr_tbl.port_remove = port_delete_fptr;                         \
    g_port_hw_fptr_tbl.cpu_port_wr_fptr = set_cpu_port_fptr;                   \
    g_port_hw_fptr_tbl.port_cut_through_wr_fptr = cut_through_fptr;            \
    g_port_hw_fptr_tbl.port_qac_rx_wr_fptr = qac_rx_fptr;                      \
    g_port_hw_fptr_tbl.port_flush_queues_fptr = qac_flush_fptr;                \
    g_port_hw_fptr_tbl.ingress_wm_clear_fptr = ig_wm_clear_fptr;               \
    g_port_hw_fptr_tbl.egress_wm_clear_fptr = eg_wm_clear_fptr;                \
    g_port_hw_fptr_tbl.ingress_drop_cntr_clear_fptr = ig_drop_cntr_clear_fptr; \
    g_port_hw_fptr_tbl.egress_drop_cntr_clear_fptr = eg_drop_cntr_clear_fptr;  \
    g_port_hw_fptr_tbl.port_skid_limit_wr_fptr = skid_limit_fptr;              \
    g_port_hw_fptr_tbl.pre_port_mask_wr_fptr = pre_port_mask;                  \
    g_port_hw_fptr_tbl.pre_port_down_mask_wr_fptr = pre_port_down_mask;        \
    g_port_hw_fptr_tbl.ingress_usage_cntr_wr_fptr = ingress_usage_cntr;        \
    g_port_hw_fptr_tbl.clr_wac_drop_state_fptr = clr_wac_drop_st;              \
    g_port_hw_fptr_tbl.egress_usage_cntr_wr_fptr = egress_usage_cntr;          \
    g_port_hw_fptr_tbl.clr_qac_drop_state_fptr = clr_qac_dropstate;            \
    g_port_hw_fptr_tbl.clr_qac_drop_lmt_fptr = clr_qac_drop_lmt;               \
    g_port_hw_fptr_tbl.set_port_pfc_state_fptr = set_pfc_state;                \
    g_port_hw_fptr_tbl.clr_port_pfc_state_fptr = clr_pfc_state;                \
    g_port_hw_fptr_tbl.port_wac_drop_limit_wr_fptr = wac_drop_limit;           \
    g_port_hw_fptr_tbl.port_qac_drop_limit_wr_fptr = qac_drop_limit;           \
    g_port_hw_fptr_tbl.port_wac_hyst_wr_fptr = wac_hyst_limit;                 \
    g_port_hw_fptr_tbl.port_qac_hyst_wr_fptr = qac_hyst_limit;                 \
  }

#define BF_TM_PORT_HW_FTBL_GET(uc_ct_limit_fptr,                               \
                               pfc_cos_map_fptr,                               \
                               ig_drop_cntr_fptr,                              \
                               eg_drop_cntr_fptr,                              \
                               wac_drop_state_fptr,                            \
                               wac_drop_state_ext_fptr,                        \
                               qac_drop_state_fptr,                            \
                               ig_usage_fptr,                                  \
                               eg_usage_fptr,                                  \
                               ig_wm_cntr_fptr,                                \
                               eg_wm_cntr_fptr,                                \
                               fc_mode_fptr,                                   \
                               fc_rx_fptr,                                     \
                               cut_through_fptr,                               \
                               credits_rd_fptr,                                \
                               eg_drop_color_cntr_fptr,                        \
                               wac_drop_limit_fptr,                            \
                               qac_drop_limit_fptr,                            \
                               wac_hyst_fptr,                                  \
                               qac_hyst_fptr,                                  \
                               pre_port_mask,                                  \
                               pre_port_down_mask,                             \
                               port_pfc_state,                                 \
                               port_pfc_state_ext,                             \
                               port_defaults_fptr,                             \
                               port_pfc_en_cos_rd_fptr)                        \
  {                                                                            \
    g_port_hw_fptr_tbl.port_uc_ct_limit_rd_fptr = uc_ct_limit_fptr;            \
    g_port_hw_fptr_tbl.port_pfc_cos_bvec_rd_fptr = pfc_cos_map_fptr;           \
    g_port_hw_fptr_tbl.ingress_drop_cntr_fptr = ig_drop_cntr_fptr;             \
    g_port_hw_fptr_tbl.egress_drop_cntr_fptr = eg_drop_cntr_fptr;              \
    g_port_hw_fptr_tbl.wac_drop_state_get_fptr = wac_drop_state_fptr;          \
    g_port_hw_fptr_tbl.wac_drop_state_ext_get_fptr = wac_drop_state_ext_fptr;  \
    g_port_hw_fptr_tbl.qac_drop_state_get_fptr = qac_drop_state_fptr;          \
    g_port_hw_fptr_tbl.ingress_usage_cntr_fptr = ig_usage_fptr;                \
    g_port_hw_fptr_tbl.egress_usage_cntr_fptr = eg_usage_fptr;                 \
    g_port_hw_fptr_tbl.ingress_wm_cntr_fptr = ig_wm_cntr_fptr;                 \
    g_port_hw_fptr_tbl.egress_wm_cntr_fptr = eg_wm_cntr_fptr;                  \
    g_port_hw_fptr_tbl.port_flowcontrol_mode_rd_fptr = fc_mode_fptr;           \
    g_port_hw_fptr_tbl.port_flowcontrol_rx_rd_fptr = fc_rx_fptr;               \
    g_port_hw_fptr_tbl.port_cut_through_rd_fptr = cut_through_fptr;            \
    g_port_hw_fptr_tbl.port_credits_rd_fptr = credits_rd_fptr;                 \
    g_port_hw_fptr_tbl.egress_drop_color_cntr_fptr = eg_drop_color_cntr_fptr;  \
    g_port_hw_fptr_tbl.port_wac_drop_limit_rd_fptr = wac_drop_limit_fptr;      \
    g_port_hw_fptr_tbl.port_qac_drop_limit_rd_fptr = qac_drop_limit_fptr;      \
    g_port_hw_fptr_tbl.port_wac_hyst_rd_fptr = wac_hyst_fptr;                  \
    g_port_hw_fptr_tbl.port_qac_hyst_rd_fptr = qac_hyst_fptr;                  \
    g_port_hw_fptr_tbl.pre_port_mask_rd_fptr = pre_port_mask;                  \
    g_port_hw_fptr_tbl.pre_port_down_mask_rd_fptr = pre_port_down_mask;        \
    g_port_hw_fptr_tbl.port_pfc_state_fptr = port_pfc_state;                   \
    g_port_hw_fptr_tbl.port_pfc_state_ext_fptr = port_pfc_state_ext;           \
    g_port_hw_fptr_tbl.port_get_defaults_fptr = port_defaults_fptr;            \
    g_port_hw_fptr_tbl.port_pfc_enabled_cos_rd_fptr = port_pfc_en_cos_rd_fptr; \
  }

void bf_tm_port_null_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  (void)tm_ctx;
  BF_TM_PORT_HW_FTBL_SET(NULL,
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
                         NULL,
                         NULL,
                         NULL);
  BF_TM_PORT_HW_FTBL_GET(NULL,
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

static void bf_tm_port_set_hw_ftbl_wr_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_SET(
        bf_tm_tofino_port_set_uc_ct_limit,
        bf_tm_tofino_port_set_flowcontrol_mode,
        bf_tm_tofino_port_set_flowcontrol_rx,
        bf_tm_tofino_port_set_pfc_cos_map,
        bf_tm_tofino_add_new_port,
        bf_tm_tofino_delete_port,
        bf_tm_tofino_port_set_cpu_port,
        bf_tm_tofino_port_set_cut_through,
        bf_tm_tofino_port_set_qac_rx,
        NULL /* flush all queues */,
        bf_tm_tofino_port_clear_ingress_watermark,
        bf_tm_tofino_port_clear_egress_watermark,
        bf_tm_tofino_port_clear_ingress_drop_cntr,
        bf_tm_tofino_port_clear_egress_drop_cntr,
        NULL /* No support for port skid limit config in Tofino */,
        bf_tm_tofino_port_clear_pre_mask,
        bf_tm_tofino_port_clear_pre_down_mask,
        bf_tm_tofino_port_clear_ingress_usage_cntr,
        bf_tm_tofino_port_clear_wac_drop_state,
        bf_tm_tofino_port_clear_egress_usage_cntr,
        bf_tm_tofino_port_clear_qac_drop_state,
        bf_tm_tofino_port_clear_qac_drop_limit,
        bf_tm_tofino_port_set_pfc_state,
        bf_tm_tofino_port_clear_pfc_state,
        bf_tm_tofino_port_set_wac_drop_limit,
        bf_tm_tofino_port_set_qac_drop_limit,
        bf_tm_tofino_port_set_wac_hyst_limit,
        bf_tm_tofino_port_set_qac_hyst_limit);
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_SET(bf_tm_tof2_port_set_uc_ct_limit,
                           bf_tm_tof2_port_set_flowcontrol_mode,
                           bf_tm_tof2_port_set_flowcontrol_rx,
                           bf_tm_tof2_port_set_pfc_cos_map,
                           bf_tm_tof2_add_new_port,
                           bf_tm_tof2_delete_port,
                           bf_tm_tof2_port_set_cpu_port,
                           bf_tm_tof2_port_set_cut_through,
                           bf_tm_tof2_port_set_qac_rx,
                           bf_tm_tof2_port_set_all_queue_flush,
                           bf_tm_tof2_port_clear_ingress_wm_cntr,
                           bf_tm_tof2_port_clear_egress_wm_cntr,
                           bf_tm_tof2_port_clear_ingress_drop_cntr,
                           bf_tm_tof2_port_clear_egress_drop_cntr,
                           bf_tm_tof2_port_set_skid_limit,
                           bf_tm_tof2_port_clear_pre_mask,
                           bf_tm_tof2_port_clear_pre_down_mask,
                           bf_tm_tof2_port_clear_ingress_usage_cntr,
                           bf_tm_tof2_port_clear_wac_drop_state,
                           bf_tm_tof2_port_clear_egress_usage_cntr,
                           bf_tm_tof2_port_clear_qac_drop_state,
                           bf_tm_tof2_port_clear_qac_drop_limit,
                           NULL,  // port_pfc_state is read only for TF2
                           bf_tm_tof2_port_clear_pfc_state,
                           bf_tm_tof2_port_set_wac_drop_limit,
                           bf_tm_tof2_port_set_qac_drop_limit,
                           NULL,
                           bf_tm_tof2_port_set_qac_hyst_limit);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_SET(bf_tm_tof3_port_set_uc_ct_limit,
                           bf_tm_tof3_port_set_flowcontrol_mode,
                           bf_tm_tof3_port_set_flowcontrol_rx,
                           bf_tm_tof3_port_set_pfc_cos_map,
                           bf_tm_tof3_add_new_port,
                           bf_tm_tof3_delete_port,
                           bf_tm_tof3_port_set_cpu_port,
                           bf_tm_tof3_port_set_cut_through,
                           bf_tm_tof3_port_set_qac_rx,
                           bf_tm_tof3_port_set_all_queue_flush,
                           bf_tm_tof3_port_clear_ingress_wm_cntr,
                           bf_tm_tof3_port_clear_egress_wm_cntr,
                           bf_tm_tof3_port_clear_ingress_drop_cntr,
                           bf_tm_tof3_port_clear_egress_drop_cntr,
                           bf_tm_tof3_port_set_skid_limit,
                           bf_tm_tof3_port_clear_pre_mask,
                           bf_tm_tof3_port_clear_pre_down_mask,
                           bf_tm_tof3_port_clear_ingress_usage_cntr,
                           bf_tm_tof3_port_clear_wac_drop_state,
                           bf_tm_tof3_port_clear_egress_usage_cntr,
                           bf_tm_tof3_port_clear_qac_drop_state,
                           bf_tm_tof3_port_clear_qac_drop_limit,
                           NULL,  // port_pfc_state is read only for TF3
                           bf_tm_tof3_port_clear_pfc_state,
                           bf_tm_tof3_port_set_wac_drop_limit,
                           bf_tm_tof3_port_set_qac_drop_limit,
                           NULL,
                           bf_tm_tof3_port_set_qac_hyst_limit);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

static void bf_tm_port_set_hw_ftbl_rd_funcs(bf_tm_dev_ctx_t *tm_ctx) {
  if (BF_TM_IS_TOFINO(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_GET(bf_tm_tofino_port_get_uc_ct_limit,
                           bf_tm_tofino_port_get_pfc_cos_mask,
                           bf_tm_tofino_port_get_ingress_drop_cntr,
                           bf_tm_tofino_port_get_egress_drop_cntr,
                           bf_tm_tofino_port_get_wac_drop_state,
                           NULL,
                           bf_tm_tofino_port_get_qac_drop_state,
                           bf_tm_tofino_port_get_ingress_usage_cntr,
                           bf_tm_tofino_port_get_egress_usage_cntr,
                           bf_tm_tofino_port_get_ingress_wm_cntr,
                           bf_tm_tofino_port_get_egress_wm_cntr,
                           bf_tm_tofino_port_get_flowcontrol_mode,
                           bf_tm_tofino_port_get_flowcontrol_rx,
                           bf_tm_tofino_port_get_cut_through,
                           bf_tm_tofino_get_port_credits,
                           bf_tm_tofino_port_get_egress_drop_color_cntr,
                           bf_tm_tofino_port_get_wac_drop_limit,
                           bf_tm_tofino_port_get_qac_drop_limit,
                           bf_tm_tofino_port_get_wac_hyst_limit,
                           bf_tm_tofino_port_get_qac_hyst_limit,
                           bf_tm_tofino_port_get_pre_mask,
                           bf_tm_tofino_port_get_pre_down_mask,
                           bf_tm_tofino_port_get_pfc_state,
                           NULL /*extended pfc state*/,
                           bf_tm_tofino_port_get_defaults,
                           bf_tm_tofino_port_get_pfc_enable_mask);
  }
  if (BF_TM_IS_TOF2(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_GET(bf_tm_tof2_port_get_uc_ct_limit,
                           bf_tm_tof2_port_get_pfc_cos_mask,
                           bf_tm_tof2_port_get_ingress_drop_cntr,
                           bf_tm_tof2_port_get_egress_drop_cntr,
                           NULL,
                           bf_tm_tof2_port_get_wac_drop_state_ext,
                           bf_tm_tof2_port_get_qac_drop_state,
                           bf_tm_tof2_port_get_ingress_usage_cntr,
                           bf_tm_tof2_port_get_egress_usage_cntr,
                           bf_tm_tof2_port_get_ingress_wm_cntr,
                           bf_tm_tof2_port_get_egress_wm_cntr,
                           bf_tm_tof2_port_get_flowcontrol_mode,
                           bf_tm_tof2_port_get_flowcontrol_rx,
                           bf_tm_tof2_port_get_cut_through,
                           NULL,
                           bf_tm_tof2_port_get_egress_drop_color_cntr,
                           bf_tm_tof2_port_get_wac_drop_limit,
                           bf_tm_tof2_port_get_qac_drop_limit,
                           NULL,
                           bf_tm_tof2_port_get_qac_hyst_limit,
                           bf_tm_tof2_port_get_pre_mask,
                           bf_tm_tof2_port_get_pre_down_mask,
                           bf_tm_tof2_port_get_pfc_state,
                           bf_tm_tof2_port_get_pfc_state_ext,
                           bf_tm_tof2_port_get_defaults,
                           bf_tm_tof2_port_get_pfc_enable_mask);
  }
  if (BF_TM_IS_TOF3(tm_ctx->asic_type)) {
    BF_TM_PORT_HW_FTBL_GET(bf_tm_tof3_port_get_uc_ct_limit,
                           bf_tm_tof3_port_get_pfc_cos_mask,
                           bf_tm_tof3_port_get_ingress_drop_cntr,
                           bf_tm_tof3_port_get_egress_drop_cntr,
                           NULL,
                           bf_tm_tof3_port_get_wac_drop_state_ext,
                           bf_tm_tof3_port_get_qac_drop_state,
                           bf_tm_tof3_port_get_ingress_usage_cntr,
                           bf_tm_tof3_port_get_egress_usage_cntr,
                           bf_tm_tof3_port_get_ingress_wm_cntr,
                           bf_tm_tof3_port_get_egress_wm_cntr,
                           bf_tm_tof3_port_get_flowcontrol_mode,
                           bf_tm_tof3_port_get_flowcontrol_rx,
                           bf_tm_tof3_port_get_cut_through,
                           NULL,
                           bf_tm_tof3_port_get_egress_drop_color_cntr,
                           bf_tm_tof3_port_get_wac_drop_limit,
                           bf_tm_tof3_port_get_qac_drop_limit,
                           NULL,
                           bf_tm_tof3_port_get_qac_hyst_limit,
                           bf_tm_tof3_port_get_pre_mask,
                           bf_tm_tof3_port_get_pre_down_mask,
                           bf_tm_tof3_port_get_pfc_state,
                           bf_tm_tof3_port_get_pfc_state_ext,
                           bf_tm_tof3_port_get_defaults,
                           bf_tm_tof3_port_get_pfc_enable_mask);
  }
  if (BF_TM_IS_TOFINOLITE(tm_ctx->asic_type)) {
    // Future addition
  }
}

void bf_tm_port_set_hw_ftbl(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_port_set_hw_ftbl_wr_funcs(tm_ctx);
  bf_tm_port_set_hw_ftbl_rd_funcs(tm_ctx);
}

void bf_tm_port_set_hw_ftbl_cfg_restore(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_port_null_hw_ftbl(tm_ctx);
  bf_tm_port_set_hw_ftbl_rd_funcs(tm_ctx);
}

bf_tm_status_t bf_tm_init_ports(bf_tm_dev_ctx_t *tm_ctx) {
  int ports_per_pipe, k, i, j;
  bf_dev_pipe_t pipe = 0;

  // Allocate #ports * pipe
  tm_ctx->ports = bf_sys_calloc(
      1,
      sizeof(bf_tm_port_t) * tm_ctx->tm_cfg.pipe_cnt *
          (tm_ctx->tm_cfg.mirror_port_cnt +
           (tm_ctx->tm_cfg.ports_per_pg * tm_ctx->tm_cfg.pg_per_pipe)));
  if (!tm_ctx->ports) {
    return (BF_NO_SYS_RESOURCES);
  }

  ports_per_pipe = (tm_ctx->tm_cfg.pg_per_pipe * tm_ctx->tm_cfg.ports_per_pg);
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
    bf_tm_port_t *port =
        tm_ctx->ports + (i * (ports_per_pipe + tm_ctx->tm_cfg.mirror_port_cnt));
    for (k = 0; k < ports_per_pipe; k++) {
      port->p_pipe = pipe;
      port->d_pipe = (pipe % 4);
      port->l_pipe = i;
      port->port = k;
      port->uport =
          DEV_PORT_TO_LOCAL_PORT(lld_sku_map_devport_from_device_to_user(
              tm_ctx->devid, MAKE_DEV_PORT(port->l_pipe, port->port)));

      port->pg = k / tm_ctx->tm_cfg.ports_per_pg;
      port->speed = BF_SPEED_10G;  // Set deafult port speed to 10G
                                   // As and when ports get added, speed will
                                   // change accordingly.
      port->speed_on_add = BF_SPEED_NONE;
      port->qid_profile = UINT8_MAX;
      port->offline =
          true;  // When port is added, port goes from offline to online.
      port->wac_resume_limit =
          32;  // Hysteresis is default to TM_TOF*_WAC_DEFAULT_HYSTERESIS
      port->qac_resume_limit =
          32;  // Hysteresis is default to TM_TOF*_QAC_DEFAULT_HYSTERESIS
      port->wac_drop_limit = tm_ctx->tm_cfg.total_cells;
      port->qac_drop_limit = tm_ctx->tm_cfg.total_cells;
      port->ppg_count = 0;
      // Disable shaping by default
      port->max_rate_enabled = false;
      // Disable cut-through for all ports by default
      port->ct_enabled = false;
      /* Set admin state to false and disable qac_rx by default */
      port->admin_state = false;
      port->qac_rx_enable = false;

      for (j = 0; j < tm_ctx->tm_cfg.icos_count; j++) {
        port->cos_to_icos[j] = j;  // Default: straight cos to icos mapping.
      }
      port++;
    }
    // Program mirror port
    for (j = 0; j < tm_ctx->tm_cfg.mirror_port_cnt; j++) {
      port->p_pipe = pipe;
      port->l_pipe = i;
      port->port = tm_ctx->tm_cfg.mirror_port_start + j;
      port->uport =
          DEV_PORT_TO_LOCAL_PORT(lld_sku_map_devport_from_device_to_user(
              tm_ctx->devid, MAKE_DEV_PORT(port->l_pipe, port->port)));

      port->speed = BF_SPEED_100G;  // Set deafult port speed to 100G.
                                    // Speed doesn't matter. This mirror port is
                                    // used only for accounting purposes in TM.
      port->speed_on_add = port->speed;
      port->qid_profile = UINT8_MAX;
      port->offline = false;  // Do not disable mirror port
      port->wac_resume_limit =
          32;  // Hysteresis is default to TM_TOF*_WAC_DEFAULT_HYSTERESIS
      port->qac_resume_limit =
          32;  // Hysteresis is default to TM_TOF*_QAC_DEFAULT_HYSTERESIS
      port->wac_drop_limit = tm_ctx->tm_cfg.total_cells;
      port->qac_drop_limit = tm_ctx->tm_cfg.total_cells;
      port->ppg_count = 0;
      // Disable shaping by default
      port->max_rate_enabled = false;
      port->skid_limit = 0;
    }
  }

  bf_tm_port_set_hw_ftbl(tm_ctx);
  return (BF_TM_EOK);
}

bf_tm_status_t bf_tm_port_flush_all_queues(bf_dev_id_t devid, bf_tm_port_t *p) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (g_port_hw_fptr_tbl.port_flush_queues_fptr) {
    rc = g_port_hw_fptr_tbl.port_flush_queues_fptr(devid, p);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR(
          "TM: flush all queues failed for dev %d, pipe %d port %d, rc %s "
          "(%d)",
          devid,
          p->p_pipe,
          p->port,
          bf_err_str(rc),
          rc);
      return (rc);
    }
  }
  return rc;
}

/*
 * Setup the Cache Counters for Port.
 * per port
 *
 * Related APIs:
 *
 * param dev        ASIC device identifier.
 * param port       port handle
 * return           Status of API call.
 */
bf_tm_status_t bf_tm_port_set_cache_counters(bf_dev_id_t dev,
                                             bf_dev_port_t port) {
  bf_tm_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  BF_TM_INVALID_ARG(TM_IS_PORT_INVALID(port, g_tm_ctx[dev]));

  rc = bf_tm_port_get_descriptor(dev, port, &p);
  if (rc != BF_SUCCESS) return (rc);

  TM_LOCK(dev, g_tm_ctx[dev]->lock);

  if (p->counter_state_list != NULL) {
    tm_free_cache_counter_node(TM_PORT, p->counter_state_list, g_tm_ctx[dev]);
    p->counter_state_list = NULL;
  }

  // Add the Cached Counter Node for < 64 bit counters wrap case.
  tm_counter_node_id_t node_id;
  tm_cache_counter_node_list_t *node_ptr = NULL;
  TRAFFIC_MGR_MEMSET(&node_id, 0, sizeof(tm_counter_node_id_t));
  node_id.port = port;
  node_id.pipe = DEV_PORT_TO_PIPE(port);
  rc |= tm_allocate_and_init_cache_counter_node(
      TM_PORT, g_tm_ctx[dev], &node_id, &node_ptr);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "Unable to add Cached Counters for TM port (%d) "
        "Freeing port",
        port);
    p->counter_state_list = NULL;

    // Not sure if should call remove_port here
    // tm_remove_port(dev, port, direction);
  } else {
    p->counter_state_list = node_ptr;
  }
  TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
  return (rc);
}

bf_tm_status_t bf_tm_port_get_credits(bf_dev_id_t dev, bf_tm_port_t *p) {
  bf_status_t rc = BF_SUCCESS;
  if (g_port_hw_fptr_tbl.port_credits_rd_fptr) {
    g_port_hw_fptr_tbl.port_credits_rd_fptr(dev, p);
  }
  return (rc);
}
