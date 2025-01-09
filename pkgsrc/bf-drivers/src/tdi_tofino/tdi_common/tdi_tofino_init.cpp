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
#include <dirent.h>

#ifdef __cplusplus
}
#endif
// tdi includes
#include <tdi/common/tdi_utils.hpp>

// target include
#include <tdi_tofino/c_frontend/tdi_tofino_init.h>
#include <tdi_tofino/tdi_tofino_init.hpp>

// local includes
#include "tdi_tofino_info.hpp"
#include "tdi_tofino_target.hpp"
#include "tdi_context_info.hpp"
#include "tdi_pipe_mgr_intf.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#include <tofino/bf_pal/dev_intf.h>
#ifdef __cplusplus
}
#endif

namespace tdi {
namespace tna {
namespace tofino {

class Device;

std::string Init::tdi_module_name = "tdi";
bf_drv_client_handle_t Init::tdi_drv_hdl;

namespace {
std::vector<std::string> tdiFixedJsonFilePathsGet(
    bf_dev_family_t dev_family, const char *non_p4_json_path) {
  std::vector<std::string> tdi_json_fixed_vec;
  // Old platform suffixes tof.json and tof2.json are denied in case these files
  // are still present in the same directory.
  std::vector<std::string> excluded_suffixes = {
      "tof.json", "tof2.json", "tof3.json"};

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      excluded_suffixes.push_back("tf2.json");
      excluded_suffixes.push_back("tf3.json");
      break;
    case BF_DEV_FAMILY_TOFINO2:
      excluded_suffixes.push_back("tf1.json");
      excluded_suffixes.push_back("tf3.json");
      break;
    case BF_DEV_FAMILY_TOFINO3:
      excluded_suffixes.push_back("tf1.json");
      excluded_suffixes.push_back("tf2.json");
      break;
    default:
      LOG_ERROR(
          "%s:%d Invalid device family %d", __func__, __LINE__, dev_family);
      return {};
  }

  // map of dev_id -> vector of all tdi.json files
  std::string tdi_fixed_json_path(non_p4_json_path);
  DIR *dir = opendir(tdi_fixed_json_path.c_str());
  if (dir != NULL) {
    std::string common_string("json");
    struct dirent *pdir;
    while ((pdir = readdir(dir)) != NULL) {
      std::string file_name(pdir->d_name);
      if (file_name.find(common_string) == std::string::npos) {
        continue;  // ignore non json files
      }
      bool is_excluded = false;
      for (auto &excluded_str : excluded_suffixes) {
        if (file_name.find(excluded_str) != std::string::npos) {
          // The json file name is not belonging to the current dev_family,
          // or it has an old name suffix.
          is_excluded = true;
          break;
        }
      }
      if (is_excluded) {
        continue;
      }
      // The json file is either with the current platform suffix,
      // or it has no explicit platform suffix.
      file_name.insert(0, tdi_fixed_json_path);
      tdi_json_fixed_vec.push_back(file_name);
    }
    int sts = closedir(dir);
    if (0 != sts) {
      LOG_ERROR("%s:%d Trying to close the tdi directory %s fail",
                __func__,
                __LINE__,
                tdi_fixed_json_path.c_str());
      return {};
    }
  }
  return tdi_json_fixed_vec;
}

tdi_status_t convertDeviceConfigToDeviceProfile(
    const std::vector<tdi::ProgramConfig> &device_config,
    bf_device_profile_t *device_profile) {
  device_profile->num_p4_programs = device_config.size();
  int program_index = 0;
  for (const auto &program : device_config) {
    std::snprintf(device_profile->p4_programs[program_index].prog_name,
                  PROG_NAME_LEN,
                  "%s",
                  program.prog_name_.c_str());
    // TODO: device_profile currently has only one bfrt_json file.
    // copying the first json path for now
    device_profile->p4_programs[program_index].bfrt_json_file =
        const_cast<char *>(program.tdi_info_file_paths_[0].c_str());
    device_profile->p4_programs[program_index].num_p4_pipelines =
        program.p4_pipelines_.size();
    int profile_index = 0;
    for (const auto &profile : program.p4_pipelines_) {
      auto device_pipe_prof = &(device_profile->p4_programs[program_index]
                                    .p4_pipelines[profile_index]);
      if (profile.name_.size() >= PROG_NAME_LEN) {
        LOG_ERROR("%s:%d Profile name %s is longer than allowed = %d",
                  __func__,
                  __LINE__,
                  profile.name_.c_str(),
                  PROG_NAME_LEN - 1);
        return TDI_INVALID_ARG;
      }
      std::snprintf(device_pipe_prof->p4_pipeline_name,
                    PROG_NAME_LEN,
                    "%s",
                    profile.name_.c_str());
      device_pipe_prof->cfg_file =
          const_cast<char *>(profile.binary_path_.c_str());
      device_pipe_prof->runtime_context_file =
          const_cast<char *>(profile.context_path_.c_str());
      device_pipe_prof->pi_config_file = nullptr;
      device_pipe_prof->num_pipes_in_scope = profile.scope_vec_.size();
      std::copy(profile.scope_vec_.begin(),
                profile.scope_vec_.end(),
                device_pipe_prof->pipe_scope);
      profile_index++;
    }
    program_index++;
  }
  return TDI_SUCCESS;
}

std::vector<tdi::ProgramConfig> convertDevProfileToDeviceConfig(
    const bf_dev_id_t &dev_id,
    const bf_device_profile_t *dev_profile,
    const std::vector<std::string> &tdi_fixed_json_path_vec) {
  // tracks P4 programs with valid bf-rt.json of their own
  uint32_t num_valid_p4_programs = 0;

  std::vector<tdi::ProgramConfig> program_config_vec;
  for (int i = 0; i < dev_profile->num_p4_programs; i++) {
    std::string prog_name(dev_profile->p4_programs[i].prog_name);
    std::vector<std::string> tdi_p4_json_vect(tdi_fixed_json_path_vec);
    std::vector<tdi::P4Pipeline> p4_pipelines;
    // If bf-rt.json doesn't exist, then continue the loop
    if (dev_profile->p4_programs[i].bfrt_json_file == nullptr) {
      LOG_ERROR(
          "%s:%d No TDI json file found for program %s"
          " Not adding TDI Info object for it",
          __func__,
          __LINE__,
          dev_profile->p4_programs[i].prog_name);
      continue;
    } else {
      num_valid_p4_programs++;
    }
    for (int j = 0; j < dev_profile->p4_programs[i].num_p4_pipelines; j++) {
      std::string profile_name =
          dev_profile->p4_programs[i].p4_pipelines[j].p4_pipeline_name;
      std::string context_json_path =
          dev_profile->p4_programs[i].p4_pipelines[j].runtime_context_file;
      std::string binary_path =
          dev_profile->p4_programs[i].p4_pipelines[j].cfg_file;

      std::vector<uint32_t> pipe_scope;
      if (dev_profile->p4_programs[i].p4_pipelines[j].num_pipes_in_scope == 0) {
        // number of p4 programs and p4 pipelines should be both 1 in this case
        // because we expect pipe_scope to be specified in this scenario
        TDI_ASSERT(dev_profile->num_p4_programs == 1);
        TDI_ASSERT(dev_profile->p4_programs[i].num_p4_pipelines == 1);
        uint32_t num_pipes = 4;
        PipeMgrIntf::getInstance()->pipeMgrGetNumPipelines(dev_id, &num_pipes);
        for (uint32_t k = 0; k < num_pipes; k++) {
          pipe_scope.push_back(k);
        }
      } else {
        pipe_scope.assign(
            dev_profile->p4_programs[i].p4_pipelines[j].pipe_scope,
            dev_profile->p4_programs[i].p4_pipelines[j].pipe_scope +
                dev_profile->p4_programs[i].p4_pipelines[j].num_pipes_in_scope);
      }
      p4_pipelines.emplace_back(
          profile_name, context_json_path, binary_path, pipe_scope);
    }
    tdi_p4_json_vect.push_back(dev_profile->p4_programs[i].bfrt_json_file);
    program_config_vec.emplace_back(prog_name, tdi_p4_json_vect, p4_pipelines);
  }
  if (num_valid_p4_programs == 0) {
    // If there is no p4_program, load TDI fixed tables only with a fixed
    // fake p4 program name "$SHARED".
    std::vector<tdi::P4Pipeline> dummy;
    std::string prog_name = "$SHARED";
    program_config_vec.emplace_back(prog_name, tdi_fixed_json_path_vec, dummy);
  }
  return program_config_vec;
}

bool file_exists(const std::string &path) { return std::ifstream(path).good(); }

tdi_status_t validateDevId(const tdi_dev_id_t &dev_id) {
  std::set<tdi_dev_id_t> device_id_list;
  tdi::DevMgr::getInstance().deviceIdListGet(&device_id_list);
  if (device_id_list.find(dev_id) == device_id_list.end()) {
    LOG_ERROR(
        "%s:%d Device with id:%d doesn't exist", __func__, __LINE__, dev_id);
    return TDI_INVALID_ARG;
  }
  return TDI_SUCCESS;
}

tdi_status_t validateDevConfig(
    const tdi_dev_id_t &dev_id,
    const std::vector<tdi::ProgramConfig> device_config) {
  for (const auto &program : device_config) {
    // validate tdi_info_file path
    for (const auto &tdi_info_file : program.tdi_info_file_paths_) {
      if (!file_exists(tdi_info_file)) {
        LOG_ERROR("%s:%d Invalid tdi_info_file path: %s",
                  __func__,
                  __LINE__,
                  tdi_info_file.c_str());
        return TDI_INVALID_ARG;
      }
    }
    // validate context.json and binary file paths
    for (const auto &pipeline : program.p4_pipelines_) {
      if (!file_exists(pipeline.context_path_)) {
        LOG_ERROR("%s:%d Invalid context.json file path: %s",
                  __func__,
                  __LINE__,
                  pipeline.context_path_.c_str());
        return TDI_INVALID_ARG;
      }
      if (!file_exists(pipeline.binary_path_)) {
        LOG_ERROR("%s:%d Invalid binary file path: %s",
                  __func__,
                  __LINE__,
                  pipeline.binary_path_.c_str());
        return TDI_INVALID_ARG;
      }
      // validate pipe_scope
      uint32_t num_pipes;
      bf_pal_num_pipes_get(dev_id, &num_pipes);
      if (pipeline.scope_vec_.size() > num_pipes) {
        LOG_ERROR("%s:%d Invalid pipe scope", __func__, __LINE__);
        return TDI_INVALID_ARG;
      }
      for (const auto &pipe : pipeline.scope_vec_) {
        if (pipe >= num_pipes) {
          LOG_ERROR("%s:%d Invalid pipe scope", __func__, __LINE__);
          return TDI_INVALID_ARG;
        }
      }
    }
  }
  return TDI_SUCCESS;
}
}  // namespace

tdi_status_t tdi_device_add(bf_dev_id_t dev_id,
                            bf_dev_family_t dev_family,
                            bf_device_profile_t *dev_profile,
                            bf_dma_info_t * /*dma_info*/,
                            bf_dev_init_mode_t warm_init_mode) {
  auto &dev_mgr_obj = DevMgr::getInstance();

  LOG_DBG("%s:%d TDI Device Add called for dev : %d : warm init mode : %d",
          __func__,
          __LINE__,
          dev_id,
          warm_init_mode);
  // Load fixed tdi json files.
  // Fixed tdi json files would be present in both fixed tdi info obj and
  // p4 tdi info obj
  std::vector<std::string> tdi_fixed_json_path_vec;
  if (dev_profile->tdi_non_p4_json_dir_path != NULL) {
    tdi_fixed_json_path_vec = tdiFixedJsonFilePathsGet(
        dev_family, dev_profile->tdi_non_p4_json_dir_path);
  }
  std::vector<tdi_mgr_type_e> mgr_list =
      {};  // Figure out a way to get mgr_list through the dev_add cb

  return dev_mgr_obj.deviceAdd<tdi::tna::tofino::Device>(
      dev_id,
      TDI_ARCH_TYPE_TNA,
      convertDevProfileToDeviceConfig(
          dev_id, dev_profile, tdi_fixed_json_path_vec),
      &mgr_list,
      nullptr);
}

tdi_status_t tdi_device_remove(bf_dev_id_t dev_id) {
  auto &dev_mgr_obj = tdi::DevMgr::getInstance();
  return dev_mgr_obj.deviceRemove(dev_id);
}

Device::Device(const tdi_dev_id_t &device_id,
               const tdi_arch_type_e &arch_type,
               const std::vector<tdi::ProgramConfig> &device_config,
               void *target_options,
               void *cookie)
    : tdi::tna::Device(device_id, arch_type, device_config, cookie) {
  auto mgr_list = static_cast<std::vector<tdi_mgr_type_e> *>(target_options);
  std::copy(
      mgr_list->begin(), mgr_list->end(), std::back_inserter(mgr_type_list_));

  uint32_t program_num = 0;
  std::string first_p4_name = "";
  // Parse tdi json for every program
  for (const auto &program_config : device_config) {
    auto tdi_info_mapper = std::unique_ptr<tdi::TdiInfoMapper>(
        new tdi::tna::tofino::TdiInfoMapper());
    auto table_factory = std::unique_ptr<tdi::TableFactory>(
        new tdi::tna::tofino::TableFactory());

    auto tdi_info_parser = std::unique_ptr<TdiInfoParser>(
        new TdiInfoParser(std::move(tdi_info_mapper)));
    tdi_info_parser->parseTdiInfo(program_config.tdi_info_file_paths_);
    auto tdi_info = tdi::TdiInfo::makeTdiInfo(program_config.prog_name_,
                                              std::move(tdi_info_parser),
                                              table_factory.get());
    // Parse context json
    auto status = ContextInfoParser::parseContextJson(
        tdi_info.get(), device_id, program_config);
    if (status) {
      LOG_ERROR("%s:%d Failed to parse context.json", __func__, __LINE__);
    }

    tdi_info_map_.emplace(program_config.prog_name_, std::move(tdi_info));
    tdi_dev_state_map_.emplace(program_config.prog_name_,
                               std::make_shared<tdi::tna::tofino::DeviceState>(
                                   program_config.prog_name_));
    // If this is not the first program, then copy fixed object state
    // shared_ptrs
    if (program_num != 0) {
      auto first_prog_state = tdi_dev_state_map_[first_p4_name];
      tdi_dev_state_map_[program_config.prog_name_]->copyFixedObjects(
          *first_prog_state);
    } else {
      first_p4_name = program_config.prog_name_;
    }
    program_num++;
  }
}
tdi_status_t Device::createSession(
    std::shared_ptr<tdi::Session> *session) const {
  auto session_t = std::make_shared<tdi::tna::tofino::Session>(mgr_type_list_);
  auto status = session_t->create();
  *session = session_t;
  if (status != TDI_SUCCESS) *session = nullptr;
  return status;
}
tdi_status_t Device::createTarget(std::unique_ptr<tdi::Target> *target) const {
  *target = std::unique_ptr<tdi::Target>(
      new tdi::tna::tofino::Target(this->device_id_,
                                   TNA_DEV_PIPE_ALL,
                                   TNA_DIRECTION_ALL,
                                   PIPE_MGR_PVS_PARSER_ALL));
  return TDI_SUCCESS;
}

tdi_status_t Init::tdiModuleInit(void * /*target_options*/) {
  tdi_status_t sts = TDI_SUCCESS;
  bf_drv_client_callbacks_t callbacks = {0};

  std::unique_ptr<tdi::WarmInitImpl> warm_init_impl(
      new tdi::tna::tofino::WarmInitImpl());

  DevMgr::warmInitImplSet(std::move(warm_init_impl));

  // Register the Device Add/ Remove functions with the DVM
  sts = bf_drv_register(tdi_module_name.c_str(), &tdi_drv_hdl);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register tdi module with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  callbacks.device_add = tdi_device_add;
  callbacks.device_del = tdi_device_remove;
  sts = bf_drv_client_register_callbacks(
      tdi_drv_hdl, &callbacks, BF_CLIENT_PRIO_0);
  if (sts != TDI_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to register tdi callbacks with DVM : %s (%d)",
              __func__,
              __LINE__,
              bf_err_str(sts),
              sts);
    return sts;
  }

  return sts;
}

tdi_status_t WarmInitImpl::deviceWarmInitBegin(
    const tdi_dev_id_t &device_id,
    const tdi::WarmInitOptions &warm_init_options) {
  // static_cast
  const auto &options =
      static_cast<const tdi::tna::tofino::WarmInitOptions &>(warm_init_options);

  if (auto status = validateDevId(device_id) != TDI_SUCCESS) {
    return status;
  }
  if (auto status =
          validateDevConfig(device_id, options.device_config_) != TDI_SUCCESS) {
    return status;
  }
  auto status = bf_pal_device_warm_init_begin(device_id,
                                              options.warm_init_mode_,
                                              options.serdes_upgrade_mode_,
                                              options.upgrade_agents_);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d: Error : bf_pal_device_warm_init_begin() failed",
              __func__,
              __LINE__);
    return status;
  }
  bf_device_profile_t device_profile = {};
  status = convertDeviceConfigToDeviceProfile(options.device_config_,
                                              &device_profile);
  if (status != TDI_SUCCESS) {
    return status;
  }
  status = bf_pal_device_add(device_id, &device_profile);
  if (status != TDI_SUCCESS) {
    LOG_ERROR("%s:%d : Error : failed to add a device", __func__, __LINE__);
    return status;
  }
  return status;
}

tdi_status_t WarmInitImpl::deviceWarmInitEnd(const tdi_dev_id_t &device_id) {
  if (auto status = validateDevId(device_id) != TDI_SUCCESS) {
    return status;
  }
  return bf_pal_device_warm_init_end(device_id);
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
