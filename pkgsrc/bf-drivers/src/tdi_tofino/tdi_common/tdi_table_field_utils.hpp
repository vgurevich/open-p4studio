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


#ifndef _TDI_TABLE_FIELD_UTILS_HPP_
#define _TDI_TABLE_FIELD_UTILS_HPP_

#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_table.hpp>

namespace tdi {

namespace utils {

template <class T1, class T2>
static const std::string getStrFromDataMaps(const T1 &str, const T2 &maps) {
  for (const auto &kv : maps) {
    if (kv.second == str) {
      return kv.first;
    }
  }
  return "UNKNOWN";
}

}  // namespace utils
}  // namespace tdi

#endif  // _TDI_TABLE_FIELD_UTILS_HPP_
