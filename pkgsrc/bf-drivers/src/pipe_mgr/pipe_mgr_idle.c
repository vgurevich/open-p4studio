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
 * @file pipe_mgr_idle.c
 * @date
 *
 * Implementation of Idle time management
 */

#include <stdint.h>
#include <stdbool.h>
#include <target-utils/bit_utils/bit_utils.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_idle_sweep.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_alpm.h"

#define IDLE_NUM_HW_MODES 4

static idle_mgr_ctx_t idle_ctx;
static idle_mgr_ctx_t *idle_mgr_ctx_p;

typedef enum {
  HW_IDLE_ACTIVE,
  HW_IDLE_DISABLE,
  HW_IDLE_INACTIVE
} idle_hw_state_e;

typedef struct idle_time_hw_modes_s {
  uint8_t bit_width;
  uint8_t entries_per_word;
  uint8_t def_active_state;
  bool multiple_active_states;
  bool supports_hw_sweep;
  bool supports_notify;
  bool supports_two_way_and_per_flow;
  uint32_t boot_value;
} idle_time_hw_modes_t;

idle_time_hw_modes_t idle_time_hw_modes[IDLE_NUM_HW_MODES] = {
    {.bit_width = 1,
     .entries_per_word = 8,
     .def_active_state = 1,
     .multiple_active_states = false,
     .supports_hw_sweep = false,
     .supports_notify = false,
     .supports_two_way_and_per_flow = false,
     .boot_value = 0xff},
    {.bit_width = 2,
     .entries_per_word = 4,
     .def_active_state = 0,
     .multiple_active_states = true,
     .supports_hw_sweep = true,
     .supports_notify = true,
     .supports_two_way_and_per_flow = false,
     .boot_value = 0xff},
    {.bit_width = 3,
     .entries_per_word = 2,
     .def_active_state = 0,
     .multiple_active_states = true,
     .supports_hw_sweep = true,
     .supports_notify = true,
     .supports_two_way_and_per_flow = true,
     .boot_value = 0x3f},
    {.bit_width = 6,
     .entries_per_word = 1,
     .def_active_state = 0,
     .multiple_active_states = true,
     .supports_hw_sweep = true,
     .supports_notify = true,
     .supports_two_way_and_per_flow = true,
     .boot_value = 0x3f}};

static pipe_status_t pipe_mgr_idle_append_task(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_task_node_t *tnode_p,
    bool move_to_front,
    bool lock_valid,
    lock_id_t lock_id);

static pipe_status_t pipe_mgr_idle_write_movereg_ctl(
    idle_tbl_info_t *idle_tbl_info);
static pipe_status_t pipe_mgr_idle_write_sweep_ctl(
    idle_tbl_info_t *idle_tbl_info, bool enable);

static pipe_status_t pipe_mgr_idle_drain_all_msgs(
    idle_tbl_info_t *idle_tbl_info);

idle_mgr_ctx_t *idle_mgr_ctx(void) { return idle_mgr_ctx_p; }

idle_mgr_dev_ctx_t *idle_mgr_dev_ctx(bf_dev_id_t dev_id) {
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return NULL;
  }

  return idle_mgr_ctx()->dev_ctx[dev_id];
}

pipe_status_t pipe_mgr_idle_add_device(bf_dev_id_t dev_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  if (idle_mgr_dev_ctx(dev_id)) {
    return PIPE_ALREADY_EXISTS;
  }

  idle_mgr_dev_ctx_t *dev_ctx = PIPE_MGR_CALLOC(1, sizeof(idle_mgr_dev_ctx_t));
  if (!dev_ctx) {
    LOG_ERROR("%s:%d Malloc failed ", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    PIPE_MGR_FREE(dev_ctx);
    return PIPE_INVALID_ARG;
  }

  int num_pipes = dev_info->num_active_pipes;
  int dev_stages = dev_info->num_active_mau;
  int dev_log_tbls = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  int p, s;
  dev_ctx->tbl_lookup =
      PIPE_MGR_CALLOC(num_pipes, sizeof(*dev_ctx->tbl_lookup));
  if (!dev_ctx->tbl_lookup) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  dev_ctx->movereg_ctl =
      PIPE_MGR_CALLOC(num_pipes, sizeof(*dev_ctx->movereg_ctl));
  if (!dev_ctx->movereg_ctl) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  dev_ctx->movereg_ctl_mtx =
      PIPE_MGR_CALLOC(num_pipes, sizeof(*dev_ctx->movereg_ctl_mtx));
  if (!dev_ctx->movereg_ctl_mtx) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (p = 0; p < num_pipes; ++p) {
    dev_ctx->tbl_lookup[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*dev_ctx->tbl_lookup[p]));
    if (!dev_ctx->tbl_lookup[p]) {
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    dev_ctx->movereg_ctl[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*dev_ctx->movereg_ctl[p]));
    if (!dev_ctx->movereg_ctl[p]) {
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    dev_ctx->movereg_ctl_mtx[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*dev_ctx->movereg_ctl_mtx[p]));
    if (!dev_ctx->movereg_ctl_mtx[p]) {
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    for (s = 0; s < dev_stages; ++s) {
      dev_ctx->tbl_lookup[p][s] =
          PIPE_MGR_CALLOC(dev_log_tbls, sizeof(*dev_ctx->tbl_lookup[p][s]));
      if (!dev_ctx->tbl_lookup[p][s]) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      dev_ctx->movereg_ctl[p][s] = PIPE_MGR_CALLOC(
          PIPE_MGR_NUM_IDLE_MOVEREG_REGS, sizeof(*dev_ctx->movereg_ctl[p][s]));
      if (!dev_ctx->tbl_lookup[p][s]) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      dev_ctx->movereg_ctl_mtx[p][s] =
          PIPE_MGR_CALLOC(PIPE_MGR_NUM_IDLE_MOVEREG_REGS,
                          sizeof(*dev_ctx->movereg_ctl_mtx[p][s]));
      if (!dev_ctx->movereg_ctl_mtx[p][s]) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
    }
  }

  int pipe, stage, r;
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (stage = 0; stage < dev_stages; stage++) {
      for (r = 0; r < PIPE_MGR_NUM_IDLE_MOVEREG_REGS; r++) {
        PIPE_MGR_LOCK_INIT(dev_ctx->movereg_ctl_mtx[pipe][stage][r]);
      }
    }
  }

  idle_mgr_ctx()->dev_ctx[dev_id] = dev_ctx;
  return PIPE_SUCCESS;
cleanup:
  if (dev_ctx) {
    if (dev_ctx->tbl_lookup) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->tbl_lookup[p]) {
          for (s = 0; s < dev_stages; ++s) {
            if (dev_ctx->tbl_lookup[p][s]) {
              PIPE_MGR_FREE(dev_ctx->tbl_lookup[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->tbl_lookup[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->tbl_lookup);
    }
    if (dev_ctx->movereg_ctl) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->movereg_ctl[p]) {
          for (s = 0; s < dev_stages; ++s) {
            if (dev_ctx->movereg_ctl[p][s]) {
              PIPE_MGR_FREE(dev_ctx->movereg_ctl[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->movereg_ctl[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->movereg_ctl);
    }
    if (dev_ctx->movereg_ctl_mtx) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->movereg_ctl_mtx[p]) {
          for (s = 0; s < dev_stages; ++s) {
            if (dev_ctx->movereg_ctl_mtx[p][s]) {
              PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx);
    }
    PIPE_MGR_FREE(dev_ctx);
  }
  return rc;
}

pipe_status_t pipe_mgr_idle_remove_device(bf_dev_id_t dev_id) {
  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  if (!idle_mgr_dev_ctx(dev_id)) {
    return PIPE_ALREADY_EXISTS;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  uint32_t pipe, stage, r;
  uint32_t num_pipes = dev_info->num_active_pipes;
  uint32_t num_stages = dev_info->num_active_mau;
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (stage = 0; stage < num_stages; stage++) {
      for (r = 0; r < PIPE_MGR_NUM_IDLE_MOVEREG_REGS; r++) {
        PIPE_MGR_LOCK_DESTROY(
            &idle_mgr_dev_ctx(dev_id)->movereg_ctl_mtx[pipe][stage][r]);
      }
    }
  }

  idle_mgr_dev_ctx_t *dev_ctx = idle_mgr_dev_ctx(dev_id);
  unsigned int p, s;
  if (dev_ctx) {
    if (dev_ctx->tbl_lookup) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->tbl_lookup[p]) {
          for (s = 0; s < num_stages; ++s) {
            if (dev_ctx->tbl_lookup[p][s]) {
              PIPE_MGR_FREE(dev_ctx->tbl_lookup[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->tbl_lookup[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->tbl_lookup);
    }
    if (dev_ctx->movereg_ctl) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->movereg_ctl[p]) {
          for (s = 0; s < num_stages; ++s) {
            if (dev_ctx->movereg_ctl[p][s]) {
              PIPE_MGR_FREE(dev_ctx->movereg_ctl[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->movereg_ctl[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->movereg_ctl);
    }
    if (dev_ctx->movereg_ctl_mtx) {
      for (p = 0; p < num_pipes; ++p) {
        if (dev_ctx->movereg_ctl_mtx[p]) {
          for (s = 0; s < num_stages; ++s) {
            if (dev_ctx->movereg_ctl_mtx[p][s]) {
              PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx[p][s]);
            }
          }
          PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx[p]);
        }
      }
      PIPE_MGR_FREE(dev_ctx->movereg_ctl_mtx);
    }
    PIPE_MGR_FREE(dev_ctx);
  }
  idle_mgr_ctx()->dev_ctx[dev_id] = NULL;
  return PIPE_SUCCESS;
}

static bool pipe_mgr_idle_is_in_fast_reconfig_mode(
    idle_tbl_info_t *idle_tbl_info) {
  if (pipe_mgr_fast_recfg_warm_init_in_progress(idle_tbl_info->dev_id)) {
    return true;
  }

  return false;
}

static idle_time_hw_modes_t *pipe_mgr_idle_get_hw_mode(uint8_t bit_width) {
  unsigned int i = 0;

  for (i = 0; i < IDLE_NUM_HW_MODES; i++) {
    if (idle_time_hw_modes[i].bit_width == bit_width) {
      break;
    }
  }

  if (i == IDLE_NUM_HW_MODES) {
    return NULL;
  }
  return &idle_time_hw_modes[i];
}

static int8_t pipe_mgr_idle_get_hw_entries_per_word(uint8_t bit_width) {
  idle_time_hw_modes_t *idle_time_hw_mode =
      pipe_mgr_idle_get_hw_mode(bit_width);

  if (idle_time_hw_mode == NULL) {
    LOG_ERROR("%s:%d - Idle time bit-width %d is not supported",
              __func__,
              __LINE__,
              bit_width);
    return -1;
  }

  return idle_time_hw_mode->entries_per_word;
}

static int8_t pipe_mgr_idle_get_hw_timeout_state_count(
    rmt_idle_time_tbl_params_t rmt_params) {
  uint8_t bit_width = rmt_params.bit_width;
  bool two_way_notify_enable = rmt_params.two_way_notify_enable;
  bool per_flow_enable = rmt_params.per_flow_enable;

  idle_time_hw_modes_t *idle_time_hw_mode =
      pipe_mgr_idle_get_hw_mode(bit_width);

  if (idle_time_hw_mode == NULL) {
    LOG_ERROR("%s:%d - Idle time bit-width %d is not supported",
              __func__,
              __LINE__,
              bit_width);
    return -1;
  }

  /* Verify that this bit-width supports hw-timeout */
  if (!idle_time_hw_mode->supports_notify) {
    LOG_ERROR(
        "%s:%d - Idle time bit-width %d does not support"
        " hw notifications",
        __func__,
        __LINE__,
        bit_width);
    return -1;
  }

  if (two_way_notify_enable && per_flow_enable &&
      !idle_time_hw_mode->supports_two_way_and_per_flow) {
    /* Compiler shouldn't allocate this way. Hw does not support this */
    PIPE_MGR_DBGCHK(0);
    return -1;
  }

  int8_t num_states = 1 << bit_width;
  num_states--;

  if (two_way_notify_enable) {
    num_states--;
  }

  if (per_flow_enable) {
    num_states--;
  }

  return num_states;
}

static int8_t pipe_mgr_idle_get_entry_init_val(
    idle_tbl_info_t *idle_tbl_info,
    rmt_idle_time_tbl_params_t rmt_params,
    idle_hw_state_e hw_state) {
  uint8_t bit_width = rmt_params.bit_width;
  bool two_way_notify_enable = rmt_params.two_way_notify_enable;
  bool per_flow_enable = rmt_params.per_flow_enable;

  idle_time_hw_modes_t *idle_time_hw_mode =
      pipe_mgr_idle_get_hw_mode(bit_width);

  if (idle_time_hw_mode == NULL) {
    LOG_ERROR("%s:%d - Idle time bit-width %d is not supported",
              __func__,
              __LINE__,
              bit_width);
    return -1;
  }

  if (!idle_time_hw_mode->supports_notify) {
    switch (hw_state) {
      case HW_IDLE_DISABLE:
      case HW_IDLE_INACTIVE:
        return 0;
        break;
      case HW_IDLE_ACTIVE:
        return 1;
        break;
    }
  }

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    switch (hw_state) {
      case HW_IDLE_DISABLE:
        return ((1 << bit_width) - 1);
        break;
      case HW_IDLE_ACTIVE:
        /* In case of notify mode, the value is either 0 or 1 */
        if (idle_time_hw_mode->multiple_active_states &&
            two_way_notify_enable) {
          return (idle_time_hw_mode->def_active_state + 1);
        } else {
          return idle_time_hw_mode->def_active_state;
        }
        break;
      case HW_IDLE_INACTIVE:
        if (per_flow_enable) {
          return (((1 << bit_width) - 1) - 1);
        } else {
          return ((1 << bit_width) - 1);
        }
        break;
    }
  } else {
    switch (hw_state) {
      case HW_IDLE_DISABLE:
      case HW_IDLE_INACTIVE:
        if (per_flow_enable) {
          return (((1 << bit_width) - 1) - 1);
        } else {
          return ((1 << bit_width) - 1);
        }
        break;
      case HW_IDLE_ACTIVE:
        return idle_time_hw_mode->def_active_state;
        break;
    }
  }

  return -1;
}

static uint32_t pipe_mgr_idle_get_boot_val(uint8_t bit_width) {
  idle_time_hw_modes_t *idle_time_hw_mode =
      pipe_mgr_idle_get_hw_mode(bit_width);

  if (idle_time_hw_mode == NULL) {
    LOG_ERROR("%s:%d - Idle time bit-width %d is not supported",
              __func__,
              __LINE__,
              bit_width);
    return -1;
  }

  return idle_time_hw_mode->boot_value;
}

bool pipe_mgr_idle_is_state_active(rmt_idle_time_tbl_params_t rmt_params,
                                   uint8_t value) {
  uint8_t bit_width = rmt_params.bit_width;
  bool two_way_notify_enable = rmt_params.two_way_notify_enable;

  idle_time_hw_modes_t *idle_time_hw_mode =
      pipe_mgr_idle_get_hw_mode(bit_width);

  if (idle_time_hw_mode == NULL) {
    LOG_ERROR("%s:%d - Idle time bit-width %d is not supported",
              __func__,
              __LINE__,
              bit_width);
    return false;
  }

  if (idle_time_hw_mode->def_active_state == value) {
    return true;
  }

  if (idle_time_hw_mode->multiple_active_states && two_way_notify_enable) {
    if (value == idle_time_hw_mode->def_active_state + 1) {
      return true;
    }
  }

  return false;
}

pipe_status_t pipe_mgr_idle_get_sweep_interval(
    idle_tbl_info_t *idle_tbl_info,
    uint64_t clock_speed,
    pipe_idle_time_params_t params,
    rmt_idle_time_tbl_params_t rmt_params,
    uint32_t *hw_sweep_interval,
    uint32_t *hw_sweep_period,
    uint32_t *hw_notify_period,
    uint32_t *sw_sweep_period) {
  *hw_sweep_interval = 0;
  *hw_sweep_period = 0;
  *hw_notify_period = 0;
  *sw_sweep_period = 0;

  idle_time_hw_modes_t *hw_mode =
      pipe_mgr_idle_get_hw_mode(rmt_params.bit_width);
  if (!hw_mode) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time bit-width %d is not supported",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rmt_params.bit_width);
    return PIPE_NOT_SUPPORTED;
  }

  switch (params.mode) {
    case POLL_MODE:
      break;
    case NOTIFY_MODE:
      if (rmt_params.notify_disable) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle time notifications are disabled in hardware "
            "cannot enable notify mode",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl);
        return PIPE_NOT_SUPPORTED;
      }

      /* Figure out the min of query interval, and min_ttl */
      uint32_t notify_time = params.u.notify.ttl_query_interval;

      int8_t num_states = pipe_mgr_idle_get_hw_timeout_state_count(rmt_params);
      if (num_states == -1) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle time notifications cannot be supported in this "
            "hardware configuration",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl);
        return PIPE_NOT_SUPPORTED;
      }

      /* After num_states of sweep, we should get a notification */
      uint64_t sweep_count = ((uint64_t)notify_time * (clock_speed / 1000));
      sweep_count /= num_states;
      sweep_count >>= TOF_MIN_IDLE_TIMEOUT_COUNTER_BIT;

      if (!sweep_count) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle time notify interval too small for "
            "bit-width %d Min notify interval %llx msecs",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            rmt_params.bit_width,
            ((1ull << TOF_MIN_IDLE_TIMEOUT_COUNTER_BIT) * num_states /
             (clock_speed / 1000)) +
                1);
        return PIPE_NOT_SUPPORTED;
      }

      if (sweep_count > (1ull << TOF_MAX_IDLE_TIMEOUT_COUNTER_WIDTH)) {
        *hw_sweep_interval = TOF_MAX_IDLE_TIMEOUT_COUNTER_WIDTH;
      } else {
        *hw_sweep_interval = log2_uint32_ceil((uint32_t)sweep_count);
      }

      if (clock_speed == 0) {
        LOG_ERROR("%s:%d Error, clock_speed is zero", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_INVALID_ARG;
      }

      uint64_t period =
          (1000ull << (*hw_sweep_interval + TOF_MIN_IDLE_TIMEOUT_COUNTER_BIT)) /
          (uint64_t)clock_speed;
      if (period >= ((1ull << 32) / num_states)) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_NOT_SUPPORTED;
      }
      *hw_sweep_period = period;
      *hw_notify_period = *hw_sweep_period * num_states;
      *sw_sweep_period = notify_time;

      if ((*sw_sweep_period) && !rmt_params.two_way_notify_enable) {
        /* 2-way notifications should be enabled. Otherwise, we cannot
         * support
         */
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle time notifications with specified params "
            "cannot be supported in this "
            "hardware configuration - two-way notifications "
            "should be enabled",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl);
        return PIPE_NOT_SUPPORTED;
      }
      break;
    default:
      LOG_ERROR("%s:%d Idle time mode %d not supported",
                __func__,
                __LINE__,
                params.mode);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NOT_SUPPORTED;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t is_table_populated(bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t tbl_hdl) {
  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t rc =
      pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, true, &count);
  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return true;
  }

  return (count != 0) ? true : false;
}

static void table_params_init(idle_tbl_t *idle_tbl) {
  idle_tbl_info_t *idle_tbl_info = idle_tbl->idle_tbl_info;
  uint64_t clock_speed =
      (uint64_t)pipe_mgr_get_sp_clock_speed(idle_tbl_info->dev_id);
  if (clock_speed == 0) {
    LOG_ERROR(
        "%s:%d Unable to determine platform clock speed", __func__, __LINE__);
    return;
  }

  int min_ttl = 0;
  for (int i = 0; i < idle_tbl->num_stages; i++) {
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];
    /* Check if notify is supported.*/
    idle_time_hw_modes_t *idle_time_hw_mode =
        pipe_mgr_idle_get_hw_mode(stage_info->rmt_params.bit_width);
    if (idle_time_hw_mode == NULL || !idle_time_hw_mode->supports_notify) {
      continue;
    }
    int8_t num_states =
        pipe_mgr_idle_get_hw_timeout_state_count(stage_info->rmt_params);
    int min_ttl_for_stage = ((1ull << TOF_MIN_IDLE_TIMEOUT_COUNTER_BIT) *
                             num_states / (clock_speed / 1000)) +
                            1;
    min_ttl = (min_ttl_for_stage > min_ttl) ? min_ttl_for_stage : min_ttl;
  }
  /* Set max and min ttl */
  idle_tbl_info->tbl_params.u.notify.min_ttl = min_ttl;
  idle_tbl_info->tbl_params.u.notify.max_ttl = UINT32_MAX;
}

static pipe_status_t pipe_mgr_idle_tbl_sweep_start(idle_tbl_t *idle_tbl) {
  int i;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];

    if (!stage_info->sw_sweep_period) {
      continue;
    }
    bf_sys_timer_status_t tsts;

    tsts = bf_sys_timer_create(&stage_info->sweep_timer,
                               stage_info->sw_sweep_period,
                               stage_info->sw_sweep_period,
                               pipe_mgr_idle_sw_timer_cb,
                               stage_info);
    if (tsts != BF_SYS_TIMER_OK) {
      // TODO: Stop and delete timers if started for any stage
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    tsts = bf_sys_timer_start(&stage_info->sweep_timer);
    if (tsts != BF_SYS_TIMER_OK) {
      // TODO: Stop and delete timers if started for any stage
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_tbl_sweep_stop(idle_tbl_t *idle_tbl) {
  int i;
  pipe_status_t status = PIPE_SUCCESS;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];

    if (!stage_info->sw_sweep_period) {
      continue;
    }
    bf_sys_timer_status_t tsts;

    /* We need to delete the timer here, in pipe_mgr_idle_tbl_sweep_start
     * it is created first then started, so it must be deleted to avoid
     * memory leaks */
    tsts = bf_sys_timer_del(&stage_info->sweep_timer);
    if (tsts != BF_SYS_TIMER_OK) {
      PIPE_MGR_DBGCHK(0);
      status |= PIPE_UNEXPECTED;
    }
  }

  return status;
}

static pipe_status_t pipe_mgr_idle_tbl_sweep_init(
    idle_tbl_t *idle_tbl, pipe_idle_time_params_t params) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info_t *idle_tbl_info = idle_tbl->idle_tbl_info;

  /* Go to each of the stage the table belongs to and populate the sweep
   * interval parameters that have been configured. */
  uint64_t clock_speed =
      (uint64_t)pipe_mgr_get_sp_clock_speed(idle_tbl_info->dev_id);
  int i;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];

    rc = pipe_mgr_idle_get_sweep_interval(idle_tbl_info,
                                          clock_speed,
                                          params,
                                          stage_info->rmt_params,
                                          &stage_info->hw_sweep_interval,
                                          &stage_info->hw_sweep_period,
                                          &stage_info->hw_notify_period,
                                          &stage_info->sw_sweep_period);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) "
          "Error calculating sweep interval rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    LOG_TRACE(
        "hw_sweep_interval %d hw_sweep_period %d hw_notify_perod %d "
        "sw_sweep_period %d",
        stage_info->hw_sweep_interval,
        stage_info->hw_sweep_period,
        stage_info->hw_notify_period,
        stage_info->sw_sweep_period);
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_idle_destroy_task_list(idle_task_list_t *tlist) {
  while (tlist->tasks) {
    idle_task_node_t *tnode = tlist->tasks;
    BF_LIST_DLL_REM(tlist->tasks, tnode, next, prev);
    PIPE_MGR_FREE(tnode);
  }
  PIPE_MGR_FREE(tlist);
}

static void pipe_mgr_idle_destroy_all_tasks(idle_task_list_t **task_list) {
  while (*task_list) {
    idle_task_list_t *llist = *task_list;

    BF_LIST_DLL_REM(*task_list, llist, next, prev);

    pipe_mgr_idle_destroy_task_list(llist);
  }
}

static void pipe_mgr_idl_tbl_stage_info_destroy(
    idle_tbl_stage_info_t *stage_info, int num_stages) {
  int i;

  if (!stage_info) {
    return;
  }

  for (i = 0; i < num_stages; i++) {
    int j = 0;
    for (j = 0; j < stage_info[i].no_words; j++) {
      PIPE_MGR_FREE(stage_info[i].entries[j]);
    }
    PIPE_MGR_FREE(stage_info[i].entries);
    PIPE_MGR_FREE(stage_info[i].hw_info.tbl_blk);

    bf_map_sts_t map_sts = BF_MAP_OK;
    unsigned long key = 0;
    while (map_sts == BF_MAP_OK) {
      idle_entry_metadata_t *ient_mdata = NULL;
      map_sts = bf_map_get_first_rmv(
          &stage_info[i].entry_mdata_map, &key, (void **)&ient_mdata);
      PIPE_MGR_FREE(ient_mdata);
    }

    bf_map_destroy(&stage_info[i].entry_mdata_map);

    pipe_mgr_idle_destroy_all_tasks(&stage_info[i].lock_pending_list);

    /* Discard the transaction list */
    pipe_mgr_idle_destroy_all_tasks(&stage_info[i].trans_list);
    pipe_mgr_idle_destroy_all_tasks(&stage_info[i].task_list);

    while (stage_info[i].pending_msgs) {
      idle_pending_msgs_t *pmsg = stage_info[i].pending_msgs;
      BF_LIST_DLL_REM(stage_info[i].pending_msgs, pmsg, next, prev);
      PIPE_MGR_FREE(pmsg->msgs);
      PIPE_MGR_FREE(pmsg);
    }
    PIPE_MGR_LOCK_DESTROY(&stage_info[i].tlist_mtx);
    PIPE_MGR_LOCK_DESTROY(&stage_info[i].pmsg_mtx);
    PIPE_MGR_LOCK_DESTROY(&stage_info[i].stage_map_mtx);
    PIPE_MGR_COND_DESTROY(&stage_info[i].tlist_cvar);
  }
  PIPE_MGR_FREE(stage_info);
}

idle_tbl_stage_info_t *pipe_mgr_idle_tbl_stage_info_alloc(
    idle_tbl_t *idle_tbl, pipe_mat_tbl_info_t *mat_tbl_info, int num_stages) {
  idle_tbl_stage_info_t *stage_info = NULL;
  rmt_tbl_info_t *rmt_info = NULL;

  stage_info = (idle_tbl_stage_info_t *)PIPE_MGR_MALLOC(
      sizeof(idle_tbl_stage_info_t) * num_stages);
  if (!stage_info) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return NULL;
  }

  PIPE_MGR_MEMSET(stage_info, 0, sizeof(idle_tbl_stage_info_t) * num_stages);

  int j, stage_idx;
  for (j = 0, stage_idx = 0; j < (int)mat_tbl_info->num_rmt_info; j++) {
    rmt_info = &mat_tbl_info->rmt_info[j];
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }

    stage_info[stage_idx].idle_tbl_p = idle_tbl;
    stage_info[stage_idx].stage_id = rmt_info->stage_id;
    stage_info[stage_idx].stage_idx = stage_idx;
    stage_info[stage_idx].stage_table_handle = rmt_info->handle;
    stage_info[stage_idx].rmt_params = rmt_info->params.idle;
    /* We cannot support getting the current ttl etc if two-way notifications
     * are not enabled
     */
    if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl->idle_tbl_info)) {
      PIPE_MGR_DBGCHK(rmt_info->params.idle.two_way_notify_enable);
    }
    stage_info[stage_idx].hw_info.pack_format = rmt_info->pack_format;

    PIPE_MGR_DBGCHK(rmt_info->num_tbl_banks == 1);
    stage_info[stage_idx].hw_info.num_blks =
        rmt_info->bank_map->num_tbl_word_blks;

    rmt_tbl_word_blk_t *tbl_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_MALLOC(
        sizeof(rmt_tbl_word_blk_t) * rmt_info->bank_map->num_tbl_word_blks);
    if (!tbl_blk) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }
    PIPE_MGR_MEMCPY(
        tbl_blk,
        rmt_info->bank_map->tbl_word_blk,
        sizeof(rmt_tbl_word_blk_t) * rmt_info->bank_map->num_tbl_word_blks);

    uint32_t max_vpn = 0;
    uint32_t tbl = 0;
    for (tbl = 0; tbl < rmt_info->bank_map->num_tbl_word_blks; tbl++) {
      if (tbl_blk[tbl].vpn_id[0] > max_vpn) {
        max_vpn = tbl_blk[tbl].vpn_id[0];
      }
    }

    stage_info[stage_idx].hw_info.tbl_blk = tbl_blk;
    stage_info[stage_idx].hw_info.max_vpn = max_vpn;
    stage_info[stage_idx].no_words =
        1 + ((rmt_info->num_entries - 1) /
             rmt_info->pack_format.entries_per_tbl_word);

    stage_info[stage_idx].entries_per_word =
        pipe_mgr_idle_get_hw_entries_per_word(rmt_info->params.idle.bit_width);
    PIPE_MGR_DBGCHK(stage_info[stage_idx].entries_per_word != -1);

    idle_entry_t **entries = (idle_entry_t **)PIPE_MGR_MALLOC(
        sizeof(idle_entry_t *) * stage_info[stage_idx].no_words);
    if (!entries) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      goto cleanup;
    }

    PIPE_MGR_MEMSET(
        entries, 0, sizeof(idle_entry_t *) * stage_info[stage_idx].no_words);

    int i;
    for (i = 0; i < stage_info[stage_idx].no_words; i++) {
      entries[i] = (idle_entry_t *)PIPE_MGR_MALLOC(
          sizeof(idle_entry_t) * stage_info[stage_idx].entries_per_word);
      if (!entries[i]) {
        LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
        goto cleanup;
      }
      PIPE_MGR_MEMSET(
          entries[i],
          0,
          sizeof(idle_entry_t) * stage_info[stage_idx].entries_per_word);
      int k;
      for (k = 0; k < stage_info[stage_idx].entries_per_word; k++) {
        entries[i][k].index = i * stage_info[stage_idx].entries_per_word + k;
      }
    }

    stage_info[stage_idx].entries = entries;

    PIPE_MGR_LOCK_INIT(stage_info[stage_idx].tlist_mtx);
    PIPE_MGR_LOCK_INIT(stage_info[stage_idx].pmsg_mtx);
    PIPE_MGR_LOCK_INIT(stage_info[stage_idx].stage_map_mtx);
    PIPE_MGR_COND_INIT(stage_info[stage_idx].tlist_cvar);

    stage_idx++;
  }

  return stage_info;
cleanup:
  if (stage_info) {
    pipe_mgr_idl_tbl_stage_info_destroy(stage_info, num_stages);
  }
  return NULL;
}

/** \brief pipe_mgr_idle_init
 *        Initialize the idle data structures
 *
 * This function initializes all the global data structures used for idle
 * management
 *
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_idle_init(void) {
  if (idle_mgr_ctx()) {
    return PIPE_SUCCESS;
  }

  idle_mgr_ctx_p = &idle_ctx;
  return PIPE_SUCCESS;
}

static void pipe_mgr_idle_tbl_destroy(uint8_t no_idle_tbls,
                                      idle_tbl_t **idle_tbl_p) {
  idle_tbl_t *idle_tbl = *idle_tbl_p;

  if (!idle_tbl) {
    return;
  }

  uint8_t i;
  for (i = 0; i < no_idle_tbls; i++) {
    idle_tbl = &((*idle_tbl_p)[i]);
    pipe_mgr_idl_tbl_stage_info_destroy(idle_tbl->stage_info,
                                        idle_tbl->num_stages);
    idle_tbl->stage_info = NULL;
    idle_tbl->num_stages = 0;
  }

  PIPE_MGR_FREE(*idle_tbl_p);
  *idle_tbl_p = NULL;
}

static pipe_status_t pipe_mgr_idle_disable_sweep(idle_tbl_info_t *idle_tbl_info,
                                                 bool hw_update) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (idle_tbl_info->update_in_progress) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time update hit state in progress. Try again",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_IDLE_UPDATE_IN_PROGRESS;
  }

  if (idle_tbl_info->enabled) {
    idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[0];
    rc = pipe_mgr_idle_tbl_sweep_stop(idle_tbl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error stopping idle time "
          "sweep timer rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }

    if (hw_update) {
      rc = pipe_mgr_idle_write_movereg_ctl(idle_tbl_info);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
      if (!pipe_mgr_is_device_locked(idle_tbl_info->dev_id)) {
        rc = pipe_mgr_idle_write_sweep_ctl(idle_tbl_info, false);
        PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
      }
    }
  }

  return rc;
}

static pipe_status_t pipe_mgr_idle_tbl_discard(idle_tbl_info_t *idle_tbl_info) {
  if (pipe_mgr_is_device_virtual(idle_tbl_info->dev_id)) {
    return PIPE_SUCCESS;
  }

  if (idle_tbl_info->update_in_progress) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time update hit state in progress. Try again",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_IDLE_UPDATE_IN_PROGRESS;
  }
  PIPE_MGR_RW_WRLOCK(&idle_tbl_info->en_dis_rw_lock);
  /* Destroy all the idle-tbls */
  pipe_mgr_idle_tbl_destroy(idle_tbl_info->no_idle_tbls,
                            &idle_tbl_info->idle_tbls);
  idle_tbl_info->idle_tbls = NULL;
  idle_tbl_info->no_idle_tbls = 0;
  idle_tbl_info->update_count = 0;
  idle_tbl_info->update_in_progress = false;
  idle_tbl_info->enabled = false;
  idle_tbl_info->update_barrier_lock_id = PIPE_MGR_INVALID_LOCK_ID;
  PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_tbl_enable_sweep(
    idle_tbl_info_t *idle_tbl_info, bool hw_update) {
  uint8_t no_idle_tbls = idle_tbl_info->no_idle_tbls;
  idle_tbl_t *idle_tbls = idle_tbl_info->idle_tbls;
  pipe_status_t rc = PIPE_SUCCESS;
  int i;

  /* Set up the sweep parameters in our software state for each instance of the
   * idle table. */
  pipe_idle_time_params_t *params = &idle_tbl_info->tbl_params;
  for (i = 0; i < no_idle_tbls; i++) {
    idle_tbl_t *idle_tbl = &idle_tbls[i];
    rc = pipe_mgr_idle_tbl_sweep_init(idle_tbl, *params);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error initing idle time "
          "sweep rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  if (hw_update) {
    rc = pipe_mgr_idle_write_movereg_ctl(idle_tbl_info);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    if (!pipe_mgr_is_device_locked(idle_tbl_info->dev_id)) {
      rc = pipe_mgr_idle_write_sweep_ctl(
          idle_tbl_info, IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info) ? true : false);
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    }
  }

  /* Start the sweep timer */
  idle_tbl_t *idle_tbl = &idle_tbls[0];
  /* Create timer and update the hardware sweep interval */
  rc = pipe_mgr_idle_tbl_sweep_start(idle_tbl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) Error starting idle time "
        "sweep timer rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
  }

  return rc;
}

static pipe_status_t pipe_mgr_idle_tbl_allocate(
    idle_tbl_info_t *idle_tbl_info) {
  if (pipe_mgr_is_device_virtual(idle_tbl_info->dev_id)) {
    return PIPE_SUCCESS;
  }
  /* Depending on table params and symmetricity, the corresponding
   * idle_tbls need to be inited
   */
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_t *idle_tbls = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  uint8_t no_idle_tbls = 0;
  rmt_tbl_info_t *rmt_info = NULL;

  mat_tbl_info = pipe_mgr_get_tbl_info(
      idle_tbl_info->dev_id, idle_tbl_info->tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table %d not found in RMT database for device %d",
              __func__,
              __LINE__,
              idle_tbl_info->tbl_hdl,
              idle_tbl_info->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_idle_time_params_t *params = &idle_tbl_info->tbl_params;

  idle_tbl_info->no_idle_tbls = 0;
  idle_tbl_info->idle_tbls = NULL;

  if (idle_tbl_info->is_symmetric) {
    if (params->mode == NOTIFY_MODE) {
      no_idle_tbls = PIPE_BITMAP_COUNT(&idle_tbl_info->pipe_bmp);
    } else if (params->mode == POLL_MODE) {
      no_idle_tbls = 1;
    }
  } else {
    if (params->mode == NOTIFY_MODE) {
      no_idle_tbls = PIPE_BITMAP_COUNT(&idle_tbl_info->pipe_bmp);
    } else if (params->mode == POLL_MODE) {
      no_idle_tbls = idle_tbl_info->num_scopes;
    }
  }

  idle_tbls = PIPE_MGR_MALLOC(sizeof(idle_tbl_t) * no_idle_tbls);
  if (!idle_tbls) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMSET(idle_tbls, 0, sizeof(idle_tbl_t) * no_idle_tbls);

  int pipe_no = -1;
  uint8_t i;
  for (i = 0; i < no_idle_tbls; i++) {
    idle_tbl_t *idle_tbl = &idle_tbls[i];

    idle_tbl->idle_tbl_info = idle_tbl_info;
    PIPE_BITMAP_INIT(&idle_tbl->inst_pipe_bmp, PIPE_BMP_SIZE);
    PIPE_BITMAP_INIT(&idle_tbl->asym_scope_pipe_bmp, PIPE_BMP_SIZE);
    if (idle_tbl_info->is_symmetric) {
      if (params->mode == NOTIFY_MODE) {
        /* symm notify mode (4 tbl) */
        pipe_no = PIPE_BITMAP_GET_NEXT_BIT(&idle_tbl_info->pipe_bmp, pipe_no);
        PIPE_MGR_DBGCHK(pipe_no != -1);
        idle_tbl->pipe_id = pipe_no;
        PIPE_BITMAP_SET(&idle_tbl->inst_pipe_bmp, pipe_no);
      } else if (params->mode == POLL_MODE) {
        /* symm poll mode (1 tbl) */
        idle_tbl->pipe_id = BF_DEV_PIPE_ALL;
        PIPE_BITMAP_ASSIGN(&idle_tbl->inst_pipe_bmp, &idle_tbl_info->pipe_bmp);
      }
    } else {
      if (params->mode == NOTIFY_MODE) {
        /* asymm notify mode (4 tbl) */
        pipe_no = PIPE_BITMAP_GET_NEXT_BIT(&idle_tbl_info->pipe_bmp, pipe_no);
        PIPE_MGR_DBGCHK(pipe_no != -1);
        idle_tbl->pipe_id = pipe_no;
        PIPE_BITMAP_SET(&idle_tbl->inst_pipe_bmp, pipe_no);
        /* Store all pipes in scope in the pipe_bmp as they are used when
           doing entry add/del
        */
        pipe_mgr_get_all_pipes_in_scope(pipe_no,
                                        idle_tbl_info->num_scopes,
                                        &idle_tbl_info->scope_pipe_bmp[0],
                                        &idle_tbl->asym_scope_pipe_bmp);
      } else if (params->mode == POLL_MODE) {
        /* asymm mode (num_tbl=num_scopes)*/
        idle_tbl->pipe_id =
            pipe_mgr_get_lowest_pipe_in_scope(idle_tbl_info->scope_pipe_bmp[i]);
        pipe_mgr_convert_scope_pipe_bmp(idle_tbl_info->scope_pipe_bmp[i],
                                        &idle_tbl->inst_pipe_bmp);
        PIPE_BITMAP_ASSIGN(&idle_tbl->asym_scope_pipe_bmp,
                           &idle_tbl->inst_pipe_bmp);
      }
    }

    uint8_t num_stages = 0;
    uint8_t j;
    for (j = 0; j < mat_tbl_info->num_rmt_info; j++) {
      rmt_info = &mat_tbl_info->rmt_info[j];
      if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
        continue;
      }
      num_stages++;
    }

    PIPE_MGR_DBGCHK(num_stages);

    idle_tbl->stage_info =
        pipe_mgr_idle_tbl_stage_info_alloc(idle_tbl, mat_tbl_info, num_stages);
    if (!idle_tbl->stage_info) {
      LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    idle_tbl->num_stages = num_stages;
  }

  PIPE_MGR_RW_WRLOCK(&idle_tbl_info->en_dis_rw_lock);
  idle_tbl_info->no_idle_tbls = no_idle_tbls;
  idle_tbl_info->idle_tbls = idle_tbls;
  PIPE_MGR_RW_UNLOCK(&idle_tbl_info->en_dis_rw_lock);

  return PIPE_SUCCESS;
cleanup:
  if (idle_tbls) {
    pipe_mgr_idle_tbl_destroy(no_idle_tbls, &idle_tbls);
  }
  return rc;
}

void pipe_mgr_idle_tbl_info_destroy(idle_tbl_info_t *idle_tbl_info) {
  if (!idle_tbl_info) {
    return;
  }
  pipe_mgr_idle_tbl_destroy(idle_tbl_info->no_idle_tbls,
                            &idle_tbl_info->idle_tbls);

  bf_map_destroy(&idle_tbl_info->notif_list);
  bf_map_destroy(&idle_tbl_info->notif_list_old);
  bf_map_destroy(&idle_tbl_info->notif_list_spec_copy);
  PIPE_MGR_LOCK_DESTROY(&idle_tbl_info->notif_list_mtx);
  PIPE_MGR_RWLOCK_DESTROY(&idle_tbl_info->en_dis_rw_lock);
  if (idle_tbl_info->scope_pipe_bmp) {
    PIPE_MGR_FREE(idle_tbl_info->scope_pipe_bmp);
  }
  PIPE_MGR_FREE(idle_tbl_info->name);
  PIPE_MGR_FREE(idle_tbl_info);
}

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }
  bf_map_sts_t msts;
  msts =
      pipe_mgr_idle_tbl_map_get(dev_id,
                                &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
                                tbl_hdl,
                                (void **)&idle_tbl_info);
  if (msts != BF_MAP_OK) {
    return NULL;
  }
  return idle_tbl_info;
}

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get_next(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  unsigned long key = *tbl_hdl_p;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }
  bf_map_sts_t msts;
  msts = pipe_mgr_idle_tbl_map_get_next(
      dev_id,
      &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
      &key,
      (void **)&idle_tbl_info);
  if (msts != BF_MAP_OK) {
    return NULL;
  }

  while (key != idle_tbl_info->tbl_hdl) {
    msts = pipe_mgr_idle_tbl_map_get_next(
        dev_id,
        &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
        &key,
        (void **)&idle_tbl_info);
    if (msts != BF_MAP_OK) {
      return NULL;
    }
  }
  *tbl_hdl_p = key;
  return idle_tbl_info;
}

idle_tbl_info_t *pipe_mgr_idle_tbl_info_get_first(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t *tbl_hdl_p) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  unsigned long key = 0;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }
  bf_map_sts_t msts;
  msts = pipe_mgr_idle_tbl_map_get_first(
      dev_id,
      &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
      &key,
      (void **)&idle_tbl_info);
  if (msts != BF_MAP_OK) {
    return NULL;
  }
  *tbl_hdl_p = key;

  if (*tbl_hdl_p != idle_tbl_info->tbl_hdl) {
    return pipe_mgr_idle_tbl_info_get_next(dev_id, tbl_hdl_p);
  }
  return idle_tbl_info;
}

static idle_tbl_t *pipe_mgr_idle_tbl_get(idle_tbl_info_t *idle_tbl_info,
                                         bf_dev_pipe_t pipe_id) {
  idle_tbl_t *idle_tbl = NULL;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    PIPE_MGR_DBGCHK(IDLE_TBL_IS_SYMMETRIC(idle_tbl_info) &&
                    IDLE_TBL_IS_POLL_MODE(idle_tbl_info));
    idle_tbl = &idle_tbl_info->idle_tbls[0];
    return idle_tbl;
  } else {
    PIPE_MGR_DBGCHK(!IDLE_TBL_IS_SYMMETRIC(idle_tbl_info) ||
                    IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info));
    int i;
    for (i = 0; i < idle_tbl_info->no_idle_tbls; i++) {
      idle_tbl = &idle_tbl_info->idle_tbls[i];
      if (idle_tbl->pipe_id == pipe_id) {
        return idle_tbl;
      }
    }
  }

  return NULL;
}

idle_tbl_stage_info_t *pipe_mgr_idle_tbl_stage_info_get(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t ipipe,
    uint32_t stage,
    rmt_tbl_hdl_t stage_table_handle) {
  bf_dev_pipe_t pipe = ipipe;

  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info) &&
      IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    pipe = BF_DEV_PIPE_ALL;
  } else {
    /* Asymm poll mode handling */
    if ((!IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) &&
        IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
      pipe_bitmap_t local_pipe_bmp;
      PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
      PIPE_BITMAP_SET(&local_pipe_bmp, pipe);
      pipe_mgr_get_all_pipes_in_scope(pipe,
                                      idle_tbl_info->num_scopes,
                                      &idle_tbl_info->scope_pipe_bmp[0],
                                      &local_pipe_bmp);
      /* Get the lowest pipe in the scope */
      pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, -1);
    }
  }

  idle_tbl_t *idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe);
  if (!idle_tbl) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle table does not exist on pipe %d",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        pipe);
    return NULL;
  }

  int i;
  idle_tbl_stage_info_t *stage_info = NULL;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    stage_info = &idle_tbl->stage_info[i];
    if ((stage_info->stage_id == stage) &&
        (stage_info->stage_table_handle == stage_table_handle)) {
      break;
    }
  }

  if (i == idle_tbl->num_stages) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle table stage %d does not exist on pipe %d",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        stage,
        pipe);
    return NULL;
  }

  return stage_info;
}

/* Get the list of pipes in notify mode */
pipe_status_t pipe_mgr_idle_tbl_notify_pipe_list_get(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *local_pipe_bmp) {
  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    return PIPE_INVALID_ARG;
  }
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_BITMAP_ASSIGN(local_pipe_bmp, &idle_tbl_info->pipe_bmp);
  } else {
    idle_tbl_t *idle_tbl_inst = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (idle_tbl_inst == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    PIPE_BITMAP_ASSIGN(local_pipe_bmp, &idle_tbl_inst->asym_scope_pipe_bmp);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_get_ttl_helper(idle_tbl_info_t *idle_tbl_info,
                                           pipe_mat_ent_hdl_t ent_hdl,
                                           bf_dev_pipe_t pipe_id,
                                           uint32_t stage_id,
                                           rmt_tbl_hdl_t stage_table_handle,
                                           uint32_t *ttl_p,
                                           uint32_t *init_ttl_p) {
  uint32_t ittl = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    *ttl_p = 0;
    if (init_ttl_p) {
      *init_ttl_p = 0;
    }
  }

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    /* We have entries in each pipe */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->dev_cfg.num_pipelines);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      uint32_t sttl = 0;
      rc = pipe_mgr_idle_entry_get_ttl(stage_info, ent_hdl, &sttl, init_ttl_p);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
      if (sttl > ittl) {
        ittl = sttl;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe_id %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    uint32_t sttl = 0;
    rc = pipe_mgr_idle_entry_get_ttl(stage_info, ent_hdl, &sttl, init_ttl_p);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    if (sttl > ittl) {
      ittl = sttl;
    }
  }

  *ttl_p = ittl;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_get_init_ttl_int(
    idle_tbl_info_t *idle_tbl_info,
    pipe_mat_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe_id,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    uint32_t *init_ttl_p) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
    if (!dev_info) {
      LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }

    /* We have entries in each pipe */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_entry_get_init_ttl(stage_info, ent_hdl, init_ttl_p);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }

      /* Just one is enough */
      break;
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe_id %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    rc = pipe_mgr_idle_entry_get_init_ttl(stage_info, ent_hdl, init_ttl_p);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_get_init_ttl(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         pipe_mat_ent_hdl_t ent_hdl,
                                         bf_dev_pipe_t pipe_id,
                                         uint32_t stage_id,
                                         rmt_tbl_hdl_t stage_table_handle,
                                         uint32_t *init_ttl_p) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  return pipe_mgr_idle_get_init_ttl_int(idle_tbl_info,
                                        ent_hdl,
                                        pipe_id,
                                        stage_id,
                                        stage_table_handle,
                                        init_ttl_p);
}

pipe_status_t pipe_mgr_idle_phy_write_line(bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t tbl_hdl,
                                           uint32_t stage,
                                           mem_id_t mem_id,
                                           uint32_t mem_offset) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  bf_dev_pipe_t pipe, phy_pipe;
  uint64_t phy_addr;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint32_t idle_val =
      idle_tbl_info->idle_tbls
          ? pipe_mgr_idle_get_boot_val(
                idle_tbl_info->idle_tbls[0].stage_info[0].rmt_params.bit_width)
          : 0x7FF;
  PIPE_BITMAP_ITER(&idle_tbl_info->pipe_bmp, pipe) {
    pipe_mgr_map_pipe_id_log_to_phy(idle_tbl_info->dev_info, pipe, &phy_pipe);
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
    phy_addr = idle_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
        idle_tbl_info->direction,
        phy_pipe,
        stage,
        mem_id,
        mem_offset,
        pipe_mem_type_map_ram);
    lld_subdev_ind_write(dev_id, subdev, phy_addr, 0, idle_val);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t init_map_rams(bf_dev_id_t dev_id,
                                   pipe_mat_tbl_info_t *mat_tbl_info,
                                   pipe_bitmap_t *pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_tbl_info_t *rmt_info = NULL;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  uint32_t i = 0, j = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  /* Convert the pipe bitmap to a simple bitmap. */
  unsigned int pipe_bitmap = 0;
  PIPE_BITMAP_ITER(pipe_bmp, i) { pipe_bitmap |= 1 << i; }

  /* Loop over all stages the table is in. */
  for (i = 0; i < mat_tbl_info->num_rmt_info; i++) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }

    uint8_t stage_id = rmt_info->stage_id;
    uint32_t idle_val =
        pipe_mgr_idle_get_boot_val(rmt_info->params.idle.bit_width);
    uint8_t map_ram_data[4] = {idle_val & 0xFF, (idle_val >> 8) & 0xFF, 0, 0};

    /* Loop over all map rams the table uses. */
    for (j = 0; j < rmt_info->bank_map[0].num_tbl_word_blks; j++) {
      mem_id_t mem_id = rmt_info->bank_map[0].tbl_word_blk[j].mem_id[0];
      uint64_t map_phy_addr = dev_cfg->get_full_phy_addr(
          rmt_info->direction, 0, stage_id, mem_id, 0, pipe_mem_type_map_ram);
      rc = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                    dev_info,
                                    4,
                                    TOF_MAP_RAM_UNIT_DEPTH,
                                    1,
                                    map_phy_addr,
                                    pipe_bitmap,
                                    map_ram_data);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("Failed to write idle map rams, dev %d status %s",
                  dev_id,
                  pipe_str_err(rc));
        return rc;
      }
    }
  }

  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_idle_tbl_create
 *         Create the local structures to hold data about a new idle tbl
 *
 * This routine creates the local data structures needed for idle tbl
 * management. It creates a structure and adds it into the global hash tbl
 *
 * \param dev_id Device ID
 * \param tbl_hdl Table hdl of the idle tbl
 * \return pipe_status_t The status of the operation
 */
pipe_status_t pipe_mgr_idle_tbl_create(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  rmt_dev_info_t *dev_info = NULL;
  rmt_tbl_info_t *rmt_info = NULL;
  uint32_t i = 0, q = 0;

  dev_info = pipe_mgr_get_dev_info(dev_id);

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table %d not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Do some sanity */
  if (mat_tbl_info->size == 0) {
    LOG_ERROR("%s: Request to create Idle time tbl 0x%x with %d entries",
              __func__,
              tbl_hdl,
              mat_tbl_info->size);
    return PIPE_INVALID_ARG;
  }

  uint8_t num_stages = 0;
  for (i = 0; i < mat_tbl_info->num_rmt_info; i++) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }
    num_stages++;
  }

  if (!num_stages) {
    /* No idle time attached here */
    return PIPE_SUCCESS;
  }

  if (pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl)) {
    /* Table already exists, error out */
    LOG_ERROR("%s:%d Table 0x%x on device %d already exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  if (!pipe_mgr_is_device_virtual(dev_id) &&
      !pipe_mgr_is_device_locked(dev_id)) {
    /* Initialize all the map-rams with disabled value */
    rc = init_map_rams(dev_id, mat_tbl_info, pipe_bmp);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error initing hw idle Table 0x%x on device %d rc 0x%x",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_id,
                rc);
      return rc;
    }
  }

  idle_tbl_info_t *idle_tbl_info = NULL;

  idle_tbl_info = (idle_tbl_info_t *)PIPE_MGR_MALLOC(sizeof(idle_tbl_info_t));
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Malloc error", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Init with 0 will also mark table as disabled */
  PIPE_MGR_MEMSET(idle_tbl_info, 0, sizeof(idle_tbl_info_t));

  idle_tbl_info->name = bf_sys_strdup(mat_tbl_info->name);
  idle_tbl_info->direction = mat_tbl_info->direction;
  idle_tbl_info->dev_id = dev_id;
  idle_tbl_info->dev_info = dev_info;
  if (idle_tbl_info->dev_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  idle_tbl_info->tbl_hdl = tbl_hdl;
  idle_tbl_info->match_type = mat_tbl_info->match_type;
  idle_tbl_info->tbl_params.mode = POLL_MODE;
  if (idle_tbl_info->match_type == ALPM_MATCH) {
    idle_tbl_info->ll_tbl_hdls = &(mat_tbl_info->alpm_info->atcam_handle);
    idle_tbl_info->num_ll_tbls = 1;
  }
  idle_tbl_info->profile_id = profile_id;
  PIPE_BITMAP_INIT(&idle_tbl_info->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&idle_tbl_info->pipe_bmp, pipe_bmp);
  idle_tbl_info->pipe_count = PIPE_BITMAP_COUNT(&idle_tbl_info->pipe_bmp);

  idle_tbl_info->stage_count = idle_tbl_info->pipe_count * num_stages;

  idle_tbl_info->is_symmetric = mat_tbl_info->symmetric;
  idle_tbl_info->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!idle_tbl_info->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (idle_tbl_info->is_symmetric) {
    idle_tbl_info->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      idle_tbl_info->scope_pipe_bmp[0] |= (1 << q);
    }
  } else {
    idle_tbl_info->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      idle_tbl_info->scope_pipe_bmp[q] |= (1 << q);
      idle_tbl_info->num_scopes += 1;
    }
  }

  PIPE_MGR_LOCK_INIT(idle_tbl_info->notif_list_mtx);
  PIPE_MGR_RWLOCK_INIT(idle_tbl_info->en_dis_rw_lock);
  bf_map_init(&idle_tbl_info->notif_list);
  bf_map_init(&idle_tbl_info->notif_list_old);
  bf_map_init(&idle_tbl_info->notif_list_spec_copy);

  bf_map_sts_t msts;
  msts =
      pipe_mgr_idle_tbl_map_add(dev_id,
                                &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
                                tbl_hdl,
                                (void *)idle_tbl_info);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding idle table sts %d",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              msts);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }
  if (idle_tbl_info->match_type == ALPM_MATCH) {
    msts =
        pipe_mgr_idle_tbl_map_add(dev_id,
                                  &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
                                  mat_tbl_info->alpm_info->atcam_handle,
                                  (void *)idle_tbl_info);
    if (msts != BF_MAP_OK) {
      LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding idle table sts %d",
                __func__,
                __LINE__,
                idle_tbl_info->name,
                idle_tbl_info->dev_id,
                idle_tbl_info->tbl_hdl,
                msts);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }
  }

  for (i = 0; i < mat_tbl_info->num_rmt_info; i++) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }
    idle_tbl_info->bit_width = rmt_info->params.idle.bit_width;
    PIPE_MGR_DBGCHK(pipe_mgr_idle_get_hw_mode(rmt_info->params.idle.bit_width));
    int stage = rmt_info->stage_id;
    int stage_table_handle = rmt_info->handle;
    int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
    int num_pipelines = dev_info->num_active_pipes;
    PIPE_MGR_DBGCHK(stage < dev_info->num_active_mau);
    PIPE_MGR_DBGCHK(stage_table_handle < num_lts);
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&idle_tbl_info->pipe_bmp, pipe);
         pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&idle_tbl_info->pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < num_pipelines);
      idle_mgr_dev_ctx(dev_id)->tbl_lookup[pipe][stage][stage_table_handle] =
          idle_tbl_info;
    }
  }

  /* Figure out the total no-words for this table */
  int j;
  int no_words = 0;
  for (j = 0; j < (int)mat_tbl_info->num_rmt_info; j++) {
    rmt_info = &mat_tbl_info->rmt_info[j];
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }
    no_words += 1 + ((rmt_info->num_entries - 1) /
                     rmt_info->pack_format.entries_per_tbl_word);
  }
  rc = pipe_mgr_idle_tbl_allocate(idle_tbl_info);
  if (rc) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error allocating idle table sts %d",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              msts);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }
  if (!pipe_mgr_is_device_virtual(idle_tbl_info->dev_id)) {
    table_params_init(&idle_tbl_info->idle_tbls[0]);
  }

  return PIPE_SUCCESS;
cleanup:
  pipe_mgr_idle_tbl_info_destroy(idle_tbl_info);
  return rc;
}

/** \brief pipe_mgr_idle_tbl_delete
 *         Deletes the local structures that hold data about a idle tbl
 *
 * This routine deletes the local data structures needed for idle tbl
 * management. It removes it from the global hash tbl and frees all the
 * memory allocated
 *
 * \param tbl_hdl Table hdl of the idle tbl
 * \return pipe_status_t The status of the operation
 */

pipe_status_t pipe_mgr_idle_tbl_delete(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table %d not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);

  if (!idle_tbl_info) {
    LOG_TRACE("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    /* First disable the sweep timer */
    rc = pipe_mgr_idle_disable_sweep(idle_tbl_info, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error disabling sweepers"
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }
  rc = pipe_mgr_idle_tbl_discard(idle_tbl_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) Error discarding idle_tbls"
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  int pipe, stage, stage_table_handle;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  int num_stages = dev_info->num_active_mau;
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (pipe = 0; pipe < (int)dev_info->num_active_pipes; pipe++) {
    for (stage = 0; stage < num_stages; stage++) {
      for (stage_table_handle = 0; stage_table_handle < num_lts;
           stage_table_handle++) {
        if (idle_mgr_dev_ctx(dev_id)
                ->tbl_lookup[pipe][stage][stage_table_handle] ==
            idle_tbl_info) {
          idle_mgr_dev_ctx(dev_id)
              ->tbl_lookup[pipe][stage][stage_table_handle] = NULL;
        }
      }
    }
  }

  bf_map_sts_t msts;
  msts = pipe_mgr_idle_tbl_map_rmv(
      dev_id, &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map, tbl_hdl);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting idle table sts %d",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              msts);
    return PIPE_UNEXPECTED;
  }
  if (idle_tbl_info->match_type == ALPM_MATCH) {
    msts =
        pipe_mgr_idle_tbl_map_rmv(dev_id,
                                  &idle_mgr_dev_ctx(dev_id)->tbl_hdl_to_tbl_map,
                                  mat_tbl_info->alpm_info->atcam_handle);
    if (msts != BF_MAP_OK) {
      LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting idle table sts %d",
                __func__,
                __LINE__,
                idle_tbl_info->name,
                idle_tbl_info->dev_id,
                idle_tbl_info->tbl_hdl,
                msts);
      return PIPE_UNEXPECTED;
    }
  }

  pipe_mgr_idle_tbl_info_destroy(idle_tbl_info);

  return PIPE_SUCCESS;
}

static uint32_t get_movereg_ctl_addr_tof(bf_dev_pipe_t phy_pipe,
                                         int stage_id,
                                         int reg_idx) {
  return offsetof(Tofino,
                  pipes[phy_pipe]
                      .mau[stage_id]
                      .rams.match.adrdist.movereg_idle_pop_ctl[reg_idx]);
}
static uint32_t get_movereg_ctl_addr_tof2(bf_dev_pipe_t phy_pipe,
                                          int stage_id,
                                          int reg_idx) {
  return offsetof(tof2_reg,
                  pipes[phy_pipe]
                      .mau[stage_id]
                      .rams.match.adrdist.movereg_idle_pop_ctl[reg_idx]);
}

static uint32_t get_movereg_ctl_addr_tof3(bf_dev_pipe_t phy_pipe,
                                          int stage_id,
                                          int reg_idx) {
  return offsetof(tof3_reg,
                  pipes[phy_pipe]
                      .mau[stage_id]
                      .rams.match.adrdist.movereg_idle_pop_ctl[reg_idx]);
}
static pipe_status_t pipe_mgr_idle_update_movereg_ctl_tof(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe_id,
    bf_dev_pipe_t phy_pipe_id,
    uint32_t stage_id,
    uint32_t stage_table_handle,
    pipe_idle_time_mode_e mode,
    rmt_idle_time_tbl_params_t rmt_params) {
  uint32_t pop_ctl_mode;
  uint32_t pop_ctl_2way;
  uint32_t pop_ctl_pfe;

  if (mode == POLL_MODE) {
    pop_ctl_mode = 0;
    pop_ctl_2way = 0;
    pop_ctl_pfe = 0;
  } else {
    pop_ctl_mode = 1;
    pop_ctl_2way = rmt_params.two_way_notify_enable ? 1 : 0;
    pop_ctl_pfe = rmt_params.per_flow_enable ? 1 : 0;
  }

  uint32_t val = (pop_ctl_pfe << 2) | (pop_ctl_2way << 1) | (pop_ctl_mode << 0);
  uint32_t mask = 0x7;

  val <<= (stage_table_handle % 8) * 3;
  mask <<= (stage_table_handle % 8) * 3;

  uint32_t reg_idx = stage_table_handle / 8;
  PIPE_MGR_DBGCHK(reg_idx < PIPE_MGR_NUM_IDLE_MOVEREG_REGS);
  uint32_t movereg;

  PIPE_MGR_LOCK(&idle_mgr_dev_ctx(dev_id)
                     ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);

  movereg =
      idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx];
  movereg = (movereg & ~mask) | (val & mask);
  idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx] =
      movereg;

  if (!pipe_mgr_is_device_locked(dev_id)) {
    /* Use lld_write_register here incase there are two sessions (threads)
     * configuring two different tables which happen to be in the same stage.
     * Since this register is shared between all tables in a stage don't let
     * it be updated with an instruction-list since that can go in any order
     * across multiple sessions. */
    uint32_t reg_addr;
    reg_addr = get_movereg_ctl_addr_tof(phy_pipe_id, stage_id, reg_idx);
    int status = lld_write_register(dev_id, reg_addr, movereg);
    if (status != LLD_OK) {
      PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                           ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
      PIPE_MGR_DBGCHK(0);
      return PIPE_COMM_FAIL;
    }
  }

  PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                       ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_update_movereg_ctl_tof3(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe_id,
    bf_dev_pipe_t phy_pipe_id,
    uint32_t stage_id,
    uint32_t stage_table_handle,
    pipe_idle_time_mode_e mode,
    rmt_idle_time_tbl_params_t rmt_params) {
  uint32_t pop_ctl_mode;
  uint32_t pop_ctl_2way;
  uint32_t pop_ctl_pfe;

  if (mode == POLL_MODE) {
    pop_ctl_mode = 0;
    pop_ctl_2way = 0;
    pop_ctl_pfe = 0;
  } else {
    pop_ctl_mode = 1;
    pop_ctl_2way = rmt_params.two_way_notify_enable ? 1 : 0;
    pop_ctl_pfe = rmt_params.per_flow_enable ? 1 : 0;
  }

  uint32_t val = (pop_ctl_pfe << 2) | (pop_ctl_2way << 1) | (pop_ctl_mode << 0);
  uint32_t mask = 0x7;

  val <<= (stage_table_handle % 8) * 3;
  mask <<= (stage_table_handle % 8) * 3;

  uint32_t reg_idx = stage_table_handle / 8;
  PIPE_MGR_ASSERT(reg_idx < PIPE_MGR_NUM_IDLE_MOVEREG_REGS);
  uint32_t movereg;

  PIPE_MGR_LOCK(&idle_mgr_dev_ctx(dev_id)
                     ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);

  movereg =
      idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx];
  movereg = (movereg & ~mask) | (val & mask);
  idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx] =
      movereg;

  if (!pipe_mgr_is_device_locked(dev_id)) {
    /* Use lld_subdev_write_register here incase there are two sessions
     * (threads)
     * configuring two different tables which happen to be in the same stage.
     * Since this register is shared between all tables in a stage don't let
     * it be updated with an instruction-list since that can go in any order
     * across multiple sessions. */
    uint32_t reg_addr;
    reg_addr = get_movereg_ctl_addr_tof3(
        phy_pipe_id % BF_SUBDEV_PIPE_COUNT, stage_id, reg_idx);
    int status = lld_subdev_write_register(
        dev_id, phy_pipe_id / BF_SUBDEV_PIPE_COUNT, reg_addr, movereg);
    if (status != LLD_OK) {
      PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                           ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
      PIPE_MGR_DBGCHK(0);
      return PIPE_COMM_FAIL;
    }
  }

  PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                       ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_update_movereg_ctl_tof2(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe_id,
    bf_dev_pipe_t phy_pipe_id,
    uint32_t stage_id,
    uint32_t stage_table_handle,
    pipe_idle_time_mode_e mode,
    rmt_idle_time_tbl_params_t rmt_params) {
  uint32_t pop_ctl_mode;
  uint32_t pop_ctl_2way;
  uint32_t pop_ctl_pfe;

  if (mode == POLL_MODE) {
    pop_ctl_mode = 0;
    pop_ctl_2way = 0;
    pop_ctl_pfe = 0;
  } else {
    pop_ctl_mode = 1;
    pop_ctl_2way = rmt_params.two_way_notify_enable ? 1 : 0;
    pop_ctl_pfe = rmt_params.per_flow_enable ? 1 : 0;
  }

  uint32_t val = (pop_ctl_pfe << 2) | (pop_ctl_2way << 1) | (pop_ctl_mode << 0);
  uint32_t mask = 0x7;

  val <<= (stage_table_handle % 8) * 3;
  mask <<= (stage_table_handle % 8) * 3;

  uint32_t reg_idx = stage_table_handle / 8;
  PIPE_MGR_DBGCHK(reg_idx < PIPE_MGR_NUM_IDLE_MOVEREG_REGS);
  uint32_t movereg;

  PIPE_MGR_LOCK(&idle_mgr_dev_ctx(dev_id)
                     ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);

  movereg =
      idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx];
  movereg = (movereg & ~mask) | (val & mask);
  idle_mgr_dev_ctx(dev_id)->movereg_ctl[log_pipe_id][stage_id][reg_idx] =
      movereg;

  if (!pipe_mgr_is_device_locked(dev_id)) {
    /* Use lld_write_register here incase there are two sessions (threads)
     * configuring two different tables which happen to be in the same stage.
     * Since this register is shared between all tables in a stage don't let
     * it be updated with an instruction-list since that can go in any order
     * across multiple sessions. */
    uint32_t reg_addr;
    reg_addr = get_movereg_ctl_addr_tof2(phy_pipe_id, stage_id, reg_idx);
    int status = lld_write_register(dev_id, reg_addr, movereg);
    if (status != LLD_OK) {
      PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                           ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
      PIPE_MGR_DBGCHK(0);
      return PIPE_COMM_FAIL;
    }
  }

  PIPE_MGR_UNLOCK(&idle_mgr_dev_ctx(dev_id)
                       ->movereg_ctl_mtx[log_pipe_id][stage_id][reg_idx]);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_write_movereg_ctl(
    idle_tbl_info_t *idle_tbl_info) {
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_t *idle_tbl = NULL;
  idle_tbl = &idle_tbl_info->idle_tbls[0];

  int i;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    bf_dev_pipe_t pipe_id;
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];
    PIPE_BITMAP_ITER(&idle_tbl_info->pipe_bmp, pipe_id) {
      bf_dev_pipe_t phy_pipe_id;
      rc = pipe_mgr_map_pipe_id_log_to_phy(
          idle_tbl_info->dev_info, pipe_id, &phy_pipe_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Error in getting physical pipe-id for logical pipe %d, dev %d, "
            "error %s",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            pipe_id,
            idle_tbl_info->dev_id,
            pipe_str_err(rc));
        PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
        return rc;
      }

      switch (idle_tbl_info->dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          rc = pipe_mgr_idle_update_movereg_ctl_tof(
              idle_tbl_info->dev_id,
              pipe_id,
              phy_pipe_id,
              stage_info->stage_id,
              stage_info->stage_table_handle,
              idle_tbl_info->tbl_params.mode,
              stage_info->rmt_params);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          rc = pipe_mgr_idle_update_movereg_ctl_tof2(
              idle_tbl_info->dev_id,
              pipe_id,
              phy_pipe_id,
              stage_info->stage_id,
              stage_info->stage_table_handle,
              idle_tbl_info->tbl_params.mode,
              stage_info->rmt_params);
          break;
        case BF_DEV_FAMILY_TOFINO3:
          rc = pipe_mgr_idle_update_movereg_ctl_tof3(
              idle_tbl_info->dev_id,
              pipe_id,
              phy_pipe_id,
              stage_info->stage_id,
              stage_info->stage_table_handle,
              idle_tbl_info->tbl_params.mode,
              stage_info->rmt_params);
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          rc = PIPE_UNEXPECTED;
      }

      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Error updating movereg pop ctl register pipe %d stage %d rc %s",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            pipe_id,
            stage_info->stage_id,
            pipe_str_err(rc));
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}

static void prepare_sweep_ctrl_data_tof(int stage_id,
                                        int log_tbl_id,
                                        bool enable,
                                        uint32_t max_vpn,
                                        uint32_t sweep_interval,
                                        uint32_t *addr,
                                        uint32_t *data) {
  setp_idletime_sweep_ctl_idletime_sweep_en(data, enable ? 1 : 0);
  setp_idletime_sweep_ctl_idletime_sweep_size(data, max_vpn);
  if (enable) {
    setp_idletime_sweep_ctl_idletime_sweep_interval(data, sweep_interval);
  }

  *addr = offsetof(
      Tofino,
      pipes[0].mau[stage_id].rams.match.adrdist.idletime_sweep_ctl[log_tbl_id]);
}

static void prepare_sweep_ctrl_data_tof2(int stage_id,
                                         int log_tbl_id,
                                         bool enable,
                                         uint32_t max_vpn,
                                         uint32_t sweep_interval,
                                         uint32_t *addr,
                                         uint32_t *data) {
  setp_tof2_idletime_sweep_ctl_idletime_sweep_en(data, enable ? 1 : 0);
  setp_tof2_idletime_sweep_ctl_idletime_sweep_size(data, max_vpn);
  if (enable) {
    setp_tof2_idletime_sweep_ctl_idletime_sweep_interval(data, sweep_interval);
  }

  *addr = offsetof(
      tof2_reg,
      pipes[0].mau[stage_id].rams.match.adrdist.idletime_sweep_ctl[log_tbl_id]);
}

static void prepare_sweep_ctrl_data_tof3(int stage_id,
                                         int log_tbl_id,
                                         bool enable,
                                         uint32_t max_vpn,
                                         uint32_t sweep_interval,
                                         uint32_t *addr,
                                         uint32_t *data) {
  setp_tof3_idletime_sweep_ctl_idletime_sweep_en(data, enable ? 1 : 0);
  setp_tof3_idletime_sweep_ctl_idletime_sweep_size(data, max_vpn);
  if (enable) {
    setp_tof3_idletime_sweep_ctl_idletime_sweep_interval(data, sweep_interval);
  }

  *addr = offsetof(
      tof3_reg,
      pipes[0].mau[stage_id].rams.match.adrdist.idletime_sweep_ctl[log_tbl_id]);
}

static pipe_status_t pipe_mgr_idle_write_sweep_ctl(
    idle_tbl_info_t *idle_tbl_info, bool enable) {
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_t *idle_tbl = NULL;
  idle_tbl = &idle_tbl_info->idle_tbls[0];

  int i;
  for (i = 0; i < idle_tbl->num_stages; i++) {
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[i];
    pipe_bitmap_t pipe_bmp;

    PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
    /* The sweep ctl is programmed same on all pipes */
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(idle_tbl_info->pipe_bmp));

    uint32_t sweep_reg_val = 0, reg_addr = 0;
    switch (idle_tbl_info->dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        prepare_sweep_ctrl_data_tof(stage_info->stage_id,
                                    stage_info->stage_table_handle,
                                    enable,
                                    stage_info->hw_info.max_vpn,
                                    stage_info->hw_sweep_interval,
                                    &reg_addr,
                                    &sweep_reg_val);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        prepare_sweep_ctrl_data_tof2(stage_info->stage_id,
                                     stage_info->stage_table_handle,
                                     enable,
                                     stage_info->hw_info.max_vpn,
                                     stage_info->hw_sweep_interval,
                                     &reg_addr,
                                     &sweep_reg_val);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        prepare_sweep_ctrl_data_tof3(stage_info->stage_id,
                                     stage_info->stage_table_handle,
                                     enable,
                                     stage_info->hw_info.max_vpn,
                                     stage_info->hw_sweep_interval,
                                     &reg_addr,
                                     &sweep_reg_val);
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }

    /* Program the hw_sweep_interval in HW */
    pipe_instr_write_reg_i_only_t instr;
    construct_instr_reg_write_no_data(idle_tbl_info->dev_id, &instr, reg_addr);

    rc = pipe_mgr_drv_ilist_add_2(&idle_tbl->cur_sess_hdl,
                                  idle_tbl_info->dev_info,
                                  &pipe_bmp,
                                  stage_info->stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_write_reg_i_only_t),
                                  (uint8_t *)&sweep_reg_val,
                                  sizeof(sweep_reg_val));
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) "
          "Error adding instruction for sweep interval rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_write_init_val(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t pipe_id,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    uint32_t index,
    idle_hw_state_e hw_state) {
  if (pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info)) {
    /* During fast-reconfig, we need not write the init-val.
     * At the end of fast-reconfig, we need to a dump of
     * current state as we know it.
     */
    return PIPE_SUCCESS;
  }
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_stage_info_t *stage_info = NULL;

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  stage_info = pipe_mgr_idle_tbl_stage_info_get(
      idle_tbl_info, idle_tbl->pipe_id, stage_id, stage_table_handle);
  if (stage_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Write the active value into the entry */
  pipe_instr_set_memdata_v_i_only_t v_wr;
  int8_t val = pipe_mgr_idle_get_entry_init_val(
      idle_tbl_info, stage_info->rmt_params, hw_state);
  PIPE_MGR_DBGCHK(val != -1);

  uint32_t idle_val = (uint32_t)val;
  /* Shift the value based on the subword position */
  int subword = index % stage_info->entries_per_word;
  int word = index / stage_info->entries_per_word;
  PIPE_MGR_DBGCHK(word < stage_info->no_words);
  int i;
  for (i = 0; i < subword; i++) {
    idle_val <<= stage_info->rmt_params.bit_width;
  }

  uint32_t no_subword_bits = log2_uint32_ceil(stage_info->entries_per_word);
  PIPE_MGR_DBGCHK(no_subword_bits < TOF_IDLE_SUBWORD_VPN_BITS);
  uint32_t no_huffman_bits = TOF_IDLE_SUBWORD_VPN_BITS - no_subword_bits;
  uint32_t huffman_bits = (1 << (no_huffman_bits - 1)) - 1;
  uint32_t virt_addr = (index << no_huffman_bits) | huffman_bits;

  construct_instr_set_v_memdata_no_data(idle_tbl_info->dev_id,
                                        &v_wr,
                                        sizeof(idle_val),
                                        stage_info->stage_table_handle,
                                        pipe_virt_mem_type_idle,
                                        virt_addr);

  pipe_bitmap_t pipe_bmp;

  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  } else {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &idle_tbl->asym_scope_pipe_bmp);
    PIPE_BITMAP_AND(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  }

  rc = pipe_mgr_drv_ilist_add_2(&idle_tbl->cur_sess_hdl,
                                idle_tbl_info->dev_info,
                                &pipe_bmp,
                                stage_info->stage_id,
                                (uint8_t *)&v_wr,
                                sizeof(pipe_instr_set_memdata_v_i_only_t),
                                (uint8_t *)&idle_val,
                                sizeof(idle_val));
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) "
        "Error adding instruction for adding idle entry rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_send_dump_tbl(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t pipe_id,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_stage_info_t *stage_info = NULL;

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  stage_info = pipe_mgr_idle_tbl_stage_info_get(
      idle_tbl_info, idle_tbl->pipe_id, stage_id, stage_table_handle);
  if (stage_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_bitmap_t pipe_bmp;
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  } else {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &idle_tbl->asym_scope_pipe_bmp);
    PIPE_BITMAP_AND(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  }

  bool clear = true;

  pipe_dump_idle_table_instr_t dump_tbl_instr;
  construct_instr_idle_dump_tbl(
      idle_tbl_info->dev_id, &dump_tbl_instr, stage_table_handle, clear);

  rc = pipe_mgr_drv_ilist_add_2(&idle_tbl->cur_sess_hdl,
                                idle_tbl_info->dev_info,
                                &pipe_bmp,
                                stage_info->stage_id,
                                (uint8_t *)&dump_tbl_instr,
                                sizeof(dump_tbl_instr),
                                NULL,
                                0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time error adding dump table instruction rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_send_barrier(
    idle_tbl_info_t *idle_tbl_info,
    bf_dev_pipe_t pipe_id,
    uint32_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    lock_id_t lock_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_stage_info_t *stage_info = NULL;

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  stage_info = pipe_mgr_idle_tbl_stage_info_get(
      idle_tbl_info, idle_tbl->pipe_id, stage_id, stage_table_handle);
  if (stage_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_bitmap_t pipe_bmp;
  PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  } else {
    PIPE_BITMAP_ASSIGN(&pipe_bmp, &idle_tbl->asym_scope_pipe_bmp);
    PIPE_BITMAP_AND(&pipe_bmp, &(idle_tbl_info->pipe_bmp));
  }

  pipe_barrier_lock_instr_t barrier_instr;
  construct_instr_barrier_idle(
      idle_tbl_info->dev_id, &barrier_instr, lock_id, stage_table_handle);

  /* Send a barrier instruction */
  rc = pipe_mgr_drv_ilist_add_2(&idle_tbl->cur_sess_hdl,
                                idle_tbl_info->dev_info,
                                &pipe_bmp,
                                stage_info->stage_id,
                                (uint8_t *)&barrier_instr,
                                sizeof(barrier_instr),
                                NULL,
                                0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time error adding dump table instruction rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_update_state_for_add_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t index,
    uint32_t ttl,
    uint32_t cur_ttl) {
  pipe_status_t rc = PIPE_SUCCESS;
  /* Add an entry for the mdata table */
  rc = pipe_mgr_idle_entry_add_mdata(stage_info, ent_hdl, index, ttl, cur_ttl);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  return rc;
}

static pipe_status_t pipe_mgr_idle_update_state_for_del_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t del_index) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (!pipe_mgr_idle_entry_mdata_exists(stage_info, ent_hdl)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Delete the mdata entry*/
  rc = pipe_mgr_idle_entry_del_mdata(stage_info, ent_hdl, del_index);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  return rc;
}

static pipe_status_t pipe_mgr_idle_update_state_for_move_entry(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t src_index,
    uint32_t dest_index) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (!pipe_mgr_idle_entry_mdata_exists(stage_info, ent_hdl)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Delete the mdata entry*/
  rc = pipe_mgr_idle_entry_move_mdata(
      stage_info, ent_hdl, src_index, dest_index);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_update_state_for_update_ttl(
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t ttl) {
  pipe_status_t rc = PIPE_SUCCESS;

  if (!pipe_mgr_idle_entry_mdata_exists(stage_info, ent_hdl)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  rc = pipe_mgr_idle_entry_set_mdata_ttl_dirty(stage_info, ent_hdl, ttl);
  PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_set_ttl_in_stage(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t ttl,
    bool reset) {
  if (IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    /* Nothing to do for poll mode */
    return PIPE_SUCCESS;
  }

  idle_task_node_t tnode;
  PIPE_MGR_MEMSET(&tnode, 0, sizeof(idle_task_node_t));

  tnode.ent_hdl = ent_hdl;
  if (reset) {
    tnode.type = IDLE_ENTRY_RESET_TTL;
  } else {
    tnode.type = IDLE_ENTRY_UPDATE_TTL;
  }
  tnode.u.update.ttl = ttl;

  pipe_status_t rc = PIPE_SUCCESS;
  rc = pipe_mgr_idle_append_task(
      idle_tbl_info, stage_info, &tnode, true, false, 0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error appending set-ttl task for "
        "stage %d rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        stage_info->stage_id,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

/* Configure idle timeout at table level */
pipe_status_t rmt_idle_params_get(bf_dev_id_t device_id,
                                  pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                  pipe_idle_time_params_t *params) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_mat_tbl_hdl_t idle_tbl_hdl = mat_tbl_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  if (!params) {
    LOG_ERROR("%s:%d Params passed is NULL", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(device_id, idle_tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *params = idle_tbl_info->tbl_params;
  return rc;
}

pipe_status_t rmt_idle_params_set(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t device_id,
                                  pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                  pipe_idle_time_params_t params) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(device_id, mat_tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Sanitize the input */
  switch (params.mode) {
    case POLL_MODE:
      /* In poll mode both pointers must be set to null in order to avoid race
       * conditions. Safe to ignore actual input. */
      params.u.notify.callback_fn = NULL;
      params.u.notify.callback_fn2 = NULL;
      break;
    case NOTIFY_MODE:
      if (idle_tbl_info->bit_width == 1) {
        // One bit idle tables do not support notify mode
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Idle notify mode is not supported with 1-bit idle tables",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl);
        return PIPE_INVALID_ARG;
      }
      if (!params.u.notify.ttl_query_interval ||
          /* Allow two kind of callback to be set at the same time*/
          (((!params.u.notify.callback_fn &&
             params.u.notify.default_callback_choice == 0) ||
            (!params.u.notify.callback_fn2 &&
             params.u.notify.default_callback_choice == 1)) &&
           !idle_tbl_info->in_state_restore)) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Invalid idle time notify mode params "
            "query interval %d, max_ttl %d min_ttl %d "
            "callback_fn %s "
            "callback_fn2 %s "
            "callback_choice %s ",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            params.u.notify.ttl_query_interval,
            params.u.notify.max_ttl,
            params.u.notify.min_ttl,
            params.u.notify.callback_fn ? "SET" : "CLR",
            params.u.notify.callback_fn2 ? "SET" : "CLR",
            params.u.notify.default_callback_choice ? "fn2" : "fn");
        return PIPE_INVALID_ARG;
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) "
          "Invalid idle time mode %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          params.mode);
      return PIPE_INVALID_ARG;
  }

  /* Backup the old config, will be used in case of an error. */
  pipe_idle_time_params_t params_bkp = idle_tbl_info->tbl_params;
  idle_tbl_info->tbl_params = params;
  idle_tbl_info->tbl_params.u.notify.min_ttl = params_bkp.u.notify.min_ttl;
  idle_tbl_info->tbl_params.u.notify.max_ttl = params_bkp.u.notify.max_ttl;

  if (params_bkp.mode != params.mode) {
    if (!pipe_mgr_hitless_warm_init_in_progress(device_id)) {
      if (is_table_populated(device_id, mat_tbl_hdl)) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Idle table mode cannot be changed when mat-entries are present",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl);
        idle_tbl_info->tbl_params = params_bkp;
        return PIPE_NOT_SUPPORTED;
      }
    }

    bool enabled = idle_tbl_info->enabled;
    if (enabled) {
      rc = rmt_idle_tmo_disable(sess_hdl, device_id, mat_tbl_hdl);
      if (rc) {
        /* Restoring is safe, because no changes were made */
        idle_tbl_info->tbl_params = params_bkp;
        return rc;
      }
    }

    if (idle_tbl_info->idle_tbls) {
      pipe_mgr_idle_tbl_destroy(idle_tbl_info->no_idle_tbls,
                                &idle_tbl_info->idle_tbls);
      idle_tbl_info->no_idle_tbls = 0;
    }

    rc = pipe_mgr_idle_tbl_allocate(idle_tbl_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error allocating idle_tbls "
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      /* This is unrecoverable error, without calling this function again.
       * Set mode to invalid so it is reflected, block enabling and adding
       * entries. */
      idle_tbl_info->tbl_params.mode = INVALID_MODE;
      return rc;
    }

    if (enabled) {
      rc = rmt_idle_tmo_enable(sess_hdl, device_id, mat_tbl_hdl);
      if (rc) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Failed to apply new idle configuration for mode %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            params.mode);
        idle_tbl_info->tbl_params.mode = INVALID_MODE;
        return rc;
      }
    }
  } else if (idle_tbl_info->enabled && params.mode == NOTIFY_MODE &&
             params.u.notify.ttl_query_interval !=
                 params_bkp.u.notify.ttl_query_interval) {
    rc = rmt_idle_tmo_disable(sess_hdl, device_id, mat_tbl_hdl);
    if (rc) {
      /* Restoring is safe, because no changes were made */
      idle_tbl_info->tbl_params = params_bkp;
      return rc;
    }

    rc = rmt_idle_tmo_enable(sess_hdl, device_id, mat_tbl_hdl);
    if (rc) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) "
          "Failed to apply new idle configuration for mode %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          params.mode);

      /* If enabling failed, retry with original params.
       * This is done in case new query interval value is not supported.*/
      idle_tbl_info->tbl_params = params_bkp;
      if (rmt_idle_tmo_enable(sess_hdl, device_id, mat_tbl_hdl)) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) "
            "Failed to restore previous configuration %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            params_bkp.mode);
      }
    }
  }

  return rc;
}

pipe_status_t rmt_idle_tmo_enable(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_mat_tbl_hdl_t idle_tbl_hdl = tbl_hdl;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->tbl_params.mode == INVALID_MODE) {
    LOG_ERROR("%s:%d Enabling timeout with invalid mode on table 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    idle_tbl_hdl = idle_tbl_info->ll_tbl_hdls[0];
  }

  if (idle_tbl_info->enabled) {
    return PIPE_SUCCESS;
  }

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[0];
    idle_tbl->cur_sess_hdl = sess_hdl;

    rc = pipe_mgr_idle_tbl_enable_sweep(idle_tbl_info, true);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }
    /* Update table managers so they know to include idle time when locking the
     * table for move-reg updates. */
    rc = pipe_mgr_mat_tbl_update_lock_type(
        dev_id,
        idle_tbl_hdl,
        true,
        false,
        IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info));
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating the lock state for mat tbl "
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      goto cleanup;
    }

    uint32_t idle_init_val_for_ttl_0;
    idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[0];

    if (stage_info->rmt_params.bit_width == 1) {
      idle_init_val_for_ttl_0 = 0;
    } else {
      idle_init_val_for_ttl_0 = 1;
    }

    /* Update the table managers so they have the correct TTL value to
     * program for entries installed with a zero TTL. */
    rc = pipe_mgr_mat_tbl_update_idle_init_val(
        dev_id, idle_tbl_hdl, idle_init_val_for_ttl_0);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating the lock state for mat tbl "
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      goto cleanup;
    }
  }
  idle_tbl_info->enabled = true;

  return PIPE_SUCCESS;
cleanup:
  return rc;
}

pipe_status_t rmt_idle_register_tmo_cb(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_idle_tmo_expiry_cb callback_fn,
                                       void *client_data) {
  idle_tbl_info_t *idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_ERROR("%s:%d Tmo callback only available for notify mode",
              __func__,
              __LINE__);
    return PIPE_NOT_SUPPORTED;
  }

  idle_tbl_info->tbl_params.u.notify.default_callback_choice = 0;
  idle_tbl_info->tbl_params.u.notify.callback_fn = callback_fn;
  idle_tbl_info->tbl_params.u.notify.client_data = client_data;

  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_register_tmo_cb_with_match_spec_copy(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy callback_fn2,
    void *client_data) {
  idle_tbl_info_t *idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_ERROR("%s:%d Tmo callback only available for notify mode",
              __func__,
              __LINE__);
    return PIPE_NOT_SUPPORTED;
  }

  idle_tbl_info->tbl_params.u.notify.default_callback_choice = 1;
  idle_tbl_info->tbl_params.u.notify.callback_fn2 = callback_fn2;
  idle_tbl_info->tbl_params.u.notify.client_data = client_data;

  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_tmo_enable_get(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      bool *enable) {
  idle_tbl_info_t *idle_tbl_info = NULL;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *enable = idle_tbl_info->enabled;
  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_tmo_disable(pipe_sess_hdl_t sess_hdl,
                                   bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t tbl_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_mat_tbl_hdl_t idle_tbl_hdl = tbl_hdl;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    idle_tbl_hdl = idle_tbl_info->ll_tbl_hdls[0];
  }

  /* If table is disabled just return success. */
  if (!idle_tbl_info->enabled) {
    return PIPE_SUCCESS;
  }
  if (!pipe_mgr_is_device_virtual(dev_id)) {
    /* Do not proceed with the disable until all the existing msgs have come
     * back
     */
    rc = pipe_mgr_idle_drain_all_msgs(idle_tbl_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error draining idle msgs "
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }

    idle_tbl_info->idle_tbls[0].cur_sess_hdl = sess_hdl;

    rc = pipe_mgr_mat_tbl_update_lock_type(
        dev_id, idle_tbl_hdl, true, false, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating the lock state for mat tbl "
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    /* First disable the sweep timer */
    rc = pipe_mgr_idle_disable_sweep(idle_tbl_info, true);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error disabling sweepers"
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }

  idle_tbl_info->enabled = false;

  return PIPE_SUCCESS;
}

/* Set the Idle timeout TTL for a given match entry */
pipe_status_t rmt_mat_ent_set_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_mat_ent_hdl_t ent_hdl,
                                       uint32_t ttl, /*< TTL value in msecs */
                                       uint32_t pipe_api_flags,
                                       bool reset) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_mat_tbl_hdl_t idle_tbl_hdl = tbl_hdl;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    idle_tbl_hdl = idle_tbl_info->ll_tbl_hdls[0];
  }

  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time notify mode not enabled. The API is supported only in "
        "notify mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (ttl != 0 && ttl < idle_tbl_info->tbl_params.u.notify.ttl_query_interval) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Request to set entry ttl with value %d"
        " is less than query interval %d. Entry hdl 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ttl,
        idle_tbl_info->tbl_params.u.notify.ttl_query_interval,
        ent_hdl);
    return PIPE_INVALID_ARG;
  }

  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t index = 0;
  uint32_t old_ttl = 0;
  uint32_t subindex = 0;
  /* The index returned here is only used to write the init-value
   * at the right location. Internally the entry might be in some
   * other index (ient_mdata)
   */
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             &index);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Mat entry 0x%x not found.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_INVALID_ARG;
  }
  for (subindex = 1; rc == PIPE_SUCCESS; subindex++) {
    /* Get the old ttl from the entry itself */
    rc = pipe_mgr_idle_get_init_ttl_int(idle_tbl_info,
                                        ent_hdl,
                                        pipe_id,
                                        stage_id,
                                        stage_table_handle,
                                        &old_ttl);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    idle_tbl_t *idle_tbl = NULL;
    if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
      PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
      idle_tbl = &idle_tbl_info->idle_tbls[0];
    } else {
      PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
      idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
      if (!idle_tbl) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            pipe_id);
        return PIPE_OBJ_NOT_FOUND;
      }
    }

    idle_tbl->cur_sess_hdl = sess_hdl;
    idle_tbl->sess_flags = pipe_api_flags;

    lock_id_t lock_id = 0;
    bool locked = false;

    /* Figure out the type of operation */
    if (!ttl || !old_ttl || reset) {
      /* It's either a new activation or disable.
       * Needs a barrier instruction
       */
      rc = pipe_mgr_mat_tbl_gen_lock_id(
          dev_id, idle_tbl_hdl, LOCK_ID_TYPE_IDLE_BARRIER, &lock_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle time error generating lock_id rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
      /* Send a barrier and write the init value */
      rc = pipe_mgr_idle_send_barrier(
          idle_tbl_info, pipe_id, stage_id, stage_table_handle, lock_id);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }

      rc = pipe_mgr_idle_write_init_val(
          idle_tbl_info,
          pipe_id,
          stage_id,
          stage_table_handle,
          index,
          (ttl == 0) ? HW_IDLE_DISABLE : HW_IDLE_ACTIVE);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }

      rc = rmt_idle_tbl_lock(sess_hdl,
                             dev_id,
                             idle_tbl_hdl,
                             lock_id,
                             pipe_id,
                             stage_id,
                             stage_table_handle,
                             pipe_api_flags);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }

      locked = true;
    }

    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_set_ttl_in_stage(
          idle_tbl_info, stage_info, ent_hdl, ttl, reset);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error setting ttl for entry 0x%x in stage %d pipe %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ent_hdl,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }

    if (locked) {
      rc = rmt_idle_tbl_unlock(sess_hdl,
                               dev_id,
                               idle_tbl_hdl,
                               lock_id,
                               pipe_id,
                               stage_id,
                               stage_table_handle,
                               pipe_api_flags);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }
    rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               subindex,
                                               &pipe_id,
                                               &stage_id,
                                               &stage_table_handle,
                                               &index);
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_mat_ent_reset_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         pipe_mat_ent_hdl_t ent_hdl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t init_ttl = 0;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* No point in reseting TTL on disabled table, because clock is not ticking.
   */
  if (!idle_tbl_info->enabled) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Idle timeouts are not enabled",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time notify mode not enabled. The API is supported only in "
        "notify mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t index = 0;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             0,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             &index);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Mat entry 0x%x not found.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return rc;
  }

  rc = pipe_mgr_idle_get_init_ttl_int(
      idle_tbl_info, ent_hdl, pipe_id, stage_id, stage_table_handle, &init_ttl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to get init ttl for entry %d in table 0x%x",
              __func__,
              __LINE__,
              ent_hdl,
              tbl_hdl);
    return rc;
  }
  if (init_ttl == 0) {
    // Nothing to reset if idletime is disabled for this entry
    return PIPE_SUCCESS;
  }

  return rmt_mat_ent_set_idle_ttl(
      sess_hdl, dev_id, tbl_hdl, ent_hdl, init_ttl, 0, true);
}

/* API function to set hit state data for a table entry. */
pipe_status_t rmt_idle_time_set_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info_t *idle_tbl_info = NULL;

  (void)sess_hdl;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time poll mode not enabled. The API is supported only in poll "
        "mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t subindex = 0;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Mat entry 0x%x not found.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_INVALID_ARG;
  }

  for (subindex = 1; rc == PIPE_SUCCESS; subindex++) {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe_id %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    rc =
        pipe_mgr_idle_entry_set_poll_state(stage_info, ent_hdl, idle_time_data);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error setting the poll state on stage %d pipe %d for ent 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id,
          ent_hdl);
      return rc;
    }
    rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               subindex,
                                               &pipe_id,
                                               &stage_id,
                                               &stage_table_handle,
                                               NULL);
  }
  return PIPE_SUCCESS;
}

/* API function to poll idle timeout data for a table entry */
pipe_status_t rmt_idle_time_get_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info_t *idle_tbl_info = NULL;

  (void)sess_hdl;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time poll mode not enabled. The API is supported only in poll "
        "mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t subindex = 0;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Mat entry 0x%x not found.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_INVALID_ARG;
  }

  *idle_time_data_p = ENTRY_IDLE;

  for (subindex = 1; rc == PIPE_SUCCESS; subindex++) {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe_id %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    rc = pipe_mgr_idle_entry_get_poll_state(
        stage_info, ent_hdl, idle_time_data_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error getting the poll state on stage %d pipe %d for ent 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id,
          ent_hdl);
      return rc;
    }
    rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               subindex,
                                               &pipe_id,
                                               &stage_id,
                                               &stage_table_handle,
                                               NULL);
  }
  return PIPE_SUCCESS;
}

/* API function that should be called
 * periodically or on-demand prior to querying for the hit state
 * The function completes asynchronously and the client will
 * be notified of it's completion via the provided callback function.
 * After fetching hit state HW values will be reset to idle.
 */
pipe_status_t rmt_idle_time_update_hit_state(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_mat_tbl_hdl_t idle_tbl_hdl = tbl_hdl;
  lock_id_t lock_id = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  if (!callback_fn) {
    LOG_ERROR("%s:%d Null idle time update complete callback function passed",
              __func__,
              __LINE__);
    return PIPE_INVALID_ARG;
  }

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    idle_tbl_hdl = idle_tbl_info->ll_tbl_hdls[0];
  }

  if (!idle_tbl_info->enabled) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Idle timeouts are not enabled",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (!IDLE_TBL_IS_POLL_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time poll mode not enabled. The API is supported only in poll "
        "mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (idle_tbl_info->update_in_progress) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time update hit state already in progress",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_IDLE_UPDATE_IN_PROGRESS;
  }

  rc = pipe_mgr_mat_tbl_gen_lock_id(
      dev_id, idle_tbl_hdl, LOCK_ID_TYPE_IDLE_BARRIER, &lock_id);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time error generating lock_id rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  int pipe;
  for (pipe = 0; pipe < idle_tbl_info->no_idle_tbls; pipe++) {
    idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[pipe];
    /* Set up the session handle */
    idle_tbl->cur_sess_hdl = sess_hdl;

    int stage;
    for (stage = 0; stage < idle_tbl->num_stages; stage++) {
      idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[stage];
      rc = pipe_mgr_idle_send_dump_tbl(idle_tbl_info,
                                       idle_tbl->pipe_id,
                                       stage_info->stage_id,
                                       stage_info->stage_table_handle);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
      rc = pipe_mgr_idle_send_barrier(idle_tbl_info,
                                      idle_tbl->pipe_id,
                                      stage_info->stage_id,
                                      stage_info->stage_table_handle,
                                      lock_id);
      if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }
  }

  idle_tbl_info->update_barrier_lock_id = lock_id;
  idle_tbl_info->update_in_progress = true;
  idle_tbl_info->update_complete_cb_fn = callback_fn;
  idle_tbl_info->update_complete_cb_data = cb_data;

  return PIPE_SUCCESS;
}

/* The below APIs are used in notify mode */
/* API function to get the current TTL value of the table entry */
pipe_status_t rmt_mat_ent_get_idle_ttl(pipe_sess_hdl_t sess_hdl,
                                       bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       pipe_mat_ent_hdl_t ent_hdl,
                                       uint32_t *ttl_p) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info_t *idle_tbl_info = NULL;

  (void)sess_hdl;
  *ttl_p = 0;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Idle time notify mode not enabled. The API is supported only in "
        "notify mode",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  rmt_tbl_hdl_t stage_table_handle = 0;
  uint32_t subindex = 0;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                             tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             &pipe_id,
                                             &stage_id,
                                             &stage_table_handle,
                                             NULL);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Mat entry 0x%x not found.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    return PIPE_INVALID_ARG;
  }

  uint32_t local_ttl = 0;
  *ttl_p = local_ttl;
  for (subindex = 1; rc == PIPE_SUCCESS; subindex++) {
    rc = pipe_mgr_idle_get_ttl_helper(idle_tbl_info,
                                      ent_hdl,
                                      pipe_id,
                                      stage_id,
                                      stage_table_handle,
                                      &local_ttl,
                                      NULL);
    if (rc != PIPE_SUCCESS) {
      return rc;
    }
    if (local_ttl > *ttl_p) {
      *ttl_p = local_ttl;
    }
    rc = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                               tbl_hdl,
                                               ent_hdl,
                                               subindex,
                                               &pipe_id,
                                               &stage_id,
                                               &stage_table_handle,
                                               NULL);
  }
  return PIPE_SUCCESS;
}

/* Mat entry APIs - add entry, delete entry, move entry */

static pipe_status_t pipe_mgr_idle_task_update_mdata(
    idle_tbl_stage_info_t *stage_info, idle_task_node_t *tnode) {
  pipe_status_t rc = PIPE_SUCCESS;

  pipe_mat_ent_hdl_t ent_hdl = tnode->ent_hdl;
  switch (tnode->type) {
    case IDLE_ENTRY_UPDATE_TTL: /* fall through */
    case IDLE_ENTRY_RESET_TTL:
      /* Update the ttl_dirty flag in the mdata */
      rc = pipe_mgr_idle_update_state_for_update_ttl(
          stage_info, ent_hdl, tnode->u.update.ttl);
      break;
    case IDLE_ENTRY_DEL:
      /* Mark the entry as being deleted to ensure that the notification
       * callback doesn't get called later
       */
      rc = pipe_mgr_idle_update_state_for_del_entry(
          stage_info, ent_hdl, tnode->u.del.del_idx);
      break;
    case IDLE_ENTRY_MOVE:
      /* Update the dest_index in the entry-mdata */
      rc = pipe_mgr_idle_update_state_for_move_entry(
          stage_info, ent_hdl, tnode->u.move.src_idx, tnode->u.move.dest_idx);
      break;
    case IDLE_ENTRY_ADD:
      rc = pipe_mgr_idle_update_state_for_add_entry(stage_info,
                                                    ent_hdl,
                                                    tnode->u.add.add_idx,
                                                    tnode->u.add.ttl,
                                                    tnode->u.add.cur_ttl);
      break;
    default:
      break;
  }

  if (rc != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
  }
  return rc;
}

static pipe_status_t pipe_mgr_idle_append_task(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    idle_task_node_t *tnode_p,
    bool move_to_front,
    bool lock_valid,
    lock_id_t lock_id) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_task_list_t **dest_list_p = NULL;

  if (pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info) ||
      pipe_mgr_hitless_warm_init_in_progress(idle_tbl_info->dev_id)) {
    PIPE_MGR_ASSERT(stage_info->lock_pending_list == NULL);
    PIPE_MGR_ASSERT(stage_info->task_list == NULL);
    idle_task_list_t tlist;
    PIPE_MGR_MEMSET(&tlist, 0, sizeof(idle_task_list_t));
    tlist.move_to_front = move_to_front;
    tlist.lock_valid = lock_valid;
    tlist.lock_id = lock_id;
    BF_LIST_DLL_AP(tlist.tasks, tnode_p, next, prev);

    rc = pipe_mgr_idle_task_update_mdata(stage_info, tnode_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating task mdata for "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_info->stage_id,
          rc);
      return rc;
    }
    /* ALERT - If in future, this gets called when there
     * are async messages coming from hardware, needs a lock
     */
    rc = pipe_mgr_idle_process_task_list(idle_tbl_info, stage_info, &tlist);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error processing task list"
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
    return PIPE_SUCCESS;
  }

  idle_task_node_t *tnode =
      (idle_task_node_t *)PIPE_MGR_MALLOC(sizeof(idle_task_node_t));
  if (!tnode) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(tnode, tnode_p, sizeof(idle_task_node_t));

  if (stage_info->lock_pending_list) {
    rc = pipe_mgr_idle_task_update_mdata(stage_info, tnode_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating task mdata for "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_info->stage_id,
          rc);
      PIPE_MGR_FREE(tnode);
      return rc;
    }
    PIPE_MGR_DBGCHK(!pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info));
    /* If a lock_pending_list already exists. Just append the
     * current task into the same list
     */
    BF_LIST_DLL_AP(stage_info->lock_pending_list->tasks, tnode, next, prev);
  } else {
    idle_task_list_t *llist =
        (idle_task_list_t *)PIPE_MGR_MALLOC(sizeof(idle_task_list_t));
    if (!llist) {
      PIPE_MGR_FREE(tnode);
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMSET(llist, 0, sizeof(idle_task_list_t));

    llist->move_to_front = move_to_front;
    llist->lock_valid = lock_valid;
    llist->lock_id = lock_id;

    BF_LIST_DLL_AP(llist->tasks, tnode, next, prev);

    PIPE_MGR_LOCK(&stage_info->tlist_mtx);
    rc = pipe_mgr_idle_task_update_mdata(stage_info, tnode_p);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error updating task mdata for "
          "stage %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_info->stage_id,
          rc);
      PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
      PIPE_MGR_FREE(tnode);
      return rc;
    }
    /* If it is part of transaction, then this has to go into trans_list */
    if (IDLE_SESS_IS_TXN(stage_info->idle_tbl_p)) {
      dest_list_p = &stage_info->trans_list;
    } else {
      PIPE_MGR_DBGCHK(!pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info));
      dest_list_p = &stage_info->task_list;
    }

    if (move_to_front) {
      BF_LIST_DLL_PP(*dest_list_p, llist, next, prev);
    } else {
      BF_LIST_DLL_AP(*dest_list_p, llist, next, prev);
    }
    if (IDLE_TBL_IS_POLL_MODE(idle_tbl_info) &&
        !idle_tbl_info->update_in_progress) {
      idle_task_list_t *tlist = stage_info->task_list;
      while (tlist) {
        rc = pipe_mgr_idle_process_task_list(idle_tbl_info, stage_info, tlist);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Error processing pending items for idle time"
              "pipe %d stage %d rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              stage_info->idle_tbl_p->pipe_id,
              stage_info->stage_id,
              rc);
          break;
        }
        BF_LIST_DLL_REM(stage_info->task_list, tlist, next, prev);
        pipe_mgr_idle_destroy_task_list(tlist);
        tlist = stage_info->task_list;
      }
    }
    PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_add_entry_to_stage(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t index,
    uint32_t ttl,
    uint32_t cur_ttl) {
  idle_task_node_t tnode;
  PIPE_MGR_MEMSET(&tnode, 0, sizeof(idle_task_node_t));

  tnode.ent_hdl = ent_hdl;
  tnode.type = IDLE_ENTRY_ADD;
  tnode.u.add.add_idx = index;
  tnode.u.add.ttl = ttl;
  tnode.u.add.cur_ttl = cur_ttl;
  if (ttl == 1) tnode.u.add.poll_state = ENTRY_ACTIVE;

  pipe_status_t rc = PIPE_SUCCESS;
  rc = pipe_mgr_idle_append_task(
      idle_tbl_info, stage_info, &tnode, false, false, 0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error appending add task for "
        "stage %d rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        stage_info->stage_id,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_del_entry_from_stage(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t index) {
  if (!pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info) &&
      IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info) && idle_tbl_info->enabled &&
      !stage_info->lock_pending_list) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deleting idle entry 0x%x without valid lock",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  idle_task_node_t tnode;
  PIPE_MGR_MEMSET(&tnode, 0, sizeof(idle_task_node_t));

  tnode.ent_hdl = ent_hdl;
  tnode.type = IDLE_ENTRY_DEL;
  tnode.u.del.del_idx = index;

  pipe_status_t rc = PIPE_SUCCESS;
  rc = pipe_mgr_idle_append_task(
      idle_tbl_info, stage_info, &tnode, false, false, 0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error appending del task for "
        "stage %d rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        stage_info->stage_id,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_move_entry_in_stage(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t src_index,
    uint32_t dest_index) {
  if (!pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info) &&
      IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info) && idle_tbl_info->enabled &&
      !stage_info->lock_pending_list) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deleting idle entry 0x%x without valid lock",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  idle_task_node_t tnode;
  PIPE_MGR_MEMSET(&tnode, 0, sizeof(idle_task_node_t));

  tnode.ent_hdl = ent_hdl;
  tnode.type = IDLE_ENTRY_MOVE;
  tnode.u.move.src_idx = src_index;
  tnode.u.move.dest_idx = dest_index;

  pipe_status_t rc = PIPE_SUCCESS;
  rc = pipe_mgr_idle_append_task(
      idle_tbl_info, stage_info, &tnode, false, false, 0);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error appending move task for "
        "stage %d rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        stage_info->stage_id,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_lock_stage(idle_tbl_info_t *idle_tbl_info,
                                              idle_tbl_stage_info_t *stage_info,
                                              lock_id_t lock_id) {
  if (pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info)) {
    /* There's nothing to do for lock processing */
    return PIPE_SUCCESS;
  }
  /* For now handling only one lock pending operation */
  PIPE_MGR_DBGCHK(!stage_info->lock_pending_list);

  idle_task_list_t *llist =
      (idle_task_list_t *)PIPE_MGR_MALLOC(sizeof(idle_task_list_t));
  if (!llist) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMSET(llist, 0, sizeof(idle_task_list_t));
  llist->lock_valid = true;
  llist->lock_id = lock_id;

  BF_LIST_DLL_AP(stage_info->lock_pending_list, llist, next, prev);

  pipe_mgr_lock_id_type_e ltype = PIPE_MGR_GET_LOCK_ID_TYPE(lock_id);
  if ((ltype == LOCK_ID_TYPE_IDLE_LOCK) || (ltype == LOCK_ID_TYPE_ALL_LOCK)) {
    /* Add one dummy task into task list. This is because
     * HW returns a lock-ack for even a lock message.
     */
    idle_task_list_t *tlist =
        (idle_task_list_t *)PIPE_MGR_MALLOC(sizeof(idle_task_list_t));
    if (!tlist) {
      LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMSET(tlist, 0, sizeof(idle_task_list_t));
    tlist->lock_valid = true;
    tlist->lock_id = lock_id;

    idle_task_list_t **dest_list_p = NULL;
    PIPE_MGR_LOCK(&stage_info->tlist_mtx);
    if (IDLE_SESS_IS_TXN(stage_info->idle_tbl_p)) {
      dest_list_p = &stage_info->trans_list;
    } else {
      dest_list_p = &stage_info->task_list;
    }

    BF_LIST_DLL_AP(*dest_list_p, tlist, next, prev);
    PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_unlock_stage(
    idle_tbl_info_t *idle_tbl_info,
    idle_tbl_stage_info_t *stage_info,
    lock_id_t lock_id) {
  if (pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info)) {
    /* There's nothing to do for unlock processing */
    return PIPE_SUCCESS;
  }
  idle_task_list_t **dest_list_p = NULL;

  PIPE_MGR_DBGCHK(stage_info->lock_pending_list);

  idle_task_list_t *llist = stage_info->lock_pending_list;
  PIPE_MGR_DBGCHK(llist->lock_valid);
  PIPE_MGR_DBGCHK(llist->lock_id == lock_id);

  /* Remove the list from the lock-pending list and add it to either trans
   * list or move list
   */

  PIPE_MGR_LOCK(&stage_info->tlist_mtx);
  if (IDLE_SESS_IS_TXN(stage_info->idle_tbl_p)) {
    dest_list_p = &stage_info->trans_list;
  } else {
    dest_list_p = &stage_info->task_list;
  }

  BF_LIST_DLL_REM(stage_info->lock_pending_list, llist, next, prev);
  BF_LIST_DLL_AP(*dest_list_p, llist, next, prev);
  PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_stage_abort_txn(
    idle_tbl_stage_info_t *stage_info) {
  pipe_mgr_idle_destroy_all_tasks(&stage_info->lock_pending_list);

  /* Discard the transaction list */
  pipe_mgr_idle_destroy_all_tasks(&stage_info->trans_list);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_idle_stage_commit_txn(
    idle_tbl_stage_info_t *stage_info) {
  idle_tbl_info_t *idle_tbl_info = stage_info->idle_tbl_p->idle_tbl_info;
  pipe_status_t rc = PIPE_SUCCESS;

  PIPE_MGR_DBGCHK(stage_info->lock_pending_list == NULL);

  /* Go through the trans list and do any updates needed
   * in the metadata
   */
  idle_task_list_t *front_list = NULL;
  idle_task_list_t *tlist = NULL;
  idle_task_list_t *tlist_next = NULL;
  for (tlist = stage_info->trans_list; tlist; tlist = tlist_next) {
    tlist_next = tlist->next;
    idle_task_node_t *tnode;
    for (tnode = tlist->tasks; tnode; tnode = tnode->next) {
      rc = pipe_mgr_idle_task_update_mdata(stage_info, tnode);
      if (rc != PIPE_SUCCESS) {
        return rc;
      }
    }
    if (tlist->move_to_front) {
      BF_LIST_DLL_REM(stage_info->trans_list, tlist, next, prev);
      BF_LIST_DLL_AP(front_list, tlist, next, prev);
    }
  }

  if (pipe_mgr_idle_is_in_fast_reconfig_mode(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(stage_info->task_list == NULL);
    BF_LIST_DLL_CAT(front_list, stage_info->trans_list, next, prev);
    stage_info->trans_list = NULL;
    while (front_list) {
      tlist = front_list;
      BF_LIST_DLL_REM(front_list, tlist, next, prev);
      rc = pipe_mgr_idle_process_task_list(idle_tbl_info, stage_info, tlist);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s(%d - 0x%x) Error processing task list"
            "rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            rc);
        return rc;
      }
      pipe_mgr_idle_destroy_task_list(tlist);
    }
  } else {
    PIPE_MGR_LOCK(&stage_info->tlist_mtx);
    BF_LIST_DLL_CAT(front_list, stage_info->task_list, next, prev);
    stage_info->task_list = front_list;
    front_list = NULL;
    BF_LIST_DLL_CAT(stage_info->task_list, stage_info->trans_list, next, prev);
    stage_info->trans_list = NULL;
    PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t rmt_idle_add_entry_internal(
    pipe_sess_hdl_t sess_hdl,
    idle_tbl_info_t *idle_tbl_info,
    pipe_mat_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_tbl_hdl_t stage_table_handle,
    uint32_t index,
    uint32_t ttl,
    uint32_t cur_ttl,
    bool end_of_move_add,
    uint32_t pipe_api_flags) {
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (idle_tbl_info->tbl_params.mode == INVALID_MODE) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Cannot add entry, invalid table mode detected.",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_UNEXPECTED;
  }

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info) && ttl) {
    if (ttl < idle_tbl_info->tbl_params.u.notify.ttl_query_interval) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Request to set entry ttl with value %d"
          " is less than query interval %d. Entry hdl 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          ttl,
          idle_tbl_info->tbl_params.u.notify.ttl_query_interval,
          ent_hdl);
      return PIPE_INVALID_ARG;
    }
  } else {
    // If the idle table is not in notify mode, the ttl must be zero or one
    if (ttl != 0 && ttl != 1) {
      LOG_ERROR("%s:%d %s(%d - 0x%x) Cannot add entry with non-zero ttl",
                __func__,
                __LINE__,
                idle_tbl_info->name,
                idle_tbl_info->dev_id,
                idle_tbl_info->tbl_hdl);
      PIPE_MGR_DBGCHK(ttl == 0);
      return PIPE_INVALID_ARG;
    }
  }

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  idle_tbl->cur_sess_hdl = sess_hdl;
  idle_tbl->sess_flags = pipe_api_flags;

  /* Hardware updates are not needed in a specific case where
   * this entry add is at the end of a move operation
   */
  if (!end_of_move_add) {
    idle_hw_state_e hw_state = HW_IDLE_ACTIVE;
    if (ttl == 0) {
      hw_state = HW_IDLE_DISABLE;
    } else if (cur_ttl != ttl) {
      /* This gets exercised during entry moves across stages.
       * When an entry which has already been idle in hardware is moved to
       * new stage, it is programmed as being inactive
       */
      hw_state = HW_IDLE_INACTIVE;
    }
    rc = pipe_mgr_idle_write_init_val(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle, index, hw_state);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error writing the init value for idle entry pipe %d "
          "stage %d index %d ttl %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id,
          stage_id,
          index,
          ttl,
          rc);
      return rc;
    }
  }

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_add_entry_to_stage(
          idle_tbl_info, stage_info, ent_hdl, index, ttl, cur_ttl);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error adding idle entry at index %d stage %d pipe %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            index,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_idle_add_entry_to_stage(
        idle_tbl_info, stage_info, ent_hdl, index, ttl, cur_ttl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error adding idle entry at index %d stage %d pipe %d"
          " rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          index,
          stage_id,
          pipe_id,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_add_entry(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                 pipe_mat_ent_hdl_t ent_hdl,
                                 bf_dev_pipe_t pipe_id,
                                 uint8_t stage_id,
                                 rmt_tbl_hdl_t stage_table_hdl,
                                 uint32_t index,
                                 uint32_t ttl,
                                 bool end_of_move_add,
                                 uint32_t pipe_api_flags) {
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    rc = pipe_mgr_alpm_ent_hdl_atcam_to_alpm(
        dev_id, idle_tbl_info->tbl_hdl, pipe_id, &ent_hdl, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Entry not found in table 0x%x on device %d",
                __func__,
                __LINE__,
                idle_tbl_info->tbl_hdl,
                dev_id);
      return rc;
    }
  }

  return rmt_idle_add_entry_internal(sess_hdl,
                                     idle_tbl_info,
                                     ent_hdl,
                                     pipe_id,
                                     stage_id,
                                     stage_table_hdl,
                                     index,
                                     ttl,
                                     ttl,
                                     end_of_move_add,
                                     pipe_api_flags);
}

pipe_status_t rmt_idle_delete_entry_internal(pipe_sess_hdl_t sess_hdl,
                                             idle_tbl_info_t *idle_tbl_info,
                                             pipe_mat_ent_hdl_t ent_hdl,
                                             bf_dev_pipe_t pipe_id,
                                             uint8_t stage_id,
                                             rmt_tbl_hdl_t stage_table_handle,
                                             uint32_t index,
                                             uint32_t pipe_api_flags) {
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  idle_tbl->cur_sess_hdl = sess_hdl;
  idle_tbl->sess_flags = pipe_api_flags;

  rc = pipe_mgr_idle_write_init_val(idle_tbl_info,
                                    pipe_id,
                                    stage_id,
                                    stage_table_handle,
                                    index,
                                    HW_IDLE_DISABLE);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error writing the init value for idle entry pipe %d "
        "stage %d index %d rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        pipe_id,
        stage_id,
        index,
        rc);
    return rc;
  }

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
    if (!dev_info) {
      LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }

    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_del_entry_from_stage(
          idle_tbl_info, stage_info, ent_hdl, index);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error deleting idle entry at index %d stage %d pipe %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            index,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_idle_del_entry_from_stage(
        idle_tbl_info, stage_info, ent_hdl, index);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error deleting idle entry at index %d stage %d pipe %d"
          " rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          index,
          stage_id,
          pipe_id,
          rc);
      return rc;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_delete_entry(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t dev_id,
                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                    pipe_mat_ent_hdl_t ent_hdl,
                                    bf_dev_pipe_t pipe_id,
                                    uint8_t stage_id,
                                    rmt_tbl_hdl_t stage_table_handle,
                                    uint32_t index,
                                    uint32_t pipe_api_flags) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (idle_tbl_info->match_type == ALPM_MATCH) {
    rc = pipe_mgr_alpm_ent_hdl_atcam_to_alpm(
        dev_id, idle_tbl_info->tbl_hdl, pipe_id, &ent_hdl, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Entry not found in table 0x%x on device %d",
                __func__,
                __LINE__,
                idle_tbl_info->tbl_hdl,
                dev_id);
      return rc;
    }
  }

  return rmt_idle_delete_entry_internal(sess_hdl,
                                        idle_tbl_info,
                                        ent_hdl,
                                        pipe_id,
                                        stage_id,
                                        stage_table_handle,
                                        index,
                                        pipe_api_flags);
}

pipe_status_t rmt_idle_tbl_lock(pipe_sess_hdl_t sess_hdl,
                                bf_dev_id_t dev_id,
                                pipe_mat_tbl_hdl_t tbl_hdl,
                                lock_id_t lock_id,
                                bf_dev_pipe_t pipe_id,
                                uint8_t stage_id,
                                rmt_tbl_hdl_t stage_table_handle,
                                uint32_t pipe_api_flags) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  idle_tbl->cur_sess_hdl = sess_hdl;
  idle_tbl->sess_flags = pipe_api_flags;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
    if (!dev_info) {
      LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }

    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_lock_stage(idle_tbl_info, stage_info, lock_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error locking idle table lock_id 0x%x stage %d pipe %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            lock_id,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_idle_lock_stage(idle_tbl_info, stage_info, lock_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error locking idle table lock_id 0x%x stage %d pipe %d"
          " rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          lock_id,
          stage_id,
          pipe_id,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_tbl_unlock(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  lock_id_t lock_id,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t stage_id,
                                  rmt_tbl_hdl_t stage_table_handle,
                                  uint32_t pipe_api_flags) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  idle_tbl->cur_sess_hdl = sess_hdl;
  idle_tbl->sess_flags = pipe_api_flags;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
    if (!dev_info) {
      LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }

    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_unlock_stage(idle_tbl_info, stage_info, lock_id);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error unlocking idle table lock_id 0x%x stage %d pipe %d"
            " rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            lock_id,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_idle_unlock_stage(idle_tbl_info, stage_info, lock_id);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error unlocking idle table lock_id 0x%x stage %d pipe %d"
          " rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          lock_id,
          stage_id,
          pipe_id,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t rmt_idle_move_entry_across_stage(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mat_ent_hdl_t ent_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t src_stage_id,
    rmt_tbl_hdl_t src_stage_table_handle,
    uint8_t dest_stage_id,
    rmt_tbl_hdl_t dest_stage_table_handle,
    uint32_t src_index,
    uint32_t dest_index,
    uint32_t pipe_api_flags) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (idle_tbl_info->match_type == ALPM_MATCH) {
    rc = pipe_mgr_alpm_ent_hdl_atcam_to_alpm(
        dev_id, idle_tbl_info->tbl_hdl, pipe_id, &ent_hdl, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Entry not found in table 0x%x on device %d",
                __func__,
                __LINE__,
                idle_tbl_info->tbl_hdl,
                dev_id);
      return rc;
    }
  }

  uint32_t init_ttl = 0, cur_ttl = 0;
  rc = pipe_mgr_idle_get_ttl_helper(idle_tbl_info,
                                    ent_hdl,
                                    pipe_id,
                                    src_stage_id,
                                    src_stage_table_handle,
                                    &cur_ttl,
                                    &init_ttl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting current ttl for mat entry 0x%x in stage %d"
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        ent_hdl,
        src_stage_id,
        rc);
    return rc;
  }

  /* Add the entry in dest-stage */
  rc = rmt_idle_add_entry_internal(sess_hdl,
                                   idle_tbl_info,
                                   ent_hdl,
                                   pipe_id,
                                   dest_stage_id,
                                   dest_stage_table_handle,
                                   dest_index,
                                   init_ttl,
                                   cur_ttl,
                                   false,
                                   pipe_api_flags);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error adding the entry to destination stage %d index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        dest_stage_id,
        dest_index,
        rc);
    return rc;
  }

  rc = rmt_idle_delete_entry_internal(sess_hdl,
                                      idle_tbl_info,
                                      ent_hdl,
                                      pipe_id,
                                      src_stage_id,
                                      src_stage_table_handle,
                                      src_index,
                                      pipe_api_flags);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error deleting the entry to src stage %d index %d "
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        src_stage_id,
        src_index,
        rc);
    return rc;
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_move_entry(pipe_sess_hdl_t sess_hdl,
                                  bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  pipe_mat_ent_hdl_t ent_hdl,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t src_stage_id,
                                  rmt_tbl_hdl_t src_stage_table_hdl,
                                  uint8_t dest_stage_id,
                                  rmt_tbl_hdl_t dest_stage_table_hdl,
                                  uint32_t src_index,
                                  uint32_t dest_index,
                                  uint32_t pipe_api_flags) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;

  if ((src_stage_id != dest_stage_id) ||
      (src_stage_table_hdl != dest_stage_table_hdl)) {
    return rmt_idle_move_entry_across_stage(sess_hdl,
                                            dev_id,
                                            tbl_hdl,
                                            ent_hdl,
                                            pipe_id,
                                            src_stage_id,
                                            src_stage_table_hdl,
                                            dest_stage_id,
                                            dest_stage_table_hdl,
                                            src_index,
                                            dest_index,
                                            pipe_api_flags);
  }

  uint8_t stage_id = src_stage_id;
  rmt_tbl_hdl_t stage_table_handle = src_stage_table_hdl;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (idle_tbl_info->match_type == ALPM_MATCH) {
    rc = pipe_mgr_alpm_ent_hdl_atcam_to_alpm(
        dev_id, idle_tbl_info->tbl_hdl, pipe_id, &ent_hdl, false);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Entry not found in table 0x%x on device %d",
                __func__,
                __LINE__,
                idle_tbl_info->tbl_hdl,
                dev_id);
      return rc;
    }
  }

  idle_tbl_t *idle_tbl = NULL;
  if (IDLE_TBL_IS_SYMMETRIC(idle_tbl_info)) {
    PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    idle_tbl = &idle_tbl_info->idle_tbls[0];
  } else {
    PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe_id);
    if (!idle_tbl) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  idle_tbl->cur_sess_hdl = sess_hdl;
  idle_tbl->sess_flags = pipe_api_flags;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
    if (!dev_info) {
      LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }

    /* Loop through all the pipes */
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      idle_tbl_stage_info_t *stage_info = NULL;
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        return PIPE_OBJ_NOT_FOUND;
      }

      rc = pipe_mgr_idle_move_entry_in_stage(
          idle_tbl_info, stage_info, ent_hdl, src_index, dest_index);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error moving idle entry from index %d to index %d "
            "stage %d pipe %d rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            src_index,
            dest_index,
            stage_id,
            pipe,
            rc);
        return rc;
      }
    }
  } else {
    idle_tbl_stage_info_t *stage_info = NULL;
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_idle_move_entry_in_stage(
        idle_tbl_info, stage_info, ent_hdl, src_index, dest_index);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Error moving idle entry from index %d to index %d "
          "stage %d pipe %d rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          src_index,
          dest_index,
          stage_id,
          pipe_id,
          rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_abort_txn(bf_dev_id_t dev_id,
                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                 bf_dev_pipe_t *pipes_list,
                                 unsigned nb_pipes) {
  /* Go through all the pipes and stages and free the trans_list and
   * lock_pending_list
   */
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_t *idle_tbl;
  unsigned i, pipe;
  bool symmetric;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (idle_tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe);
    if (!idle_tbl) continue;

    idle_tbl->cur_sess_hdl = -1;
    idle_tbl->sess_flags = 0;

    int stage;
    for (stage = 0; stage < idle_tbl->num_stages; stage++) {
      idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[stage];
      rc = pipe_mgr_idle_stage_abort_txn(stage_info);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error aborting txn for "
            "stage %d pipe %d rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_info->stage_id,
            idle_tbl->pipe_id,
            rc);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_idle_commit_txn(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes) {
  /* Go through all the pipes and stages and move the trans_list to
   * the task_list. If the task_list is empty, then all the non-lock-valid
   * items in the head end of trans-list needs to be evaluated directly
   */
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_t *idle_tbl;
  unsigned i, pipe;
  bool symmetric;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (idle_tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    idle_tbl = pipe_mgr_idle_tbl_get(idle_tbl_info, pipe);
    if (!idle_tbl) continue;

    idle_tbl->cur_sess_hdl = -1;
    idle_tbl->sess_flags = 0;

    int stage;
    for (stage = 0; stage < idle_tbl->num_stages; stage++) {
      idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[stage];
      rc = pipe_mgr_idle_stage_commit_txn(stage_info);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Error committing txn for "
            "stage %d pipe %d rc 0x%x",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_info->stage_id,
            idle_tbl->pipe_id,
            rc);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       idle_tbl_info->is_symmetric,
                                       idle_tbl_info->num_scopes,
                                       &idle_tbl_info->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num scopes %d ",
              __func__,
              idle_tbl_info->name,
              idle_tbl_info->is_symmetric,
              idle_tbl_info->num_scopes);
    return rc;
  }

  if (is_table_populated(dev_id, tbl_hdl)) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) "
        "Idle symmetric mode cannot be changed when mat-entries are present",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_NOT_SUPPORTED;
  }

  /* Do not proceed with the change in mode until all the existing msgs have
   * come back */
  rc = pipe_mgr_idle_drain_all_msgs(idle_tbl_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error draining idle msgs "
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  bool idle_enabled = false;
  if (idle_tbl_info->enabled) {
    idle_enabled = true;

    /* First disable the sweep timer */
    rc = rmt_idle_tmo_disable(sess_hdl, dev_id, tbl_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error disabling idle table"
          "rc 0x%x",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          rc);
      return rc;
    }
  }
  /* Save params to restore after changing mode */
  pipe_idle_time_params_t params = {0};
  rc = rmt_idle_params_get(dev_id, tbl_hdl, &params);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) Error saving table params idle_tbls"
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  rc = pipe_mgr_idle_tbl_discard(idle_tbl_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) Error discarding idle_tbls"
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  /* Switch the mode */
  idle_tbl_info->is_symmetric = symmetric;
  PIPE_MGR_DBGCHK(num_scopes <= PIPE_BITMAP_COUNT(&idle_tbl_info->pipe_bmp));
  /* Copy the new scope info */
  idle_tbl_info->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(idle_tbl_info->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  rc = pipe_mgr_idle_tbl_allocate(idle_tbl_info);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) Error allocating idle_tbls"
        "rc 0x%x",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl,
        rc);
    return rc;
  }

  /* Restore configuration. It is possible that callbacks are not set after
   * restore function was used. */
  idle_tbl_info->in_state_restore = true;
  rc = rmt_idle_params_set(sess_hdl, dev_id, tbl_hdl, params);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error restoring idle tbl config rc 0x%x",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              rc);
    return rc;
  }
  idle_tbl_info->in_state_restore = false;

  if (idle_enabled) {
    rc = rmt_idle_tmo_enable(sess_hdl, dev_id, tbl_hdl);
  }

  return rc;
}

pipe_status_t pipe_mgr_idle_tbl_set_repeated_notify(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    bool repeated_notify) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  rc = pipe_mgr_tbl_get_entry_count(dev_tgt, tbl_hdl, true, &count);
  if (count != 0) {
    LOG_ERROR(
        "%s:%d %s(%d - 0x%x) "
        "Idle repeated notification mode cannot be changed when mat-entries "
        "are present",
        __func__,
        __LINE__,
        idle_tbl_info->name,
        idle_tbl_info->dev_id,
        idle_tbl_info->tbl_hdl);
    return PIPE_NOT_SUPPORTED;
  }

  if (idle_tbl_info->repeated_notify == repeated_notify) {
    LOG_TRACE("%s: Table %s, No change to repeated aging notification mode %d ",
              __func__,
              idle_tbl_info->name,
              idle_tbl_info->repeated_notify);
    return rc;
  }

  idle_tbl_info->repeated_notify = repeated_notify;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_tbl_get_repeated_notify(bf_dev_id_t dev_id,
                                                    pipe_mat_tbl_hdl_t tbl_hdl,
                                                    bool *repeated_notify) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exists",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *repeated_notify = idle_tbl_info->repeated_notify;

  return PIPE_SUCCESS;
}

static bool pipe_mgr_idle_task_list_needs_ack(idle_task_list_t *task_list) {
  idle_task_list_t *tlist;
  for (tlist = task_list; tlist; tlist = tlist->next) {
    if (tlist->lock_valid) {
      return true;
    }
  }
  return false;
}

static pipe_status_t pipe_mgr_idle_drain_all_msgs(
    idle_tbl_info_t *idle_tbl_info) {
  int pipe;
  for (pipe = 0; pipe < idle_tbl_info->no_idle_tbls; pipe++) {
    idle_tbl_t *idle_tbl = &idle_tbl_info->idle_tbls[pipe];
    int stage;
    for (stage = 0; stage < idle_tbl->num_stages; stage++) {
      idle_tbl_stage_info_t *stage_info = &idle_tbl->stage_info[stage];
      bool pending = true;
      /* As long as there are pending tasks (e.g. un-acked locks), service the
       * DRs to ensure DMAs are processed and messages from HW are picked up. */
      while (pending) {
        PIPE_MGR_LOCK(&stage_info->tlist_mtx);
        pending = pipe_mgr_idle_task_list_needs_ack(stage_info->task_list);
        PIPE_MGR_UNLOCK(&stage_info->tlist_mtx);
        if (pending) {
          pipe_mgr_drv_service_drs(idle_tbl_info->dev_id);
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_drv_idl_buf_init(pipe_sess_hdl_t h, bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_drv_ses_state_t *st;
  st = pipe_mgr_drv_get_ses_state(&h, __func__, __LINE__);
  if (NULL == st) {
    return PIPE_INVALID_ARG;
  }
  idle_mgr_dev_ctx_t *dev = idle_mgr_dev_ctx(dev_id);
  uint32_t num_subdevices = pipe_mgr_get_num_active_subdevices(dev_id);
  for (uint32_t subdev = 0; subdev < num_subdevices; subdev++) {
    /* Maintain an array of the buffers' physical addresses and a pointer to the
     * next expected buffer. */
    dev->buf_addr_idx[subdev] = 0;
    int idle_buf_cnt =
        pipe_mgr_drv_subdev_buf_count(dev_id, subdev, PIPE_MGR_DRV_BUF_IDL);
    dev->buf_cnt[subdev] = idle_buf_cnt;
    dev->buf_addrs[subdev] =
        PIPE_MGR_MALLOC(idle_buf_cnt * sizeof dev->buf_addrs[0][0]);
    if (!dev->buf_addrs[subdev]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto err_cleanup;
    }

    /* Initialize the lock for the array of buffer addresses. */
    PIPE_MGR_LOCK_INIT(dev->buf_addrs_mtx[subdev]);
    PIPE_MGR_COND_INIT(dev->buf_addrs_cv[subdev]);

    /* Allocate all the idle time DMA buffers. */
    int idle_buf_sz =
        pipe_mgr_drv_subdev_buf_size(dev_id, subdev, PIPE_MGR_DRV_BUF_IDL);
    for (int idl_buf = 0; idl_buf < idle_buf_cnt; ++idl_buf) {
      pipe_mgr_drv_buf_t *b = NULL;
      b = pipe_mgr_drv_buf_alloc_subdev(
          st->sid, dev_id, subdev, idle_buf_sz, PIPE_MGR_DRV_BUF_IDL, false);
      if (!b) {
        PIPE_MGR_DBGCHK(b);
        sts = PIPE_NO_SYS_RESOURCES;
        goto err_cleanup;
      }
      BF_LIST_DLL_AP(dev->bufs[subdev], b, next, prev);
      dev->buf_addrs[subdev][idl_buf] = b->phys_addr;
    }
  }

  if (!pipe_mgr_is_device_locked(dev_id)) {
    sts = pipe_mgr_idl_buf_load(dev_id);
    if (PIPE_SUCCESS != sts) {
      goto err_cleanup;
    }
  }
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_drv_idl_buf_cleanup(dev_id);
  return sts;
}

pipe_status_t pipe_mgr_idl_buf_load(bf_dev_id_t dev_id) {
  uint32_t num_subdevices = pipe_mgr_get_num_active_subdevices(dev_id);
  for (uint32_t subdev = 0; subdev < num_subdevices; subdev++) {
    pipe_mgr_drv_buf_t *b = idle_mgr_dev_ctx(dev_id)->bufs[subdev];
    for (; b; b = b->next) {
      bf_dma_addr_t dma_addr;
      if (bf_sys_dma_map(b->pool,
                         b->addr,
                         b->phys_addr,
                         b->size,
                         &dma_addr,
                         BF_DMA_TO_CPU) != 0) {
        LOG_ERROR("Unable to map DMA buffer %p at %s:%d",
                  b->addr,
                  __func__,
                  __LINE__);
        return PIPE_COMM_FAIL;
      }
      int x =
          lld_subdev_push_fm(dev_id, subdev, lld_dr_fm_idle, dma_addr, b->size);
      if (x != LLD_OK) {
        /* Unmap the buffer */
        if (bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_TO_CPU) != 0) {
          LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                    b->addr,
                    __func__,
                    __LINE__);
        }
        LOG_ERROR(
            "%s Error pushing idle free memory to device %d rc %d addr "
            "0x%" PRIx64 " size %d",
            __func__,
            dev_id,
            x,
            dma_addr,
            b->size);
        PIPE_MGR_DBGCHK(x == LLD_OK);
        return PIPE_LLD_FAILED;
      }
    }
  }
  return pipe_mgr_drv_push_idle_time_drs(dev_id);
}

/** \brief pipe_mgr_drv_idl_buf_cleanup
 *        Cleanup the memory and buffers allocated for the device.
 *        SHOULD BE DONE ONLY DURING DEVICE REMOVE. The DMA buffers will
 *        be put into the free pool
 */
pipe_status_t pipe_mgr_drv_idl_buf_cleanup(uint8_t dev_id) {
  idle_mgr_dev_ctx_t *dev_ctx = idle_mgr_dev_ctx(dev_id);
  if (!dev_ctx) {
    LOG_ERROR("%s:%d Dev %d unknown", __func__, __LINE__, dev_id);
    return PIPE_UNEXPECTED;
  }

  uint32_t num_subdevices = pipe_mgr_get_num_active_subdevices(dev_id);
  for (uint32_t subdev = 0; subdev < num_subdevices; subdev++) {
    while (dev_ctx->bufs[subdev]) {
      pipe_mgr_drv_buf_t *b = dev_ctx->bufs[subdev];
      BF_LIST_DLL_REM(dev_ctx->bufs[subdev], b, next, prev);
      pipe_mgr_drv_buf_free(b);
    }

    if (dev_ctx->buf_addrs[subdev]) {
      PIPE_MGR_LOCK_DESTROY(&dev_ctx->buf_addrs_mtx[subdev]);
      PIPE_MGR_COND_DESTROY(&dev_ctx->buf_addrs_cv[subdev]);
      PIPE_MGR_FREE(dev_ctx->buf_addrs[subdev]);
      dev_ctx->buf_addrs[subdev] = NULL;
      dev_ctx->buf_cnt[subdev] = 0;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_write_all_idle_ctrl_regs(pipe_sess_hdl_t sess_hdl,
                                                     rmt_dev_info_t *dev_info) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t tbl_hdl;
  int num_pipes = dev_info->num_active_pipes;
  bf_dev_id_t dev_id = dev_info->dev_id;
  int num_stages = dev_info->num_active_mau;

  /* Write the Move Reg control register. */
  int s, r, p;
  for (s = 0; s < num_stages; ++s) {
    for (r = 0; r < PIPE_MGR_NUM_IDLE_MOVEREG_REGS; ++r) {
      /* Go over all pipes and check (for this state and for this register) if
       * the value is the same in each pipe and if the value is zero in each
       * pipe.  If it is zero in each pipe then there is nothing to do.  If it
       * is non-zero but the same in all pipes then do a write for all pipes.
       * If it is non-zero and different across pipes then do separate writes
       * for each pipe. */
      uint32_t val0 = idle_mgr_dev_ctx(dev_id)->movereg_ctl[0][s][r];
      bool all_zero = !val0, all_same = true;
      for (p = 1; p < num_pipes; ++p) {
        uint32_t valx = idle_mgr_dev_ctx(dev_id)->movereg_ctl[p][s][r];
        all_same = all_same && val0 == valx;
        all_zero = all_zero && !valx;
      }
      if (all_zero) continue;

      /* Get the address of the register.  We don't care about the pipe id here
       * since the instruction list will take care of the pipe id. */
      uint32_t reg_addr = 0;
      switch (dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          reg_addr = get_movereg_ctl_addr_tof(0, s, r);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          reg_addr = get_movereg_ctl_addr_tof2(0, s, r);
          break;
        case BF_DEV_FAMILY_TOFINO3:
          reg_addr = get_movereg_ctl_addr_tof3(0, s, r);
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
      }

      if (all_same) {
        pipe_bitmap_t pipe_bmp;
        PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
        for (p = 0; p < num_pipes; ++p) {
          PIPE_BITMAP_SET(&pipe_bmp, p);
        }
        pipe_instr_write_reg_t instr;
        construct_instr_reg_write(
            dev_id,
            &instr,
            reg_addr,
            idle_mgr_dev_ctx(dev_id)->movereg_ctl[0][s][r]);
        rc = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    dev_info,
                                    &pipe_bmp,
                                    s,
                                    (uint8_t *)(&instr),
                                    sizeof instr);
        if (PIPE_SUCCESS != rc) {
          LOG_ERROR(
              "Failed to add movereg idle config instruction on dev %d sts %s",
              dev_id,
              pipe_str_err(rc));
          PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
          return rc;
        }
      } else {
        pipe_bitmap_t pipe_bmp;
        for (p = 0; p < num_pipes; ++p) {
          PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
          PIPE_BITMAP_SET(&pipe_bmp, p);
          pipe_instr_write_reg_t instr;
          construct_instr_reg_write(
              dev_id,
              &instr,
              reg_addr,
              idle_mgr_dev_ctx(dev_id)->movereg_ctl[p][s][r]);
          rc = pipe_mgr_drv_ilist_add(&sess_hdl,
                                      dev_info,
                                      &pipe_bmp,
                                      s,
                                      (uint8_t *)(&instr),
                                      sizeof instr);
          if (PIPE_SUCCESS != rc) {
            LOG_ERROR(
                "Failed to add movereg idle config instruction on dev %d sts "
                "%s",
                dev_id,
                pipe_str_err(rc));
            PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
            return rc;
          }
        }
      }
    }
  }

  /* Write the sweep control registers. */
  for (idle_tbl_info = pipe_mgr_idle_tbl_info_get_first(dev_id, &tbl_hdl);
       idle_tbl_info;
       idle_tbl_info = pipe_mgr_idle_tbl_info_get_next(dev_id, &tbl_hdl)) {
    if (!idle_tbl_info->enabled) {
      continue;
    }
    idle_tbl_info->idle_tbls[0].cur_sess_hdl = sess_hdl;
    bool en = IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info) ? true : false;
    rc = pipe_mgr_idle_write_sweep_ctl(idle_tbl_info, en);
    if (PIPE_SUCCESS != rc) {
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == rc);
      return rc;
    }
  }
  return PIPE_SUCCESS;
}

/* A helper to get an entry's location and the ttl value to write in the map
 * ram during fast reconfig. */
static pipe_status_t get_entry_loc_and_ttl(
    idle_tbl_info_t *idle_tbl_info,
    pipe_mat_ent_hdl_t ent_hdl,
    uint32_t subindex,
    bf_dev_pipe_t *ent_pipe,
    dev_stage_t *ent_stage,
    rmt_tbl_hdl_t *ent_stage_table_handle,
    uint32_t *ent_idx,
    uint32_t *ttl) {
  pipe_status_t rc;
  rc = pipe_mgr_mat_ent_get_dir_ent_location(idle_tbl_info->dev_id,
                                             idle_tbl_info->tbl_hdl,
                                             ent_hdl,
                                             subindex,
                                             ent_pipe,
                                             ent_stage,
                                             ent_stage_table_handle,
                                             ent_idx);
  if (PIPE_SUCCESS != rc) {
    if (subindex) return PIPE_OBJ_NOT_FOUND;
    LOG_ERROR("Failed to lookup entry %#x in tbl %#x on dev %d for idle init",
              ent_hdl,
              idle_tbl_info->tbl_hdl,
              idle_tbl_info->dev_id);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    return PIPE_INIT_ERROR;
  }
  if ((*ent_pipe == DEV_PIPE_ALL) != idle_tbl_info->is_symmetric) {
    LOG_ERROR(
        "Entry %#x in tbl %#x on dev %d has pipe %#x but table symmetric mode "
        "is %d; cannot get location and ttl",
        ent_hdl,
        idle_tbl_info->tbl_hdl,
        idle_tbl_info->dev_id,
        *ent_pipe,
        idle_tbl_info->is_symmetric);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INIT_ERROR;
  }
  /* From the location get the TTL.  Note that we only care about the initial
   * TTL here so we can check if it is idle time disabled (value of 0) or not.
   */
  uint32_t cur_ttl = 0, init_ttl = 0;
  rc = pipe_mgr_idle_get_ttl_helper(idle_tbl_info,
                                    ent_hdl,
                                    *ent_pipe,
                                    *ent_stage,
                                    *ent_stage_table_handle,
                                    &cur_ttl,
                                    &init_ttl);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("Failed to lookup ttl for entry %#x in tbl %#x on dev %d",
              ent_hdl,
              idle_tbl_info->tbl_hdl,
              idle_tbl_info->dev_id);
    PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
    return PIPE_INIT_ERROR;
  }
  int i, j;
  idle_tbl_t *t = NULL;
  for (i = 0; i < idle_tbl_info->no_idle_tbls; ++i) {
    t = &idle_tbl_info->idle_tbls[i];
    if (t->pipe_id == *ent_pipe || (idle_tbl_info->is_symmetric && !i)) {
      idle_tbl_stage_info_t *ts = NULL;
      for (j = 0; j < t->num_stages; ++j) {
        ts = &t->stage_info[j];
        if (ts->stage_id == *ent_stage) {
          uint8_t ret = pipe_mgr_idle_get_entry_init_val(
              idle_tbl_info,
              ts->rmt_params,
              init_ttl ? HW_IDLE_ACTIVE : HW_IDLE_DISABLE);
          if ((uint8_t)-1 == ret) {
            PIPE_MGR_DBGCHK((uint8_t)-1 != ret);
            return PIPE_UNEXPECTED;
          }
          *ttl = ret;
          return PIPE_SUCCESS;
        }
      }
    }
  }
  LOG_ERROR("Failed to get entry ttl for entry %#x in tbl %#x on dev %d",
            ent_hdl,
            idle_tbl_info->tbl_hdl,
            idle_tbl_info->dev_id);
  PIPE_MGR_DBGCHK(0);
  return PIPE_INIT_ERROR;
}
static int get_idle_tbl_idx(idle_tbl_info_t *t, bf_dev_pipe_t pipe) {
  int i;
  for (i = 0; i < t->no_idle_tbls; ++i) {
    if (t->is_symmetric || t->idle_tbls[i].pipe_id == pipe) return i;
  }
  return -1;
}
static pipe_status_t create_default_map_ram_shadows(bf_dev_id_t dev_id,
                                                    rmt_tbl_info_t *rmt_info,
                                                    bf_map_t *map_ram_data) {
  uint32_t idle_disable_val =
      pipe_mgr_idle_get_boot_val(rmt_info->params.idle.bit_width);
  /* Loop over all map rams the table uses. */
  unsigned int mem_idx = 0;
  for (; mem_idx < rmt_info->bank_map[0].num_tbl_word_blks; mem_idx++) {
    mem_id_t mem_id = rmt_info->bank_map[0].tbl_word_blk[mem_idx].mem_id[0];
    /* Allocate memory to hold the shadow of this map ram's data. */
    uint8_t *buf = PIPE_MGR_MALLOC(TOF_MAP_RAM_UNIT_DEPTH);
    if (!buf) {
      LOG_ERROR("Failed to allocated memory for idle map ram shadow on dev %d",
                dev_id);
      return PIPE_NO_SYS_RESOURCES;
    }
    /* Populate it with the default "disable" value. */
    int buf_idx = 0;
    for (; buf_idx < TOF_MAP_RAM_UNIT_DEPTH; ++buf_idx) {
      buf[buf_idx] = idle_disable_val;
    }
    /* Save it for later (when we go through all table entries). */
    bf_map_sts_t s = bf_map_add(map_ram_data, mem_id, buf);
    if (s != BF_MAP_OK) {
      LOG_ERROR(
          "Failed to cache data for idle map ram on dev %d, sts %d", dev_id, s);
      PIPE_MGR_FREE(buf);
      return PIPE_NO_SYS_RESOURCES;
    }
  }
  return PIPE_SUCCESS;
}
static pipe_status_t populate_map_ram_shadows(pipe_sess_hdl_t sess_hdl,
                                              idle_tbl_info_t *idle_tbl_info,
                                              bf_map_t **map_ram_data) {
  pipe_mat_tbl_hdl_t tbl_hdl = idle_tbl_info->tbl_hdl;
  bf_dev_id_t dev_id = idle_tbl_info->dev_info->dev_id;
  pipe_status_t rc;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};

  int h;
  for (rc = pipe_mgr_tbl_get_first_entry_handle(sess_hdl, tbl_hdl, dev_tgt, &h);
       rc == PIPE_SUCCESS && h != -1;
       rc = pipe_mgr_tbl_get_next_entry_handles(
           sess_hdl, tbl_hdl, dev_tgt, h, 1, &h)) {
    pipe_mat_ent_hdl_t ent_hdl = h;
    uint32_t subindex = 0;
    bf_dev_pipe_t ent_pipe;
    dev_stage_t ent_stage;
    rmt_tbl_hdl_t ent_stage_table_handle;
    uint32_t ent_idx;
    uint32_t ttl;

    for (subindex = 0; rc == PIPE_SUCCESS; ++subindex) {
      rc = get_entry_loc_and_ttl(idle_tbl_info,
                                 ent_hdl,
                                 subindex,
                                 &ent_pipe,
                                 &ent_stage,
                                 &ent_stage_table_handle,
                                 &ent_idx,
                                 &ttl);
      if (PIPE_SUCCESS != rc) break;
      /* Convert the entry's pipe id into the index which holds the pipe-level
       * table for that pipe in the idle_table_info_t struct. */
      int pipe_idx = get_idle_tbl_idx(idle_tbl_info, ent_pipe);

      bf_dev_pipe_t ipipe = ent_pipe;
      if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
        pipe_bitmap_t local_pipe_bmp;
        PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
        pipe_mgr_idle_tbl_notify_pipe_list_get(
            idle_tbl_info, ipipe, &local_pipe_bmp);
        ipipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, -1);
      }

      /* Map the entry's pipe-stage-index to a map ram shadow, line in that
       * shadow, and subword within that line. */
      idle_tbl_stage_info_t *sinfo = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, ipipe, ent_stage, ent_stage_table_handle);
      if (!sinfo) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            ent_stage,
            ipipe);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }
      int stg_lvl_line = ent_idx / sinfo->entries_per_word;
      unsigned int mem_idx = stg_lvl_line / TOF_MAP_RAM_UNIT_DEPTH;
      PIPE_MGR_DBGCHK(mem_idx <= sinfo->hw_info.num_blks);
      int mem_id = sinfo->hw_info.tbl_blk[mem_idx].mem_id[0];
      int line = stg_lvl_line % TOF_MAP_RAM_UNIT_DEPTH;
      int subword = ent_idx % sinfo->entries_per_word;
      int entry_width = sinfo->rmt_params.bit_width;
      int entry_shift = entry_width * subword;
      unsigned int mask = ~(((1 << entry_width) - 1) << entry_shift);
      uint8_t *shadow;
      bf_map_sts_t s = bf_map_get(
          &map_ram_data[pipe_idx][sinfo->stage_idx], mem_id, (void **)&shadow);
      if (BF_MAP_OK != s) {
        LOG_ERROR(
            "Unexpected map ram id %d in stage %d pipe %x dev %d for tbl %#x "
            "entry %#x idx %d, status %d",
            mem_id,
            sinfo->stage_id,
            ent_pipe,
            dev_id,
            tbl_hdl,
            h,
            ent_idx,
            s);
        rc = PIPE_UNEXPECTED;
        break;
      }
      shadow[line] = (shadow[line] & mask) | (ttl << entry_shift);
    }
    if (subindex != 0 && rc == PIPE_OBJ_NOT_FOUND) {
      /* Okay, this means there are no more instances of the entry; we went
       * through all the valid subwords/locations of the entry. */
    } else if (rc != PIPE_SUCCESS) {
      /* Not okay... */
      LOG_ERROR(
          "Failed to setup initial TTL value on dev %d tbl %#x entry %#x, "
          "status %s",
          dev_id,
          tbl_hdl,
          h,
          pipe_str_err(rc));
      PIPE_MGR_DBGCHK(rc == PIPE_SUCCESS);
      return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_idle_fast_reconfig_push_state(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id) {
  /* Push the idle time state at the end of fast reconfig to update
   * all the map-rams
   */
  idle_tbl_info_t *idle_tbl_info = NULL;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mat_tbl_hdl_t tbl_hdl;
  uint32_t buf_sz =
      pipe_mgr_drv_subdev_buf_size(dev_id, 0, PIPE_MGR_DRV_BUF_BWR);
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  /* This function does two things
   *    1. Creates MAP RAM data with idle disable values.
   *    2. Walks over any entry handles that may exist in the table and
   *       UPDATES the MAP RAM data with init ttl values.
   * Now, this may be happening in two scenarios, the first one is during
   * fast re-cfg after API replay and the second one in cold-boot. For the
   * second scenario, the idle tmo would not have been enabled and hence
   * idle_tbl software structure is not fully allocated yet, and step 2 is
   * not applicable. We use rmt_info state to walk over all the stages and
   * and table instances to populate the disable value and the idle tbl
   * structures to program the init ttl value.
   */

  /* An array of maps, the first dimension is the pipe, the second dimension is
   * the stage, the key is the mem_id of a map ram and the data is the map ram
   * shadow. */
  bf_map_t **map_ram_data = NULL;
  unsigned int map_array_len_1 = 0;
  unsigned int map_array_len_2 = 0;

  /* For each idle table in every profile on the device. */
  for (idle_tbl_info = pipe_mgr_idle_tbl_info_get_first(dev_id, &tbl_hdl);
       idle_tbl_info;
       idle_tbl_info = pipe_mgr_idle_tbl_info_get_next(dev_id, &tbl_hdl)) {
    /* If the table is symmetric then we only need to deal with the first pipe
     * level table.  If it is asymmetric then we need to go through all of
     * them. */
    mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
    if (mat_tbl_info == NULL) {
      LOG_ERROR("%s:%d Table 0x%x not found in RMT database for device %d",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    bool tbl_is_sym = idle_tbl_info->is_symmetric;
    unsigned int num_tbls =
        tbl_is_sym
            ? 1
            : (idle_tbl_info->no_idle_tbls ? idle_tbl_info->no_idle_tbls : 1);

    /* A map per pipe-table per stage to hold the map ram shadow data, key will
     * be stage and mem-id. */
    map_array_len_1 = num_tbls;
    map_ram_data = PIPE_MGR_CALLOC(map_array_len_1, sizeof *map_ram_data);
    if (!map_ram_data) {
      LOG_ERROR("Failed to allocated memory for idle map ram shadows on dev %d",
                dev_id);
      return PIPE_NO_SYS_RESOURCES;
    }

    /* For each pipe level table or just once if the table is symmetric. */
    unsigned int tbl_index, stage_idx = 0;
    uint8_t stage_indices[mat_tbl_info->num_rmt_info];
    for (tbl_index = 0; tbl_index < num_tbls; ++tbl_index) {
      stage_idx = 0;
      /* Allocate a map for each logical table. */
      map_ram_data[tbl_index] =
          PIPE_MGR_CALLOC(mat_tbl_info->num_rmt_info, sizeof(bf_map_t));
      if (!map_ram_data[tbl_index]) {
        LOG_ERROR(
            "Failed to allocated memory for idle map ram shadows on dev %d",
            dev_id);
        rc = PIPE_NO_SYS_RESOURCES;
        goto clean_up_this_table;
      }
      unsigned j = 0;
      for (j = 0; j < mat_tbl_info->num_rmt_info; j++) {
        rmt_tbl_info_t *rmt_info = &mat_tbl_info->rmt_info[j];
        if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
          continue;
        }
        /* Go through each stage the table is in and prepare the map ram
         * shadows.
         */
        bf_map_init(&map_ram_data[tbl_index][stage_idx]);
        /* Create the map ram shadows for all memories in the stage and popupate
         * then with the default disable value. */
        rc = create_default_map_ram_shadows(
            dev_id, rmt_info, &map_ram_data[tbl_index][stage_idx]);
        if (PIPE_SUCCESS != rc) {
          PIPE_MGR_DBGCHK(PIPE_SUCCESS == rc);
          goto clean_up_this_table;
        }
        stage_indices[stage_idx] = rmt_info->stage_id;
        stage_idx++;
      }
    }

    map_array_len_2 = stage_idx;
    /* Go through each entry in this table and update the map ram shadow with
     * the entry's TTL value. */
    rc = populate_map_ram_shadows(sess_hdl, idle_tbl_info, map_ram_data);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(PIPE_SUCCESS == rc);
      goto clean_up_this_table;
    }

    /*
     * We've gone through all entries in the table go ahead and perform the
     * write blocks to prepare the table's map rams.
     */

    /* For each pipe table... */
    for (tbl_index = 0; tbl_index < num_tbls; ++tbl_index) {
      /* Set the pipe mask to contain all pipes the table belongs to.  This is
       * a single pipe for asymmetric tables or all pipes of the profile for
       * symmetric tables. */
      int pipe_bitmap = 0;
      if (idle_tbl_info->is_symmetric || !idle_tbl_info->enabled) {
        /* If no idle tables have been allocated, then this implies that
         * we are only writing disable value, which is common for all pipes
         * Hence, regardless of the symmetricity of the table, just use
         * ALL pipes and stop at one write.
         */
        unsigned int i;
        PIPE_BITMAP_ITER(&idle_tbl_info->pipe_bmp, i) { pipe_bitmap |= 1 << i; }
      } else {
        unsigned int i;
        PIPE_BITMAP_ITER(&(idle_tbl_info->idle_tbls[tbl_index].inst_pipe_bmp),
                         i) {
          pipe_bitmap |= 1 << i;
        }
      }

      for (stage_idx = 0; stage_idx < map_array_len_2; stage_idx++) {
        int stage_id = stage_indices[stage_idx];
        bf_map_t *map = &map_ram_data[tbl_index][stage_idx];
        unsigned long mem_id;
        uint8_t *shdw;
        while (BF_MAP_OK ==
               bf_map_get_first_rmv(map, &mem_id, (void **)&shdw)) {
          uint64_t map_phy_addr =
              idle_tbl_info->dev_info->dev_cfg.get_full_phy_addr(
                  idle_tbl_info->direction,
                  0,
                  stage_id,
                  mem_id,
                  0,
                  pipe_mem_type_map_ram);
          pipe_mgr_drv_buf_t *b = pipe_mgr_drv_buf_alloc(
              sess_hdl, dev_id, buf_sz, PIPE_MGR_DRV_BUF_BWR, false);
          if (!b) {
            LOG_ERROR(
                "Failed to initialize idle map ram shadow dev %d no DMA "
                "buffers",
                dev_id);
            PIPE_MGR_FREE(shdw);
            rc = PIPE_NO_SYS_RESOURCES;
            goto clean_up_this_table;
          }
          PIPE_MGR_MEMSET(b->addr, 0, TOF_MAP_RAM_UNIT_DEPTH * 4);
          int line;
          for (line = 0; line < TOF_MAP_RAM_UNIT_DEPTH; ++line) {
            b->addr[line * 4] = shdw[line];
          }
          PIPE_MGR_FREE(shdw);
          rc = pipe_mgr_drv_blk_wr(&sess_hdl,
                                   4,
                                   TOF_MAP_RAM_UNIT_DEPTH,
                                   1,
                                   map_phy_addr,
                                   pipe_bitmap,
                                   b);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR("Failed to write idle map ram shadow, dev %d status %s",
                      dev_id,
                      pipe_str_err(rc));
            goto clean_up_this_table;
          }
        }
        bf_map_destroy(map);
      }
      PIPE_MGR_FREE(map_ram_data[tbl_index]);
      map_ram_data[tbl_index] = NULL;
    }
    PIPE_MGR_FREE(map_ram_data);
    map_ram_data = NULL;
    map_array_len_1 = 0;
    map_array_len_2 = 0;
  }

clean_up_this_table : {
  unsigned int i, j;
  if (map_ram_data) {
    for (i = 0; i < map_array_len_1; ++i) {
      if (map_ram_data[i]) {
        for (j = 0; j < map_array_len_2; ++j) {
          unsigned long dc;
          void *ptr;
          while (BF_MAP_OK ==
                 bf_map_get_first_rmv(&map_ram_data[i][j], &dc, &ptr)) {
            PIPE_MGR_FREE(ptr);
          }
          bf_map_destroy(&map_ram_data[i][j]);
        }
        PIPE_MGR_FREE(map_ram_data[i]);
      }
    }
    PIPE_MGR_FREE(map_ram_data);
    map_ram_data = NULL;
  }
}
  return rc;
}

pipe_status_t pipe_mgr_idle_log_state(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t tbl_hdl,
                                      cJSON *idle_tbls) {
  bf_map_sts_t msts;
  idle_tbl_info_t *tbl_info = NULL;
  idle_tbl_t *pipe_info = NULL;
  idle_tbl_stage_info_t *stage_info = NULL;
  idle_entry_t *entry_info = NULL;
  idle_entry_metadata_t *ient_mdata = NULL;
  idle_entry_location_t *il = NULL;
  int32_t pipe_idx = 0;
  uint32_t stage_idx = 0;
  int32_t i, j;
  cJSON *idle_tbl, *idle_params, *pipe_tbls, *pipe_tbl, *stage_tbls, *stage_tbl;
  cJSON *idle_ents, *idle_ent, *idle_mdata, *idle_locs, *idle_loc;
  pipe_status_t status = PIPE_SUCCESS;

  tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Idle table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  PIPE_MGR_RW_RDLOCK(&tbl_info->en_dis_rw_lock);

  idle_tbl = cJSON_CreateObject();
  cJSON_AddStringToObject(idle_tbl, "name", tbl_info->name);
  cJSON_AddNumberToObject(idle_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(idle_tbl, "symmetric", tbl_info->is_symmetric);
  cJSON_AddBoolToObject(idle_tbl, "repeated_notify", tbl_info->repeated_notify);
  cJSON_AddBoolToObject(idle_tbl, "enabled", tbl_info->enabled);
  cJSON_AddItemToObject(idle_tbl, "params", idle_params = cJSON_CreateObject());
  cJSON_AddNumberToObject(idle_params, "mode", tbl_info->tbl_params.mode);
  if (IDLE_TBL_IS_NOTIFY_MODE(tbl_info)) {
    cJSON_AddNumberToObject(idle_params,
                            "ttl_query_interval",
                            tbl_info->tbl_params.u.notify.ttl_query_interval);
    cJSON_AddNumberToObject(
        idle_params, "max_ttl", tbl_info->tbl_params.u.notify.max_ttl);
    cJSON_AddNumberToObject(
        idle_params, "min_ttl", tbl_info->tbl_params.u.notify.min_ttl);
  }

  if (tbl_info->no_idle_tbls) {
    PIPE_MGR_DBGCHK(tbl_info->idle_tbls);
    if (!tbl_info->idle_tbls) {
      status = PIPE_UNEXPECTED;
      goto error;
    }
  }
  cJSON_AddItemToObject(idle_tbl, "pipe_tbls", pipe_tbls = cJSON_CreateArray());
  for (pipe_idx = 0; pipe_idx < tbl_info->no_idle_tbls; pipe_idx++) {
    pipe_info = &(tbl_info->idle_tbls[pipe_idx]);
    cJSON_AddItemToArray(pipe_tbls, pipe_tbl = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe_tbl, "pipe_id", pipe_info->pipe_id);
    cJSON_AddItemToObject(
        pipe_tbl, "stage_tbls", stage_tbls = cJSON_CreateArray());
    for (stage_idx = 0; stage_idx < pipe_info->num_stages; stage_idx++) {
      stage_info = &(pipe_info->stage_info[stage_idx]);
      cJSON_AddItemToArray(stage_tbls, stage_tbl = cJSON_CreateObject());
      cJSON_AddNumberToObject(stage_tbl, "stage_id", stage_info->stage_id);
      cJSON_AddItemToObject(
          stage_tbl, "idle_ents", idle_ents = cJSON_CreateArray());
      for (i = 0; i < stage_info->no_words; i++) {
        for (j = 0; j < stage_info->entries_per_word; j++) {
          entry_info = &(stage_info->entries[i][j]);
          if (entry_info->inuse) {
            cJSON_AddItemToArray(idle_ents, idle_ent = cJSON_CreateObject());
            cJSON_AddNumberToObject(idle_ent, "ent_idx", entry_info->index);
            cJSON_AddNumberToObject(idle_ent, "ent_hdl", entry_info->ent_hdl);
            if (IDLE_TBL_IS_NOTIFY_MODE(tbl_info)) {
              cJSON_AddNumberToObject(
                  idle_ent, "notify_state", entry_info->notify_state);
              cJSON_AddNumberToObject(
                  idle_ent, "init_ttl", entry_info->init_ttl);
              cJSON_AddNumberToObject(idle_ent, "cur_ttl", entry_info->cur_ttl);
            }
            if (IDLE_TBL_IS_POLL_MODE(tbl_info)) {
              cJSON_AddBoolToObject(
                  idle_ent, "active", (entry_info->poll_state == ENTRY_ACTIVE));
            }

            PIPE_MGR_LOCK(&stage_info->stage_map_mtx);
            msts = bf_map_get(&stage_info->entry_mdata_map,
                              entry_info->ent_hdl,
                              (void **)&ient_mdata);
            PIPE_MGR_UNLOCK(&stage_info->stage_map_mtx);
            if (msts == BF_MAP_OK) {
              cJSON_AddItemToObject(
                  idle_ent, "mdata", idle_mdata = cJSON_CreateObject());
              cJSON_AddNumberToObject(
                  idle_mdata, "refcount", ient_mdata->refcount);
              cJSON_AddNumberToObject(
                  idle_mdata, "new_ttl", ient_mdata->new_ttl);
              cJSON_AddNumberToObject(
                  idle_mdata, "cur_ttl", ient_mdata->cur_ttl);
              cJSON_AddItemToObject(
                  idle_mdata, "locs", idle_locs = cJSON_CreateArray());
              for (il = ient_mdata->locations; il; il = il->n) {
                cJSON_AddItemToArray(idle_locs,
                                     idle_loc = cJSON_CreateObject());
                cJSON_AddBoolToObject(idle_loc, "valid", il->index_valid);
                cJSON_AddNumberToObject(idle_loc, "cur_index", il->cur_index);
              }
            }
          }
        }
      }
    }
  }
  cJSON_AddItemToArray(idle_tbls, idle_tbl);
error:
  PIPE_MGR_RW_UNLOCK(&tbl_info->en_dis_rw_lock);
  return status;
}

pipe_status_t pipe_mgr_idle_restore_init(bf_dev_id_t dev_id, cJSON *idle_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  idle_tbl_info_t *tbl_info = NULL;
  pipe_mat_tbl_hdl_t tbl_hdl;
  bool symmetric;
  pipe_idle_time_params_t params = {0};
  cJSON *idle_params;
  scope_pipes_t scopes = 0xf;

  tbl_hdl = cJSON_GetObjectItem(idle_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Idle table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  tbl_info->in_state_restore = true;
  tbl_info->repeated_notify =
      (cJSON_GetObjectItem(idle_tbl, "repeated_notify")->type == cJSON_True);

  /* Initially mark table as disabled, it will be properly enabled if needed
     by rmt_idle_tmo_enable */
  tbl_info->enabled = false;
  bool enabled = (cJSON_GetObjectItem(idle_tbl, "enabled")->type == cJSON_True);
  symmetric = (cJSON_GetObjectItem(idle_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != tbl_info->is_symmetric) {
    sts = pipe_mgr_idle_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, 1, &scopes);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, idle tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      goto done;
    }
  }

  idle_params = cJSON_GetObjectItem(idle_tbl, "params");
  if (idle_params) {
    params.mode = cJSON_GetObjectItem(idle_params, "mode")->valueint;
    if (params.mode == NOTIFY_MODE) {
      params.u.notify.ttl_query_interval =
          cJSON_GetObjectItem(idle_params, "ttl_query_interval")->valueint;
      params.u.notify.max_ttl =
          cJSON_GetObjectItem(idle_params, "max_ttl")->valueint;
      params.u.notify.min_ttl =
          cJSON_GetObjectItem(idle_params, "min_ttl")->valueint;
    }
    sts = rmt_idle_params_set(sess_hdl, dev_id, tbl_hdl, params);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to enable timeout state for idle table 0x%x device %u",
                tbl_hdl,
                dev_id);
      goto done;
    }

    if (enabled) {
      sts = rmt_idle_tmo_enable(sess_hdl, dev_id, tbl_hdl);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to enable timeout state for idle table 0x%x device %u",
            tbl_hdl,
            dev_id);
        goto done;
      }
    }
  }

done:
  tbl_info->in_state_restore = false;
  return sts;
}

pipe_status_t pipe_mgr_idle_restore_state(bf_dev_id_t dev_id, cJSON *idle_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  idle_tbl_info_t *tbl_info = NULL;
  idle_tbl_t *pipe_info = NULL;
  idle_tbl_stage_info_t *stage_info = NULL;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t ent_hdl;
  uint32_t pipe_id, pipe_idx;
  uint32_t stage_idx, entry_idx;
  uint32_t init_ttl, curr_ttl;
  cJSON *pipe_tbls, *pipe_tbl, *stage_tbls, *stage_tbl;
  cJSON *idle_ents, *idle_ent, *idle_mdata, *idle_locs, *idle_loc;

  tbl_hdl = cJSON_GetObjectItem(idle_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR("%s:%d Idle table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbls = cJSON_GetObjectItem(idle_tbl, "pipe_tbls");
  for (pipe_tbl = pipe_tbls->child, pipe_idx = 0; pipe_tbl;
       pipe_tbl = pipe_tbl->next, pipe_idx++) {
    pipe_info = &tbl_info->idle_tbls[pipe_idx];
    pipe_id = (uint32_t)cJSON_GetObjectItem(pipe_tbl, "pipe_id")->valueint;
    PIPE_MGR_DBGCHK(pipe_info->pipe_id == pipe_id);
    if (IDLE_TBL_IS_SYMMETRIC(tbl_info)) {
      pipe_id = BF_DEV_PIPE_ALL;
    }
    stage_tbls = cJSON_GetObjectItem(pipe_tbl, "stage_tbls");
    for (stage_tbl = stage_tbls->child, stage_idx = 0; stage_tbl;
         stage_tbl = stage_tbl->next, stage_idx++) {
      stage_info = &pipe_info->stage_info[stage_idx];
      PIPE_MGR_DBGCHK(
          stage_info->stage_id ==
          (uint8_t)cJSON_GetObjectItem(stage_tbl, "stage_id")->valueint);
      idle_ents = cJSON_GetObjectItem(stage_tbl, "idle_ents");
      for (idle_ent = idle_ents->child; idle_ent; idle_ent = idle_ent->next) {
        ent_hdl = cJSON_GetObjectItem(idle_ent, "ent_hdl")->valuedouble;
        if (IDLE_TBL_IS_NOTIFY_MODE(tbl_info)) {
          init_ttl = cJSON_GetObjectItem(idle_ent, "init_ttl")->valuedouble;
          curr_ttl = cJSON_GetObjectItem(idle_ent, "cur_ttl")->valuedouble;
        } else {
          init_ttl = 0;
          curr_ttl = 0;
        }

        idle_mdata = cJSON_GetObjectItem(idle_ent, "mdata");
        idle_locs = cJSON_GetObjectItem(idle_mdata, "locs");
        for (idle_loc = idle_locs->child; idle_loc; idle_loc = idle_loc->next) {
          entry_idx = cJSON_GetObjectItem(idle_loc, "cur_index")->valuedouble;
          sts = rmt_idle_add_entry_internal(sess_hdl,
                                            tbl_info,
                                            ent_hdl,
                                            pipe_id,
                                            stage_info->stage_id,
                                            stage_info->stage_table_handle,
                                            entry_idx,
                                            init_ttl,
                                            curr_ttl,
                                            false,
                                            0);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Failed to add idle entry with hdl %d index %d"
                " to table 0x%x device %d",
                __func__,
                __LINE__,
                ent_hdl,
                entry_idx,
                tbl_hdl,
                dev_id);
            return sts;
          }
        }
      }
    }
    if (IDLE_TBL_IS_SYMMETRIC(tbl_info)) {
      break;
    }
  }

  return sts;
}

pipe_status_t rmt_idle_get_cookie(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t pipe_id,
                                  uint8_t stage_id,
                                  rmt_tbl_hdl_t stage_table_handle,
                                  idle_tbl_ha_cookie_t *cookie) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_stage_info_t *stage_info = NULL;

  idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
  if (!idle_tbl_info) {
    LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(idle_tbl_info->dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, idle_tbl_info->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  uint32_t idx = 0;

  if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_idle_tbl_notify_pipe_list_get(
        idle_tbl_info, pipe_id, &local_pipe_bmp);
    int pipe = -1;
    for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
         pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
      PIPE_MGR_DBGCHK(pipe < (int)dev_info->num_active_pipes);
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }
      cookie->stage_info[idx++] = stage_info;
    }
    cookie->num_stage_info = idx;
  } else {
    stage_info = pipe_mgr_idle_tbl_stage_info_get(
        idle_tbl_info, pipe_id, stage_id, stage_table_handle);
    if (!stage_info) {
      LOG_ERROR(
          "%s:%d - %s (%d - 0x%x) "
          "Idle table stage %d does not exist on pipe %d",
          __func__,
          __LINE__,
          idle_tbl_info->name,
          idle_tbl_info->dev_id,
          idle_tbl_info->tbl_hdl,
          stage_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    cookie->stage_info[0] = stage_info;
    cookie->num_stage_info = 1;
  }
  cookie->tbl_info = idle_tbl_info;
  cookie->pipe_id = pipe_id;
  cookie->stage_id = stage_id;
  return PIPE_SUCCESS;
}

pipe_status_t rmt_mat_ent_update_ttl(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     pipe_mat_ent_hdl_t ent_hdl,
                                     bf_dev_pipe_t pipe_id,
                                     uint8_t stage_id,
                                     rmt_tbl_hdl_t stage_table_handle,
                                     idle_tbl_ha_cookie_t *cookie,
                                     uint32_t ttl) {
  idle_tbl_info_t *idle_tbl_info = NULL;
  idle_tbl_stage_info_t *stage_info = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  if (cookie) {
    for (unsigned idx = 0; idx < cookie->num_stage_info; idx++) {
      status = pipe_mgr_idle_update_state_for_update_ttl(
          cookie->stage_info[idx], ent_hdl, ttl);
      if (status != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return status;
      }
    }
  } else {
    idle_tbl_info = pipe_mgr_idle_tbl_info_get(dev_id, tbl_hdl);
    if (!idle_tbl_info) {
      LOG_ERROR("%s:%d Table 0x%x on device %d does not exist",
                __func__,
                __LINE__,
                tbl_hdl,
                dev_id);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (IDLE_TBL_IS_NOTIFY_MODE(idle_tbl_info)) {
      pipe_bitmap_t local_pipe_bmp;
      PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
      pipe_mgr_idle_tbl_notify_pipe_list_get(
          idle_tbl_info, pipe_id, &local_pipe_bmp);
      int pipe = -1;
      for (pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe); pipe != -1;
           pipe = PIPE_BITMAP_GET_NEXT_BIT(&local_pipe_bmp, pipe)) {
        stage_info = pipe_mgr_idle_tbl_stage_info_get(
            idle_tbl_info, pipe, stage_id, stage_table_handle);
        if (!stage_info) {
          LOG_ERROR(
              "%s:%d - %s (%d - 0x%x) "
              "Idle table stage %d does not exist on pipe %d",
              __func__,
              __LINE__,
              idle_tbl_info->name,
              idle_tbl_info->dev_id,
              idle_tbl_info->tbl_hdl,
              stage_id,
              pipe);
          PIPE_MGR_DBGCHK(0);
          return PIPE_OBJ_NOT_FOUND;
        }
        status =
            pipe_mgr_idle_update_state_for_update_ttl(stage_info, ent_hdl, ttl);
        if (status != PIPE_SUCCESS) {
          PIPE_MGR_DBGCHK(0);
          return status;
        }
      }
    } else {
      stage_info = pipe_mgr_idle_tbl_stage_info_get(
          idle_tbl_info, pipe_id, stage_id, stage_table_handle);
      if (!stage_info) {
        LOG_ERROR(
            "%s:%d - %s (%d - 0x%x) "
            "Idle table stage %d does not exist on pipe %d",
            __func__,
            __LINE__,
            idle_tbl_info->name,
            idle_tbl_info->dev_id,
            idle_tbl_info->tbl_hdl,
            stage_id,
            pipe_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      status =
          pipe_mgr_idle_update_state_for_update_ttl(stage_info, ent_hdl, ttl);
      if (status != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
        return status;
      }
    }
  }
  return PIPE_SUCCESS;
}
