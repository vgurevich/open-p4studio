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
 * @file pipe_mgr_exm_ucli.c
 * @date
 *
 * Exact match table related ucli commands, command-handlers and the world.
 */

/* Standard header includes */
#include <getopt.h>
#include <limits.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

/* Local header includes */
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_tbl_dump.h"
#include "pipe_mgr_exm_utest.h"

#define PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_exm_tbl_ucli_ucli__##name##__
#define PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(tbl_info) {
  PIPE_MGR_CLI_PROLOGUE("tbl_info",
                        "Dump exact match table level info",
                        "-d <dev_id> -h <tbl_hdl> [-q]");

  int c;
  bf_dev_id_t device_id = 0;
  pipe_mat_tbl_hdl_t exm_tbl_hdl = 0;
  bool quiet = false;

  while ((c = getopt(argc, argv, "d:h:q")) != -1) {
    switch (c) {
      case 'd':
        device_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        exm_tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'q':
        quiet = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (device_id < 0 || device_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
    return UCLI_STATUS_OK;
  }
  if (exm_tbl_hdl == 0 || exm_tbl_hdl == 0xFFFFFFFF) {
    aim_printf(&uc->pvs, "tbl_info : Invalid table handle 0x%x\n", exm_tbl_hdl);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_exm_dump_tbl_info(uc, device_id, exm_tbl_hdl);
  aim_printf(&uc->pvs, "Entries : \n");
  pipe_mgr_exm_tbl_dump_entries(uc, device_id, exm_tbl_hdl, !quiet);

  return UCLI_STATUS_OK;
}

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(hash_info) {
  PIPE_MGR_CLI_PROLOGUE("hash_info",
                        "Dump exact match table hash info",
                        "-d <dev_id> -h <tbl_hdl>");

  int c;
  bf_dev_id_t device_id = 0;
  pipe_mat_tbl_hdl_t exm_tbl_hdl = 0;

  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        device_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        exm_tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (device_id < 0 || device_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
    return UCLI_STATUS_OK;
  }
  if (exm_tbl_hdl == 0 || exm_tbl_hdl == 0xFFFFFFFF) {
    aim_printf(&uc->pvs, "tbl_info : Invalid table handle 0x%x\n", exm_tbl_hdl);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_exm_tbl_dump_hash(uc, device_id, exm_tbl_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(entry_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, eflag;
  char *d_arg, *h_arg, *e_arg;
  int argc;
  uint8_t device_id;
  pipe_mat_tbl_hdl_t exm_tbl_hdl;
  pipe_mat_ent_hdl_t entry_hdl;
  char *const *argv;
  static char usage[] =
      "Usage : entry_info -d <dev_id> -h <tbl_hdl> -e <entr_hdl>\n";

  UCLI_COMMAND_INFO(
      uc,
      "entry_info",
      -1,
      "Dump exact match entry info"
      " Usage : entry_info -d <dev_id> -h <tbl_hdl> -e <entry_hdl>");

  dflag = hflag = eflag = 0;
  d_arg = h_arg = e_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;

      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "entry_info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    exm_tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (eflag == 1) {
    entry_hdl = strtoul(e_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_exm_dump_entry_info(uc, device_id, exm_tbl_hdl, entry_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(entry_phy_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, eflag;
  char *d_arg, *h_arg, *e_arg;
  int argc;
  uint8_t device_id;
  pipe_mat_tbl_hdl_t exm_tbl_hdl;
  pipe_mat_ent_hdl_t entry_hdl;
  char *const *argv;
  static char usage[] =
      "Usage : entry_phy_info -d <dev_id> -h <tbl_hdl> -e <entr_hdl>\n";

  UCLI_COMMAND_INFO(
      uc,
      "entry_phy_info",
      -1,
      "Dump exact match entry physical info"
      " Usage : entry_phy_info -d <dev_id> -h <tbl_hdl> -e <entry_hdl>");

  dflag = hflag = eflag = 0;
  d_arg = h_arg = e_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;

      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "entry_info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    exm_tbl_hdl = strtoul(h_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (eflag == 1) {
    entry_hdl = strtoul(e_arg, NULL, 0);
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_exm_dump_phy_entry_info(uc, device_id, exm_tbl_hdl, entry_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(entry_count) {
  PIPE_MGR_CLI_PROLOGUE("entry_count",
                        "Dump exact match table programmed entry count",
                        "-d <dev_id> -h <tbl_hdl>");
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  uint8_t device_id;
  pipe_mat_tbl_hdl_t exm_tbl_hdl;

  dflag = hflag = 0;
  d_arg = h_arg = NULL;

  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else /* if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "entry_count : Invalid device_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    exm_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  pipe_mgr_exm_tbl_t *exm_tbl;
  exm_tbl = pipe_mgr_exm_tbl_get(device_id, exm_tbl_hdl);
  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs, "exm table not found for handle 0x%x\n", exm_tbl_hdl);
    return UCLI_STATUS_OK;
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};
  if (pipe_mgr_is_device_virtual(device_id)) {
    pipe_mgr_exm_tbl_get_placed_entry_count(dev_tgt, exm_tbl_hdl, &count);
  } else {
    pipe_mgr_exm_tbl_get_programmed_entry_count(dev_tgt, exm_tbl_hdl, &count);
  }
  aim_printf(
      &uc->pvs, "exm tbl 0x%x: %u entries occupied\n", exm_tbl_hdl, count);

  return UCLI_STATUS_OK;
}

PIPE_MGR_EXM_TBL_CLI_CMD_DECLARE(entry_move_stats) {
  PIPE_MGR_CLI_PROLOGUE("entry_move_stats",
                        "Dump exact match table entry move statistics info",
                        "-d <dev_id> -h <tbl_hdl> [-c] <clear stats>");
  extern char *optarg;
  int c;
  bf_dev_id_t device_id = 0;
  pipe_mat_tbl_hdl_t exm_tbl_hdl = 0;
  bool clear = false;

  while ((c = getopt(argc, argv, "d:h:c")) != -1) {
    switch (c) {
      case 'd':
        device_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        exm_tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'c':
        clear = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (device_id < 0 || device_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
    return UCLI_STATUS_OK;
  }
  if (exm_tbl_hdl == 0 || exm_tbl_hdl == 0xFFFFFFFF) {
    aim_printf(&uc->pvs, "tbl_info : Invalid table handle 0x%x\n", exm_tbl_hdl);
    return UCLI_STATUS_OK;
  }
  if (clear) {
    pipe_mgr_exm_entry_move_stats_clear(uc, device_id, exm_tbl_hdl);
  } else {
    pipe_mgr_exm_entry_move_stats_dump(uc, device_id, exm_tbl_hdl);
  }
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f pipe_mgr_exm_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(hash_info),
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(entry_info),
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(entry_phy_info),
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(entry_count),
    PIPE_MGR_EXM_TBL_CLI_CMD_HNDLR(entry_move_stats),
    NULL};

static ucli_module_t pipe_mgr_exm_tbl_ucli_module__ = {
    "exm_tbl_ucli",
    NULL,
    pipe_mgr_exm_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_exm_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_exm_tbl_ucli_module__);
  m = ucli_node_create("exm_tbl_mgr", n, &pipe_mgr_exm_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("exm_tbl_mgr"));
  return m;
}

#else

#include <target-utils/uCli/ucli.h>
ucli_node_t *pipe_mgr_exm_tbl_ucli_node_create(ucli_node_t *n) {
  (void)n;
  return NULL;
}

#endif
