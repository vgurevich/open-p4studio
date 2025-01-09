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
#include <stddef.h>
#include <inttypes.h>
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include "pipe_mgr_tof_pkt_path_counter_display.h"
#include "pipe_mgr_tof2_pkt_path_counter_display.h"
#include "pipe_mgr_tof3_pkt_path_counter_display.h"
#include "pipe_mgr_int.h"

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

bf_packetpath_counter_t *bf_packet_path_buffer_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_ibuf_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_ipb_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_ipb_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_parser_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_iprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_iprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_iprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_deparser_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count) {
  bf_packetpath_counter_t *r = NULL;
  int c = 0;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;
  if (pipe >= dev_info->num_active_pipes) goto done;

  bf_dev_pipe_t phy_pipe;
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d pipe (%d, %d)",
              __func__,
              devid,
              pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_idprsr_counter(devid, phy_pipe, &c);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_idprsr_read_counter(dev_info, phy_pipe, &c);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_idprsr_read_counter(dev_info, phy_pipe, &c);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  if (count) *count = c;
  return r;
}

bf_packetpath_counter_t *bf_packet_path_parser2mau_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  bf_dev_pipe_t phy_pipe;
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d pipe (%d, %d)",
              __func__,
              devid,
              pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_pmarb_read_counter(devid, phy_pipe, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_pmarb_read_counter(devid, phy_pipe, count);
      break;
    default:
      r = NULL;
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_tm_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_s2p_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_s2p_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}
bf_packetpath_counter_t *bf_packet_path_tm_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_p2s_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_p2s_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_buffer_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_ebuf_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_epb_ebuf_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_epb_ebuf_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_parser_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  bf_dev_pipe_t phy_pipe, log_pipe;
  log_pipe = dev_cfg->dev_port_to_pipe(port);
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d port %d pipe (%d, %d)",
              __func__,
              devid,
              port,
              log_pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_eprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_eprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_eprsr_read_counter(
          devid, phy_pipe, dev_cfg->dev_port_to_local_port(port), 1, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

bf_packetpath_counter_t *bf_packet_path_deparser_egress_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count) {
  bf_packetpath_counter_t *r = NULL;
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);

  if (!dev_info) goto done;

  bf_dev_pipe_t phy_pipe;
  status = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Invalid pipe id requested on dev %d pipe (%d, %d)",
              __func__,
              devid,
              pipe,
              dev_info->num_active_pipes);
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      r = pipe_mgr_tof_pkt_path_edprsr_counter(devid, phy_pipe, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      r = pipe_mgr_tof2_pkt_path_edprsr_read_counter(dev_info, phy_pipe, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      r = pipe_mgr_tof3_pkt_path_edprsr_read_counter(dev_info, phy_pipe, count);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return r;
}

const char *bf_packet_path_counter_description_get(uint32_t description_index) {
  return (pipe_mgr_tof_pkt_path_get_counter_description(description_index));
}

void bf_packet_path_buffer_ingress_counter_clear(bf_dev_id_t devid,
                                                 bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end, port_tmp;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      for (port_tmp = port_start; port_tmp < port_end; port_tmp++) {
        pipe_mgr_tof_pkt_path_clear_ibuf_counter(
            devid, dev_cfg->dev_port_to_pipe(port), port_tmp);
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_ipb_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_ipb_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_parser_ingress_counter_clear(bf_dev_id_t devid,
                                                 bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end, port_tmp;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      for (port_tmp = port_start; port_tmp < port_end; port_tmp++) {
        pipe_mgr_tof_pkt_path_clear_iprsr_counter(
            devid, dev_cfg->dev_port_to_pipe(port), port_tmp);
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_iprsr_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_iprsr_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_deparser_ingress_counter_clear(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_pkt_path_clear_idprsr_counter(devid, pipe);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_idprsr_counter(dev_info, pipe);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_idprsr_counter(dev_info, pipe);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_buffer_egress_counter_clear(bf_dev_id_t devid,
                                                bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end, port_tmp;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      for (port_tmp = port_start; port_tmp < port_end; port_tmp++) {
        pipe_mgr_tof_pkt_path_clear_ebuf_counter(
            devid, dev_cfg->dev_port_to_pipe(port), port_tmp);
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_ebuf_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      pipe_mgr_tof2_pkt_path_clear_epb_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_ebuf_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      pipe_mgr_tof3_pkt_path_clear_epb_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}
void bf_packet_path_tm_ingress_counter_clear(bf_dev_id_t devid,
                                             bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_s2p_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_s2p_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_tm_egress_counter_clear(bf_dev_id_t devid,
                                            bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      goto done;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_p2s_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_p2s_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_parser_egress_counter_clear(bf_dev_id_t devid,
                                                bf_dev_port_t port) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  int port_start, port_end, port_tmp;
  port_start = dev_cfg->dev_port_to_local_port(port);
  port_end = port_start + dev_cfg->num_chan_per_port;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      for (port_tmp = port_start; port_tmp < port_end; port_tmp++) {
        pipe_mgr_tof_pkt_path_clear_eprsr_counter(
            devid, dev_cfg->dev_port_to_pipe(port), port_tmp);
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_eprsr_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_eprsr_counter(
          devid, dev_cfg->dev_port_to_pipe(port), port_start, port_end);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

void bf_packet_path_deparser_egress_counter_clear(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe) {
  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) goto done;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) goto done;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_pkt_path_clear_edprsr_counter(devid, pipe);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_path_clear_edprsr_counter(dev_info, pipe);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_path_clear_edprsr_counter(dev_info, pipe);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  pipe_mgr_api_exit(shdl);
  return;
}

pipe_status_t bf_pkt_path_ibp_drop_cntr_get(bf_dev_id_t devid,
                                            bf_dev_port_t port,
                                            bf_ibp_cntrs_t *ibp_cntrs) {
  bf_packetpath_counter_t *counters = NULL;
  int count = 0;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || (dev_info->dev_family != BF_DEV_FAMILY_TOFINO)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_api_exit(shdl);

  counters = bf_packet_path_buffer_ingress_counter_get(devid, port, &count);
  if (counters == NULL) {
    LOG_ERROR(
        "%s: bf_pkt_path_ibuf_read_counter returned NULL for "
        "requested on dev %d port %d",
        __func__,
        devid,
        port);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (int cntr_index = 0; cntr_index < count; cntr_index++) {
    switch (counters[cntr_index].description_index) {
      case IBUF_TOTAL_PKTS_DROPPED_ON_CHANNEL0:
      case IBUF_TOTAL_PKTS_DROPPED_ON_CHANNEL1:
      case IBUF_TOTAL_PKTS_DROPPED_ON_CHANNEL2:
      case IBUF_TOTAL_PKTS_DROPPED_ON_CHANNEL3:
        ibp_cntrs->total_pkts_drop = counters[cntr_index].value;
        break;
      case IBUF_DISCARDED_PKTS_IN_IBUF_ON_CHANNEL0:
      case IBUF_DISCARDED_PKTS_IN_IBUF_ON_CHANNEL1:
      case IBUF_DISCARDED_PKTS_IN_IBUF_ON_CHANNEL2:
      case IBUF_DISCARDED_PKTS_IN_IBUF_ON_CHANNEL3:
        ibp_cntrs->total_pkts_disc = counters[cntr_index].value;
        break;
      case IBUF_DISCARDED_RECIRCULATED_PKTS_ON_CH0:
      case IBUF_DISCARDED_RECIRCULATED_PKTS_ON_CH1:
      case IBUF_DISCARDED_RECIRCULATED_PKTS_ON_CH2:
      case IBUF_DISCARDED_RECIRCULATED_PKTS_ON_CH3:
        ibp_cntrs->total_recirc_pkt_disc = counters[cntr_index].value;
        break;
      case IBUF_DISCARDED_PKTS_IN_PRSR_DUE_TO_IBUF_CH0_FULL_OR_PRSR_REQ_TO_DROP:
      case IBUF_DISCARDED_PKTS_IN_PRSR_DUE_TO_IBUF_CH1_FULL_OR_PRSR_REQ_TO_DROP:
      case IBUF_DISCARDED_PKTS_IN_PRSR_DUE_TO_IBUF_CH2_FULL_OR_PRSR_REQ_TO_DROP:
      case IBUF_DISCARDED_PKTS_IN_PRSR_DUE_TO_IBUF_CH3_FULL_OR_PRSR_REQ_TO_DROP:
        ibp_cntrs->total_prsr_pkt_disc = counters[cntr_index].value;
    }
  }

  PIPE_MGR_FREE((void *)counters);
  return PIPE_SUCCESS;
}

pipe_status_t bf_pkt_path_iprsr_drop_cntr_get(bf_dev_id_t devid,
                                              bf_dev_port_t port,
                                              bf_iprsr_cntrs_t *iprsr_cntrs) {
  bf_packetpath_counter_t *counters = NULL;
  int count = 0;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || (dev_info->dev_family != BF_DEV_FAMILY_TOFINO)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_api_exit(shdl);

  counters = bf_packet_path_parser_ingress_counter_get(devid, port, &count);
  if (counters == NULL) {
    LOG_ERROR(
        "%s: bf_pkt_path_iprsr_read_counter returned NULL for "
        "requested on dev %d port %d",
        __func__,
        devid,
        port);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (int cntr_index = 0; cntr_index < count; cntr_index++) {
    switch (counters[cntr_index].description_index) {
      case IPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL0:
      case IPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL1:
      case IPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL2:
      case IPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL3:
        iprsr_cntrs->total_pkts_drop = counters[cntr_index].value;
        break;
      case IPRSR_FCS_ERROR:
        iprsr_cntrs->fcs_err_count = counters[cntr_index].value;
        break;
      case IPRSR_CHECKSUM_ERROR:
        iprsr_cntrs->csum_err_count = counters[cntr_index].value;
        break;
      case IPRSR_TCAM_PARITY_ERROR:
        iprsr_cntrs->tcam_parity_err_count = counters[cntr_index].value;
        break;
    }
  }

  PIPE_MGR_FREE((void *)counters);
  return PIPE_SUCCESS;
}

pipe_status_t bf_pkt_path_idprsr_drop_cntr_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, bf_idprsr_cntrs_t *idprsr_cntrs) {
  bf_packetpath_counter_t *counters = NULL;
  int count = 0;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || (dev_info->dev_family != BF_DEV_FAMILY_TOFINO)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_api_exit(shdl);

  counters = bf_packet_path_deparser_ingress_counter_get(devid, pipe, &count);
  if (counters == NULL) {
    LOG_ERROR(
        "%s: bf_pkt_path_idprsr_counter returned NULL for pipe id "
        "requested on dev %d (%d)",
        __func__,
        devid,
        pipe);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (int cntr_index = 0; cntr_index < count; cntr_index++) {
    switch (counters[cntr_index].description_index) {
      case IDPRSR_PACKETS_DISCARDED:
        idprsr_cntrs->pkts_disc = counters[cntr_index].value;
        break;
      case IDPRSR_PACKETS_DISCARDED_AT_TM_INTERFACE:
        idprsr_cntrs->pkts_disc_at_tm = counters[cntr_index].value;
        break;
      case IDPRSR_ERRORED_PACKETS_TO_TM:
        idprsr_cntrs->err_pkts_to_tm = counters[cntr_index].value;
        break;
      case IDPRSR_ERRORED_PACKETS_FROM_IBUF_TO_ICTM:
        idprsr_cntrs->err_pkts_to_ictm = counters[cntr_index].value;
        break;
      default:
        if (counters[cntr_index].description_index >=
                IDPRSR_CRC_ERROR_ON_PACKETS_FROM_IBUF_0 &&
            counters[cntr_index].description_index <=
                IDPRSR_CRC_ERROR_ON_PACKETS_FROM_IBUF_71) {
          idprsr_cntrs->crc_err[counters[cntr_index].description_index -
                                IDPRSR_CRC_ERROR_ON_PACKETS_FROM_IBUF_0] =
              counters[cntr_index].value;
        }
        break;
    }
  }

  PIPE_MGR_FREE((void *)counters);
  return PIPE_SUCCESS;
}

pipe_status_t bf_pkt_path_eprsr_drop_cntr_get(bf_dev_id_t devid,
                                              bf_dev_port_t port,
                                              bf_eprsr_cntrs_t *eprsr_cntrs) {
  bf_packetpath_counter_t *counters = NULL;
  int count = 0;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || (dev_info->dev_family != BF_DEV_FAMILY_TOFINO)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_api_exit(shdl);

  counters = bf_packet_path_parser_egress_counter_get(devid, port, &count);
  if (counters == NULL) {
    LOG_ERROR(
        "%s: bf_pkt_path_eprsr_read_counter returned NULL for "
        "requested on dev %d port %d",
        __func__,
        devid,
        port);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (int cntr_index = 0; cntr_index < count; cntr_index++) {
    switch (counters[cntr_index].description_index) {
      case EPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL0:
      case EPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL1:
      case EPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL2:
      case EPRSR_TOTAL_PKTS_DROPPED_ON_CHANNEL3:
        eprsr_cntrs->total_pkts_drop = counters[cntr_index].value;
        break;
      case EPRSR_FCS_ERROR:
        eprsr_cntrs->fcs_err_count = counters[cntr_index].value;
        break;
      case EPRSR_CHECKSUM_ERROR:
        eprsr_cntrs->csum_err_count = counters[cntr_index].value;
        break;
      case EPRSR_TCAM_PARITY_ERROR:
        eprsr_cntrs->tcam_parity_err_count = counters[cntr_index].value;
        break;
    }
  }

  PIPE_MGR_FREE((void *)counters);
  return PIPE_SUCCESS;
}

pipe_status_t bf_pkt_path_edprsr_drop_cntr_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, bf_edprsr_cntrs_t *edprsr_cntrs) {
  bf_packetpath_counter_t *counters = NULL;
  int count = 0;

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info || (dev_info->dev_family != BF_DEV_FAMILY_TOFINO)) {
    pipe_mgr_api_exit(shdl);
    return PIPE_NOT_SUPPORTED;
  }
  pipe_mgr_api_exit(shdl);

  counters = bf_packet_path_deparser_egress_counter_get(devid, pipe, &count);
  if (counters == NULL) {
    LOG_ERROR(
        "%s: bf_pkt_path_edprsr_counter returned NULL for pipe id "
        "requested on dev %d (%d)",
        __func__,
        devid,
        pipe);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (int cntr_index = 0; cntr_index < count; cntr_index++) {
    switch (counters[cntr_index].description_index) {
      case EDPRSR_PACKETS_DISCARDED:
        edprsr_cntrs->pkts_disc = counters[cntr_index].value;
        break;
      case EDPRSR_ERRORED_PACKETS_FROM_DEPRSR_TO_EBUF:
        edprsr_cntrs->err_pkts_to_ebuf = counters[cntr_index].value;
        break;
      case EDPRSR_ERRORED_PACKETS_FROM_EBUF_TO_ECTM:
        edprsr_cntrs->err_pkts_to_ectm = counters[cntr_index].value;
        break;
      default:
        if (counters[cntr_index].description_index >=
                EDPRSR_CRC_ERROR_ON_PACKETS_FROM_EBUF_0 &&
            counters[cntr_index].description_index <=
                EDPRSR_CRC_ERROR_ON_PACKETS_FROM_EBUF_71) {
          edprsr_cntrs->crc_err[counters[cntr_index].description_index -
                                EDPRSR_CRC_ERROR_ON_PACKETS_FROM_EBUF_0] =
              counters[cntr_index].value;
        }
        break;
    }
  }

  PIPE_MGR_FREE((void *)counters);
  return PIPE_SUCCESS;
}
