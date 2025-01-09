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


#ifndef _TDI_MIRROR_TABLE_KEY_IMPL_HPP
#define _TDI_MIRROR_TABLE_KEY_IMPL_HPP

#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_key.hpp>
#include "bf_types/bf_types.h"

namespace tdi {
namespace tna {
namespace tofino {

class MirrorCfgTableKey : public tdi::TableKey {
 public:
  MirrorCfgTableKey(const Table *table) : TableKey(table){};
  ~MirrorCfgTableKey() = default;

  virtual tdi_status_t setValue(const tdi_id_t &field_id,
                                const tdi::KeyFieldValue &field_value) override;

  virtual tdi_status_t getValue(const tdi_id_t &field_id,
                                tdi::KeyFieldValue *value) const override;

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint64_t &value);

  tdi_status_t setValue(const tdi::KeyFieldInfo *key_field,
                        const uint8_t *value,
                        const size_t &size);

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        uint64_t *value) const;

  tdi_status_t getValue(const tdi::KeyFieldInfo *key_field,
                        const size_t &size,
                        uint8_t *value) const;

  tdi_status_t reset() override final;

  const bf_mirror_id_t &getId() const { return session_id_; }

  void setId(const bf_mirror_id_t id) { session_id_ = id; }

 private:
  bf_mirror_id_t session_id_ = 0;
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi
#endif  //_TDI_MIRROR_TABLE_KEY_IMPL_HPP
