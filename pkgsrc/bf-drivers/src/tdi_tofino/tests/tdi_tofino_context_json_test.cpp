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


#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>   // std::ifstream
#include <iterator>  // std::distance
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>
#include <cstring>  // std::memcmp

#if 0
#include <tdi/tdi_common.h>
#include <tdi/tdi_info.hpp>
#include <tdi/tdi_table_operations.hpp>
#include <../tdi_common/tdi_info_impl.hpp>
#include <../tdi_common/tdi_cjson.hpp>
#include <../tdi_common/tdi_utils.hpp>
#include <tdi/tdi_table.hpp>
#include <../tdi_common/tdi_table_impl.hpp>

#include "tdi_info_test.hpp"
#endif

#include <tdi/common/tdi_info.hpp>
#include <tdi/common/tdi_learn.hpp>
#include <tdi/common/tdi_operations.hpp>
#include <tdi/common/tdi_table.hpp>

#include <tdi/common/tdi_json_parser/tdi_cjson.hpp>
#include <tdi/common/tdi_utils.hpp>

#include "tdi_tofino_context_json_test.hpp"

namespace tdi {
namespace tna {
namespace tofino {
namespace tofino_test {

namespace {
void getTable(const TdiInfo *tdi_info,
              std::string table_name,
              const Table *&table) {
  auto status = tdi_info->tableFromNameGet(table_name, &table);

  ASSERT_EQ(status, TDI_SUCCESS);
  ASSERT_NE(table, nullptr);
  ASSERT_EQ(table->tableInfoGet()->nameGet(), table_name);
}

void verifyRefTableMap(const table_ref_map_t &table_ref_map,
                       table_ref_map_t &exp_table_ref_map) {
  EXPECT_EQ(table_ref_map.size(), exp_table_ref_map.size());
  for (const auto &kv : table_ref_map) {
    if (exp_table_ref_map.find(kv.first) != exp_table_ref_map.end()) {
      exp_table_ref_map.erase(kv.first);
    }
  }

  if (table_ref_map.size() != 0) {
    EXPECT_EQ(exp_table_ref_map.size(), 0)
        << "All the expected references are not present" << std::endl;
    std::cout << "Missing reference: " << std::endl;
    for (const auto &kv : exp_table_ref_map) {
      for (const auto &elem : kv.second)
        std::cout << kv.first << ": " << elem.name << std::endl;
    }
  }
}

void verifyActHdlToIdMap(
    const std::map<pipe_act_fn_hdl_t, tdi_id_t> &act_hdl_to_id_map,
    std::map<pipe_act_fn_hdl_t, tdi_id_t> &exp_map) {
  EXPECT_EQ(act_hdl_to_id_map.size(), exp_map.size());
  for (const auto &kv : act_hdl_to_id_map) {
    if (exp_map.find(kv.first) != exp_map.end()) {
      exp_map.erase(kv.first);
    }
  }

  if (act_hdl_to_id_map.size() != 0) {
    EXPECT_EQ(exp_map.size(), 0) << "All act_hdl_to_id not found" << std::endl;
    std::cout << "Missing items: " << std::endl;
    for (const auto &kv : exp_map) {
      std::cout << kv.first << ":" << kv.second << std::endl;
    }
  }
}
}  // Anonymous namespace
// TnaExactMatch
TEST_P(TnaExactMatchTest, TestContextInfoContents) {
  table_ref_map_t table_ref_map;
  const Table *table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", table);

  const auto tableContextInfo = static_cast<const TofinoTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());
  ASSERT_NE(tableContextInfo, nullptr);

  EXPECT_EQ(tableContextInfo->tableNameGet(), "pipe.SwitchIngress.forward");
  EXPECT_EQ(tableContextInfo->tableHdlGet(), 16777218);
  verifyRefTableMap(tableContextInfo->tableRefMapGet(), table_ref_map);
  EXPECT_EQ(tableContextInfo->isConstTable(), false);
  const auto key_size = tableContextInfo->keySizeGet();
  EXPECT_EQ(key_size.bits, 48);

  std::map<pipe_act_fn_hdl_t, tdi_id_t> exp_act_hdl_id_map;
  exp_act_hdl_id_map[536870913] = 32848556;  // hit
  exp_act_hdl_id_map[536870914] = 17988458;  // miss

  verifyActHdlToIdMap(tableContextInfo->actFnHdlToIdGet(), exp_act_hdl_id_map);
}

TEST_P(TnaExactMatchTest, TestKeyFieldContextInfoContents) {
  const Table *tdi_table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", tdi_table);
  const auto *tdi_table_info = tdi_table->tableInfoGet();

  const auto &name_key_map = tdi_table_info->name_key_map_;
  const auto iter = name_key_map.find("hdr.ethernet.dst_addr");
  ASSERT_NE(iter, name_key_map.end()) << "key not found" << std::endl;

  const auto key_field_context_info =
      static_cast<const TofinoKeyFieldContextInfo *>(
          iter->second->keyFieldContextInfoGet());
  ASSERT_NE(key_field_context_info, nullptr);

  EXPECT_EQ(key_field_context_info->nameGet(), "hdr.ethernet.dst_addr");
  EXPECT_EQ(key_field_context_info->startBitGet(), 0);
  EXPECT_EQ(key_field_context_info->fieldOffSetGet(), 0);
  EXPECT_EQ(key_field_context_info->parentFieldFullByteSizeGet(), 6);
  EXPECT_EQ(key_field_context_info->isPartition(), false);
}

TEST_P(TnaExactMatchTest, TestActionContextInfoContents) {
  const Table *tdi_table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", tdi_table);
  const auto *tdi_table_info = tdi_table->tableInfoGet();

  const auto &name_action_map = tdi_table_info->name_action_map_;
  const auto iter = name_action_map.find("SwitchIngress.hit");
  ASSERT_NE(iter, name_action_map.end()) << "action not found" << std::endl;

  const auto action_context_info = static_cast<const TofinoActionContextInfo *>(
      iter->second->actionContextInfoGet());
  ASSERT_NE(action_context_info, nullptr);
  EXPECT_EQ(action_context_info->nameGet(), "SwitchIngress.hit");
  EXPECT_EQ(action_context_info->actionFnHdlGet(), 536870913);
  EXPECT_EQ(action_context_info->dataSzGet(), 2);
}

TEST_P(TnaExactMatchTest, TestDataFieldContextInfoContents) {
  const Table *tdi_table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", tdi_table);
  const auto *tdi_table_info = tdi_table->tableInfoGet();

  const auto &name_action_map = tdi_table_info->name_action_map_;
  const auto &action_info = name_action_map.find("SwitchIngress.hit")->second;

  const auto iter = action_info->data_fields_names_.find("port");
  ASSERT_NE(iter, action_info->data_fields_names_.end())
      << "dataField not found" << std::endl;
  const auto data_field_context_info =
      static_cast<const TofinoDataFieldContextInfo *>(
          iter->second->dataFieldContextInfoGet());

  ASSERT_NE(data_field_context_info, nullptr);
  EXPECT_EQ(data_field_context_info->nameGet(), "port");
}

// TnaActionProfileTest
TEST_P(TnaActionProfileTest, TestContextInfoContents) {
  table_ref_map_t table_ref_map;
  const Table *table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", table);
  ASSERT_EQ(1, 1);
  const auto tableContextInfo = static_cast<const TofinoTableContextInfo *>(
      table->tableInfoGet()->tableContextInfoGet());
  ASSERT_NE(tableContextInfo, nullptr);

  EXPECT_EQ(tableContextInfo->tableNameGet(), "pipe.SwitchIngress.forward");
  EXPECT_EQ(tableContextInfo->tableHdlGet(), 16777221);

  tdi_table_ref_info_t ref_info = {
      "SwitchIngress.action_profile",
      33554433,    // tbl_hdl
      2179865181,  // id
      true         // indirect_ref
  };

  table_ref_map["action_data_table_refs"] =
      std::vector<tdi_table_ref_info_t>(1, ref_info);
  verifyRefTableMap(tableContextInfo->tableRefMapGet(), table_ref_map);
  EXPECT_EQ(tableContextInfo->isConstTable(), false);
  const auto key_size = tableContextInfo->keySizeGet();
  EXPECT_EQ(key_size.bits, 21);
}

TEST_P(TnaActionProfileTest, TestKeyFieldContextInfoContents) {
  const Table *tdi_table = nullptr;
  getTable(tdi_info.get(), "pipe.SwitchIngress.forward", tdi_table);
  const auto *tdi_table_info = tdi_table->tableInfoGet();

  const auto &name_key_map = tdi_table_info->name_key_map_;
  const auto iter = name_key_map.find("vid");
  ASSERT_NE(iter, name_key_map.end()) << "key not found" << std::endl;

  const auto key_field_context_info =
      static_cast<const TofinoKeyFieldContextInfo *>(
          iter->second->keyFieldContextInfoGet());
  ASSERT_NE(key_field_context_info, nullptr);

  EXPECT_EQ(key_field_context_info->nameGet(), "vid");
  EXPECT_EQ(key_field_context_info->startBitGet(), 0);
  EXPECT_EQ(key_field_context_info->fieldOffSetGet(), 2);
  EXPECT_EQ(key_field_context_info->parentFieldFullByteSizeGet(), 2);
  EXPECT_EQ(key_field_context_info->isPartition(), false);
}

}  // namespace tofino_test
}  // namespace tofino
}  // namespace tna
}  // namespace tdi
