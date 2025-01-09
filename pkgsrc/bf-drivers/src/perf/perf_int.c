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


#include <errno.h>
#include <time.h>
#include <math.h>

#include <target-utils/uCli/ucli.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <pipe_mgr/pipe_mgr_drv.h>
#include <tofino_regs/tofino.h>

#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <lld/tof2_reg_drv_defs.h>
#include <lld/tofino_defs.h>

#include "perf_util.h"
#include <perf/perf_common_intf.h>
#include <perf/perf_int_intf.h>
#include "perf_int.h"

static perf_int_cache_t perf_int_cache[PERF_INT_CACHE_MAX];
int int_cache_idx = 0;

/**
 * @brief Return whether interrupts are support for a given bus type
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @return bool
 */
bool is_bus_ints_supported(bf_dev_id_t dev_id, perf_bus_t_enum bus_type) {
  bool is_tofino = lld_dev_is_tofino(dev_id);
  if (!is_tofino && (bus_type == MBUS || bus_type == HOSTIF)) {
    return false;
  }
  return true;
}

/**
 * @brief Handle interrupt test callback
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @param intr_address intr address
 * @param intr_status_val intr status value
 * @param enable_hi_addr enable hi address
 * @param enable_lo_addr enable lo address
 * @param userdata user data
 * @return uint32_t
 */
static uint32_t perf_int_test_callback(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       uint32_t intr_address,
                                       uint32_t intr_status_val,
                                       uint32_t enable_hi_addr,
                                       uint32_t enable_lo_addr,
                                       void *userdata) {
  perf_int_cache_t *data = (perf_int_cache_t *)userdata;

  (void)enable_hi_addr;
  (void)enable_lo_addr;
  (void)subdev_id;

  if (!data) {
    printf("Received callback with NULL callback data\n");
    return -1;
  }
  if (!data->valid) {
    printf("Received callback with invalid callback data\n");
    return -1;
  }
  if (intr_status_val == 0) {
    printf("Received callback with invalid status of zero\n");
    return -1;
  }

  /* Clear the interrupt */
  lld_write_register(dev_id, intr_address, intr_status_val);

  if ((intr_status_val & data->exp_int_status_val) !=
      data->exp_int_status_val) {
    printf("All interrupts did not fire (msi)\n");
  }

  clock_gettime(CLOCK_MONOTONIC, &(data->stop));
  data->callback_received = true;

  return 0;
}

/**
 * @brief Save current interrupts settings
 *
 * @param dev_id device id
 * @param status_addr status address
 * @param status_val status value
 * @param enable_addr enable address
 * @param inject_addr inject address
 * @param cache_idx cache index
 */
static void perf_int_save_curr_settings(bf_dev_id_t dev_id,
                                        uint32_t status_addr,
                                        uint32_t status_val,
                                        uint32_t enable_addr,
                                        uint32_t inject_addr,
                                        int cache_idx) {
  void *userdata = NULL;

  /* Out of callback data, poll interrupt */
  if (cache_idx >= PERF_INT_CACHE_MAX) {
    return;
  }

  perf_int_cache[cache_idx].valid = true;
  perf_int_cache[cache_idx].status_addr = status_addr;
  perf_int_cache[cache_idx].exp_int_status_val = status_val;
  perf_int_cache[cache_idx].enable_addr = enable_addr;
  perf_int_cache[cache_idx].inject_addr = inject_addr;
  lld_read_register(
      dev_id, status_addr, &(perf_int_cache[cache_idx].status_val));
  lld_read_register(
      dev_id, enable_addr, &(perf_int_cache[cache_idx].enable_val));

  /* Disable interrupt */
  lld_write_register(dev_id, enable_addr, 0);

  /* Clear the interrupt status */
  lld_write_register(dev_id, status_addr, 0xffffffff);

  /* Get existing callback function */
  perf_int_cache[cache_idx].cb_fn =
      lld_get_int_cb(dev_id, 0, status_addr, &userdata);
  perf_int_cache[cache_idx].userdata = userdata;

  /* Register new callback */
  lld_int_register_cb(dev_id,
                      0,
                      status_addr,
                      perf_int_test_callback,
                      &perf_int_cache[cache_idx]);
}

/**
 * @brief Restore interrupts settings
 *
 * @param dev_id device id
 */
static void perf_int_restore_settings(bf_dev_id_t dev_id) {
  int_cache_idx = 0;
  for (int idx = 0; idx < PERF_INT_CACHE_MAX; idx++) {
    if (!perf_int_cache[idx].valid) {
      continue;
    }
    perf_int_cache[idx].valid = false;
    /* If callback data contains valid info, restore it */
    lld_int_register_cb(dev_id,
                        0,
                        perf_int_cache[idx].status_addr,
                        perf_int_cache[idx].cb_fn,
                        perf_int_cache[idx].userdata);
    /* Clear status register */
    lld_write_register(dev_id, perf_int_cache[idx].status_addr, 0xffffffff);

    /* Restore status bits */
    lld_write_register(dev_id,
                       perf_int_cache[idx].status_addr,
                       perf_int_cache[idx].status_val);

    /* Restore enable bits */
    lld_write_register(dev_id,
                       perf_int_cache[idx].enable_addr,
                       perf_int_cache[idx].enable_val);

    memset(&perf_int_cache[idx], 0, sizeof(perf_int_cache_t));
  }
}

/**
 * @brief Test pbus interrupt processing latency
 *
 * @param dev_id device id
 * @param it number of iterations
 * @param interrupts number of interrupts received
 * @param double avg latency in us
 * @param double sd latency in us
 * @param double min latency in us
 * @param double max latency in us
 * @return bf_status_t
 */
static bf_status_t perf_int_pbus_test(bf_dev_id_t dev_id,
                                      int it,
                                      int *interrupts,
                                      int *iterations,
                                      double *avg_latency_us,
                                      double *sd_latency_us,
                                      double *min_latency_us,
                                      double *max_latency_us) {
  uint32_t sram_data = 0, tcam_data = 0;
  uint32_t status_addr = 0, enable_addr = 0, inject_addr = 0;
  uint32_t stage = 0, num_stages = 0, num_pipes = 0, pipe_bmp = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;

  double results[PERF_INT_CACHE_MAX] = {0};
  double partial_avg = 0;
  double partial_sd = 0;
  *avg_latency_us = 0;
  *min_latency_us = __DBL_MAX__;
  *max_latency_us = 0;
  *sd_latency_us = 0;
  *interrupts = 0;
  *iterations = 0;

  int iter_cnt = 0;

  memset(perf_int_cache, 0, sizeof(perf_int_cache));

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_ARG;
  }

  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);

  uint8_t num_sram_rows, num_sram_cols, num_tcam_rows;
  num_sram_rows = dev_info->dev_cfg.stage_cfg.num_sram_rows;
  num_sram_cols = dev_info->dev_cfg.stage_cfg.num_sram_cols;
  num_tcam_rows = dev_info->dev_cfg.stage_cfg.num_tcam_rows;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (uint32_t i = 0; i < num_pipes; i++) {
    pipe_bmp |= (0x1u << i);
  }

  /* Test only sram and tcam ecc interrupts */
  /* 12 bits for sbe errors, 12 bits for mbe errors */
  for (int idx = 0; idx < (2 * num_sram_cols); idx++) {
    sram_data |= 0x1 << idx;
  }
  /* bits 4-15 for single bit error */
  for (int idx = 0; idx < num_tcam_rows; idx++) {
    tcam_data |= 0x1 << (4 + idx);
  }

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  if (it > PERF_INT_CACHE_MAX) {
    iter_cnt = PERF_INT_CACHE_MAX;
  } else {
    iter_cnt = it;
  }

  /* Test pbus interrupts */
  for (int i = 0; i < iter_cnt; i++) {
    int cache_idx = 0;
    for (pipe = 0; pipe < num_pipes; pipe++) {
      if (!((1 << pipe) & pipe_bmp)) {
        continue;
      }
      lld_sku_map_pipe_id_to_phy_pipe_id(dev_id, pipe, &phy_pipe);
      lld_sku_get_num_active_mau_stages(dev_id, &num_stages, phy_pipe);

      for (stage = 0; stage < num_stages; stage++) {
        /* TCAM */
        if (is_tofino) {
          status_addr = offsetof(
              Tofino,
              pipes[phy_pipe].mau[stage].tcams.intr_status_mau_tcam_array);
          enable_addr = offsetof(
              Tofino,
              pipes[phy_pipe].mau[stage].tcams.intr_enable0_mau_tcam_array);
          inject_addr = offsetof(
              Tofino,
              pipes[phy_pipe].mau[stage].tcams.intr_inject_mau_tcam_array);

        } else if (is_tofino2) {
          status_addr = offsetof(
              tof2_reg,
              pipes[phy_pipe].mau[stage].tcams.intr_status_mau_tcam_array);
          enable_addr = offsetof(
              tof2_reg,
              pipes[phy_pipe].mau[stage].tcams.intr_enable0_mau_tcam_array);
          inject_addr = offsetof(
              tof2_reg,
              pipes[phy_pipe].mau[stage].tcams.intr_inject_mau_tcam_array);

        } else {
          LOG_ERROR("%s:%d: Device type not supported %d\n",
                    __func__,
                    __LINE__,
                    dev_info->dev_family);
          bf_sys_dbgchk(0);
          return BF_INVALID_ARG;
        }

        /* Save current settings */
        perf_int_save_curr_settings(dev_id,
                                    status_addr,
                                    tcam_data,
                                    enable_addr,
                                    inject_addr,
                                    cache_idx++);

        for (int row = 0; row < num_sram_rows; row++) {
          /* SRAM */
          if (is_tofino) {
            status_addr = offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_status_mau_unit_ram_row);
            enable_addr = offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_enable0_mau_unit_ram_row);
            inject_addr = offsetof(Tofino,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_inject_mau_unit_ram_row);

          } else if (is_tofino2) {
            status_addr = offsetof(tof2_reg,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_status_mau_unit_ram_row);
            enable_addr = offsetof(tof2_reg,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_enable0_mau_unit_ram_row);
            inject_addr = offsetof(tof2_reg,
                                   pipes[phy_pipe]
                                       .mau[stage]
                                       .rams.array.row[row]
                                       .intr_inject_mau_unit_ram_row);

          } else {
            LOG_ERROR("%s:%d: Device type not supported %d\n",
                      __func__,
                      __LINE__,
                      dev_info->dev_family);
            bf_sys_dbgchk(0);
            return BF_INVALID_ARG;
          }
          /* Save current settings */
          perf_int_save_curr_settings(dev_id,
                                      status_addr,
                                      sram_data,
                                      enable_addr,
                                      inject_addr,
                                      cache_idx++);
        }
      }
    }

    for (int iter = 0; iter < cache_idx; iter++) {
      /* Inject the interrupt */
      lld_write_register(dev_id,
                         perf_int_cache[iter].inject_addr,
                         perf_int_cache[iter].exp_int_status_val);

      clock_gettime(CLOCK_MONOTONIC, &perf_int_cache[iter].start);

      /* Enable the interrupt */
      lld_write_register(dev_id,
                         perf_int_cache[iter].enable_addr,
                         perf_int_cache[iter].exp_int_status_val);

      /* Sleep for 1sec at most (500us granurality) to allow interrupt
        callback to be processed.
        First sleep will be enough to cover most of the cases
      */
      int cnt = 2000;
      do {
        bf_sys_usleep(500);
      } while (!perf_int_cache[iter].callback_received && cnt--);

      if (!perf_int_cache[iter].callback_received) {
        LOG_ERROR(
            "%s:%d: Interrupt callback was not received (cache index %d)\n",
            __func__,
            __LINE__,
            iter);
        perf_int_restore_settings(dev_id);
        bf_sys_dbgchk(0);
        return BF_INVALID_ARG;
      }
      results[iter] =
          time_delta_ns(perf_int_cache[iter].start, perf_int_cache[iter].stop);
      /* Provide results in us instead of ns*/
      results[iter] /= 1000;
    }

    /* Calculate minimum and maximum for the specific iteration */
    double min = results[0], max = results[0];
    for (cache_idx = 0; cache_idx < PERF_INT_CACHE_MAX; cache_idx++) {
      if (!perf_int_cache[cache_idx].valid) {
        break;
      }
      min = min > results[cache_idx] ? results[cache_idx] : min;
      max = max < results[cache_idx] ? results[cache_idx] : max;
    }

    /* Return a number of interrupts */
    *interrupts = cache_idx * it;

    /* Calculate average and standard deviation for the specific iteration */
    if (cache_idx > 0 && cache_idx < PERF_INT_CACHE_MAX) {
      basic_stats(results, cache_idx, &partial_avg, &partial_sd);
    }

    /* Calculate final results */
    if (min < *min_latency_us) {
      *min_latency_us = min;
    }
    if (max > *max_latency_us) {
      *max_latency_us = max;
    }
    *avg_latency_us += partial_avg / it;
    *sd_latency_us = sqrt((*sd_latency_us) * (*sd_latency_us) +
                          (partial_sd * partial_sd / it));

    /* Restore callbacks and enable status */
    perf_int_restore_settings(dev_id);
  }

  *iterations = iter_cnt;

  return BF_SUCCESS;
}

/**
 * @brief Test mbus interrupt processing latency
 *
 * @param dev_id device id
 * @param it number of iterations
 * @param interrupts number of interrupts received
 * @param double avg latency in us
 * @param double sd latency in us
 * @param double min latency in us
 * @param double max latency in us
 * @return bf_status_t
 */
static bf_status_t perf_int_mbus_test(bf_dev_id_t dev_id,
                                      int it,
                                      int *interrupts,
                                      int *iterations,
                                      double *avg_latency_us,
                                      double *sd_latency_us,
                                      double *min_latency_us,
                                      double *max_latency_us) {
  int iter_cnt = 0;
  int cache_idx = 0;
  uint32_t status_data = 0;
  uint32_t status_addr = 0, enable_addr = 0, inject_addr = 0;
  double results[PERF_INT_CACHE_MAX] = {0};

  *avg_latency_us = 0;
  *min_latency_us = 0;
  *max_latency_us = 0;
  *sd_latency_us = 0;
  *interrupts = 0;
  *iterations = 0;

  memset(perf_int_cache, 0, sizeof(perf_int_cache));

  if (it > PERF_INT_CACHE_MAX) {
    iter_cnt = PERF_INT_CACHE_MAX;
  } else {
    iter_cnt = it;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_ARG;
  }

  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);

  /* Test mbus (port-mgr) interrupt */
  if (is_tofino) {
    status_addr = DEF_tofino_device_select_mbc_mbc_mbus_int_stat_address;
    status_data = 0x200;
    enable_addr = DEF_tofino_device_select_mbc_mbc_mbus_int_en_0_address;
    inject_addr = DEF_tofino_device_select_mbc_mbc_mbus_int_inj_address;

  } else if (is_tofino2) {
    status_addr = DEF_tof2_reg_device_select_mbc_mbc_mbus_intr_stat_address;
    status_data = 0x200;
    enable_addr = DEF_tof2_reg_device_select_mbc_mbc_mbus_intr_en_0_address;
    inject_addr = DEF_tof2_reg_device_select_mbc_mbc_mbus_intr_inj_address;

  } else {
    LOG_ERROR("%s:%d: Device type not supported %d\n",
              __func__,
              __LINE__,
              dev_info->dev_family);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }

  for (cache_idx = 0; cache_idx < iter_cnt; cache_idx++) {
    /* Save current settings */
    perf_int_save_curr_settings(
        dev_id, status_addr, status_data, enable_addr, inject_addr, cache_idx);

    /* Inject the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].inject_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);

    clock_gettime(CLOCK_MONOTONIC, &perf_int_cache[cache_idx].start);

    /* Enable the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].enable_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);

    /* Sleep for 1sec at most (500us granurality) to allow interrupt
       callback to be processed.
       First sleep will be enough to cover most of the cases
    */
    int cnt = 2000;
    do {
      bf_sys_usleep(500);
    } while (!perf_int_cache[cache_idx].callback_received && cnt--);

    if (!perf_int_cache[cache_idx].callback_received) {
      LOG_ERROR("%s:%d: Interrupt callback was not received (cache index %d)\n",
                __func__,
                __LINE__,
                cache_idx);
      perf_int_restore_settings(dev_id);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
    }

    results[cache_idx] = time_delta_ns(perf_int_cache[cache_idx].start,
                                       perf_int_cache[cache_idx].stop);
    /* Provide results in us instead of ns*/
    results[cache_idx] /= 1000;
  }

  /* Calculate minimum and maximum */
  double min = results[0], max = results[0];
  for (cache_idx = 0; cache_idx < it; cache_idx++) {
    if (!perf_int_cache[cache_idx].valid) {
      break;
    }
    min = min > results[cache_idx] ? results[cache_idx] : min;
    max = max < results[cache_idx] ? results[cache_idx] : max;
  }
  *min_latency_us = min;
  *max_latency_us = max;

  /* Return a number of interrupts */
  *interrupts = cache_idx;
  *iterations = iter_cnt;

  /* Calculate average and standard deviation */
  if (cache_idx > 0 && cache_idx < PERF_INT_CACHE_MAX) {
    basic_stats(results, cache_idx, avg_latency_us, sd_latency_us);
  }

  /* Restore callbacks and enable status */
  perf_int_restore_settings(dev_id);

  return BF_SUCCESS;
}

/**
 * @brief Test cbus interrupt processing latency
 *
 * @param dev_id device id
 * @param it number of iterations
 * @param interrupts number of interrupts received
 * @param double avg latency in us
 * @param double sd latency in us
 * @param double min latency in us
 * @param double max latency in us
 * @return bf_status_t
 */
static bf_status_t perf_int_cbus_test(bf_dev_id_t dev_id,
                                      int it,
                                      int *interrupts,
                                      int *iterations,
                                      double *avg_latency_us,
                                      double *sd_latency_us,
                                      double *min_latency_us,
                                      double *max_latency_us) {
  int iter_cnt = 0;
  int cache_idx = 0;
  uint32_t status_data = 0;
  uint32_t status_addr = 0, enable_addr = 0, inject_addr = 0;
  double results[PERF_INT_CACHE_MAX] = {0};

  *avg_latency_us = 0;
  *min_latency_us = 0;
  *max_latency_us = 0;
  *sd_latency_us = 0;
  *interrupts = 0;
  *iterations = 0;

  memset(perf_int_cache, 0, sizeof(perf_int_cache));

  if (it > PERF_INT_CACHE_MAX) {
    iter_cnt = PERF_INT_CACHE_MAX;
  } else {
    iter_cnt = it;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_ARG;
  }

  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);

  /* Test CBUS (TM) interrupt */
  if (is_tofino) {
    status_addr =
        DEF_tofino_device_select_tm_top_tm_prc_top_prc_intr_status_address;
    status_data = 0x1;
    enable_addr =
        DEF_tofino_device_select_tm_top_tm_prc_top_prc_intr_enable0_address;
    inject_addr =
        DEF_tofino_device_select_tm_top_tm_prc_top_prc_intr_inject_address;

  } else if (is_tofino2) {
    status_addr =
        DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_intr_stat_address;
    status_data = 0x1;
    enable_addr =
        DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_intr_en0_address;
    inject_addr =
        DEF_tof2_reg_device_select_tm_top_tm_prc_top_prc_intr_inj_address;

  } else {
    LOG_ERROR("%s:%d: Device type not supported %d\n",
              __func__,
              __LINE__,
              dev_info->dev_family);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }

  for (cache_idx = 0; cache_idx < iter_cnt; cache_idx++) {
    /* Save current settings */
    perf_int_save_curr_settings(
        dev_id, status_addr, status_data, enable_addr, inject_addr, cache_idx);

    /* Inject the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].inject_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);

    clock_gettime(CLOCK_MONOTONIC, &perf_int_cache[cache_idx].start);

    /* Enable the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].enable_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);
    /* Sleep for 1sec at most (500us granurality) to allow interrupt
       callback to be processed.
       First sleep will be enough to cover most of the cases
    */
    int cnt = 2000;
    do {
      bf_sys_usleep(500);
    } while (!perf_int_cache[cache_idx].callback_received && cnt--);

    if (!perf_int_cache[cache_idx].callback_received) {
      LOG_ERROR("%s:%d: Interrupt callback was not received (cache index %d)\n",
                __func__,
                __LINE__,
                cache_idx);
      perf_int_restore_settings(dev_id);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
    }

    results[cache_idx] = time_delta_ns(perf_int_cache[cache_idx].start,
                                       perf_int_cache[cache_idx].stop);

    /* Provide results in us instead of ns*/
    results[cache_idx] /= 1000;
  }

  /* Calculate minimum and maximum */
  double min = results[0], max = results[0];
  for (cache_idx = 0; cache_idx < it; cache_idx++) {
    if (!perf_int_cache[cache_idx].valid) {
      break;
    }
    min = min > results[cache_idx] ? results[cache_idx] : min;
    max = max < results[cache_idx] ? results[cache_idx] : max;
  }
  *min_latency_us = min;
  *max_latency_us = max;

  /* Return a number of interrupts */
  *interrupts = cache_idx;
  *iterations = iter_cnt;

  /* Calculate average and standard deviation */
  if (cache_idx > 0 && cache_idx < PERF_INT_CACHE_MAX) {
    basic_stats(results, cache_idx, avg_latency_us, sd_latency_us);
  }

  /* Restore callbacks and enable status */
  perf_int_restore_settings(dev_id);

  return BF_SUCCESS;
}

/**
 * @brief Test hostif interrupt processing latency
 *
 * @param dev_id device id
 * @param it number of iterations
 * @param interrupts number of interrupts received
 * @param double avg latency in us
 * @param double sd latency in us
 * @param double min latency in us
 * @param double max latency in us
 * @return bf_status_t
 */
static bf_status_t perf_int_hostif_test(bf_dev_id_t dev_id,
                                        int it,
                                        int *interrupts,
                                        int *iterations,
                                        double *avg_latency_us,
                                        double *sd_latency_us,
                                        double *min_latency_us,
                                        double *max_latency_us) {
  int iter_cnt = 0;
  int cache_idx = 0;
  uint32_t status_data = 0;
  uint32_t status_addr = 0, enable_addr = 0, inject_addr = 0;
  double results[PERF_INT_CACHE_MAX] = {0};

  *avg_latency_us = 0;
  *min_latency_us = 0;
  *max_latency_us = 0;
  *sd_latency_us = 0;
  *interrupts = 0;

  memset(perf_int_cache, 0, sizeof(perf_int_cache));

  if (it > PERF_INT_CACHE_MAX) {
    iter_cnt = PERF_INT_CACHE_MAX;
  } else {
    iter_cnt = it;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_ARG;
  }

  bool is_tofino = lld_dev_is_tofino(dev_id);
  bool is_tofino2 = lld_dev_is_tof2(dev_id);

  /* Test hostif (pcie) interrupt */
  if (is_tofino) {
    status_addr =
        DEF_tofino_device_select_pcie_bar01_regs_pcie_regs_pcie_int_stat_address,
    status_data = 0x1000;
    enable_addr =
        DEF_tofino_device_select_pcie_bar01_regs_pcie_regs_pcie_int_en_address;
    inject_addr =
        DEF_tofino_device_select_pcie_bar01_regs_pcie_regs_pcie_int_inj_address;

  } else if (is_tofino2) {
    status_addr =
        DEF_tof2_reg_device_select_pcie_bar01_regs_pcie_intr_stat_address,
    status_data = 0x1000;
    enable_addr =
        DEF_tof2_reg_device_select_pcie_bar01_regs_pcie_intr_en0_address;
    inject_addr =
        DEF_tof2_reg_device_select_pcie_bar01_regs_pcie_intr_inj_address;

  } else {
    LOG_ERROR("%s:%d: Device type not supported %d\n",
              __func__,
              __LINE__,
              dev_info->dev_family);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }

  for (cache_idx = 0; cache_idx < iter_cnt; cache_idx++) {
    /* Save current settings */
    perf_int_save_curr_settings(
        dev_id, status_addr, status_data, enable_addr, inject_addr, cache_idx);

    /* Inject the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].inject_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);

    clock_gettime(CLOCK_MONOTONIC, &perf_int_cache[cache_idx].start);

    /* Enable the interrupt */
    lld_write_register(dev_id,
                       perf_int_cache[cache_idx].enable_addr,
                       perf_int_cache[cache_idx].exp_int_status_val);

    /* Sleep for 1sec at most (500us granurality) to allow interrupt
       callback to be processed.
       First sleep will be enough to cover most of the cases
    */
    int cnt = 2000;
    do {
      bf_sys_usleep(500);
    } while (!perf_int_cache[cache_idx].callback_received && cnt--);

    if (!perf_int_cache[cache_idx].callback_received) {
      LOG_ERROR("%s:%d: Interrupt callback was not received (cache index %d)\n",
                __func__,
                __LINE__,
                cache_idx);
      perf_int_restore_settings(dev_id);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
    }

    results[cache_idx] = time_delta_ns(perf_int_cache[cache_idx].start,
                                       perf_int_cache[cache_idx].stop);
    /* Provide results in us instead of ns*/
    results[cache_idx] /= 1000;
  }

  /* Calculate minimum and maximum */
  double min = results[0], max = results[0];
  for (cache_idx = 0; cache_idx < it; cache_idx++) {
    if (!perf_int_cache[cache_idx].valid) {
      break;
    }
    min = min > results[cache_idx] ? results[cache_idx] : min;
    max = max < results[cache_idx] ? results[cache_idx] : max;
  }
  *min_latency_us = min;
  *max_latency_us = max;

  /* Return a number of interrupts */
  *interrupts = cache_idx;
  *iterations = iter_cnt;

  /* Calculate average and standard deviation */
  if (cache_idx > 0 && cache_idx < PERF_INT_CACHE_MAX) {
    basic_stats(results, cache_idx, avg_latency_us, sd_latency_us);
  }

  /* Restore callbacks and enable status */
  perf_int_restore_settings(dev_id);

  return BF_SUCCESS;
}

/**
 * @brief Run performance test that will measure latency of
 * interrupts processing
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @param it number of iterations
 * @return bf_status_t
 */
bf_status_t run_int_test(bf_dev_id_t dev_id,
                         perf_bus_t_enum bus_type,
                         int it,
                         struct interrupts_result *result) {
  bf_status_t status = BF_UNEXPECTED;

  if (!result) {
    LOG_ERROR("%s:%d: No allocated memory for results\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  result->status = false;

  bool is_sw_model;
  bf_status_t sts;
  sts = bf_drv_device_type_get(dev_id, &is_sw_model);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d: Error getting device type for device: %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  if (is_sw_model) {
    LOG_ERROR(
        "%s:%d: The test can only be run on hardware, it does not apply to the "
        "model\n",
        __func__,
        __LINE__);
    return BF_NOT_SUPPORTED;
  }

  if (!is_bus_ints_supported(dev_id, bus_type)) {
    LOG_ERROR("%s:%d: %s interrupt not supported for device: %d\n",
              __func__,
              __LINE__,
              bus_type_name[bus_type],
              dev_id);
    bf_sys_dbgchk(0);
    return BF_NOT_SUPPORTED;
  }

  if (bus_type == PBUS) {
    status = perf_int_pbus_test(dev_id,
                                it,
                                &result->interrupts,
                                &result->iterations,
                                &result->avg_latency_us,
                                &result->sd_latency_us,
                                &result->min_latency_us,
                                &result->max_latency_us);
  } else if (bus_type == MBUS) {
    status = perf_int_mbus_test(dev_id,
                                it,
                                &result->interrupts,
                                &result->iterations,
                                &result->avg_latency_us,
                                &result->sd_latency_us,
                                &result->min_latency_us,
                                &result->max_latency_us);
  } else if (bus_type == CBUS) {
    status = perf_int_cbus_test(dev_id,
                                it,
                                &result->interrupts,
                                &result->iterations,
                                &result->avg_latency_us,
                                &result->sd_latency_us,
                                &result->min_latency_us,
                                &result->max_latency_us);
  } else if (bus_type == HOSTIF) {
    status = perf_int_hostif_test(dev_id,
                                  it,
                                  &result->interrupts,
                                  &result->iterations,
                                  &result->avg_latency_us,
                                  &result->sd_latency_us,
                                  &result->min_latency_us,
                                  &result->max_latency_us);
  } else {
    LOG_ERROR(
        "%s:%d: Bus type not supported %d\n", __func__, __LINE__, bus_type);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d: %s interrupt error for device: %d\n",
              __func__,
              __LINE__,
              bus_type_name[bus_type],
              dev_id);
    bf_sys_dbgchk(0);
    return status;
  }

  result->status = true;

  return BF_SUCCESS;
}

struct test_description interrupts_test = {
    .test_name = "interrupts",
    .description =
        "The Interrupts test measures interrupt processing latency.\n"
        "It uses the CLOCK_MONOTONIC POSIX clock to measure the processing\n"
        "latency of a single interrupt event. The bus type and number of\n"
        "iterations are specified within the test parameters.\n"
        "The test sequentially injects the maximum number of interrupts\n"
        "available for the given bus. Each bus may have a different number of\n"
        "interrupts.\n"
        "The reported test result value is the average processing latency of\n"
        "the interrupt events.\n",
    .params = {{.name = "bus", .type = "enum", .defaults = "0"},
               {.name = "iterations", .type = "int", .defaults = "10"},
               // last element
               {.name = ""}},
    .results = {{.header = "bus", .unit = "[-]", .type = "enum"},
                {.header = "interrupts", .unit = "[-]", .type = "int"},
                {.header = "iterations", .unit = "[-]", .type = "int"},
                {.header = "latency_avg", .unit = "[us]", .type = "double"},
                {.header = "latency_sd", .unit = "[us]", .type = "double"},
                {.header = "latency_min", .unit = "[us]", .type = "double"},
                {.header = "latency_max", .unit = "[us]", .type = "double"},
                // last element
                {.header = ""}}};

/**
 * @brief Run performance test that will write/read to/from SRAM memory,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type Bus type
 * @param it number of iterations
 * @return interrupts_result
 */
struct test_results interrupts(bf_dev_id_t dev_id,
                               perf_bus_t_enum bus_type,
                               int it) {
  struct interrupts_result raw_results;
  struct test_results results;
  memset(&raw_results, 0, sizeof(raw_results));
  memset(&results, 0, sizeof(results));

  run_int_test(dev_id, bus_type, it, &raw_results);

  results.status = raw_results.status;
  results.res_enum[RES_INT_BUS] = bus_type;
  results.res_int[RES_ITERATIONS] = raw_results.iterations;
  results.res_int[RES_INTERRUPTS] = raw_results.interrupts;
  results.res_double[RES_LAT_AVG] = raw_results.avg_latency_us;
  results.res_double[RES_LAT_SD] = raw_results.sd_latency_us;
  results.res_double[RES_LAT_MIN] = raw_results.min_latency_us;
  results.res_double[RES_LAT_MAX] = raw_results.max_latency_us;

  return results;
}
