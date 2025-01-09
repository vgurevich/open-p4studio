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
 * @file pipe_mgr_adt_ucli.c
 * @date
 *
 * Action data table manager, ucli commands, command-handlers and the world.
 */

/* Standard header includes */
#include <getopt.h>
#include <limits.h>

/* Module header includes */

/* Local header includes */
#include "pipe_mgr_act_tbl.h"
#include "pipe_mgr_adt_mgr_dump.h"
#include "pipe_mgr_adt_mgr_int.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_adt_tbl_ucli_ucli__##name##__
#define PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(tbl_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  int argc;
  uint8_t device_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;
  char *const *argv;
  static char usage[] = "Usage : tbl_info -d <dev_id> -h <tbl_hdl/\n";

  UCLI_COMMAND_INFO(uc,
                    "tbl_info",
                    -1,
                    "Dump action data table level info"
                    " Usage : tbl_info -d <dev_id> -h <tbl_hdl>");

  dflag = hflag = 0;
  d_arg = h_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

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
  } else /*if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 1) {
    adt_tbl_hdl = strtoul(h_arg, NULL, 0);
  } else
    adt_tbl_hdl = 0xFFFF;

  pipe_mgr_adt_dump_tbl_info(uc, device_id, adt_tbl_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(entry_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, eflag;
  char *d_arg, *h_arg, *e_arg;
  int argc;

  uint8_t device_id = -1;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = -1;
  pipe_adt_ent_hdl_t adt_ent_hdl = 0x3FF;
  char *const *argv;
  static char usage[] =
      "Usage : entry_info -d <dev_id> -h <tbl_hdl> -e <entry_hdl>\n";
  UCLI_COMMAND_INFO(
      uc,
      "entry_info",
      -1,
      "Dump action data entry information"
      " Usage : entry info -d <dev_id> -h <tbl_hdl> -e <entry_hdl>");

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
  } else if (dflag == 1) {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else if (hflag == 1) {
    adt_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  if (eflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else /*if (eflag == 1)*/
  {
    adt_ent_hdl = strtoul(e_arg, NULL, 0);
  }

  pipe_mgr_adt_dump_entry_info(uc, device_id, adt_tbl_hdl, adt_ent_hdl);

  return UCLI_STATUS_OK;
}

PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(entry_count) {
  PIPE_MGR_CLI_PROLOGUE("entry_count",
                        "Dump action data table entry count",
                        "-d <dev_id> -h <tbl_hdl>");
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  uint8_t device_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;

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
  } else /*if (dflag == 1)*/
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
    adt_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  sts = pipe_mgr_adt_tbl_get_num_entries_placed(dev_tgt, adt_tbl_hdl, &count);

  if (sts == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "adt table 0x%x: %u entries occupied\n", adt_tbl_hdl, count);
  } else {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(entry_count_programmed) {
  PIPE_MGR_CLI_PROLOGUE("entry_count_programmed",
                        " Dump action data table entry count in hw",
                        "-d <dev_id> -h <tbl_hdl>");
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  uint8_t device_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;

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
  } else /*if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(
          &uc->pvs, "entry_count_programmed : Invalid device_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }
  if (pipe_mgr_is_device_virtual(device_id)) {
    aim_printf(&uc->pvs,
               "entry_count_programmed : CMD not available on this device\n");
    return UCLI_STATUS_OK;
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    adt_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  sts =
      pipe_mgr_adt_tbl_get_num_entries_programmed(dev_tgt, adt_tbl_hdl, &count);

  if (sts == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "adt table 0x%x: %u entries occupied\n", adt_tbl_hdl, count);
  } else {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_ADT_TBL_CLI_CMD_DECLARE(entry_count_reserved) {
  PIPE_MGR_CLI_PROLOGUE(
      "entry_count_reserved",
      " Dump action data table entry count placed and reserved",
      "-d <dev_id> -h <tbl_hdl>");
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  uint8_t device_id;
  pipe_adt_tbl_hdl_t adt_tbl_hdl;

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
  } else /*if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(
          &uc->pvs, "entry_count_reserved : Invalid device_id %s\n", d_arg);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    adt_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  sts = pipe_mgr_adt_tbl_get_num_entries_reserved(dev_tgt, adt_tbl_hdl, &count);

  if (sts == PIPE_SUCCESS) {
    aim_printf(
        &uc->pvs, "adt table 0x%x: %u entries reserved\n", adt_tbl_hdl, count);
  } else {
    aim_printf(&uc->pvs, "No information found for the table specified\n");
  }

  return UCLI_STATUS_OK;
}

static ucli_command_handler_f pipe_mgr_adt_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(entry_info),
    PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(entry_count),
    PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(entry_count_programmed),
    PIPE_MGR_ADT_TBL_CLI_CMD_HNDLR(entry_count_reserved),
    NULL};

static ucli_module_t pipe_mgr_adt_tbl_ucli_module__ = {
    "adt_tbl_ucli",
    NULL,
    pipe_mgr_adt_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_adt_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_adt_tbl_ucli_module__);
  m = ucli_node_create("adt_mgr", n, &pipe_mgr_adt_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("adt_mgr"));
  return m;
}

#else

void *pipe_mgr_adt_tbl_ucli_node_create(void) { return NULL; }

#endif
