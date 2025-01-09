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


#include <inttypes.h>
#include <string.h>

#include "tm_ctx.h"
#include "tm_init.h"
#include "tm_hw_access.h"
#include <traffic_mgr/tm_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <dvm/bf_drv_intf.h>
#include <mc_mgr/mc_mgr_shared_dr.h>

/*
 *    This file sets up TM driver / management layer for configuring TM.
 *    A set of init routines before NorthBound APIs can work
 */

extern bf_status_t bf_tm_tofino_set_default(bf_dev_id_t);
extern bf_status_t bf_tm_tofinolite_set_default(bf_dev_id_t);
extern bf_status_t bf_tm_tof2_set_default(bf_dev_id_t);
extern bf_status_t bf_tm_tof3_set_default(bf_dev_id_t);





extern bf_status_t bf_tm_tofino_start_init_seq_during_fast_recfg(bf_dev_id_t);
extern bf_status_t bf_tm_tofinolite_start_init_seq_during_fast_recfg(
    bf_dev_id_t);
extern bf_status_t bf_tm_tof2_start_init_seq_during_fast_recfg(bf_dev_id_t);
extern bf_status_t bf_tm_tof3_start_init_seq_during_fast_recfg(bf_dev_id_t);

bf_tm_dev_ctx_t *g_tm_ctx[BF_TM_NUM_ASIC] = {NULL};
static bool tm_dev_locked[BF_TM_NUM_ASIC] = {0};

// This lock is used to protect the timer_cb from using g_tm_ctx[] that is
// deleted
tm_mutex_t g_tm_timer_lock[BF_TM_NUM_ASIC];
bool g_tm_ctx_valid[BF_TM_NUM_ASIC] = {false};

/* Used for hitless HA; when in UT mode, g_tm_restore_ctx and
 * g_tm_ctx are 2 seperate memory areas and different ctx_type
 * (BF_TM_CTX_RESTORED and  BF_TM_CTX_REGULAR respectively)
 * In regular Hitless HA mode, g_tm_restore_ctx and g_tm_ctx point
 * to common memory area.
 */
bf_tm_dev_ctx_t *g_tm_restore_ctx[BF_TM_NUM_ASIC] = {NULL};

void bf_tm_destruct_tm_ctx(bf_tm_dev_ctx_t *tm_ctx) {
  if (!tm_ctx) {
    return;
  }

  // Only regular (i.e. normal) g_tm_ctx has g_tm_ctx_valid flag
  if (tm_ctx->ctx_type == BF_TM_CTX_REGULAR) {
    g_tm_ctx_valid[tm_ctx->devid] = false;
  }

  if (tm_ctx->sw_inited) {
    // Free PPG
    bf_tm_ppg_delete(tm_ctx);
    // Free Ingress Pool
    bf_tm_ig_pool_delete(tm_ctx);
    // Free Egress Pool
    bf_tm_eg_pool_delete(tm_ctx);
    // Free ports
    bf_tm_ports_delete(tm_ctx);
    // scheduler
    bf_tm_sch_delete(tm_ctx);
    // queue
    bf_tm_q_delete(tm_ctx);
    // Pipe
    bf_tm_pipe_delete(tm_ctx);
    // mc_fifo
    bf_tm_mcast_delete(tm_ctx);
    // Uninit the cached counter Infra.
    tm_uninit_cached_counters(tm_ctx);
  }
  if (bf_sys_rmutex_del(&tm_ctx->lock)) {
    LOG_ERROR("%s:%d Unable to destroy TM CTX mutex for dev %d",
              __func__,
              __LINE__,
              tm_ctx->devid);
  }
  bf_sys_free(tm_ctx);
  tm_ctx = NULL;
  return;
}

bf_tm_status_t bf_tm_construct_modules(bf_tm_dev_ctx_t *tm_ctx) {
  bf_tm_status_t rc = BF_TM_EOK;
  // Create PPGs, POOLs, Queues etc and init them to default values.

  // Constraints:
  //  1. PPGs have to be inited before PORTS
  //     (i.e call bf_tm_init_ppg() before bf_tm_init_ports())
  rc = bf_tm_init_ppg(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create PPG context");
    return rc;
  }
  rc = bf_tm_init_q(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create queue context");
    return rc;
  }
  rc = bf_tm_init_ig_pool(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create Ig Pools context");
    return rc;
  }
  rc = bf_tm_init_eg_pool(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create Eg Pools context");
    return rc;
  }
  rc = bf_tm_init_ports(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create Port context");
    return rc;
  }
  rc = bf_tm_init_sch(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create scheduler context");
    return rc;
  }
  rc = bf_tm_init_pipe(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create pipe context");
    return rc;
  }
  rc = bf_tm_init_mcast(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create mc fifo context");
    return rc;
  }
  rc = bf_tm_init_counters(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create drop counter context");
    return rc;
  }
  rc = bf_tm_init_dev(tm_ctx);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to init misc TM");
    return rc;
  }

  tm_ctx->sw_inited = true;

  return (BF_SUCCESS);
}

/* According to ASIC capabilities setup resource counts */
static void bf_tm_setup_asic_type_cfg(bf_tm_dev_ctx_t *tm_ctx,
                                      bf_tm_cfg_t *tm_cfg) {
  memcpy(&(tm_ctx->tm_cfg), tm_cfg, sizeof(bf_tm_cfg_t));
}

bf_tm_status_t bf_tm_create_tm_ctx(bf_tm_asic_en asic_type,
                                   bf_dev_id_t devid,
                                   bf_tm_ctx_type_en ctx_type,
                                   bf_tm_dev_ctx_t **_tm_ctx) {
  bf_tm_cfg_t tm_cfg = {0};
  bf_tm_dev_ctx_t *tm_ctx;

  // Later on allocate memory using shm_open/mmap for
  // tm-ctx persistency across restarts
  // across client restarts
  if (*_tm_ctx) {
    bf_tm_destruct_tm_ctx(*_tm_ctx);
  }
  tm_ctx = bf_sys_calloc(1, sizeof(bf_tm_dev_ctx_t));
  if (!tm_ctx) {
    LOG_ERROR("TM: Unable to create TM context. Memory issue");
    return (BF_NO_SYS_RESOURCES);
  }
  tm_ctx->ctx_type = ctx_type;
  tm_ctx->asic_type = asic_type;
  tm_ctx->devid = devid;
  tm_ctx->batch_mode =
      true;  // By default TM configuration will be via DMA channel
  tm_ctx->fast_reconfig_init_seq = false;

  tm_ctx->read_por_wac_profile_table = false;
  tm_ctx->read_por_qac_profile_table = false;

  // These locks will protect access to the g_tm_ctx[] and any TM functions
  // that depend on that.
  if (!g_tm_ctx_valid[devid]) {
    TM_LOCK_INIT(g_tm_timer_lock[devid]);
  }

  g_tm_ctx_valid[devid] = true;

  TM_LOCK_INIT(tm_ctx->lock);

  if (BF_TM_IS_TOFINO(asic_type)) {
    bf_tm_tofino_cfg(&tm_cfg, devid);
  }
  if (BF_TM_IS_TOFINOLITE(asic_type)) {
    bf_tm_tofinolite_cfg(&tm_cfg);
  }
  if (BF_TM_IS_TOF2(asic_type)) {
    bf_tm_tof2_cfg(&tm_cfg, devid);
  }
  if (BF_TM_IS_TOF3(asic_type)) {
    bf_tm_tof3_cfg(&tm_cfg, devid);
  }






  bf_tm_setup_asic_type_cfg(tm_ctx, &tm_cfg);

  tm_ctx->target = BF_TM_TARGET_ASIC;  // By default TM target is ASIC.
                                       // However when running unit test with
                                       // model UT program will change target
                                       // to model before executing all tests.
  *_tm_ctx = tm_ctx;

  return (BF_TM_EOK);
}

bf_tm_status_t bf_tm_init_new_device(bf_dev_id_t dev,
                                     bf_tm_asic_en asic_type,
                                     bf_dma_info_t *dma_info) {
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t i = 0;
  uint32_t num_subdev;

  if (lld_sku_get_num_subdev(dev, &num_subdev, NULL) != LLD_OK) {
    return BF_INVALID_ARG;
  }
  if (num_subdev > BF_TM_NUM_SUBDEV) {
    return BF_INVALID_ARG;
  }

  rc = bf_tm_create_tm_ctx(asic_type, dev, BF_TM_CTX_REGULAR, &g_tm_ctx[dev]);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create TM context dev %d error 0x%0x", dev, rc);
    return (rc);
  }
  // For Tofino3 Number of SubDev can be 1 or 2
  g_tm_ctx[dev]->subdev_count = num_subdev;

  rc = bf_tm_construct_modules(g_tm_ctx[dev]);
  if (BF_TM_IS_NOTOK(rc)) {
    // Initiate destruction
    LOG_ERROR(
        "TM: Unable to create sub-modules of TM dev %d error 0x%0x", dev, rc);
    bf_tm_destruct_tm_ctx(g_tm_ctx[dev]);
    return (rc);
  }

  for (i = 0; i < num_subdev; i++) {
    if (asic_type != BF_TM_ASIC_TOFINO) {
      /* On Tofino the WriteList DRs are shared with mc-mgr who requests
       * locking.
       * On other chips we have exclusive access so nobody else will request the
       * locks for us.  Note we still need the locks since multiple threads may
       * operate on the DRs. */
      int x;
      x = lld_subdev_dr_lock_required(
          dev, (bf_subdev_id_t)i, lld_dr_tx_que_write_list);
      if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
      x = lld_subdev_dr_lock_required(
          dev, (bf_subdev_id_t)i, lld_dr_cmp_que_write_list);
      if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
    }
    if (!tm_is_device_locked(dev)) {
      tm_enable_all_dr(dev);
    }

    // Before any writes to HW device is performed, setup DMA memory pool
    rc = bf_tm_setup_dma_sizes(
        dev,
        i,
        dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST].dma_buf_size *
            dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST].dma_buf_cnt,
        dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST].dma_buf_size);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("TM: Unable to setup DMA memory sizes");
    }

    rc = bf_tm_setup_dma(
        dev,
        i,
        dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST].dma_buf_pool_handle);
    if (BF_TM_IS_NOTOK(rc)) {
      LOG_ERROR("TM: Unable to setup DMA memory");
    } else {
      LOG_TRACE("TM on device %d is ready for hw configurations", dev);
    }
    dma_info++;
  }

  if (BF_TM_IS_NOTOK(rc)) {
    if (g_tm_ctx[dev] != NULL) {
      bf_tm_destruct_tm_ctx(g_tm_ctx[dev]);
      g_tm_ctx[dev] = NULL;
    }
  }
  return (rc);
}

bf_tm_status_t bf_tm_program_default_cfg(bf_dev_id_t dev,
                                         bf_tm_asic_en asic_type) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (asic_type == BF_TM_ASIC_TOFINO) {
    LOG_TRACE("TM: Configuring Tofino TM with default setting");
    rc = bf_tm_tofino_set_default(dev);
  } else if (asic_type == BF_TM_ASIC_TOFINOLITE) {
    rc = bf_tm_tofinolite_set_default(dev);
  } else if (asic_type == BF_TM_ASIC_TOF2) {
    rc = bf_tm_tof2_set_default(dev);
  } else if (asic_type == BF_TM_ASIC_TOF3) {
    rc = bf_tm_tof3_set_default(dev);




  } else {
    LOG_TRACE(
        "TM: Driver doesn't configure this generation's TM at this time.");
  }
  return (rc);
}

bf_tm_status_t bf_tm_init_interrupts(bf_dev_id_t dev) {
  bf_status_t status = BF_SUCCESS;

  for (uint8_t i = 0; i < g_tm_ctx[dev]->subdev_count; i++) {
    // Claim the traffic manager interrupts
    status = bf_int_claim_cbus(dev, i);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "interrupt registration with LLD failed for device %d, sts %s (%d)",
          dev,
          bf_err_str(status),
          status);
      return status;
    }
    // Enable the traffic manager interrupts
    status = bf_int_ena_cbus(dev, i);
    if (status != BF_SUCCESS) {
      LOG_ERROR("interrupt enable with LLD failed for device %d, sts %s (%d)",
                dev,
                bf_err_str(status),
                status);
      return status;
    }
  }
  LOG_TRACE("TM on device %d start interrupt processing", dev);
  return (status);
}

static bf_status_t bf_tm_set_dev_info(bf_dev_id_t dev) {
  lld_err_t lld_err;
  bf_sku_chip_part_rev_t part_rev;
  bf_status_t status;
  uint64_t bps_clock_speed, pps_clock_speed;

  /* Get core clock frequency */
  status = bf_drv_get_clock_speed(dev, &bps_clock_speed, &pps_clock_speed);
  if (status != BF_SUCCESS) {
    LOG_ERROR("TM: Not able to determine core clock speed, dev %d, sts %s (%d)",
              dev,
              bf_err_str(status),
              status);
    return status;
  }

  g_tm_ctx[dev]->clock_speed = pps_clock_speed;

  if (BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    // This needs to be TM_BPS clk_tm 1.5GHZ
    g_tm_ctx[dev]->bps_clock_speed = BF_SKU_CORE_CLK_1_5_GHZ;
  } else {
    g_tm_ctx[dev]->bps_clock_speed = bps_clock_speed;
  }

  LOG_TRACE("TM: set clock speed to %" PRIu64 " for dev %d",
            g_tm_ctx[dev]->clock_speed,
            dev);

  /* Get the chip part revision number. */
  lld_err = lld_sku_get_chip_part_revision_number(dev, &part_rev);
  if (lld_err != LLD_OK) {
    LOG_ERROR("TM: Not able to get chip part revision number, dev %d sts %d",
              dev,
              lld_err);
    return BF_UNEXPECTED;
  }

  g_tm_ctx[dev]->part_rev = part_rev;
  LOG_TRACE("TM: set chip part revision number, dev %d, part rev %d",
            dev,
            g_tm_ctx[dev]->part_rev);

  return BF_SUCCESS;
}

/* Warm init in quick mode (add device is not done) */
bf_status_t tm_warm_init_quick(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  /* Program the default config for now, actually the config from shadow
     needs to be set
  */
  /* Another workaround is to use TM_MUTEX_LOCK here.
     If we won't lock this mutex, timer will continue run refresh_counters() and
     then set_default_cfg() will corrupt shared counter_ctx memory.
     General TM_LOCK() will not works because set_default() turn it off using
     g_tm_ctx->internall_call = true flag.
  */
  if (!g_tm_ctx_valid[dev]) {
    LOG_ERROR("TM: Context is not valid for dev %d", dev);
    return BF_UNEXPECTED;
  }
  TM_MUTEX_LOCK(&g_tm_timer_lock[dev]);
  rc = bf_tm_program_default_cfg(dev, g_tm_ctx[dev]->asic_type);
  if (rc != BF_SUCCESS) {
    TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev]);
    return (rc);
  }
  TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev]);

  return rc;
}

bf_status_t tm_add_device(bf_dev_id_t dev,
                          bf_dev_family_t dev_family,
                          bf_device_profile_t *profile,
                          bf_dma_info_t *dma_info,
                          bf_dev_init_mode_t warm_init_mode) {
  bf_status_t rc = BF_SUCCESS;
  bool is_sw_model;
  (void)profile;

  LOG_TRACE("New device discovered... start TM init");

  // Use dev_type to distinguish tofino/tofino-lite
  bf_tm_asic_en asic_type;
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      asic_type = BF_TM_ASIC_TOFINO;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      asic_type = BF_TM_ASIC_TOF2;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      asic_type = BF_TM_ASIC_TOF3;
      break;





    default:
      rc = BF_INVALID_ARG;
      LOG_ERROR(
          "TM: Device initialization failed for dev %d, rc = %d, device type "
          "not recognized.",
          dev,
          rc);
      return (rc);
  }

  rc = bf_tm_init_new_device(dev, asic_type, dma_info);  // TBD TF3 fix

  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Device initialization failed for dev %d, rc = %d", dev, rc);
    return (rc);
  }

  /* Set device specific info (core clk freq, dev revision) from LLD */
  rc = bf_tm_set_dev_info(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Device initialization failed for dev %d, rc = %d", dev, rc);
    // free up TM ctx memory.
    tm_cleanup_device(g_tm_ctx[dev]);
    g_tm_ctx[dev] = NULL;
    return BF_UNEXPECTED;
  }

  /* Get the device type from DVM and cache it */
  rc = bf_drv_device_type_get(dev, &is_sw_model);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Device type get failed for device %d, status %s (%d)",
              dev,
              bf_err_str(rc),
              rc);
    // free up TM ctx memory.
    tm_cleanup_device(g_tm_ctx[dev]);
    g_tm_ctx[dev] = NULL;
    return (rc);
  }

#ifdef DEVICE_IS_EMULATOR
  g_tm_ctx[dev]->target = BF_TM_TARGET_ASIC;
#else
  if (is_sw_model) {
    g_tm_ctx[dev]->target = BF_TM_TARGET_MODEL;
  } else {
    g_tm_ctx[dev]->target = BF_TM_TARGET_ASIC;
  }
#endif
  LOG_TRACE("TM: device %d, device type set to %s",
            dev,
            (g_tm_ctx[dev]->target == BF_TM_TARGET_MODEL) ? "Model" : "ASIC");

  g_tm_ctx[dev]->current_init_mode = warm_init_mode;
  if ((warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
      (warm_init_mode == BF_DEV_INIT_COLD)) {
    rc = bf_tm_program_default_cfg(dev, asic_type);
  } else if (warm_init_mode == BF_DEV_WARM_INIT_HITLESS) {
    LOG_TRACE("TM: Start TM init in hitless restart mode.");
    rc = bf_tm_restore_device_cfg(dev);
  }

  if (rc == BF_SUCCESS && warm_init_mode == BF_DEV_INIT_COLD) {
    // claim/process interrupts after device init in cold boot case.
    // In case of fast reconfig or hitless restart, wait until
    // fast reconfig is complete (device_unlock()) or warm_init_end
    // phase to begin processing interrupts.
    rc = bf_tm_init_interrupts(dev);
  }
  if (rc != BF_SUCCESS) {
    // free up TM ctx memory.
    tm_cleanup_device(g_tm_ctx[dev]);
    g_tm_ctx[dev] = NULL;
    return (rc);
  }

  /* On normal init, make sure all DMAs have completed before returning. */
  if (warm_init_mode == BF_DEV_INIT_COLD) {
    bf_tm_complete_operations(dev);

#ifndef DEVICE_IS_EMULATOR
    // All the resources are initialized so we can start the Cached Counter
    // Timer
    rc = tm_start_cached_counters_timer(g_tm_ctx[dev]);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: failed to start Cache counter Timer"
          " for device %d, status %s (%d)",
          dev,
          bf_err_str(rc),
          rc);
      // free up TM ctx memory.
      tm_cleanup_device(g_tm_ctx[dev]);
      g_tm_ctx[dev] = NULL;
      return rc;
    }
#endif
  }

  /* Device initialization is completed, update the flag */
  g_tm_ctx[dev]->hw_inited = true;
  LOG_TRACE("TM on device %d successfully initialized and configured", dev);

  return (rc);
}

bf_status_t tm_remove_device(bf_dev_id_t dev) {
  if (g_tm_ctx[dev] == NULL) {
    // remove device is called before the add_device this can happen because
    // of error at a top level and we can unwind gracefully.
    LOG_ERROR(
        "TM: tm_remove_device called without valid tm_ctx"
        " for device %d",
        dev);
    return BF_SUCCESS;
  }

  // Disable the DRs
  tm_disable_all_dr(dev);

  // Stop the cached counter timer before cleaning up
  tm_stop_cached_counters_timer(g_tm_ctx[dev]);

  /* Cleanup any pending DMAs */
  bf_tm_cleanup_wlist(dev);

  // Grab this Mutex so that the tm_ctx stays valid in the timer_cb until
  // it exits
  TM_MUTEX_LOCK(&g_tm_timer_lock[dev]);
  tm_cleanup_device(g_tm_ctx[dev]);
  g_tm_ctx[dev] = NULL;
  TM_MUTEX_UNLOCK(&g_tm_timer_lock[dev]);
  if (bf_sys_rmutex_del(&g_tm_timer_lock[dev])) {
    LOG_ERROR("%s:%d Unable to destroy TM timer mutex for dev %d",
              __func__,
              __LINE__,
              dev);
  }
  return BF_SUCCESS;
}

bf_status_t tm_cleanup_device(bf_tm_dev_ctx_t *ctx) {
  // Invoked when device is under going fast reconfig
  // or when device is functionally removed. Example : Pluggable card.
  // In either case just destructing device context is good enough.
  // In fast reconfig mode, device will be added again during which time
  // context will be recreated.
  bf_tm_destruct_tm_ctx(ctx);
  return BF_SUCCESS;
}

bf_status_t tm_add_port(bf_dev_id_t dev,
                        bf_dev_port_t port,
                        bf_port_attributes_t *port_attrib,
                        bf_port_cb_direction_t direction) {
  bf_status_t rc = BF_SUCCESS;
  bf_tm_port_t *p;
  bf_port_speeds_t speed = port_attrib->port_speeds;

  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    rc = bf_tm_add_new_port(dev, port, speed);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not add port (%d)", port);
    }
    // Enable port for scheduling
    if (rc == BF_SUCCESS) {
      rc = bf_tm_sched_port_enable(dev, port, speed);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Could not enable port (%d) for scheduling", port);
      }
      rc = bf_tm_port_get_descriptor(dev, port, &p);
      if (rc != BF_SUCCESS) return (rc);
      TM_LOCK(dev, g_tm_ctx[dev]->lock);
      rc = bf_tm_port_set_uc_cut_through_limit(dev, p, p->uc_cut_through_limit);
      TM_UNLOCK_AND_FLUSH(dev);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Could not set cut through limit for port (%d) ", port);
      }
    }
  }
  return (rc);
}

bf_status_t tm_remove_port(bf_dev_id_t dev,
                           bf_dev_port_t port,
                           bf_port_cb_direction_t direction) {
  bf_status_t rc = BF_SUCCESS;

  if (tm_is_device_locked(dev)) {
    // We are in fast reconfig mode.
    // Do nothing.
    return BF_SUCCESS;
  }

  LOG_TRACE("Remove dev:%d port:%d direction:%d", dev, port, direction);

  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    rc = bf_tm_delete_port(dev, port);
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Could not remove port (%d) cleanly", port);
    }
    (void)bf_tm_sched_port_disable(dev, port);
  }
  return (rc);
}

bf_status_t tm_update_port_status(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bool state) {
  bf_status_t rc = BF_SUCCESS;

  rc = bf_tm_update_port_status(dev, port, state);

  return (rc);
}

bf_status_t tm_update_port_admin_state(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bool enable) {
  bf_status_t rc = BF_SUCCESS;

  LOG_TRACE("Set dev:%d port:%d admin_state:%d", dev, port, enable);

  rc = bf_tm_update_port_admin_state(dev, port, enable);

  return (rc);
}

bf_status_t tm_disable_all_port_tx(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;
  bf_dev_port_t port;
  bf_tm_port_t *p;
  int lport;
  uint32_t pipe;
  uint32_t num_pipes;

  /*
   * Disable scheduling for ALL ports. Ideally we would need this
   * for added and qac_rx enabled port only. But since config replay
   * during fast reconfig could push a new or different config, there is no
   * way to figure out the qac_rx enabled ports (as of now) before config
   * replay. So, scheduling has to be disabled for ALL ports.
   *
   * TODO:
   *      Optimize this to disable scheduling for added & qac_rx enabled
   *      ports only
   */
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (lport = 0; lport < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(pipe, lport);

      rc = bf_tm_port_get_descriptor(dev, port, &p);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s - Failed to get port descriptor for dev %d, dev_port %d",
            __func__,
            dev,
            port);
        return (rc);
      }

      rc = bf_tm_sch_force_disable_port_sched(dev, p);
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s - Failed to disable port scheduling for dev %d, dev_port "
            "%d",
            __func__,
            dev,
            port);
        return (rc);
      }
    }
  }

  return (rc);
}

/* tm_enable_all_dr
 *
 * Enables all the DRs used by the traffic manager
 */
void tm_enable_all_dr(bf_dev_id_t dev_id) {
  bf_dma_dr_id_t dr_id = lld_dr_tx_que_write_list;
  // Enable the lld_dr_tx_que_write_list
  lld_dr_enable_set(dev_id, dr_id, true);

  dr_id = lld_dr_cmp_que_write_list;
  // Enable the lld_dr_cmp_que_write_list
  lld_dr_enable_set(dev_id, dr_id, true);

  // Set the dr state for the dev to enable
  mcmgr_tm_set_dr_state(dev_id, true);
}

/* tm_disable_all_dr
 *
 * Disables all the DRs used by the traffic manager
 */
void tm_disable_all_dr(bf_dev_id_t dev_id) {
  bf_dma_dr_id_t dr_id = lld_dr_tx_que_write_list;
  // Disable the lld_dr_tx_que_write_list
  lld_dr_enable_set(dev_id, dr_id, false);

  dr_id = lld_dr_cmp_que_write_list;
  // Disable the lld_dr_cmp_que_write_list
  lld_dr_enable_set(dev_id, dr_id, false);

  // Set the dr state for the dev to disable
  mcmgr_tm_set_dr_state(dev_id, false);
}

bf_status_t tm_lock_device(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= BF_TM_NUM_ASIC) {
    return BF_INVALID_ARG;
  }

  tm_dev_locked[dev_id] = true;

  return status;
}

bf_status_t tm_unlock_device(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= BF_TM_NUM_ASIC) {
    return BF_INVALID_ARG;
  }
  tm_dev_locked[dev_id] = false;

  // Enable all DRs
  tm_enable_all_dr(dev_id);

  if ((g_tm_ctx[dev_id]->current_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
      (g_tm_ctx[dev_id]->current_init_mode ==
       BF_DEV_WARM_INIT_FAST_RECFG_QUICK)) {
    g_tm_ctx[dev_id]->current_init_mode = 0xffff;
  }

  /* Flush write lists */
  bf_tm_flush_wlist(dev_id);

  if ((g_tm_ctx[dev_id]->current_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
      (g_tm_ctx[dev_id]->current_init_mode ==
       BF_DEV_WARM_INIT_FAST_RECFG_QUICK)) {
    status = bf_tm_init_interrupts(dev_id);
  }

  return status;
}

bool tm_is_device_locked(bf_dev_id_t dev_id) {
  if (dev_id >= BF_TM_NUM_ASIC) {
    return false;
  }
  return tm_dev_locked[dev_id];
}

bf_status_t tm_chip_init_sequence_during_fast_reconfig(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  bf_tm_asic_en asic_type;

  if (!tm_is_device_locked(dev_id)) {
    return (status);
  }

  asic_type = g_tm_ctx[dev_id]->asic_type;

  // In fast-reconfig phase.. do TM init-sequence in core reset phase.
  // when init sequence is executed, config has to be pushed
  // using lld_write (do not cache write operations)
  if (asic_type == BF_TM_ASIC_TOFINO) {
    LOG_TRACE("Executing Init Sequence on Tofino in fast-reconfig mode ");
    status = bf_tm_tofino_start_init_seq_during_fast_recfg(dev_id);
  } else if (asic_type == BF_TM_ASIC_TOFINOLITE) {
    LOG_TRACE("Executing Init Sequence on TofinoLite in fast-reconfig mode ");
    status = bf_tm_tofinolite_start_init_seq_during_fast_recfg(dev_id);
  } else if (asic_type == BF_TM_ASIC_TOF2) {
    LOG_TRACE("Executing Init Sequence on Tof2 in fast-reconfig mode ");
    status = bf_tm_tof2_start_init_seq_during_fast_recfg(dev_id);
  } else if (asic_type == BF_TM_ASIC_TOF3) {
    LOG_TRACE("Executing Init Sequence on Tof3 in fast-reconfig mode ");
    status = bf_tm_tof3_start_init_seq_during_fast_recfg(dev_id);
  } else {
    // future asic
  }
  return (status);
}

bf_status_t tm_config_complete(bf_dev_id_t dev_id) {
  bf_status_t rc = BF_SUCCESS;

  bf_tm_complete_operations(dev_id);

#ifndef DEVICE_IS_EMULATOR
  // All the resources are initialized so we can start the Cached Counter Timer
  rc = tm_start_cached_counters_timer(g_tm_ctx[dev_id]);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "TM: failed to start Cache counter Timer"
        " for device %d, status %s (%d)",
        dev_id,
        bf_err_str(rc),
        rc);
    // If this call fails return error and the caller will do cold boot
  }
#endif
  return rc;
}

bf_status_t tm_push_delta_cfg_hitless_restart(bf_dev_id_t dev) {
  bf_status_t rc = BF_SUCCESS;

  if (!g_tm_ctx[dev]) {
    /* We don't know this device, hence nothing to do */
    return BF_SUCCESS;
  }

  rc = tm_unlock_device(dev);
  if (rc != BF_SUCCESS) {
    return rc;
  }

  if (g_tm_ctx[dev]->current_init_mode == BF_DEV_WARM_INIT_HITLESS) {
    LOG_TRACE(
        "Executing config delta change DMA in hitless warm init end phase");
    g_tm_ctx[dev]->current_init_mode = 0xffff;
    bf_tm_flush_wlist(dev);
    rc = bf_tm_init_interrupts(dev);

#ifndef DEVICE_IS_EMULATOR
    // All the resources are initialized so we can start the Cached Counter
    // Timer
    rc = tm_start_cached_counters_timer(g_tm_ctx[dev]);
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "TM: failed to start Cache counter Timer"
          " for device %d, status %s (%d)",
          dev,
          bf_err_str(rc),
          rc);
      // If this call fails return error and the caller will do cold boot
    }
#endif
  }
  return (rc);
}
