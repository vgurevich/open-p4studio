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


/** @file tdi_target.hpp
 *
 *  @brief Contains common cpp defines like Target, Flags
 */
#ifndef _TDI_TOFINO_TARGET_HPP_
#define _TDI_TOFINO_TARGET_HPP_

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

// tdi includes
#include <tdi/common/tdi_defs.h>
#include <tdi/common/tdi_target.hpp>

// tna includes
#include <tdi/arch/tna/tna_target.hpp>

// tdi tofino includes
#include <tdi_tofino/tdi_tofino_defs.h>

// local includes
#include "tdi_state.hpp"

namespace tdi {
namespace tna {
namespace tofino {

/**
 * @brief Class which encapsulates static info of a device eg.
 * Arch type,
 * Mgrs it was started with, State information, TdiInfo of all
 * programs, Pipeline profile information.
 *
 * Static info means that none of this can be changed after
 * device add happens.
 */
class Device : public tdi::tna::Device {
 public:
  Device(const tdi_dev_id_t &device_id,
         const tdi_arch_type_e &arch_type,
         const std::vector<tdi::ProgramConfig> &device_config,
         void *target_options,
         void *cookie);

  virtual tdi_status_t createSession(
      std::shared_ptr<tdi::Session> *session) const override final;
  virtual tdi_status_t createTarget(
      std::unique_ptr<tdi::Target> *target) const override final;
  virtual tdi_status_t createFlags(
      const uint64_t & /*flags_val*/,
      std::unique_ptr<tdi::Flags> * /*flags*/) const override final {
    return TDI_SUCCESS;
  }

  std::shared_ptr<tdi::tna::tofino::DeviceState> devStateGet(
      const std::string &prog_name) const {
    auto state = tdi_dev_state_map_.find(prog_name);
    if (state == tdi_dev_state_map_.end()) {
      return nullptr;
    }
    return state->second;
  };

 private:
  std::vector<tdi_mgr_type_e> mgr_type_list_;
  std::map<std::string, std::shared_ptr<tdi::tna::tofino::DeviceState>>
      tdi_dev_state_map_;
};

/**
 * @brief Can be constructed by \ref tdi::Device::createTarget()
 */
class Target : public tdi::tna::Target {
 public:
  Target(tdi_dev_id_t dev_id,
         tna_pipe_id_t pipe_id,
         tna_direction_e direction,
         uint8_t parser_id)
      : tdi::tna::Target(dev_id, pipe_id, direction), parser_id_(parser_id){};
  virtual tdi_status_t setValue(const tdi_target_e &target,
                                const uint64_t &value) override;
  virtual tdi_status_t getValue(const tdi_target_e &target,
                                uint64_t *value) const override;

  void getTargetVals(bf_dev_target_t *dev_tgt,
                     bf_dev_direction_t *direction,
                     uint8_t *prsr_id) const;

 private:
  uint8_t parser_id_{0};
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_TOFINO_TARGET_HPP_
