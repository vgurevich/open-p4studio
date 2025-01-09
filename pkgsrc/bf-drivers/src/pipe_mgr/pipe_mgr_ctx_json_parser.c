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


/**
 * @file pipe_mgr_ctx_json_parser.c
 * @date 08/2017
 *
 * Parse the parser PVS information from the ContextJSON.
 */

#include <bf_types/bf_types.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <target-utils/third-party/cJSON/cJSON.h>

#include "pipe_mgr_ctx_json.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_p4parser.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_db.h"
#include <tofino_regs/pipe_top_level.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof3_regs/tof3_mem_drv.h>

extern pipemgr_p4parser_ctx_t *g_p4parser_ctx[BF_MAX_DEV_COUNT];
#define PIPE_MGR_P4PARSER_CTX(_dev, _prof) \
  (g_p4parser_ctx[_dev]->profiles[_prof])

pipe_status_t pipe_mgr_pvs_get_global_gress_node(
    bf_dev_id_t devid,
    uint32_t pvs_handle,
    pipemgr_p4parser_global_gress_node_t **pvs_node) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipemgr_p4parser_global_gress_node_t *htbl_node;
  profile_id_t prof_id = 0;

  *pvs_node = NULL;

  for (prof_id = 0; prof_id < (int)g_p4parser_ctx[devid]->num_profiles;
       prof_id++) {
    map_sts = bf_map_get(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                         (unsigned long)pvs_handle,
                         (void **)&htbl_node);
    if (map_sts == BF_MAP_NO_KEY) {
      continue;
    }
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in getting global pvs node for dev: %d, prof %d pvs "
          "handle :0x%x",
          __func__,
          __LINE__,
          devid,
          prof_id,
          pvs_handle);
      return PIPE_INVALID_ARG;
    }

    *pvs_node = htbl_node;
    return PIPE_SUCCESS;
  }
  return PIPE_OBJ_NOT_FOUND;
}

static void ctx_json_parse_parser_free_this_node(
    bf_dev_id_t dev_id, pipemgr_p4parser_pvs_t *pvs_node) {
  uint32_t i, j;
  rmt_dev_info_t *dev_info = NULL;
  uint8_t prsr_state_no = 0;
  if (!pvs_node) return;

  prsr_state_no = pvs_node->parser_state_numb - 1;
  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return;
  }

  if (pvs_node->encoding) PIPE_MGR_FREE(pvs_node->encoding);

  if (pvs_node->pipe_instance[prsr_state_no]) {
    for (i = 0; i < dev_info->num_active_pipes; i++) {
      if (pvs_node->pipe_instance[prsr_state_no][i].parser_instance) {
        for (j = 0; j < dev_info->dev_cfg.num_prsrs; j++) {
          if (pvs_node->pipe_instance[prsr_state_no][i]
                  .parser_instance[j]
                  .tcam_rows) {
            PIPE_MGR_FREE(pvs_node->pipe_instance[prsr_state_no][i]
                              .parser_instance[j]
                              .tcam_rows);
          }
        }
        PIPE_MGR_FREE(
            pvs_node->pipe_instance[prsr_state_no][i].parser_instance);
      }
    }
    PIPE_MGR_FREE(pvs_node->pipe_instance[prsr_state_no]);
  }
  PIPE_MGR_FREE(pvs_node);
}

void pipe_mgr_free_pvs_helper(bf_dev_id_t devid, profile_id_t prof_id) {
  pipemgr_p4parser_global_gress_node_t *gress_node;
  unsigned long key = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;

  if (!g_p4parser_ctx[devid]) return;

  map_sts =
      bf_map_get_first(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                       &key,
                       (void **)&gress_node);
  while (map_sts == BF_MAP_OK) {
    PIPE_MGR_FREE(gress_node->pvs_name);
    ctx_json_parse_parser_free_this_node(devid, gress_node->ingress_pvs);
    ctx_json_parse_parser_free_this_node(devid, gress_node->egress_pvs);
    map_sts =
        bf_map_get_next(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
                        &key,
                        (void **)&gress_node);
  }
}

static pipe_status_t ctx_json_parse_parser_tcam_entries(
    bf_dev_id_t dev_id, cJSON *pvs_cjson, pipemgr_p4parser_pvs_t *pvs_node) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = NULL;
  uint32_t i = 0, j = 0;
  uint8_t prsr_state_no = 0;

  if (!pvs_cjson || !pvs_node) {
    LOG_ERROR("%s:%d: NULL parameter passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  prsr_state_no = pvs_node->parser_state_numb - 1;

  dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }

  // Parse the TCAM rows.
  cJSON *tcam_rows_cjson = NULL;
  err |=
      bf_cjson_get_object(pvs_cjson, CTX_JSON_PVS_TCAM_ROWS, &tcam_rows_cjson);
  CHECK_ERR(err, rc, cleanup);

  if (pvs_node->pvs_size == 0) {
    pvs_node->pvs_size = GETARRSZ(tcam_rows_cjson);
    if (!pvs_node->pvs_size) {
      LOG_ERROR("%s: Invalid pvs size (%d)", __func__, pvs_node->pvs_size);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  } else {
    if (pvs_node->pvs_size != GETARRSZ(tcam_rows_cjson)) {
      LOG_ERROR("%s: Invalid pvs size %d in state_id %d, existing pvs size %d",
                __func__,
                GETARRSZ(tcam_rows_cjson),
                pvs_node->parser_state_id[prsr_state_no],
                pvs_node->pvs_size);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  }

  pvs_node->pipe_instance[prsr_state_no] =
      PIPE_MGR_CALLOC(dev_info->num_active_pipes,
                      sizeof(pipemgr_p4parser_pvs_pipe_parser_instance_t));
  if (!pvs_node->pipe_instance[prsr_state_no]) {
    LOG_ERROR("%s: Failed to allocate memory for parsers for %d pipes in PVS",
              __func__,
              dev_info->num_active_pipes);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  for (i = 0; i < dev_info->num_active_pipes; i++) {
    pvs_node->pipe_instance[prsr_state_no][i].parser_instance =
        PIPE_MGR_CALLOC(dev_info->dev_cfg.num_prsrs,
                        sizeof(pipemgr_p4parser_pvs_hw_parser_instance_t));
    if (!pvs_node->pipe_instance[prsr_state_no][i].parser_instance) {
      LOG_ERROR("%s: Failed to allocate memory for %d parsers in PVS",
                __func__,
                dev_info->dev_cfg.num_prsrs);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    for (j = 0; j < dev_info->dev_cfg.num_prsrs; j++) {
      pvs_node->pipe_instance[prsr_state_no][i].parser_instance[j].tcam_rows =
          PIPE_MGR_CALLOC(pvs_node->pvs_size,
                          sizeof *pvs_node->pipe_instance[prsr_state_no]
                              ->parser_instance->tcam_rows);
      if (!pvs_node->pipe_instance[prsr_state_no][i]
               .parser_instance[j]
               .tcam_rows) {
        LOG_ERROR("%s: Failed to allocate memory for %d rows in PVS",
                  __func__,
                  pvs_node->pvs_size);
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      cJSON *tcam_row_cjson = NULL;
      int row = 0;
      CTX_JSON_FOR_EACH(tcam_row_cjson, tcam_rows_cjson) {
        pvs_node->pipe_instance[prsr_state_no][i]
            .parser_instance[j]
            .tcam_rows[row]
            .tcam_row = tcam_row_cjson->valueint;
        pvs_node->pipe_instance[prsr_state_no][i]
            .parser_instance[j]
            .tcam_rows[row]
            .allocated = PIPEMGR_P4PARSER_PRSR_TCAM_FREE;
        ++row;
      }
    }
  }

  cJSON *match_registers_cjson = NULL;
  err |= bf_cjson_get_object(
      pvs_cjson, CTX_JSON_PVS_MATCH_REGISTERS, &match_registers_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_register_cjson = NULL;
  CTX_JSON_FOR_EACH(match_register_cjson, match_registers_cjson) {
    int container_hardware_id = 0;
    int container_width = 0, expected_width = 0;
    int container_shift = 0;
    err |= bf_cjson_get_int(match_register_cjson,
                            CTX_JSON_MATCH_REGISTER_CONTAINER_HARDWARE_ID,
                            &container_hardware_id);
    err |= bf_cjson_get_int(match_register_cjson,
                            CTX_JSON_MATCH_REGISTER_CONTAINER_WIDTH,
                            &container_width);
    CHECK_ERR(err, rc, cleanup);

    if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
      switch (container_hardware_id) {
        case 0:
          container_shift = 0;
          expected_width = 16;
          break;
        case 2:
          container_shift = 16;
          expected_width = 8;
          break;
        case 3:
          container_shift = 24;
          expected_width = 8;
          break;
      }
    } else if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
               (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
      if (container_hardware_id < 4) {
        container_shift = (container_hardware_id)*8;
        expected_width = 8;
      }
    } else {
      LOG_ERROR("%s: Invalid dev_family %d", __func__, dev_info->dev_family);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    if (expected_width != container_width) {
      LOG_ERROR(
          "%s: Invalid PVS configuration: container %d has unexpected width "
          "%d.",
          __func__,
          container_hardware_id,
          container_width);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    /* Parse the field mapping which specifies how the input bits map the the
     * container bits. */
    cJSON *field_mappings_cjson = NULL;
    cJSON *field_mapping_cjson = NULL;
    struct pipe_mgr_pvs_encoding *encoding = NULL;
    err = bf_cjson_get_object(match_register_cjson,
                              CTX_JSON_PVS_MATCH_REGISTER_FIELD_MAPPING,
                              &field_mappings_cjson);
    CHECK_ERR(err, rc, cleanup);
    int old_count = pvs_node->encoding_cnt;
    pvs_node->encoding_cnt += GETARRSZ(field_mappings_cjson);
    if (!pvs_node->encoding_cnt) {
      LOG_ERROR("%s: No encoding data for PVS", __func__);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
    pvs_node->encoding =
        PIPE_MGR_REALLOC(pvs_node->encoding,
                         pvs_node->encoding_cnt * sizeof *pvs_node->encoding);
    encoding = &pvs_node->encoding[old_count];
    CTX_JSON_FOR_EACH(field_mapping_cjson, field_mappings_cjson) {
      int reg_bit, select_bit;
      err = bf_cjson_get_int(
          field_mapping_cjson,
          CTX_JSON_PVS_MATCH_REGISTER_FIELD_MAPPING_REGISTER_BIT,
          &reg_bit);
      CHECK_ERR(err, rc, cleanup);
      if (reg_bit >= container_width) {
        LOG_ERROR(
            "%s: Has invalid PVS configuration: container %d width %d but "
            "reg bit %d.",
            __func__,
            container_hardware_id,
            container_width,
            reg_bit);
        PIPE_MGR_DBGCHK(reg_bit < container_width);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      err = bf_cjson_get_int(
          field_mapping_cjson,
          CTX_JSON_PVS_MATCH_REGISTER_FIELD_MAPPING_SELECT_STATEMENT_BIT,
          &select_bit);
      CHECK_ERR(err, rc, cleanup);
      encoding->src_data_mask = 1u << select_bit;
      encoding->masked_src_right_shift = select_bit;
      encoding->masked_src_left_shift = reg_bit + container_shift;
      if (encoding->masked_src_right_shift > encoding->masked_src_left_shift) {
        encoding->masked_src_right_shift -= encoding->masked_src_left_shift;
        encoding->masked_src_left_shift = 0;
      } else {
        encoding->masked_src_left_shift -= encoding->masked_src_right_shift;
        encoding->masked_src_right_shift = 0;
      }
      ++encoding;
    }
  }

  return rc;

cleanup:
  ctx_json_parse_parser_free_this_node(dev_id, pvs_node);
  return rc;
}

static void ctx_json_parse_parser_free_global_pvs(bf_dev_id_t devid,
                                                  profile_id_t prof_id) {
  pipemgr_p4parser_global_gress_node_t *gress_node;
  unsigned long key = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;

  if (!g_p4parser_ctx[devid]) return;

  pipe_mgr_free_pvs_helper(devid, prof_id);

  while ((map_sts = bf_map_get_first_rmv(
              &PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl,
              &key,
              (void **)&gress_node)) == BF_MAP_OK) {
    PIPE_MGR_FREE(gress_node);
  }

  bf_map_destroy(&PIPE_MGR_P4PARSER_CTX(devid, prof_id).gbl_hash_tbl);
  PIPE_MGR_FREE(g_p4parser_ctx[devid]);
  g_p4parser_ctx[devid] = NULL;
}

static pipe_status_t pipe_mgr_parser_instance_init(
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    char *prsr_instance_name,
    struct pipe_mgr_prsr_instance_t *instance,
    uint32_t *pvs_hdl,
    uint32_t pvs_numb,
    uint64_t prsr_map,
    uint32_t phase0_hdl) {
  if ((instance->pvs_hdl != NULL) || (instance->name != NULL)) {
    LOG_ERROR(
        "%s:%d the shadow of the prsr instance 0x%x is already initialized",
        __func__,
        __LINE__,
        prsr_instance_hdl);
  }
  instance->prsr_map = prsr_map;
  instance->prsr_map_default = prsr_map;
  instance->pvs_hdl_numb = pvs_numb;
  instance->pvs_hdl = PIPE_MGR_CALLOC(pvs_numb, sizeof(uint32_t));
  if (instance->pvs_hdl == NULL) return PIPE_NO_SYS_RESOURCES;
  PIPE_MGR_MEMCPY(instance->pvs_hdl, pvs_hdl, pvs_numb * sizeof(uint32_t));
  instance->name = bf_sys_strdup(prsr_instance_name);
  instance->phase0_hdl = phase0_hdl;
  // init prsr_reg_data to default values
  // register hdr_len_adj
  instance->bin_cfg.tof.prsr_reg_data[4] = 0x10;
  return PIPE_SUCCESS;
}

static pipe_status_t ctx_json_parse_parser_pvs(
    bf_dev_id_t dev_id,
    profile_id_t prof_id,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    cJSON *prsr_instance_states,
    uint32_t *pvs_hdl,
    uint32_t *pvs_numb) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;

  pipemgr_p4parser_pvs_t *pvs_node = NULL;
  pipemgr_p4parser_global_gress_node_t *global_gress_node = NULL;

  cJSON *pvs_cjson = NULL;
  CTX_JSON_FOR_EACH(pvs_cjson, prsr_instance_states) {
    bool uses_pvs = true;
    err = bf_cjson_try_get_bool(pvs_cjson, CTX_JSON_PVS_USES_PVS, &uses_pvs);
    if (err != 0) return PIPE_UNEXPECTED;
    if (uses_pvs) {
      // Parse name, state_id, handle, gress and mode.
      char *pvs_name = NULL;
      int parser_state_id = 0;
      uint32_t pvs_handle = 0;
      int handle = 0;
      err = bf_cjson_get_string(pvs_cjson, CTX_JSON_PVS_PVS_NAME, &pvs_name);
      if (err != 0) return PIPE_UNEXPECTED;
      err = bf_cjson_get_int(
          pvs_cjson, CTX_JSON_PVS_PARSER_STATE_ID, &parser_state_id);
      if (err != 0) return PIPE_UNEXPECTED;
      err = bf_cjson_get_handle(
          dev_id, prof_id, pvs_cjson, CTX_JSON_PVS_PVS_HANDLE, &handle);
      if (err != 0) return PIPE_UNEXPECTED;
      pvs_handle = (uint32_t)handle;
      if (*pvs_numb >= 256) {
        LOG_ERROR(
            "%s:%d the parser program cannot have more than 256 pvs tables, "
            "dev %d",
            __func__,
            __LINE__,
            dev_id);
        rc = PIPE_UNEXPECTED;
        return rc;
      }
      pvs_hdl[(*pvs_numb)++] = pvs_handle;

      // Get the global egress parser node instance with this pvs_handle
      rc = pipe_mgr_pvs_get_global_gress_node(
          dev_id, pvs_handle, &global_gress_node);
      if (rc == PIPE_OBJ_NOT_FOUND) {
        // Indicates that we have not seen this pvs node before
        global_gress_node =
            PIPE_MGR_CALLOC(1, sizeof(pipemgr_p4parser_global_gress_node_t));
        if (global_gress_node == NULL) {
          rc = PIPE_NO_SYS_RESOURCES;
          return rc;
        }
        map_sts =
            bf_map_add(&PIPE_MGR_P4PARSER_CTX(dev_id, prof_id).gbl_hash_tbl,
                       (unsigned long)pvs_handle,
                       (void *)global_gress_node);
        if (map_sts != BF_MAP_OK) {
          LOG_ERROR(
              "%s:%d Unable to add gress node into the hash map for dev :%d, "
              "pvs_handle :0x%x",
              __func__,
              __LINE__,
              dev_id,
              pvs_handle);
          rc = PIPE_UNEXPECTED;
          return rc;
        }
        global_gress_node->pvs_handle = pvs_handle;
        global_gress_node->pvs_name = bf_sys_strdup(pvs_name);
        global_gress_node->profile_id = prof_id;
        global_gress_node->gress_scope = PIPE_MGR_PVS_SCOPE_ALL_GRESS;
        global_gress_node->prsr_instance_hdl = prsr_instance_hdl;
        if (global_gress_node->ent_hdl_mgr == NULL) {
          global_gress_node->ent_hdl_mgr =
              (pipemgr_p4parser_ent_hdl_mgmt_t *)PIPE_MGR_CALLOC(
                  1, sizeof(pipemgr_p4parser_ent_hdl_mgmt_t));
          if (global_gress_node->ent_hdl_mgr == NULL) {
            rc = PIPE_NO_SYS_RESOURCES;
            return rc;
          }
        }
        pvs_node = PIPE_MGR_CALLOC(1, sizeof(pipemgr_p4parser_pvs_t));
        if (pvs_node == NULL) {
          rc = PIPE_NO_SYS_RESOURCES;
          return rc;
        }
        pvs_node->gress = gress;
        pvs_node->parser_state_numb = 1;
        pvs_node->parser_state_id[0] = parser_state_id;
        pvs_node->parser_scope = PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE;
        pvs_node->pipe_scope = PIPE_MGR_PVS_SCOPE_ALL_PIPELINES;
        pvs_node->pvs_size = 0;
        // Parse tcam entries.
        rc = ctx_json_parse_parser_tcam_entries(dev_id, pvs_cjson, pvs_node);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Unable to parse tcam entries for dev :%d, pvs_handle "
              ":0x%x "
              "pvs name :%s",
              __func__,
              __LINE__,
              dev_id,
              pvs_handle,
              global_gress_node->pvs_name);
          return rc;
        }
        if (gress == BF_DEV_DIR_INGRESS) {
          global_gress_node->ingress_pvs = pvs_node;
        } else {
          global_gress_node->egress_pvs = pvs_node;
        }
      } else if (rc == PIPE_SUCCESS) {
        // already existing, add state_id and rows.
        if (gress == BF_DEV_DIR_INGRESS) {
          pvs_node = global_gress_node->ingress_pvs;
        } else {
          pvs_node = global_gress_node->egress_pvs;
        }
        if (pvs_node != NULL) {
          if (pvs_node->parser_state_numb >= PIPE_PVS_MAX_SHARE_PVS_STATE) {
            LOG_ERROR(
                "%s:%d Unable to share the same pvs table in more than %d "
                "states"
                "for dev :%d",
                __func__,
                __LINE__,
                PIPE_PVS_MAX_SHARE_PVS_STATE,
                dev_id);
            rc = PIPE_UNEXPECTED;
            return rc;
          }
          pvs_node->parser_state_id[pvs_node->parser_state_numb++] =
              parser_state_id;
        } else {
          pvs_node = PIPE_MGR_CALLOC(1, sizeof(pipemgr_p4parser_pvs_t));
          if (pvs_node == NULL) {
            rc = PIPE_NO_SYS_RESOURCES;
            return rc;
          }
          pvs_node->gress = gress;
          pvs_node->parser_state_numb = 1;
          pvs_node->parser_state_id[0] = parser_state_id;
          pvs_node->parser_scope = PIPE_MGR_PVS_SCOPE_ALL_PARSERS_IN_PIPE;
          pvs_node->pipe_scope = PIPE_MGR_PVS_SCOPE_ALL_PIPELINES;
          pvs_node->pvs_size = 0;
          if (gress == BF_DEV_DIR_INGRESS) {
            global_gress_node->ingress_pvs = pvs_node;
          } else {
            global_gress_node->egress_pvs = pvs_node;
          }
        }
        // Parse tcam entries.
        rc = ctx_json_parse_parser_tcam_entries(dev_id, pvs_cjson, pvs_node);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Unable to parse tcam entries for dev :%d, pvs_handle "
              ":0x%x "
              "pvs name :%s",
              __func__,
              __LINE__,
              dev_id,
              pvs_handle,
              global_gress_node->pvs_name);
          return rc;
        }
      } else if (rc != PIPE_SUCCESS) {
        PIPE_MGR_DBGCHK(0);
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t ctx_json_parse_parser_default_profile(
    rmt_dev_info_t *dev_info,
    cJSON *prsr_instance_default,
    uint64_t *prsr_map) {
  if (prsr_map == NULL) return PIPE_INVALID_ARG;
  int prsr_max;
  uint64_t prsr_map_tmp = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      prsr_max = TOF_NUM_PARSERS - 1;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      prsr_max = (TOF2_NUM_PARSERS / 4) - 1;  // four prsr per prsr group
      break;
    case BF_DEV_FAMILY_TOFINO3:
      prsr_max = (TOF3_NUM_PARSERS / 4) - 1;  // four prsr per prsr group
      break;
    default:
      LOG_ERROR("%s: Invalid dev_family %d", __func__, dev_info->dev_family);
      return PIPE_INVALID_ARG;
  }
  if (prsr_instance_default == NULL) {
    for (int prsr_id = 0; prsr_id <= prsr_max; prsr_id++) {
      prsr_map_tmp |= (1u << prsr_id);
    }
  } else {
    cJSON *prsr_id_cjson = NULL;
    int prsr_id;
    CTX_JSON_FOR_EACH(prsr_id_cjson, prsr_instance_default) {
      prsr_id = prsr_id_cjson->valueint;
      if (prsr_id > prsr_max) {
        LOG_ERROR("%s: Invalid parser number %d", __func__, prsr_id);
        return PIPE_INVALID_ARG;
      }
      prsr_map_tmp |= (1u << prsr_id);
    }
  }
  *prsr_map = prsr_map_tmp;
  return PIPE_SUCCESS;
}

static pipe_status_t ctx_json_parse_parser_gress(bf_dev_id_t dev_id,
                                                 profile_id_t prof_id,
                                                 cJSON *gress_cjson,
                                                 uint8_t gress,
                                                 bool multi_parser_programs,
                                                 bool virtual_device) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;

  cJSON *prsr_instance_cjson = NULL;
  cJSON *prsr_instance_states = NULL;
  cJSON *prsr_instance_default = NULL;

  pipe_prsr_instance_hdl_t prsr_instance_hdl;
  char *prsr_instance_name;
  uint32_t pvs_hdl[256];
  uint32_t pvs_numb = 0;
  uint32_t log_pipe_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  struct pipe_mgr_prsr_instance_t *instance = NULL;
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_INVALID_ARG;
  }
  // Iterate over all prsr instance.
  CTX_JSON_FOR_EACH(prsr_instance_cjson, gress_cjson) {
    if (multi_parser_programs) {
      // Get prsr instance handler, states, default profile
      int handle = 0;
      err |= bf_cjson_get_handle(dev_id,
                                 prof_id,
                                 prsr_instance_cjson,
                                 CTX_JSON_PARSER_INSTANCE_HDL2,
                                 &handle);
      prsr_instance_hdl = (uint32_t)handle;
      err |= bf_cjson_get_string(prsr_instance_cjson,
                                 CTX_JSON_PARSER_INSTANCE_NAME,
                                 &prsr_instance_name);
      err |= bf_cjson_get_object(prsr_instance_cjson,
                                 CTX_JSON_PARSER_INSTANCE_STATES,
                                 &prsr_instance_states);
      err |= bf_cjson_get_object(prsr_instance_cjson,
                                 CTX_JSON_PARSER_INSTANCE_DEFAULT,
                                 &prsr_instance_default);
      CHECK_ERR(err, rc, cleanup);
    } else {
      prsr_instance_hdl = DEFAULT_PRSR_INSTANCE_HDL;  // default handler
      /* if it's single prsr program case, the prsr instance states are directly
       * taken from gress_cjson */
      prsr_instance_states = gress_cjson;
      prsr_instance_name = "$DEFAULT_PARSER_PROGRAM";
    }

    // Iterate over all PVS
    pvs_numb = 0;
    rc = ctx_json_parse_parser_pvs(dev_id,
                                   prof_id,
                                   gress,
                                   prsr_instance_hdl,
                                   prsr_instance_states,
                                   pvs_hdl,
                                   &pvs_numb);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to parse parser pvs tables, prsr_instance_handle 0x%x, "
          "dev %d",
          __func__,
          __LINE__,
          prsr_instance_hdl,
          dev_id);
      goto cleanup;
    }
    // if virtual_device, no prsr instance shadow
    if (!virtual_device) {
      // Get prsr_map
      uint64_t prsr_map = 0;
      rc = ctx_json_parse_parser_default_profile(
          dev_info, prsr_instance_default, &prsr_map);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Unable to parse parser default profile, "
            "prsr_instance_handle "
            "0x%x, dev %d, gress %s",
            __func__,
            __LINE__,
            prsr_instance_hdl,
            dev_id,
            ((gress == 0) ? "ingress" : "egress"));
        goto cleanup;
      }

      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        // new instance
        instance = PIPE_MGR_CALLOC(1, sizeof(struct pipe_mgr_prsr_instance_t));
        if (instance == NULL) {
          rc = PIPE_NO_SYS_RESOURCES;
          goto cleanup;
        }
        map_sts = bf_map_add(&PIPE_PRSR_DATA(dev_id, log_pipe_id, gress),
                             (unsigned long)prsr_instance_hdl,
                             (void *)instance);
        if (map_sts == BF_MAP_KEY_EXISTS) {
          LOG_ERROR("%s:%d instance hdl 0x%x already exists, dev :%d",
                    __func__,
                    __LINE__,
                    prsr_instance_hdl,
                    dev_id);
          rc = PIPE_UNEXPECTED;
          goto cleanup;
        }
        if (map_sts != BF_MAP_OK) {
          LOG_ERROR(
              "%s:%d Unable to add prsr instance node into prsr_db for dev "
              ":%d, "
              "prsr_instance_hdl :0x%x",
              __func__,
              __LINE__,
              dev_id,
              prsr_instance_hdl);
          rc = PIPE_UNEXPECTED;
          goto cleanup;
        }
        // Init instance shadow
        rc = pipe_mgr_parser_instance_init(
            prsr_instance_hdl,
            prsr_instance_name,
            instance,
            pvs_hdl,
            pvs_numb,
            prsr_map,
            0);  // phase0_hdl would be from tables
        if (rc != PIPE_SUCCESS) {
          goto cleanup;
        }
      }
    }
    if (!multi_parser_programs) {
      return rc;
    }
  }
  return rc;

cleanup:
  ctx_json_parse_parser_free_global_pvs(dev_id, prof_id);
  if (instance != NULL) PIPE_MGR_FREE(instance);
  return rc;
}

pipe_status_t ctx_json_parse_parser(bf_dev_id_t devid,
                                    profile_id_t prof_id,
                                    cJSON *root,
                                    bool virtual_device) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  /* Do not use profile-id as the mapping of parsers to profiles is not clear */
  bool multi_parser_programs = true;
  // Parse ContextJSON parser node appropriately.
  cJSON *parser_cjson = NULL;
  err = bf_cjson_try_get_object(root, CTX_JSON_PARSERS_NODE, &parser_cjson);
  if (parser_cjson == NULL) {
    // single parser program
    err = bf_cjson_get_object(root, CTX_JSON_PARSER_NODE, &parser_cjson);
    CHECK_ERR(err, rc, cleanup);
    multi_parser_programs = false;
  }

  cJSON *ingress_cjson = NULL;
  cJSON *egress_cjson = NULL;
  err = bf_cjson_get_object(
      parser_cjson, CTX_JSON_PARSER_INGRESS, &ingress_cjson);
  err |=
      bf_cjson_get_object(parser_cjson, CTX_JSON_PARSER_EGRESS, &egress_cjson);
  CHECK_ERR(err, rc, cleanup);

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("%s:%d: Invalid device id found: %d.", __func__, __LINE__, devid);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Parse ingress and egress.
  rc = ctx_json_parse_parser_gress(devid,
                                   prof_id,
                                   ingress_cjson,
                                   BF_DEV_DIR_INGRESS,
                                   multi_parser_programs,
                                   virtual_device);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d: Error while parsing the ingress PVS instances for dev %d : %s "
        "(%d)",
        __func__,
        __LINE__,
        devid,
        pipe_str_err(rc),
        rc);
    goto cleanup;
  }

  rc = ctx_json_parse_parser_gress(devid,
                                   prof_id,
                                   egress_cjson,
                                   BF_DEV_DIR_EGRESS,
                                   multi_parser_programs,
                                   virtual_device);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d: Error while parsing the egress PVS instances for dev %d : %s "
        "(%d)",
        __func__,
        __LINE__,
        devid,
        pipe_str_err(rc),
        rc);
    goto cleanup;
  }

  return rc;

cleanup:
  return rc;
}
