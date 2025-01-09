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


#include <getopt.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include <traffic_mgr/traffic_mgr_config.h>

#include "traffic_mgr/common/tm_hw_access.h"
#include "traffic_mgr/common/tm_init.h"
#include "traffic_mgr_ucli_tables.h"

#if TRAFFIC_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define TM_UCLI_DEV_PIPE_DECL_    \
  volatile bf_dev_id_t devid = 0; \
  volatile int pipe = 0;          \
  volatile int hex = 0;           \
  volatile int nz = 0;            \
  int c;                          \
  int argc, arg_start = 0;        \
  size_t i;                       \
  char *const *argv;              \
  extern char *optarg;

#define TM_UCLI_DEV_DECL_         \
  volatile bf_dev_id_t devid = 0; \
  volatile int hex = 0;           \
  volatile int nz = 0;            \
  int c;                          \
  int argc, arg_start = 0;        \
  char *const *argv;              \
  extern char *optarg;            \
  size_t i;

#define TM_UCLI_DEV_PIPE_RSRC_DECL_ \
  bf_dev_id_t devid = 0;            \
  int pipe = 0;                     \
  int rsrc = 0;                     \
  int hex = 0;                      \
  int nz = 0;                       \
  int c;                            \
  int argc, arg_start = 0;          \
  size_t i;                         \
  char *const *argv;                \
  extern char *optarg;

#define TM_UCLI_DEV_PIPE(table_name)                                          \
  for (i = 0;                                                                 \
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);      \
       i++) {                                                                 \
    if (!strncmp(uc->pargs[0].args__[i], #table_name, strlen(#table_name))) { \
      arg_start = i;                                                          \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
  if (i >= sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0])) {    \
    return UCLI_STATUS_OK;                                                    \
  }                                                                           \
  argc = (uc->pargs->count + 1);                                              \
  argv = (char *const *)&(uc->pargs->args__[arg_start]);                      \
  optind = 0;                                                                 \
  if (argc == 1) {                                                            \
    devid = 0;                                                                \
    pipe = 0;                                                                 \
  } else {                                                                    \
    while (argv && (c = getopt(argc, argv, "zXd:p:")) != -1) {                \
      switch (c) {                                                            \
        case 'd':                                                             \
          devid = strtoul(optarg, NULL, 0);                                   \
          if (devid >= BF_TM_NUM_ASIC) {                                      \
            printf("Incorrect device ID: %d\n", devid);                       \
            printf("Using device ID 0\n");                                    \
            devid = 0;                                                        \
          }                                                                   \
          if (!g_tm_ctx_valid[devid]) {                                       \
            aim_printf(&uc->pvs, "Device ID %d is not registered.\n", devid); \
            return UCLI_STATUS_CONTINUE;                                      \
          }                                                                   \
          break;                                                              \
        case 'p':                                                             \
          pipe = strtoul(optarg, NULL, 0);                                    \
          if (pipe >= BF_PIPE_COUNT) {                                        \
            printf("Incorrect pipe ID: %d\n", pipe);                          \
            printf("Using pipe ID 0\n");                                      \
            pipe = 0;                                                         \
          }                                                                   \
          break;                                                              \
        case 'X':                                                             \
          hex = 1;                                                            \
          break;                                                              \
        case 'z':                                                             \
          nz = 1;                                                             \
          break;                                                              \
        default:                                                              \
          printf("%s\n",                                                      \
                 "Usage: -d <dev_id> -p <pipe_id> -X [hex output] -z [print " \
                 "zero values too]");                                         \
          printf("%s\n", "Using device ID 0, pipeid 0");                      \
          devid = 0;                                                          \
          pipe = 0;                                                           \
      }                                                                       \
    }                                                                         \
  }

#define TM_UCLI_DEV(table_name)                                                \
  for (i = 0;                                                                  \
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);       \
       i++) {                                                                  \
    if (!strncmp(uc->pargs[0].args__[i], #table_name, strlen(#table_name))) {  \
      arg_start = i;                                                           \
      break;                                                                   \
    }                                                                          \
  }                                                                            \
  if (i >= sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0])) {     \
    return UCLI_STATUS_OK;                                                     \
  }                                                                            \
  argc = (uc->pargs->count + 1);                                               \
  argv = (char *const *)&(uc->pargs->args__[arg_start]);                       \
  optind = 0;                                                                  \
  if (argc == 1) {                                                             \
    devid = 0;                                                                 \
  } else {                                                                     \
    while (argv && (c = getopt(argc, argv, "zXd:")) != -1) {                   \
      switch (c) {                                                             \
        case 'd':                                                              \
          devid = strtoul(optarg, NULL, 0);                                    \
          if (devid >= BF_TM_NUM_ASIC) {                                       \
            printf("Incorrect device ID: %d\n", devid);                        \
            printf("Using device ID 0\n");                                     \
            devid = 0;                                                         \
          }                                                                    \
          if (!g_tm_ctx_valid[devid]) {                                        \
            aim_printf(&uc->pvs, "Device ID %d is not registered.\n", devid);  \
            return UCLI_STATUS_CONTINUE;                                       \
          }                                                                    \
          break;                                                               \
        case 'X':                                                              \
          hex = 1;                                                             \
          break;                                                               \
        case 'z':                                                              \
          nz = 1;                                                              \
          break;                                                               \
        default:                                                               \
          printf("%s\n",                                                       \
                 "Usage: -d <dev_id> -X [hex output] -z [print zero counters " \
                 "too]");                                                      \
          printf("%s\n", "Using device ID 0");                                 \
          devid = 0;                                                           \
      }                                                                        \
    }                                                                          \
  }

#define TM_UCLI_DEV_PIPE_RSRC(rsrc_name)                                      \
  for (i = 0;                                                                 \
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);      \
       i++) {                                                                 \
    if (!strncmp(uc->pargs[0].args__[i], #rsrc_name, strlen(#rsrc_name))) {   \
      arg_start = i;                                                          \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
  if (i >= sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0])) {    \
    return UCLI_STATUS_OK;                                                    \
  }                                                                           \
  argc = (uc->pargs->count + 1);                                              \
  argv = (char *const *)&(uc->pargs->args__[arg_start]);                      \
  optind = 0;                                                                 \
  if (argc == 1) {                                                            \
    devid = 0;                                                                \
    pipe = 0;                                                                 \
  } else {                                                                    \
    while (argv && (c = getopt(argc, argv, "zXd:p:r:")) != -1) {              \
      switch (c) {                                                            \
        case 'd':                                                             \
          devid = strtoul(optarg, NULL, 0);                                   \
          if (devid >= BF_TM_NUM_ASIC) {                                      \
            printf("Incorrect device ID: %d\n", devid);                       \
            printf("Using device ID 0\n");                                    \
            devid = 0;                                                        \
          }                                                                   \
          if (!g_tm_ctx_valid[devid]) {                                       \
            aim_printf(&uc->pvs, "Device ID %d is not registered.\n", devid); \
            return UCLI_STATUS_CONTINUE;                                      \
          }                                                                   \
          break;                                                              \
        case 'p':                                                             \
          pipe = strtoul(optarg, NULL, 0);                                    \
          if (pipe >= BF_PIPE_COUNT) {                                        \
            printf("Incorrect pipe ID: %d\n", pipe);                          \
            printf("Using pipe ID 0\n");                                      \
            pipe = 0;                                                         \
          }                                                                   \
          break;                                                              \
        case 'r':                                                             \
          rsrc = strtoul(optarg, NULL, 0);                                    \
          break;                                                              \
        case 'X':                                                             \
          hex = 1;                                                            \
          break;                                                              \
        case 'z':                                                             \
          nz = 1;                                                             \
          break;                                                              \
        default:                                                              \
          printf("%s\n",                                                      \
                 "Usage: -d <dev_id> -p <pipe_id>  -r <ppg#/q#/port#> -X "    \
                 "[hex output] -z [print zero values too]");                  \
          printf("%s\n", "Using device ID 0, pipeid 0");                      \
          devid = 0;                                                          \
          pipe = 0;                                                           \
      }                                                                       \
    }                                                                         \
  }

#define TRAFFIC_MGR_CLI_CMD_HNDLR(name) traffic_mgr_ucli_ucli__##name##__

#define TRAFFIC_MGR_CLI_CMD_DECLARE(name) \
  static ucli_status_t TRAFFIC_MGR_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

void tm_tof3_display_command_not_supported(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  (void)devid;
  (void)pipe;
  aim_printf(&uc->pvs, "This command is NOT supported on this TOF3 currently");
  aim_printf(&uc->pvs, "\n");

  return;
}

void tm_tof2_display_command_not_supported(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  (void)devid;
  (void)pipe;
  aim_printf(&uc->pvs, "This command is NOT supported on this TOF2 currently");
  aim_printf(&uc->pvs, "\n");

  return;
}

void tm_tof_display_command_not_supported(
    ucli_context_t *uc, int nz, int hex, bf_dev_id_t devid, int pipe) {
  (void)nz;
  (void)hex;
  (void)devid;
  (void)pipe;
  aim_printf(&uc->pvs,
             "This command is NOT supported on this TOFINO currently");
  aim_printf(&uc->pvs, "\n");

  return;
}

/********************* TM Config Table CLIs  ************************/
#undef TM_NEW_PER_PIPE_TABLE
#undef TM_NEW_NONPIPE_TABLE

#define TM_NEW_PER_PIPE_TABLE(table_name, description, func)                \
  TRAFFIC_MGR_CLI_CMD_DECLARE(table_name) {                                 \
    TM_UCLI_DEV_PIPE_DECL_                                                  \
    UCLI_COMMAND_INFO(uc,                                                   \
                      #table_name,                                          \
                      -1,                                                   \
                      "  " #description                                     \
                      "  Usage: -d <devid> -p <pipe> -X [print in hex] -z " \
                      "[print zero counters too]");                         \
    TM_UCLI_DEV_PIPE(table_name)                                            \
    return func(uc, nz, hex, devid, pipe);                                  \
  }

#define TM_NEW_NONPIPE_TABLE(table_name, description, func)                \
  TRAFFIC_MGR_CLI_CMD_DECLARE(table_name) {                                \
    TM_UCLI_DEV_DECL_                                                      \
    UCLI_COMMAND_INFO(uc,                                                  \
                      #table_name,                                         \
                      -1,                                                  \
                      "  " #description                                    \
                      "   Usage: -d <devid>, -X [print in hex] -z [print " \
                      "zero counters too]");                               \
    TM_UCLI_DEV(table_name)                                                \
    return func(uc, nz, hex, devid);                                       \
  }

// Define all TM per pipe tables  here
#define TM_PIPE_CFG_TABLES                                                   \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      wac_qid_map, WAC Qid Map Tof2 Only, tm_ucli_display_wac_qid_map)       \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_min, PPG Minimum Limits, tm_ucli_display_ppg_gmin_limit)           \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_hdr, PPG Head Room Limits, tm_ucli_display_ppg_hdr_limit)          \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      port_limit, WAC Max Port Limits, tm_ucli_display_ing_port_drop_limit)  \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_resume_limit, PPG Resume Limits, tm_ucli_display_ppg_resume_limit) \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      port_ppg, Port to PPG Mapping Table, tm_ucli_display_port_ppg)         \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_shared_limit, PPG Shared Limits, tm_ucli_display_ppg_shared_limit) \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_icos_map, PPG iCOS Mapping, tm_ucli_display_ppg_icos_mapping)      \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      ppg_offset_prof, Offset Profile, tm_ucli_display_wac_offset_profile)   \
  TM_NEW_PER_PIPE_TABLE(q_min_thrd, Q Min Limit, tm_ucli_display_q_min_thrd) \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      q_shr_thrd, Q Shared Limit, tm_ucli_display_q_shr_thrd)                \
  TM_NEW_PER_PIPE_TABLE(q_ap, Q App pool Mapping, tm_ucli_display_q_ap)      \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      q_color_limit, Q Color Limit, tm_ucli_display_q_color_limit)           \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      qac_port_limit, QAC Port Limit, tm_ucli_display_qac_port_limit)        \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      qac_qid_profile, QAC Q Profile ID, tm_ucli_display_qac_qid_profile)    \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      q_offset_prof, Offset Profile, tm_ucli_display_qac_offset_profile)     \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      qac_qid_map, QAC Q Mapping, tm_ucli_display_qac_qid_map)               \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      q_min_shaper, SCH Q Min Rate, tm_ucli_display_q_min_rate)              \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      q_max_shaper, SCH Q Max Rate, tm_ucli_display_q_max_rate)              \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      Port_max_shaper, SCH Port Max Rate, tm_ucli_display_port_max_rate)     \
  TM_NEW_PER_PIPE_TABLE(                                                     \
      port_q_map, PortQ to physQ mapping, tm_ucli_display_port_q_mapping)

// Define all TM tables that are not pipe specific
#define TM_NONPIPE_CFG_TABLES                              \
  TM_NEW_NONPIPE_TABLE(wac_eg_qid_map,                     \
                       WAC Egress QID mapping Tofino only, \
                       tm_ucli_display_wac_eg_qid_map)

// Define CLI functionalities that are not pipe specific
#define TM_NONPIPE_DDR_CLI \
  TM_NEW_NONPIPE_TABLE(train, DDR training TF3 only, tm_ucli_set_ddr_train)

/*
 * MACRO TRICKS FROM HERE ON... Generally not needed to edit / add
 * in the following code. To add new table expand TM_PIPE_TABLES and
 * TM_NONPIPE_TABLES listed above.
 */

TM_PIPE_CFG_TABLES
TM_NONPIPE_CFG_TABLES
TM_NONPIPE_DDR_CLI

#undef TM_NEW_PER_PIPE_TABLE
#undef TM_NEW_NONPIPE_TABLE

#define TM_NEW_PER_PIPE_TABLE(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),
#define TM_PER_PIPE_CFG_TABLE_HNDLR TM_PIPE_CFG_TABLES

#define TM_NEW_NONPIPE_TABLE(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),
#define TM_NONPIPE_CFG_TABLE_HNDLR TM_NONPIPE_CFG_TABLES
#define TM_NONPIPE_DDR_HNDLR TM_NONPIPE_DDR_CLI

static ucli_command_handler_f traffic_mgr_cfg_table_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_CFG_TABLE_HNDLR TM_NONPIPE_CFG_TABLE_HNDLR NULL};

static ucli_command_handler_f traffic_mgr_ddr_ucli_ucli_handlers__[] = {
    TM_NONPIPE_DDR_HNDLR NULL};

static ucli_module_t traffic_mgr_cfg_table_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_cfg_table_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_module_t traffic_mgr_ddr_ucli_module__ = {
    "traffic_mgr_ddr_ucli",
    NULL,
    traffic_mgr_ddr_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_cfg_table_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_cfg_table_ucli_module__);
  m = ucli_node_create("cfg_table", n, &traffic_mgr_cfg_table_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("cfg_table"));
  return m;
}

static ucli_node_t *tm_ddr_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_ddr_ucli_module__);
  m = ucli_node_create("ddr", n, &traffic_mgr_ddr_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("ddr"));
  return m;
}

/************* TM resource dump routines*****************/

#define TM_NEW_PER_PIPE_RSRC(rsrc_name, description, func)                   \
  TRAFFIC_MGR_CLI_CMD_DECLARE(rsrc_name) {                                   \
    TM_UCLI_DEV_PIPE_RSRC_DECL_                                              \
    UCLI_COMMAND_INFO(uc,                                                    \
                      #rsrc_name,                                            \
                      -1,                                                    \
                      "  " #description                                      \
                      "  Usage: -d <devid> -p <pipe> -r <ppg#/q#/port#> -X " \
                      "[print in hex] -z [print zero counters too]");        \
    TM_UCLI_DEV_PIPE_RSRC(rsrc_name)                                         \
    return func(uc, nz, hex, devid, pipe, rsrc);                             \
  }

#define TM_PIPE_RSRC_CLI                                                \
  TM_NEW_PER_PIPE_RSRC(ppg, PPG Details, tm_ucli_display_ppg_details)   \
  TM_NEW_PER_PIPE_RSRC(q, Queue Details, tm_ucli_display_queue_details) \
  TM_NEW_PER_PIPE_RSRC(port, TM Port Details, tm_ucli_display_port_details)

TM_PIPE_RSRC_CLI

#undef TM_NEW_PER_PIPE_RSRC

#define TM_NEW_PER_PIPE_RSRC(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#define TM_PER_PIPE_RSRC_HNDLR TM_PIPE_RSRC_CLI

static ucli_command_handler_f traffic_mgr_rsrc_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_RSRC_HNDLR NULL};

static ucli_module_t traffic_mgr_rsrc_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_rsrc_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_rsrc_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_rsrc_ucli_module__);
  m = ucli_node_create("cfg", n, &traffic_mgr_rsrc_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("cfg"));
  return m;
}

/************* TM Usage, WaterMark and Drop Status Tables *****************/

#undef TM_NEW_PER_PIPE_TABLE
#undef TM_NEW_NONPIPE_TABLE

#define TM_NEW_PER_PIPE_TABLE(table_name, description, func)                \
  TRAFFIC_MGR_CLI_CMD_DECLARE(table_name) {                                 \
    TM_UCLI_DEV_PIPE_DECL_                                                  \
    UCLI_COMMAND_INFO(uc,                                                   \
                      #table_name,                                          \
                      -1,                                                   \
                      "  " #description                                     \
                      "  Usage: -d <devid> -p <pipe> -X [print in hex] -z " \
                      "[print zero values too]");                           \
    TM_UCLI_DEV_PIPE(table_name)                                            \
    return func(uc, nz, hex, devid, pipe);                                  \
  }

#define TM_NEW_NONPIPE_TABLE(table_name, description, func)                \
  TRAFFIC_MGR_CLI_CMD_DECLARE(table_name) {                                \
    TM_UCLI_DEV_DECL_                                                      \
    UCLI_COMMAND_INFO(uc,                                                  \
                      #table_name,                                         \
                      -1,                                                  \
                      "  " #description                                    \
                      "   Usage: -d <devid>, -X [print in hex] -z [print " \
                      "zero values too]");                                 \
    TM_UCLI_DEV(table_name)                                                \
    return func(uc, nz, hex, devid);                                       \
  }

#define TM_PIPE_USAGE_TABLES                                      \
  TM_NEW_PER_PIPE_TABLE(ppg_gmin_usage,                           \
                        PPG cell usage in Gmin Pool,              \
                        tm_ucli_display_ppg_cell_usage_gmin)      \
  TM_NEW_PER_PIPE_TABLE(dpg_gmin_usage,                           \
                        DPG cell usage in Gmin Pool,              \
                        tm_ucli_display_dpg_cell_usage_gmin)      \
  TM_NEW_PER_PIPE_TABLE(ppg_shrd_usage,                           \
                        PPG cell usage in shared Pool,            \
                        tm_ucli_display_ppg_cell_usage_shrd_pool) \
  TM_NEW_PER_PIPE_TABLE(dpg_shrd_usage,                           \
                        DPG cell usage in shared Pool,            \
                        tm_ucli_display_dpg_cell_usage_shrd_pool) \
  TM_NEW_PER_PIPE_TABLE(ppg_skid_usage,                           \
                        PPG cell usage in Skid pool,              \
                        tm_ucli_display_ppg_cell_usage_skid_pool) \
  TM_NEW_PER_PIPE_TABLE(wac_portusage,                            \
                        Port cell usage in wac,                   \
                        tm_ucli_display_wac_port_usage_count)     \
  TM_NEW_PER_PIPE_TABLE(qac_portusage,                            \
                        Port cell usage in QAC,                   \
                        tm_ucli_display_qac_port_cellusage)       \
  TM_NEW_PER_PIPE_TABLE(                                          \
      qac_q_usage, Queue cell usage in QAC, tm_ucli_display_qac_q_cellusage)

#define TM_PIPE_WATERMARK_TABLES                                           \
  TM_NEW_PER_PIPE_TABLE(                                                   \
      ppg_wm, PPG Usage WaterMark, tm_ucli_display_wac_ppg_watermark)      \
  TM_NEW_PER_PIPE_TABLE(wac_port_wm,                                       \
                        Port Usage WaterMark in WAC,                       \
                        tm_ucli_display_wac_port_watermark)                \
  TM_NEW_PER_PIPE_TABLE(q_wm, Q Usage WaterMark, tm_ucli_display_qac_q_wm) \
  TM_NEW_PER_PIPE_TABLE(                                                   \
      qac_port_wm, Port Usage WaterMark in QAC, tm_ucli_display_qac_port_wm)

#define TM_PIPE_DROPSTATUS_TABLES                                             \
  TM_NEW_PER_PIPE_TABLE(                                                      \
      ppg_drop_st, Drop status of PPG, tm_ucli_display_ppg_dropstate)         \
  TM_NEW_PER_PIPE_TABLE(port_drop_st,                                         \
                        Drop status of Port in Wac,                           \
                        tm_ucli_display_port_dropstate)                       \
  TM_NEW_PER_PIPE_TABLE(                                                      \
      q_green_drop, QAC Q Drop State(Green), tm_ucli_display_q_green_drop)    \
  TM_NEW_PER_PIPE_TABLE(                                                      \
      q_yellow_drop, QAC Q Drop State(Yellow), tm_ucli_display_q_yellow_drop) \
  TM_NEW_PER_PIPE_TABLE(                                                      \
      q_red_drop, QAC Q Drop State(Red), tm_ucli_display_q_red_drop)          \
  TM_NEW_PER_PIPE_TABLE(port_drop_State,                                      \
                        Port Drop Status in QAC,                              \
                        tm_ucli_display_qac_port_drop_state)

#define TM_NONPIPE_DROPSTATUS_TABLES                                          \
  TM_NEW_NONPIPE_TABLE(colordrop,                                             \
                       Color Drop State in wac,                               \
                       tm_ucli_display_wac_color_drop_state)                  \
  TM_NEW_NONPIPE_TABLE(skiddrop,                                              \
                       Skid Pool Drop State in wac,                           \
                       tm_ucli_display_wac_skidpool_dropstate)                \
  TM_NEW_NONPIPE_TABLE(wac_qshadowstate,                                      \
                       Queue Shadow State in wac,                             \
                       tm_ucli_display_wac_queue_shadow_state)                \
  TM_NEW_NONPIPE_TABLE(ap_drop_state,                                         \
                       Application Pool Drop status in qac,                   \
                       tm_ucli_display_qac_dropstate)                         \
  TM_NEW_NONPIPE_TABLE(apg_green_drop_state,                                  \
                       Application Pool Drop status of green packets in qac,  \
                       tm_ucli_display_qac_green_dropstate)                   \
  TM_NEW_NONPIPE_TABLE(apy_yel_drop_state,                                    \
                       Application Pool Drop status of yellow packets in qac, \
                       tm_ucli_display_qac_yel_dropstate)                     \
  TM_NEW_NONPIPE_TABLE(apr_red_drop_state,                                    \
                       Application Pool Drop status of red packets in qac,    \
                       tm_ucli_display_qac_red_dropstate)                     \
  TM_NEW_NONPIPE_TABLE(prefifo_drop_state,                                    \
                       Pre FIFO Drop status,                                  \
                       tm_ucli_display_qac_pipe_pre_fifo_dropstate)

#define TM_PIPE_PFCSTATUS_TABLES                                \
  TM_NEW_PER_PIPE_TABLE(port_pfc,                               \
                        Per port pfc State generated in wac,    \
                        tm_ucli_display_wac_pfc_state)          \
  TM_NEW_PER_PIPE_TABLE(port_pfc_rx,                            \
                        Per port pfc state received in qac,     \
                        tm_ucli_display_egress_port_pfc_status) \
  TM_NEW_PER_PIPE_TABLE(q_pfc_rx,                               \
                        Per q pfc state received in qac,        \
                        tm_ucli_display_egress_q_pfc_status)

TM_PIPE_USAGE_TABLES
TM_PIPE_WATERMARK_TABLES
TM_PIPE_DROPSTATUS_TABLES
TM_NONPIPE_DROPSTATUS_TABLES
TM_PIPE_PFCSTATUS_TABLES

#undef TM_NEW_PER_PIPE_TABLE
#undef TM_NEW_NONPIPE_TABLE
#define TM_NEW_PER_PIPE_TABLE(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),
#define TM_NEW_NONPIPE_TABLE(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#define TM_PER_PIPE_USAGE_HNDLR TM_PIPE_USAGE_TABLES
#define TM_PER_PIPE_WATERMARK_HNDLR TM_PIPE_WATERMARK_TABLES
#define TM_PER_PIPE_DROPSTATUS_HNDLR TM_PIPE_DROPSTATUS_TABLES
#define TM_NONPIPE_DROPSTATUS_HNDLR TM_NONPIPE_DROPSTATUS_TABLES
#define TM_PER_PIPE_PFCSTATUS_HNDLR TM_PIPE_PFCSTATUS_TABLES

static ucli_command_handler_f traffic_mgr_usage_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_USAGE_HNDLR NULL};

static ucli_command_handler_f traffic_mgr_watermark_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_WATERMARK_HNDLR NULL};

static ucli_command_handler_f traffic_mgr_dropstatus_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_DROPSTATUS_HNDLR NULL};

static ucli_command_handler_f
    traffic_mgr_nonpipe_dropstatus_ucli_ucli_handlers__[] = {
        TM_NONPIPE_DROPSTATUS_HNDLR NULL};

static ucli_command_handler_f traffic_mgr_pfcstatus_ucli_ucli_handlers__[] = {
    TM_PER_PIPE_PFCSTATUS_HNDLR NULL};

static ucli_module_t traffic_mgr_usage_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_usage_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_module_t traffic_mgr_watermark_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_watermark_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_module_t traffic_mgr_dropstatus_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_dropstatus_ucli_ucli_handlers__,
    NULL,
    NULL,
};
static ucli_module_t traffic_mgr_nonpipe_dropstatus_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_nonpipe_dropstatus_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_module_t traffic_mgr_pfcstatus_ucli_module__ = {
    "traffic_mgr_table_ucli",
    NULL,
    traffic_mgr_pfcstatus_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_usage_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_usage_ucli_module__);
  m = ucli_node_create("usage", n, &traffic_mgr_usage_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("usage"));
  return m;
}

static ucli_node_t *tm_watermark_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_watermark_ucli_module__);
  m = ucli_node_create("watermark", n, &traffic_mgr_watermark_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("watermark"));
  return m;
}

static ucli_node_t *tm_dropstatus_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_dropstatus_ucli_module__);
  m = ucli_node_create("dropstatus", n, &traffic_mgr_dropstatus_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("per pipe dropstatus"));
  return m;
}

static ucli_node_t *tm_nonpipe_dropstatus_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_nonpipe_dropstatus_ucli_module__);
  m = ucli_node_create(
      "dl_dropstatus", n, &traffic_mgr_nonpipe_dropstatus_ucli_module__);
  ucli_node_subnode_add(m,
                        ucli_module_log_node_create("device level dropstatus"));
  return m;
}

static ucli_node_t *tm_pfcstatus_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_pfcstatus_ucli_module__);
  m = ucli_node_create("pfcstatus", n, &traffic_mgr_pfcstatus_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("pfcstatus"));
  return m;
}

/********************* TM Counter CLIs  ************************/

#define TM_PERPIPE_MODULE_COUNTER_AND_CTBL(module, description, func1, func2) \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                                       \
    TM_UCLI_DEV_PIPE_DECL_                                                    \
    UCLI_COMMAND_INFO(uc,                                                     \
                      #module,                                                \
                      -1,                                                     \
                      "  " #description                                       \
                      "  Usage: -d <devid> -p <pipe> -X [print in hex] -z "   \
                      "[print zero counters too]");                           \
    TM_UCLI_DEV_PIPE(module)                                                  \
    func1(uc, nz, hex, devid, pipe);                                          \
    func2(uc, nz, hex, devid, pipe);                                          \
    return UCLI_STATUS_OK;                                                    \
  }

#define TM_PERPIPE_ALLMODULE_COUNTER(module, description, func)             \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                                     \
    TM_UCLI_DEV_PIPE_DECL_                                                  \
    UCLI_COMMAND_INFO(uc,                                                   \
                      #module,                                              \
                      -1,                                                   \
                      "  " #description                                     \
                      "  Usage: -d <devid> -p <pipe> -X [print in hex] -z " \
                      "[print zero counters too]");                         \
    TM_UCLI_DEV_PIPE(module)                                                \
    return func(uc, nz, hex, devid, pipe);                                  \
  }

#define TM_NONPIPE_ALLMODULE_COUNTER(module, description, func)    \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                            \
    TM_UCLI_DEV_DECL_                                              \
    UCLI_COMMAND_INFO(uc,                                          \
                      #module,                                     \
                      -1,                                          \
                      "  " #description                            \
                      "  Usage: -d <devid>  -X [print in hex] -z " \
                      "[print zero counters too]");                \
    TM_UCLI_DEV(module)                                            \
    return func(uc, nz, hex, devid);                               \
  }

#define TM_PERPIPE_ALLMODULE_COUNTERS                                       \
  TM_PERPIPE_ALLMODULE_COUNTER(                                             \
      blocklevel, Dump all block counters, tm_ucli_display_perpipe_counter) \
  TM_PERPIPE_ALLMODULE_COUNTER(wac_perport,                                 \
                               Dump per port drop counter as seen in wac,   \
                               tm_ucli_display_wac_per_port_drop_counter)   \
  TM_PERPIPE_ALLMODULE_COUNTER(wac_perppg,                                  \
                               Dump per ppg drop counter as seen in wac,    \
                               tm_ucli_display_wac_per_ppg_drop_counter)    \
  TM_PERPIPE_ALLMODULE_COUNTER(qac_q_drop,                                  \
                               Dump per queue drop counter as seen in qac,  \
                               tm_ucli_display_qac_queue_drop_counter)      \
  TM_PERPIPE_ALLMODULE_COUNTER(qac_port_drop,                               \
                               Dump per port drop counter as seen in qac,   \
                               tm_ucli_display_qac_port_drop_color_counter) \
  TM_NONPIPE_ALLMODULE_COUNTER(pre_fifo_drop,                               \
                               Dump all pre fifo drop counter,              \
                               tm_ucli_display_nonpipe_counter)             \
  TM_NONPIPE_ALLMODULE_COUNTER(stop_stats_cache_timer,                      \
                               Stop TM Stats Cache Timer,                   \
                               tm_ucli_stop_stats_cache_timer)              \
  TM_NONPIPE_ALLMODULE_COUNTER(start_stats_cache_timer,                     \
                               Start TM Stats Cache Timer,                  \
                               tm_ucli_start_stats_cache_timer)

TM_PERPIPE_ALLMODULE_COUNTERS

#undef TM_PERPIPE_ALLMODULE_COUNTER
#define TM_PERPIPE_ALLMODULE_COUNTER(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#undef TM_NONPIPE_ALLMODULE_COUNTER
#define TM_NONPIPE_ALLMODULE_COUNTER(t, d, f) TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#define TM_PERPIPE_ALLMODULE_COUNTER_HNDLR_LIST TM_PERPIPE_ALLMODULE_COUNTERS

static ucli_command_handler_f traffic_mgr_counter_ucli_ucli_handlers__[] = {
    TM_PERPIPE_ALLMODULE_COUNTER_HNDLR_LIST NULL};

static ucli_module_t traffic_mgr_counter_ucli_module__ = {
    "traffic_mgr_counter_ucli",
    NULL,
    traffic_mgr_counter_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_counter_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_counter_ucli_module__);
  m = ucli_node_create("counter", n, &traffic_mgr_counter_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("counter"));
  return m;
}

/********************* TM Clear Counter CLIs  ************************/

// will be deleted soon
void tm_tof2_display_clr_command_not_supported(bf_dev_id_t devid, int pipe) {
  (void)devid;
  (void)pipe;
  return;
}

#define TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(module, description, func)        \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                                      \
    TM_UCLI_DEV_PIPE_DECL_(void) hex;                                        \
    (void)nz;                                                                \
    UCLI_COMMAND_INFO(                                                       \
        uc, #module, -1, "  " #description "  Usage: -d <devid> -p <pipe>"); \
    TM_UCLI_DEV_PIPE(module)                                                 \
    return func(devid, pipe);                                                \
  }

#define TM_NONPIPE_ALLMODULE_CLEAR_COUNTER(module, description, func) \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                               \
    TM_UCLI_DEV_DECL_(void) hex;                                      \
    (void)nz;                                                         \
    UCLI_COMMAND_INFO(                                                \
        uc, #module, -1, "  " #description "  Usage: -d <devid>");    \
    TM_UCLI_DEV(module)                                               \
    return func(devid);                                               \
  }

#define TM_PERPIPE_ALLMODULE_CLEAR_COUNTERS                                    \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(clr_blocklevel,                           \
                                     Clear block level counters in all blocks, \
                                     tm_ucli_clear_perpipe_counter)            \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(                                          \
      clr_wacperport,                                                          \
      Clear per port drop counter as seen in wac,                              \
      tm_ucli_clear_wac_per_port_drop_counter)                                 \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(                                          \
      clr_wacperppg,                                                           \
      Clear per ppg drop counter as seen in wac,                               \
      tm_ucli_clear_wac_per_ppg_drop_counter)                                  \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(                                          \
      clr_qac_qdrop,                                                           \
      Clear per queue drop counter as seen in qac,                             \
      tm_ucli_clear_qac_q_drop_counter)                                        \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(                                          \
      clr_qac_portdrop,                                                        \
      Clear per port drop counter as seen in qac,                              \
      tm_ucli_clear_qac_port_drop_counter)                                     \
  TM_NONPIPE_ALLMODULE_CLEAR_COUNTER(clr_pre_fifodrop,                         \
                                     Clear all pre fifo drop counter,          \
                                     tm_ucli_clear_nonpipe_counter)

TM_PERPIPE_ALLMODULE_CLEAR_COUNTERS

#undef TM_PERPIPE_ALLMODULE_CLEAR_COUNTER
#define TM_PERPIPE_ALLMODULE_CLEAR_COUNTER(t, d, f) \
  TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#undef TM_NONPIPE_ALLMODULE_CLEAR_COUNTER
#define TM_NONPIPE_ALLMODULE_CLEAR_COUNTER(t, d, f) \
  TRAFFIC_MGR_CLI_CMD_HNDLR(t),

#define TM_PERPIPE_ALLMODULE_CLEAR_COUNTER_HNDLR \
  TM_PERPIPE_ALLMODULE_CLEAR_COUNTERS

static ucli_command_handler_f traffic_mgr_clear_counter_ucli_ucli_handlers__[] =
    {TM_PERPIPE_ALLMODULE_CLEAR_COUNTER_HNDLR NULL};

static ucli_module_t traffic_mgr_clear_counter_ucli_module__ = {
    "traffic_mgr_clear_counter_ucli",
    NULL,
    traffic_mgr_clear_counter_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_clear_counter_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_clear_counter_ucli_module__);
  m = ucli_node_create(
      "clr_counter", n, &traffic_mgr_clear_counter_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("clr_counter"));
  return m;
}

/********************* TM Clear Watermark CLIs  ************************/

#define TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(module, description, func)      \
  TRAFFIC_MGR_CLI_CMD_DECLARE(module) {                                      \
    TM_UCLI_DEV_PIPE_DECL_(void) hex;                                        \
    (void)nz;                                                                \
    UCLI_COMMAND_INFO(                                                       \
        uc, #module, -1, "  " #description "  Usage: -d <devid> -p <pipe>"); \
    TM_UCLI_DEV_PIPE(module)                                                 \
    return func(devid, pipe);                                                \
  }

#define TM_PERPIPE_ALLMODULE_CLEAR_WATERMARKS                             \
  TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(clr_wac_port_wm,                   \
                                       Clear port usage watermark in wac, \
                                       tm_ucli_reset_wac_port_watermark)  \
  TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(clr_ppg_wm,                        \
                                       Clear ppg usage watermark in wac,  \
                                       tm_ucli_reset_wac_ppg_watermark)   \
  TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(clr_qac_port_wm,                   \
                                       Clear port usage watermark in qac, \
                                       tm_ucli_reset_qac_port_wm)         \
  TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(                                   \
      clr_q_wm, Clear queue usage watermark in qac, tm_ucli_reset_qac_q_wm)

TM_PERPIPE_ALLMODULE_CLEAR_WATERMARKS

#undef TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK
#define TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK(t, d, f) \
  TRAFFIC_MGR_CLI_CMD_HNDLR(t),
#define TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK_HNDLR \
  TM_PERPIPE_ALLMODULE_CLEAR_WATERMARKS

static ucli_command_handler_f
    traffic_mgr_clear_watermark_ucli_ucli_handlers__[] = {
        TM_PERPIPE_ALLMODULE_CLEAR_WATERMARK_HNDLR NULL};

static ucli_module_t traffic_mgr_clear_watermark_ucli_module__ = {
    "traffic_mgr_clear_watermark_ucli",
    NULL,
    traffic_mgr_clear_watermark_ucli_ucli_handlers__,
    NULL,
    NULL,
};

static ucli_node_t *tm_clear_watermark_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&traffic_mgr_clear_watermark_ucli_module__);
  m = ucli_node_create(
      "clr_watermark", n, &traffic_mgr_clear_watermark_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("clr_watermark"));
  return m;
}

/* --- Ucli to trigger hitless HA config restore unit Testing --- */
#define TM_UCLI_DEV_DECL_HA_      \
  volatile bf_dev_id_t devid = 0; \
  int c;                          \
  int argc, arg_start = 0;        \
  char *const *argv;              \
  extern char *optarg;            \
  size_t i;

#define TM_UCLI_DEV_HA(table_name)                                            \
  for (i = 0;                                                                 \
       i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]);      \
       i++) {                                                                 \
    if (!strncmp(uc->pargs[0].args__[i], #table_name, strlen(#table_name))) { \
      arg_start = i;                                                          \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
  if (i >= sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0])) {    \
    return UCLI_STATUS_OK;                                                    \
  }                                                                           \
  argc = (uc->pargs->count + 1);                                              \
  argv = (char *const *)&(uc->pargs->args__[arg_start]);                      \
  optind = 0;                                                                 \
  if (argc == 1) {                                                            \
    devid = 0;                                                                \
  } else {                                                                    \
    while (argv && (c = getopt(argc, argv, "d:")) != -1) {                    \
      switch (c) {                                                            \
        case 'd':                                                             \
          devid = strtoul(optarg, NULL, 0);                                   \
          if (devid >= BF_TM_NUM_ASIC) {                                      \
            printf("Incorrect device ID: %d\n", devid);                       \
            printf("Using device ID 0\n");                                    \
            devid = 0;                                                        \
          }                                                                   \
          if (!g_tm_ctx_valid[devid]) {                                       \
            aim_printf(&uc->pvs, "Device ID %d is not registered.\n", devid); \
            return UCLI_STATUS_CONTINUE;                                      \
          }                                                                   \
          break;                                                              \
        default:                                                              \
          printf("%s\n", "Usage: -d <dev_id>");                               \
          printf("%s\n", "Using device ID 0");                                \
          devid = 0;                                                          \
      }                                                                       \
    }                                                                         \
  }

TRAFFIC_MGR_CLI_CMD_DECLARE(ut_hitless_cfg) {
  TM_UCLI_DEV_DECL_HA_
  UCLI_COMMAND_INFO(uc,
                    "ut_hitless_cfg",
                    -1,
                    "Rebuild TM device config reading from asic and verify");
  TM_UCLI_DEV_HA(ut_hitless_cfg)
  bf_tm_ut_restore_device_cfg(uc, devid);
  return UCLI_STATUS_OK;
}

#define TRAFFIC_MGR_CLI_PROLOGUE(CMD, HELP, USAGE)                       \
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

TRAFFIC_MGR_CLI_CMD_DECLARE(bulk_memory_dma) {
  TRAFFIC_MGR_CLI_PROLOGUE("bulk-memory-dma",
                           "Read dma data from file and write to memories",
                           "-f <file>");

  bool got_file = false;

  char *file = NULL;

  int x;
  while (-1 != (x = getopt(argc, argv, "f:"))) {
    switch (x) {
      case 'f':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        file = optarg;
        got_file = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!got_file) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  FILE *stream;
  char *line = NULL;
  size_t len = 0;

  stream = fopen(file, "r");
  if (stream == NULL) {
    perror("fopen");
    return UCLI_STATUS_OK;
  }

  bf_dev_id_t old_dev;
  bf_dev_id_t dev = -1;
  uint64_t addr, hi, lo;
  char *errstr = "Improperly formatted line.";

  while (getline(&line, &len, stream) != -1) {
    char *token;
    token = strtok(line, " ");
    if (token == NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    bool reg = false;
    if (!(strcmp(token, "lld_ind_write") == 0 ||
          strcmp(token, "lld_write_register") == 0))
      continue;

    reg = strcmp(token, "lld_write_register") == 0;

    token = strtok(NULL, " ");
    if (token == NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    old_dev = dev;
    dev = strtoull(token, NULL, 0);
    if (TM_IS_DEV_INVALID(dev)) {
      aim_printf(&uc->pvs, "Invalid Device Id %d\n", dev);
      break;
    }

    if (old_dev != -1 && dev != old_dev) {
      if ((old_dev >= 0) && (old_dev < BF_MAX_DEV_COUNT)) {
        bf_tm_flush_wlist(old_dev);
      }
    }
    token = strtok(NULL, " ");
    if (token == NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    addr = strtoull(token, NULL, 0);

    token = strtok(NULL, " ");
    if (token == NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    hi = strtoull(token, NULL, 0);

    if (reg) {
      aim_printf(&uc->pvs,
                 "Writing value %#" PRIx64 " at register %#" PRIx64 "\n",
                 hi,
                 addr);
      bf_tm_write_register(dev, (uint32_t)addr, (uint32_t)hi);
      continue;
    }

    token = strtok(NULL, " ");
    if (token == NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    lo = strtoull(token, NULL, 0);

    token = strtok(NULL, " ");
    if (token != NULL) {
      aim_printf(&uc->pvs, "%s\n", errstr);
      break;
    }

    aim_printf(&uc->pvs,
               "Writing values hi: %#" PRIx64 " lo: %#" PRIx64
               " at memory %#" PRIx64 "\n",
               hi,
               lo,
               addr);
    bf_tm_write_memory(dev, addr << 4, 16, hi, lo);
  }

  free(line);
  fclose(stream);
  if (!TM_IS_DEV_INVALID(dev)) {
    bf_tm_flush_wlist(dev);
  }
  return UCLI_STATUS_OK;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f traffic_mgr_ucli_ucli_handlers__[] = {
    TRAFFIC_MGR_CLI_CMD_HNDLR(bulk_memory_dma),
    TRAFFIC_MGR_CLI_CMD_HNDLR(ut_hitless_cfg),
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t traffic_mgr_ucli_module__ = {
    "traffic_mgr_ucli",
    NULL,
    traffic_mgr_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *traffic_mgr_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&traffic_mgr_ucli_module__);
  n = ucli_node_create("traffic_mgr", NULL, &traffic_mgr_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("traffic_mgr"));
  tm_cfg_table_node_create(n);
  tm_rsrc_node_create(n);
  tm_usage_node_create(n);
  tm_watermark_node_create(n);
  tm_dropstatus_node_create(n);
  tm_nonpipe_dropstatus_node_create(n);
  tm_pfcstatus_node_create(n);
  tm_counter_node_create(n);
  tm_clear_counter_node_create(n);
  tm_clear_watermark_node_create(n);
  tm_ddr_node_create(n);

#ifdef MIRROR_UCLI_AS_TEMP_WORKAROUND
  mirror_node_create(n);
#endif

  return n;
}

static ucli_command_handler_f bf_drv_show_tech_ucli_tm_handlers__[] = {
    traffic_mgr_ucli_ucli__ppg_wm__,
    traffic_mgr_ucli_ucli__wac_port_wm__,
    traffic_mgr_ucli_ucli__q_wm__,
    traffic_mgr_ucli_ucli__qac_port_wm__,
    traffic_mgr_ucli_ucli__blocklevel__,
    traffic_mgr_ucli_ucli__wac_perport__,
    traffic_mgr_ucli_ucli__wac_perppg__,
    traffic_mgr_ucli_ucli__qac_q_drop__,
    traffic_mgr_ucli_ucli__qac_port_drop__,
    traffic_mgr_ucli_ucli__pre_fifo_drop__,
    traffic_mgr_ucli_ucli__ppg_gmin_usage__,
    traffic_mgr_ucli_ucli__dpg_gmin_usage__,
    traffic_mgr_ucli_ucli__ppg_shrd_usage__,
    traffic_mgr_ucli_ucli__dpg_shrd_usage__,
    traffic_mgr_ucli_ucli__ppg_skid_usage__,
    traffic_mgr_ucli_ucli__wac_portusage__,
    traffic_mgr_ucli_ucli__qac_portusage__,
    traffic_mgr_ucli_ucli__qac_q_usage__};

char *tm_cmd[] = {"ppg_wm",
                  "wac_port_wm",
                  "q_wm",
                  "qac_port_wm",
                  "blocklevel",
                  "wac_perport",
                  "wac_perppg",
                  "qac_q_drop",
                  "qac_port_drop",
                  "pre_fifo_drop",
                  "ppg_gmin_usage",
                  "dpg_gmin_usage",
                  "ppg_shrd_usage",
                  "dpg_shrd_usage",
                  "ppg_skid_usage",
                  "wac_portusage",
                  "qac_portusage",
                  "qac_q_usage"};

ucli_status_t bf_drv_show_tech_ucli_tm__(ucli_context_t *uc) {
  unsigned hdl_iter = 0;
  int status_hdl = UCLI_STATUS_CONTINUE;
  const char *restore_arg[UCLI_CONFIG_MAX_ARGS];
  unsigned hdl_num = sizeof(tm_cmd) / sizeof(tm_cmd[0]);
  aim_printf(&uc->pvs, "-------------------- TM --------------------\n");
  *restore_arg = *uc->pargs[0].args__;
  /*
   * Since one of the module parameter is not required and
   * lower function do not expect this parameter.
   */
  uc->pargs->count--;
  while (hdl_iter < hdl_num) {
    if (bf_drv_show_tech_ucli_tm_handlers__[hdl_iter] && tm_cmd[hdl_iter]) {
      *uc->pargs[0].args__ = tm_cmd[hdl_iter];
      status_hdl = bf_drv_show_tech_ucli_tm_handlers__[hdl_iter](uc);
      if (status_hdl == UCLI_STATUS_OK) {
        aim_printf(&uc->pvs, "Status Handler return = %d   \n", status_hdl);
      }
      hdl_iter++;
    }
  }
  /* Restore to original. */
  *uc->pargs[0].args__ = *restore_arg;
  uc->pargs->count++;
  return 0;
}

#else
void *traffic_mgr_ucli_node_create(void) { return NULL; }
#endif
