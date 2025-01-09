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


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>

#include <dvm/bf_drv_intf.h>
#include <bf_types/bf_types.h>
#include <lld/bf_dma_if.h>
#include <pkt_mgr/bf_pkt.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "pkt_mgr_priv.h"
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

extern pkt_drv_info_t
    *pkt_drv_info[PKT_MGR_NUM_DEVICES][PKT_MGR_NUM_SUBDEVICES];

static int pkt_mgr_pkt_tx_setup_done = 0;

/* Used in pkt_stats_delta handler */
static pkt_drv_info_t drv_info_old;

static bf_status_t pkt_ucli_test_tx_cb(bf_dev_id_t dev_id,
                                       bf_pkt_tx_ring_t tx_ring,
                                       uint64_t tx_cookie,
                                       uint32_t status) {
  if (bf_pkt_free(dev_id, (bf_pkt *)(uintptr_t)tx_cookie) != 0) {
    printf("error freeing pkt in tx notify tx_ring %d pkt_ptr 0x%" PRIx64
           " status %x\n",
           tx_ring,
           tx_cookie,
           status);
  }
  return BF_SUCCESS;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_tx_setup__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int cos;

  UCLI_COMMAND_INFO(uc, "pkt_tx_setup", 2, "pkt_tx_setup <dev_id> <cos>");
  dev = atoi(uc->pargs->args[0]);
  cos = strtoul(uc->pargs->args[1], NULL, 10);
  if (!bf_pkt_is_inited(dev) || cos > 3) {
    aim_printf(&uc->pvs, "Invalid dev or cos\n");
    return 0;
  }
  if (bf_pkt_tx_done_notif_register(dev, pkt_ucli_test_tx_cb, cos) !=
      BF_SUCCESS) {
    aim_printf(&uc->pvs, "tx reg failed for ring %d\n", cos);
    return 0;
  }
  pkt_mgr_pkt_tx_setup_done = 1;
  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_tx_cleanup__(
    ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int cos;

  UCLI_COMMAND_INFO(uc, "pkt_tx_cleanup", 2, "pkt_tx_cleanup <dev_id> <cos>");
  dev = atoi(uc->pargs->args[0]);
  cos = strtoul(uc->pargs->args[1], NULL, 10);
  if (!bf_pkt_is_inited(dev) || cos > 3) {
    aim_printf(&uc->pvs, "Invalid dev or cos\n");
    return 0;
  }
  bf_pkt_tx_done_notif_deregister(dev, cos);
  pkt_mgr_pkt_tx_setup_done = 0;
  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_int_en__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int enable;

  UCLI_COMMAND_INFO(
      uc, "pkt_int_en", 2, "pkt_int_en <dev_id> <1: enable, 0: disable>");
  dev = atoi(uc->pargs->args[0]);
  enable = strtoul(uc->pargs->args[1], NULL, 10);
  if (!bf_pkt_is_inited(dev) || enable > 1) {
    aim_printf(&uc->pvs, "Invalid dev or enable\n");
    return 0;
  }
  if (pkt_mgr_dr_int_en(dev, enable) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "Error configuring interrupt\n");
  }
  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_tx__(ucli_context_t *uc) {
  bf_pkt **test_pkt;
  bf_dev_id_t dev = 0;
  int i, j = -1;
  int batch_sz, batch_cnt, cos, len;
  uint8_t *pkt_data = NULL;

  UCLI_COMMAND_INFO(
      uc,
      "pkt_tx",
      -1,
      "pkt_tx <dev_id> <len> <cos> <pkt_batch_size> <batch_cnt> <hex_string>");

  if (uc->pargs->count < 5 || uc->pargs->count > 6) {
    aim_printf(&uc->pvs, "Invalid argument list\n");
    return 0;
  }

  dev = atoi(uc->pargs->args[0]);
  len = strtoul(uc->pargs->args[1], NULL, 10);
  cos = strtoul(uc->pargs->args[2], NULL, 10);
  batch_sz = strtoul(uc->pargs->args[3], NULL, 10);
  batch_cnt = strtoul(uc->pargs->args[4], NULL, 10);

  if (!bf_pkt_is_inited(dev)) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }
  if (len > BF_PKT_MAX_SIZE || cos > 3) {
    aim_printf(&uc->pvs, "Invalid len or cos\n");
    return 0;
  }
  if (!pkt_mgr_pkt_tx_setup_done) {
    aim_printf(&uc->pvs, "pkt_tx not yet setup\n");
    return 0;
  }

  if (uc->pargs->count == 6) {
    char *hex_string = (char *)uc->pargs->args[5];
    int bytes_read = 0;

    pkt_data = (uint8_t *)bf_sys_calloc(len, sizeof(uint8_t));
    if (!pkt_data) {
      aim_printf(&uc->pvs, "malloc error\n");
      return 0;
    }

    // %2hhx reads two unsigned chars and treats them as a hex number
    while (bytes_read < len &&
           sscanf(hex_string, "%2hhx", &pkt_data[bytes_read]) == 1) {
      ++bytes_read;
      hex_string += 2;
    }

    if (bytes_read <= 0) {
      aim_printf(&uc->pvs,
                 "failed to parse input string: %s\n"
                 "Sending empty packet\n",
                 hex_string);
    }
  }

  /* pre-allocate space to hold  batch_sz number of bf_pkt(s) */
  test_pkt = (bf_pkt **)bf_sys_malloc((sizeof(bf_pkt *)) * batch_sz);
  if (!test_pkt) {
    aim_printf(&uc->pvs, "malloc error \n");
    goto cleanup;
  }
  for (i = 0; i < batch_cnt; i++) {
    for (j = 0; j < batch_sz; j++) {
      if (bf_pkt_alloc(
              dev, &test_pkt[j], len, BF_DMA_CPU_PKT_TRANSMIT_0 + cos) != 0) {
        aim_printf(
            &uc->pvs, "bf_pkt_alloc error in batch_itr %d batch %d\n", j, i);
        /* failed to create j'th packet, so decrese j before cleanup */
        --j;
        goto cleanup;
      }

      if (pkt_data && bf_pkt_data_copy(test_pkt[j], pkt_data, len) != 0) {
        aim_printf(&uc->pvs,
                   "bf_pkt_data_copy error in batch_itr %d batch %d\n",
                   j,
                   i);
        goto cleanup;
      }
    }
    /* transmit pkts */
    for (j = 0; j < batch_sz; j++) {
      if (bf_pkt_tx(dev, test_pkt[j], cos, (void *)test_pkt[j]) != BF_SUCCESS) {
        aim_printf(
            &uc->pvs, "bf_pkt_tx error in batch_itr %d batch %d\n", j, i);
        goto cleanup;
      }
    }
    bf_sys_usleep(1000);
  }

  /* Invalidate j to avoid double free */
  j = -1;

cleanup:
  aim_printf(&uc->pvs, "pkt_tx over\n");
  if (pkt_data) {
    bf_sys_free(pkt_data);
  }
  /* free up unsent bf_pkts */
  while (j >= 0) {
    bf_pkt_free(dev, test_pkt[j]);
    j--;
  }
  if (test_pkt) {
    bf_sys_free(test_pkt);
  }
  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_stat__clear__(
    ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_subdev_id_t subdev = 0;
  pkt_drv_info_t *drv_info;

  UCLI_COMMAND_INFO(uc, "pkt_stats_clear", 2, "pkt_stats_clear <dev> <subdev>");
  dev = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  if ((dev >= PKT_MGR_NUM_DEVICES) || (subdev >= PKT_MGR_NUM_SUBDEVICES)) {
    aim_printf(&uc->pvs, "Invalid dev/subdev\n");
    return 0;
  }

  if (!bf_pkt_is_inited(dev)) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  drv_info = pkt_drv_info[dev][subdev];

  if (!drv_info) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  memset(&drv_info->alloc_stat, 0, sizeof(drv_info->alloc_stat));
  memset(&drv_info->rx_stat, 0, sizeof(drv_info->rx_stat));
  memset(&drv_info->tx_stat, 0, sizeof(drv_info->tx_stat));

  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_stat__delta__(
    ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_subdev_id_t subdev = 0;
  pkt_drv_info_t *drv_info;
  int zero_print = 0;
  uint64_t delta;
  int i;

  UCLI_COMMAND_INFO(uc,
                    "pkt_stats_delta",
                    3,
                    "pkt_stats_delta <dev> <subdev> <zero_print 0:1>");
  dev = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  zero_print = atoi(uc->pargs->args[2]);

  if ((dev >= PKT_MGR_NUM_DEVICES) || (subdev >= PKT_MGR_NUM_SUBDEVICES)) {
    aim_printf(&uc->pvs, "Invalid dev/subdev\n");
    return 0;
  }

  if (!bf_pkt_is_inited(dev)) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  drv_info = pkt_drv_info[dev][subdev];

  if (!drv_info) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  aim_printf(&uc->pvs, "packet heap counters delta\n");
  delta =
      drv_info->alloc_stat.pkt_alloc_ok - drv_info_old.alloc_stat.pkt_alloc_ok;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "pkt_alloc:\t\t%" PRIu64 "\n", delta);
  delta =
      drv_info->alloc_stat.pkt_free_ok - drv_info_old.alloc_stat.pkt_free_ok;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "pkt_free:\t\t%" PRIu64 "\n", delta);
  delta =
      drv_info->alloc_stat.buf_alloc_ok - drv_info_old.alloc_stat.buf_alloc_ok;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "buf_alloc:\t\t%" PRIu64 "\n", delta);
  delta =
      drv_info->alloc_stat.buf_free_ok - drv_info_old.alloc_stat.buf_free_ok;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "buf_free:\t\t%" PRIu64 "\n", delta);
  delta = drv_info->alloc_stat.pkt_alloc_err -
          drv_info_old.alloc_stat.pkt_alloc_err;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "pkt_alloc_err:\t\t%" PRIu64 "\n", delta);
  delta =
      drv_info->alloc_stat.pkt_free_err - drv_info_old.alloc_stat.pkt_free_err;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "pkt_free_err:\t\t%" PRIu64 "\n", delta);
  delta = drv_info->alloc_stat.buf_alloc_err -
          drv_info_old.alloc_stat.buf_alloc_err;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "buf_alloc_err:\t\t%" PRIu64 "\n", delta);
  delta =
      drv_info->alloc_stat.buf_free_err - drv_info_old.alloc_stat.buf_free_err;
  if (zero_print || delta)
    aim_printf(&uc->pvs, "buf_free_err:\t\t%" PRIu64 "\n", delta);

  aim_printf(&uc->pvs, "packet Rx counters delta\n");
  for (i = 0; i < 8; i++) {
    aim_printf(&uc->pvs, "packet Rx Ring %d counters delta:\n", i);
    delta = drv_info->rx_stat[i].pkt_ok - drv_info_old.rx_stat[i].pkt_ok;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_ok:\t\t\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_addr_err -
            drv_info_old.rx_stat[i].pkt_addr_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_addr_err:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_alloc_err -
            drv_info_old.rx_stat[i].pkt_alloc_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_alloc_err:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_free_err -
            drv_info_old.rx_stat[i].pkt_free_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_free_err:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_param_err -
            drv_info_old.rx_stat[i].pkt_param_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_param_err:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_assembly_err -
            drv_info_old.rx_stat[i].pkt_assembly_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_assembly_err:\t%" PRIu64 "\n", delta);
    delta = drv_info->rx_stat[i].pkt_no_hndl_err -
            drv_info_old.rx_stat[i].pkt_no_hndl_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_no_hndl_err:\t%" PRIu64 "\n", delta);
  }

  aim_printf(&uc->pvs, "packet Tx counters delta\n");
  for (i = 0; i < 4; i++) {
    aim_printf(&uc->pvs, "packet Tx Ring %d counters delta:\n", i);
    delta = drv_info->tx_stat[i].pkt_ok - drv_info_old.tx_stat[i].pkt_ok;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_ok:\t\t\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_compl_ok -
            drv_info_old.tx_stat[i].pkt_compl_ok;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_compl_ok:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_drop - drv_info_old.tx_stat[i].pkt_drop;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_drop:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_param_err -
            drv_info_old.tx_stat[i].pkt_param_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_param_err:\t\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_compl_type_err -
            drv_info_old.tx_stat[i].pkt_compl_type_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_comp_type_err:\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_compl_assembly_err -
            drv_info_old.tx_stat[i].pkt_compl_assembly_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_assembly_err:\t%" PRIu64 "\n", delta);
    delta = drv_info->tx_stat[i].pkt_no_hndl_err -
            drv_info_old.tx_stat[i].pkt_no_hndl_err;
    if (zero_print || delta)
      aim_printf(&uc->pvs, "pkt_no_hndl_err:\t%" PRIu64 "\n", delta);
  }
  return 0;
}

static ucli_status_t pkt_mgr_ucli_ucli__bf_pkt_stat__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_subdev_id_t subdev = 0;
  int i;
  pkt_drv_info_t *drv_info;

  UCLI_COMMAND_INFO(uc, "pkt_stats", 2, "pkt_stats <dev> <subdev>");
  dev = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  if ((dev >= PKT_MGR_NUM_DEVICES) || (subdev >= PKT_MGR_NUM_SUBDEVICES)) {
    aim_printf(&uc->pvs, "Invalid dev/subdev\n");
    return 0;
  }

  if (!bf_pkt_is_inited(dev)) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  drv_info = pkt_drv_info[dev][subdev];

  if (!drv_info) {
    aim_printf(&uc->pvs, "Invalid dev\n");
    return 0;
  }

  /* Keep current stats in global copy */
  memcpy(&drv_info_old, drv_info, sizeof(drv_info_old));

  aim_printf(&uc->pvs, "packet heap counters\n");
  aim_printf(&uc->pvs,
             "pkt_alloc:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.pkt_alloc_ok);
  aim_printf(
      &uc->pvs, "pkt_free:\t\t%" PRIu64 "\n", drv_info->alloc_stat.pkt_free_ok);
  aim_printf(&uc->pvs,
             "buf_alloc:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.buf_alloc_ok);
  aim_printf(
      &uc->pvs, "buf_free:\t\t%" PRIu64 "\n", drv_info->alloc_stat.buf_free_ok);
  aim_printf(&uc->pvs,
             "pkt_alloc_err:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.pkt_alloc_err);
  aim_printf(&uc->pvs,
             "pkt_free_err:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.pkt_free_err);
  aim_printf(&uc->pvs,
             "buf_alloc_err:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.buf_alloc_err);
  aim_printf(&uc->pvs,
             "buf_free_err:\t\t%" PRIu64 "\n",
             drv_info->alloc_stat.buf_free_err);
  aim_printf(&uc->pvs, "packet Rx counters\n");
  for (i = 0; i < 8; i++) {
    aim_printf(&uc->pvs, "packet Rx Ring %d counters:\n", i);
    aim_printf(
        &uc->pvs, "pkt_ok:\t\t\t%" PRIu64 "\n", drv_info->rx_stat[i].pkt_ok);
    aim_printf(&uc->pvs,
               "pkt_addr_err:\t\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_addr_err);
    aim_printf(&uc->pvs,
               "pkt_alloc_err:\t\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_alloc_err);
    aim_printf(&uc->pvs,
               "pkt_free_err:\t\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_free_err);
    aim_printf(&uc->pvs,
               "pkt_param_err:\t\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_param_err);
    aim_printf(&uc->pvs,
               "pkt_assembly_err:\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_assembly_err);
    aim_printf(&uc->pvs,
               "pkt_no_hndl_err:\t%" PRIu64 "\n",
               drv_info->rx_stat[i].pkt_no_hndl_err);
  }
  aim_printf(&uc->pvs, "packet Tx counters\n");
  for (i = 0; i < 4; i++) {
    aim_printf(&uc->pvs, "packet Tx Ring %d counters:\n", i);
    aim_printf(
        &uc->pvs, "pkt_ok:\t\t\t%" PRIu64 "\n", drv_info->tx_stat[i].pkt_ok);
    aim_printf(&uc->pvs,
               "pkt_compl_ok:\t\t%" PRIu64 "\n",
               drv_info->tx_stat[i].pkt_compl_ok);
    aim_printf(
        &uc->pvs, "pkt_drop:\t\t%" PRIu64 "\n", drv_info->tx_stat[i].pkt_drop);
    aim_printf(&uc->pvs,
               "pkt_param_err:\t\t%" PRIu64 "\n",
               drv_info->tx_stat[i].pkt_param_err);
    aim_printf(&uc->pvs,
               "pkt_comp_type_err:\t%" PRIu64 "\n",
               drv_info->tx_stat[i].pkt_compl_type_err);
    aim_printf(&uc->pvs,
               "pkt_assembly_err:\t%" PRIu64 "\n",
               drv_info->tx_stat[i].pkt_compl_assembly_err);
    aim_printf(&uc->pvs,
               "pkt_no_hndl_err:\t%" PRIu64 "\n",
               drv_info->tx_stat[i].pkt_no_hndl_err);
  }
  return 0;
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */
static ucli_command_handler_f pkt_mgr_ucli_ucli_handlers__[] = {
    pkt_mgr_ucli_ucli__bf_pkt_stat__,
    pkt_mgr_ucli_ucli__bf_pkt_stat__clear__,
    pkt_mgr_ucli_ucli__bf_pkt_stat__delta__,
    pkt_mgr_ucli_ucli__bf_pkt_tx_setup__,
    pkt_mgr_ucli_ucli__bf_pkt_tx_cleanup__,
    pkt_mgr_ucli_ucli__bf_pkt_tx__,
    pkt_mgr_ucli_ucli__bf_pkt_int_en__,
    NULL};

static ucli_module_t pkt_mgr_ucli_module__ = {
    "pkt_mgr_ucli",
    NULL,
    pkt_mgr_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pkt_mgr_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&pkt_mgr_ucli_module__);
  n = ucli_node_create("pkt_mgr", NULL, &pkt_mgr_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("pkt_mgr"));
  return n;
}
