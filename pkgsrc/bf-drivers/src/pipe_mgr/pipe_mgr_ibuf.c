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


/*!
 * @file pipe_mgr_ibuf.c
 * @date
 *
 * Implementation of IBUF related initializations
 */

#include <dvm/bf_drv_intf.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_tof_ibuf.h"
#include "pipe_mgr_tof2_parde.h"
#include "pipe_mgr_tof3_parde.h"

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

pipe_status_t pipe_mgr_ibuf_set_version_bits(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             uint8_t version) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_status_t sts;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_set_version_bits(sess_hdl, dev_info, version);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_ibuf_tof2_set_version_bits(sess_hdl, dev_info, version);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_ibuf_tof3_set_version_bits(sess_hdl, dev_info, version);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }
  return sts;
}

// Report congestion notification to Parser
pipe_status_t bf_pipe_enable_ibuf_congestion_notif_to_parser(
    bf_dev_id_t dev_id,
    bf_dev_port_t port_id,
    uint16_t low_wm_bytes,
    uint16_t hi_wm_bytes) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_status_t sts = pipe_mgr_api_enter(sess_hdl);
  if (sts != PIPE_SUCCESS) return sts;

  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR(
        "%s: Dev %d port %d is not configured", __func__, dev_id, port_id);
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  bool dis_conf_notif_bkup = port_info->disable_cong_notif;
  bool use_custom_wm_bkup = port_info->use_custom_wm_bytes;
  uint16_t lo_wm_bkup = port_info->lo_wm_bytes;
  uint16_t hi_wm_bkup = port_info->hi_wm_bytes;
  port_info->use_custom_wm_bytes = true;
  port_info->disable_cong_notif = false;
  port_info->lo_wm_bytes = low_wm_bytes;
  port_info->hi_wm_bytes = hi_wm_bytes;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_enable_congestion_notif_to_parser(dev_info,
                                                                port_info);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_ibuf_tof2_config_congestion_notif_to_parser(
          sess_hdl, dev_info, port_info);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_ibuf_tof3_config_congestion_notif_to_parser(
          sess_hdl, dev_info, port_info);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "Dev %d port %d Failed to update parser congestion notification, %s",
        dev_id,
        port_id,
        pipe_str_err(sts));
    port_info->use_custom_wm_bytes = use_custom_wm_bkup;
    port_info->disable_cong_notif = dis_conf_notif_bkup;
    port_info->lo_wm_bytes = lo_wm_bkup;
    port_info->hi_wm_bytes = hi_wm_bkup;
  }

done:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

// Based on ingress buffer usage, enable TX_OFF so that
// MAC stops sending
pipe_status_t bf_pipe_parb_enable_flow_control_to_mac(bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id,
                                                      uint16_t low_wm_bytes,
                                                      uint16_t hi_wm_bytes) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_status_t sts = pipe_mgr_api_enter(sess_hdl);
  if (sts != PIPE_SUCCESS) return sts;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_parb_enable_flow_control_to_mac(
          dev_info, port_id, low_wm_bytes, hi_wm_bytes);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }

  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

pipe_status_t bf_pipe_disable_ibuf_congestion_notif_to_parser(
    bf_dev_id_t dev_id, bf_dev_port_t port_id) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_status_t sts = pipe_mgr_api_enter(sess_hdl);
  if (sts != PIPE_SUCCESS) return sts;

  rmt_port_info_t *port_info = pipe_mgr_get_port_info(dev_id, port_id);
  if (!port_info) {
    LOG_ERROR(
        "%s: Dev %d port %d is not configured", __func__, dev_id, port_id);
    sts = PIPE_OBJ_NOT_FOUND;
    goto done;
  }

  bool dis_conf_notif_bkup = port_info->disable_cong_notif;
  bool use_custom_wm_bkup = port_info->use_custom_wm_bytes;
  uint16_t lo_wm_bkup = port_info->lo_wm_bytes;
  uint16_t hi_wm_bkup = port_info->hi_wm_bytes;
  port_info->use_custom_wm_bytes = false;
  port_info->disable_cong_notif = true;
  port_info->lo_wm_bytes = 0;
  port_info->hi_wm_bytes = 0;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_disable_congestion_notif_to_parser(dev_info,
                                                                 port_id);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_ibuf_tof2_config_congestion_notif_to_parser(
          sess_hdl, dev_info, port_info);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_ibuf_tof3_config_congestion_notif_to_parser(
          sess_hdl, dev_info, port_info);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "Dev %d port %d Failed to update parser congestion notification, %s",
        dev_id,
        port_id,
        pipe_str_err(sts));
    port_info->use_custom_wm_bytes = use_custom_wm_bkup;
    port_info->disable_cong_notif = dis_conf_notif_bkup;
    port_info->lo_wm_bytes = lo_wm_bkup;
    port_info->hi_wm_bytes = hi_wm_bkup;
  }

done:
  pipe_mgr_api_exit(sess_hdl);
  return sts;
}

pipe_status_t bf_pipe_parb_disable_flow_control_to_mac(bf_dev_id_t dev_id,
                                                       bf_dev_port_t port_id) {
  pipe_sess_hdl_t sess_hdl = pipe_mgr_ctx->int_ses_hndl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_status_t sts = pipe_mgr_api_enter(sess_hdl);
  if (sts != PIPE_SUCCESS) return sts;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts =
          pipe_mgr_ibuf_tof_parb_disable_flow_control_to_mac(dev_info, port_id);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }

  pipe_mgr_api_exit(sess_hdl);
  return sts;
}
