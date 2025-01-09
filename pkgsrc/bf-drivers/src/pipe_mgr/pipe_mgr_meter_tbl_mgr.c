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
 * @file pipe_mgr_meter_tbl_mgr.c
 * @date
 *
 * Meter table manager implementation.
 */

/* Standard header includes */
#include <float.h>

/* Module header includes */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_drv_workflows.h"
#include "pipe_mgr_meter_mgr_int.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_phy_mem_map.h"

typedef enum pipe_meter_data_type {
  METER_DATA_TYPE_METER = PIPE_METER_TYPE_STANDARD,
  METER_DATA_TYPE_LPF = PIPE_METER_TYPE_LPF,
  METER_DATA_TYPE_WRED = PIPE_METER_TYPE_WRED
} pipe_meter_data_type_e;

typedef struct pipe_mgr_meter_op_list_t {
  struct pipe_mgr_meter_op_list_t *next;
  bf_dev_pipe_t pipe_id;
  uint8_t stage_id;
  pipe_meter_idx_t meter_stage_idx;
  pipe_meter_data_type_e type;
  union {
    pipe_meter_spec_t meter_spec;
    pipe_lpf_spec_t lpf_spec;
    pipe_wred_spec_t wred_spec;
  } meter_spec;
} pipe_mgr_meter_op_list_t;

static pipe_mgr_meter_op_list_t *alloc_meter_op_list(
    pipe_mgr_meter_op_list_t *ml,
    pipe_meter_data_type_e meter_data_type,
    void *meter_spec) {
  pipe_mgr_meter_op_list_t *x =
      PIPE_MGR_MALLOC(sizeof(pipe_mgr_meter_op_list_t));
  if (x) {
    x->next = NULL;
    x->type = meter_data_type;
    switch (x->type) {
      case METER_DATA_TYPE_METER:
        x->meter_spec.meter_spec = *(pipe_meter_spec_t *)meter_spec;
        break;
      case METER_DATA_TYPE_LPF:
        x->meter_spec.lpf_spec = *(pipe_lpf_spec_t *)meter_spec;
        break;
      case METER_DATA_TYPE_WRED:
        x->meter_spec.wred_spec = *(pipe_wred_spec_t *)meter_spec;
        break;
    }
    if (ml) ml->next = x;
  }
  return x;
}

void pipe_mgr_meter_free_ops(struct pipe_mgr_meter_op_list_t **l) {
  struct pipe_mgr_meter_op_list_t *x = *l;
  while (x) {
    *l = x->next;
    PIPE_MGR_FREE(x);
    x = *l;
  }
}

static pipe_status_t pipe_mgr_meter_get_default_lpf_spec(pipe_lpf_spec_t *lpf) {
  pipe_lpf_spec_t lpf_spec = {0};
  lpf_spec.lpf_type = LPF_TYPE_SAMPLE;
  *lpf = lpf_spec;
  return PIPE_SUCCESS;
}
static pipe_status_t pipe_mgr_meter_get_default_wred_spec(
    pipe_wred_spec_t *wred) {
  wred->time_constant = 0;
  wred->red_min_threshold = 0xffffffff;
  wred->red_max_threshold = 0xffffffff;
  wred->max_probability = 1;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_meter_get_default_meter_spec(
    bf_dev_id_t dev_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_spec_t *meter) {
  pipe_meter_tbl_info_t *meter_tbl_info =
      pipe_mgr_get_meter_tbl_info(dev_id, meter_tbl_hdl, __func__, __LINE__);
  if (!meter_tbl_info) {
    LOG_ERROR("%s:%d Failed to find meter info for meter 0x%x device %d",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (meter_tbl_info->meter_type == PIPE_METER_TYPE_STANDARD) {
    meter->meter_type = meter_tbl_info->enable_color_aware
                            ? METER_TYPE_COLOR_AWARE
                            : METER_TYPE_COLOR_UNAWARE;
    switch (meter_tbl_info->meter_granularity) {
      case PIPE_METER_GRANULARITY_BYTES:
        meter->cir.type = METER_RATE_TYPE_KBPS;
        meter->cir.value.kbps = meter_tbl_info->max_rate;
        meter->pir.type = METER_RATE_TYPE_KBPS;
        meter->pir.value.kbps = meter_tbl_info->max_rate;
        break;
      case PIPE_METER_GRANULARITY_PACKETS:
        meter->cir.type = METER_RATE_TYPE_PPS;
        meter->cir.value.pps = meter_tbl_info->max_rate;
        meter->pir.type = METER_RATE_TYPE_PPS;
        meter->pir.value.pps = meter_tbl_info->max_rate;
        break;
      default:
        LOG_ERROR(
            "%s:%d Invalid meter granularity type %d for meter tbl 0x%x device "
            "%d",
            __func__,
            __LINE__,
            meter_tbl_info->meter_granularity,
            meter_tbl_hdl,
            dev_id);
        return PIPE_UNEXPECTED;
    }
    meter->cburst = meter_tbl_info->max_burst_size;
    meter->pburst = meter_tbl_info->max_burst_size;
  }
  return PIPE_SUCCESS;
}

/* API to reset a meter table */
pipe_status_t pipe_mgr_meter_mgr_meter_reset(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /*
   * Build a meter spec from the defaults.
   */
  pipe_meter_spec_t meter_spec = {0};
  pipe_status_t status = pipe_mgr_meter_get_default_meter_spec(
      dev_tgt.device_id, meter_tbl_hdl, &meter_spec);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Meter table instance 0x%x failed to build default"
        " meter spec",
        __func__,
        __LINE__,
        meter_tbl_hdl);
    return status;
  }

  /*
   * Range of meter IDs is num_entries per stage
   * Loop over all the stages and
   * Add this meter spec update to a move list.
   */
  unsigned meter_idx = 0;
  struct pipe_mgr_meter_op_list_t *ml_tail = NULL;
  for (uint8_t stage_idx = 0; stage_idx < meter_tbl_instance->num_stages;
       stage_idx++) {
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info;
    meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[stage_idx];
    stage_id = meter_tbl_stage_info->stage_id;
    for (meter_idx = 0; meter_idx < meter_tbl_stage_info->num_entries;
         meter_idx++) {
      struct pipe_mgr_meter_op_list_t *op =
          alloc_meter_op_list(ml_tail, METER_DATA_TYPE_METER, &meter_spec);
      ml_tail = op;
      op->pipe_id = dev_tgt.dev_pipe_id;
      op->stage_id = stage_id;
      op->meter_stage_idx = meter_idx;
      if (!(*head_p)) {
        *head_p = op;
      }
    }
  }
  return PIPE_SUCCESS;
}

/* API to update a meter entry specification */
pipe_status_t pipe_mgr_meter_mgr_meter_ent_set(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t meter_idx,
    pipe_meter_spec_t *meter_spec,
    pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_meter_stage_idx_t meter_stage_idx = 0;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Validate the index. */
  if (meter_tbl->num_entries <= meter_idx) {
    LOG_ERROR(
        "Dev %d Meter Tbl 0x%x (%s): requested index %d is out of range, only "
        "%d entries in table",
        meter_tbl->dev_info->dev_id,
        meter_tbl->meter_tbl_hdl,
        meter_tbl->name,
        meter_idx,
        meter_tbl->num_entries);
    return PIPE_INVALID_ARG;
  }

  /* Add this meter spec update to a move list.
   */
  unsigned i = 0;
  if (meter_tbl->over_allocated) {
    struct pipe_mgr_meter_op_list_t *ml_tail = NULL;

    /* The meter table is indirect.  If it is multi-stage (but it shouldn't be)
     * then the index must be updated in each stage. */
    for (i = 0; i < meter_tbl_instance->num_stages; i++) {
      meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[i];
      if (meter_tbl_stage_info == NULL) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      meter_stage_idx = meter_idx - meter_tbl_stage_info->ent_idx_offset;

      struct pipe_mgr_meter_op_list_t *op =
          alloc_meter_op_list(ml_tail, METER_DATA_TYPE_METER, meter_spec);
      ml_tail = op;
      op->pipe_id = dev_tgt.dev_pipe_id;
      op->stage_id = meter_tbl_stage_info->stage_id;
      op->meter_stage_idx = meter_stage_idx;
      if (!(*head_p)) {
        *head_p = op;
      }
    }
  } else {
    stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, meter_idx);

    struct pipe_mgr_meter_op_list_t *op =
        alloc_meter_op_list(NULL, METER_DATA_TYPE_METER, meter_spec);
    op->pipe_id = dev_tgt.dev_pipe_id;
    op->stage_id = stage_id;
    op->meter_stage_idx = meter_idx;
    *head_p = op;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_lpf_reset(dev_target_t dev_tgt,
                                           pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                                           pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, lpf_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              lpf_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_LPF) {
    LOG_ERROR("%s:%d Table 0x%x, device %d is not a LPF table",
              __func__,
              __LINE__,
              lpf_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(meter_tbl->type == PIPE_METER_TYPE_LPF);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        lpf_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /*
   * Build a lpf spec as default. Default type is SAMPLE.
   */
  pipe_lpf_spec_t lpf_spec;
  pipe_mgr_meter_get_default_lpf_spec(&lpf_spec);

  /*
   * Range of lpf IDs is num_entries
   * Loop over all the entries and
   * Add this lpf spec update to a move list.
   */
  unsigned lpf_idx = 0;
  struct pipe_mgr_meter_op_list_t *ml_tail = NULL;

  for (lpf_idx = 0; lpf_idx < meter_tbl->num_entries; lpf_idx++) {
    stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, lpf_idx);

    struct pipe_mgr_meter_op_list_t *op =
        alloc_meter_op_list(ml_tail, METER_DATA_TYPE_LPF, &lpf_spec);
    ml_tail = op;
    op->pipe_id = dev_tgt.dev_pipe_id;
    op->stage_id = stage_id;
    op->meter_stage_idx = lpf_idx;
    if (!(*head_p)) {
      *head_p = op;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_lpf_ent_set(
    dev_target_t dev_tgt,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    pipe_lpf_idx_t lpf_idx,
    pipe_lpf_spec_t *lpf_spec,
    pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, lpf_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              lpf_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_LPF) {
    LOG_ERROR("%s:%d Table 0x%x, device %d is not a LPF table",
              __func__,
              __LINE__,
              lpf_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        lpf_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* First validate the index. */
  if (meter_tbl->num_entries <= lpf_idx) {
    LOG_ERROR(
        "Dev %d LPF Tbl 0x%x (%s): requested index %d is out of range, only "
        "%d entries in table",
        meter_tbl->dev_info->dev_id,
        meter_tbl->meter_tbl_hdl,
        meter_tbl->name,
        lpf_idx,
        meter_tbl->num_entries);
    return PIPE_INVALID_ARG;
  }

  unsigned i = 0;
  if (meter_tbl->over_allocated) {
    /* If the meter table is replicated in every stage (this happens when
     * a match table which is indirectly referencing the meter table is
     * in more than one stage, then a given meter index corresponds to an
     * entry index in every stage.
     */

    struct pipe_mgr_meter_op_list_t *ml_tail = NULL;

    for (i = 0; i < meter_tbl_instance->num_stages; i++) {
      struct pipe_mgr_meter_op_list_t *op =
          alloc_meter_op_list(ml_tail, METER_DATA_TYPE_LPF, lpf_spec);
      ml_tail = op;
      op->pipe_id = dev_tgt.dev_pipe_id;
      op->stage_id = meter_tbl_instance->meter_tbl_stage_info[i].stage_id;
      op->meter_stage_idx = lpf_idx;
      if (!(*head_p)) {
        *head_p = op;
      }
    }
  } else {
    stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, lpf_idx);

    struct pipe_mgr_meter_op_list_t *op =
        alloc_meter_op_list(NULL, METER_DATA_TYPE_LPF, lpf_spec);
    op->pipe_id = dev_tgt.dev_pipe_id;
    op->stage_id = stage_id;
    op->meter_stage_idx = lpf_idx;
    *head_p = op;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_wred_reset(dev_target_t dev_tgt,
                                            pipe_wred_tbl_hdl_t wred_tbl_hdl,
                                            pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, wred_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              wred_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_WRED) {
    LOG_ERROR("%s:%d Table 0x%x, device %d is not a WRED table",
              __func__,
              __LINE__,
              wred_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(meter_tbl->type == PIPE_METER_TYPE_WRED);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        wred_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /*
   * Build a wred spec from the defaults.
   */
  pipe_wred_spec_t wred_spec = {0};
  pipe_mgr_meter_get_default_wred_spec(&wred_spec);

  /*
   * Range of wred IDs is num_entries
   * Loop over all the entries and
   * Add this wred spec update to a move list.
   */
  unsigned wred_idx = 0;
  struct pipe_mgr_meter_op_list_t *ml_tail = NULL;

  for (wred_idx = 0; wred_idx < meter_tbl->num_entries; wred_idx++) {
    stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, wred_idx);

    struct pipe_mgr_meter_op_list_t *op =
        alloc_meter_op_list(ml_tail, METER_DATA_TYPE_WRED, &wred_spec);
    ml_tail = op;
    op->pipe_id = dev_tgt.dev_pipe_id;
    op->stage_id = stage_id;
    op->meter_stage_idx = wred_idx;
    if (!(*head_p)) {
      *head_p = op;
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_wred_ent_set(
    dev_target_t dev_tgt,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    pipe_wred_idx_t wred_idx,
    pipe_wred_spec_t *wred_spec,
    pipe_mgr_meter_op_list_t **head_p) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, wred_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              wred_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_WRED) {
    LOG_ERROR("%s:%d Table 0x%x, device %d is not a WRED table",
              __func__,
              __LINE__,
              wred_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        wred_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* First validate the index. */
  if (meter_tbl->num_entries <= wred_idx) {
    LOG_ERROR(
        "Dev %d WRED Tbl 0x%x (%s): requested index %d is out of range, only "
        "%d entries in table",
        meter_tbl->dev_info->dev_id,
        meter_tbl->meter_tbl_hdl,
        meter_tbl->name,
        wred_idx,
        meter_tbl->num_entries);
    return PIPE_INVALID_ARG;
  }

  unsigned i = 0;
  if (meter_tbl->over_allocated) {
    /* If the meter table is replicated in every stage (this happens when
     * a match table which is indirectly referencing the meter table is
     * in more than one stage, then a given meter index corresponds to an
     * entry index in every stage.
     */

    struct pipe_mgr_meter_op_list_t *ml_tail = NULL;

    for (i = 0; i < meter_tbl_instance->num_stages; i++) {
      struct pipe_mgr_meter_op_list_t *op =
          alloc_meter_op_list(ml_tail, METER_DATA_TYPE_WRED, wred_spec);
      ml_tail = op;
      op->pipe_id = dev_tgt.dev_pipe_id;
      op->stage_id = meter_tbl_instance->meter_tbl_stage_info[i].stage_id;
      op->meter_stage_idx = wred_idx;
      if (!(*head_p)) {
        *head_p = op;
      }
    }
  } else {
    stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, wred_idx);

    struct pipe_mgr_meter_op_list_t *op =
        alloc_meter_op_list(NULL, METER_DATA_TYPE_WRED, wred_spec);
    op->pipe_id = dev_tgt.dev_pipe_id;
    op->stage_id = stage_id;
    op->meter_stage_idx = wred_idx;
    *head_p = op;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t save_one_meter_op(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_mgr_meter_op_list_t *op) {
  unsigned long key = op->stage_id;
  key = (key << 24) | op->meter_stage_idx;
  pipe_mgr_meter_op_list_t *x = PIPE_MGR_MALLOC(sizeof *x);
  if (!x) return PIPE_NO_SYS_RESOURCES;
  *x = *op;
  x->next = NULL;
  bf_map_sts_t s = bf_map_add(&meter_tbl_instance->replayed, key, x);
  if (BF_MAP_KEY_EXISTS == s) {
    /* If an index is set multiple times keep only the most recent spec. */
    pipe_mgr_meter_op_list_t *old = NULL;
    bf_map_get_rmv(&meter_tbl_instance->replayed, key, (void **)&old);
    if (old) PIPE_MGR_FREE(old);
    s = bf_map_add(&meter_tbl_instance->replayed, key, x);
  }
  if (BF_MAP_OK != s) {
    return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}
static pipe_status_t pop_one_meter_op(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    dev_stage_t stage,
    pipe_meter_idx_t idx,
    pipe_mgr_meter_op_list_t *op) {
  pipe_mgr_meter_op_list_t *x = NULL;
  unsigned long key = stage;
  key = (key << 24) | idx;
  bf_map_sts_t sts =
      bf_map_get_rmv(&meter_tbl_instance->replayed, key, (void **)&x);
  if (sts == BF_MAP_NO_KEY)
    return PIPE_OBJ_NOT_FOUND;
  else if (sts != BF_MAP_OK)
    return PIPE_UNEXPECTED;
  *op = *x;
  PIPE_MGR_FREE(x);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_process_op_list(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_id_t dev_id,
                                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                             pipe_mgr_meter_op_list_t *ml,
                                             uint32_t *success_count) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  uint8_t stage_id = 0;
  uint32_t blk_idx = 0;
  vpn_id_t vpn = 0;
  mem_id_t ram_id = 0;
  uint32_t ram_line_num = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (; ml; ml = ml->next) {
    meter_tbl_instance =
        pipe_mgr_meter_tbl_get_instance(meter_tbl, ml->pipe_id);
    if (meter_tbl_instance == NULL) {
      LOG_ERROR(
          "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
          " not found",
          __func__,
          __LINE__,
          meter_tbl_hdl,
          dev_id,
          ml->pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    meter_tbl_stage_info =
        pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, ml->stage_id);
    if (meter_tbl_stage_info == NULL) {
      LOG_ERROR(
          "%s:%d Meter table stage info for tbl 0x%x stage id %d"
          " not found",
          __func__,
          __LINE__,
          meter_tbl_hdl,
          stage_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (ml->meter_stage_idx >= meter_tbl_stage_info->num_entries) {
      LOG_ERROR(
          "%s:%d Meter table 0x%x (%s) stage %d requested idx %d out of range, "
          "stage size %d",
          __func__,
          __LINE__,
          meter_tbl_hdl,
          meter_tbl->name,
          stage_id,
          ml->meter_stage_idx,
          meter_tbl_stage_info->num_entries);
      return PIPE_INVALID_ARG;
    }

    if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
      sts = save_one_meter_op(meter_tbl_instance, ml);
    } else {
      ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
      blk_idx = ml->meter_stage_idx / TOF_SRAM_UNIT_DEPTH;
      vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
      ram_line_num = ml->meter_stage_idx % TOF_SRAM_UNIT_DEPTH;
      ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];

      switch (ml->type) {
        case METER_DATA_TYPE_METER:
          sts = pipe_mgr_meter_spec_update_drv_workflow(
              sess_hdl,
              dev_id,
              meter_tbl,
              meter_tbl_instance->pipe_bmp,
              meter_tbl_stage_info,
              vpn,
              ram_id,
              ram_line_num,
              ml->meter_stage_idx,
              &(ml->meter_spec.meter_spec));
          break;
        case METER_DATA_TYPE_LPF:
          sts = pipe_mgr_lpf_spec_update_drv_workflow(
              sess_hdl,
              dev_id,
              meter_tbl,
              meter_tbl_instance->pipe_bmp,
              meter_tbl_stage_info->stage_table_handle,
              meter_tbl_stage_info->stage_id,
              vpn,
              ram_id,
              ram_line_num,
              &(ml->meter_spec.lpf_spec));
          break;
        case METER_DATA_TYPE_WRED:
          sts = pipe_mgr_wred_spec_update_drv_workflow(
              sess_hdl,
              dev_id,
              meter_tbl,
              meter_tbl_instance->pipe_bmp,
              meter_tbl_stage_info->stage_table_handle,
              meter_tbl_stage_info->stage_id,
              vpn,
              ram_id,
              ram_line_num,
              &(ml->meter_spec.wred_spec));
          break;
      }
    }

    if (PIPE_SUCCESS != sts) {
      break;
    }
    (*success_count)++;
  }
  return sts;
}

static pipe_status_t meter_prog_table_from_replay(
    pipe_sess_hdl_t shdl, bf_dev_id_t dev_id, pipe_meter_tbl_hdl_t tbl_hdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Skip direct meter tables, they are handled as part of the match table
   * reconcilation. */
  if (meter_tbl->ref_type == PIPE_TBL_REF_TYPE_DIRECT) return PIPE_SUCCESS;

  pipe_mgr_meter_op_list_t dflt = {0};
  dflt.type = (pipe_meter_data_type_e)meter_tbl->type;
  if (meter_tbl->type == PIPE_METER_TYPE_STANDARD) {
    pipe_mgr_meter_get_default_meter_spec(
        dev_id, tbl_hdl, &dflt.meter_spec.meter_spec);
  } else if (meter_tbl->type == PIPE_METER_TYPE_WRED) {
    pipe_mgr_meter_get_default_wred_spec(&dflt.meter_spec.wred_spec);
  } else if (meter_tbl->type == PIPE_METER_TYPE_LPF) {
    pipe_mgr_meter_get_default_lpf_spec(&dflt.meter_spec.lpf_spec);
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  for (unsigned i = 0; i < meter_tbl->num_tbl_instances; i++) {
    pipe_mgr_meter_tbl_instance_t *tbl_instance;
    tbl_instance = &meter_tbl->meter_tbl_instances[i];
    dflt.pipe_id = tbl_instance->pipe_id;
    for (unsigned s = 0; s < tbl_instance->num_stages; ++s) {
      pipe_mgr_meter_tbl_stage_info_t *stage_info;
      stage_info = &tbl_instance->meter_tbl_stage_info[s];
      dflt.stage_id = stage_info->stage_id;
      for (unsigned e = 0; e < stage_info->num_entries; ++e) {
        pipe_mgr_meter_op_list_t op;
        sts = pop_one_meter_op(tbl_instance, stage_info->stage_id, e, &op);
        if (PIPE_SUCCESS == sts) {
          uint32_t unused;
          sts = pipe_mgr_meter_process_op_list(
              shdl, dev_id, tbl_hdl, &op, &unused);
          if (PIPE_SUCCESS != sts) return sts;
        } else if (PIPE_OBJ_NOT_FOUND == sts) {
          uint32_t unused;
          /* Program a default spec meter for anything not replayed. */
          dflt.meter_stage_idx = e;
          sts = pipe_mgr_meter_process_op_list(
              shdl, dev_id, tbl_hdl, &dflt, &unused);
          if (PIPE_SUCCESS != sts) return sts;
        } else {
          return sts;
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_download_specs_from_shadow(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *dev_profile_info,
    pipe_meter_tbl_info_t *meter_tbl_info,
    void *arg) {
  (void)dev_profile_info;
  pipe_sess_hdl_t shdl = *(pipe_sess_hdl_t *)arg;
  return meter_prog_table_from_replay(
      shdl, dev_info->dev_id, meter_tbl_info->handle);
}

pipe_status_t pipe_mgr_meter_verify_idx(bf_dev_id_t device_id,
                                        bf_dev_pipe_t pipe_id,
                                        pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                        pipe_meter_idx_t meter_idx) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR("%s:%d Meter table instance for tbl 0x%x, pipe id %d, not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_idx >= meter_tbl_instance->num_entries) {
    LOG_ERROR(
        "%s:%d  Meter idx %d passed exceeds the number of entries %d, for "
        "meter tbl 0x%x, device id %d, pipe id %d",
        __func__,
        __LINE__,
        meter_idx,
        meter_tbl_instance->num_entries,
        meter_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_meter_mgr_meter_attach(bf_dev_id_t device_id,
                                         bf_dev_pipe_t pipe_id,
                                         uint8_t stage_id,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         pipe_meter_idx_t meter_idx,
                                         rmt_virt_addr_t *ent_virt_addr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_meter_stage_idx_t meter_stage_idx = 0;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t blk_idx = 0;
  bool color_aware = false;
  uint32_t ram_line_num = 0;
  vpn_id_t vpn = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl %d, device id %d not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR("%s:%d Meter table instance for tbl %d, pipe id %d, not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_idx >= meter_tbl_instance->num_entries) {
    LOG_ERROR(
        "%s:%d  Meter idx %d passed exceeds the number of entries %d, for  "
        "meter tbl 0x%x, device id %d, pipe id %d, stage id %d",
        __func__,
        __LINE__,
        meter_idx,
        meter_tbl_instance->num_entries,
        meter_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_INVALID_ARG;
  }

  if (meter_tbl->enable_color_aware == true) {
    status = pipe_mgr_meter_mgr_get_color_aware(
        meter_tbl, pipe_id, meter_idx, &color_aware);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting color aware flag for meter idx %d"
          " meter tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          meter_idx,
          meter_tbl_hdl,
          device_id,
          pipe_str_err(status));
      return status;
    }
  } else {
    color_aware = false;
  }

  stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, meter_idx);

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;

  PIPE_MGR_DBGCHK(meter_idx >= meter_tbl_stage_info->ent_idx_offset);
  meter_stage_idx = meter_idx - meter_tbl_stage_info->ent_idx_offset;
  blk_idx = meter_stage_idx / TOF_SRAM_UNIT_DEPTH;
  vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
  ram_line_num = meter_stage_idx % TOF_SRAM_UNIT_DEPTH;

  /* Form the entry virtual address to be returned */
  if (ent_virt_addr != NULL) {
    *ent_virt_addr = pipe_mgr_meter_mgr_construct_virt_addr(
        meter_tbl, vpn, ram_line_num, color_aware);
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_meter_mgr_direct_meter_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t meter_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_meter_spec_t *meter_spec,
    rmt_virt_addr_t *ent_virt_addr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t blk_idx = 0;
  vpn_id_t vpn = 0;
  mem_id_t ram_id = 0;
  uint32_t ram_line_num = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for device id %d, tbl handle %d not found",
              __func__,
              __LINE__,
              device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter table instance for pipe id %d, tbl %d, device id %d"
        " not found",
        __func__,
        __LINE__,
        pipe_id,
        meter_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter table stage info not found for stage id %d table %d"
        " device id %d",
        __func__,
        __LINE__,
        stage_id,
        meter_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;

  blk_idx = meter_stage_idx / TOF_SRAM_UNIT_DEPTH;
  vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
  ram_line_num = meter_stage_idx % TOF_SRAM_UNIT_DEPTH;
  ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];

  /* Form the entry virtual address to be returned */
  if (ent_virt_addr != NULL) {
    *ent_virt_addr = pipe_mgr_meter_mgr_construct_virt_addr(
        meter_tbl, vpn, ram_line_num, meter_tbl->enable_color_aware);
  }
  status = pipe_mgr_meter_spec_update_drv_workflow(sess_hdl,
                                                   device_id,
                                                   meter_tbl,
                                                   meter_tbl_instance->pipe_bmp,
                                                   meter_tbl_stage_info,
                                                   vpn,
                                                   ram_id,
                                                   ram_line_num,
                                                   meter_stage_idx,
                                                   meter_spec);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating the meter spec for table %#x,"
        " index %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        meter_stage_idx,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_meter_mgr_direct_lpf_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    pipe_lpf_stage_idx_t lpf_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_lpf_spec_t *lpf_spec,
    rmt_virt_addr_t *ent_virt_addr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_lpf_idx_t lpf_idx = 0;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t blk_idx = 0;
  vpn_id_t vpn = 0;
  uint32_t ram_line_num = 0;
  mem_id_t ram_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, lpf_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for device id %d, tbl handle %d not found",
              __func__,
              __LINE__,
              device_id,
              lpf_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_LPF) {
    LOG_ERROR("%s:%d Table %d, device id %d is not an LPF table",
              __func__,
              __LINE__,
              lpf_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_LPF) {
    LOG_ERROR("%s:%d Table 0x%x, device %d is not a LPF table",
              __func__,
              __LINE__,
              lpf_tbl_hdl,
              device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter table instance for pipe id %d, tbl %d, device id %d"
        " not found",
        __func__,
        __LINE__,
        pipe_id,
        lpf_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter table stage info not found for stage id %d table %d"
        " device id %d",
        __func__,
        __LINE__,
        stage_id,
        lpf_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  lpf_idx = meter_tbl_stage_info->ent_idx_offset + lpf_stage_idx;

  blk_idx = lpf_idx / TOF_SRAM_UNIT_DEPTH;
  vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
  ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
  ram_line_num = lpf_idx % TOF_SRAM_UNIT_DEPTH;

  /* Form the entry virtual address to be returned */
  if (ent_virt_addr != NULL) {
    *ent_virt_addr = pipe_mgr_meter_mgr_construct_virt_addr(
        meter_tbl, vpn, ram_line_num, false);
  }

  status = pipe_mgr_lpf_spec_update_drv_workflow(
      sess_hdl,
      device_id,
      meter_tbl,
      meter_tbl_instance->pipe_bmp,
      meter_tbl_stage_info->stage_table_handle,
      stage_id,
      vpn,
      ram_id,
      ram_line_num,
      lpf_spec);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating the lpf spec for table %#x,"
        " index %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        lpf_tbl_hdl,
        lpf_idx,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t rmt_meter_mgr_direct_wred_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    pipe_wred_stage_idx_t wred_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_wred_spec_t *wred_spec,
    rmt_virt_addr_t *ent_virt_addr) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_wred_idx_t wred_idx = 0;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t blk_idx = 0;
  vpn_id_t vpn = 0;
  mem_id_t ram_id = 0;
  uint32_t ram_line_num = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, wred_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for device id %d, tbl handle %d not found",
              __func__,
              __LINE__,
              device_id,
              wred_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_WRED) {
    LOG_ERROR("%s:%d Table %d, device id %d is not an WRED table",
              __func__,
              __LINE__,
              wred_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->type != PIPE_METER_TYPE_WRED) {
    LOG_ERROR("%s:%d Table %d, device id %d is not an WRED table",
              __func__,
              __LINE__,
              wred_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter table instance for pipe id %d, tbl %d, device id %d"
        " not found",
        __func__,
        __LINE__,
        pipe_id,
        wred_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter table stage info not found for stage id %d table %d"
        " device id %d",
        __func__,
        __LINE__,
        stage_id,
        wred_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  wred_idx = meter_tbl_stage_info->ent_idx_offset + wred_stage_idx;

  blk_idx = wred_idx / TOF_SRAM_UNIT_DEPTH;
  vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
  ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
  ram_line_num = wred_idx % TOF_SRAM_UNIT_DEPTH;

  /* Form the entry virtual address to be returned */
  if (ent_virt_addr != NULL) {
    *ent_virt_addr = pipe_mgr_meter_mgr_construct_virt_addr(
        meter_tbl, vpn, ram_line_num, false);
  }
  status = pipe_mgr_wred_spec_update_drv_workflow(
      sess_hdl,
      device_id,
      meter_tbl,
      meter_tbl_instance->pipe_bmp,
      meter_tbl_stage_info->stage_table_handle,
      meter_tbl_stage_info->stage_id,
      vpn,
      ram_id,
      ram_line_num,
      wred_spec);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating the wred spec for table 0x%x,"
        " index %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        wred_tbl_hdl,
        wred_idx,
        pipe_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_move_meter_spec(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t pipe_id,
    uint8_t src_stage_id,
    uint8_t dst_stage_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t src_idx,
    pipe_meter_stage_idx_t dst_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *src_meter_tbl_stage_info = NULL;
  pipe_mgr_meter_tbl_stage_info_t *dst_meter_tbl_stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *src_ram_alloc_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *dst_ram_alloc_info = NULL;
  pipe_meter_idx_t src_meter_idx = 0;
  pipe_meter_idx_t dst_meter_idx = 0;
  uint32_t src_blk_idx = 0;
  uint32_t dst_blk_idx = 0;
  mem_id_t src_ram_id = 0;
  mem_id_t dst_ram_id = 0;
  uint32_t src_ram_line_num = 0;
  uint32_t dst_ram_line_num = 0;
  vpn_id_t dst_vpn = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for device id %d, tbl handle %d not found",
              __func__,
              __LINE__,
              device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter table instance for pipe id %d, tbl %d, device id %d"
        " not found",
        __func__,
        __LINE__,
        pipe_id,
        meter_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  src_meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, src_stage_id);

  if (src_meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter table stage info not found for stage id %d table %d"
        " device id %d",
        __func__,
        __LINE__,
        src_stage_id,
        meter_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  dst_meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, dst_stage_id);

  if (dst_meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter table stage info not found for stage id %d table %d"
        " device id %d",
        __func__,
        __LINE__,
        dst_stage_id,
        meter_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  src_meter_idx = src_meter_tbl_stage_info->ent_idx_offset + src_idx;
  dst_meter_idx = dst_meter_tbl_stage_info->ent_idx_offset + dst_idx;

  src_ram_alloc_info = src_meter_tbl_stage_info->ram_alloc_info;
  /* First compute source location params */
  src_blk_idx = src_idx / TOF_SRAM_UNIT_DEPTH;
  src_ram_id = src_ram_alloc_info->tbl_word_blk[src_blk_idx].mem_id[0];
  src_ram_line_num = src_idx % TOF_SRAM_UNIT_DEPTH;

  /* Next, compute destination location params */
  dst_ram_alloc_info = dst_meter_tbl_stage_info->ram_alloc_info;
  dst_blk_idx = dst_idx / TOF_SRAM_UNIT_DEPTH;
  dst_ram_id = dst_ram_alloc_info->tbl_word_blk[dst_blk_idx].mem_id[0];
  dst_vpn = dst_ram_alloc_info->tbl_word_blk[dst_blk_idx].vpn_id[0];
  dst_ram_line_num = dst_idx % TOF_SRAM_UNIT_DEPTH;

  status = pipe_mgr_meter_spec_move_drv_workflow(
      sess_hdl,
      device_id,
      meter_tbl,
      meter_tbl_instance->pipe_bmp,
      src_stage_id,
      dst_stage_id,
      src_ram_id,
      src_ram_line_num,
      dst_ram_id,
      dst_ram_line_num,
      dst_vpn,
      dst_meter_tbl_stage_info->stage_table_handle);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in moving meter spec from idx %d to %d, pipe id %d"
        " src stage id %d, dst stage id %d meter tbl 0x%x, device id %d,"
        " err %s",
        __func__,
        __LINE__,
        src_meter_idx,
        dst_meter_idx,
        pipe_id,
        src_stage_id,
        dst_stage_id,
        meter_tbl_hdl,
        device_id,
        pipe_str_err(status));
    return status;
  }
  return status;
}

pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get(
    bf_dev_id_t device_id, pipe_meter_tbl_hdl_t meter_tbl_hdl) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  if (device_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(device_id < PIPE_MGR_NUM_DEVICES);
    return NULL;
  }

  key = meter_tbl_hdl;

  map_sts = pipe_mgr_meter_tbl_map_get(device_id, key, (void **)&meter_tbl);

  if (map_sts != BF_MAP_OK) {
    return NULL;
  }

  return meter_tbl;
}

pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get_first(bf_dev_id_t device_id) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  if (device_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(device_id < PIPE_MGR_NUM_DEVICES);
    return NULL;
  }

  map_sts =
      pipe_mgr_meter_tbl_map_get_first(device_id, &key, (void **)&meter_tbl);

  if (map_sts != BF_MAP_OK) {
    return NULL;
  }

  return meter_tbl;
}

pipe_mgr_meter_tbl_t *pipe_mgr_meter_tbl_get_next(pipe_mgr_meter_tbl_t *tbl) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = tbl->meter_tbl_hdl;
  bf_dev_id_t dev_id = tbl->dev_info->dev_id;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(dev_id < PIPE_MGR_NUM_DEVICES);
    return NULL;
  }

  map_sts = pipe_mgr_meter_tbl_map_get_next(dev_id, &key, (void **)&meter_tbl);

  if (map_sts != BF_MAP_OK) {
    return NULL;
  }

  return meter_tbl;
}

pipe_mgr_meter_tbl_instance_t *pipe_mgr_meter_tbl_get_instance(
    pipe_mgr_meter_tbl_t *meter_tbl, bf_dev_pipe_t pipe_id) {
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;

  if (meter_tbl == NULL) {
    return NULL;
  }

  if (meter_tbl->symmetric == true) {
    if ((pipe_id != BF_DEV_PIPE_ALL) || (meter_tbl->num_tbl_instances != 1)) {
      return NULL;
    }
    return meter_tbl->meter_tbl_instances;
  }

  unsigned i = 0;
  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    meter_tbl_instance = &meter_tbl->meter_tbl_instances[i];

    if (meter_tbl_instance->pipe_id == pipe_id) {
      return meter_tbl_instance;
    }
  }

  return NULL;
}

static pipe_mgr_meter_tbl_instance_t *meter_tbl_get_instance_from_any_pipe(
    pipe_mgr_meter_tbl_t *meter_tbl, bf_dev_pipe_t pipe_id) {
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;

  if (meter_tbl == NULL) {
    return NULL;
  }

  if (meter_tbl->symmetric == true) {
    if (meter_tbl->num_tbl_instances != 1) {
      return NULL;
    }
    if (pipe_id == BF_DEV_PIPE_ALL) {
      return meter_tbl->meter_tbl_instances;
    }
  }

  unsigned i = 0;
  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    meter_tbl_instance = &meter_tbl->meter_tbl_instances[i];
    if (PIPE_BITMAP_GET(&meter_tbl_instance->pipe_bmp, pipe_id)) {
      return meter_tbl_instance;
    }
  }

  return NULL;
}

pipe_mgr_meter_tbl_stage_info_t *pipe_mgr_meter_tbl_get_stage_info(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance, uint8_t stage_id) {
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  unsigned i = 0;

  for (i = 0; i < meter_tbl_instance->num_stages; i++) {
    meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[i];

    if (meter_tbl_stage_info->stage_id == stage_id) {
      return meter_tbl_stage_info;
    }
  }
  return NULL;
}

uint8_t pipe_mgr_meter_mgr_get_stage(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_meter_idx_t meter_idx) {
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  unsigned i = 0;

  PIPE_MGR_DBGCHK(meter_idx < meter_tbl_instance->num_entries);

  if (meter_idx >= meter_tbl_instance->num_entries) {
    return 0xff;
  }

  for (i = 0; i < meter_tbl_instance->num_stages; i++) {
    meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[i];

    if (meter_idx < (meter_tbl_stage_info->ent_idx_offset +
                     meter_tbl_stage_info->num_entries)) {
      return meter_tbl_stage_info->stage_id;
    }
  }

  return 0xff;
}

rmt_virt_addr_t pipe_mgr_meter_mgr_construct_virt_addr(
    pipe_mgr_meter_tbl_t *meter_tbl,
    vpn_id_t vpn,
    uint32_t ram_line_num,
    bool color_aware) {
  rmt_virt_addr_t addr = 0;
  bool lpf_red = false;

  addr |= ((ram_line_num & 0x3ff) << TOF_METER_NUM_LOWER_HUFFMAN_BITS);
  addr |= ((vpn & 0x3f) << (10 + TOF_METER_NUM_LOWER_HUFFMAN_BITS));

  /* Always encode the per-flow bit. When this address is encoded into the
   * match overhead, based on whether the compiler has defaulted the per-flow
   * bit or not, it will be included/excluded in the overhead.
   * excluding the default lower huffman bits added by the compiler. The
   * per-flow bit is required to be encoded for default entries.
   */
  addr |= (1 << TOF_METER_ADDR_PFE_BIT_POSITION);

  if (meter_tbl->enable_color_aware == true) {
    if (color_aware == true) {
      addr |= (TOF_METER_ADDR_TYPE_METER_TYPE_COLOR_AWARE
               << TOF_METER_ADDR_METER_TYPE_BIT_POSITION);
    } else {
      addr |= (TOF_METER_ADDR_TYPE_METER_TYPE_LPF_COLOR_BLIND
               << TOF_METER_ADDR_METER_TYPE_BIT_POSITION);
    }
  } else {
    addr |= (TOF_METER_ADDR_TYPE_METER_TYPE_LPF_COLOR_BLIND
             << TOF_METER_ADDR_METER_TYPE_BIT_POSITION);
  }

  if (meter_tbl->type == PIPE_METER_TYPE_LPF ||
      meter_tbl->type == PIPE_METER_TYPE_WRED) {
    lpf_red = true;
  }
  /* Set the MSB bits of the address to indicate LPF */
  if (lpf_red == true) {
    addr |= (TOF_METER_ADDR_TYPE_METER_TYPE_LPF_COLOR_BLIND
             << TOF_METER_ADDR_METER_TYPE_BIT_POSITION);
  }

  return addr;
}

pipe_status_t pipe_mgr_meter_mgr_construct_disabled_virt_addr(
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    rmt_virt_addr_t *virt_addr) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table for device id %d, table handle %d not found",
              __func__,
              __LINE__,
              device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->enable_per_flow_enable == false) {
    LOG_ERROR(
        "%s:%d Attempt to generate a disabled meter entry address for table for"
        " which per flow is not enabled, device id %d, table handle %d",
        __func__,
        __LINE__,
        device_id,
        meter_tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  *virt_addr |= ~(1 << meter_tbl->per_flow_enable_bit_position);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_read_dir_entry(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_ent_hdl_t entry_hdl,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    bool from_hw,
    pipe_res_get_data_t *res_data) {
  pipe_status_t ret = PIPE_SUCCESS;
  uint32_t subindex = 0;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  uint32_t entry_idx;
  ret = pipe_mgr_mat_ent_get_dir_ent_location(dev_id,
                                              mat_tbl_hdl,
                                              entry_hdl,
                                              subindex,
                                              &pipe_id,
                                              &stage_id,
                                              NULL,
                                              &entry_idx);
  if (PIPE_SUCCESS != ret) {
    LOG_ERROR("%s: Dev %d Error getting entry %d location %d from MAT 0x%x, %s",
              __func__,
              dev_id,
              entry_hdl,
              subindex,
              mat_tbl_hdl,
              pipe_str_err(ret));
    return ret;
  }

  pipe_mgr_meter_tbl_t *meter_tbl =
      pipe_mgr_meter_tbl_get(dev_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_meter_spec_type_e tbl_type = SPEC_TYPE_METER;
  switch (meter_tbl->type) {
    case PIPE_METER_TYPE_STANDARD:
      tbl_type = SPEC_TYPE_METER;
      break;
    case PIPE_METER_TYPE_LPF:
      tbl_type = SPEC_TYPE_LPF;
      break;
    case PIPE_METER_TYPE_WRED:
      tbl_type = SPEC_TYPE_WRED;
      break;
  }
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev_id;
  dev_tgt.dev_pipe_id = pipe_id;
  pipe_res_data_spec_t res_spec;
  ret = pipe_mgr_meter_mgr_read_entry_in_stage(dev_tgt,
                                               meter_tbl_hdl,
                                               stage_id,
                                               entry_idx,
                                               &res_spec,
                                               tbl_type,
                                               from_hw);
  if (PIPE_SUCCESS != ret) {
    LOG_ERROR(
        "%s: Dev %d Error reading direct meter for entry %d in table 0x%x at "
        "pipe %x stage %d index %u, %s",
        __func__,
        dev_id,
        entry_hdl,
        mat_tbl_hdl,
        pipe_id,
        stage_id,
        entry_idx,
        pipe_str_err(ret));
    return ret;
  }
  res_data->has_meter = tbl_type == SPEC_TYPE_METER;
  res_data->has_lpf = tbl_type == SPEC_TYPE_LPF;
  res_data->has_red = tbl_type == SPEC_TYPE_WRED;
  if (res_data->has_meter) {
    res_data->mtr.meter = res_spec.meter;
  } else if (res_data->has_lpf) {
    res_data->mtr.lpf = res_spec.lpf;
  } else if (res_data->has_red) {
    res_data->mtr.red = res_spec.red;
  } else {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  return ret;
}

pipe_status_t pipe_mgr_meter_mgr_read_entry(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t meter_idx,
    pipe_res_data_spec_t *res_spec,
    pipe_mgr_meter_spec_type_e spec_type,
    bool from_hw) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  uint8_t stage_id = 0;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Validate the index. */
  if (meter_tbl->num_entries <= meter_idx) {
    LOG_ERROR(
        "Dev %d Tbl 0x%x (%s): requested index %d is out of range, only %d "
        "entries in table",
        meter_tbl->dev_info->dev_id,
        meter_tbl->meter_tbl_hdl,
        meter_tbl->name,
        meter_idx,
        meter_tbl->num_entries);
    return PIPE_INVALID_ARG;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      meter_tbl_get_instance_from_any_pipe(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (meter_tbl->over_allocated) {
    /* If the meter table is replicated in every stage (this happens when
     * a match table which is indirectly referencing the meter table is
     * in more than one stage, then a given meter index corresponds to an
     * entry index in every stage.
     */
    stage_id = meter_tbl_instance->meter_tbl_stage_info[0].stage_id;
  } else {
    unsigned i;
    for (i = 0; i < meter_tbl_instance->num_stages; i++) {
      meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[i];
      if (meter_idx < (meter_tbl_stage_info->ent_idx_offset +
                       meter_tbl_stage_info->num_entries)) {
        stage_id = meter_tbl_stage_info->stage_id;
        meter_idx -= meter_tbl_stage_info->ent_idx_offset;
      }
    }
  }

  return pipe_mgr_meter_mgr_read_entry_in_stage(dev_tgt,
                                                meter_tbl_hdl,
                                                stage_id,
                                                meter_idx,
                                                res_spec,
                                                spec_type,
                                                from_hw);
}

pipe_status_t pipe_mgr_meter_mgr_read_entry_in_stage(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    uint8_t stage_id,
    pipe_meter_stage_idx_t meter_stage_idx,
    pipe_res_data_spec_t *res_spec,
    pipe_mgr_meter_spec_type_e spec_type,
    bool from_hw) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_meter_rate_type_e meter_rate_type;
  pipe_meter_spec_t *meter_spec = NULL;
  vpn_id_t vpn = 0;
  uint32_t blk_idx = 0;
  mem_id_t ram_id = 0;
  uint32_t ram_line_num = 0;
  bf_dev_pipe_t pipe_id = dev_tgt.dev_pipe_id;

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_instance =
      meter_tbl_get_instance_from_any_pipe(meter_tbl, dev_tgt.dev_pipe_id);

  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) && meter_tbl->symmetric)
    pipe_id = meter_tbl->lowest_pipe_id;

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR("%s:%d Meter table stage info for stage id %d not found",
              __func__,
              __LINE__,
              stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;

  blk_idx = meter_stage_idx / TOF_SRAM_UNIT_DEPTH;
  vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
  ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
  ram_line_num = meter_stage_idx % TOF_SRAM_UNIT_DEPTH;

  switch (spec_type) {
    case SPEC_TYPE_METER:
      meter_spec = (pipe_meter_spec_t *)res_spec;
      if (meter_tbl->meter_granularity == PIPE_METER_GRANULARITY_PACKETS) {
        meter_rate_type = METER_RATE_TYPE_PPS;
        meter_spec->cir.type = METER_RATE_TYPE_PPS;
        meter_spec->pir.type = METER_RATE_TYPE_PPS;
      } else if (meter_tbl->meter_granularity == PIPE_METER_GRANULARITY_BYTES) {
        meter_rate_type = METER_RATE_TYPE_KBPS;
        meter_spec->cir.type = METER_RATE_TYPE_KBPS;
        meter_spec->pir.type = METER_RATE_TYPE_KBPS;
      } else {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }

      status = pipe_mgr_meter_ent_read_drv_workflow(
          meter_tbl->dev_info,
          meter_tbl->direction,
          pipe_id,
          stage_id,
          meter_tbl_stage_info->stage_table_handle,
          vpn,
          ram_id,
          ram_line_num,
          meter_rate_type,
          meter_spec,
          from_hw);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in reading meter spec from device id %d, tbl 0x%x,"
            " stage id %d, pipe id %d, meter idx %d err %s",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            meter_tbl_hdl,
            stage_id,
            dev_tgt.dev_pipe_id,
            meter_stage_idx,
            pipe_str_err(status));
        return status;
      }
      break;
    case SPEC_TYPE_LPF:
      PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
      status = pipe_mgr_lpf_ent_read_drv_workflow(
          meter_tbl->dev_info,
          meter_tbl->direction,
          pipe_id,
          stage_id,
          meter_tbl_stage_info->stage_table_handle,
          vpn,
          ram_id,
          ram_line_num,
          (pipe_lpf_spec_t *)res_spec,
          from_hw);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in reading lpf spec from device id %d, tbl 0x%x,"
            " stage id %d, pipe id %d, meter idx %d err %s",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            meter_tbl_hdl,
            stage_id,
            dev_tgt.dev_pipe_id,
            meter_stage_idx,
            pipe_str_err(status));
        return status;
      }
      break;
    case SPEC_TYPE_WRED:
      PIPE_MGR_DBGCHK(pipe_id != BF_DEV_PIPE_ALL);
      status = pipe_mgr_wred_ent_read_drv_workflow(
          meter_tbl->dev_info,
          meter_tbl->direction,
          pipe_id,
          stage_id,
          meter_tbl_stage_info->stage_table_handle,
          vpn,
          ram_id,
          ram_line_num,
          (pipe_wred_spec_t *)res_spec,
          from_hw);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in reading wred spec from device id %d, tbl 0x%x,"
            " stage id %d, pipe id %d, meter idx %d err %s",
            __func__,
            __LINE__,
            dev_tgt.device_id,
            meter_tbl_hdl,
            stage_id,
            dev_tgt.dev_pipe_id,
            meter_stage_idx,
            pipe_str_err(status));
        return status;
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d Reading unexpected resource spec type %d from meter table "
          "0x%x on device %d",
          __func__,
          __LINE__,
          spec_type,
          meter_tbl_hdl,
          dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_get_color_aware(
    pipe_mgr_meter_tbl_t *meter_tbl,
    bf_dev_pipe_t pipe_id,
    pipe_meter_idx_t meter_idx,
    bool *color_aware) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t stage_id = 0;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_meter_stage_idx_t meter_stage_idx = 0;
  uint32_t blk_idx = 0;
  uint32_t ram_line_num = 0;
  mem_id_t ram_id = 0;
  uint8_t *data = 0;

  meter_tbl_instance = pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);

  if (meter_tbl_instance == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_id = pipe_mgr_meter_mgr_get_stage(meter_tbl_instance, meter_idx);

  meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);

  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter tbl stage info not found found for stage id %d"
        " pipe id %d, meter tbl 0x%x",
        __func__,
        __LINE__,
        stage_id,
        pipe_id,
        meter_tbl->meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(meter_idx >= meter_tbl_stage_info->ent_idx_offset);
  meter_stage_idx = meter_idx - meter_tbl_stage_info->ent_idx_offset;

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  blk_idx = meter_stage_idx / TOF_SRAM_UNIT_DEPTH;
  ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
  ram_line_num = meter_stage_idx % TOF_SRAM_UNIT_DEPTH;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = meter_tbl->lowest_pipe_id;
  }

  status = pipe_mgr_phy_mem_map_get_ref(meter_tbl->dev_info->dev_id,
                                        meter_tbl->direction,
                                        pipe_mem_type_unit_ram,
                                        pipe_id,
                                        stage_id,
                                        ram_id,
                                        ram_line_num,
                                        &data,
                                        false);

  if (status != PIPE_SUCCESS) {
    return status;
  }

  *color_aware = (data[TOF_METER_TIMESTAMP_BIT_POS / 8] >>
                  (TOF_METER_TIMESTAMP_BIT_POS % 8)) &
                 0x1;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_mgr_decode_virt_addr(
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t virt_addr,
    bool *pfe,
    bool *pfe_defaulted,
    pipe_meter_idx_t *meter_idx) {
  pipe_mgr_meter_tbl_t *meter_tbl =
      pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter tbl info for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d Meter tbl instance for tbl 0x%x, device id %d, pipe id %d not "
        "found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_meter_tbl_stage_info_t *meter_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);
  if (meter_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Meter stage info for tbl 0x%x, device id %d, pipe id %d, stage "
        "id %d not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (meter_tbl->enable_per_flow_enable) {
    *pfe_defaulted = false;
    if ((virt_addr >> TOF_METER_ADDR_PFE_BIT_POSITION) & 0x1) {
      *pfe = true;
    } else {
      *pfe = false;
    }
  } else {
    *pfe_defaulted = true;
    *pfe = true;
  }
  /* Here, a full meter address along with the LSB ZERO bits need to be passed
   */
  uint32_t ram_line = (virt_addr >> TOF_METER_NUM_LOWER_HUFFMAN_BITS) &
                      ((1 << TOF_SRAM_NUM_RAM_LINE_BITS) - 1);
  vpn_id_t vpn = (virt_addr >> (TOF_METER_NUM_LOWER_HUFFMAN_BITS +
                                TOF_SRAM_NUM_RAM_LINE_BITS)) &
                 ((1 << TOF_METER_NUM_VPN_BITS) - 1);

  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info =
      meter_stage_info->ram_alloc_info;
  uint8_t blk_idx = 0;
  for (blk_idx = 0; blk_idx < ram_alloc_info->num_wide_word_blks; blk_idx++) {
    if (vpn == ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0]) {
      break;
    }
  }
  if (blk_idx == ram_alloc_info->num_wide_word_blks) {
    LOG_ERROR(
        "%s:%d Invalid vpn %d for meter tbl 0x%x, device id %d, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        vpn,
        meter_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_INVALID_ARG;
  }
  *meter_idx = ram_line + (blk_idx * TOF_SRAM_UNIT_DEPTH);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_log_state(bf_dev_id_t dev_id,
                                       pipe_meter_tbl_hdl_t tbl_hdl,
                                       cJSON *meter_tbls) {
  pipe_status_t sts;
  pipe_mgr_meter_tbl_t *meter_info = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;
  pipe_meter_spec_t meter_spec = {0};
  pipe_lpf_spec_t lpf_spec = {0};
  pipe_wred_spec_t wred_spec = {0};
  pipe_meter_rate_t *meter_rate;
  uint32_t pipe_id = 0;
  uint32_t pipe_idx = 0;
  uint32_t stage_idx = 0;
  uint32_t ent_idx = 0;
  uint32_t blk_idx = 0;
  uint32_t ram_line_num = 0;
  vpn_id_t vpn = 0;
  mem_id_t ram_id = 0;
  pipe_meter_rate_type_e meter_rate_type;
  bool is_kbps = false;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(dev_id);
  float zero_time_constant = 0;
  cJSON *meter_tbl, *meter_insts, *meter_inst, *meter_stgs, *meter_stg;
  cJSON *meter_ents, *meter_ent;

  if (!clock_speed) {
    LOG_ERROR(
        "%s:%d Clock speed for device id %d is 0", __func__, __LINE__, dev_id);
    return PIPE_UNEXPECTED;
  }
  zero_time_constant = 1000000000.0 / clock_speed;

  meter_info = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
  if (meter_info == NULL) {
    LOG_ERROR("%s:%d Meter tbl for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  cJSON_AddItemToArray(meter_tbls, meter_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(meter_tbl, "name", meter_info->name);
  cJSON_AddNumberToObject(meter_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(meter_tbl, "symmetric", meter_info->symmetric);
  cJSON_AddItemToObject(
      meter_tbl, "meter_insts", meter_insts = cJSON_CreateArray());

  for (pipe_idx = 0; pipe_idx < meter_info->num_tbl_instances; pipe_idx++) {
    meter_instance = &(meter_info->meter_tbl_instances[pipe_idx]);
    if (meter_info->symmetric) {
      pipe_id = meter_info->lowest_pipe_id;
    } else {
      pipe_id = meter_instance->pipe_id;
    }
    cJSON_AddItemToArray(meter_insts, meter_inst = cJSON_CreateObject());
    cJSON_AddNumberToObject(meter_inst, "pipe_id", meter_instance->pipe_id);
    cJSON_AddItemToObject(
        meter_inst, "stage_tbls", meter_stgs = cJSON_CreateArray());
    for (stage_idx = 0; stage_idx < meter_instance->num_stages; stage_idx++) {
      cJSON_AddItemToArray(meter_stgs, meter_stg = cJSON_CreateObject());
      stage_info = &(meter_instance->meter_tbl_stage_info[stage_idx]);
      cJSON_AddNumberToObject(meter_stg, "stage_id", stage_info->stage_id);
      cJSON_AddItemToObject(
          meter_stg, "meter_ents", meter_ents = cJSON_CreateArray());
      for (ent_idx = 0; ent_idx < stage_info->num_entries; ent_idx++) {
        ram_alloc_info = stage_info->ram_alloc_info;
        blk_idx = ent_idx / TOF_SRAM_UNIT_DEPTH;
        vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
        ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
        ram_line_num = ent_idx % TOF_SRAM_UNIT_DEPTH;

        switch (meter_info->type) {
          case METER_DATA_TYPE_METER:
            PIPE_MGR_MEMSET(&meter_spec, 0, sizeof(pipe_meter_spec_t));
            if (meter_info->meter_granularity ==
                PIPE_METER_GRANULARITY_PACKETS) {
              meter_rate_type = METER_RATE_TYPE_PPS;
            } else {
              meter_rate_type = METER_RATE_TYPE_KBPS;
            }
            sts = pipe_mgr_meter_ent_read_drv_workflow(
                dev_info,
                meter_info->direction,
                pipe_id,
                stage_info->stage_id,
                stage_info->stage_table_handle,
                vpn,
                ram_id,
                ram_line_num,
                meter_rate_type,
                &meter_spec,
                false);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Meter entry read for meter table 0x%x"
                  " stage %u index %u failed",
                  __func__,
                  __LINE__,
                  tbl_hdl,
                  stage_info->stage_id,
                  ent_idx);
              return sts;
            }
            if (meter_spec.cir.value.kbps == 0 && meter_spec.cburst == 0 &&
                meter_spec.pir.value.kbps == 0 && meter_spec.pburst == 0) {
              break;
            }
            cJSON_AddItemToArray(meter_ents, meter_ent = cJSON_CreateObject());
            cJSON_AddStringToObject(meter_ent, "type", "meter");
            cJSON_AddNumberToObject(meter_ent, "ent_idx", ent_idx);
            cJSON_AddBoolToObject(
                meter_ent,
                "color_aware",
                (meter_spec.meter_type == METER_TYPE_COLOR_AWARE));
            meter_rate = &(meter_spec.cir);
            is_kbps = (meter_rate->type == METER_RATE_TYPE_KBPS);
            cJSON_AddStringToObject(
                meter_ent, "cir_type", (is_kbps ? "kbps" : "pps"));
            cJSON_AddNumberToObject(
                meter_ent,
                "cir_rate",
                (is_kbps ? meter_rate->value.kbps : meter_rate->value.pps));
            cJSON_AddNumberToObject(meter_ent, "cburst", meter_spec.cburst);
            meter_rate = &(meter_spec.pir);
            is_kbps = (meter_rate->type == METER_RATE_TYPE_KBPS);
            cJSON_AddStringToObject(
                meter_ent, "pir_type", (is_kbps ? "kbps" : "pps"));
            cJSON_AddNumberToObject(
                meter_ent,
                "pir_rate",
                (is_kbps ? meter_rate->value.kbps : meter_rate->value.pps));
            cJSON_AddNumberToObject(meter_ent, "pburst", meter_spec.pburst);
            break;
          case METER_DATA_TYPE_LPF:
            PIPE_MGR_MEMSET(&lpf_spec, 0, sizeof(pipe_lpf_spec_t));
            sts = pipe_mgr_lpf_ent_read_drv_workflow(
                dev_info,
                meter_info->direction,
                pipe_id,
                stage_info->stage_id,
                stage_info->stage_table_handle,
                vpn,
                ram_id,
                ram_line_num,
                &lpf_spec,
                false);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Lpf entry read for lpf table 0x%x"
                  " stage %u index %u failed",
                  __func__,
                  __LINE__,
                  tbl_hdl,
                  stage_info->stage_id,
                  ent_idx);
              return sts;
            }
            if (lpf_spec.lpf_type == LPF_TYPE_SAMPLE &&
                lpf_spec.gain_decay_separate_time_constant == false &&
                lpf_spec.time_constant == zero_time_constant &&
                lpf_spec.output_scale_down_factor == 0) {
              break;
            }
            cJSON_AddItemToArray(meter_ents, meter_ent = cJSON_CreateObject());
            cJSON_AddStringToObject(meter_ent, "type", "lpf");
            cJSON_AddNumberToObject(meter_ent, "ent_idx", ent_idx);
            cJSON_AddStringToObject(
                meter_ent,
                "lpf_type",
                (lpf_spec.lpf_type == LPF_TYPE_RATE) ? "rate" : "sample");
            if (lpf_spec.gain_decay_separate_time_constant) {
              cJSON_AddNumberToObject(
                  meter_ent, "gain_time_const", lpf_spec.gain_time_constant);
              cJSON_AddNumberToObject(
                  meter_ent, "decay_time_const", lpf_spec.decay_time_constant);
            } else {
              cJSON_AddNumberToObject(
                  meter_ent, "time_const", lpf_spec.time_constant);
            }
            cJSON_AddNumberToObject(meter_ent,
                                    "output_scale_down_factor",
                                    lpf_spec.output_scale_down_factor);
            break;
          case METER_DATA_TYPE_WRED:
            PIPE_MGR_MEMSET(&wred_spec, 0, sizeof(pipe_wred_spec_t));
            sts = pipe_mgr_wred_ent_read_drv_workflow(
                dev_info,
                meter_info->direction,
                pipe_id,
                stage_info->stage_id,
                stage_info->stage_table_handle,
                vpn,
                ram_id,
                ram_line_num,
                &wred_spec,
                false);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Wred entry read for wred table 0x%x"
                  " stage %u index %u failed",
                  __func__,
                  __LINE__,
                  tbl_hdl,
                  stage_info->stage_id,
                  ent_idx);
              return sts;
            }
            if (wred_spec.time_constant == zero_time_constant &&
                wred_spec.red_min_threshold == 0 &&
                wred_spec.red_max_threshold == 0 &&
                wred_spec.max_probability == 0) {
              break;
            }
            cJSON_AddItemToArray(meter_ents, meter_ent = cJSON_CreateObject());
            cJSON_AddStringToObject(meter_ent, "type", "wred");
            cJSON_AddNumberToObject(meter_ent, "ent_idx", ent_idx);
            cJSON_AddNumberToObject(
                meter_ent, "time_const", wred_spec.time_constant);
            cJSON_AddNumberToObject(
                meter_ent, "red_min_threshold", wred_spec.red_min_threshold);
            cJSON_AddNumberToObject(
                meter_ent, "red_max_threshold", wred_spec.red_max_threshold);
            cJSON_AddNumberToObject(
                meter_ent, "max_probability", wred_spec.max_probability);
            break;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_restore_state(bf_dev_id_t dev_id,
                                           cJSON *meter_tbl) {
  pipe_status_t sts;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  pipe_meter_tbl_hdl_t tbl_hdl;
  bool symmetric;
  pipe_mgr_meter_tbl_t *meter_info = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *stage_info = NULL;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_meter_spec_t meter_spec;
  pipe_meter_rate_t *meter_rate;
  pipe_lpf_spec_t lpf_spec;
  pipe_wred_spec_t wred_spec;
  char *type;
  uint32_t pipe_id = 0;
  uint32_t pipe_idx = 0;
  uint32_t stage_id = 0;
  uint32_t stage_idx = 0;
  uint32_t ent_idx = 0;
  uint32_t blk_idx = 0;
  vpn_id_t vpn = 0;
  uint32_t ram_line_num = 0;
  mem_id_t ram_id = 0;
  cJSON *meter_insts, *meter_inst, *meter_stgs, *meter_stg;
  cJSON *meter_ents, *meter_ent;
  cJSON *time_const;
  scope_pipes_t scopes = 0xf;

  tbl_hdl = cJSON_GetObjectItem(meter_tbl, "handle")->valueint;
  meter_info = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
  if (meter_info == NULL) {
    LOG_ERROR("%s:%d Meter tbl for hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  symmetric = (cJSON_GetObjectItem(meter_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != meter_info->symmetric) {
    sts = pipe_mgr_meter_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, 1, &scopes);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, meter tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      return sts;
    }
  }

  meter_insts = cJSON_GetObjectItem(meter_tbl, "meter_insts");
  pipe_idx = 0;
  for (meter_inst = meter_insts->child; meter_inst;
       meter_inst = meter_inst->next) {
    meter_instance = &(meter_info->meter_tbl_instances[pipe_idx]);
    pipe_id = cJSON_GetObjectItem(meter_inst, "pipe_id")->valueint;
    if (meter_info->symmetric) {
      PIPE_MGR_DBGCHK(pipe_id == BF_DEV_PIPE_ALL);
    } else {
      PIPE_MGR_DBGCHK(pipe_id == meter_instance->pipe_id);
      pipe_idx++;
    }

    meter_stgs = cJSON_GetObjectItem(meter_inst, "stage_tbls");
    for (stage_idx = 0, meter_stg = meter_stgs->child;
         stage_idx < meter_instance->num_stages && meter_stg;
         stage_idx++, meter_stg = meter_stg->next) {
      stage_info = &(meter_instance->meter_tbl_stage_info[stage_idx]);
      stage_id = cJSON_GetObjectItem(meter_stg, "stage_id")->valueint;
      PIPE_MGR_DBGCHK(stage_id == stage_info->stage_id);

      meter_ents = cJSON_GetObjectItem(meter_stg, "meter_ents");
      for (meter_ent = meter_ents->child; meter_ent;
           meter_ent = meter_ent->next) {
        ent_idx = cJSON_GetObjectItem(meter_ent, "ent_idx")->valuedouble;
        type = cJSON_GetObjectItem(meter_ent, "type")->valuestring;

        ram_alloc_info = stage_info->ram_alloc_info;
        blk_idx = ent_idx / TOF_SRAM_UNIT_DEPTH;
        vpn = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
        ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
        ram_line_num = ent_idx % TOF_SRAM_UNIT_DEPTH;

        if (!strcmp(type, "meter")) {
          meter_spec.meter_type =
              (cJSON_GetObjectItem(meter_ent, "color_aware")->type ==
               cJSON_True)
                  ? METER_TYPE_COLOR_AWARE
                  : METER_TYPE_COLOR_UNAWARE;

          meter_rate = &(meter_spec.cir);
          if (!strcmp(cJSON_GetObjectItem(meter_ent, "cir_type")->valuestring,
                      "kbps")) {
            meter_rate->type = METER_RATE_TYPE_KBPS;
            meter_rate->value.kbps =
                cJSON_GetObjectItem(meter_ent, "cir_rate")->valuedouble;
          } else {
            meter_rate->type = METER_RATE_TYPE_PPS;
            meter_rate->value.pps =
                cJSON_GetObjectItem(meter_ent, "cir_rate")->valuedouble;
          }
          meter_spec.cburst =
              cJSON_GetObjectItem(meter_ent, "cburst")->valuedouble;

          meter_rate = &(meter_spec.pir);
          if (!strcmp(cJSON_GetObjectItem(meter_ent, "pir_type")->valuestring,
                      "kbps")) {
            meter_rate->type = METER_RATE_TYPE_KBPS;
            meter_rate->value.kbps =
                cJSON_GetObjectItem(meter_ent, "pir_rate")->valuedouble;
          } else {
            meter_rate->type = METER_RATE_TYPE_PPS;
            meter_rate->value.pps =
                cJSON_GetObjectItem(meter_ent, "pir_rate")->valuedouble;
          }
          meter_spec.pburst =
              cJSON_GetObjectItem(meter_ent, "pburst")->valuedouble;

          sts =
              pipe_mgr_meter_spec_update_drv_workflow(sess_hdl,
                                                      dev_id,
                                                      meter_info,
                                                      meter_instance->pipe_bmp,
                                                      stage_info,
                                                      vpn,
                                                      ram_id,
                                                      ram_line_num,
                                                      ent_idx,
                                                      &meter_spec);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in updating the meter spec for table 0x%x,"
                " index %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                tbl_hdl,
                ent_idx,
                pipe_id,
                stage_id,
                pipe_str_err(sts));
            return sts;
          }
        } else if (!strcmp(type, "lpf")) {
          PIPE_MGR_MEMSET(&lpf_spec, 0, sizeof(pipe_lpf_spec_t));
          lpf_spec.lpf_type =
              !strcmp(cJSON_GetObjectItem(meter_ent, "lpf_type")->valuestring,
                      "rate")
                  ? LPF_TYPE_RATE
                  : LPF_TYPE_SAMPLE;
          time_const = cJSON_GetObjectItem(meter_ent, "time_const");
          if (time_const) {
            lpf_spec.gain_decay_separate_time_constant = false;
            lpf_spec.time_constant = time_const->valuedouble;
          } else {
            lpf_spec.gain_decay_separate_time_constant = true;
            lpf_spec.gain_time_constant =
                cJSON_GetObjectItem(meter_ent, "gain_time_const")->valuedouble;
            lpf_spec.decay_time_constant =
                cJSON_GetObjectItem(meter_ent, "decay_time_const")->valuedouble;
          }
          lpf_spec.output_scale_down_factor =
              cJSON_GetObjectItem(meter_ent, "output_scale_down_factor")
                  ->valuedouble;
          sts = pipe_mgr_lpf_spec_update_drv_workflow(
              sess_hdl,
              dev_id,
              meter_info,
              meter_instance->pipe_bmp,
              stage_info->stage_table_handle,
              stage_info->stage_id,
              vpn,
              ram_id,
              ram_line_num,
              &lpf_spec);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in updating the lpf spec for table 0x%x,"
                " index %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                tbl_hdl,
                ent_idx,
                pipe_id,
                stage_id,
                pipe_str_err(sts));
            return sts;
          }
        } else if (!strcmp(type, "wred")) {
          wred_spec.time_constant =
              cJSON_GetObjectItem(meter_ent, "time_const")->valuedouble;
          wred_spec.red_min_threshold =
              cJSON_GetObjectItem(meter_ent, "red_min_threshold")->valuedouble;
          wred_spec.red_max_threshold =
              cJSON_GetObjectItem(meter_ent, "red_max_threshold")->valuedouble;
          wred_spec.max_probability =
              cJSON_GetObjectItem(meter_ent, "max_probability")->valuedouble;
          sts = pipe_mgr_wred_spec_update_drv_workflow(
              sess_hdl,
              dev_id,
              meter_info,
              meter_instance->pipe_bmp,
              stage_info->stage_table_handle,
              stage_info->stage_id,
              vpn,
              ram_id,
              ram_line_num,
              &wred_spec);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in updating the wred spec for table 0x%x,"
                " index %d, pipe id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                tbl_hdl,
                ent_idx,
                pipe_id,
                stage_id,
                pipe_str_err(sts));
            return sts;
          }
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

bool pipe_mgr_meter_tbl_is_indirect(bf_dev_id_t device_id,
                                    pipe_meter_tbl_hdl_t meter_tbl_hdl) {
  pipe_mgr_meter_tbl_t *meter_tbl =
      pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    return false;
  }
  if (meter_tbl->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    return true;
  }
  return false;
}

pipe_status_t pipe_mgr_meter_tbl_sbe_correct(bf_dev_id_t dev_id,
                                             bf_dev_pipe_t log_pipe_id,
                                             dev_stage_t stage_id,
                                             pipe_meter_tbl_hdl_t tbl_hdl,
                                             int line) {
  /* If the device is locked we cannot correct anything since the software state
   * may not agree with the hardware state. */
  if (pipe_mgr_is_device_locked(dev_id)) return PIPE_SUCCESS;

  pipe_mgr_meter_tbl_t *meter_tbl = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_id,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  rmt_dev_info_t *dev_info = meter_tbl->dev_info;

  /* Pipe id is always a single pipe so when getting the instance special case
   * symmetric tables. */
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance =
      meter_tbl->symmetric
          ? meter_tbl->meter_tbl_instances
          : pipe_mgr_meter_tbl_get_instance(meter_tbl, log_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        log_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info =
      pipe_mgr_meter_tbl_get_stage_info(meter_tbl_instance, stage_id);
  if (meter_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Dev %d meter table 0x%x pipe %d stage id %d info not found",
        __func__,
        __LINE__,
        dev_id,
        tbl_hdl,
        log_pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_dev_pipe_t phy_pipe_id;
  pipe_status_t sts = pipe_mgr_map_pipe_id_log_to_phy(
      meter_tbl->dev_info, log_pipe_id, &phy_pipe_id);
  if (sts) return sts;
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);

  /* Issue a virtual read for every VPN on the ram line.  Virtual accesses will
   * always read-modify-write the unit RAM and by accesses every VPN all the
   * map RAMs will update as well. */
  for (unsigned int i = 0;
       i < meter_tbl_stage_info->ram_alloc_info->num_wide_word_blks;
       ++i) {
    vpn_id_t vpn =
        meter_tbl_stage_info->ram_alloc_info->tbl_word_blk[i].vpn_id[0];
    rmt_virt_addr_t low_vir_addr =
        pipe_mgr_meter_compute_ent_virt_addr(vpn, line);

    pipe_full_virt_addr_t vaddr;
    vaddr.addr = 0;
    construct_full_virt_addr(dev_info,
                             &vaddr,
                             meter_tbl_stage_info->stage_table_handle,
                             pipe_virt_mem_type_meter,
                             low_vir_addr,
                             phy_pipe_id,
                             stage_id);
    LOG_DBG(
        "Dev %d pipe %d stage %d lt %d meter tbl 0x%x SBE correct vpn %d line "
        "%d virt 0x%" PRIx64,
        dev_id,
        log_pipe_id,
        stage_id,
        meter_tbl_stage_info->stage_table_handle,
        tbl_hdl,
        vpn,
        line,
        vaddr.addr);
    uint64_t dont, care;
    lld_subdev_ind_read(dev_id, subdev, vaddr.addr, &dont, &care);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tbl_bytecount_adjust_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int bytecount) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;
  struct pipe_config_cache_meter_ctl_t *cc = NULL;
  pipe_status_t status;
  uint32_t i;
  uint32_t j;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  // Check bytecount within [13:0] (-2^13 ~ (2^13-1)) = (-8192 ~ 8191)
  if ((bytecount < -8192) || (bytecount > (8192 - 1))) {
    LOG_ERROR(
        "%s:%d Invalid bytecount 0x%x for tbl 0x%x, device id %d, pipe id %d",
        __func__,
        __LINE__,
        bytecount,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
  }

  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* get db */
  bf_map_sts_t sts =
      bf_map_get(&dev_info->profile_info[meter_tbl->profile_id]->config_cache,
                 pipe_cck_meter_ctrl,
                 (void **)&cc);
  if (BF_MAP_OK != sts || !cc) {
    LOG_ERROR(
        "%s:%d : Meter ctl config cache for tbl 0x%x, device id %d, profile id "
        "%d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        meter_tbl->profile_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < meter_tbl_instance->num_stages; i++) {
    meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[i];
    if (meter_tbl_stage_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    for (j = 0; j < meter_tbl_stage_info->num_alu_ids; j++) {
      status = pipe_mgr_meter_set_bytecount_adjust_drv_workflow(
          sess_hdl,
          dev_tgt.device_id,
          meter_tbl_instance->pipe_bmp,
          &(cc->val[meter_tbl_stage_info->stage_id]
                   [meter_tbl_stage_info->alu_ids[j]]),
          meter_tbl_stage_info->stage_id,
          meter_tbl_stage_info->alu_ids[j],
          bytecount);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in configuring bytecount adjust for table %#x,"
            "pipe id %d, stage id %d, alu idx %d, bytecount 0x%x, err %s",
            __func__,
            __LINE__,
            meter_tbl_hdl,
            dev_tgt.dev_pipe_id,
            meter_tbl_stage_info->stage_id,
            meter_tbl_stage_info->alu_ids[j],
            bytecount,
            pipe_str_err(status));
        return status;
      }
    }
  }
  /* cache the adjust bytecount value to the table instance */
  meter_tbl_instance->adj_byt_cnt = bytecount;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tbl_bytecount_adjust_get(
    dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, int *bytecount) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (bytecount == NULL) {
    return PIPE_INVALID_ARG;
  }
  meter_tbl = pipe_mgr_meter_tbl_get(dev_tgt.device_id, meter_tbl_hdl);
  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table not found for device id %d, tbl hdl 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              meter_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Get the meter table instance based on the pipe id */
  meter_tbl_instance =
      pipe_mgr_meter_tbl_get_instance(meter_tbl, dev_tgt.dev_pipe_id);
  if (meter_tbl_instance == NULL) {
    LOG_ERROR(
        "%s:%d : Meter table instance for tbl 0x%x, device id %d, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  *bytecount = meter_tbl_instance->adj_byt_cnt;
  return PIPE_SUCCESS;
}
