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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifdef __cplusplus
}
#endif

// tdi includes
//#include <tdi/common/tdi_info.hpp>

// target include
#include <tdi_tofino/tdi_tofino_init.hpp>
#include <tdi_tofino/c_frontend/tdi_tofino_warm_init.h>

typedef struct program_config_ {
  std::string prog_name;
  std::string base_path;
  std::string tdi_info_file_path;
  std::vector<tdi::P4Pipeline> p4_pipelines;
} program_config_t;

const std::map<const std::string, bf_dev_init_mode_t> str_to_init_mode_map{
    {"INIT_COLD", BF_DEV_INIT_COLD},
    {"WARM_INIT_FAST_RECFG", BF_DEV_WARM_INIT_FAST_RECFG},
    {"WARM_INIT_HITLESS", BF_DEV_WARM_INIT_HITLESS}};

const std::map<const std::string, bf_dev_serdes_upgrade_mode_t>
    str_to_serdes_mode_map{
        {"SERDES_UPD_NONE", BF_DEV_SERDES_UPD_NONE},
        {"SERDES_UPD_FORCED_PORT_RECFG", BF_DEV_SERDES_UPD_FORCED_PORT_RECFG},
        {"SERDES_UPD_DEFERRED_PORT_RECFG",
         BF_DEV_SERDES_UPD_DEFERRED_PORT_RECFG}};

bool file_exists(const std::string &path) { return std::ifstream(path).good(); }

tdi_status_t dev_config_allocate(int num_programs,
                                 tdi_dev_config_hdl **dev_config_hdl) {
  std::vector<program_config_t> *dev_config =
      new std::vector<program_config_t>(num_programs);
  if (dev_config == nullptr) {
    LOG_ERROR("%s:%d Failed to allocate memory", __func__, __LINE__);
    return TDI_NO_SYS_RESOURCES;
  }
  *dev_config_hdl = reinterpret_cast<tdi_dev_config_hdl *>(dev_config);
  return TDI_SUCCESS;
}

tdi_status_t dev_config_deallocate(tdi_dev_config_hdl *dev_config_hdl) {
  auto dev_config =
      reinterpret_cast<std::vector<program_config_t> *>(dev_config_hdl);
  if (dev_config == nullptr) {
    LOG_ERROR("%s:%d null param passed", __func__, __LINE__);
    return TDI_INVALID_ARG;
  }
  delete dev_config;
  return TDI_SUCCESS;
}

tdi_status_t set_program_name(tdi_dev_config_hdl *dev_config_hdl,
                              int prog_index,
                              char *prog_name) {
  auto &dev_config =
      *reinterpret_cast<std::vector<program_config_t> *>(dev_config_hdl);
  dev_config[prog_index].prog_name = std::string(prog_name);
  return TDI_SUCCESS;
}

tdi_status_t set_tdi_info_path(tdi_dev_config_hdl *dev_config_hdl,
                               int prog_index,
                               char *tdi_info_path) {
  auto &dev_config =
      *reinterpret_cast<std::vector<program_config_t> *>(dev_config_hdl);
  dev_config[prog_index].tdi_info_file_path = std::string(tdi_info_path);
  return TDI_SUCCESS;
}

tdi_status_t set_pipeline(tdi_dev_config_hdl *dev_config_hdl,
                          int prog_index,
                          tdi_pipeline_t pipeline) {
  auto &dev_config =
      *reinterpret_cast<std::vector<program_config_t> *>(dev_config_hdl);
  auto &pipeline_vec = dev_config[prog_index].p4_pipelines;

  tdi::P4Pipeline pipeline_local(
      std::string(pipeline.name),
      std::string(pipeline.context_path),
      std::string(pipeline.binary_path),
      std::vector<uint32_t>(pipeline.scope_vec,
                            pipeline.scope_vec + pipeline.num_pipes));

  pipeline_vec.push_back(pipeline_local);
  return TDI_SUCCESS;
}

tdi_status_t device_warm_init_begin(tdi_dev_id_t dev_id,
                                    char *init_mode_str,
                                    char *serdes_upgrade_mode_str,
                                    bool upgrade_agents,
                                    tdi_dev_config_hdl *dev_config_hdl) {
  bf_dev_init_mode_t init_mode;
  bf_dev_serdes_upgrade_mode_t serdes_upgrade_mode;

  if (str_to_init_mode_map.find(std::string(init_mode_str)) !=
      str_to_init_mode_map.end()) {
    init_mode = str_to_init_mode_map.at(std::string(init_mode_str));
  } else {
    LOG_ERROR("%s:%d Invalid init_mode: %s", __func__, __LINE__, init_mode_str);
    return TDI_INVALID_ARG;
  }

  if (str_to_serdes_mode_map.find(std::string(serdes_upgrade_mode_str)) !=
      str_to_serdes_mode_map.end()) {
    serdes_upgrade_mode =
        str_to_serdes_mode_map.at(std::string(serdes_upgrade_mode_str));
  } else {
    LOG_ERROR("%s:%d Invalid serdes mode: %s",
              __func__,
              __LINE__,
              serdes_upgrade_mode_str);
    return TDI_INVALID_ARG;
  }

  std::vector<tdi::ProgramConfig> dev_config;
  if (dev_config_hdl != nullptr) {
    auto &dev_config_in =
        *reinterpret_cast<std::vector<program_config_t> *>(dev_config_hdl);

    for (const auto &prog_config : dev_config_in) {
      std::vector<std::string> tdi_info_file_paths{
          prog_config.tdi_info_file_path};
      dev_config.push_back(tdi::ProgramConfig(prog_config.prog_name,
                                              tdi_info_file_paths,
                                              prog_config.p4_pipelines));
    }
  } else {
    LOG_TRACE("%s:%d dev_config is empty", __func__, __LINE__);
  }

  tdi::tna::tofino::WarmInitOptions warm_init_options(
      init_mode, serdes_upgrade_mode, upgrade_agents, dev_config);

  return tdi::DevMgr::getInstance().deviceWarmInitBegin(dev_id,
                                                        warm_init_options);
}

tdi_status_t device_warm_init_end(tdi_dev_id_t dev_id) {
  return tdi::DevMgr::getInstance().deviceWarmInitEnd(dev_id);
}
