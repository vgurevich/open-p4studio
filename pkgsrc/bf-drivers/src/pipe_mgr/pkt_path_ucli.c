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
 *  This file contains display routines that spits out counters that packet
 *  walks through.
 */

#define __STDC_FORMAT_MACROS
#include <stddef.h>
#include <inttypes.h>
#include <getopt.h>
#include <lld/lld.h>
#include <lld/lld_sku.h>
#include "pipe_mgr_int.h"
#include <pipe_mgr/bf_packetpath_counter.h>
#include "pipe_mgr_tof_pkt_path_counter_display.h"
#include "pipe_mgr_tof2_pkt_path_counter_display.h"
#include "pipe_mgr_tof3_pkt_path_counter_display.h"
#include "pipe_mgr_rmt_cfg.h"
#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

static ucli_status_t pipr_mgr_ucli_get_pg_port_range(rmt_dev_info_t *dev_info,
                                                     int pipe,
                                                     int pg_port,
                                                     int *range) {
  bf_dev_port_t port_id = dev_info->dev_cfg.make_dev_port(pipe, pg_port);
  rmt_port_info_t *port_info = NULL;
  port_info = pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) {
    return UCLI_STATUS_E_NOT_FOUND;
  }
  if (range == NULL) return UCLI_STATUS_E_ERROR;
  if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
      (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    switch (port_info->speed) {
      case BF_SPEED_NONE:
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
      case BF_SPEED_40G:
      case BF_SPEED_40G_R2:
      case BF_SPEED_50G:
      case BF_SPEED_50G_CONS:
        *range = 1;
        break;
      case BF_SPEED_100G:
        *range = 2;
        break;
      case BF_SPEED_200G:
        *range = 4;
        break;
      case BF_SPEED_400G:
        *range = 8;
        break;
      default:
        return UCLI_STATUS_E_ERROR;
    }
  } else if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    switch (port_info->speed) {
      case BF_SPEED_1G:
      case BF_SPEED_10G:
      case BF_SPEED_25G:
        *range = 1;
        break;
      case BF_SPEED_50G:
        *range = 2;
        break;
      case BF_SPEED_40G:
      case BF_SPEED_100G:
        *range = 4;
        break;
      default:
        return UCLI_STATUS_E_ERROR;
    }
  } else {
    return UCLI_STATUS_E_ERROR;
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t pipr_mgr_ucli_parser_uc(ucli_context_t *uc,
                                             char *key_words,
                                             int *hex,
                                             int *all,
                                             int *devid,
                                             int *pipe,
                                             int *pg_port,
                                             int *zero) {
  int c;
  int argc, arg_start = 0;
  size_t i;
  char *const *argv;
  extern char *optarg;
  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       i++) {
    if (!strcmp(uc->pargs[0].args__[i], key_words)) {
      arg_start = i;
      break;
    }
  }
  if (i >= sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0])) {
    return UCLI_STATUS_E_NOT_FOUND;
  }
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;
  if (argc == 1) {
    if (devid != NULL) *devid = 0;
    if (pipe != NULL) *pipe = 0;
  } else {
    while (argv && (c = getopt(argc, argv, "AXzd:p:m:")) != -1) {
      switch (c) {
        case 'z':
          if (zero != NULL) *zero = 1;
          break;
        case 'd':
          if (devid != NULL) *devid = strtoul(optarg, NULL, 0);
          break;
        case 'p':
          if (pipe != NULL) *pipe = strtoul(optarg, NULL, 0);
          break;
        case 'm':
          if (pg_port != NULL) *pg_port = strtoul(optarg, NULL, 0);
          break;
        case 'X':
          if (hex != NULL) *hex = 1;
          break;
        case 'A':
          if (all != NULL) *all = 1;
          break;
        case ':':
        case '?':
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Usage: -d <dev_id> -p <logicalpipe> -m <port> -z [print "
                     "zero counters too] "
                     "-X [hex output] -A [all chnls in the port]");
          return UCLI_STATUS_E_ERROR;
        default:
          // Allows to run command without specifying any args.
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Usage: -d <dev_id> -p <logicalpipe> -m <port> -z [print "
                     "zero counters too] "
                     "-X [hex output] -A [all chnls in the port]");
          aim_printf(&uc->pvs, "%s\n", "Using device ID 0, pipeid 0, port 0");
          if (devid != NULL) *devid = 0;
          if (pipe != NULL) *pipe = 0;
          if (pg_port != NULL) *pg_port = 0;
      }
    }
  }
  if (devid != NULL && pipe != NULL) {
    bf_dev_pipe_t p_pipe = 0;
    if (lld_sku_map_pipe_id_to_phy_pipe_id(*devid, *pipe, &p_pipe)) {
      aim_printf(&uc->pvs, "Failed to map logical to physical pipe\n");
      return UCLI_STATUS_E_ERROR;
    }
    *pipe = p_pipe;
  }
  if ((pipe != NULL) && (*pipe > 7)) {
    return UCLI_STATUS_E_ERROR;
  }
  if ((pg_port != NULL) && (*pg_port > 71)) {
    return UCLI_STATUS_E_ERROR;
  }
  return UCLI_STATUS_OK;
}

/********************* Counter CLIs  ************************/
extern void tofino_nonzero_chip_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid);
extern void tofino_per_chip_counter_cli(ucli_context_t *uc, bf_dev_id_t devid);
extern void tofino_nonzero_pipe_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid,
                                            int pipe);
extern void tofino_pipe_counter_cli(ucli_context_t *uc,
                                    bf_dev_id_t devid,
                                    int pipe);
extern void tofino_nonzero_pipe_and_port_counter_cli(ucli_context_t *uc,
                                                     bf_dev_id_t devid,
                                                     int pipe,
                                                     int port);
extern void tofino_pipe_and_port_counter_cli(ucli_context_t *uc,
                                             bf_dev_id_t devid,
                                             int pipe,
                                             int port);

static ucli_status_t packet_path_ucli_ucli__ibuf__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "ibuf",
                    -1,
                    "  ibuf counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "ibuf", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_display_ibuf_counter(
              uc, hex, devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof2_pkt_path_display_ipb_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof3_pkt_path_display_ipb_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__iprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "iprsr",
                    -1,
                    "  ingress parser counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "iprsr", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_display_iprsr_counter(
              uc, hex, devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof2_pkt_path_display_iprsr_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof3_pkt_path_display_iprsr_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__eprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "eprsr",
                    -1,
                    "  egress parser counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "eprsr", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_display_eprsr_counter(
              uc, hex, devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof2_pkt_path_display_eprsr_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof3_pkt_path_display_eprsr_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__epb_ebuf__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "epb_ebuf",
                    -1,
                    "  egress parser buffer counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "epb_ebuf", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_display_ebuf_counter(
              uc, hex, devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof2_pkt_path_display_ebuf_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        pipe_mgr_tof2_pkt_path_display_epb_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof3_pkt_path_display_ebuf_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        pipe_mgr_tof3_pkt_path_display_epb_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__idprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int hex = 0;
  int show_zero_cntrs = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "idprsr",
                    -1,
                    "  ingress deparser counters"
                    "  Usage: -d <devid> -p <pipe> [-X output in hexadecimal]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "idprsr", &hex, NULL, &devid, &pipe, NULL, &show_zero_cntrs);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_display_idprsr_counter(uc, hex, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_display_idprsr_counter(
            uc, hex, !show_zero_cntrs, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_display_idprsr_counter(
            uc, hex, !show_zero_cntrs, devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}
static ucli_status_t packet_path_ucli_ucli__edprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int hex = 0;
  int show_zero_cntrs = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(
      uc,
      "edprsr",
      -1,
      "  egress deparser counters"
      "  Usage: -d <devid> -p <pipe> [-X output in hexadecimal ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "edprsr", &hex, NULL, &devid, &pipe, NULL, &show_zero_cntrs);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_display_edprsr_counter(uc, hex, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_display_edprsr_counter(
            uc, hex, !show_zero_cntrs, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_display_edprsr_counter(
            uc, hex, !show_zero_cntrs, devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__p2s__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "p2s",
                    -1,
                    "  p2s counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "p2s", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof2_pkt_path_display_p2s_counter(
              uc, hex, false, devid, pipe, pg_port, port_range);
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof3_pkt_path_display_p2s_counter(
              uc, hex, false, devid, pipe, pg_port, port_range);
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}
static ucli_status_t packet_path_ucli_ucli__s2p__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "s2p",
                    -1,
                    "  s2p counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "s2p", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof2_pkt_path_display_s2p_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        pipe_mgr_tof3_pkt_path_display_s2p_counter(
            uc, hex, false, devid, pipe, pg_port, port_range);
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}
static ucli_status_t packet_path_ucli_ucli__pmarb__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int hex = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "pmarb",
                    -1,
                    "  pmarb counters"
                    "  Usage: -d <devid> -p <pipe> [-X output in "
                    "hexadecimal ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "pmarb", &hex, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_display_pmarb_counter(
            uc, hex, false, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_display_pmarb_counter(
            uc, hex, false, devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__pipe_cnt__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int hex = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(
      uc,
      "pipe_cnt",
      -1,
      "  ingress and egress deparser counters"
      "  Usage: -d <devid> -p <pipe> [-X output in hexadecimal ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "pipe_cnt", &hex, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_display_pipe_counter(uc, hex, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_display_pipe_counter(uc, hex, devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_display_pipe_counter(uc, hex, devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__device__(ucli_context_t *uc) {
  int zero = 0;
  bf_dev_id_t devid = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "device",
                    -1,
                    "  All device - level counters"
                    "  Usage: -d <devid> -z <print zero counters too> [ -X "
                    "output in hexadecimal ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "device", NULL, NULL, &devid, NULL, NULL, &zero);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (zero) {
          tofino_per_chip_counter_cli(uc, devid);
        } else {
          tofino_nonzero_chip_counter_cli(uc, devid);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        if (zero) {
          pipe_mgr_tof2_per_chip_counter_cli(uc, devid);
        } else {
          pipe_mgr_tof2_nonzero_chip_counter_cli(uc, devid);
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        if (zero) {
          pipe_mgr_tof3_per_chip_counter_cli(uc, devid);
        } else {
          pipe_mgr_tof3_nonzero_chip_counter_cli(uc, devid);
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__pipe__(ucli_context_t *uc) {
  int zero = 0;
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(
      uc,
      "pipe",
      -1,
      "  All pipe - level counters"
      "  Usage: -d <devid> -p <pipe> -z <print zero counters too> ");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "pipe", NULL, NULL, &devid, &pipe, NULL, &zero);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (zero) {
          tofino_pipe_counter_cli(uc, devid, pipe);
        } else {
          tofino_nonzero_pipe_counter_cli(uc, devid, pipe);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        if (zero) {
          pipe_mgr_tof2_pipe_counter_cli(uc, devid, pipe);
        } else {
          pipe_mgr_tof2_nonzero_pipe_counter_cli(uc, devid, pipe);
        }
        break;
      case BF_DEV_FAMILY_TOFINO3:
        if (zero) {
          pipe_mgr_tof3_pipe_counter_cli(uc, devid, pipe);
        } else {
          pipe_mgr_tof3_nonzero_pipe_counter_cli(uc, devid, pipe);
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__port__(ucli_context_t *uc) {
  int zero = 0;
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(
      uc,
      "port",
      -1,
      "  All port - level counters in a pipe"
      "  Usage: -d <devid> -p <pipe> -m <port> -z <print "
      "zero counters too> [ -A output for all channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "port", NULL, &all, &devid, &pipe, &pg_port, &zero);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          if (zero) {
            tofino_pipe_and_port_counter_cli(uc, devid, pipe, port_tmp);
          } else {
            tofino_nonzero_pipe_and_port_counter_cli(uc, devid, pipe, port_tmp);
          }
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        if (zero) {
          pipe_mgr_tof2_pipe_and_port_counter_cli(
              uc, devid, pipe, pg_port, port_range);
        } else {
          pipe_mgr_tof2_nonzero_pipe_and_port_counter_cli(
              uc, devid, pipe, pg_port, port_range);
        }
        //}
        break;
      case BF_DEV_FAMILY_TOFINO3:
        // for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
        if (zero) {
          pipe_mgr_tof3_pipe_and_port_counter_cli(
              uc, devid, pipe, pg_port, port_range);
        } else {
          pipe_mgr_tof3_nonzero_pipe_and_port_counter_cli(
              uc, devid, pipe, pg_port, port_range);
        }
        //}
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_pipe__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_pipe",
                    -1,
                    "  Clear ingress and egress deparser counters"
                    "  Usage: -d <devid> -p <pipe>");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_pipe", NULL, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_clear_pipe_counter(devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_pipe_counter(devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_pipe_counter(devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_ibuf__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_ibuf",
                    -1,
                    "  Clear ibuf counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_ibuf", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_ipb_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_ipb_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_iprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_iprsr",
                    -1,
                    "  Clear ingress parser counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_iprsr", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_iprsr_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_iprsr_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_idprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_idprsr",
                    -1,
                    "  ingress deparser counters"
                    "  Usage: -d <devid> -p <pipe>");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_idprsr", NULL, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
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
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_eprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_eprsr",
                    -1,
                    "  Clear egress parser counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_eprsr", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_eprsr_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_eprsr_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_edprsr__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_edprsr",
                    -1,
                    "  egress deparser counters"
                    "  Usage: -d <devid> -p <pipe>");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_edprsr", NULL, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
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
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_epb_ebuf__(
    ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_epb_ebuf",
                    -1,
                    "  Clear egress parser buffer counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_epb_ebuf", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_clear_ebuf_counter(devid, pipe, port_tmp);
        }
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_ebuf_counter(
            devid, pipe, pg_port, port_range);
        pipe_mgr_tof2_pkt_path_clear_epb_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_ebuf_counter(
            devid, pipe, pg_port, port_range);
        pipe_mgr_tof3_pkt_path_clear_epb_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_pmarb__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_pmarb",
                    -1,
                    "  Clear Pmarb counters"
                    "  Usage: -d <devid> -p <pipe>");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_pmarb", NULL, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_pmarb_counter(devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_pmarb_counter(devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}
static ucli_status_t packet_path_ucli_ucli__clear_p2s__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_p2s",
                    -1,
                    "  Clear p2s counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_p2s", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_p2s_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_p2s_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}
static ucli_status_t packet_path_ucli_ucli__clear_s2p__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int all = 0;
  int port_range = 1;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_s2p",
                    -1,
                    "  Clear s2p counters"
                    "  Usage: -d <devid> -p <pipe> -m <port> [ -A clear all "
                    "channels in the port ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_s2p", NULL, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_s2p_counter(
            devid, pipe, pg_port, port_range);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_s2p_counter(
            devid, pipe, pg_port, port_range);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino2 supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__clear_all_pipe__(
    ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "clear_all_pipe",
                    -1,
                    "  Clear all port counters in data path"
                    "  Usage: -d <devid> -p <pipe>");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "clear_all_pipe", NULL, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_clear_all_counter(devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_pkt_path_clear_all_counter(devid, pipe);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_pkt_path_clear_all_counter(devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Wrong devid");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f bf_drv_show_tech_ucli_packet_path_handlers__[] = {
    packet_path_ucli_ucli__ibuf__,
    packet_path_ucli_ucli__iprsr__,
    packet_path_ucli_ucli__idprsr__,
    packet_path_ucli_ucli__eprsr__,
    packet_path_ucli_ucli__epb_ebuf__,
    packet_path_ucli_ucli__edprsr__,
    packet_path_ucli_ucli__pipe_cnt__,
    packet_path_ucli_ucli__s2p__,
    packet_path_ucli_ucli__p2s__,
    packet_path_ucli_ucli__pmarb__,
    packet_path_ucli_ucli__device__,
    packet_path_ucli_ucli__pipe__,
    packet_path_ucli_ucli__port__};

char *packet_path_cmd[] = {"ibuf",
                           "iprsr",
                           "idprsr",
                           "eprsr",
                           "epb_ebuf",
                           "edprsr",
                           "pipe_cnt",
                           "s2p",
                           "p2s",
                           "pmarb",
                           "device",
                           "pipe",
                           "port"};

ucli_status_t packet_path_ucli_show_tech_drivers(ucli_context_t *uc) {
  unsigned hdl_iter = 0;
  const char *restore_arg[UCLI_CONFIG_MAX_ARGS];
  int status_hdl = UCLI_STATUS_CONTINUE;
  *restore_arg = *uc->pargs[0].args__;
  unsigned hdl_num = sizeof(packet_path_cmd) / sizeof(packet_path_cmd[0]);
  while (hdl_iter < hdl_num) {
    if (bf_drv_show_tech_ucli_packet_path_handlers__[hdl_iter] &&
        packet_path_cmd[hdl_iter]) {
      *uc->pargs[0].args__ = packet_path_cmd[hdl_iter];
      status_hdl = bf_drv_show_tech_ucli_packet_path_handlers__[hdl_iter](uc);
      if (status_hdl == UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, " Status Handler return = %d   \n", status_hdl);
      }
      hdl_iter++;
    }
  }
  /* Restore to original. */
  *uc->pargs[0].args__ = *restore_arg;
  return 0;
}

/********************* Interrupt CLIs  ************************/
static ucli_status_t packet_path_ucli_ucli__intr_status__(ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int pg_port = 0;
  int hex = 0;
  int all = 0;
  int port_range = 1, port_tmp;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(uc,
                    "intr_status",
                    -1,
                    "  "
                    "Interrupts of parde blocks"
                    "  Usage: -d <devid> -p <pipe> -m <port> [-X output in "
                    "hexadecimal -A output for all channels in the port");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "intr_status", &hex, &all, &devid, &pipe, &pg_port, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    if (all == 1) {
      // get pg_port range
      sts =
          pipr_mgr_ucli_get_pg_port_range(dev_info, pipe, pg_port, &port_range);
      if (sts != UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Failed to get all channels in the port\n");
        return UCLI_STATUS_OK;
      }
    }
    port_range += pg_port;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        if (port_range > (4 + pg_port)) {
          aim_printf(&uc->pvs,
                     "%s\n",
                     "Failed to get all channels in the port for Tofino\n");
          return UCLI_STATUS_OK;
        }
        for (port_tmp = pg_port; port_tmp < port_range; port_tmp++) {
          pipe_mgr_tof_pkt_path_display_interrupts_intr(
              uc, hex, devid, pipe, port_tmp);
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_status_t packet_path_ucli_ucli__deparser_intr_status__(
    ucli_context_t *uc) {
  bf_dev_id_t devid = 0;
  int pipe = 0;
  int hex = 0;
  rmt_dev_info_t *dev_info = NULL;
  UCLI_COMMAND_INFO(
      uc,
      "deparser_intr_status",
      -1,
      "  Interrupts of parde blocks"
      "  Usage: -d <devid> -p <pipe> [-X output in hexadecimal ]");
  ucli_status_t sts = pipr_mgr_ucli_parser_uc(
      uc, "deparser_intr_status", &hex, NULL, &devid, &pipe, NULL, NULL);
  if (sts != UCLI_STATUS_OK) return UCLI_STATUS_OK;
  dev_info = pipe_mgr_get_dev_info(devid);
  if (dev_info != NULL) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_pkt_path_display_dprsr_interrupt_intr(
            uc, hex, devid, pipe);
        break;
      default:
        aim_printf(&uc->pvs, "%s\n", "Only Tofino supports.\n");
        return UCLI_STATUS_OK;
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f packet_path_counter_ucli_ucli_handlers__[] = {
    packet_path_ucli_ucli__ibuf__,
    packet_path_ucli_ucli__iprsr__,
    packet_path_ucli_ucli__idprsr__,
    packet_path_ucli_ucli__eprsr__,
    packet_path_ucli_ucli__epb_ebuf__,
    packet_path_ucli_ucli__edprsr__,
    packet_path_ucli_ucli__pipe_cnt__,
    packet_path_ucli_ucli__s2p__,
    packet_path_ucli_ucli__p2s__,
    packet_path_ucli_ucli__pmarb__,
    packet_path_ucli_ucli__device__,
    packet_path_ucli_ucli__pipe__,
    packet_path_ucli_ucli__port__,
    packet_path_ucli_ucli__clear_pipe__,
    packet_path_ucli_ucli__clear_ibuf__,
    packet_path_ucli_ucli__clear_iprsr__,
    packet_path_ucli_ucli__clear_idprsr__,
    packet_path_ucli_ucli__clear_eprsr__,
    packet_path_ucli_ucli__clear_edprsr__,
    packet_path_ucli_ucli__clear_epb_ebuf__,
    packet_path_ucli_ucli__clear_pmarb__,
    packet_path_ucli_ucli__clear_p2s__,
    packet_path_ucli_ucli__clear_s2p__,
    packet_path_ucli_ucli__clear_all_pipe__,
    packet_path_ucli_ucli__intr_status__,
    packet_path_ucli_ucli__deparser_intr_status__,
    NULL};

static ucli_module_t packet_path_counter_ucli_module__ = {
    "packet_path_ucli",
    NULL,
    packet_path_counter_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *packet_path_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&packet_path_counter_ucli_module__);
  m = ucli_node_create(
      "pkt_path_counter", n, &packet_path_counter_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("pkt_path_counter"));
  return n;
}

#else
void *packet_path_ucli_node_create(void) { return NULL; }
#endif
