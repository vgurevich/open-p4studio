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


#include <tdi_common/tdi_tofino_target.hpp>

namespace tdi {
namespace tna {
namespace tofino {

tdi_status_t Target::setValue(const tdi_target_e &target_field,
                              const uint64_t &value) {
  if (target_field == static_cast<tdi_target_e>(TDI_TOFINO_TARGET_PARSER_ID)) {
    this->parser_id_ = value;
  } else {
    return tdi::tna::Target::setValue(target_field, value);
  }
  return TDI_SUCCESS;
}

tdi_status_t Target::getValue(const tdi_target_e &target_field,
                              uint64_t *value) const {
  if (target_field == static_cast<tdi_target_e>(TDI_TOFINO_TARGET_PARSER_ID)) {
    *value = this->parser_id_;
  } else {
    return tdi::tna::Target::getValue(target_field, value);
  }
  return TDI_SUCCESS;
}

void Target::getTargetVals(bf_dev_target_t *dev_tgt,
                           bf_dev_direction_t *direction,
                           uint8_t *parser_id) const {
  if (dev_tgt) {
    dev_tgt->device_id = this->dev_id_;
    dev_tgt->dev_pipe_id = this->pipe_id_;
  }
  if (direction) {
    *direction = static_cast<bf_dev_direction_t>(this->direction_);
  }
  if (parser_id) {
    *parser_id = this->parser_id_;
  }
}

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
