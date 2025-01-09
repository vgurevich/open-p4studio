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
 r @file pipe_mgr_mau_tbl_dbg_counters.c
 * @date
 *
 * Implementation of MAU table debug counters
 */

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_mau_tbl_dbg_counters.h"
#include <tofino_regs/tofino.h>

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/* tbl dbg counter DB */
typedef struct pipe_mgr_tbl_dbg_cntr_db {
  /* Per pipe, stage, logical table. */
  bf_tbl_dbg_counter_type_t ***type;
} pipe_mgr_tbl_dbg_cntr_db_t;

/* DB for caching per log tbl counter type */
pipe_mgr_tbl_dbg_cntr_db_t *tbl_cntr_db[PIPE_MGR_NUM_DEVICES] = {NULL};

#define PIPE_MGR_TBL_DBG_CNTR_DB(dev) (tbl_cntr_db[dev])

#define PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe, stage, log_tbl_id) \
  (tbl_cntr_db[dev]->type[pipe][stage][log_tbl_id])

#define PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev) \
  (dev < PIPE_MGR_NUM_DEVICES) ? true : false

static inline bool tbl_dbg_cntr_pipe_valid(rmt_dev_info_t *dev_info,
                                           bf_dev_pipe_t pipe) {
  return pipe == BF_DEV_PIPE_ALL || dev_info->num_active_pipes > pipe;
}

static inline bool tbl_dbg_cntr_stage_valid(rmt_dev_info_t *dev_info,
                                            dev_stage_t stage) {
  return stage < dev_info->num_active_mau;
}

#define PIPE_MGR_LOG_CNTR_TYPE_BITS 3
#define PIPE_MGR_LOG_CNTR_TYPE_MASK 0x7

static pipe_status_t pipe_mgr_tbl_dbg_counter_clear(rmt_dev_info_t *dev_info,
                                                    bf_dev_pipe_t pipe,
                                                    dev_stage_t stage,
                                                    uint8_t log_tbl_id);

/* Wrapper API for reg write */
static inline pipe_status_t pipe_mgr_tbl_db_counter_write_register(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev,
    uint32_t reg_addr,
    uint32_t reg_data) {
  return pipe_mgr_write_register(dev, subdev, reg_addr, reg_data);
}

/* Wrapper API for reg read */
static inline int pipe_mgr_tbl_dbg_counter_read_register(bf_dev_id_t dev,
                                                         uint32_t reg_addr,
                                                         uint32_t *reg_data) {
  *reg_data = 0;
  return lld_subdev_read_register(dev, 0, reg_addr, reg_data);
}

/** Init the tbl cntr db
 * dev - Device target
 */
pipe_status_t pipe_mgr_tbl_dbg_counter_init(bf_dev_id_t dev) {
  bf_dev_pipe_t pipe = 0;
  dev_stage_t stage = 0;
  uint8_t log_tbl_id = 0;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (PIPE_MGR_TBL_DBG_CNTR_DB(dev)) {
    return PIPE_SUCCESS;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  dev_stage_t num_stages = dev_info->num_active_mau;
  unsigned int p, s;
  pipe_mgr_tbl_dbg_cntr_db_t *db = PIPE_MGR_CALLOC(1, sizeof(*db));
  if (!db) goto cleanup;
  db->type = PIPE_MGR_CALLOC(dev_info->num_active_pipes, sizeof(*db->type));
  if (!db->type) goto cleanup;
  for (p = 0; p < dev_info->num_active_pipes; ++p) {
    db->type[p] = PIPE_MGR_CALLOC(num_stages, sizeof(*db->type[p]));
    if (!db->type[p]) goto cleanup;
    for (s = 0; s < num_stages; ++s) {
      db->type[p][s] =
          PIPE_MGR_CALLOC(dev_info->dev_cfg.stage_cfg.num_logical_tables,
                          sizeof(*db->type[p][s]));
      if (!db->type[p][s]) goto cleanup;
    }
  }

  PIPE_MGR_TBL_DBG_CNTR_DB(dev) = db;

  /* The type is initialized to log tbl hit in asic, start with that state in DB
   * too */
  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    for (stage = 0; stage < num_stages; stage++) {
      for (log_tbl_id = 0;
           log_tbl_id < dev_info->dev_cfg.stage_cfg.num_logical_tables;
           log_tbl_id++) {
        PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe, stage, log_tbl_id) =
            BF_TBL_DBG_CNTR_LOG_TBL_HIT;
      }
    }
  }

  return PIPE_SUCCESS;
cleanup:
  if (db) {
    if (db->type) {
      for (p = 0; p < dev_info->num_active_pipes; ++p) {
        if (db->type[p]) {
          for (s = 0; s < num_stages; ++s) {
            if (db->type[p][s]) {
              PIPE_MGR_FREE(db->type[p][s]);
            }
          }
          PIPE_MGR_FREE(db->type[p]);
        }
      }
      PIPE_MGR_FREE(db->type);
    }
    PIPE_MGR_FREE(db);
  }
  return PIPE_NO_SYS_RESOURCES;
}

/** Cleanup the tbl cntr db
 * dev - Device target
 */
pipe_status_t pipe_mgr_tbl_dbg_counter_cleanup(bf_dev_id_t dev) {
  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!PIPE_MGR_TBL_DBG_CNTR_DB(dev)) {
    return PIPE_SUCCESS;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  dev_stage_t num_stages = dev_info->num_active_mau;
  pipe_mgr_tbl_dbg_cntr_db_t *db = PIPE_MGR_TBL_DBG_CNTR_DB(dev);
  PIPE_MGR_TBL_DBG_CNTR_DB(dev) = NULL;
  if (db) {
    if (db->type) {
      unsigned int p, s;
      for (p = 0; p < dev_info->num_active_pipes; ++p) {
        if (db->type[p]) {
          for (s = 0; s < num_stages; ++s) {
            if (db->type[p][s]) {
              PIPE_MGR_FREE(db->type[p][s]);
            }
          }
          PIPE_MGR_FREE(db->type[p]);
        }
      }
      PIPE_MGR_FREE(db->type);
    }
    PIPE_MGR_FREE(db);
  }

  return PIPE_SUCCESS;
}

/* Get list of tbl logical ids from table name or stage */
static pipe_status_t pipe_mgr_tbl_dbg_counter_log_id_list(bf_dev_id_t dev,
                                                          bf_dev_pipe_t pipe,
                                                          char *tbl_name,
                                                          dev_stage_t stage,
                                                          bool stage_valid,
                                                          uint32_t *log_id_arr,
                                                          int *num_entries) {
  pipe_status_t status = PIPE_SUCCESS;
  profile_id_t prof_id = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (stage_valid && (!tbl_dbg_cntr_stage_valid(dev_info, stage))) {
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) !=
      PIPE_SUCCESS) {
    return PIPE_INVALID_ARG;
  }

  status = pipe_mgr_ctxjson_tof_tbl_name_to_log_id(dev,
                                                   prof_id,
                                                   tbl_name,
                                                   stage,
                                                   stage_valid,
                                                   log_id_arr,
                                                   num_entries,
                                                   PIPE_MGR_TBL_MAX_LOG_IDS);

  return status;
}

/* Convert dbg counter type to string */
char *pipe_mgr_tbl_dbg_counter_type_to_string(bf_tbl_dbg_counter_type_t type,
                                              char *buf) {
  switch (type) {
    case BF_TBL_DBG_CNTR_DISABLED:
      strcpy(buf, "disabled");
      break;
    case BF_TBL_DBG_CNTR_LOG_TBL_MISS:
      strcpy(buf, "Log tbl miss");
      break;
    case BF_TBL_DBG_CNTR_LOG_TBL_HIT:
      strcpy(buf, "Log tbl hit");
      break;
    case BF_TBL_DBG_CNTR_GW_TBL_MISS:
      strcpy(buf, "GW tbl miss");
      break;
    case BF_TBL_DBG_CNTR_GW_TBL_HIT:
      strcpy(buf, "GW tbl hit");
      break;
    case BF_TBL_DBG_CNTR_GW_TBL_INHIBIT:
      strcpy(buf, "GW tbl inhibit");
      break;
    default:
      strcpy(buf, "unknown");
      break;
  }

  return buf;
}

/* Recalculate the type value to be written into the control reg
   There are two control regs of 32 bit each.
   Each logical id has 4 bits in control reg
*/
pipe_status_t pipe_mgr_tbl_dbg_counter_type_recalculate(bf_dev_id_t dev,
                                                        bf_dev_pipe_t pipe,
                                                        dev_stage_t stage,
                                                        uint32_t *value0,
                                                        uint32_t *value1) {
  int log_tbl_id = 0, reg_index = 0;
  uint32_t type = 0;
  uint32_t type_val[2];

  PIPE_MGR_MEMSET(type_val, 0, sizeof(type_val));

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (log_tbl_id = 0; log_tbl_id < num_lts; log_tbl_id++) {
    reg_index = log_tbl_id / 8;
    type = PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe, stage, log_tbl_id) &
           PIPE_MGR_LOG_CNTR_TYPE_MASK;

    type_val[reg_index] |=
        (type << ((log_tbl_id % 8) * PIPE_MGR_LOG_CNTR_TYPE_BITS));
  }

  *value0 = type_val[0];
  *value1 = type_val[1];

  return PIPE_SUCCESS;
}

static pipe_status_t tbl_dbg_counter_type_set_tof(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t log_tbl_id,
    bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, type_val[2];
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;
  PIPE_MGR_MEMSET(type_val, 0, sizeof(type_val));

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    /* log tbl 0-7 in index 0, 8-15 in index 1 */
    reg_index = log_tbl_id / 8;
    address = offsetof(Tofino,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.merge.mau_table_counter_ctl[reg_index]);

    /* Set new value in DB */
    PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe_idx, stage, log_tbl_id) = type;

    /* Recalculate the value to write to asic */
    pipe_mgr_tbl_dbg_counter_type_recalculate(
        dev, pipe_idx, stage, &type_val[0], &type_val[1]);

    LOG_TRACE(
        "%s: dev %d pipe %d log_tbl %d setting counter type %s with value 0x%x",
        __func__,
        dev,
        pipe_idx,
        log_tbl_id,
        bf_tbl_dbg_counter_type_to_str(type),
        type_val[reg_index]);
    /* Write the value in asic */
    pipe_mgr_tbl_db_counter_write_register(
        dev, 0, address, type_val[reg_index]);
  }

  return status;
}

static pipe_status_t tbl_dbg_counter_type_set_tof2(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t log_tbl_id,
    bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, type_val[2];
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;
  PIPE_MGR_MEMSET(type_val, 0, sizeof(type_val));

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    /* log tbl 0-7 in index 0, 8-15 in index 1 */
    reg_index = log_tbl_id / 8;
    address = offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .mau[stage]
                           .rams.match.merge.mau_table_counter_ctl[reg_index]);

    /* Set new value in DB */
    PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe_idx, stage, log_tbl_id) = type;

    /* Recalculate the value to write to asic */
    pipe_mgr_tbl_dbg_counter_type_recalculate(
        dev, pipe_idx, stage, &type_val[0], &type_val[1]);

    LOG_TRACE(
        "%s: dev %d pipe %d log_tbl %d setting counter type %s with value 0x%x",
        __func__,
        dev,
        pipe_idx,
        log_tbl_id,
        bf_tbl_dbg_counter_type_to_str(type),
        type_val[reg_index]);
    /* Write the value in asic */
    pipe_mgr_tbl_db_counter_write_register(
        dev, 0, address, type_val[reg_index]);
  }

  return status;
}

static pipe_status_t tbl_dbg_counter_type_set_tof3(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t log_tbl_id,
    bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, type_val[2];
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;
  PIPE_MGR_MEMSET(type_val, 0, sizeof(type_val));

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    /* log tbl 0-7 in index 0, 8-15 in index 1 */
    reg_index = log_tbl_id / 8;
    address = offsetof(tof3_reg,
                       pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                           .mau[stage]
                           .rams.match.merge.mau_table_counter_ctl[reg_index]);

    /* Set new value in DB */
    PIPE_MGR_TBL_DBG_CNTR_TYPE(dev, pipe_idx, stage, log_tbl_id) = type;

    /* Recalculate the value to write to asic */
    pipe_mgr_tbl_dbg_counter_type_recalculate(
        dev, pipe_idx, stage, &type_val[0], &type_val[1]);

    LOG_TRACE(
        "%s: dev %d pipe %d log_tbl %d setting counter type %s with value 0x%x",
        __func__,
        dev,
        pipe_idx,
        log_tbl_id,
        bf_tbl_dbg_counter_type_to_str(type),
        type_val[reg_index]);
    /* Write the value in asic */
    pipe_mgr_tbl_db_counter_write_register(
        dev, phy_pipe / BF_SUBDEV_PIPE_COUNT, address, type_val[reg_index]);
  }

  return status;
}

/* Dbg counter type set helper */
static pipe_status_t pipe_mgr_tbl_dbg_counter_type_set(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t log_tbl_id,
    bf_tbl_dbg_counter_type_t type) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tbl_dbg_counter_type_set_tof(
          dev_info, pipe, stage, log_tbl_id, type);
    case BF_DEV_FAMILY_TOFINO2:
      return tbl_dbg_counter_type_set_tof2(
          dev_info, pipe, stage, log_tbl_id, type);
    case BF_DEV_FAMILY_TOFINO3:
      return tbl_dbg_counter_type_set_tof3(
          dev_info, pipe, stage, log_tbl_id, type);
    default:
      PIPE_MGR_DBGCHK(0);
  }
  return PIPE_UNEXPECTED;
}

/* Dbg counter type set */
pipe_status_t bf_tbl_dbg_counter_type_set(dev_target_t dev_tgt,
                                          char *tbl_name,
                                          bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  dev_stage_t stage = 0;
  int num_entries = 0, idx = 0;
  uint8_t log_tbl_id = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (type >= BF_TBL_DBG_CNTR_MAX) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));

  /* Get log-tbl_id list from table name */
  pipe_mgr_tbl_dbg_counter_log_id_list(
      dev, pipe, tbl_name, 0, false, &log_id_arr[0], &num_entries);
  if (num_entries == 0) {
    LOG_ERROR("Invalid table name %s ", tbl_name);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (idx = 0; idx < num_entries; idx++) {
    stage = log_id_arr[idx] / num_lts;
    log_tbl_id = log_id_arr[idx] % num_lts;
    LOG_TRACE(
        "%s: Dev %d Pipe %x Tbl %s Stage %d Log id %d setting counter to %s",
        __func__,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id,
        tbl_name,
        stage,
        log_tbl_id,
        bf_tbl_dbg_counter_type_to_str(type));
    status = pipe_mgr_tbl_dbg_counter_type_set(
        dev_info, pipe, stage, log_tbl_id, type);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set tbl dbg counter for tbl-id %d ",
                log_id_arr[idx]);
    } else {
      /* Clear the counter as type has changed */
      pipe_mgr_tbl_dbg_counter_clear(dev_info, pipe, stage, log_tbl_id);
    }
  }
  pipe_mgr_api_exit(shdl);
  return status;
}

pipe_status_t bf_log_tbl_dbg_counter_type_set(dev_target_t dev_tgt,
                                              uint32_t stage,
                                              uint32_t log_tbl_id,
                                              bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (type >= BF_TBL_DBG_CNTR_MAX) {
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  status = pipe_mgr_tbl_dbg_counter_type_set(
      dev_info, pipe, stage, log_tbl_id, type);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to set tbl dbg counter for pipe %d stage %d log_tbl %d",
              pipe,
              stage,
              log_tbl_id);
  } else {
    /* Clear the counter as type has changed */
    pipe_mgr_tbl_dbg_counter_clear(dev_info, pipe, stage, log_tbl_id);
  }
  pipe_mgr_api_exit(shdl);
  return status;
}

static pipe_status_t tbl_dbg_counter_get_tof(rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t stage,
                                             uint8_t log_tbl_id,
                                             bf_tbl_dbg_counter_type_t *type,
                                             uint32_t *value) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, data = 0;
  *type = 0;
  *value = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* Read counter type */
  /* log tbl 0-7 in index 0, 8-15 in index 1 */
  reg_index = log_tbl_id / 8;
  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter_ctl[reg_index]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, &data);
  *type = (data >> ((log_tbl_id % 8) * PIPE_MGR_LOG_CNTR_TYPE_BITS)) &
          PIPE_MGR_LOG_CNTR_TYPE_MASK;

  /* Read counter value */
  address = offsetof(Tofino,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter[log_tbl_id]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, value);
  LOG_TRACE("%s: Dev %d pipe %d stage %d log_id %d counter %s is 0x%x",
            __func__,
            dev,
            pipe,
            stage,
            log_tbl_id,
            bf_tbl_dbg_counter_type_to_str(*type),
            *value);

  return status;
}

static pipe_status_t tbl_dbg_counter_get_tof2(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              uint8_t log_tbl_id,
                                              bf_tbl_dbg_counter_type_t *type,
                                              uint32_t *value) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, data = 0;
  *type = 0;
  *value = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* Read counter type */
  /* log tbl 0-7 in index 0, 8-15 in index 1 */
  reg_index = log_tbl_id / 8;
  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter_ctl[reg_index]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, &data);
  *type = (data >> ((log_tbl_id % 8) * PIPE_MGR_LOG_CNTR_TYPE_BITS)) &
          PIPE_MGR_LOG_CNTR_TYPE_MASK;

  /* Read counter value */
  address = offsetof(tof2_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter[log_tbl_id]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, value);
  LOG_TRACE("%s: Dev %d pipe %d stage %d log_id %d counter %s is 0x%x",
            __func__,
            dev,
            pipe,
            stage,
            log_tbl_id,
            bf_tbl_dbg_counter_type_to_str(*type),
            *value);

  return status;
}

static pipe_status_t tbl_dbg_counter_get_tof3(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              dev_stage_t stage,
                                              uint8_t log_tbl_id,
                                              bf_tbl_dbg_counter_type_t *type,
                                              uint32_t *value) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t phy_pipe = 0;
  int reg_index = 0;
  uint32_t address = 0, data = 0;
  *type = 0;
  *value = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  /* Get physical pipe from logical pipe */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  /* Read counter type */
  /* log tbl 0-7 in index 0, 8-15 in index 1 */
  reg_index = log_tbl_id / 8;
  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter_ctl[reg_index]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, &data);
  *type = (data >> ((log_tbl_id % 8) * PIPE_MGR_LOG_CNTR_TYPE_BITS)) &
          PIPE_MGR_LOG_CNTR_TYPE_MASK;

  /* Read counter value */
  address = offsetof(tof3_reg,
                     pipes[phy_pipe]
                         .mau[stage]
                         .rams.match.merge.mau_table_counter[log_tbl_id]);
  pipe_mgr_tbl_dbg_counter_read_register(dev_info->dev_id, address, value);
  LOG_TRACE("%s: Dev %d pipe %d stage %d log_id %d counter %s is 0x%x",
            __func__,
            dev,
            pipe,
            stage,
            log_tbl_id,
            bf_tbl_dbg_counter_type_to_str(*type),
            *value);

  return status;
}

/* Dbg counter get helper */
static pipe_status_t pipe_mgr_tbl_dbg_counter_get(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t log_tbl_id,
    bf_tbl_dbg_counter_type_t *type,
    uint32_t *value) {
  if (pipe == BF_DEV_PIPE_ALL) {
    LOG_ERROR("Invalid pipe %d ", pipe);
    return PIPE_INVALID_ARG;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tbl_dbg_counter_get_tof(
          dev_info, pipe, stage, log_tbl_id, type, value);
    case BF_DEV_FAMILY_TOFINO2:
      return tbl_dbg_counter_get_tof2(
          dev_info, pipe, stage, log_tbl_id, type, value);
    case BF_DEV_FAMILY_TOFINO3:
      return tbl_dbg_counter_get_tof3(
          dev_info, pipe, stage, log_tbl_id, type, value);
    default:
      PIPE_MGR_DBGCHK(0);
  }
  return PIPE_UNEXPECTED;
}

/* Dbg counter get */
pipe_status_t bf_tbl_dbg_counter_get(dev_target_t dev_tgt,
                                     char *tbl_name,
                                     bf_tbl_dbg_counter_type_t *type,
                                     uint32_t *value) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  dev_stage_t stage = 0;
  int num_entries = 0, idx = 0;
  uint8_t log_tbl_id = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  uint32_t temp_value = 0;
  uint32_t pipe_value = 0;
  bf_tbl_dbg_counter_type_t temp_type = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }

  *type = 0;
  *value = 0;

  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));
    pipe_value = 0;
    /* Get log-tbl_id list from table name */
    pipe_mgr_tbl_dbg_counter_log_id_list(
        dev, pipe_idx, tbl_name, 0, false, &log_id_arr[0], &num_entries);
    if (num_entries == 0) {
      LOG_TRACE("Invalid table name %s for pipe %d", tbl_name, pipe_idx);
      return PIPE_INVALID_ARG;
    }

    /* Go over all the logical ids */
    int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
    for (idx = 0; idx < num_entries; idx++) {
      stage = log_id_arr[idx] / num_lts;
      log_tbl_id = log_id_arr[idx] % num_lts;
      temp_type = 0;
      temp_value = 0;
      status = pipe_mgr_tbl_dbg_counter_get(
          dev_info, pipe_idx, stage, log_tbl_id, &temp_type, &temp_value);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR("Failed to get tbl dbg counter for tbl-id %d ",
                  log_id_arr[idx]);
      }
      if ((*type != 0) && (*type != temp_type)) {
        LOG_ERROR("Log tbl cnt types are different for table %s ", tbl_name);
        return PIPE_INVALID_ARG;
      }
      *type = temp_type;
      if ((temp_type == BF_TBL_DBG_CNTR_LOG_TBL_HIT) ||
          (temp_type == BF_TBL_DBG_CNTR_GW_TBL_HIT)) {
        pipe_value += temp_value;
      } else {
        /* For misses, use value of last log-tbl */
        pipe_value = temp_value;
      }
    }
    *value += pipe_value;
  }
  return status;
}

pipe_status_t bf_log_tbl_dbg_counter_get(dev_target_t dev_tgt,
                                         uint32_t stage,
                                         uint32_t log_tbl_id,
                                         bf_tbl_dbg_counter_type_t *type,
                                         uint32_t *value) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  uint32_t temp_value = 0;
  bf_tbl_dbg_counter_type_t temp_type = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }

  *type = 0;
  *value = 0;

  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    temp_type = 0;
    temp_value = 0;
    status = pipe_mgr_tbl_dbg_counter_get(
        dev_info, pipe_idx, stage, log_tbl_id, &temp_type, &temp_value);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to get tbl dbg counter for pipe %d stage %d log_tbl %d",
                pipe,
                stage,
                log_tbl_id);
    }
    if ((*type != 0) && (*type != temp_type)) {
      LOG_ERROR(
          "Log tbl cnt types are different for pipe %d stage %d log_tbl %d",
          pipe,
          stage,
          log_tbl_id);
      return PIPE_INVALID_ARG;
    }
    *type = temp_type;
    *value += temp_value;
  }
  return status;
}

static pipe_status_t tbl_dbg_counter_clear_tof(rmt_dev_info_t *dev_info,
                                               bf_dev_pipe_t pipe,
                                               dev_stage_t stage,
                                               uint8_t log_tbl_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    address = offsetof(
        Tofino,
        pipes[phy_pipe].mau[stage].rams.match.merge.mau_table_counter_clear);

    /* Each logical id has 1 bit to clear it */
    data = 0x1 << log_tbl_id;
    LOG_TRACE("%s: dev %d pipe %d stage %d log_tbl %d clearing counter",
              __func__,
              dev,
              pipe_idx,
              stage,
              log_tbl_id);
    pipe_mgr_tbl_db_counter_write_register(dev_info->dev_id, 0, address, data);
  }

  return status;
}

static pipe_status_t tbl_dbg_counter_clear_tof2(rmt_dev_info_t *dev_info,
                                                bf_dev_pipe_t pipe,
                                                dev_stage_t stage,
                                                uint8_t log_tbl_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    address = offsetof(
        tof2_reg,
        pipes[phy_pipe].mau[stage].rams.match.merge.mau_table_counter_clear);

    /* Each logical id has 1 bit to clear it */
    data = 0x1 << log_tbl_id;
    LOG_TRACE("%s: dev %d pipe %d stage %d log_tbl %d clearing counter",
              __func__,
              dev,
              pipe_idx,
              stage,
              log_tbl_id);
    pipe_mgr_tbl_db_counter_write_register(dev_info->dev_id, 0, address, data);
  }

  return status;
}

static pipe_status_t tbl_dbg_counter_clear_tof3(rmt_dev_info_t *dev_info,
                                                bf_dev_pipe_t pipe,
                                                dev_stage_t stage,
                                                uint8_t log_tbl_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t start_pipe = 0, pipe_count = 0, pipe_idx = 0;
  bf_dev_pipe_t phy_pipe = 0;
  uint32_t address = 0, data = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  start_pipe = pipe == BF_DEV_PIPE_ALL ? 0 : pipe;
  pipe_count = pipe == BF_DEV_PIPE_ALL ? dev_info->num_active_pipes : 1;

  for (pipe_idx = start_pipe; pipe_idx < (start_pipe + pipe_count);
       pipe_idx++) {
    /* Get physical pipe from logical pipe */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_idx, &phy_pipe);

    address = offsetof(tof3_reg,
                       pipes[phy_pipe % BF_SUBDEV_PIPE_COUNT]
                           .mau[stage]
                           .rams.match.merge.mau_table_counter_clear);

    /* Each logical id has 1 bit to clear it */
    data = 0x1 << log_tbl_id;
    LOG_TRACE("%s: dev %d pipe %d stage %d log_tbl %d clearing counter",
              __func__,
              dev,
              pipe_idx,
              stage,
              log_tbl_id);
    pipe_mgr_tbl_db_counter_write_register(
        dev_info->dev_id, phy_pipe / BF_SUBDEV_PIPE_COUNT, address, data);
  }

  return status;
}

/* Dbg counter clear helper  */
static pipe_status_t pipe_mgr_tbl_dbg_counter_clear(rmt_dev_info_t *dev_info,
                                                    bf_dev_pipe_t pipe,
                                                    dev_stage_t stage,
                                                    uint8_t log_tbl_id) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tbl_dbg_counter_clear_tof(dev_info, pipe, stage, log_tbl_id);
    case BF_DEV_FAMILY_TOFINO2:
      return tbl_dbg_counter_clear_tof2(dev_info, pipe, stage, log_tbl_id);
    case BF_DEV_FAMILY_TOFINO3:
      return tbl_dbg_counter_clear_tof3(dev_info, pipe, stage, log_tbl_id);
    default:
      PIPE_MGR_DBGCHK(0);
  }
  return PIPE_UNEXPECTED;
}

/* Dbg counter clear */
pipe_status_t bf_tbl_dbg_counter_clear(dev_target_t dev_tgt, char *tbl_name) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  dev_stage_t stage = 0;
  uint8_t log_tbl_id = 0;
  int num_entries = 0, idx = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));

  /* Get log-tbl_id list from table name */
  pipe_mgr_tbl_dbg_counter_log_id_list(
      dev, pipe, tbl_name, 0, false, &log_id_arr[0], &num_entries);
  if (num_entries == 0) {
    LOG_ERROR("Invalid table name %s ", tbl_name);
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (idx = 0; idx < num_entries; idx++) {
    log_tbl_id = log_id_arr[idx] % num_lts;
    stage = log_id_arr[idx] / num_lts;
    status = pipe_mgr_tbl_dbg_counter_clear(dev_info, pipe, stage, log_tbl_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to clear tbl dbg counter for tbl-id %d ",
                log_id_arr[idx]);
    }
  }

  pipe_mgr_api_exit(shdl);
  return status;
}

pipe_status_t bf_log_tbl_dbg_counter_clear(dev_target_t dev_tgt,
                                           uint32_t stage,
                                           uint32_t log_tbl_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  status = pipe_mgr_tbl_dbg_counter_clear(dev_info, pipe, stage, log_tbl_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("Failed to clear tbl dbg counter for pipe %d stage %d log_tbl %d",
              pipe,
              stage,
              log_tbl_id);
  }

  pipe_mgr_api_exit(shdl);
  return status;
}
/* Dbg counter type set on all tables in stage  */
pipe_status_t bf_tbl_dbg_counter_type_stage_set(
    dev_target_t dev_tgt, dev_stage_t stage, bf_tbl_dbg_counter_type_t type) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  int num_entries = 0, idx = 0;
  uint8_t log_tbl_id = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_stage_valid(dev_info, stage)) {
    return PIPE_INVALID_ARG;
  }
  if (type >= BF_TBL_DBG_CNTR_MAX) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));

  /* Get log-tbl_id list from stage */
  pipe_mgr_tbl_dbg_counter_log_id_list(
      dev, pipe, NULL, stage, true, &log_id_arr[0], &num_entries);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (idx = 0; idx < num_entries; idx++) {
    log_tbl_id = log_id_arr[idx] % num_lts;
    status = pipe_mgr_tbl_dbg_counter_type_set(
        dev_info, pipe, stage, log_tbl_id, type);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set tbl dbg counter for tbl-id %d ",
                log_id_arr[idx]);
    }
  }

  pipe_mgr_api_exit(shdl);
  return status;
}

/* Dbg counter get on all tables in stage */
pipe_status_t bf_tbl_dbg_counter_stage_get(
    dev_target_t dev_tgt,
    dev_stage_t stage,
    bf_tbl_dbg_counter_type_t *type_arr,
    uint32_t *value_arr,
    char tbl_name[][PIPE_MGR_TBL_NAME_LEN],
    int *num_counters) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  int num_entries = 0, idx = 0;
  uint8_t log_tbl_id = 0;
  uint32_t value = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  char buf[PIPE_MGR_TBL_NAME_LEN];
  bf_tbl_dbg_counter_type_t temp_type = 0;
  profile_id_t prof_id = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_stage_valid(dev_info, stage)) {
    return PIPE_INVALID_ARG;
  }
  if (pipe == BF_DEV_PIPE_ALL) {
    LOG_ERROR("Invalid pipe %d ", pipe);
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) !=
      PIPE_SUCCESS) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));
  PIPE_MGR_MEMSET(buf, 0, sizeof(buf));
  *num_counters = 0;

  /* Get log-tbl_id list from stage */
  pipe_mgr_tbl_dbg_counter_log_id_list(
      dev, pipe, NULL, stage, true, &log_id_arr[0], &num_entries);

  /* Go over all the logical ids */
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (idx = 0; (idx < num_entries) && (idx < num_lts); idx++) {
    log_tbl_id = log_id_arr[idx] % num_lts;
    status = pipe_mgr_tbl_dbg_counter_get(
        dev_info, pipe, stage, log_tbl_id, &temp_type, &value);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to get tbl dbg counter for tbl-id %d ",
                log_id_arr[idx]);
    }
    type_arr[idx] = temp_type;
    value_arr[idx] = value;
    PIPE_MGR_MEMSET(buf, 0, sizeof(buf));
    /* Get table name from logical id */
    status = pipe_mgr_ctxjson_tof_log_id_to_tbl_name(
        dev, prof_id, log_id_arr[idx], buf);
    PIPE_MGR_MEMSET(tbl_name[idx], 0, PIPE_MGR_TBL_NAME_LEN);
    strncpy(tbl_name[idx], buf, PIPE_MGR_TBL_NAME_LEN);
    tbl_name[idx][PIPE_MGR_TBL_NAME_LEN - 1] = '\0';

    *num_counters += 1;
  }

  return status;
}

/* Dbg counter clear on all tables in stage  */
pipe_status_t bf_tbl_dbg_counter_stage_clear(dev_target_t dev_tgt,
                                             dev_stage_t stage) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  uint8_t log_tbl_id = 0;
  int num_entries = 0, idx = 0;
  uint32_t log_id_arr[PIPE_MGR_TBL_MAX_LOG_IDS];
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_stage_valid(dev_info, stage)) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_MEMSET(log_id_arr, 0, sizeof(log_id_arr));

  /* Get log-tbl_id list from stage */
  pipe_mgr_tbl_dbg_counter_log_id_list(
      dev, pipe, NULL, stage, true, &log_id_arr[0], &num_entries);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  /* Go over all the logical ids */
  int num_lts = dev_info->dev_cfg.stage_cfg.num_logical_tables;
  for (idx = 0; idx < num_entries; idx++) {
    log_tbl_id = log_id_arr[idx] % num_lts;
    if (stage != (log_id_arr[idx] / num_lts)) {
      LOG_ERROR("Stage value %d in log-id is different ", stage);
    }
    status = pipe_mgr_tbl_dbg_counter_clear(dev_info, pipe, stage, log_tbl_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("Failed to clear tbl dbg counter for tbl-id %d ",
                log_id_arr[idx]);
    }
  }

  pipe_mgr_api_exit(shdl);
  return status;
}

/* Get direction of table from table name */
int pipe_mgr_tbl_name_to_dir(bf_dev_id_t dev,
                             bf_dev_pipe_t pipe,
                             char *tbl_name) {
  int dir = 0;
  profile_id_t prof_id = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) !=
      PIPE_SUCCESS) {
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_ctxjson_tof_tbl_name_to_direction(dev, prof_id, tbl_name, &dir);

  return dir;
}

bf_status_t pipe_mgr_tbl_dbg_counter_get_list(dev_target_t dev_tgt,
                                              char **tbl_names,
                                              int *num_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  profile_id_t prof_id = 0;
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t pipe = dev_tgt.dev_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return PIPE_INVALID_ARG;
  if (!tbl_names) return PIPE_INVALID_ARG;

  if (!PIPE_MGR_TBL_DBG_CNTR_DEV_VALID(dev)) {
    return PIPE_INVALID_ARG;
  }
  if (!tbl_dbg_cntr_pipe_valid(dev_info, pipe)) {
    return PIPE_INVALID_ARG;
  }
  if (pipe_mgr_pipe_to_profile(dev_info, pipe, &prof_id, __func__, __LINE__) !=
      PIPE_SUCCESS) {
    return PIPE_INVALID_ARG;
  }
  int num_logical_tables =
      dev_info->num_active_mau * dev_info->dev_cfg.stage_cfg.num_logical_tables;
  int i = 0;
  for (int log_id = 0; log_id < num_logical_tables; log_id++) {
    if (tbl_names[i] == NULL) {
      LOG_ERROR(
          "Provided array is not big enough to contain all table names. ");
      return PIPE_INVALID_ARG;
    }
    status = pipe_mgr_ctxjson_tof_log_id_to_tbl_name(
        dev, prof_id, log_id, tbl_names[i]);
    /* If table not found, just continue. */
    if (status == PIPE_OBJ_NOT_FOUND)
      continue;
    else if (status != PIPE_SUCCESS)
      return status;
    i++;
  }

  *num_tbl = i;
  return PIPE_SUCCESS;
}
