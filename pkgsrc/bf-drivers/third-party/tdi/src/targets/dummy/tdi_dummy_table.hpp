/*******************************************************************************
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this software except as stipulated in the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * $Id: $
 *
 ******************************************************************************/
#ifndef _TDI_DUMMY_TABLE_HPP
#define _TDI_DUMMY_TABLE_HPP

#include <tdi/common/tdi_table.hpp>

namespace tdi {
namespace tna {
namespace dummy {

class MatchActionDirect : public tdi::Table {
 public:
  MatchActionDirect(const tdi::TdiInfo *tdi_info,
                    const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };

  bool actionIdApplicable() const override { return true; };
};

class MatchActionIndirect : public tdi::Table {
 public:
  MatchActionIndirect(const tdi::TdiInfo *tdi_info,
                      const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class ActionProfile : public tdi::Table {
 public:
  ActionProfile(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class Selector : public tdi::Table {
 public:
  Selector(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class CounterIndirect : public tdi::Table {
 public:
  CounterIndirect(const tdi::TdiInfo *tdi_info,
                  const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class MeterIndirect : public tdi::Table {
 public:
  MeterIndirect(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class RegisterIndirect : public tdi::Table {
 public:
  RegisterIndirect(const tdi::TdiInfo *tdi_info,
                   const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class PortConfigure : public tdi::Table {
 public:
  PortConfigure(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

class PortStat : public tdi::Table {
 public:
  PortStat(const tdi::TdiInfo *tdi_info, const tdi::TableInfo *table_info)
      : tdi::Table(tdi_info, table_info) {
    LOG_ERROR("Creating table for %s", table_info->nameGet().c_str());
  };
};

}  // namespace dummy
}  // namespace tna
}  // namespace tdi

#endif  //
