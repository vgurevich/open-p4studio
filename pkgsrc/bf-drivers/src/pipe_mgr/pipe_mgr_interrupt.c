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
 * @file pipe_mgr_interrupt.c
 * @date
 *
 * ASIC-independent interrupt handling
 */

/* Local header files */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_interrupt.h"
#include "pipe_mgr_tof_interrupt.h"
#include "pipe_mgr_tof2_interrupt.h"
#include "pipe_mgr_tof3_interrupt.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_pktgen.h"
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_idle.h"
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <lld/tofino_defs.h>
#include <target-sys/bf_sal/bf_sys_intf.h>

pipe_mgr_intr_db_t *pipe_intr_evt_db[PIPE_MGR_NUM_DEVICES];

#define PIPE_INTR_TCAM_SCRUB_TIMER(dev) \
  (pipe_intr_evt_db[dev]->tcam_scrub_timer)
#define PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev) \
  (pipe_intr_evt_db[dev]->tcam_scrub_timer_val)

/* Interrupt mode set */
bf_status_t pipe_mgr_err_interrupt_mode_set(bf_dev_id_t dev, bool enable) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_interrupt_en_set_helper(dev_info, enable, true);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_interrupt_en_set_helper(dev_info, enable, true);
    case BF_DEV_FAMILY_TOFINO3:
      /* Enable interrupts */
      return pipe_mgr_tof3_interrupt_en_set_helper(dev_info, enable, true);
    default:
      return PIPE_UNEXPECTED;
  }
}

/* Enable interrupts by setting the enable0 register. */
pipe_status_t pipe_mgr_enable_interrupt_notifs(rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_interrupt_en_set_helper(dev_info, true, false);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_interrupt_en_set_helper(dev_info, true, false);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_interrupt_en_set_helper(dev_info, true, false);





    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_UNEXPECTED;
}

/* Register for interrupt notifications with lld */
pipe_status_t pipe_mgr_register_interrupt_notifs(rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_register_interrupt_notifs(dev_info);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_register_interrupt_notifs(dev_info);
    case BF_DEV_FAMILY_TOFINO3:
/* TODO TOFINO3, Enable after populating Tof3_int_top */
#if 0
      /* Enable interrupts */
      status |= pipe_mgr_tof3_interrupt_mode_en_set_helper(dev, true, true);

      /* Register for mau interrupt notifications */
      status |= pipe_mgr_tof3_register_mau_interrupt_notifs(dev);

      /* Register for mirror interrupt notifications */
      status |= pipe_mgr_tof3_register_mirror_interrupt_notifs(dev);

      /* Register for tm interrupt notifications */
      status |= pipe_mgr_tof3_register_tm_interrupt_notifs(dev);

      /* Register for parser interrupt notifications */
      status |= pipe_mgr_tof3_register_parser_interrupt_notifs(dev);

      /* Register for ig_deparser interrupt notifications */
      status |= pipe_mgr_tof3_register_deparser_interrupt_notifs(dev);

      /* Register for pgr interrupt notifications */
      status |= pipe_mgr_tof3_register_pgr_interrupt_notifs(dev);

      /* Register for gfm interrupt notifications */
      status |= pipe_mgr_tof3_register_gfm_interrupt_notifs(dev);

      /* Register for SBC interrupt notifications */
      status |= pipe_mgr_tof3_register_sbc_interrupt_notifs(dev);
#endif
      return PIPE_SUCCESS;





    case BF_DEV_FAMILY_UNKNOWN:
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_UNEXPECTED;
}

static pipe_status_t pipe_mgr_tcam_read(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_tcam_read(dev);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_tcam_read(dev);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_tcam_read(dev);
    default:
      return PIPE_UNEXPECTED;
  }
}

/* TCAM Timer Callback function */
void pipe_mgr_intr_tcam_refresh_cb(bf_sys_timer_t *timer, void *data) {
  bf_dev_id_t dev = 0;
  pipe_mgr_intr_db_t *db = NULL;

  db = (pipe_mgr_intr_db_t *)data;
  if (!db) return;
  dev = db->dev;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return;
  }
  (void)timer;

  /* Scrub parser and MAU TCAM memory */
  pipe_mgr_tcam_read(dev);

  return;
}

pipe_status_t pipe_mgr_intr_init(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  int dev_pipes = dev_info->dev_cfg.num_pipelines;
  int dev_stages = dev_info->num_active_mau;
  bf_status_t bf_status = BF_SUCCESS;
  pipe_status_t sts = PIPE_SUCCESS;
  int max_rows = dev_info->dev_cfg.stage_cfg.num_sram_rows >
                         dev_info->dev_cfg.stage_cfg.num_tcam_rows
                     ? dev_info->dev_cfg.stage_cfg.num_sram_rows
                     : dev_info->dev_cfg.stage_cfg.num_tcam_rows;
  max_rows = dev_info->dev_cfg.num_prsrs > max_rows
                 ? dev_info->dev_cfg.num_prsrs
                 : max_rows;

  pipe_intr_evt_db[dev] = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_intr_db_t));
  if (!pipe_intr_evt_db[dev]) return PIPE_NO_SYS_RESOURCES;

  pipe_intr_evt_db[dev]->cb_data =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_intr_evt_db[dev]->cb_data));
  if (!pipe_intr_evt_db[dev]->cb_data) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (int p = 0; p < dev_pipes; ++p) {
    pipe_intr_evt_db[dev]->cb_data[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*pipe_intr_evt_db[dev]->cb_data[p]));
    if (!pipe_intr_evt_db[dev]->cb_data[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    for (int s = 0; s < dev_stages; ++s) {
      pipe_intr_evt_db[dev]->cb_data[p][s] = PIPE_MGR_CALLOC(
          max_rows, sizeof(*pipe_intr_evt_db[dev]->cb_data[p][s]));
      if (!pipe_intr_evt_db[dev]->cb_data[p][s]) {
        sts = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      for (int r = 0; r < max_rows; ++r) {
        pipe_intr_evt_db[dev]->cb_data[p][s][r].pipe = p;
        pipe_intr_evt_db[dev]->cb_data[p][s][r].stage = s;
        pipe_intr_evt_db[dev]->cb_data[p][s][r].row = r;
      }
    }
  }

  pipe_intr_evt_db[dev]->err_evt_logs.front = -1;
  pipe_intr_evt_db[dev]->err_evt_logs.log =
      PIPE_MGR_CALLOC(PIPE_MGR_ERR_EVT_LOG_MAX, sizeof(pipe_mgr_err_evt_log_t));
  if (!(pipe_intr_evt_db[dev]->err_evt_logs.log)) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  PIPE_MGR_LOCK_INIT(pipe_intr_evt_db[dev]->err_evt_logs.mtx);

  bool is_model = false;
  bf_status = bf_drv_device_type_get(dev, &is_model);
#ifdef DEVICE_IS_EMULATOR
  is_model = false;
#endif
#define PIPE_MGR_INTR_TCAM_TIMER_DEF_VAL 120000
  if (BF_SUCCESS == bf_status && !is_model &&
      (!pipe_mgr_is_p4_skipped(dev_info))) {
    /* Set the default TCAM scrub timer interval. */
    PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev) = PIPE_MGR_INTR_TCAM_TIMER_DEF_VAL;
  } else {
    PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev) = 0;
  }

  return PIPE_SUCCESS;
cleanup:
  if (pipe_intr_evt_db[dev]) {
    for (int p = 0; p < dev_pipes; ++p) {
      if (pipe_intr_evt_db[dev]->cb_data && pipe_intr_evt_db[dev]->cb_data[p]) {
        for (int s = 0; s < dev_stages; ++s) {
          if (pipe_intr_evt_db[dev]->cb_data[p][s])
            PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data[p][s]);
        }
        PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data[p]);
      }
    }
    if (pipe_intr_evt_db[dev]->cb_data)
      PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data);
    PIPE_MGR_FREE(pipe_intr_evt_db[dev]);
    pipe_intr_evt_db[dev] = NULL;
  }
  return sts;
}

pipe_status_t pipe_mgr_intr_start_scrub_timer(bf_dev_id_t dev_id) {
  bf_sys_timer_status_t x;
  bool is_model = false;
  bf_status_t bf_status = bf_drv_device_type_get(dev_id, &is_model);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    return PIPE_SUCCESS;
  }
#ifdef DEVICE_IS_EMULATOR
  is_model = false;
#endif
  /* Turn off tcam scrub on tofino3 asic till bringup is done */
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    is_model = true;
  }
  if ((BF_SUCCESS == bf_status) && (is_model)) {
    return PIPE_SUCCESS;
  }
  if (!PIPE_INTR_TCAM_SCRUB_TIMER(dev_id).timer) {
    x = bf_sys_timer_create(&(PIPE_INTR_TCAM_SCRUB_TIMER(dev_id)),
                            PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev_id),
                            PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev_id),
                            pipe_mgr_intr_tcam_refresh_cb,
                            pipe_intr_evt_db[dev_id]);
    if (BF_SYS_TIMER_OK != x) {
      LOG_ERROR("Error %d creating TCAM scrub timer for dev %d", x, dev_id);
      PIPE_MGR_DBGCHK(BF_SYS_TIMER_OK == x);
      return PIPE_NO_SYS_RESOURCES;
    }
  }
  x = bf_sys_timer_start(&(PIPE_INTR_TCAM_SCRUB_TIMER(dev_id)));
  if (BF_SYS_TIMER_OK != x) {
    LOG_ERROR("Error %d starting TCAM scrub timer for dev %d", x, dev_id);
    PIPE_MGR_DBGCHK(x == BF_SYS_TIMER_OK);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_intr_cleanup(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s:%d Error, dev_info is NULL", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
  }

  if (pipe_intr_evt_db[dev]) {
    if (dev_info) {
      int dev_pipes = dev_info->num_active_pipes;
      int dev_stages = dev_info->num_active_mau;
      for (int p = 0; p < dev_pipes; ++p) {
        if (pipe_intr_evt_db[dev]->cb_data &&
            pipe_intr_evt_db[dev]->cb_data[p]) {
          for (int s = 0; s < dev_stages; ++s) {
            if (pipe_intr_evt_db[dev]->cb_data[p][s])
              PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data[p][s]);
          }
          PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data[p]);
        }
      }
    }
    if (pipe_intr_evt_db[dev]->cb_data)
      PIPE_MGR_FREE(pipe_intr_evt_db[dev]->cb_data);
    if (pipe_intr_evt_db[dev]->err_evt_logs.log) {
      PIPE_MGR_FREE(pipe_intr_evt_db[dev]->err_evt_logs.log);
      PIPE_MGR_LOCK_DESTROY(&pipe_intr_evt_db[dev]->err_evt_logs.mtx);
    }
    PIPE_MGR_FREE(pipe_intr_evt_db[dev]);
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_tcam_scrub_timer_stop(bf_dev_id_t dev_id) {
  bf_sys_timer_status_t x;
  if (PIPE_INTR_TCAM_SCRUB_TIMER(dev_id).timer) {
    if ((x = bf_sys_timer_del(&(PIPE_INTR_TCAM_SCRUB_TIMER(dev_id))))) {
      LOG_ERROR("Error %d deleting TCAM scrub timer for dev %d", x, dev_id);
    }
    PIPE_INTR_TCAM_SCRUB_TIMER(dev_id).timer = NULL;
  }
}

pipe_status_t pipe_mgr_tcam_scrub_timer_set(bf_dev_id_t dev,
                                            uint32_t msec_timer) {
  bf_status_t bf_status = BF_SUCCESS;
  uint32_t curr_timer = 0;
  bool is_model = false;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_INVALID_ARG;
  }

  bf_status = bf_drv_device_type_get(dev, &is_model);
#ifdef DEVICE_IS_EMULATOR
  is_model = false;
#endif
  /* Turn off tcam scrub on tofino3 asic till bringup is done */
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3) {
    is_model = true;
  }
  if ((BF_SUCCESS == bf_status) && (is_model)) {
    return PIPE_SUCCESS;
  }

  LOG_TRACE("Tcam scrub timer value set to %d (dev %d)", msec_timer, dev);
  curr_timer = PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev);
  PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev) = msec_timer;

  if (curr_timer != 0) {
    /* Stop the timer */
    bf_status = bf_sys_timer_stop(&(PIPE_INTR_TCAM_SCRUB_TIMER(dev)));
    if (BF_SYS_TIMER_OK != bf_status) {
      LOG_TRACE("Current Tcam scrub timer stop failed");
    }
    bf_status = bf_sys_timer_del(&(PIPE_INTR_TCAM_SCRUB_TIMER(dev)));
    if (BF_SYS_TIMER_OK != bf_status) {
      LOG_TRACE("Current Tcam scrub timer delete failed");
    } else {
      LOG_TRACE("Current Tcam scrub timer deleted");
    }
  }

  if (msec_timer != 0) {
    /* Start the timer */
    pipe_status_t sts = pipe_mgr_intr_start_scrub_timer(dev);
    if (sts == PIPE_SUCCESS) {
      LOG_TRACE("New Tcam scrub timer started with timer val %d",
                PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev));
    }
    return sts;
  }
  return PIPE_SUCCESS;
}

uint32_t pipe_mgr_tcam_scrub_timer_get(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_INVALID_ARG;
  }
  return (PIPE_INTR_TCAM_SCRUB_TIMER_VAL(dev));
}

void pipe_mgr_err_evt_log_dump(ucli_context_t *uc, bf_dev_id_t dev, int n) {
  int front = 0, index = 0, cntr = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid dev : %d\n", dev);
    return;
  }
  if (n < 0) {
    n = PIPE_MGR_ERR_EVT_LOG_MAX;
  }

  aim_printf(&uc->pvs, "Dumping dev %d interrupts \n", dev);
  front = PIPE_INTR_ERR_EVT_FRONT(dev);
  if (front == -1) {
    aim_printf(&uc->pvs, "No interrupts \n");
    aim_printf(&uc->pvs, "\nTotal number of interrupts: %d \n", cntr);
    return;
  }

  /* Dump backwards from front till zero */
  for (index = front; index >= 0; index--) {
    if (!(PIPE_INTR_ERR_EVT_LOG(dev, index).valid)) {
      continue;
    }
    if (n > 0) {
      pipe_mgr_err_evt_log_dump_by_index(uc, dev, index, cntr);
      --n;
    }
    ++cntr;
  }
  /* Dump from max backwards to front */
  for (index = (PIPE_MGR_ERR_EVT_LOG_MAX - 1); index > front; index--) {
    if (!(PIPE_INTR_ERR_EVT_LOG(dev, index).valid)) {
      continue;
    }
    if (n > 0) {
      pipe_mgr_err_evt_log_dump_by_index(uc, dev, index, cntr);
      --n;
    }
    ++cntr;
  }
  aim_printf(&uc->pvs, "\nTotal number of interrupts: %d \n", cntr);
}
