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


/*******************************************************************************
 *
 *
 *
 *****************************************************************************/
#include <mc_mgr/mc_mgr_config.h>

#if MC_MGR_CONFIG_INCLUDE_UCLI == 1

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <pipe_mgr/pipe_mgr_mc_intf.h>

#include <mc_mgr/mc_mgr_intf.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_reg.h"
#include "mc_mgr_mem.h"
#include "mc_mgr_bh.h"
#include "mc_mgr_handle.h"

#include <tofino_regs/pipe_top_level.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof2_regs/tof2_mem_addr.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_mem_addr.h>
#include <errno.h>
#include <limits.h>

/* Maximum number of links the l1-read command will dump.
 * Enough for 256 LAG nodes and four L2 nodes. */
#define L1_READ_MAX_LINKS 260

#define MC_MGR_CLI_CMD_HNDLR(name) mc_mgr_ucli_ucli__##name##__
#define MC_MGR_CLI_CMD_DECLARE(name) \
  static ucli_status_t MC_MGR_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

#define MC_MGR_CLI_PROLOGUE(CMD, HELP, USAGE)                            \
  extern char *optarg;                                                   \
  extern int optind;                                                     \
  UCLI_COMMAND_INFO(uc, CMD, -1, HELP);                                  \
  char usage[] = "Usage: " USAGE "\n";                                   \
  int arg_start = 0;                                                     \
  size_t i;                                                              \
  for (i = 0;                                                            \
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]); \
       ++i) {                                                            \
    if (!strncmp(uc->pargs[0].args__[i], CMD, strlen(CMD))) {            \
      arg_start = i;                                                     \
      break;                                                             \
    }                                                                    \
  }                                                                      \
  int argc = uc->pargs->count + 1;                                       \
  if (1 == argc) {                                                       \
    aim_printf(&uc->pvs, "Usage: " USAGE "\n");                          \
    return UCLI_STATUS_OK;                                               \
  }                                                                      \
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);    \
  optind = 0;

MC_MGR_CLI_CMD_DECLARE(init) {
  UCLI_COMMAND_INFO(uc, "init", 0, "Initialize the MC driver.");
  bf_status_t sts = mc_mgr_init(true);
  aim_printf(&uc->pvs, "Status %s (%u)\n", bf_err_str(sts), sts);
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(set_in_prog) {
  UCLI_COMMAND_INFO(uc, "set-in-prog", 0, "Set the API-in-progress flag.");
  mc_mgr_one_at_a_time_begin();
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(clr_in_prog) {
  UCLI_COMMAND_INFO(uc, "clr-in-prog", 0, "Clear the API-in-progress flag.");
  mc_mgr_one_at_a_time_end();
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(show_sess) {
  UCLI_COMMAND_INFO(uc, "show-sess", 0, "Show driver sessions.");
  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (int i = 0; i < MC_MGR_NUM_SESSIONS; ++i)
    aim_printf(&uc->pvs,
               "Session %d Hdl %#x Valid %d\n",
               i,
               mc_mgr_ctx_session_state(i)->shdl,
               mc_mgr_ctx_session_state(i)->valid);
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(create_sess) {
  UCLI_COMMAND_INFO(uc, "create-sess", 0, "Create driver sessions.");
  bf_mc_session_hdl_t shdl = 0;
  bf_status_t sts = bf_mc_create_session(&shdl);
  if (BF_SUCCESS == sts) {
    aim_printf(&uc->pvs, "Created session w/ handle %#x\n", shdl);
  } else {
    aim_printf(
        &uc->pvs, "Session create fails, %s (%d)n", bf_err_str(sts), sts);
  }
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(destroy_sess) {
  MC_MGR_CLI_PROLOGUE("destroy-sess",
                      "Destroy a multicast driver session.",
                      "-s <session handle>")

  bf_mc_session_hdl_t shdl = -1;
  bool got_shdl = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  bf_status_t sts = bf_mc_destroy_session(shdl);
  aim_printf(&uc->pvs,
             "Session %#x destroy status: %s (%d)\n",
             shdl,
             bf_err_str(sts),
             sts);
  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mgrp_create) {
  MC_MGR_CLI_PROLOGUE("mgrp-create",
                      "Create a multicast group.",
                      "-s <session handle> -d <device> -i <id>")

  bf_mc_session_hdl_t shdl = -1;
  bool shdl_ = false;
  bf_dev_id_t dev = -1;
  int id = -1;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:i:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        shdl_ = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        id = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (0 > id || 0x10000 <= id || !shdl_) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_mc_mgrp_hdl_t ghdl;
  bf_status_t sts = bf_mc_mgrp_create(shdl, dev, id, &ghdl);
  aim_printf(
      &uc->pvs,
      "Create group %#x on device %d returned status %s and handle %#x\n",
      id,
      dev,
      bf_err_str(sts),
      ghdl);

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(mgrp_destroy) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "mgrp-destroy", -1, "Destroy a multicast group.");

  char usage[] = "Usage: -s <session handle> -d <device> -g <group handle>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "mgrp-destroy", strlen("mgrp-destroy"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_mc_session_hdl_t shdl = -1;
  bf_dev_id_t dev = 0;
  bf_mc_mgrp_hdl_t ghdl = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:g:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ghdl = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_mgrp_destroy(shdl, dev, ghdl);
  aim_printf(&uc->pvs,
             "Destroy group handle %#x returned status %s\n",
             ghdl,
             bf_err_str(sts));

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(node_create) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "node-create", -1, "Create a node.");

  char usage[] =
      "Usage: -s <session handle> -d <Device> -r <RID> [-p <port id>] [-l <LAG "
      "id>]\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "node-create", strlen("node-create"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_mc_session_hdl_t shdl = -1;
  bool got_sh = false;
  bf_dev_id_t dev = -1;
  bool got_dev = false;
  uint16_t rid = 0;
  bool got_rid = false;
  bf_mc_port_map_t ports;
  bf_mc_lag_map_t lags;
  BF_MC_PORT_MAP_INIT(ports);
  BF_MC_LAG_MAP_INIT(lags);
  int *port_args = NULL;
  int ports_len = 0;
  int ports_cnt = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:r:p:l:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'r':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        rid = strtoull(optarg, NULL, 0);
        got_rid = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        x = strtoull(optarg, NULL, 0);
        if (ports_len == ports_cnt) {
          ports_len += 10;
          port_args =
              MC_MGR_REALLOC(port_args, ports_len * sizeof port_args[0]);
        }
        port_args[ports_cnt] = x;
        ++ports_cnt;
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        x = strtoull(optarg, NULL, 0);
        if (0 > x || BF_LAG_COUNT <= x) {
          aim_printf(&uc->pvs, "%s", usage);
          goto done;
        }
        BF_MC_LAG_MAP_SET(lags, x);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        goto done;
    }
  }

  if (!got_dev || !got_sh || !got_rid) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  for (x = 0; x < ports_cnt; ++x) {
    if (!mc_dev_port_validate(dev, port_args[x])) {
      aim_printf(&uc->pvs, "Bad port (%d)\n", port_args[x]);
      aim_printf(&uc->pvs, "%s", usage);
      goto done;
    }
    BF_MC_PORT_MAP_SET(ports, port_args[x]);
  }

  bf_mc_node_hdl_t nhdl;
  bf_status_t sts = bf_mc_node_create(shdl, dev, rid, ports, lags, &nhdl);
  aim_printf(&uc->pvs,
             "Node created for dev %d with RID %#x returned status %s and node "
             "handle %#x\n",
             dev,
             rid,
             bf_err_str(sts),
             nhdl);

done:
  if (port_args) MC_MGR_FREE(port_args);
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(node_destroy) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "node-destroy", -1, "Destroy a node.");

  char usage[] = "Usage: -s <session handle> -d <dev> -n <node handle>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "node-destroy", strlen("node-destroy"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_mc_session_hdl_t shdl = -1;
  bf_dev_id_t dev = -1;
  bf_mc_node_hdl_t nhdl = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:n:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nhdl = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_node_destroy(shdl, dev, nhdl);
  aim_printf(&uc->pvs,
             "Destroy node %#x for dev %d returned status %s\n",
             nhdl,
             dev,
             bf_err_str(sts));

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(l1_associate) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "l1-associate", -1, "Associate an L1 node.");

  char usage[] =
      "Usage: -s <session handle> -d <device> -g <group handle> -n <node "
      "handle> [-x <xid>]\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "l1-associate", strlen("l1-associate"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_mc_session_hdl_t shdl = -1;
  bf_dev_id_t dev = 0;
  bf_mc_mgrp_hdl_t ghdl = -1;
  bf_mc_node_hdl_t nhdl = -1;
  uint16_t xid = 0;
  bool use_xid = false;

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:g:n:x:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ghdl = strtoull(optarg, NULL, 0);
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nhdl = strtoull(optarg, NULL, 0);
        break;
      case 'x':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        xid = strtoull(optarg, NULL, 0);
        use_xid = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_associate_node(shdl, dev, ghdl, nhdl, use_xid, xid);
  if (use_xid) {
    aim_printf(
        &uc->pvs,
        "Associate L1 node %#x to group %#x with XID %#x returned status %s\n",
        nhdl,
        ghdl,
        xid,
        bf_err_str(sts));
  } else {
    aim_printf(&uc->pvs,
               "Associate L1 node %#x to group %#x returned status %s\n",
               nhdl,
               ghdl,
               bf_err_str(sts));
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(l1_dissociate) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "l1-dissociate", -1, "Dissociate an L1 node.");

  char usage[] =
      "Usage: -s <session handle> -d <device> -g <group handle> -n <node "
      "handle>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(
            uc->pargs[0].args__[i], "l1-dissociate", strlen("l1-dissociate"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_mc_session_hdl_t shdl = -1;
  bf_dev_id_t dev = 0;
  bf_mc_mgrp_hdl_t ghdl = -1;
  bf_mc_node_hdl_t nhdl = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:g:n:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ghdl = strtoull(optarg, NULL, 0);
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nhdl = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_dissociate_node(shdl, dev, ghdl, nhdl);
  aim_printf(&uc->pvs,
             "Dissociate L1 node %#x from group %#x returned status %s\n",
             nhdl,
             ghdl,
             bf_err_str(sts));

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(l1_list_all) {
  MC_MGR_CLI_PROLOGUE(
      "l1-list-all", "List all node handles on device.", "-d <dev>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while ((x = getopt(argc, argv, "d:")) != -1) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);

  unsigned long key = 0;
  mc_l1_node_t *n = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;

  map_sts = bf_map_get_first(db, &key, (void **)&n);
  while (map_sts == BF_MAP_OK) {
    if (n && n->rid != 0xDEAD) {
      aim_printf(&uc->pvs, "Node %#lx RID 0x%04x", key, n->rid);
      if (n->mgid != -1)
        aim_printf(&uc->pvs, " MGID 0x%04x", n->mgid);
      else
        aim_printf(&uc->pvs, " MGID (none)");
      aim_printf(&uc->pvs, "\n");
    }
    map_sts = bf_map_get_next(db, &key, (void **)&n);
  }

  return UCLI_STATUS_OK;
}

/* Dumps a software L1 node */
MC_MGR_CLI_CMD_DECLARE(l1_dump) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "l1-dump", -1, "Dump an L1 node.");

  char usage[] = "Usage: -d <device id> -n <node handle>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "l1-dump", strlen("l1-dump"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bool got_node = false;
  bool got_dev = false;
  bf_mc_node_hdl_t nhdl = -1;
  bf_dev_id_t dev = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:n:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nhdl = strtoull(optarg, NULL, 0);
        got_node = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_node || !got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_l1_node_t *n = mc_mgr_lookup_l1_node(dev, nhdl, __func__, __LINE__);
  if (!n) {
    aim_printf(&uc->pvs, "Node %#x not found\n", nhdl);
    return UCLI_STATUS_OK;
  }
  aim_printf(&uc->pvs, "Dev %d\n", n->dev);
  aim_printf(&uc->pvs, "RID 0x%04x\n", n->rid);
  if (-1 != n->mgid)
    aim_printf(&uc->pvs, "MGID 0x%04x\n", n->mgid);
  else
    aim_printf(&uc->pvs, "MGID not associated\n");
  if (n->xid_valid)
    aim_printf(&uc->pvs, "XID 0x%04x\n", n->xid);
  else
    aim_printf(&uc->pvs, "XID not valid\n");
  aim_printf(&uc->pvs,
             "ECMP Group %#x (%p)\n",
             n->ecmp_grp ? n->ecmp_grp->handle : 0,
             (void *)n->ecmp_grp);
  int p = 0;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++p) {
    aim_printf(&uc->pvs,
               "Pipe%d Ports: %02" PRIx64 " %02x%02x%02x%02x%02x%02x%02x%02x\n",
               p,
               n->l2_chain.ports[p][1],
               (unsigned int)(n->l2_chain.ports[p][0] >> 56) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 48) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 40) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 32) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 24) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 16) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 8) & 0xFF,
               (unsigned int)(n->l2_chain.ports[p][0] >> 0) & 0xFF);
    aim_printf(&uc->pvs,
               "Pipe%d Rdm 0x%05x Self %p Prev %p Next %p\n",
               p,
               n->hw_nodes[p].rdm_addr,
               (void *)n->hw_nodes[p].sw_node,
               (void *)n->hw_nodes[p].prev,
               (void *)n->hw_nodes[p].next);
  }
  aim_printf(&uc->pvs,
             "LAGs[255:192] %02x%02x%02x%02x%02x%02x%02x%02x\n",
             (unsigned int)(n->l2_chain.lags[3] >> 56) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 48) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 40) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 32) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 24) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 16) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 8) & 0xFF,
             (unsigned int)(n->l2_chain.lags[3] >> 0) & 0xFF);
  aim_printf(&uc->pvs,
             "LAGs[191:128] %02x%02x%02x%02x%02x%02x%02x%02x\n",
             (unsigned int)(n->l2_chain.lags[2] >> 56) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 48) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 40) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 32) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 24) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 16) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 8) & 0xFF,
             (unsigned int)(n->l2_chain.lags[2] >> 0) & 0xFF);
  aim_printf(&uc->pvs,
             "LAGs[127: 64] %02x%02x%02x%02x%02x%02x%02x%02x\n",
             (unsigned int)(n->l2_chain.lags[1] >> 56) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 48) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 40) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 32) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 24) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 16) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 8) & 0xFF,
             (unsigned int)(n->l2_chain.lags[1] >> 0) & 0xFF);
  aim_printf(&uc->pvs,
             "LAGs[ 63:  0] %02x%02x%02x%02x%02x%02x%02x%02x\n",
             (unsigned int)(n->l2_chain.lags[0] >> 56) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 48) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 40) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 32) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 24) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 16) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 8) & 0xFF,
             (unsigned int)(n->l2_chain.lags[0] >> 0) & 0xFF);
  aim_printf(&uc->pvs,
             "Prev Hdl %#x, Next Hdl %#x\n",
             n->ecmp_prev ? n->ecmp_prev->handle : 0,
             n->ecmp_next ? n->ecmp_next->handle : 0);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(ecmp_dump) {
  MC_MGR_CLI_PROLOGUE(
      "ecmp-dump", "Dump an ECMP group.", "-h <ECMP Group Handle> -d <Device>");

  bool got_ecmp = false;
  bool got_dev = false;
  bf_mc_ecmp_hdl_t hdl = -1;
  bf_dev_id_t dev = 0;
  long long conv_result;
  char *unsupp_char = NULL;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (!(*unsupp_char)) got_dev = true;
        dev = (bf_dev_id_t)conv_result;
        break;
      case 'h':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        hdl = strtoull(optarg, NULL, 0);
        got_ecmp = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_ecmp || !got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, hdl, __func__, __LINE__);
  if (!g) {
    aim_printf(&uc->pvs, "ECMP %#x not found\n", hdl);
    return UCLI_STATUS_OK;
  }
  aim_printf(&uc->pvs, "ValidMap 0x%08x\n", g->valid_map);
  for (i = 0; i < sizeof(g->mbrs) / sizeof(g->mbrs[0]); ++i) {
    aim_printf(
        &uc->pvs, "Mbr%2d %#x\n", (int)i, g->mbrs[i] ? g->mbrs[i]->handle : 0);
  }

  aim_printf(&uc->pvs,
             "Mbr Block Size %d Bases[0:3] 0x%05x 0x%05x 0x%05x 0x%05x\n",
             g->allocated_sz,
             g->base[0],
             g->base[1],
             g->base[2],
             g->base[3]);
  aim_printf(&uc->pvs,
             "Vector Addr[0:3]0/1 0x%05x/0x%05x 0x%05x/0x%05x 0x%05x/0x%05x "
             "0x%05x/0x%05x\n",
             g->vector_node_addr[0][0],
             g->vector_node_addr[1][0],
             g->vector_node_addr[0][1],
             g->vector_node_addr[1][1],
             g->vector_node_addr[0][2],
             g->vector_node_addr[1][2],
             g->vector_node_addr[0][3],
             g->vector_node_addr[1][3]);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(ecmp_create) {
  MC_MGR_CLI_PROLOGUE("ecmp-create",
                      "Create an ECMP group.",
                      "-s <Session Handle> -d <Device>");

  bool got_dev = false;
  bool got_sh = false;
  bf_dev_id_t dev = 0;
  bf_mc_session_hdl_t sh = 0;
  bf_mc_ecmp_hdl_t hdl = 0;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  int x;
  while (-1 != (x = getopt(argc, argv, "s:d:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_ecmp_create(sh, dev, &hdl);
  aim_printf(&uc->pvs,
             "ECMP create by session %#x on dev %d returned status %s and ecmp "
             "handle %#x\n",
             sh,
             dev,
             bf_err_str(sts),
             hdl);
  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(ecmp_destroy) {
  MC_MGR_CLI_PROLOGUE("ecmp-destroy",
                      "Destroy an ECMP group.",
                      "-s <Session Handle> -d <Device> -g <ECMP Group>");

  bool got_eh = false;
  bool got_dev = false;
  bool got_sh = false;
  bf_mc_session_hdl_t sh = 0;
  bf_dev_id_t dev = 0;
  bf_mc_ecmp_hdl_t eh = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:g:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_eh || !got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_ecmp_destroy(sh, dev, eh);
  aim_printf(
      &uc->pvs,
      "ECMP Destroy by session %#x for ECMP group %#x returned status %s\n",
      sh,
      eh,
      bf_err_str(sts));
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_mbr_add) {
  MC_MGR_CLI_PROLOGUE(
      "ecmp-mbr-add",
      "Add a node to an ECMP group.",
      "-s <Session Handle> -d <Device> -e <ECMP Handle> -n <Node Handle>");

  bool got_sh = false;
  bool got_dev = false;
  bool got_eh = false;
  bool got_nh = false;
  bf_mc_session_hdl_t sh = 0;
  bf_dev_id_t dev = 0;
  bf_mc_ecmp_hdl_t eh = 0;
  bf_mc_node_hdl_t nh = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:e:n:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nh = strtoull(optarg, NULL, 0);
        got_nh = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_dev || !got_eh || !got_nh) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_ecmp_mbr_add(sh, dev, eh, nh);
  aim_printf(&uc->pvs,
             "Session %#x added node %#x to ECMP group %#x, status %s\n",
             sh,
             nh,
             eh,
             bf_err_str(sts));
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_mbr_rmv) {
  MC_MGR_CLI_PROLOGUE(
      "ecmp-mbr-rmv",
      "Remove a node from an ECMP group.",
      "-s <Session Handle> -d <Device> -e <ECMP Handle> -n <Node Handle>");

  bool got_sh = false;
  bool got_dev = false;
  bool got_eh = false;
  bool got_nh = false;
  bf_mc_session_hdl_t sh = 0;
  bf_dev_id_t dev = 0;
  bf_mc_ecmp_hdl_t eh = 0;
  bf_mc_node_hdl_t nh = 0;
  long long conv_result;
  char *unsupp_char = NULL;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:e:n:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (!(*unsupp_char)) got_dev = true;
        dev = (bf_dev_id_t)conv_result;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nh = strtoull(optarg, NULL, 0);
        got_nh = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_dev || !got_eh || !got_nh) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_ecmp_mbr_rem(sh, dev, eh, nh);
  aim_printf(&uc->pvs,
             "Session %#x remove node %#x from ECMP group %#x, status %s\n",
             sh,
             nh,
             eh,
             bf_err_str(sts));
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_associate) {
  MC_MGR_CLI_PROLOGUE("ecmp-associate",
                      "Add an ECMP group to a multicast group.",
                      "-s <Session Handle> -d <Device> -m <MGrp Handle> -e "
                      "<ECMP Handle> [-x <XID>]");

  bool got_sh = false;
  bool got_dev = false;
  bool got_mh = false;
  bool got_eh = false;
  bool got_xid = false;
  bf_mc_session_hdl_t sh = 0;
  bf_dev_id_t dev = 0;
  bf_mc_mgrp_hdl_t mh = 0;
  bf_mc_ecmp_hdl_t eh = 0;
  bf_mc_l1_xid_t xid = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:m:e:x:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mh = strtoull(optarg, NULL, 0);
        got_mh = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      case 'x':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        xid = strtoull(optarg, NULL, 0);
        got_xid = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_dev || !got_mh || !got_eh) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_associate_ecmp(sh, dev, mh, eh, got_xid, xid);
  if (got_xid) {
    aim_printf(&uc->pvs,
               "Session %#x added ECMP group %#x to multicast group %#x with "
               "xid %#x, status %s\n",
               sh,
               eh,
               mh,
               xid,
               bf_err_str(sts));
  } else {
    aim_printf(&uc->pvs,
               "Session %#x added ECMP group %#x to multicast group %#x with "
               "no xid, status %s\n",
               sh,
               eh,
               mh,
               bf_err_str(sts));
  }
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_dissociate) {
  MC_MGR_CLI_PROLOGUE(
      "ecmp-dissociate",
      "Remove an ECMP group from a multicast group.",
      "-s <Session Handle> -d <Device> -m <MGrp Handle> -e <ECMP Handle>");

  bool got_sh = false;
  bool got_dev = false;
  bool got_mh = false;
  bool got_eh = false;
  bf_mc_session_hdl_t sh = 0;
  bf_dev_id_t dev = 0;
  bf_mc_mgrp_hdl_t mh = 0;
  bf_mc_ecmp_hdl_t eh = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:m:e:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        sh = strtoull(optarg, NULL, 0);
        got_sh = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mh = strtoull(optarg, NULL, 0);
        got_mh = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_sh || !got_dev || !got_mh || !got_eh) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_dissociate_ecmp(sh, dev, mh, eh);
  aim_printf(&uc->pvs,
             "Session %#x removed ECMP group %#x from multicast group %#x, "
             "status %s\n",
             sh,
             eh,
             mh,
             bf_err_str(sts));
  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_modify) {
  MC_MGR_CLI_PROLOGUE("ecmp-modify",
                      "Change values in an ECMP group (SW only).",
                      "-d <device_id> -e <ECMP Handle> [-x <Member bit map>] "
                      "[-i <mbr Idx> -m <Node Handle>] [-p <pipe> "
                      "-b <Mbr Base>] [-v <Version> -p <pipe> "
                      "-a <Vector Addr>] [-z <allocated size>] "
                      "[-n <Node Handle for refs>]");

  bool got_eh = false;
  bool got_bm = false;
  bool got_idx = false;
  bool got_mbr = false;
  bool got_pipe = false;
  bool got_mbr_base = false;
  bool got_ver = false;
  bool got_vec_addr = false;
  bool got_size = false;
  bool got_ref_hdl = false;
  bool got_dev = false;

  bf_mc_ecmp_hdl_t eh = 0;
  uint32_t bm = 0;
  int idx = 0;
  bf_mc_node_hdl_t mbr = 0;
  bf_dev_pipe_t pipe = 0;
  uint32_t mbr_base = 0;
  int ver = 0;
  uint32_t vec_addr = 0;
  int size = 0;
  bf_mc_node_hdl_t ref = 0;
  bf_dev_id_t dev = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:e:x:i:m:p:b:v:a:z:n:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        eh = strtoull(optarg, NULL, 0);
        got_eh = true;
        break;
      case 'x':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        bm = strtoull(optarg, NULL, 0);
        got_bm = true;
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        idx = strtoull(optarg, NULL, 0);
        got_idx = true;
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mbr = strtoull(optarg, NULL, 0);
        got_mbr = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      case 'b':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mbr_base = strtoull(optarg, NULL, 0);
        got_mbr_base = true;
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ver = strtoull(optarg, NULL, 0);
        got_ver = true;
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        vec_addr = strtoull(optarg, NULL, 0);
        got_vec_addr = true;
        break;
      case 'z':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        size = strtoull(optarg, NULL, 0);
        got_size = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ref = strtoull(optarg, NULL, 0);
        got_ref_hdl = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  bool set_bm = got_bm;
  bool set_mbr = got_idx && got_mbr;
  bool set_base = got_pipe && got_mbr_base;
  bool set_vec_base = got_pipe && got_ver && got_vec_addr;
  bool set_size = got_size;
  bool set_ref = got_ref_hdl;
  if (!got_dev || !got_eh ||
      !(set_bm || set_mbr || set_base || set_vec_base || set_ref)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (got_idx && !(idx >= 0 && idx <= 31)) {
    aim_printf(&uc->pvs, "invalid index %d\n", idx);
    return UCLI_STATUS_OK;
  }
  if (got_ver && !(ver == 1 || ver == 0)) {
    aim_printf(&uc->pvs, "invalid version %d\n", ver);
    return UCLI_STATUS_OK;
  }
  if (got_pipe && !(pipe < 3)) {
    aim_printf(&uc->pvs, "invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }
  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, eh, __func__, __LINE__);
  if (!g) {
    aim_printf(&uc->pvs, "ECMP %#x not found\n", eh);
    return UCLI_STATUS_OK;
  }
  mc_l1_node_t *new_mbr = NULL;
  if (set_mbr && (mbr != 0)) {
    new_mbr = mc_mgr_lookup_l1_node(dev, mbr, __func__, __LINE__);
    if (!new_mbr) {
      aim_printf(&uc->pvs, "Node %#x not found\n", mbr);
      return UCLI_STATUS_OK;
    }
  }
  mc_l1_node_t *new_ref = NULL;
  if (set_ref && (ref != 0)) {
    new_ref = mc_mgr_lookup_l1_node(dev, ref, __func__, __LINE__);
    if (!new_ref) {
      aim_printf(&uc->pvs, "Node %#x not found\n", mbr);
      return UCLI_STATUS_OK;
    }
  }

  if (set_bm) {
    g->valid_map = bm;
  }
  if (set_mbr) {
    g->mbrs[idx] = new_mbr;
  }
  if (set_base) {
    g->base[pipe] = mbr_base;
  }
  if (set_vec_base) {
    g->vector_node_addr[ver][pipe] = vec_addr;
  }
  if (set_size) {
    g->allocated_sz = size;
  }
  if (set_ref) {
    g->refs = new_ref;
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(ecmp_list_all_handles) {
  MC_MGR_CLI_PROLOGUE("ecmp-list-all-handles",
                      "Display all ECMP handles on device.",
                      "-d <Device>");

  bool got_dev = false;
  bf_dev_id_t dev = 0;
  long conv_result;
  int x;
  char *unsupp_char = NULL;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtol(optarg, &unsupp_char, 0);

        if ((unsupp_char == optarg) || (*unsupp_char != '\0') ||
            (conv_result < 0) || (conv_result > INT_MAX)) {
          aim_printf(&uc->pvs, "%s", "Invalid device identifier\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }

        got_dev = true;
        dev = conv_result;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_mc_ecmp_hdl_t eh = 0;
  mc_ecmp_grp_t *g = NULL;
  bf_map_t *db = mc_mgr_ctx_db_ecmp(dev);
  bf_map_sts_t msts = BF_MAP_OK;

  unsigned long key = eh;
  msts = bf_map_get_first(db, &key, (void **)&g);
  while (BF_MAP_OK == msts) {
    mc_mgr_decode_ecmp_hdl(key, __func__, __LINE__);
    aim_printf(&uc->pvs, "ECMP Handle %#lx on device %d\n", key, dev);
    msts = bf_map_get_next(db, &key, (void **)&g);
  }

  return UCLI_STATUS_OK;
}

/**
 * Dumps a decoded RDM node.
 *
 * The line buffer may contain up to two nodes. We dump the node
 * specified by the low-order bit of the RDM address.
 *
 * @param addr RDM address
 * @param line Decoded RMD line
 * @param pvs  Output stream
 *
 * @return 0 if successful, -1 if failure
 */
static int dump_rdm_node(uint32_t addr,
                         const mc_mgr_rdm_line_t *line,
                         aim_pvs_t *pvs) {
  switch (line->type[addr & 1]) {
    case mc_mgr_rdm_node_type_invalid:
      aim_printf(pvs, "Invalid\n");
      return -1;
    case mc_mgr_rdm_node_type_rid:
      /* L1 RID */
      aim_printf(pvs,
                 "L1-Rid Rid 0x%04x L2 0x%05x Next 0x%05x\n",
                 line->u.rid.rid,
                 line->u.rid.next_l2,
                 line->u.rid.next_l1);
      break;
    case mc_mgr_rdm_node_type_xid:
      /* L1 RID XID */
      aim_printf(pvs,
                 "L1-Xid Rid 0x%04x Xid 0x%04x L2 0x%05x Next 0x%05x\n",
                 line->u.xid.rid,
                 line->u.xid.xid,
                 line->u.xid.next_l2,
                 line->u.xid.next_l1);
      break;
    case mc_mgr_rdm_node_type_end:
      /* L1 RID no L1_next */
      aim_printf(pvs,
                 "L1-End Rid 0x%04x L2 0x%05x\n",
                 line->u.end[addr & 1].rid,
                 line->u.end[addr & 1].next_l2);
      break;
    case mc_mgr_rdm_node_type_ecmp:
      /* L1 ECMP Pointer */
      aim_printf(pvs,
                 "ECMP-Ptr ptr0 0x%05x ptr1 0x%05x Next 0x%05x\n",
                 line->u.ecmp.vector0,
                 line->u.ecmp.vector1,
                 line->u.ecmp.next_l1);
      break;
    case mc_mgr_rdm_node_type_ecmp_xid:
      /* L1 ECMP Pointer XID */
      aim_printf(pvs,
                 "ECMP-PtrXid Xid 0x%04x ptr0 0x%05x ptr1 0x%05x Next 0x%05x\n",
                 line->u.ecmp_xid.xid,
                 line->u.ecmp_xid.vector0,
                 line->u.ecmp_xid.vector1,
                 line->u.ecmp_xid.next_l1);
      break;
    case mc_mgr_rdm_node_type_vector:
      /* L1 ECMP Vector */
      aim_printf(pvs,
                 "ECMP-Vec  Base 0x%05x Vec 0x%08x Len %d\n",
                 line->u.vector.base_l1,
                 line->u.vector.vector,
                 line->u.vector.length);
      break;
    case mc_mgr_rdm_node_type_port18:
      /* L2 Port 18 */
      aim_printf(pvs,
                 "L2-Port18 Pipe %d Last %s spv 0x%x npv 0x%04x\n",
                 line->u.port18[addr & 1].pipe,
                 line->u.port18[addr & 1].last ? "Y" : "N",
                 line->u.port18[addr & 1].spv,
                 line->u.port18[addr & 1].ports);
      break;
    case mc_mgr_rdm_node_type_port72:
      /* L2 Port 72 */
      aim_printf(pvs,
                 "L2-Port72 Pipe %d Last %s spv 0x%02x npv 0x%016" PRIx64 "\n",
                 line->u.port72.pipe,
                 line->u.port72.last ? "Y" : "N",
                 line->u.port72.spv,
                 line->u.port72.ports);
      break;
    case mc_mgr_rdm_node_type_lag:
      /* L2 LAG */
      aim_printf(pvs,
                 "L2-LAG LagId %3d Next 0x%05x\n",
                 line->u.lag[addr & 1].lag_id,
                 line->u.lag[addr & 1].next_l2);
      break;
    default:
      aim_printf(pvs, "Unknown Node Type\n");
      return -1;
  }
  return 0;
}

/**
 * Reads an RDM line from HW or Shadow RAM.
 *
 * @param dev  Device identifier
 * @param addr RDM address
 * @param hw   Whether to read from HW
 * @param line RDM line buffer
 * @param pvs  Output stream
 *
 * @return BF_SUCCESS if successful
 */
static bf_status_t read_rdm_line(bf_dev_id_t dev,
                                 uint32_t addr,
                                 bool hw,
                                 mc_mgr_rdm_line_t *line,
                                 aim_pvs_t *pvs) {
  /* Zero the line buffer */
  MC_MGR_MEMSET(line, 0, sizeof(*line));

  if (hw) {
    /* Read from hardware */
    bf_status_t bfs;
    bfs = mc_mgr_get_rdm_reg(0, dev, addr / 2, &line->data[1], &line->data[0]);
    if (bfs != BF_SUCCESS) {
      aim_printf(
          pvs, "Failed to read RDM %05x from HW (%s)\n", addr, bf_err_str(bfs));
      return bfs;
    }
  } else {
    /* Read from shadow RAM */
    mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
    if ((addr / 2) >= rdm_map->rdm_line_count || addr == 0) {
      aim_printf(pvs, "Invalid RDM address %05x\n", addr);
      return BF_INVALID_ARG;
    }
    line->data[0] = rdm_map->rdm[addr / 2].data[0];
    line->data[1] = rdm_map->rdm[addr / 2].data[1];
  }
  return BF_SUCCESS;
}

MC_MGR_CLI_CMD_DECLARE(rdm_read) {
  MC_MGR_CLI_PROLOGUE(
      "rdm-read", "Read the RDM memory.", "-h -d <Device> -a <Address>");

  bool got_dev = false;
  bool got_addr = false;
  bf_dev_id_t dev = 0;
  uint32_t addr = 0;
  bool hw = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:a:h"))) {
    switch (x) {
      case 'h':
        hw = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        addr = strtoull(optarg, NULL, 0);
        got_addr = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_addr) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (addr > 0xFFFFF) {
    aim_printf(&uc->pvs, "Invalid addr %#x\n", addr);
    return UCLI_STATUS_OK;
  }

  mc_mgr_rdm_line_t line;
  bf_status_t bfs = read_rdm_line(dev, addr, hw, &line, &uc->pvs);
  if (bfs != BF_SUCCESS) return UCLI_STATUS_OK;
  mc_mgr_rdm_decode_line(dev, &line);

  aim_printf(&uc->pvs,
             "%04x %016" PRIx64 "\n",
             (unsigned int)line.data[1] & 0xFFFF,
             line.data[0]);
  dump_rdm_node(addr, &line, &uc->pvs);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(rdm_alloc_dump) {
  MC_MGR_CLI_PROLOGUE(
      "rdm-alloc-dump", "Dump RDM Block Assignment.", "[-h] -d <device>");

  bool got_dev = false;
  bf_dev_id_t dev = 0;
  bool hw = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:h"))) {
    switch (x) {
      case 'h':
        hw = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_mgr_rdm_t *r = mc_mgr_ctx_rdm_map(dev);
  aim_printf(&uc->pvs,
             "Free Bit Map [203..0] %08x %08x %08x %08x %08x %08x %08x %08x\n",
             (uint32_t)(r->free_blocks_[3] >> 32),
             (uint32_t)(r->free_blocks_[3] & 0xFFFFFFFF),
             (uint32_t)(r->free_blocks_[2] >> 32),
             (uint32_t)(r->free_blocks_[2] & 0xFFFFFFFF),
             (uint32_t)(r->free_blocks_[1] >> 32),
             (uint32_t)(r->free_blocks_[1] & 0xFFFFFFFF),
             (uint32_t)(r->free_blocks_[0] >> 32),
             (uint32_t)(r->free_blocks_[0] & 0xFFFFFFFF));
  int p;
  for (p = 0; p < 4; ++p) {
    aim_printf(
        &uc->pvs,
        "Pipe %d L1s [203..0]   %08x %08x %08x %08x %08x %08x %08x %08x\n",
        p,
        (uint32_t)(r->used_blocks_[p][0][3] >> 32),
        (uint32_t)(r->used_blocks_[p][0][3] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][0][2] >> 32),
        (uint32_t)(r->used_blocks_[p][0][2] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][0][1] >> 32),
        (uint32_t)(r->used_blocks_[p][0][1] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][0][0] >> 32),
        (uint32_t)(r->used_blocks_[p][0][0] & 0xFFFFFFFF));
    aim_printf(
        &uc->pvs,
        "Pipe %d L2s [203..0]   %08x %08x %08x %08x %08x %08x %08x %08x\n",
        p,
        (uint32_t)(r->used_blocks_[p][1][3] >> 32),
        (uint32_t)(r->used_blocks_[p][1][3] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][1][2] >> 32),
        (uint32_t)(r->used_blocks_[p][1][2] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][1][1] >> 32),
        (uint32_t)(r->used_blocks_[p][1][1] & 0xFFFFFFFF),
        (uint32_t)(r->used_blocks_[p][1][0] >> 32),
        (uint32_t)(r->used_blocks_[p][1][0] & 0xFFFFFFFF));
  }
  aim_printf(&uc->pvs,
             "Assignment SW [203..0] %08x %08x %08x %08x %08x %08x %08x %08x "
             "%08x %08x %08x %08x %08x\n",
             r->blk_ids[12],
             r->blk_ids[11],
             r->blk_ids[10],
             r->blk_ids[9],
             r->blk_ids[8],
             r->blk_ids[7],
             r->blk_ids[6],
             r->blk_ids[5],
             r->blk_ids[4],
             r->blk_ids[3],
             r->blk_ids[2],
             r->blk_ids[1],
             r->blk_ids[0]);
  if (hw) {
    uint32_t hw_ids[13] = {0};
    int idx;
    for (idx = 0; idx < 13; ++idx) {
      mc_mgr_get_blk_id_grp_hw(dev, idx, &hw_ids[idx]);
    }
    aim_printf(&uc->pvs,
               "Assignment HW [203..0] %08x %08x %08x %08x %08x %08x %08x %08x "
               "%08x %08x %08x %08x %08x\n",
               hw_ids[12],
               hw_ids[11],
               hw_ids[10],
               hw_ids[9],
               hw_ids[8],
               hw_ids[7],
               hw_ids[6],
               hw_ids[5],
               hw_ids[4],
               hw_ids[3],
               hw_ids[2],
               hw_ids[1],
               hw_ids[0]);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mgrp_show) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "mgrp-show", -1, "Show group allocation.");

  char usage[] = "Usage: -d <device> [-s <start MGID>] [-e <end MGID>]\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "mgrp-show", strlen("mgrp-show"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  int first = 0, last = 0xFFFF;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:s:e:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last || last > 0xFFFF || 0 > first) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  first /= 256;
  last /= 256;
  for (x = first; x <= last; ++x) {
    aim_printf(&uc->pvs,
               "MGID[0x%04x...0x%04x] = 0x%016" PRIx64 " 0x%016" PRIx64
               " 0x%016" PRIx64 " 0x%016" PRIx64 "\n",
               x * 256 + 255,
               x * 256,
               mc_mgr_ctx_mgid_blk(dev, x * 4 + 3),
               mc_mgr_ctx_mgid_blk(dev, x * 4 + 2),
               mc_mgr_ctx_mgid_blk(dev, x * 4 + 1),
               mc_mgr_ctx_mgid_blk(dev, x * 4 + 0));
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mgrp_set) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "mgrp-set", -1, "Mark an MGID as used.");

  char usage[] = "Usage: -d <device> -g <MGID>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "mgrp-set", strlen("mgrp-set"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  int grp = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:g:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        grp = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (grp > 0xFFFF || 0 > grp) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_mgr_set_mgid_map_bit(dev, grp, 1);
  aim_printf(&uc->pvs, "Setting MGID %#x as used on device %d\n", grp, dev);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mgrp_clr) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "mgrp-clr", -1, "Mark an MGID as free.");

  char usage[] = "Usage: -d <device> -g <MGID>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "mgrp-clr", strlen("mgrp-clr"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  int grp = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:g:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        grp = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (grp > 0xFFFF || 0 > grp) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_mgr_set_mgid_map_bit(dev, grp, 0);
  aim_printf(&uc->pvs, "Setting MGID %#x as free on device %d\n", grp, dev);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(pvt_rd) {
  MC_MGR_CLI_PROLOGUE("pvt-rd",
                      "Read the PVT table.",
                      "[-h] -d <device> [-s <start MGID>] [-e <end MGID>] [-t "
                      "<0 or 1 for specific pvt>]");

  bf_dev_id_t dev = 0;
  int first = 0, last = 0xFFFF;
  bool hw = false;
  uint32_t table = 0;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:s:e:ht:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      case 't':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        table = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last || last > 0xFFFF || 0 > first || table > 1) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  for (x = first; x <= last; ++x) {
    uint8_t pvt[MC_MGR_NUM_PIPES + 1];
    int pipe = 0;
    pvt[0] = mc_mgr_get_pvt(dev, x);
    if (hw) {
      bool err = false;
      uint64_t pvt_tmp;
      aim_printf(&uc->pvs, "PVT[0x%04x] = shadow: 0x%x pipes:", x, pvt[0]);

      /* Due to HW implementation of PVT in Tofino1, the pair of SW values are
       * used for comparision. HW table is 32K, hence the per pair of MGIDs
       * are used, one with upper bit set and !set.
       */
      for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
        mc_mgr_pvt_entry_t val = {0};
        switch (mc_mgr_ctx_dev_family(dev)) {
          case BF_DEV_FAMILY_TOFINO:
            pvt[0] = (mc_mgr_get_pvt(dev, x & 0x7FFF) |
                      mc_mgr_get_pvt(dev, x | 0x8000));
            mc_mgr_get_pvt_reg(dev, pipe, table, x >> 3, &val);
            pvt_tmp = (val.d >> (4 * (x & 7))) & 0xFllu;
            pvt[1 + pipe] = (uint8_t)pvt_tmp;
            err = err || (pvt[pipe] != pvt[1 + pipe]);
            break;
          case BF_DEV_FAMILY_TOFINO2:
            mc_mgr_get_pvt_reg(dev, pipe, table, x >> 2, &val);
            pvt_tmp = (val.d >> (5 * (x & 3))) & 0x1Fllu;
            pvt[1 + pipe] = (uint8_t)pvt_tmp;
            err = err || (pvt[pipe] != pvt[1 + pipe]);
            break;
          case BF_DEV_FAMILY_TOFINO3:
            mc_mgr_get_pvt_reg(dev, pipe, 0, x >> 3, &val);
            pvt_tmp = (val.d >> (8 * (x & 7))) & 0xFFllu;
            pvt[1 + pipe] = (uint8_t)pvt_tmp;
            err = err || (pvt[pipe] != pvt[1 + pipe]);
            break;
          default:
            aim_printf(&uc->pvs,
                       "Unexpected chip family %d for dev %d\n",
                       mc_mgr_ctx_dev_family(dev),
                       dev);
            MC_MGR_DBGCHK(0);
            return UCLI_STATUS_OK;
        }
        aim_printf(&uc->pvs, " 0x%x", pvt[1 + pipe]);
      }
      aim_printf(&uc->pvs, "%s\n", err ? " ERROR, HW != shadow" : "");
    } else {
      aim_printf(&uc->pvs, "PVT[0x%04x] = 0x%x\n", x, pvt[0]);
    }
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(pvt_wr) {
  MC_MGR_CLI_PROLOGUE("pvt-wr",
                      "Write the PVT table.",
                      "-s <session> -d <device> -g <MGID> -m <mask> [-t <0 or "
                      "1 for specific pvt>]");

  bf_dev_id_t dev = 0;
  int grp = 0, mask = 0;
  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  uint32_t tbl_msk = MC_PVT_MASK_ALL;
#define MC_MGR_PVT_BOTH 2
  uint32_t table = MC_MGR_PVT_BOTH;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:g:m:t:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        grp = strtoull(optarg, NULL, 0);
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        mask = strtoull(optarg, NULL, 0);
        break;
      case 't':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        table = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || grp > 0xFFFF || 0 > grp || mask > 0xF || 0 > mask ||
      table > MC_MGR_PVT_BOTH) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    aim_printf(&uc->pvs, "Invalid session %#x", shdl);
    return UCLI_STATUS_OK;
  }
  if (table != MC_MGR_PVT_BOTH) {
    tbl_msk = 1 << table;
  }

  aim_printf(
      &uc->pvs, "Setting PVT to %#x on dev %d for group %#x\n", mask, dev, grp);
  bf_status_t sts = mc_mgr_program_pvt(
      sid, dev, grp, mask, tbl_msk, false, __func__, __LINE__);
  if (BF_SUCCESS != sts) {
    aim_printf(&uc->pvs, "Error programming pvt %s (%d)", bf_err_str(sts), sts);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(tvt_rd) {
  MC_MGR_CLI_PROLOGUE("tvt-rd",
                      "Read the TVT table.",
                      "[-h] -d <device> [-s <start MGID>] [-e <end MGID>]");

  bf_dev_id_t dev = 0;
  int first = 0, last = 0xFFFF;
  int x;
  bool got_dev = false;
  bool from_hw = false;
  bool err = false;  // Indicates inconsistency between shadow mem and pipes
  long long conv_result;
  char *unsupp_char = NULL;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:hs:e:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        got_dev = true;
        dev = (bf_dev_id_t)conv_result;
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = (int)conv_result;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = (int)conv_result;
        break;
      case 'h':
        from_hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || first > last || last > 0xFFFF || 0 > first) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    aim_printf(&uc->pvs,
               "Unexpected chip family %d for dev %d\n",
               mc_mgr_ctx_dev_family(dev),
               dev);
    return UCLI_STATUS_OK;
  }

  for (x = first; x <= last; ++x) {
    uint8_t tvt[MC_MGR_NUM_PIPES + 1];
    int pipe = 0;
    tvt[0] = mc_mgr_get_tvt(dev, x);
    if (from_hw) {
      aim_printf(&uc->pvs, "TVT[0x%04x] = shadow:0x%x pipes:", x, tvt[0]);
      for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
        tvt[1 + pipe] = mc_mgr_get_tvt_hw(dev, pipe, x);
        if (tvt[1 + pipe] != tvt[0]) err = true;
        aim_printf(&uc->pvs, " 0x%x", tvt[1 + pipe]);
      }
      if (err)
        aim_printf(&uc->pvs, " ERROR, HW != shadow\n");
      else
        aim_printf(&uc->pvs, "\n");
    } else
      aim_printf(&uc->pvs, "TVT[0x%04x] = 0x%x\n", x, tvt[0]);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(tvt_wr) {
  MC_MGR_CLI_PROLOGUE("tvt-wr",
                      "Write the TVT table.",
                      "-s <session> -d <device> -g <MGID> -m <mask>");

  bf_dev_id_t dev = 0;
  int grp = 0, mask = 0;
  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  int x;
  bool got_dev = false, got_grp = false, got_mask = false;
  long long conv_result;
  char *unsupp_char = NULL;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:g:m:t:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        got_shdl = true;
        shdl = (bf_mc_session_hdl_t)conv_result;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        got_dev = true;
        dev = (bf_dev_id_t)conv_result;
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        got_grp = true;
        grp = (int)conv_result;
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        errno = 0;
        conv_result = strtoll(optarg, &unsupp_char, 0);
        if ((conv_result > INT_MAX) || (errno && conv_result == 0) ||
            (conv_result < 0)) {
          aim_printf(&uc->pvs, "%s", "Number out of range\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        if (*unsupp_char) {
          aim_printf(&uc->pvs, "%s", "Invalid number\n");
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        got_mask = true;
        mask = (int)conv_result;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_shdl || !got_grp || !got_mask || grp > 0xFFFF ||
      0 > grp || mask > 0xF || 0 > mask) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    aim_printf(&uc->pvs, "Invalid session %#x", shdl);
    return UCLI_STATUS_OK;
  }

  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    aim_printf(&uc->pvs,
               "Unexpected chip family %d for dev %d\n",
               mc_mgr_ctx_dev_family(dev),
               dev);
    return UCLI_STATUS_OK;
  }

  aim_printf(
      &uc->pvs, "Setting TVT to %#x on dev %d for group %#x\n", mask, dev, grp);

  bf_status_t sts =
      mc_mgr_program_tvt(sid, dev, grp, mask, false, __func__, __LINE__);

  if (BF_SUCCESS != sts) {
    aim_printf(&uc->pvs, "Error programming pvt %s (%d)", bf_err_str(sts), sts);
  }
  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(lit_rd) {
  MC_MGR_CLI_PROLOGUE(
      "lit-rd",
      "Read the LIT table.",
      "[-h] -d <device> -v <table version> [-s <start LAG>] [-e <end LAG>]");

  bf_dev_id_t dev = 0;
  int first = 0, last = 0xFF;
  bool hw = false, ver = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:v:s:e:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ver = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last || last > 0xFF || 0 > first) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;
  for (x = first; x <= last; ++x) {
    bf_bitset_t lit_sw[MC_MGR_NUM_PIPES];
    bf_bitset_t lit_hw[MC_MGR_NUM_PIPES];
    uint64_t lit_sw_[MC_MGR_NUM_PIPES]
                    [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    uint64_t lit_hw_[MC_MGR_NUM_PIPES]
                    [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    int p;
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      bf_bs_init(&lit_sw[p], BF_PIPE_PORT_COUNT, lit_sw_[p]);
      bf_bs_init(&lit_hw[p], BF_PIPE_PORT_COUNT, lit_hw_[p]);
    }
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      bf_bs_copy(&lit_sw[p], mc_mgr_ctx_lit(dev, x, p));
    }

    if (hw) {
      sts = mc_mgr_get_lag_hw(dev, ver, x, lit_hw);
      if (BF_SUCCESS != sts) {
        aim_printf(&uc->pvs,
                   "Failed to read LAG %d from HW, %s (%d)\n",
                   x,
                   bf_err_str(sts),
                   sts);
        continue;
      }

      bool error = true;
      for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
        error = !(error && bf_bs_equal(&lit_sw[p], &lit_hw[p]));
      }

      p = (int)mc_mgr_ctx_num_max_pipes(dev) - 1;
      aim_printf(&uc->pvs,
                 "LIT[%3d] = 0x%02" PRIx64 "_%016" PRIx64,
                 x,
                 bf_bs_get_word(&lit_hw[p], 64, 8),
                 bf_bs_get_word(&lit_hw[p], 0, 64));
      do {
        p--;
        aim_printf(&uc->pvs,
                   " 0x%02" PRIx64 "_%016" PRIx64,
                   bf_bs_get_word(&lit_hw[p], 64, 8),
                   bf_bs_get_word(&lit_hw[p], 0, 64));
      } while (p > 0);
      aim_printf(&uc->pvs, "%s\n", error ? " ERROR" : "");
    } else {
      p = (int)mc_mgr_ctx_num_max_pipes(dev) - 1;
      aim_printf(&uc->pvs,
                 "LIT[%3d] = 0x%02" PRIx64 "_%016" PRIx64,
                 x,
                 bf_bs_get_word(&lit_sw[p], 64, 8),
                 bf_bs_get_word(&lit_sw[p], 0, 64));
      do {
        p--;
        aim_printf(&uc->pvs,
                   " 0x%02" PRIx64 "_%016" PRIx64,
                   bf_bs_get_word(&lit_sw[p], 64, 8),
                   bf_bs_get_word(&lit_sw[p], 0, 64));
      } while (p > 0);
      aim_printf(&uc->pvs, "\n");
    }
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(lit_wr) {
  MC_MGR_CLI_PROLOGUE("lit-wr",
                      "Write the LIT table.",
                      "-s <session> -d <device> -i <LAG id> -p <port, can "
                      "specify multiple times> ");

  bf_mc_session_hdl_t shdl = 0;
  bf_dev_id_t dev = 0;
  int id = 0, p;
  bool got_shdl = false;
  bool got_id = 0;
  bf_mc_port_map_t pm;
  BF_MC_PORT_MAP_INIT(pm);
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:i:p:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        id = strtoull(optarg, NULL, 0);
        got_id = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        p = strtoull(optarg, NULL, 0);
        BF_MC_PORT_MAP_SET(pm, p);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_id || !got_shdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;

  sts = bf_mc_set_lag_membership(shdl, dev, id, pm);
  aim_printf(&uc->pvs,
             "Setting LAG 0x%0x, status %s (%d)\n",
             id,
             bf_err_str(sts),
             sts);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(lit_np_rd) {
  MC_MGR_CLI_PROLOGUE(
      "lit-np-rd",
      "Read the LIT NP table from HW.",
      "-d <device> -v <table version> [-s <start LAG>] [-e <end LAG>]");

  bf_dev_id_t dev = 0;
  int first = 0, last = 0xFF;
  bool ver = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:v:s:e:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ver = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last || last > 0xFF || 0 > first) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;
  for (x = first; x <= last; ++x) {
    int l, r;
    sts = mc_mgr_get_lag_np_hw(dev, ver, x, &l, &r);
    if (BF_SUCCESS != sts) {
      aim_printf(&uc->pvs,
                 "Failed to read LAG NP %d from HW, %s (%d)\n",
                 x,
                 bf_err_str(sts),
                 sts);
      continue;
    }
    aim_printf(&uc->pvs, "LIT[0x%02x]: left 0x%02x right 0x%02x\n", x, l, r);
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(lit_np_wr) {
  MC_MGR_CLI_PROLOGUE("lit-np-wr",
                      "Write the LIT NP table.",
                      "-s <session> -d <device> -i <LAG id> "
                      "-l <left count> -r <right count>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int lag = 0;
  bool got_lag = false;
  int left = 0;
  bool got_left = false;
  int right = 0;
  bool got_right = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:i:l:r:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        lag = strtoull(optarg, NULL, 0);
        got_lag = true;
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        left = strtoull(optarg, NULL, 0);
        got_left = true;
        break;
      case 'r':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        right = strtoull(optarg, NULL, 0);
        got_right = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_lag || !got_left || !got_right) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (lag > 0xFF || 0 > lag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;
  sts = bf_mc_set_remote_lag_member_count(shdl, dev, lag, left, right);
  if (BF_SUCCESS != sts) {
    aim_printf(&uc->pvs,
               "Failed to write LAG NP %d %s (%d)\n",
               lag,
               bf_err_str(sts),
               sts);
  } else {
    aim_printf(&uc->pvs,
               "Set dev %d lag %d with left %d right %d\n",
               dev,
               lag,
               left,
               right);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(pmt_rd) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "pmt-rd", -1, "Read the PMT table.");

  char usage[] = "Usage: [-h] -d <device> [-s <start Id>] [-e <end Id>]\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "pmt-rd", strlen("pmt-rd"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = 0;
  int first = 0, last = 287;
  bool hw = false;
  int x;

  while (-1 != (x = getopt(argc, argv, "d:v:s:e:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last ||
      last >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev)) ||
      0 > first || 0 > dev || dev >= MC_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = BF_SUCCESS;
  for (x = first; x <= last; ++x) {
    bf_bitset_t pmt_sw[MC_MGR_NUM_PIPES];
    bf_bitset_t pmt_hw[2][MC_MGR_NUM_PIPES];
    uint64_t pmt_sw_[MC_MGR_NUM_PIPES]
                    [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    uint64_t pmt_hw_[2][MC_MGR_NUM_PIPES]
                    [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    int p;
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      bf_bs_init(&pmt_sw[p], BF_PIPE_PORT_COUNT, pmt_sw_[p]);
      bf_bs_init(&pmt_hw[0][p], BF_PIPE_PORT_COUNT, pmt_hw_[0][p]);
      bf_bs_init(&pmt_hw[1][p], BF_PIPE_PORT_COUNT, pmt_hw_[1][p]);
    }
    bf_bs_set_word(
        pmt_sw + 0, 0, 64, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 0, 64));
    bf_bs_set_word(
        pmt_sw + 0, 64, 8, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 64, 8));
    bf_bs_set_word(
        pmt_sw + 1, 0, 64, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 72, 64));
    bf_bs_set_word(
        pmt_sw + 1, 64, 8, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 136, 8));
    bf_bs_set_word(
        pmt_sw + 2, 0, 64, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 144, 64));
    bf_bs_set_word(
        pmt_sw + 2, 64, 8, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 208, 8));
    bf_bs_set_word(
        pmt_sw + 3, 0, 64, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 216, 64));
    bf_bs_set_word(
        pmt_sw + 3, 64, 8, bf_bs_get_word(mc_mgr_ctx_pmt(dev, x), 280, 8));

    if (hw) {
      int v, seg;
      for (v = 0; v < 2; ++v) {
        for (seg = 0; seg < 4; ++seg) {
          sts = mc_mgr_get_pmt_seg_reg(dev, v, x, seg, &pmt_hw[v][seg]);
          if (BF_SUCCESS != sts) {
            aim_printf(&uc->pvs,
                       "Failed to read PMT %d.%d ver %d from HW, %s (%d)\n",
                       x,
                       seg,
                       v,
                       bf_err_str(sts),
                       sts);
            continue;
          }
        }
      }
      bool error = !(bf_bs_equal(&pmt_sw[0], &pmt_hw[0][0]) &&
                     bf_bs_equal(&pmt_sw[1], &pmt_hw[0][1]) &&
                     bf_bs_equal(&pmt_sw[2], &pmt_hw[0][2]) &&
                     bf_bs_equal(&pmt_sw[3], &pmt_hw[0][3]) &&
                     bf_bs_equal(&pmt_sw[0], &pmt_hw[1][0]) &&
                     bf_bs_equal(&pmt_sw[1], &pmt_hw[1][1]) &&
                     bf_bs_equal(&pmt_sw[2], &pmt_hw[1][2]) &&
                     bf_bs_equal(&pmt_sw[3], &pmt_hw[1][3])) ||
                   sts != BF_SUCCESS;
      if (error) {
        aim_printf(&uc->pvs,
                   "ERROR PMT[0x%03x] HW0 = 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 "\n",
                   x,
                   bf_bs_get_word(&pmt_hw[0][3], 64, 8),
                   bf_bs_get_word(&pmt_hw[0][3], 0, 64),
                   bf_bs_get_word(&pmt_hw[0][2], 64, 8),
                   bf_bs_get_word(&pmt_hw[0][2], 0, 64),
                   bf_bs_get_word(&pmt_hw[0][1], 64, 8),
                   bf_bs_get_word(&pmt_hw[0][1], 0, 64),
                   bf_bs_get_word(&pmt_hw[0][0], 64, 8),
                   bf_bs_get_word(&pmt_hw[0][0], 0, 64));
        aim_printf(&uc->pvs,
                   "ERROR PMT[0x%03x] HW1 = 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 "\n",
                   x,
                   bf_bs_get_word(&pmt_hw[1][3], 64, 8),
                   bf_bs_get_word(&pmt_hw[1][3], 0, 64),
                   bf_bs_get_word(&pmt_hw[1][2], 64, 8),
                   bf_bs_get_word(&pmt_hw[1][2], 0, 64),
                   bf_bs_get_word(&pmt_hw[1][1], 64, 8),
                   bf_bs_get_word(&pmt_hw[1][1], 0, 64),
                   bf_bs_get_word(&pmt_hw[1][0], 64, 8),
                   bf_bs_get_word(&pmt_hw[1][0], 0, 64));
        aim_printf(&uc->pvs,
                   "ERROR PMT[0x%03x] SW  = 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64 "_%016" PRIx64
                   " 0x%02" PRIx64 "_%016" PRIx64 "\n",
                   x,
                   bf_bs_get_word(&pmt_sw[3], 64, 8),
                   bf_bs_get_word(&pmt_sw[3], 0, 64),
                   bf_bs_get_word(&pmt_sw[2], 64, 8),
                   bf_bs_get_word(&pmt_sw[2], 0, 64),
                   bf_bs_get_word(&pmt_sw[1], 64, 8),
                   bf_bs_get_word(&pmt_sw[1], 0, 64),
                   bf_bs_get_word(&pmt_sw[0], 64, 8),
                   bf_bs_get_word(&pmt_sw[0], 0, 64));
      }
      aim_printf(&uc->pvs,
                 "PMT[0x%03x] = 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64
                 "_%016" PRIx64 " 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64
                 "_%016" PRIx64 "\n",
                 x,
                 bf_bs_get_word(&pmt_sw[3], 64, 8),
                 bf_bs_get_word(&pmt_sw[3], 0, 64),
                 bf_bs_get_word(&pmt_sw[2], 64, 8),
                 bf_bs_get_word(&pmt_sw[2], 0, 64),
                 bf_bs_get_word(&pmt_sw[1], 64, 8),
                 bf_bs_get_word(&pmt_sw[1], 0, 64),
                 bf_bs_get_word(&pmt_sw[0], 64, 8),
                 bf_bs_get_word(&pmt_sw[0], 0, 64));
    } else {
      aim_printf(&uc->pvs,
                 "PMT[0x%03x] = 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64
                 "_%016" PRIx64 " 0x%02" PRIx64 "_%016" PRIx64 " 0x%02" PRIx64
                 "_%016" PRIx64 "\n",
                 x,
                 bf_bs_get_word(&pmt_sw[3], 64, 8),
                 bf_bs_get_word(&pmt_sw[3], 0, 64),
                 bf_bs_get_word(&pmt_sw[2], 64, 8),
                 bf_bs_get_word(&pmt_sw[2], 0, 64),
                 bf_bs_get_word(&pmt_sw[1], 64, 8),
                 bf_bs_get_word(&pmt_sw[1], 0, 64),
                 bf_bs_get_word(&pmt_sw[0], 64, 8),
                 bf_bs_get_word(&pmt_sw[0], 0, 64));
    }
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(pmt_wr) {
  MC_MGR_CLI_PROLOGUE(
      "pmt-wr",
      "Write the PMT table.",
      "-s <session> -d <device> -i <id> -p <port, can specify multiple times>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int id = 0;
  bool got_id = false;
  int p = 0;
  bf_mc_port_map_t pm;
  BF_MC_PORT_MAP_INIT(pm);
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:i:p:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        id = strtoull(optarg, NULL, 0);
        got_id = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        p = strtoull(optarg, NULL, 0);
        BF_MC_PORT_MAP_SET(pm, p);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_id) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;
  sts = bf_mc_set_port_prune_table(shdl, dev, id, pm);
  aim_printf(&uc->pvs,
             "Setting dev %d PMT[0x%0x] status %s (%d)\n",
             dev,
             id,
             bf_err_str(sts),
             sts);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(tbl_ver_rd) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "tbl-ver-rd", -1, "Read the table version.");

  char usage[] = "Usage: [-h] -d <device>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "tbl-ver-rd", strlen("tbl-ver-rd"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  bool hw = false;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bool ver = mc_mgr_get_tbl_ver(dev);
  bool hw_ver[MC_MGR_NUM_PIPES] = {false};
  if (hw) {
    bool error = false;
    aim_printf(&uc->pvs, "Table Version: %d", ver);
    for (bf_dev_pipe_t pipe = 0; pipe < mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      int v;
      mc_mgr_get_tbl_ver_reg(dev, pipe, &v);
      hw_ver[pipe] = v;
      error = error || (hw_ver[pipe] != ver);
      aim_printf(&uc->pvs, " %d", hw_ver[pipe]);
    }
    aim_printf(&uc->pvs, "%s\n", error ? " ERROR =====\n" : "\n");
  } else {
    aim_printf(&uc->pvs, "Table Version: %d\n", ver);
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(tbl_ver_wr) {
  MC_MGR_CLI_PROLOGUE(
      "tbl-ver-wr", "Write the table version.", "-d <device> -v <version>");

  bf_dev_id_t dev = -1;
  int ver = -1;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:v:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ver = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (0 != ver && 1 != ver) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  dev_target_t dev_tgt = {dev, DEV_PIPE_ALL};
  pipe_status_t sts =
      pipe_mgr_mc_tbl_ver_set(mc_mgr_ctx_pipe_sess(), dev_tgt, ver, false);
  aim_printf(&uc->pvs,
             "Set device %d table version to %d, status %s (%d)\n",
             dev,
             ver,
             pipe_str_err(sts),
             sts);

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(gbl_rid_rd) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "gbl-rid-rd", -1, "Read the global RID.");

  char usage[] = "Usage: [-h] -d <device>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "gbl-rid-rd", strlen("gbl-rid-rd"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  bool hw = false;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  uint16_t rid = 0;
  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_get_global_rid(dev, &rid);
  if (BF_SUCCESS != sts) {
    aim_printf(&uc->pvs, "Failed to get global RID\n");
    return UCLI_STATUS_OK;
  }
  uint16_t hw_rid = rid;
  if (hw) {
    sts = mc_mgr_get_global_rid_reg(dev, &hw_rid);
    if (BF_SUCCESS != sts) {
      aim_printf(&uc->pvs, "Failed to get global RID from HW\n");
      return UCLI_STATUS_OK;
    }
  }
  if (hw_rid == rid) {
    aim_printf(&uc->pvs, "Global RID: %#x\n", rid);
  } else {
    aim_printf(&uc->pvs, "Global RID MISMATCH: SW %#x HW %#x\n", rid, hw_rid);
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(gbl_rid_wr) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "gbl-rid-wr", -1, "Write the global RID.");

  char usage[] = "Usage: -d <device> -r <rid>\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "gbl-rid-wr", strlen("gbl-rid-wr"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = -1;
  int rid = -1;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:r:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'r':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        rid = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (0 > rid || 0xFFFF < rid) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = mc_mgr_set_global_rid(dev, rid);
  aim_printf(&uc->pvs,
             "Set device %d global RID to %#x, status %s (%d)\n",
             dev,
             rid,
             bf_err_str(sts),
             sts);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mit_rd) {
  extern char *optarg;
  extern int optind;
  UCLI_COMMAND_INFO(uc, "mit-rd", -1, "Read the MIT table.");

  char usage[] = "Usage: [-h] -d <device> [-s <start MGID>] [-e <end MGID>]\n";
  int arg_start = 0;
  size_t i;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  for (i = 0; i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);
       ++i) {
    if (!strncmp(uc->pargs[0].args__[i], "mit-rd", strlen("mit-rd"))) {
      arg_start = i;
      break;
    }
  }
  int argc = uc->pargs->count + 1;
  if (1 == argc) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);
  optind = 0;

  bf_dev_id_t dev = 0;
  int first = 0, last = 0xFFFF;
  bool hw = false;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:s:e:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        first = strtoull(optarg, NULL, 0);
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        last = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (first > last || last > 0xFFFF || 0 > first) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  for (x = first; x <= last; ++x) {
    uint32_t mit[2][4];
    mit[1][0] = mit[0][0] = mc_mgr_get_mit(dev, 0, x);
    mit[1][1] = mit[0][1] = mc_mgr_get_mit(dev, 1, x);
    mit[1][2] = mit[0][2] = mc_mgr_get_mit(dev, 2, x);
    mit[1][3] = mit[0][3] = mc_mgr_get_mit(dev, 3, x);
    if (hw) {
      uint32_t m[4];
      mc_mgr_get_mit_row_reg(dev, 0, x >> 2, m + 0, m + 1, m + 2, m + 3);
      mit[1][0] = m[x & 3];
      mc_mgr_get_mit_row_reg(dev, 1, x >> 2, m + 0, m + 1, m + 2, m + 3);
      mit[1][1] = m[x & 3];
      mc_mgr_get_mit_row_reg(dev, 2, x >> 2, m + 0, m + 1, m + 2, m + 3);
      mit[1][2] = m[x & 3];
      mc_mgr_get_mit_row_reg(dev, 3, x >> 2, m + 0, m + 1, m + 2, m + 3);
      mit[1][3] = m[x & 3];
    }
    bool error = !(mit[0][0] == mit[1][0] && mit[0][1] == mit[1][1] &&
                   mit[0][2] == mit[1][2] && mit[0][3] == mit[1][3]);
    if (error) {
      aim_printf(
          &uc->pvs,
          "ERROR SW MIT[0x%04x] = 0x%05x 0x%05x 0x%05x 0x%05x *************\n",
          x,
          mit[0][0],
          mit[0][1],
          mit[0][2],
          mit[0][3]);
      aim_printf(
          &uc->pvs,
          "ERROR HW MIT[0x%04x] = 0x%05x 0x%05x 0x%05x 0x%05x *************\n",
          x,
          mit[1][0],
          mit[1][1],
          mit[1][2],
          mit[1][3]);
    } else {
      aim_printf(&uc->pvs,
                 "MIT[0x%04x] = 0x%05x 0x%05x 0x%05x 0x%05x\n",
                 x,
                 mit[0][0],
                 mit[0][1],
                 mit[0][2],
                 mit[0][3]);
    }
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(mit_wr_hw) {
  MC_MGR_CLI_PROLOGUE("mit-wr-hw",
                      "Write the HW MIT table.",
                      "-d <device> -p <pipe> -g <MGID> -a <addr>");

  bf_dev_id_t dev = -1;
  int pipe = -1, grp = -1, addr = -1;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:g:a:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        break;
      case 'g':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        grp = strtoull(optarg, NULL, 0);
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        addr = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%c", x);
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (grp > 0xFFFF || 0 > grp || addr > 0xFFFFF || 0 > addr || pipe > 3 ||
      0 > pipe) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_ctx_tree(dev, pipe, grp)) {
    aim_printf(&uc->pvs,
               "Dev %d pipe %d group %#x does not have an L1 node",
               dev,
               pipe,
               grp);
    return UCLI_STATUS_OK;
  }

  uint64_t mit[4];
  for (i = 0; i < 4; ++i) {
    uint32_t base = grp & 0xFFFC;
    mit[i] = mc_mgr_ctx_tree(dev, pipe, base + i)
                 ? mc_mgr_ctx_tree(dev, pipe, base + i)->rdm_addr
                 : 0;
  }
  mit[grp & 3] = addr;

  uint64_t data0 = 0, data1 = 0;
  data1 = mit[3] >> 4;
  data0 = mit[3] & 0xF;
  data0 = (data0 << 4) | mit[2];
  data0 = (data0 << 24) | mit[1];
  data0 = (data0 << 44) | mit[0];

  uint64_t mem_addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      mem_addr =
          (grp >> 2) * pipe_top_level_tm_pre_mit0_mem_word_array_element_size +
          ((0 == pipe)
               ? pipe_top_level_tm_pre_mit0_mem_word_address
               : (1 == pipe)
                     ? pipe_top_level_tm_pre_mit1_mem_word_address
                     : (2 == pipe)
                           ? pipe_top_level_tm_pre_mit2_mem_word_address
                           : pipe_top_level_tm_pre_mit3_mem_word_address);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      mem_addr = tof2_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, grp >> 2);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      mem_addr = tof3_mem_tm_tm_pre_pre_pipe_mem_mit_mem_word(pipe, grp >> 2);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }

  bf_subdev_id_t subdev_id = pipe % BF_SUBDEV_PIPE_COUNT;
  x = lld_subdev_ind_write(dev, subdev_id, mem_addr / 16, data1, data0);

  if (0 == x) {
    aim_printf(&uc->pvs, "MIT[0x%04x] pipe %d = 0x%05x\n", grp, pipe, addr);
  } else {
    aim_printf(&uc->pvs, "Failed to program MIT (%d)\n", x);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(port_mask_rd) {
  MC_MGR_CLI_PROLOGUE("port-mask-rd",
                      "Read port mask table.",
                      "[-h] -d <device> -v <table version> -p <port>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bool ver = false;
  bool got_ver = false;
  int port = 0;
  bool got_port = false;
  bool hw = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:v:p:h"))) {
    switch (x) {
      case 'h':
        hw = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        ver = strtoull(optarg, NULL, 0);
        got_ver = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        port = strtoull(optarg, NULL, 0);
        got_port = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_ver || !got_port) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (!mc_dev_port_validate(dev, port)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  int idx = mc_dev_port_to_bit_idx(dev, port);

  bool val = (mc_mgr_ctx_port_fwd_state(dev, idx / 32) >> (idx % 32)) & 1;
  bool val_hw = val;
  if (hw) {
    bf_status_t sts;
    sts = mc_mgr_get_port_mask_reg(dev, ver, idx, NULL, &val_hw);
    if (BF_SUCCESS != sts) {
      aim_printf(&uc->pvs,
                 "Failed to read port mask %d %s (%d)\n",
                 idx,
                 bf_err_str(sts),
                 sts);
    }
  }

  if (val != val_hw) {
    aim_printf(&uc->pvs,
               "Dev %d Port %#x Ver %d, Fwd State=%d (SW) %d (HW) ERROR\n",
               dev,
               port,
               ver,
               val,
               val_hw);
  } else {
    aim_printf(&uc->pvs,
               "Dev %d Port %#x Ver %d, Fwd State=%d (%s)\n",
               dev,
               port,
               ver,
               val,
               val ? "DOWN" : "UP");
  }

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(port_mask_wr) {
  MC_MGR_CLI_PROLOGUE(
      "port-mask-wr",
      "Write port mask table.",
      "-s <session> -d <device> -p <port> -f <0:Fwd, 1:non-Fwd>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int port = 0;
  bool got_port = false;
  bool data = false;
  bool got_data = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:p:f:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        port = strtoull(optarg, NULL, 0);
        got_port = true;
        break;
      case 'f':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        data = strtoull(optarg, NULL, 0);
        got_data = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_port || !got_data) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts;
  sts = bf_mc_set_port_mc_fwd_state(shdl, dev, port, data);
  if (BF_SUCCESS != sts) {
    aim_printf(&uc->pvs,
               "Failed to write port mask dev %d port %d %s (%d)\n",
               dev,
               port,
               bf_err_str(sts),
               sts);
  } else {
    aim_printf(&uc->pvs, "Set Dev %d Port %#x Fwd State=%d\n", dev, port, data);
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(port_ff_mode_rd) {
  MC_MGR_CLI_PROLOGUE(
      "port-ff-mode-rd", "Read port fast failover mode.", "-d <device>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  uint8_t pipe_sel;
  bool val = false, bkup;
  mc_mgr_get_comm_ctrl_reg(dev, &pipe_sel, &val, &bkup);

  aim_printf(&uc->pvs,
             "Dev %d Port Fast Failover %s\n",
             dev,
             val ? "enabled" : "disabled");

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(port_ff_mode_wr) {
  MC_MGR_CLI_PROLOGUE("port-ff-mode-wr",
                      "Write port fast failover mode.",
                      "-s <session> -d <device> -v <0:disable, 1:enable>");

  bf_status_t sts = BF_SUCCESS;
  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bool data = false;
  bool got_data = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:v:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        data = strtoull(optarg, NULL, 0);
        got_data = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_data) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (data) {
    sts = bf_mc_enable_port_fast_failover(shdl, dev);
  } else {
    sts = bf_mc_disable_port_fast_failover(shdl, dev);
  }

  if (sts == BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "Dev %d Port Fast Failover %s\n",
               dev,
               data ? "enabled" : "disabled");
  } else {
    aim_printf(&uc->pvs,
               "Dev %d Port Fast Failover %s failed, status %s\n",
               dev,
               data ? "enable" : "disable",
               bf_err_str(sts));
  }

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(backup_port_mode_rd) {
  MC_MGR_CLI_PROLOGUE(
      "backup-port-mode-rd", "Read backup port mode.", "-d <device>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  uint8_t pipe_sel;
  bool val = false, ff;
  mc_mgr_get_comm_ctrl_reg(dev, &pipe_sel, &ff, &val);

  aim_printf(
      &uc->pvs, "Dev %d backup ports %s\n", dev, val ? "enabled" : "disabled");

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(backup_port_mode_wr) {
  MC_MGR_CLI_PROLOGUE("backup-port-mode-wr",
                      "Write backup port mode.",
                      "-s <session> -d <device> -v <0:disable, 1:enable>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bool data = false;
  bool got_data = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:v:s:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        data = strtoull(optarg, NULL, 0);
        got_data = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_data) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (data) {
    bf_mc_enable_port_protection(shdl, dev);
  } else {
    bf_mc_disable_port_protection(shdl, dev);
  }

  aim_printf(
      &uc->pvs, "Dev %d backup ports %s\n", dev, data ? "enabled" : "disabled");

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(backup_port_rd) {
  MC_MGR_CLI_PROLOGUE(
      "backup-port-rd", "Read backup port table.", "[-h] -d <device>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bool hw = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:h"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'h':
        hw = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  int p;
  bool printed = false;
  for (p = 0; p < (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev));
       ++p) {
    int b0 = 0, b1 = 0, bs = 0;
    b0 = b1 = bs = mc_mgr_ctx_bkup_port(dev, p);
    if (hw) {
      mc_mgr_get_bkup_port_reg(dev, 0, p, &b0);
      mc_mgr_get_bkup_port_reg(dev, 1, p, &b1);
    }
    if (bs == b0 && bs == b1) {
      if (p != bs) {
        aim_printf(&uc->pvs,
                   "Port %3d has backup %3d\n",
                   mc_bit_idx_to_dev_port(dev, p),
                   mc_bit_idx_to_dev_port(dev, bs));
        printed = true;
      }
    } else {
      aim_printf(&uc->pvs,
                 "Port %3d has sw backup %3d, but hw backup v0 %3d (%3d) and "
                 "v1 %3d (%3d) ERROR\n",
                 mc_bit_idx_to_dev_port(dev, p),
                 mc_bit_idx_to_dev_port(dev, bs),
                 mc_bit_idx_to_dev_port(dev, b0),
                 b0,
                 mc_bit_idx_to_dev_port(dev, b1),
                 b1);
      printed = true;
    }
  }
  if (!printed) aim_printf(&uc->pvs, "No backup port configured.\n");

  return UCLI_STATUS_OK;
}
MC_MGR_CLI_CMD_DECLARE(backup_port_wr) {
  MC_MGR_CLI_PROLOGUE(
      "backup-port-wr",
      "Set backup port.",
      "-s <session> -d <device> -p <protect port> -b <backup port>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_port_t protect = 0;
  bool got_protect = false;
  bf_dev_port_t backup = 0;
  bool got_backup = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:p:b:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        protect = strtoull(optarg, NULL, 0);
        got_protect = true;
        break;
      case 'b':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        backup = strtoull(optarg, NULL, 0);
        got_backup = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_protect || !got_backup) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_status_t sts = bf_mc_set_port_protection(shdl, dev, protect, backup);
  aim_printf(&uc->pvs,
             "Session %#x dev %d set port %d to have backup %d, status %s\n",
             shdl,
             dev,
             protect,
             backup,
             bf_err_str(sts));

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(port_down_mask_rd) {
  MC_MGR_CLI_PROLOGUE(
      "port-down-mask-rd", "Read port down mask.", "-d <device> -p <port>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int port = 0;
  bool got_port = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        port = strtoull(optarg, NULL, 0);
        got_port = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_port) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }
  if (!mc_dev_port_validate(dev, port)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  int idx = mc_dev_port_to_bit_idx(dev, port);

  bool val = false;
  mc_mgr_get_port_down_reg(dev, idx, &val);

  aim_printf(&uc->pvs, "Dev %d port %#x %s\n", dev, port, val ? "down" : "up");

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(port_down_mask_clr) {
  MC_MGR_CLI_PROLOGUE(
      "port-down-mask-clr", "Clear port down mask.", "-d <device> -p <port>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int port = 0;
  bool got_port = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        port = strtoull(optarg, NULL, 0);
        got_port = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_port) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  if (!mc_dev_port_validate(dev, port)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  int idx = mc_dev_port_to_bit_idx(dev, port);

  mc_mgr_clr_port_down_reg(dev, idx);

  aim_printf(&uc->pvs, "Dev %d port %#x cleared\n", dev, port);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(cpu_port_rd) {
  MC_MGR_CLI_PROLOGUE(
      "cpu-port-rd", "Read CPU port info.", "-d <device> -p <pipe>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_pipe_t pipe = 0;
  bool got_pipe = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_pipe) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if ((mc_mgr_ctx_num_max_pipes(dev) <= pipe) || MC_MGR_INVALID_DEV(dev)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }
  bool en = false;
  int port = 0;
  uint8_t l1_slice;
  mc_mgr_get_pre_ctrl_reg(dev, pipe, &en, &port, &l1_slice);

  aim_printf(
      &uc->pvs, "Dev %d pipe %d enable %d port %d\n", dev, pipe, en, port);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(cpu_port_wr) {
  MC_MGR_CLI_PROLOGUE(
      "cpu-port-wr",
      "Write CPU port info.",
      "-d <device> -p <pipe> -e <0:disable 1:enable> -v <port 0-71>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_pipe_t pipe = 0;
  bool got_pipe = false;
  bool en = false;
  bool got_en = false;
  int port = 0;
  bool got_port = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:e:v:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        en = strtoull(optarg, NULL, 0);
        got_en = true;
        break;
      case 'v':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        port = strtoull(optarg, NULL, 0);
        got_port = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_pipe || !got_en || !got_port) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (mc_mgr_ctx_num_max_pipes(dev) <= pipe || MC_MGR_INVALID_DEV(dev) ||
      0 > port || 71 < port) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_dev_port_t dev_port = mc_make_dev_port(dev, pipe, port);
  bf_mc_set_copy_to_cpu(dev, en, dev_port);

  aim_printf(
      &uc->pvs, "Dev %d pipe %d enable %d port %d\n", dev, pipe, en, port);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(l1_per_slice_rd) {
  MC_MGR_CLI_PROLOGUE(
      "l1-per-slice-rd", "Read L1-per-slice config.", "-d <device> -p <pipe>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_pipe_t pipe = 0;
  bool got_pipe = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_pipe) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (mc_mgr_ctx_num_max_pipes(dev) <= pipe || MC_MGR_INVALID_DEV(dev)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bool en = false;
  int port = 0;
  uint8_t l1_slice;
  mc_mgr_get_pre_ctrl_reg(dev, pipe, &en, &port, &l1_slice);

  aim_printf(&uc->pvs, "Dev %d pipe %d count %u\n", dev, pipe, l1_slice);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(l1_per_slice_wr) {
  MC_MGR_CLI_PROLOGUE("l1-per-slice-wr",
                      "Write L1-per-slice config.",
                      "-s <session handle> -d <device> -c <count 0-255>");

  bf_mc_session_hdl_t shdl = -1;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int count = 0;
  bool got_count = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:c:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'c':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        count = strtoull(optarg, NULL, 0);
        got_count = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_count) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (0 > count || 0xFF < count) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_mc_set_max_nodes_before_yield(shdl, dev, count);

  aim_printf(&uc->pvs, "Dev %d count %d\n", dev, count);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(max_nodes) {
  MC_MGR_CLI_PROLOGUE("max-nodes",
                      "Write max L1 and L2 config.",
                      "-s <session handle> -d <device> -n <L1 count 0-0xFFFFF> "
                      "-p <L2 count 0-0xFFFFF>");

  bf_mc_session_hdl_t shdl = 0;
  bool got_shdl = false;
  bf_dev_id_t dev = 0;
  bool got_dev = false;
  int l1c = 0;
  bool got_l1c = false;
  int l2c = 0;
  bool got_l2c = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "s:d:n:p:"))) {
    switch (x) {
      case 's':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        shdl = strtoull(optarg, NULL, 0);
        got_shdl = true;
        break;
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        l1c = strtoull(optarg, NULL, 0);
        got_l1c = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        l2c = strtoull(optarg, NULL, 0);
        got_l2c = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_shdl || !got_dev || !got_l1c || !got_l2c) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (0 > l1c || 0xFFFFF < l1c || 0 > l2c || 0xFFFFF < l2c) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  bf_mc_set_max_node_threshold(shdl, dev, l1c, l2c);

  aim_printf(&uc->pvs,
             "Shdl 0x%x Dev %d L1-count %d L2-count %d\n",
             shdl,
             dev,
             l1c,
             l2c);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(max_l1_rd) {
  MC_MGR_CLI_PROLOGUE(
      "max-l1-rd", "Read max L1 config.", "-d <device> -p <pipe>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_pipe_t pipe = 0;
  bool got_pipe = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_pipe) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (mc_mgr_ctx_num_max_pipes(dev) <= pipe || MC_MGR_INVALID_DEV(dev)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  int count = 0;
  mc_mgr_get_max_l1_reg(dev, pipe, &count);

  aim_printf(&uc->pvs, "Dev %d pipe %d count %d\n", dev, pipe, count);

  return UCLI_STATUS_OK;
}

MC_MGR_CLI_CMD_DECLARE(max_l2_rd) {
  MC_MGR_CLI_PROLOGUE(
      "max-l2-rd", "Read max L2 config.", "-d <device> -p <pipe>");

  bf_dev_id_t dev = 0;
  bool got_dev = false;
  bf_dev_pipe_t pipe = 0;
  bool got_pipe = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe = strtoull(optarg, NULL, 0);
        got_pipe = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_pipe) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (mc_mgr_ctx_num_max_pipes(dev) <= pipe || MC_MGR_INVALID_DEV(dev)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  int count = 0;
  mc_mgr_get_max_l2_reg(dev, pipe, &count);

  aim_printf(&uc->pvs, "Dev %d pipe %d count %d\n", dev, pipe, count);

  return UCLI_STATUS_OK;
}

/**
 * Walks an L2 chain, reading and dumping each node.
 *
 * @param dev  Device identifier
 * @param addr RDM address of head of L2 chain
 * @param hw   Whether to read from HW
 * @param max_links Maximum number of links to dump
 * @param pvs  Output stream
 *
 * @return 0 if successful, -1 if failure
 */
static int dump_l2_chain(
    bf_dev_id_t dev, uint32_t addr, bool hw, int max_links, aim_pvs_t *pvs) {
  uint32_t next_l2 = 0;

  /* Head of chain may not be zero */
  if (addr == 0) {
    aim_printf(pvs, "Badly configured tree\n");
    return -1;
  }

  /* Include L1 link in count */
  int num_links = 1;

  /* Traverse the L2 chain */
  for (; addr != 0; addr = next_l2) {
    mc_mgr_rdm_line_t line;

    /* Artifically limit the number of links displayed
     * as a cheap hedge against runaway output. */
    if (num_links >= max_links) {
      aim_printf(pvs, "Limiting output...\n");
      break;
    }
    ++num_links;

    /* Read and dump L2 node */
    if (read_rdm_line(dev, addr, hw, &line, pvs) != BF_SUCCESS) return -1;
    mc_mgr_rdm_decode_line(dev, &line);
    aim_printf(pvs, "%05x: ", addr);
    if (dump_rdm_node(addr, &line, pvs) < 0) return -1;

    /* Extract forward pointer */
    next_l2 = 0;

    switch (line.type[addr & 1]) {
      case mc_mgr_rdm_node_type_lag:
        /* L2 LAG */
        next_l2 = line.u.lag[addr & 1].next_l2;
        break;

      case mc_mgr_rdm_node_type_port18:
        /* L2 Port 18 */
        if (!line.u.port18[addr & 1].last) next_l2 = addr + 1;
        break;

      case mc_mgr_rdm_node_type_port72:
        /* L2 Port 72 */
        if (!line.u.port72.last) next_l2 = addr + 2;
        break;

      default:
        aim_printf(pvs, "Unexpected node type!\n");
        return -1;
    }
  }

  return 0;
}

/**
 * Reads and dumps an L1 node and its subordinate L2 nodes.
 *
 * Will traverse the L1 chain if 'walk' is enabled.
 *
 * @param dev  Device identifier
 * @param addr RDM address of head of L1 chain
 * @param hw   Whether to read from HW
 * @param walk Whether to walk the L1 chain
 * @param max_links Maximum number of links to dump
 * @param pvs  Output stream
 *
 * @return 0 if successful, -1 if failure
 */
static int dump_l1_chain(bf_dev_id_t dev,
                         uint32_t addr,
                         bool hw,
                         bool walk,
                         int max_links,
                         aim_pvs_t *pvs) {
  const uint32_t first_l1 = addr;
  uint32_t next_l1 = 0;

  /* Traverse the L1 chain */
  for (; addr != 0; addr = next_l1) {
    mc_mgr_rdm_line_t line;

    if (addr != first_l1 && !walk) break;

    /* Read and dump L1 node */
    if (read_rdm_line(dev, addr, hw, &line, pvs) != BF_SUCCESS) return -1;
    mc_mgr_rdm_decode_line(dev, &line);
    aim_printf(pvs, "%05x: ", addr);
    if (dump_rdm_node(addr, &line, pvs) < 0) return -1;

    /* Extract forward L2 pointers */
    next_l1 = 0;
    uint32_t next_l2 = 0;

    switch (line.type[addr & 1]) {
      case mc_mgr_rdm_node_type_rid:
        /* L1 RID */
        next_l1 = line.u.rid.next_l1;
        next_l2 = line.u.rid.next_l2;
        break;

      case mc_mgr_rdm_node_type_xid:
        /* L1 RID XID */
        next_l1 = line.u.xid.next_l1;
        next_l2 = line.u.xid.next_l2;
        break;

      case mc_mgr_rdm_node_type_end:
        /* L1 RID no L1_next */
        next_l2 = line.u.end[addr & 1].next_l2;
        break;

      case mc_mgr_rdm_node_type_ecmp:
        /* L1 ECMP Pointer */
        aim_printf(pvs, "ECMP group not decoded\n");
        next_l1 = line.u.ecmp.next_l1;
        continue;

      case mc_mgr_rdm_node_type_ecmp_xid:
        /* L1 ECMP Pointer XID */
        aim_printf(pvs, "ECMP group not decoded\n");
        next_l1 = line.u.ecmp_xid.next_l1;
        continue;

      default:
        aim_printf(pvs, "Unexpected node type!\n");
        return -1;
    }

    /* Process subordinate nodes */
    if (dump_l2_chain(dev, next_l2, hw, max_links, pvs) < 0) return -1;
  }

  return 0;
}

/*
 * Reads an L1 node from RDM (hardware or shadow RAM).
 * Dumps the L1 node and its L2 chain.
 * Does not currently support ECMP.
 */
MC_MGR_CLI_CMD_DECLARE(l1_read) {
  MC_MGR_CLI_PROLOGUE(
      "l1-read", "Read L1 node.", "-d <dev> -n <node> -m <max> -h");

  bf_dev_id_t dev = 0;
  bf_mc_node_hdl_t nhdl = 0;
  int max_links = 16;
  bool got_dev = false;
  bool got_node = false;
  bool hw = false;
  bool walk = false;
  int x;

  if (!mc_mgr_ready()) {
    aim_printf(&uc->pvs, "Multicast Manager not present or not ready\n");
    return UCLI_STATUS_OK;
  }

  while ((x = getopt(argc, argv, "d:n:m:hw")) != -1) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'n':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        nhdl = strtoull(optarg, NULL, 0);
        got_node = true;
        break;
      case 'm':
        /* Maximum number of links (per pipe) */
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        max_links = strtoull(optarg, NULL, 0);
        break;
      case 'h':
        hw = true;
        break;
      case 'w':
        walk = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!got_dev || !got_node) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (max_links < 1 || max_links > L1_READ_MAX_LINKS) {
    aim_printf(&uc->pvs, "-m must be 1..%d\n", L1_READ_MAX_LINKS);
    return UCLI_STATUS_OK;
  }

  if (!mc_mgr_dev_present(dev)) {
    aim_printf(&uc->pvs, "Device %d not present\n", dev);
    return UCLI_STATUS_OK;
  }

  mc_l1_node_t *n = mc_mgr_lookup_l1_node(dev, nhdl, __func__, __LINE__);
  if (!n) {
    aim_printf(&uc->pvs, "Node %#x not found\n", nhdl);
    return UCLI_STATUS_OK;
  }

  aim_printf(&uc->pvs, "Reading %s\n", (hw) ? "Hardware" : "Shadow RAM");

  /* Iterate over pipes */
  for (int pn = 0; pn < MC_MGR_NUM_PIPES; ++pn) {
    /* RDM address of L1 node in this pipe */
    uint32_t addr = n->hw_nodes[pn].rdm_addr;

    if (addr == 0) continue;

    aim_printf(&uc->pvs, "--- Pipe %d ---\n", pn);

    if (dump_l1_chain(dev, addr, hw, walk, max_links, &uc->pvs) != BF_SUCCESS)
      break;
  }

  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f mc_mgr_ucli_ucli_handlers__[] = {
    MC_MGR_CLI_CMD_HNDLR(init),
    MC_MGR_CLI_CMD_HNDLR(show_sess),
    MC_MGR_CLI_CMD_HNDLR(create_sess),
    MC_MGR_CLI_CMD_HNDLR(destroy_sess),
    MC_MGR_CLI_CMD_HNDLR(mgrp_create),
    MC_MGR_CLI_CMD_HNDLR(mgrp_destroy),
    MC_MGR_CLI_CMD_HNDLR(node_create),
    MC_MGR_CLI_CMD_HNDLR(node_destroy),
    MC_MGR_CLI_CMD_HNDLR(l1_associate),
    MC_MGR_CLI_CMD_HNDLR(l1_dissociate),
    MC_MGR_CLI_CMD_HNDLR(l1_list_all),
    MC_MGR_CLI_CMD_HNDLR(l1_dump),
    MC_MGR_CLI_CMD_HNDLR(ecmp_dump),
    MC_MGR_CLI_CMD_HNDLR(ecmp_create),
    MC_MGR_CLI_CMD_HNDLR(ecmp_destroy),
    MC_MGR_CLI_CMD_HNDLR(ecmp_mbr_add),
    MC_MGR_CLI_CMD_HNDLR(ecmp_mbr_rmv),
    MC_MGR_CLI_CMD_HNDLR(ecmp_associate),
    MC_MGR_CLI_CMD_HNDLR(ecmp_dissociate),
    MC_MGR_CLI_CMD_HNDLR(ecmp_modify),
    MC_MGR_CLI_CMD_HNDLR(ecmp_list_all_handles),
    MC_MGR_CLI_CMD_HNDLR(rdm_read),
    MC_MGR_CLI_CMD_HNDLR(rdm_alloc_dump),
    MC_MGR_CLI_CMD_HNDLR(set_in_prog),
    MC_MGR_CLI_CMD_HNDLR(clr_in_prog),
    MC_MGR_CLI_CMD_HNDLR(mgrp_show),
    MC_MGR_CLI_CMD_HNDLR(mgrp_set),
    MC_MGR_CLI_CMD_HNDLR(mgrp_clr),
    MC_MGR_CLI_CMD_HNDLR(tbl_ver_rd),
    MC_MGR_CLI_CMD_HNDLR(tbl_ver_wr),
    MC_MGR_CLI_CMD_HNDLR(gbl_rid_rd),
    MC_MGR_CLI_CMD_HNDLR(gbl_rid_wr),
    MC_MGR_CLI_CMD_HNDLR(pvt_rd),
    MC_MGR_CLI_CMD_HNDLR(pvt_wr),
    MC_MGR_CLI_CMD_HNDLR(tvt_rd),
    MC_MGR_CLI_CMD_HNDLR(tvt_wr),
    MC_MGR_CLI_CMD_HNDLR(lit_rd),
    MC_MGR_CLI_CMD_HNDLR(lit_wr),
    MC_MGR_CLI_CMD_HNDLR(lit_np_rd),
    MC_MGR_CLI_CMD_HNDLR(lit_np_wr),
    MC_MGR_CLI_CMD_HNDLR(pmt_rd),
    MC_MGR_CLI_CMD_HNDLR(pmt_wr),
    MC_MGR_CLI_CMD_HNDLR(mit_rd),
    MC_MGR_CLI_CMD_HNDLR(mit_wr_hw),
    MC_MGR_CLI_CMD_HNDLR(port_mask_rd),
    MC_MGR_CLI_CMD_HNDLR(port_mask_wr),
    MC_MGR_CLI_CMD_HNDLR(port_ff_mode_rd),
    MC_MGR_CLI_CMD_HNDLR(port_ff_mode_wr),
    MC_MGR_CLI_CMD_HNDLR(backup_port_mode_rd),
    MC_MGR_CLI_CMD_HNDLR(backup_port_mode_wr),
    MC_MGR_CLI_CMD_HNDLR(backup_port_rd),
    MC_MGR_CLI_CMD_HNDLR(backup_port_wr),
    MC_MGR_CLI_CMD_HNDLR(port_down_mask_rd),
    MC_MGR_CLI_CMD_HNDLR(port_down_mask_clr),
    MC_MGR_CLI_CMD_HNDLR(cpu_port_rd),
    MC_MGR_CLI_CMD_HNDLR(cpu_port_wr),
    MC_MGR_CLI_CMD_HNDLR(l1_per_slice_rd),
    MC_MGR_CLI_CMD_HNDLR(l1_per_slice_wr),
    MC_MGR_CLI_CMD_HNDLR(l1_read),
    MC_MGR_CLI_CMD_HNDLR(max_nodes),
    MC_MGR_CLI_CMD_HNDLR(max_l1_rd),
    MC_MGR_CLI_CMD_HNDLR(max_l2_rd),
    NULL};
/* <auto.ucli.handlers.end> */

static ucli_module_t mc_mgr_ucli_module__ = {
    "mc_mgr_ucli",
    NULL,
    mc_mgr_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *mc_mgr_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&mc_mgr_ucli_module__);
  n = ucli_node_create("mc_mgr", NULL, &mc_mgr_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("mc_mgr"));
  return n;
}

static ucli_command_handler_f bf_drv_show_tech_ucli_mc_handlers__[] = {
    mc_mgr_ucli_ucli__rdm_alloc_dump__, mc_mgr_ucli_ucli__mgrp_show__};

char *mc_cmd[] = {"rdm-alloc-dump", "mgrp-show"};

ucli_status_t bf_drv_show_tech_ucli_mc__(ucli_context_t *uc) {
  unsigned hdl_iter = 0;
  const char *restore_arg[UCLI_CONFIG_MAX_ARGS];
  int status_hdl = UCLI_STATUS_CONTINUE;
  bool dev_enable = true;
  unsigned hdl_num = sizeof(mc_cmd) / sizeof(mc_cmd[0]);
  aim_printf(&uc->pvs, "-------------------- MC --------------------\n");
  *restore_arg = *uc->pargs[0].args__;
  /*
   * Since one of the module parameter is not required and
   * lower function do not expect this parameter.
   */
  if (!(*uc->pargs->args[0]) || !uc->pargs->count) {
    uc->pargs->args__[1] = "-d0";
    dev_enable = false;
    if (!uc->pargs->count) {
      uc->pargs->count++;
    }
  } else {
    if (uc->pargs->count > 1) {
      uc->pargs->count--;
    }
  }
  while (hdl_iter < hdl_num) {
    if (bf_drv_show_tech_ucli_mc_handlers__[hdl_iter] && mc_cmd[hdl_iter]) {
      *uc->pargs[0].args__ = mc_cmd[hdl_iter];
      status_hdl = bf_drv_show_tech_ucli_mc_handlers__[hdl_iter](uc);
      if (status_hdl == UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, " Status Handler return = %d   \n", status_hdl);
      }
      hdl_iter++;
    }
  }
  /* Restore to original. */
  if (dev_enable) {
    uc->pargs->count++;
  }
  *uc->pargs[0].args__ = *restore_arg;
  return 0;
}

#else
void *mc_mgr_ucli_node_create(void) { return NULL; }
#endif
