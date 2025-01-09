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


#include "switch_config.h"
#include <target-utils/third-party/cJSON/cJSON.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <regex.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int get_int(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  assert(NULL != value);
  assert(cJSON_Number == value->type);
  return value->valueint;
}

const char *get_string(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  assert(NULL != value);
  assert(cJSON_String == value->type);
  return value->valuestring;
}

uint32_t check_and_get_int(cJSON *item, const char *key, int default_value) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (NULL != value)
    return get_int(item, key);
  else
    return default_value;
}

const char *check_and_get_string(cJSON *item, const char *key) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (NULL != value)
    return get_string(item, key);
  else
    return "";
}

bool check_and_get_bool(cJSON *item, const char *key, bool default_value) {
  cJSON *value = cJSON_GetObjectItem(item, key);
  if (!value) return default_value;
  assert(value->type == cJSON_False || value->type == cJSON_True);
  return (value->type == cJSON_True);
}

/**
 * @brief  This function copies details of each program in a bf_device_profile_t
 *  to a p4_devices_t.
 *
 * @param[out] p4_device
 * @param[in] device_profile
 * @param[in] install_dir base path in case of non-absolute path
 * @param[in] absolute_paths
 */
void switch_p4_pipeline_config_each_program_update(
    p4_devices_t *p4_device,
    bf_device_profile_t *device_profile,
    const char *install_dir,
    bool absolute_paths) {
  if (p4_device == NULL) {
    return;
  }
  p4_device->num_p4_programs = device_profile->num_p4_programs;
  for (uint8_t i = 0; i < device_profile->num_p4_programs; i++) {
    p4_programs_t *p4_program = &(p4_device->p4_programs[i]);
    bf_p4_program_t *bf_p4_program = &(device_profile->p4_programs[i]);

    // Reset
    if (p4_program->program_name) {
      free(p4_program->program_name);
    }
    p4_program->program_name = strndup(bf_p4_program->prog_name, PROG_NAME_LEN);

    if (bf_p4_program->bfrt_json_file) {
      if (absolute_paths) {
        snprintf(p4_program->bfrt_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_program->bfrt_json_file);
      } else {
        snprintf(p4_program->bfrt_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_program->bfrt_json_file);
      }
    } else {
      // src is NULL. Make dst NULL too.
      p4_program->bfrt_config[0] = 0;
    }
    p4_program->num_p4_pipelines = bf_p4_program->num_p4_pipelines;
    switch_p4_pipeline_config_each_profile_update(
        p4_program, bf_p4_program, install_dir, absolute_paths);
  }
  return;
}

/**
 * @brief Copies into a bf_p4_program struct from a p4_programs
 * struct
 *
 * @param[out] p4_program
 * @param[in] bf_p4_program
 * @param[in] install_dir base path in case of non-absolute path
 * @param[in] absolute_paths
 */
void switch_p4_pipeline_config_each_profile_update(
    p4_programs_t *p4_program,
    bf_p4_program_t *bf_p4_program,
    const char *install_dir,
    bool absolute_paths) {
  for (int j = 0; j < p4_program->num_p4_pipelines; j++) {
    p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[j]);
    bf_p4_pipeline_t *bf_p4_pipeline = &(bf_p4_program->p4_pipelines[j]);
    // Reset
    if (p4_pipeline->p4_pipeline_name) {
      free(p4_pipeline->p4_pipeline_name);
      p4_pipeline->p4_pipeline_name = NULL;
    }
    p4_pipeline->tofino_bin[0] = 0;
    p4_pipeline->table_config[0] = 0;
    p4_pipeline->pi_native_config_path[0] = 0;

    // Set
    if (bf_p4_pipeline->p4_pipeline_name[0] != '\0') {
      p4_pipeline->p4_pipeline_name =
          strndup(bf_p4_pipeline->p4_pipeline_name, PROG_NAME_LEN);
    }
    if (bf_p4_pipeline->cfg_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->tofino_bin,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->cfg_file);
      } else {
        snprintf(p4_pipeline->tofino_bin,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->cfg_file);
      }
    }
    if (bf_p4_pipeline->runtime_context_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->table_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->runtime_context_file);
      } else {
        snprintf(p4_pipeline->table_config,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->runtime_context_file);
      }
    }
    if (bf_p4_pipeline->pi_config_file) {
      if (absolute_paths) {
        snprintf(p4_pipeline->pi_native_config_path,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s",
                 bf_p4_pipeline->pi_config_file);
      } else {
        snprintf(p4_pipeline->pi_native_config_path,
                 BF_SWITCHD_MAX_FILE_NAME,
                 "%s/%s",
                 install_dir,
                 bf_p4_pipeline->pi_config_file);
      }
    }
    p4_pipeline->num_pipes_in_scope = bf_p4_pipeline->num_pipes_in_scope;
    for (uint8_t k = 0; k < p4_pipeline->num_pipes_in_scope; k++) {
      p4_pipeline->pipe_scope[k] = bf_p4_pipeline->pipe_scope[k];
    }
  }
}

/* Check if the required path is absolute, and if not
 * prepend the install directory */
static void to_abs_path(char *dest,
                        cJSON *p4_program_obj,
                        const char *key,
                        const char *install_dir) {
  if (strlen(check_and_get_string(p4_program_obj, key))) {
    const char *path = check_and_get_string(p4_program_obj, key);
    if (path[0] != '/')  // absolute path -- don't prepend install_dir
      sprintf(dest, "%s/%s", install_dir, path);
    else
      sprintf(dest, "%s", path);
  }
}

static void switch_p4_pipeline_config_init(
    const char *install_dir,
    cJSON *switch_config_obj,
    bf_switchd_internal_context_t *self) {
  bf_sys_log_and_trace(
      BF_MOD_SWITCHD, BF_LOG_DBG, "bf_switchd: processing P4 configuration...");

  int agent_idx = 0, count = 0;

  /* Use new format of json file if possible */
  cJSON *p4_device_arr = cJSON_GetObjectItem(switch_config_obj, "p4_devices");
  if (p4_device_arr != NULL) {
    int j = 0, k = 0, s = 0;
    /* New Format */
    /* Go over all devices */
    for (int i = 0; i < cJSON_GetArraySize(p4_device_arr); ++i) {
      cJSON *p4_device_obj = cJSON_GetArrayItem(p4_device_arr, i);

      const bf_dev_id_t device = get_int(p4_device_obj, "device-id");
      p4_devices_t *p4_device = &(self->p4_devices[device]);
      cJSON *p4_program_arr = cJSON_GetObjectItem(p4_device_obj, "p4_programs");
      assert(p4_program_arr != NULL);
      assert(cJSON_Array == p4_program_arr->type);

      p4_device->num_p4_programs = cJSON_GetArraySize(p4_program_arr);
      /* Go over all programs */
      for (j = 0; j < cJSON_GetArraySize(p4_program_arr); ++j) {
        cJSON *p4_program_obj = cJSON_GetArrayItem(p4_program_arr, j);
        p4_programs_t *p4_program = &(p4_device->p4_programs[j]);
        p4_program->program_name =
            strdup(get_string(p4_program_obj, "program-name"));

        to_abs_path(
            p4_program->switchapi, p4_program_obj, "switchapi", install_dir);
        to_abs_path(p4_program->switchsai, p4_program_obj, "sai", install_dir);
        to_abs_path(p4_program->diag, p4_program_obj, "diag", install_dir);
        to_abs_path(p4_program->accton_diag,
                    p4_program_obj,
                    "accton_diag",
                    install_dir);
        p4_program->add_ports_to_switchapi =
            check_and_get_bool(p4_program_obj, "switchapi_port_add", true);
        // CPU port name (if using ethernet CPU port)
        if (strlen(check_and_get_string(p4_program_obj, "cpu_port"))) {
          p4_program->use_eth_cpu_port = true;
          sprintf(p4_program->eth_cpu_port_name,
                  "%s",
                  check_and_get_string(p4_program_obj, "cpu_port"));
        } else {
          p4_program->use_eth_cpu_port = false;
        }
        // BFRT config
        to_abs_path(p4_program->bfrt_config,
                    p4_program_obj,
                    "bfrt-config",
                    install_dir);

        to_abs_path(self->board_port_map_conf_file,
                    p4_program_obj,
                    "board-port-map",
                    install_dir);

        cJSON *p4_pipeline_arr =
            cJSON_GetObjectItem(p4_program_obj, "p4_pipelines");
        assert(p4_pipeline_arr != NULL);

        p4_program->num_p4_pipelines = cJSON_GetArraySize(p4_pipeline_arr);
        if (p4_program->num_p4_pipelines > BF_SWITCHD_MAX_P4_PIPELINES) {
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_ERR,
                               "Too many P4 control flows %d ",
                               p4_program->num_p4_pipelines);
          assert(0);
        }
        /* Go over all p4_pipelines */
        for (k = 0; k < cJSON_GetArraySize(p4_pipeline_arr); ++k) {
          cJSON *p4_pipeline_obj = cJSON_GetArrayItem(p4_pipeline_arr, k);
          p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[k]);

          p4_pipeline->p4_pipeline_name =
              strdup(get_string(p4_pipeline_obj, "p4_pipeline_name"));
          to_abs_path(p4_pipeline->pd, p4_pipeline_obj, "pd", install_dir);
          to_abs_path(p4_pipeline->pd_thrift,
                      p4_pipeline_obj,
                      "pd-thrift",
                      install_dir);
          to_abs_path(p4_pipeline->table_config,
                      p4_pipeline_obj,
                      "context",
                      install_dir);
          to_abs_path(
              p4_pipeline->tofino_bin, p4_pipeline_obj, "config", install_dir);

          /* Get the pipe scope */
          cJSON *pipe_scope_arr =
              cJSON_GetObjectItem(p4_pipeline_obj, "pipe_scope");
          /*
           * pipe_scope_arr == Null is only valid for 1-Program, 1-Pipeline
           * case where we can go and set everything to default. Assert
           * if that's not true
           */
          if (pipe_scope_arr == NULL) {
            assert(cJSON_GetArraySize(p4_program_arr) <= 1);
            assert(cJSON_GetArraySize(p4_pipeline_arr) <= 1);
            /* Set num_pipes to 0 so that it can be populated later
             * with the correct values from the info queried from lld
             */
            p4_pipeline->num_pipes_in_scope = 0;
          } else {
            p4_pipeline->num_pipes_in_scope =
                cJSON_GetArraySize(pipe_scope_arr);
          }
          if (p4_pipeline->num_pipes_in_scope > BF_SWITCHD_MAX_PIPES) {
            p4_pipeline->num_pipes_in_scope = BF_SWITCHD_MAX_PIPES;
          }
          for (s = 0; s < p4_pipeline->num_pipes_in_scope; ++s) {
            cJSON *pipe_scope_obj = cJSON_GetArrayItem(pipe_scope_arr, s);
            p4_pipeline->pipe_scope[s] = pipe_scope_obj->valueint;
          }
        }
      }

      // coalescing mirror configuration from <p4-name>.conf
      p4_device->coal_mirror_enable =
          check_and_get_int(p4_device_obj, "coal_mirror_enable", 0);
      p4_device->coal_sessions_num =
          check_and_get_int(p4_device_obj, "coal_sessions_num", 0);
      p4_device->coal_min = check_and_get_int(p4_device_obj, "coal_min", 0);
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_DBG,
                           "coal_mirror_enable=%d coal_min=%d sessions_num=%d",
                           p4_device->coal_mirror_enable,
                           p4_device->coal_min,
                           p4_device->coal_sessions_num);

      /* Agents are specific to device */
      char *agent_str = "agent";
      for (agent_idx = 0; agent_idx < BF_SWITCHD_MAX_AGENTS; agent_idx++) {
        char agent_idx_str[50];
        snprintf(agent_idx_str, 50, "%s%d", agent_str, agent_idx);
        to_abs_path(p4_device->agent[agent_idx],
                    p4_device_obj,
                    agent_idx_str,
                    install_dir);
      }

      /* Print config */
      bf_sys_log_and_trace(
          BF_MOD_SWITCHD, BF_LOG_DBG, "P4 profile for dev_id %d", device);
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_DBG,
                           "num P4 programs %d",
                           p4_device->num_p4_programs);
      for (j = 0; j < p4_device->num_p4_programs; ++j) {
        p4_programs_t *p4_program = &(p4_device->p4_programs[j]);
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_DBG,
                             "  p4_name: %s",
                             p4_program->program_name);
        for (k = 0; k < p4_program->num_p4_pipelines; k++) {
          p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[k]);
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "  p4_pipeline_name: %s",
                               p4_pipeline->p4_pipeline_name);

          bf_sys_log_and_trace(
              BF_MOD_SWITCHD, BF_LOG_DBG, "    libpd: %s", p4_pipeline->pd);
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "    libpdthrift: %s",
                               p4_pipeline->pd_thrift);
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "    context: %s",
                               p4_pipeline->table_config);
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "    config: %s",
                               p4_pipeline->tofino_bin);
          if (p4_pipeline->num_pipes_in_scope > 0) {
            bf_sys_log_and_trace(
                BF_MOD_SWITCHD, BF_LOG_DBG, "  Pipes in scope [");
            for (s = 0; s < p4_pipeline->num_pipes_in_scope; ++s) {
              bf_sys_log_and_trace(BF_MOD_SWITCHD,
                                   BF_LOG_DBG,
                                   "%d ",
                                   p4_pipeline->pipe_scope[s]);
            }
            bf_sys_log_and_trace(BF_MOD_SWITCHD, BF_LOG_DBG, "]");
          }
          bf_sys_log_and_trace(
              BF_MOD_SWITCHD, BF_LOG_DBG, "  diag: %s", p4_program->diag);
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "  accton diag: %s",
                               p4_program->accton_diag);
          if (strlen(self->board_port_map_conf_file)) {
            bf_sys_log_and_trace(BF_MOD_SWITCHD,
                                 BF_LOG_DBG,
                                 "  board-port-map: %s",
                                 self->board_port_map_conf_file);
          }
        }
      }
      for (agent_idx = 0; agent_idx < BF_SWITCHD_MAX_AGENTS; agent_idx++) {
        if (strlen(p4_device->agent[agent_idx])) {
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "  Agent[%d]: %s",
                               agent_idx,
                               p4_device->agent[agent_idx]);
        }
      }
    }
  } else {
    /* Old Format */
    cJSON *p4_program_arr =
        cJSON_GetObjectItem(switch_config_obj, "p4_program_list");
    assert(NULL != p4_program_arr);
    assert(cJSON_Array == p4_program_arr->type);

    for (int i = 0; i < cJSON_GetArraySize(p4_program_arr); ++i) {
      cJSON *p4_program_obj = cJSON_GetArrayItem(p4_program_arr, i);
      const int instance = get_int(p4_program_obj, "instance");
      p4_devices_t *p4_device = &(self->p4_devices[instance]);

      /* Device specific items */
      /* Agents are specific to device */
      char *agent_str = "agent";
      for (agent_idx = 0; agent_idx < BF_SWITCHD_MAX_AGENTS; agent_idx++) {
        char agent_idx_str[50];
        snprintf(agent_idx_str, 50, "%s%d", agent_str, agent_idx);
        to_abs_path(p4_device->agent[agent_idx],
                    p4_program_obj,
                    agent_idx_str,
                    install_dir);
      }

      /* only 1 program per device in old json format */
      p4_device->num_p4_programs = 1;
      p4_programs_t *p4_program = &(p4_device->p4_programs[0]);

      /* Program specific items */
      p4_program->program_name =
          strdup(get_string(p4_program_obj, "program-name"));
      to_abs_path(
          p4_program->switchapi, p4_program_obj, "switchapi", install_dir);
      to_abs_path(p4_program->switchsai, p4_program_obj, "sai", install_dir);
      to_abs_path(p4_program->diag, p4_program_obj, "diag", install_dir);
      to_abs_path(
          p4_program->accton_diag, p4_program_obj, "accton_diag", install_dir);
      p4_program->add_ports_to_switchapi =
          check_and_get_bool(p4_program_obj, "switchapi_port_add", true);
      // CPU port name (if using ethernet CPU port)
      if (strlen(check_and_get_string(p4_program_obj, "cpu_port"))) {
        p4_program->use_eth_cpu_port = true;
        sprintf(p4_program->eth_cpu_port_name,
                "%s",
                check_and_get_string(p4_program_obj, "cpu_port"));
      } else {
        p4_program->use_eth_cpu_port = false;
      }
      // BFRT config
      to_abs_path(
          p4_program->bfrt_config, p4_program_obj, "bfrt-config", install_dir);

      to_abs_path(self->board_port_map_conf_file,
                  p4_program_obj,
                  "board-port-map",
                  install_dir);

      /* only 1 profile per program in old json format */
      p4_program->num_p4_pipelines = 1;

      /* p4_pipeline specific items */
      p4_pipeline_config_t *p4_pipeline = &(p4_program->p4_pipelines[0]);
      p4_pipeline->num_pipes_in_scope = 0;
      p4_pipeline->p4_pipeline_name =
          strdup(get_string(p4_program_obj, "program-name"));
      to_abs_path(p4_pipeline->pd, p4_program_obj, "pd", install_dir);
      to_abs_path(
          p4_pipeline->pd_thrift, p4_program_obj, "pd-thrift", install_dir);
      to_abs_path(p4_pipeline->table_config,
                  p4_program_obj,
                  "table-config",
                  install_dir);
      to_abs_path(
          p4_pipeline->tofino_bin, p4_program_obj, "tofino-bin", install_dir);

      /* Print config */
      bf_sys_log_and_trace(
          BF_MOD_SWITCHD, BF_LOG_DBG, "P4 profile for dev_id %d", instance);
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_DBG,
                           "  p4_name: %s",
                           p4_program->program_name);
      for (count = 0; count < p4_program->num_p4_pipelines; count++) {
        p4_pipeline = &(p4_program->p4_pipelines[count]);

        bf_sys_log_and_trace(
            BF_MOD_SWITCHD, BF_LOG_DBG, "    libpd: %s", p4_pipeline->pd);
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_DBG,
                             "    libpdthrift: %s",
                             p4_pipeline->pd_thrift);
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_DBG,
                             "    context: %s",
                             p4_pipeline->table_config);
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_DBG,
                             "    config: %s",
                             p4_pipeline->tofino_bin);
        for (agent_idx = 0; agent_idx < BF_SWITCHD_MAX_AGENTS; agent_idx++) {
          if (strlen(p4_device->agent[agent_idx])) {
            bf_sys_log_and_trace(BF_MOD_SWITCHD,
                                 BF_LOG_DBG,
                                 "  Agent[%d]: %s",
                                 agent_idx,
                                 p4_device->agent[agent_idx]);
          }
        }
        bf_sys_log_and_trace(
            BF_MOD_SWITCHD, BF_LOG_DBG, "  diag: %s", p4_program->diag);
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_DBG,
                             "  accton diag: %s",
                             p4_program->accton_diag);
        if (strlen(self->board_port_map_conf_file)) {
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "  board-port-map: %s",
                               self->board_port_map_conf_file);
        }
      }
    }  // for all programs
  }    // if p4_devices
}

static int switch_parse_chip_list(cJSON *root,
                                  bf_switchd_internal_context_t *ctx) {
  /* Get the chip_list node */
  cJSON *chip_list = cJSON_GetObjectItem(root, "chip_list");
  if (!chip_list || chip_list->type == cJSON_NULL) {
    bf_sys_log_and_trace(
        BF_MOD_SWITCHD, BF_LOG_DBG, "Cannot find chip_list in conf file");
    return -1;
  }

  /* For each chip in the chip_list... */
  for (cJSON *chip = chip_list->child; chip; chip = chip->next) {
    /* Get the asic id. */
    bf_dev_id_t chip_id = get_int(chip, "instance");

    /* Get the virtual and slave flags. */
    ctx->asic[chip_id].is_virtual = check_and_get_int(chip, "virtual", 0);

    /* Get chip family */
    const char *fam = check_and_get_string(chip, "chip_family");
    if (!strlen(fam)) {
      /* The family wasn't specified, default to Tofino. */
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_TOFINO;
      fam = "Tofino";
    } else if (!strcasecmp(fam, "tofino")) {
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_TOFINO;
    } else if (!strcasecmp(fam, "tofino2")) {
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_TOFINO2;
    } else if (!strcasecmp(fam, "tofino3")) {
      ctx->asic[chip_id].chip_family = BF_DEV_FAMILY_TOFINO3;


    } else {
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_ERR,
                           "Unexpected chip family \"%s\" for instance %d",
                           fam,
                           chip_id);
      return -1;
    }

    /* Get the PCIe fields. */
    switchd_pcie_cfg_t *pcie_cfg = &ctx->asic[chip_id].pcie_cfg;
    const char *pcie_sysfs_prefix =
        check_and_get_string(chip, "pcie_sysfs_prefix");
    sprintf(pcie_cfg->pci_sysfs_str, "%s", pcie_sysfs_prefix);

    switch (ctx->asic[chip_id].chip_family) {

      case BF_DEV_FAMILY_TOFINO3: {
        int subdev_id;
        /* Get PCIe info for subdevices */
        /* default to a single subdevice case if no configuration found */
        switchd_pcie_map_t *pcie_map = &ctx->pcie_map[chip_id][0];
        pcie_map->configured = true;
        snprintf(pcie_map->cdev_name,
                 sizeof(pcie_map->cdev_name),
                 "%s",
                 "/dev/bf0s0");
        for (subdev_id = 1; subdev_id < BF_MAX_SUBDEV_COUNT; subdev_id++) {
          pcie_map = &ctx->pcie_map[chip_id][subdev_id];
          pcie_map->cdev_name[0] = '\0';
          pcie_map->configured = false;
        }
        /* Get the chip_list node */
        cJSON *subdev_list = cJSON_GetObjectItem(chip, "subdev_list");
        if (!subdev_list || subdev_list->type == cJSON_NULL) {
          bf_sys_log_and_trace(
              BF_MOD_SWITCHD,
              BF_LOG_ERR,
              "Cannot find subdev_list in conf file defaulting to a "
              "single subdevice");
        } else {
          for (cJSON *subdev = subdev_list->child; subdev;
               subdev = subdev->next) {
            subdev_id = check_and_get_int(subdev, "subdev_instance", 0);
            if (subdev_id >= BF_MAX_SUBDEV_COUNT) {
              bf_sys_log_and_trace(BF_MOD_SWITCHD,
                                   BF_LOG_ERR,
                                   "cfg error, subdevice id %d out of range",
                                   subdev_id);
              break;
            }
            pcie_map = &ctx->pcie_map[chip_id][subdev_id];
            pcie_map->configured = true;
            snprintf(pcie_map->cdev_name,
                     sizeof(pcie_map->cdev_name),
                     "%s",
                     check_and_get_string(subdev, "cdev_name"));
          }
        }
        break;
      }
      default:
        ctx->pcie_map[chip_id][0].configured = true;
        break;
    }

    /* Parse the serdes firmware path (sds_fw_path) from the conf file. */
    switchd_serdes_cfg_t *sd_cfg = &ctx->asic[chip_id].serdes_cfg;
    sd_cfg->sds_fw_path = NULL;
    const char *tmp_str;
    tmp_str = check_and_get_string(chip, "sds_fw_path");
    if (strlen(tmp_str)) {
      sd_cfg->sds_fw_path = bf_sys_strdup(tmp_str);
    }

    ctx->asic[chip_id].configured = true;

    /* Display the parsed data. */
    bf_sys_log_and_trace(
        BF_MOD_SWITCHD, BF_LOG_DBG, "Configuration for dev_id %d", chip_id);
    bf_sys_log_and_trace(
        BF_MOD_SWITCHD, BF_LOG_DBG, "  Family        : %s", fam);
    if (ctx->asic[chip_id].is_virtual)
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_DBG,
                           "  Virtual   : %d",
                           ctx->asic[chip_id].is_virtual);
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_DBG,
                         "  pci_sysfs_str : %s",
                         ctx->asic[chip_id].pcie_cfg.pci_sysfs_str);
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_DBG,
                         "  pci_int_mode  : %d",
                         ctx->asic[chip_id].pcie_cfg.int_mode);
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_DBG,
                         "  sds_fw_path   : %s",
                         ctx->asic[chip_id].serdes_cfg.sds_fw_path);
    switch (ctx->asic[chip_id].chip_family) {

      case BF_DEV_FAMILY_TOFINO3:
        for (int i = 0; i < BF_MAX_SUBDEV_COUNT; i++) {
          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_DBG,
                               "  subdevice%d     : %s",
                               i,
                               ctx->pcie_map[chip_id][i].cdev_name);
        }
        break;
      default:
        // No need to list subdevs if not supported
        break;
    }
  }
  return 0;
}

/* Count the number of occurences of a "character" in the "string_to_parse" */
static int count_chars(const char *string_to_parse, char character) {
  int count = 0;

  for (int i = 0; string_to_parse[i]; i++) {
    if (string_to_parse[i] == character) count++;
  }

  return count;
}

/* Get the certain "line_no" line in a multiline "string_to_parse" */
static char *get_line(char *string_to_parse, int line_no) {
  char *save, *ret;
  int count = 0;

  do {
    ret = strtok_r(string_to_parse, (const char *)"\n", &save);
    if (ret) {
      count++;
      if (count == line_no) return ret;
      string_to_parse = NULL;
    }
  } while (ret);

  return NULL;
}

/* Helper function to parse the conf file.  The conf file is JSON format and
 * contains two sections used by bf_switchd.  It may contain additional
 * information if an application or library wants to do so, for example the
 * bf-switch library includes a "switch_config" section.  The first section used
 * by bf_switchd is the "chip_list", is a list of chip objects representing the
 * devices to use and contains the following fields:
 *   "instance": int, identifying the chip, will be used as the bf_dev_id_t.
 *   "virtual": optional int, used as a boolean indicating if this device is a
 *                            virtual device.
 *   "chip_family": optional string, identifying the type of asic.  Expected
 *                                   values are "tofino", "tofino2", or
 *                                   "tofino3".  If not specified Tofino-1 is
 *                                   assumed.
 *   "pcie_sysfs_prefix": optional string, identifies the sysfs path to the
 *                                         device.  For single device cases this
 *                                         is not required and the sysfs path to
 *                                         the first (or only) device in the
 *                                         system is used.
 *                                         For multi-device systems this is
 *                                         required.
 *   "sds_fw_path": optional string, identifies the path to the directory
 *                                   containing serdes firmware files.  If not
 *                                   provided the standard install path is used.
 * The second section is a "p4_devices" list which describes the P4 and agent
 * library configuration to be applied to the device.  There should be one
 * p4_devices entry for each asic and it may contain the following commonly used
 * fields:
 *   "device-id": int, identifying the chip, will be used as the bf_dev_id_t.
 *   "p4_programs": array, describes the P4 programs to apply to the device.
 *     "program-name": string, name of the P4 program.
 *     "bfrt-config": string, path to the bf-rt.json file for the program.
 *     "p4_pipelines": array, describes the p4-pipelines of the P4 program.
 *       "p4_pipeline_name": string, the name of the pipeline.
 *       "context": string, the path to the context.json file for the pipeline.
 *       "config": string, the path to the pipeline's bin file.
 *       "pipe_scope": integer list, describes the logical pipelines of the chip
 *                                   which will be programmed with this P4
 *                                   pipeline.
 */
int switch_dev_config_init(const char *install_dir,
                           const char *config_filename,
                           bf_switchd_internal_context_t *self) {
  int ret = 0;
  FILE *file;

  if (!config_filename) {
    return -EINVAL;
  }

  file = fopen(config_filename, "r");
  if (file) {
    int fd = fileno(file);
    struct stat stat_b;
    fstat(fd, &stat_b);
    size_t to_alloc = stat_b.st_size + 1;
    char *config_file = malloc(to_alloc);
    if (config_file) {
      config_file[to_alloc - 1] = 0;
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_DBG,
                           "bf_switchd: processing device configuration...");
      int i = fread(config_file, stat_b.st_size, 1, file);
      if (i != 1) {
        bf_sys_log_and_trace(BF_MOD_SWITCHD,
                             BF_LOG_ERR,
                             "Error reading conf file %s",
                             config_file);
        ret = -EINVAL;
      } else {
        const char *ptr = NULL;

        /* Parse the config file, print wrong line if it fails */
        cJSON *swch = cJSON_ParseWithOpts(config_file, &ptr, true);
        if (swch) {
          switch_parse_chip_list(swch, self);
          switch_p4_pipeline_config_init(install_dir, swch, self);
        } else {
          int lines_in_cfg = count_chars(config_file, '\n');
          int lines_after_err = count_chars(ptr, '\n');
          int at_line = lines_in_cfg - lines_after_err + 1;
          char *line = get_line(config_file, at_line);

          bf_sys_log_and_trace(BF_MOD_SWITCHD,
                               BF_LOG_ERR,
                               "Error in config file at line %d:\n[%s]",
                               at_line,
                               (line) ? line : "NONE");
          ret = -EINVAL;
        }
        cJSON_Delete(swch);
      }
      free(config_file);
    } else {
      bf_sys_log_and_trace(BF_MOD_SWITCHD,
                           BF_LOG_ERR,
                           "Could not allocate %zd bytes to load conf file",
                           to_alloc);
      ret = -ENOMEM;
    }
    fclose(file);
  } else {
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_ERR,
                         "Could not open conf file %s",
                         config_filename);
    ret = -errno;
  }

  return ret;
}

/* return the sysfs file name of the first Tofino ASIC found */
int switch_pci_sysfs_str_get(char *name,
                             size_t name_size,
                             bf_dev_family_t chip_family) {
  if (chip_family == BF_DEV_FAMILY_TOFINO3) {
    snprintf(name, name_size, "/sys/class/bf/bf0s0/device");
  } else {
    snprintf(name, name_size, "/sys/class/bf/bf0/device");
  }
  return 0;
}

/* determine if the linux platform is iommu enabled */
bool switch_is_iommu_enabled(void) {
  struct stat statbuf;
  if (stat("/sys/devices/virtual/iommu", &statbuf)) {
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_DBG,
                         "detecting.. IOMMU not enabled on the platform");
    return false;
  } else {
    bf_sys_log_and_trace(BF_MOD_SWITCHD,
                         BF_LOG_DBG,
                         "detecting.. IOMMU is enabled on the platform");
    return true;
  }
}
