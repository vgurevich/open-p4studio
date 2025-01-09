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


#include <dvm/dvm_config.h>

#if DVM_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include <dvm/bf_drv_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>

extern void bf_drv_dump_cfg(ucli_context_t *uc);

static ucli_status_t dvm_ucli_ucli__log_dev__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  const char *filepath;

  UCLI_COMMAND_INFO(uc, "log_dev", 2, "log_dev <asic> <filepath>");

  asic = atoi(uc->pargs->args[0]);
  filepath = uc->pargs->args[1];

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "DVM:: Log device <%d> to file %s\n", asic, filepath);

  bf_device_log(asic, filepath);
  return 0;
}

static ucli_status_t dvm_ucli_ucli__restore_dev__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  const char *filepath;

  UCLI_COMMAND_INFO(uc, "restore_dev", 2, "restore_dev <asic> <filepath>");

  asic = atoi(uc->pargs->args[0]);
  filepath = uc->pargs->args[1];

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(
      &uc->pvs, "DVM:: Restore device <%d> to file %s\n", asic, filepath);

  bf_device_restore(asic, filepath);
  return 0;
}

static void dvm_ucli_tolower(const char *str, char *str_new) {
  int i = 0, i_new = 0;
  int diff = 'A' - 'a';
  if ((str == NULL) || (str_new == NULL)) return;
  while (str[i] != '\0') {
    if ((str[i] == 'G') || (str[i] == 'R') || (str[i] == 'N') ||
        (str[i] == 'B')) {
      str_new[i_new++] = str[i++] - diff;
    } else {
      str_new[i_new++] = str[i++];
    }
  }
  str_new[i_new] = '\0';
  return;
}

static int dvm_ucli_get_speed(const char *str,
                              bf_port_speed_t *ps,
                              uint32_t *n_lanes) {
  uint32_t max_len = strlen("40G_NON_BREAKABLE");
  char str_new[max_len + 1];
  if (strlen(str) > max_len) return -1;
  dvm_ucli_tolower(str, str_new);

  if ((strcmp(str_new, "1g") == 0) || (strcmp(str_new, "1") == 0)) {
    *ps = BF_SPEED_1G;
    *n_lanes = 1;
    return 0;
  } else if ((strcmp(str_new, "10g") == 0) || (strcmp(str_new, "10") == 0)) {
    *ps = BF_SPEED_10G;
    *n_lanes = 1;
    return 0;
  } else if ((strcmp(str_new, "25g") == 0) || (strcmp(str_new, "25") == 0)) {
    *ps = BF_SPEED_25G;
    *n_lanes = 1;
    return 0;
  } else if ((strcmp(str_new, "40g") == 0) || ((strcmp(str_new, "40") == 0))) {
    *ps = BF_SPEED_40G;
    *n_lanes = 4;
    return 0;
  } else if ((strcmp(str_new, "40g-r2") == 0) ||
             (strcmp(str_new, "40-r2") == 0)) {
    *ps = BF_SPEED_40G_R2;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "50g") == 0) || (strcmp(str_new, "50") == 0)) {
    *ps = BF_SPEED_50G;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "50g-r2") == 0) ||
             (strcmp(str_new, "50-r2") == 0)) {
    *ps = BF_SPEED_50G;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "50g-r2-cons") == 0) ||
             (strcmp(str_new, "50-r2-cons") == 0)) {
    *ps = BF_SPEED_50G_CONS;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "50g-r1") == 0) ||
             (strcmp(str_new, "50-r1") == 0)) {
    *ps = BF_SPEED_50G;
    *n_lanes = 1;
    return 0;
  } else if ((strcmp(str_new, "100g") == 0) || (strcmp(str_new, "100") == 0)) {
    *ps = BF_SPEED_100G;
    *n_lanes = 4;
    return 0;
  } else if ((strcmp(str_new, "100g-r4") == 0) ||
             (strcmp(str_new, "100-r4") == 0)) {
    *ps = BF_SPEED_100G;
    *n_lanes = 4;
    return 0;
  } else if ((strcmp(str_new, "100g-r2") == 0) ||
             (strcmp(str_new, "100-r2") == 0)) {
    *ps = BF_SPEED_100G;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "100g-r1") == 0) ||
             (strcmp(str_new, "100-r1") == 0)) {
    *ps = BF_SPEED_100G;
    *n_lanes = 1;
    return 0;
  } else if ((strcmp(str_new, "200g") == 0) || (strcmp(str_new, "200") == 0)) {
    *ps = BF_SPEED_200G;
    *n_lanes = 4;
    return 0;
  } else if ((strcmp(str_new, "200g-r2") == 0) ||
             (strcmp(str_new, "200-r2") == 0)) {
    *ps = BF_SPEED_200G;
    *n_lanes = 2;
    return 0;
  } else if ((strcmp(str_new, "200g-r4") == 0) ||
             (strcmp(str_new, "200-r4") == 0)) {
    *ps = BF_SPEED_200G;
    *n_lanes = 4;
    return 0;
  } else if ((strcmp(str_new, "200g-r8") == 0) ||
             (strcmp(str_new, "200-r8") == 0)) {
    *ps = BF_SPEED_200G;
    *n_lanes = 8;
    return 0;
  } else if ((strcmp(str_new, "400g") == 0) || (strcmp(str_new, "400") == 0)) {
    *ps = BF_SPEED_400G;
    *n_lanes = 8;
    return 0;
  } else if ((strcmp(str_new, "400g-r4") == 0) ||
             (strcmp(str_new, "400-r4") == 0)) {
    *ps = BF_SPEED_400G;
    *n_lanes = 4;
    return 0;

  } else {
    return -1;
  }
}

static int dvm_ucli_fec_type_get(const char *str, bf_fec_type_t *fec_type) {
  if ((strcmp(str, "NONE") == 0) || (strcmp(str, "none") == 0)) {
    *fec_type = BF_FEC_TYP_NONE;
    return 0;
  } else if ((strcmp(str, "FC") == 0) || (strcmp(str, "fc") == 0)) {
    *fec_type = BF_FEC_TYP_FIRECODE;
    return 0;
  } else if ((strcmp(str, "RS") == 0) || (strcmp(str, "rs") == 0)) {
    *fec_type = BF_FEC_TYP_REED_SOLOMON;
    return 0;
  } else {
    return -1;
  }
}

static ucli_status_t dvm_ucli_ucli__add_port__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int port, fec_arg, n_lanes;
  bf_port_speeds_t port_speed;
  bf_fec_types_t port_fec_type;

  UCLI_COMMAND_INFO(uc,
                    "add_port",
                    4,
                    "add_port <asic> <devport> <speed (1G, 10G, 25G, 40G, "
                    "40G-R2, "
                    "50G(50G/50G-R2, 50G-R2-C, 50G-R1), 100G(100G/100G-R4, "
                    "100G-R2, 100G-R1), 200G(200G/200G-R4, 200G-R2, 200G-R8), "
                    "400G(400G/400G-R8, 400G-R4))> <fec (NONE, FC, RS)>");

  asic = atoi(uc->pargs->args[0]);
  port = strtoul(uc->pargs->args[1], NULL, 0);
  fec_arg = atoi(uc->pargs->args[3]);

  port_fec_type = (fec_arg == 0)
                      ? BF_FEC_TYP_NONE
                      : (fec_arg == 1) ? BF_FEC_TYP_FC : BF_FEC_TYP_RS;

  if (dvm_ucli_get_speed(uc->pargs->args[2], &port_speed, &n_lanes) != 0) {
    aim_printf(&uc->pvs, "Unknown speed\n");
    return 0;
  }

  if (dvm_ucli_fec_type_get(uc->pargs->args[3], &port_fec_type) != 0) {
    aim_printf(&uc->pvs, "Unknown FEC type\n");
    return 0;
  }

  bf_status_t rc2, rc = bf_port_add_with_lane_numb(
                       asic, port, port_speed, n_lanes, port_fec_type);
  char *speed_str = "";
  rc2 = bf_port_speed_to_str(port_speed, &speed_str);
  if (rc2 != BF_SUCCESS) {
    speed_str = "unknown";
  }
  aim_printf(&uc->pvs,
             "DVM:: Add port asic %d port %d speed %s <%s> status %s\n",
             asic,
             port,
             speed_str,
             bf_fec_type_str(port_fec_type),
             bf_err_str(rc));

  return 0;
}

static ucli_status_t dvm_ucli_ucli__ena_port__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int port;

  UCLI_COMMAND_INFO(uc, "ena_port", 2, "ena_port <asic> <port>");

  asic = atoi(uc->pargs->args[0]);
  port = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "DVM:: Enable port <asic=%d> <port=%d>\n", asic, port);

  bf_port_enable(asic, port, true);
  return 0;
}

static ucli_status_t dvm_ucli_ucli__dis_port__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int port;

  UCLI_COMMAND_INFO(uc, "dis_port", 2, "dis_port <asic> <port>");

  asic = atoi(uc->pargs->args[0]);
  port = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "DVM:: Disable port <asic=%d> <port=%d>\n", asic, port);

  bf_port_enable(asic, port, false);
  return 0;
}

static ucli_status_t dvm_ucli_ucli__rmv_port__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int port;

  UCLI_COMMAND_INFO(uc, "rmv_port", 2, "rmv_port <asic> <port>");

  asic = atoi(uc->pargs->args[0]);
  port = atoi(uc->pargs->args[1]);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  aim_printf(&uc->pvs, "DVM:: Remove port <asic=%d> <port=%d>\n", asic, port);

  bf_port_remove(asic, port);
  return 0;
}

static ucli_status_t dvm_ucli_ucli__port_ct__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  unsigned int port, ct_mode;
  bf_status_t rc = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "port-ct", 3, "port-ct <asic> <port> <0/1>");

  asic = atoi(uc->pargs->args[0]);
  port = atoi(uc->pargs->args[1]);
  ct_mode = atoi(uc->pargs->args[2]);

  if (asic >= BF_MAX_DEV_COUNT) {
    aim_printf(&uc->pvs,
               "Only %d chips defined. Correct command or update "
               "lldif/inc/lldif/sdk_bsp.h\n",
               BF_MAX_DEV_COUNT);
    return 0;
  }

  if (ct_mode) {
    rc = bf_pal_port_cut_through_enable(asic, port);
  } else {
    rc = bf_pal_port_cut_through_disable(asic, port);
  }

  aim_printf(&uc->pvs,
             "DVM:: port-ct <asic=%d> <port=%d> <ct-mode=%d> Status=%s\n",
             asic,
             port,
             ct_mode,
             bf_err_str(rc));

  return 0;
}

static ucli_status_t dvm_ucli_ucli__cfg__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "cfg", 0, "cfg");

  aim_printf(&uc->pvs, "DVM:: Board configuration:\n");
  bf_drv_dump_cfg(uc);

  return 0;
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */
static ucli_command_handler_f dvm_ucli_ucli_handlers__[] = {
    dvm_ucli_ucli__log_dev__,
    dvm_ucli_ucli__restore_dev__,
    dvm_ucli_ucli__add_port__,
    dvm_ucli_ucli__rmv_port__,
    dvm_ucli_ucli__ena_port__,
    dvm_ucli_ucli__dis_port__,
    dvm_ucli_ucli__port_ct__,
    dvm_ucli_ucli__cfg__,
    NULL};

static ucli_module_t dvm_ucli_module__ = {
    "dvm_ucli",
    NULL,
    dvm_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *dvm_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&dvm_ucli_module__);
  n = ucli_node_create("dvm", NULL, &dvm_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("dvm"));
  return n;
}

ucli_status_t bf_drv_show_tech_ucli_dvm__(ucli_context_t *uc) {
  aim_printf(&uc->pvs, "-------------------- DVM --------------------\n");
  dvm_ucli_ucli__cfg__(uc);
  return 0;
}

#ifndef ENABLE_SWITCHDUCLI
ucli_node_t *switchd_ucli_node_create(void) { return NULL; }
#endif /* ENABLE_SWITCHDUCLI */

#else
void *dvm_ucli_node_create(void) { return NULL; }
#endif
