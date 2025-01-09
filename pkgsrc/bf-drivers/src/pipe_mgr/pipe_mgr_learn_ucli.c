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
/* Standard includes */
#include <getopt.h>
#include <limits.h>

/* Module includes */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_learn.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_LEARN_CLI_CMD_HNDLR(name) pipe_mgr_learn_ucli_ucli__##name##__
#define PIPE_MGR_LEARN_CLI_CMD_DECLARE(name) \
  static ucli_status_t PIPE_MGR_LEARN_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

PIPE_MGR_LEARN_CLI_CMD_DECLARE(lrn_int_ena) {
  PIPE_MGR_CLI_PROLOGUE("lrn_int_ena",
                        "Enable interrupt based learn digest processing",
                        "-d <dev_id> -e <1:enable, 0:disable>");
  bool got_dev = false;
  bool got_en = false;

  bf_dev_id_t dev_id = 0;
  bool en = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:e:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
        got_dev = true;
        break;
      case 'e':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        en = strtoul(optarg, NULL, 0);
        got_en = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_dev || !got_en || dev_id < 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_flow_lrn_int_enable(dev_id, en);

  return UCLI_STATUS_OK;
}

PIPE_MGR_LEARN_CLI_CMD_DECLARE(lrn_cnt_reset) {
  PIPE_MGR_CLI_PROLOGUE(
      "lrn_cnt_reset", "Reset the learn DR digest count", "-d <dev_id>");

  bool got_dev = false;

  bf_dev_id_t dev_id = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
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

  pipe_mgr_flow_lrn_dr_count_reset(dev_id);

  return UCLI_STATUS_OK;
}

PIPE_MGR_LEARN_CLI_CMD_DECLARE(lrn_cnt) {
  PIPE_MGR_CLI_PROLOGUE(
      "lrn_cnt", "Print the learn DR digest count", "-d <dev_id>");

  bool got_dev = false;

  bf_dev_id_t dev_id = 0;
  uint64_t learn_digest_count = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:h:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev_id = strtoul(optarg, NULL, 0);
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

  learn_digest_count = pipe_mgr_flow_lrn_dr_count(dev_id);
  aim_printf(&uc->pvs, "Learn count : %" PRIu64 "\n", learn_digest_count);

  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f pipe_mgr_learn_ucli_ucli_handlers__[] = {
    PIPE_MGR_LEARN_CLI_CMD_HNDLR(lrn_cnt),
    PIPE_MGR_LEARN_CLI_CMD_HNDLR(lrn_cnt_reset),
    PIPE_MGR_LEARN_CLI_CMD_HNDLR(lrn_int_ena),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t pipe_mgr_learn_ucli_module__ = {
    "learn_ucli",
    NULL,
    pipe_mgr_learn_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_learn_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_learn_ucli_module__);
  m = ucli_node_create("learn", n, &pipe_mgr_learn_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("learn"));
  return m;
}

#else
void *pipe_mgr_learn_ucli_node_create(void) { return NULL; }
#endif
