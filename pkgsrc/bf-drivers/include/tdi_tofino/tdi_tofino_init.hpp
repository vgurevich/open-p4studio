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


/** @file tdi_tofino_init.hpp
 *
 *  @brief Contains TDI Dev Manager APIs. These APIs help manage TdiInfo
 *  \n objects with respect to the devices and programs in the ecosystem.
 *
 *  \n Contains DevMgr, Device, Target, Flags
 */
#ifndef _TDI_TOFINO_INIT_HPP_
#define _TDI_TOFINO_INIT_HPP_

#include <functional>
#include <memory>
#include <mutex>

// tdi includes

// tna includes
#include <tdi/arch/tna/tna_init.hpp>

// local includes

#ifdef __cplusplus
extern "C" {
#endif

#include <dvm/bf_drv_intf.h>

#ifdef __cplusplus
}
#endif

namespace tdi {
namespace tna {
namespace tofino {

/**
 * @brief Class to manage initialization of TDI <br>
 * <B>Creation: </B> Cannot be created
 */
class Init : public tdi::Init {
 public:
  /**
   * @brief Bf Rt Module Init API. This function needs to be called to
   * initialize TDI. Some specific managers can be specified to be skipped
   * TDI initialization. This allows TDI session layer to not know about these
   * managers. By default, no mgr initialization is skipped if empty vector is
   * passed
   *
   * @param[in] mgr_type_list vector of mgrs to skip initializing. If
   * empty, don't skip anything
   * @return Status of the API call
   */
  static tdi_status_t tdiModuleInit(void *target_options);
  static std::string tdi_module_name;
  static bf_drv_client_handle_t tdi_drv_hdl;
};  // Init

class WarmInitOptions : public tdi::WarmInitOptions {
 public:
  WarmInitOptions() = delete;
  WarmInitOptions(bf_dev_init_mode_t warm_init_mode,
                  bf_dev_serdes_upgrade_mode_t serdes_upgrade_mode,
                  bool upgrade_agents,
                  std::vector<tdi::ProgramConfig> device_config)
      : warm_init_mode_(warm_init_mode),
        serdes_upgrade_mode_(serdes_upgrade_mode),
        upgrade_agents_(upgrade_agents),
        device_config_(device_config){};

  bf_dev_init_mode_t warm_init_mode_;
  bf_dev_serdes_upgrade_mode_t serdes_upgrade_mode_;
  bool upgrade_agents_;
  std::vector<tdi::ProgramConfig> device_config_;
};

class WarmInitImpl : public tdi::WarmInitImpl {
 public:
  tdi_status_t deviceWarmInitBegin(
      const tdi_dev_id_t &device_id,
      const tdi::WarmInitOptions &warm_init_options) override;
  tdi_status_t deviceWarmInitEnd(const tdi_dev_id_t &device_id) override;
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif
