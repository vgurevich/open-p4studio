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
#include <unordered_set>

#include <bf_rt/bf_rt.hpp>
#include <bf_rt_common/bf_rt_cjson.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>

#include "bf_rt_table_test.hpp"

// Some Guidelines for writing unit tests
// 1. For any of the get APIs, don't set the default values of any of the
//    variables (the pointers to which will be passed to the get API)

namespace bfrt {
namespace bfrt_test {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::WithArgs;

namespace {
bool checkDefaultOnly(const BfRtTable &table, const BfRtTableData &data) {
  bf_rt_id_t action_id = 0;
  auto bf_status = data.actionIdGet(&action_id);
  if (bf_status != BF_SUCCESS) {
    return false;
  }

  AnnotationSet annotations;
  bf_status = table.actionAnnotationsGet(action_id, &annotations);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  auto def_an = Annotation("@defaultonly", "");
  if (annotations.find(def_an) != annotations.end()) {
    return true;
  }
  return false;
}
}  // anonymous namespace

void BfRtTableTest::tableRandomKeyGet(
    const BfRtTable &table,
    const BfRtEntryGen &entry_generator,
    const uint32_t &num_entries,
    std::vector<GenericKey> *random_keys_arg) {
  *random_keys_arg = entry_generator.getTableKeyEntries(num_entries);
}

void BfRtTableTest::tableRandomDataGet(
    const BfRtTable &table,
    const BfRtEntryGen &entry_generator,
    const uint32_t &num_entries,
    std::vector<GenericData> *random_data_arg) {
  *random_data_arg = entry_generator.getTableDataEntries(num_entries);

  // If a table has idle timeout enabled, currently we are only checking the
  // notify mode of it. Hence iterate over all the random_data objects and
  // remove any fields which are $ENTRY_HIT_STATE
  for (auto &single_random_data : *random_data_arg) {
    auto &data_field_list = single_random_data.dataFieldListGet();
    for (auto data_field = data_field_list.begin();
         data_field != data_field_list.end();
         data_field++) {
      std::string field_name;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(table.dataFieldNameGet(data_field->fieldIdGet(),
                                              single_random_data.actionIdGet(),
                                              &field_name));
      } else {
        EXPECT_SUCCESS(
            table.dataFieldNameGet(data_field->fieldIdGet(), &field_name));
      }
      if (field_name == "$ENTRY_HIT_STATE") {
        data_field_list.erase(data_field);
        break;
      }
    }
  }
}

void BfRtTableTest::tableKeySetHelper(const GenericKey &random_key,
                                      BfRtTableKey &table_key) {
  const BfRtTable *table = nullptr;
  ASSERT_SUCCESS(table_key.tableGet(&table));
  ASSERT_NE(table, nullptr);
  for (const auto &field_id : random_key.fieldIdListGet()) {
    const GenericKeyField &generic_key_field = random_key.keyFieldGet(field_id);
    KeyFieldType key_type;
    ASSERT_SUCCESS(table->keyFieldTypeGet(field_id, &key_type));
    size_t size;
    EXPECT_SUCCESS(table->keyFieldSizeGet(field_id, &size));
    size = (size + 7) / 8;
    // The following is a safety check to ensure the sanity of the generator
    ASSERT_EQ(size, generic_key_field.sizeGet())
        << "Generator key field size of : " << generic_key_field.sizeGet()
        << " does not match the size of the key obtained from the table : "
        << size << "\n";
    switch (key_type) {
      case KeyFieldType::EXACT: {
        if (size > 8) {
          const auto &val_vec = generic_key_field.getValue();
          ASSERT_SUCCESS(
              table_key.setValue(field_id, val_vec.data(), val_vec.size()));
        } else {
          uint64_t val;
          generic_key_field.getValue<uint64_t>(&val);
          ASSERT_SUCCESS(table_key.setValue(field_id, val));
        }
        break;
      }
      case KeyFieldType::TERNARY: {
        if (size > 8) {
          const auto &val_vec = generic_key_field.getValue();
          const auto &mask_vec = generic_key_field.getMask();
          ASSERT_SUCCESS(table_key.setValueandMask(
              field_id, val_vec.data(), mask_vec.data(), val_vec.size()));
        } else {
          uint64_t val, mask;
          generic_key_field.getValue<uint64_t>(&val);
          generic_key_field.getMask<uint64_t>(&mask);
          ASSERT_SUCCESS(table_key.setValueandMask(field_id, val, mask));
        }
        break;
      }
      case KeyFieldType::RANGE: {
        if (size > 8) {
          const auto &start_vec = generic_key_field.getValue();
          const auto &end_vec = generic_key_field.getMask();
          ASSERT_SUCCESS(table_key.setValueRange(
              field_id, start_vec.data(), end_vec.data(), start_vec.size()));
        } else {
          uint64_t start, end;
          generic_key_field.getValue<uint64_t>(&start);
          generic_key_field.getMask<uint64_t>(&end);
          ASSERT_SUCCESS(table_key.setValueRange(field_id, start, end));
        }
        break;
      }
      case KeyFieldType::LPM: {
        if (size > 8) {
          const auto &val_vec = generic_key_field.getValue();
          uint16_t p_len = generic_key_field.getPrefixLen();
          ASSERT_SUCCESS(table_key.setValueLpm(
              field_id, val_vec.data(), p_len, val_vec.size()));
        } else {
          uint64_t val;
          generic_key_field.getValue<uint64_t>(&val);
          uint16_t p_len = generic_key_field.getPrefixLen();
          ASSERT_SUCCESS(table_key.setValueLpm(field_id, val, p_len));
        }
        break;
      }
      case KeyFieldType::INVALID:
      default:
        EXPECT_EQ(1, 0) << "Field : " << field_id
                        << " Invalid key type : " << static_cast<int>(key_type)
                        << "\n";
    }
  }
}

void BfRtTableTest::tableDataSetHelper(
    const BfRtTable &table,
    const GenericData &single_random_data,
    BfRtTableData &table_data,
    const std::vector<bf_rt_id_t> &interested_fields) {
  for (const auto &field_id : interested_fields) {
    const auto &generic_data_field = single_random_data.dataFieldGet(field_id);
    // Get the type of the field
    DataType data_type;
    if (single_random_data.actionIdGet()) {
      ASSERT_SUCCESS(table.dataFieldDataTypeGet(
          field_id, single_random_data.actionIdGet(), &data_type));
    } else {
      ASSERT_SUCCESS(table.dataFieldDataTypeGet(field_id, &data_type));
    }

    switch (data_type) {
      case DataType::BOOL: {
        uint8_t val;
        generic_data_field.getValue(&val);
        EXPECT_SUCCESS(table_data.setValue(field_id, (bool)val));
        break;
      }
      case DataType::INT_ARR: {
        std::vector<bf_rt_id_t> mbr_ids = generic_data_field.getValue_intarr();
        EXPECT_SUCCESS(table_data.setValue(field_id, mbr_ids));
        break;
      }
      case DataType::BOOL_ARR: {
        std::vector<bool> mbr_sts = generic_data_field.getValue_boolarr();
        EXPECT_SUCCESS(table_data.setValue(field_id, mbr_sts));
        break;
      }
      case DataType::UINT64: {
        uint64_t val;
        generic_data_field.getValue(&val);
        EXPECT_SUCCESS(table_data.setValue(field_id, val));
        break;
      }
      case DataType::BYTE_STREAM: {
        const auto &val_vec = generic_data_field.getValue();
        EXPECT_SUCCESS(
            table_data.setValue(field_id, val_vec.data(), val_vec.size()));
        break;
      }
      case DataType::FLOAT: {
        // FIXME
        BF_RT_ASSERT(0);
        break;
      }
      case DataType::STRING: {
        std::string val_str;
        generic_data_field.getValue(&val_str);
        EXPECT_SUCCESS(table_data.setValue(field_id, val_str));
        // FIXME
        // For the time being we are not testing string data types.
        break;
      }
      default:
        EXPECT_EQ(1, 0) << "Field : " << field_id << " Invalid Data type : "
                        << static_cast<int>(data_type) << "\n";
    }
  }
}

void BfRtTableTest::tableDataSetHelper(const BfRtTable &table,
                                       const GenericData &single_random_data,
                                       BfRtTableData &table_data) {
  tableDataSetHelper(table,
                     single_random_data,
                     table_data,
                     single_random_data.fieldIdListGet());
}

void BfRtTableTest::tableExpectedMatchSpecGet(
    const BfRtTable &table,
    const std::vector<GenericKey> &random_keys_arg,
    std::vector<std::unique_ptr<MatchSpec>> *ms_vec) {
  for (const auto &key : random_keys_arg) {
    std::unique_ptr<MatchSpec> ms(new MatchSpec(key));
    ms_vec->push_back(std::move(ms));
  }
}

void BfRtTableTest::tableExpectedActionSpecGet(
    const BfRtTable &table,
    const std::vector<GenericData> &random_data_arg,
    std::vector<std::unique_ptr<ActionSpec>> *as_vec) {
  for (const auto &data : random_data_arg) {
    std::unique_ptr<ActionSpec> as(new ActionSpec(data));
    as_vec->push_back(std::move(as));
  }
}

void BfRtTableTest::compareKeyObjects(const BfRtTableKey &exp_obj,
                                      const BfRtTableKey &rcv_obj) const {
  // Get the parent table from the key objects
  const BfRtTable *table_1 = nullptr, *table_2 = nullptr;
  ASSERT_SUCCESS(exp_obj.tableGet(&table_1));
  ASSERT_SUCCESS(rcv_obj.tableGet(&table_2));
  // ???
  // The parent tables have to be equal (is it ok if I just compare the table
  // pointers here?)
  // ???
  ASSERT_EQ(table_1, table_2);

  // Get the key field id vectors
  std::vector<bf_rt_id_t> list_1, list_2;
  ASSERT_SUCCESS(table_1->keyFieldIdListGet(&list_1));
  ASSERT_SUCCESS(table_2->keyFieldIdListGet(&list_2));

  // Now compare the key field values
  ASSERT_EQ(list_1.size(), list_2.size())
      << "Expected field id vec size : " << list_1.size()
      << " Received field id vec size : " << list_2.size() << "\n";
  for (uint32_t i = 0; i < list_1.size(); i++) {
    // Since we are quering the same table for the field id list, they ought
    // to be in the same order in both the lists. If they are not, that
    // indicates an error? That's why we can simply iterate over the lists
    // and expect the field is to be in the exact same order
    ASSERT_EQ(list_1[i], list_2[i])
        << "Expected Field id : " << list_1[i]
        << " Received Field id : " << list_2[i] << "\n";

    // Get the type of the field
    KeyFieldType key_type_1, key_type_2;
    ASSERT_SUCCESS(table_1->keyFieldTypeGet(list_1[i], &key_type_1));
    ASSERT_SUCCESS(table_2->keyFieldTypeGet(list_2[i], &key_type_2));
    ASSERT_EQ(key_type_1, key_type_2)
        << "Field : " << list_1[i]
        << " Expected key type : " << static_cast<int>(key_type_1)
        << " Received key type : " << static_cast<int>(key_type_2) << "\n";

    size_t size_1, size_2;
    EXPECT_SUCCESS(table_1->keyFieldSizeGet(list_1[i], &size_1));
    EXPECT_SUCCESS(table_2->keyFieldSizeGet(list_2[i], &size_2));
    ASSERT_EQ(size_1, size_2)
        << "Field : " << list_1[i] << " Expected size : " << size_1
        << " Received size : " << size_2 << "\n";
    size_1 = (size_1 + 7) / 8;
    size_2 = (size_2 + 7) / 8;
    // Finally compare the values
    switch (key_type_1) {
      case KeyFieldType::EXACT: {
        if (size_1 > 8) {
          std::unique_ptr<uint8_t> key_1(new uint8_t[size_1]);
          std::unique_ptr<uint8_t> key_2(new uint8_t[size_2]);
          EXPECT_SUCCESS(exp_obj.getValue(list_1[i], size_1, key_1.get()));
          EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], size_2, key_2.get()));
          for (uint32_t j = 0; j < size_1; j++) {
            EXPECT_EQ(key_1.get()[j], key_2.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected byte : " << static_cast<int>(key_1.get()[j])
                << " Received byte : " << static_cast<int>(key_2.get()[j])
                << "\n";
          }
        } else {
          uint64_t val_1, val_2;
          EXPECT_SUCCESS(exp_obj.getValue(list_1[i], &val_1));
          EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], &val_2));
          EXPECT_EQ(val_1, val_2)
              << "Field : " << list_1[i] << " Expected val : " << val_1
              << " Received val : " << val_2 << "\n";
        }
        break;
      }
      case KeyFieldType::TERNARY: {
        if (size_1 > 8) {
          std::unique_ptr<uint8_t> key_1(new uint8_t[size_1]);
          std::unique_ptr<uint8_t> mask_1(new uint8_t[size_1]);
          std::unique_ptr<uint8_t> key_2(new uint8_t[size_2]);
          std::unique_ptr<uint8_t> mask_2(new uint8_t[size_2]);
          EXPECT_SUCCESS(exp_obj.getValueandMask(
              list_1[i], size_1, key_1.get(), mask_1.get()));
          EXPECT_SUCCESS(rcv_obj.getValueandMask(
              list_2[i], size_2, key_2.get(), mask_2.get()));
          for (uint32_t j = 0; j < size_1; j++) {
            EXPECT_EQ(key_1.get()[j], key_2.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected val byte : " << static_cast<int>(key_1.get()[j])
                << " Received val byte : " << static_cast<int>(key_2.get()[j])
                << "\n";
            EXPECT_EQ(mask_1.get()[j], mask_2.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected mask byte : " << static_cast<int>(mask_1.get()[j])
                << " Received mask byte : " << static_cast<int>(mask_2.get()[j])
                << "\n";
          }
        } else {
          uint64_t key_1, key_2, mask_1, mask_2;
          EXPECT_SUCCESS(exp_obj.getValueandMask(list_1[i], &key_1, &mask_1));
          EXPECT_SUCCESS(rcv_obj.getValueandMask(list_2[i], &key_2, &mask_2));
          EXPECT_EQ(key_1, key_2)
              << "Field : " << list_1[i] << " Expected val : " << key_1
              << " Received val : " << key_2 << "\n";
          EXPECT_EQ(mask_1, mask_2)
              << "Field : " << list_1[i] << " Expected mask : " << mask_1
              << " Received mask : " << mask_2 << "\n";
        }
        break;
      }
      case KeyFieldType::RANGE: {
        if (size_1 > 8) {
          std::unique_ptr<uint8_t> key_1_start(new uint8_t[size_1]);
          std::unique_ptr<uint8_t> key_1_end(new uint8_t[size_1]);
          std::unique_ptr<uint8_t> key_2_start(new uint8_t[size_2]);
          std::unique_ptr<uint8_t> key_2_end(new uint8_t[size_2]);
          EXPECT_SUCCESS(exp_obj.getValueRange(
              list_1[i], size_1, key_1_start.get(), key_1_end.get()));
          EXPECT_SUCCESS(rcv_obj.getValueRange(
              list_2[i], size_2, key_2_start.get(), key_2_end.get()));
          for (uint32_t j = 0; j < size_1; j++) {
            EXPECT_EQ(key_1_start.get()[j], key_2_start.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected start byte : "
                << static_cast<int>(key_1_start.get()[j])
                << " Received start byte : "
                << static_cast<int>(key_2_start.get()[j]) << "\n";
            EXPECT_EQ(key_1_end.get()[j], key_2_end.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected end byte : "
                << static_cast<int>(key_1_end.get()[j])
                << " Received end byte : "
                << static_cast<int>(key_2_end.get()[j]) << "\n";
          }
        } else {
          uint64_t key_1_start, key_1_end, key_2_start, key_2_end;
          ;
          EXPECT_SUCCESS(
              exp_obj.getValueRange(list_1[i], &key_1_start, &key_1_end));
          EXPECT_SUCCESS(
              rcv_obj.getValueRange(list_2[i], &key_2_start, &key_2_end));
          EXPECT_EQ(key_1_start, key_2_start)
              << "Field : " << list_1[i]
              << " Expected start val : " << key_1_start
              << " Received start val : " << key_2_start << "\n";
          EXPECT_EQ(key_1_end, key_2_end)
              << "Field : " << list_1[i] << " Expected end val : " << key_1_end
              << " Received end val : " << key_2_end << "\n";
        }
        break;
      }
      case KeyFieldType::LPM: {
        if (size_1 > 8) {
          std::unique_ptr<uint8_t> key_1(new uint8_t[size_1]);
          uint16_t p_1_len;
          std::unique_ptr<uint8_t> key_2(new uint8_t[size_2]);
          uint16_t p_2_len;
          EXPECT_SUCCESS(
              exp_obj.getValueLpm(list_1[i], size_1, key_1.get(), &p_1_len));
          EXPECT_SUCCESS(
              rcv_obj.getValueLpm(list_2[i], size_2, key_2.get(), &p_2_len));
          for (uint32_t j = 0; j < size_1; j++) {
            EXPECT_EQ(key_1.get()[j], key_2.get()[j])
                << "Field : " << list_1[i] << " Byte Stream Index : " << j
                << " Expected byte : " << static_cast<int>(key_1.get()[j])
                << " Received byte : " << static_cast<int>(key_2.get()[j])
                << "\n";
          }
          EXPECT_EQ(p_1_len, p_2_len)
              << "Field : " << list_1[i] << " Expected prefix len : " << p_1_len
              << " Received prefix len : " << p_2_len << "\n";
        } else {
          uint64_t val_1, val_2;
          uint16_t p_1_len, p_2_len;
          EXPECT_SUCCESS(exp_obj.getValueLpm(list_1[i], &val_1, &p_1_len));
          EXPECT_SUCCESS(rcv_obj.getValueLpm(list_2[i], &val_2, &p_2_len));
          EXPECT_EQ(val_1, val_2)
              << "Field : " << list_1[i] << " Expected val : " << val_1
              << " Received val : " << val_2 << "\n";
          EXPECT_EQ(p_1_len, p_2_len)
              << "Field : " << list_1[i] << " Expected prefix len : " << p_1_len
              << " Received prefix len : " << p_2_len << "\n";
        }
        break;
      }
      case KeyFieldType::INVALID:
      default:
        EXPECT_EQ(1, 0) << "Field : " << list_1[i] << " Invalid key type : "
                        << static_cast<int>(key_type_1) << "\n";
    }
  }
}

void BfRtTableTest::compareDataObjects(const BfRtTableData &exp_obj,
                                       const BfRtTableData &rcv_obj) const {
  // Get and compare  action id from the table data objects
  bf_rt_id_t act_id_1 = 0, act_id_2 = 0;
  ASSERT_SUCCESS(exp_obj.actionIdGet(&act_id_1));
  ASSERT_SUCCESS(rcv_obj.actionIdGet(&act_id_2));

  // We need to have an ASSERT_EQ instead of EXPECT_EQ because if the action
  // ids don't match up then obviously the data fields are not going to match
  // up
  ASSERT_EQ(act_id_1, act_id_2) << "Expected action id : " << act_id_1
                                << " Received action id : " << act_id_2 << "\n";

  // Once the action ids are equal, get the parent table pointer
  const BfRtTable *table_1 = nullptr, *table_2 = nullptr;
  ASSERT_SUCCESS(exp_obj.getParent(&table_1));
  ASSERT_SUCCESS(rcv_obj.getParent(&table_2));
  // ???
  // The parent tables have to be equal (is it ok if I just compare the table
  // pointers here?)
  // ???
  ASSERT_EQ(table_1, table_2);

  // Get the data field id vectors
  std::vector<bf_rt_id_t> list_1, list_2;
  if (act_id_1) {
    ASSERT_SUCCESS(table_1->dataFieldIdListGet(act_id_1, &list_1));
    ASSERT_SUCCESS(table_2->dataFieldIdListGet(act_id_2, &list_2));
  } else {
    ASSERT_SUCCESS(table_1->dataFieldIdListGet(&list_1));
    ASSERT_SUCCESS(table_2->dataFieldIdListGet(&list_2));
  }

  // If a table has idle timeout enabled, currently we are only checking the
  // notify mode of it. Hence iterate over all the random_data objects and
  // remove any fields which are $ENTRY_HIT_STATE
  std::string field_name;
  auto iter2 = list_2.begin();
  for (auto iter1 = list_1.begin();
       iter1 != list_1.end(), iter2 != list_2.end();
       iter1++, iter2++) {
    if (act_id_1) {
      EXPECT_SUCCESS(table_1->dataFieldNameGet(*iter1, act_id_1, &field_name));
    } else {
      EXPECT_SUCCESS(table_1->dataFieldNameGet(*iter1, &field_name));
    }
    if (field_name == "$ENTRY_HIT_STATE") {
      list_1.erase(iter1);
      list_2.erase(iter2);
      break;
    }
  }

  // Now compare the data field values
  ASSERT_EQ(list_1.size(), list_2.size())
      << "Expected field id vec size : " << list_1.size()
      << " Received field id vec size : " << list_2.size() << "\n";
  for (uint32_t i = 0; i < list_1.size(); i++) {
    // ???
    // Since we are quering the same table for the field id list, they ought
    // to be in the same order in both the lists. If they are not, that
    // indicates an error? That's why we can simply iterate over the lists
    // and expect the field is to be in the exact same order
    // ???
    ASSERT_EQ(list_1[i], list_2[i])
        << "Expected Field id : " << list_1[i]
        << " Received Field id : " << list_2[i] << "\n";

    // Get the type of the field
    DataType data_type_1, data_type_2;
    if (act_id_1) {
      ASSERT_SUCCESS(
          table_1->dataFieldDataTypeGet(list_1[i], act_id_1, &data_type_1));
      ASSERT_SUCCESS(
          table_2->dataFieldDataTypeGet(list_2[i], act_id_2, &data_type_2));
    } else {
      ASSERT_SUCCESS(table_1->dataFieldDataTypeGet(list_1[i], &data_type_1));
      ASSERT_SUCCESS(table_2->dataFieldDataTypeGet(list_2[i], &data_type_2));
    }
    ASSERT_EQ(data_type_1, data_type_2)
        << "Field : " << list_1[i]
        << " Expected Data type : " << static_cast<int>(data_type_1)
        << " Received Data type : " << static_cast<int>(data_type_2) << "\n";

    // Finally compare the values
    switch (data_type_1) {
      case DataType::INT_ARR: {
        std::vector<bf_rt_id_t> val_1, val_2;
        EXPECT_SUCCESS(exp_obj.getValue(list_1[i], &val_1));
        EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], &val_2));
        ASSERT_EQ(val_1.size(), val_2.size())
            << "Field : " << list_1[i]
            << " Expected Int Array size : " << val_1.size()
            << " Received Int Array size : " << val_2.size() << "\n";
        for (uint32_t j = 0; j < val_1.size(); j++) {
          EXPECT_EQ(val_1[j], val_2[j])
              << "Field : " << list_1[i] << " Int array index : " << j
              << " Expected Int : " << val_1[j]
              << " Received Int : " << val_2[j] << "\n";
        }
        break;
      }
      case DataType::BOOL_ARR: {
        std::vector<bool> val_1, val_2;
        EXPECT_SUCCESS(exp_obj.getValue(list_1[i], &val_1));
        EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], &val_2));
        ASSERT_EQ(val_1.size(), val_2.size())
            << "Field : " << list_1[i]
            << " Expected Bool Array size : " << val_1.size()
            << " Received Bool Array size : " << val_2.size() << "\n";
        for (uint32_t j = 0; j < val_1.size(); j++) {
          EXPECT_EQ(val_1[j], val_2[j])
              << "Field : " << list_1[i] << " Bool array index : " << j
              << " Expected bool : " << val_1[j]
              << " Received bool : " << val_2[j] << "\n";
        }
        break;
      }
      case DataType::UINT64: {
        uint64_t val_1, val_2;
        EXPECT_SUCCESS(exp_obj.getValue(list_1[i], &val_1));
        EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], &val_2));
        EXPECT_EQ(val_1, val_2)
            << "Field : " << list_1[i] << " Expected val : " << val_1
            << " Received val : " << val_2 << "\n";
        break;
      }
      case DataType::BYTE_STREAM: {
        size_t size_1, size_2;
        if (act_id_1) {
          EXPECT_SUCCESS(
              table_1->dataFieldSizeGet(list_1[i], act_id_1, &size_1));
          EXPECT_SUCCESS(
              table_2->dataFieldSizeGet(list_2[i], act_id_2, &size_2));
        } else {
          EXPECT_SUCCESS(table_1->dataFieldSizeGet(list_1[i], &size_1));
          EXPECT_SUCCESS(table_2->dataFieldSizeGet(list_2[i], &size_2));
        }
        ASSERT_EQ(size_1, size_2) << "Field : " << list_1[i]
                                  << " Byte Stream Expected size : " << size_1
                                  << " Received size : " << size_2 << "\n";
        size_1 = (size_1 + 7) / 8;
        size_2 = (size_2 + 7) / 8;
        std::unique_ptr<uint8_t> data_1(new uint8_t[size_1]);
        std::unique_ptr<uint8_t> data_2(new uint8_t[size_2]);
        EXPECT_SUCCESS(exp_obj.getValue(list_1[i], size_1, data_1.get()));
        EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], size_2, data_2.get()));
        for (uint32_t j = 0; j < size_1; j++) {
          EXPECT_EQ(data_1.get()[j], data_2.get()[j])
              << "Field : " << list_1[i] << " Byte Stream Index : " << j
              << " Expected byte : " << static_cast<int>(data_1.get()[j])
              << " Received byte : " << static_cast<int>(data_2.get()[j])
              << "\n";
        }
        break;
      }
      case DataType::FLOAT: {
        float val_1, val_2;
        EXPECT_SUCCESS(exp_obj.getValue(list_1[i], &val_1));
        EXPECT_SUCCESS(rcv_obj.getValue(list_2[i], &val_2));
        EXPECT_EQ(val_1, val_2)
            << "Field : " << list_1[i] << " Expected val : " << val_1
            << " Received val : " << val_2 << "\n";
        break;
      }
      case DataType::STRING:
        // FIXME
        std::cout
            << "Expectations for DataType String not set. Please set them "
               "and proceed. Asserting until then\n";
        BF_RT_ASSERT(0);
        break;
      default:
        EXPECT_EQ(1, 0) << "Field : " << list_1[i] << " Invalid Data type : "
                        << static_cast<int>(data_type_1) << "\n";
    }
  }
}

/*
ACTION_P4(AddEntryToDatabase, DataBaseObjPointer, func, entry_hdl, entry) {
  (DataBaseObjPointer->*func)(entry_hdl, entry);
}*/

void BfRtMatchActionTableTest::initialTestSetup(
    const BfRtTable &table,
    const BfRtEntryGen &entry_generator,
    const uint32_t &num_entries) {
  // Call Base's initial setup
  BfRtTableTest::initialTestSetup(table, entry_generator, num_entries);

  // Get the randomly generator key and data objects
  tableRandomKeyGet(table, entry_generator, num_entries, &random_keys);
  tableRandomDataGet(table, entry_generator, num_entries, &random_data);

  // Form the 'ground truth' copies of the match and action spec to compare
  tableExpectedMatchSpecGet(table, random_keys, &exp_ms_vec);
  tableExpectedActionSpecGet(table, random_data, &exp_as_vec);
}

// This function iterates over all the fields in the single random data
// and returns the value of the ttl field if it exists
void BfRtMatchActionTableTest::getTTL(const BfRtTable &table,
                                      GenericData &single_random_data,
                                      uint64_t *ttl_val) {
  const BfRtTableDataField *table_data_field = nullptr;
  *ttl_val = 0;
  for (const auto &field_id : single_random_data.fieldIdListGet()) {
    const auto &generic_data_field = single_random_data.dataFieldGet(field_id);
    if (single_random_data.actionIdGet()) {
      ASSERT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
          field_id, single_random_data.actionIdGet(), &table_data_field));
    } else {
      ASSERT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
          field_id, &table_data_field));
    }

    auto types_vec = table_data_field->getTypes();
    for (const auto &field_type : types_vec) {
      if (field_type == DataFieldType::TTL) {
        // Generate a value between MIN and MAX TTL
        single_random_data.regenerateTTL(field_id, MIN_TTL, MAX_TTL);
        generic_data_field.getValue(ttl_val);
        return;
      }
    }
  }
}

// This function adds entries in the table
bf_status_t BfRtMatchActionTableTest::tableEntryAdd(
    const uint32_t &num_entries,
    const BfRtTable &table,
    std::map<pipe_ent_hdl_t, int> *entry_hdls) {
  // - Allocate 'num_entries' number of key and data objects
  // - Get 'num_entries' number of randomly generated key and data objects
  //   from the Generator
  // - For all the key and data objects, iterate over all the key and data
  //   fields respectively and set their values
  // - For all the key and data objects, form the 'ground truth' copy of the
  //   match and action spec
  // - Create match and action spec matchers using the 'ground_truth' copies
  // - Set expectations on pipeMgrMatEntAdd

  std::string table_name;
  table.tableNameGet(&table_name);
  std::unique_ptr<BfRtTableKey> table_key;

  // Do one keyAllocate and dataAllocate and reset across uses
  auto sts = table.keyAllocate(&table_key);
  EXPECT_EQ(sts, BF_SUCCESS)
      << "Key Allocate failed for table : " << table_name.c_str()
      << " Expected status : " << 0 << " Received status : " << sts << "\n";
  BF_RT_ASSERT(sts == BF_SUCCESS);

  // Data Allocate is done without action-ID here
  std::unique_ptr<BfRtTableData> table_data;
  sts = table.dataAllocate(&table_data);
  EXPECT_EQ(sts, BF_SUCCESS)
      << "Data Allocate failed for table : " << table_name.c_str()
      << " Expected status : " << 0 << " Received status : " << sts << "\n";
  const BfRtTableObj &table_obj = static_cast<const BfRtTableObj &>(table);

  for (uint32_t n = 0; n < num_entries; n++) {
    pipe_ent_hdl_t ent_hdl = n + 1;
    // Create match and action spec matchers
    auto ms_matcher = BfRtMatchersFactory::makeMatchSpecMatcher(
        exp_ms_vec[n]->getPipeMatchSpec());
    const pipe_action_spec_t *as = exp_as_vec[n]->getPipeActionSpec();
    auto as_matcher = BfRtMatchersFactory::makeActionSpecMatcher(as);

    // Get ttl if applicable
    uint64_t ttl_val = 0;
    auto &single_random_data = random_data[n];
    getTTL(table, single_random_data, &ttl_val);

    // Get Key object from random key
    formTableKey_from_idx(table, n, table_key.get());

    // Get data object from random data
    formTableData_from_idx(table, n, table_data.get());

    bool action_default_only = checkDefaultOnly(table, *table_data);
    // Set Expectation on pipe mgr entry add only if the action is default only
    // If the action is default only, then we expect tableEntryAdd to return
    // before calling pipeMgrMatEntAdd
    bool is_const_table = false;
    table.tableIsConst(&is_const_table);

    if (!action_default_only && !is_const_table) {
      EXPECT_CALL(
          *pipe_mgr_obj,
          pipeMgrMatEntAdd(_,
                           _,
                           _,
                           ms_matcher,
                           static_cast<const BfRtTableObj &>(table).getActFnHdl(
                               single_random_data.actionIdGet()),
                           as_matcher,
                           ttl_val,
                           _,
                           _))
          .Times(1)
          .WillOnce(InvokeWithoutArgs([&]() {
            Entry mt(exp_ms_vec[n]->getPipeMatchSpec(),
                     exp_as_vec[n]->getPipeActionSpec(),
                     static_cast<const BfRtTableObj &>(table).getActFnHdl(
                         single_random_data.actionIdGet()),
                     ttl_val,
                     &table);
            pipe_mgr_obj->getMockIPipeMgrIntfHelper().addEntryToDatabase(
                ent_hdl, &mt);
            return PIPE_SUCCESS;
          }))
          .RetiresOnSaturation();
    }

    // Add entry in the table
    sts = table.tableEntryAdd(
        getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data);
    if (!action_default_only && !is_const_table) {
      EXPECT_EQ(sts, BF_SUCCESS)
          << "Table entry add failed for entry : " << n
          << " For table : " << table_name << " Expected status : " << 0
          << " Received status : " << sts << "\n";
      if (sts == BF_SUCCESS) {
        // Add the enrtry handle only in case of success
        if (entry_hdls->find(ent_hdl) != entry_hdls->end()) {
          // This indicates that we are trying to add the same entry handle
          // twice.
          // This indicates a bug in the unit test logic
          BF_RT_ASSERT(0);
        }
        entry_hdls->insert(std::make_pair(ent_hdl, n));
      }
    } else {
      EXPECT_EQ(sts, BF_INVALID_ARG)
          << "Table entry add succeeded when we "
             "expected it to fail for entry : "
          << n << " Expected status : " << BF_INVALID_ARG
          << " Received status : " << sts << "\n";
    }
  }
  return BF_SUCCESS;
}

// This API sets expectations on the low level pipe mgr APIs
void BfRtMatchActionTableTest::tableEntryModSetExpForField(
    const BfRtTable &table,
    const pipe_ent_hdl_t &ent_hdl,
    const int &index,
    const bool &direct_resource_found,
    const bool &action_spec_found,
    const bool &ttl_found,
    const bool &direct_counter_found,
    const uint64_t &ttl_value_ground_truth) {
  const auto &single_random_data = random_data[index];
  if (direct_resource_found) {
    // Get the pointer to the resource spec array from the ground truth
    // action spec
    auto *as = exp_as_vec[index].get();
    pipe_res_spec_t *resource_spec = &as->getPipeActionSpec()->resources[0];
    const int resource_count = as->getPipeActionSpec()->resource_count;
    // Form the resource spec matcher
    auto rs_matcher = BfRtMatchersFactory::makeResourceSpecArrayMatcher(
        resource_spec, resource_count);
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrMatEntSetResource(
                    _, _, _, ent_hdl, rs_matcher, resource_count, _))
        .Times(1)
        .RetiresOnSaturation();
  } else {
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatEntSetResource(_, _, _, _, _, _, _))
        .Times(0);
  }
  if (action_spec_found) {
    pipe_action_spec_t *as = exp_as_vec[index]->getPipeActionSpec();
    auto as_matcher = BfRtMatchersFactory::makeActionSpecMatcher(as);
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrMatEntSetAction(
                    _,
                    _,
                    _,
                    ent_hdl,
                    static_cast<const BfRtTableObj &>(table).getActFnHdl(
                        single_random_data.actionIdGet()),
                    as_matcher,
                    _))
        .Times(1)
        .RetiresOnSaturation();
  } else {
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatEntSetAction(_, _, _, _, _, _, _))
        .Times(0);
  }
  if (ttl_found) {
    EXPECT_CALL(
        *pipe_mgr_obj,
        pipeMgrMatEntSetIdleTtl(_, _, _, ent_hdl, ttl_value_ground_truth, _, _))
        .Times(1)
        .RetiresOnSaturation();
  } else {
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatEntSetIdleTtl(_, _, _, _, _, _, _))
        .Times(0);
  }
  if (direct_counter_found) {
    // Get the counter table handle which this match table refers to
    // The DataFieldType is hardcoded in the following API call as we just
    // want to get hold of the counter table reference by this match table.
    // It could very well have been DataFieldType::COUNTER_SPEC_PACKETS
    pipe_tbl_hdl_t counter_tbl_hdl =
        static_cast<const BfRtTableObj &>(table).getResourceHdl(
            DataFieldType::COUNTER_SPEC_BYTES);
    // Once we have the counter table hdl, iterate over the resource spec
    // array in the action spec and get hold of the counter spec
    pipe_action_spec_t *as = exp_as_vec[index]->getPipeActionSpec();
    pipe_stat_data_t *counter_spec = nullptr;
    for (int i = 0; i < as->resource_count; i++) {
      if (as->resources[i].tbl_hdl == counter_tbl_hdl) {
        counter_spec = &as->resources[i].data.counter;
      }
    }
    if (counter_spec == nullptr) {
      // Indicates that we have a field which is of type counter spec but
      // dont have a corresponding counter table handle in our "ground truth"
      // pipe action spec. This indicates an error (mostly in the ground
      // truth action spec generation)
      BF_RT_ASSERT(0);
    }
    // Once we have the counter spec, form the counter spec matcher
    auto cs_matcher = BfRtMatchersFactory::makeCounterSpecMatcher(counter_spec);
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrMatEntDirectStatSet(_, _, _, ent_hdl, cs_matcher))
        .Times(1)
        .RetiresOnSaturation();
  } else {
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatEntDirectStatSet(_, _, _, _, _))
        .Times(0);
  }
}

// Given a field id, this function calculates the final set of pipe mgr APIs
// which need to have positive and negative expectations
void BfRtMatchActionTableTest::tableEntryModIterateFieldsSetExpForFieldId(
    const BfRtTable &table,
    const pipe_ent_hdl_t &ent_hdl,
    const int &index,
    const bf_rt_id_t &field_id,
    const BfRtTableData *table_data_mod) {
  std::set<DataFieldType> field_types;
  const auto &single_random_data = random_data[index];
  if (single_random_data.actionIdGet()) {
    ASSERT_SUCCESS(static_cast<const BfRtTableObj &>(table).dataFieldTypeGet(
        field_id, single_random_data.actionIdGet(), &field_types));
  } else {
    ASSERT_SUCCESS(static_cast<const BfRtTableObj &>(table).dataFieldTypeGet(
        field_id, &field_types));
  }

  const auto &generic_data_field = single_random_data.dataFieldGet(field_id);
  bool direct_resource_found = false;
  bool action_spec_found = false;
  bool ttl_found = false;
  bool direct_counter_found = false;
  uint64_t ttl_value_ground_truth;
  auto field_destination =
      BfRtTableDataField::getDataFieldDestination(field_types);
  switch (field_destination) {
    case fieldDestination::DIRECT_LPF:
    case fieldDestination::DIRECT_METER:
    case fieldDestination::DIRECT_WRED:
    case fieldDestination::DIRECT_REGISTER: {
      direct_resource_found = true;
      break;
    }
    case fieldDestination::ACTION_SPEC: {
      action_spec_found = true;
      break;
    }
    case fieldDestination::TTL: {
      generic_data_field.getValue(&ttl_value_ground_truth);
      ttl_found = true;
      break;
    }
    case fieldDestination::DIRECT_COUNTER: {
      direct_counter_found = true;
      break;
    }
    default:
      BF_RT_ASSERT(0);
  }
  bf_rt_id_t action_id = 0;
  auto bf_status = table_data_mod->actionIdGet(&action_id);
  ASSERT_EQ(bf_status, BF_SUCCESS);
  if (action_id) {
    // Indicates that we have an action with no action params (no-op). Even
    // then we need to progrma this action to the lower level pipe mgr.
    // Hence set action_spec_found to true
    action_spec_found = true;
  }

  if (action_spec_found) {
    // Pipe-mgr exposes different APIs to modify different parts of the data
    // 1. To modify any part of the action spec, pipe_mgr_mat_ent_set_action
    // is
    // the API to use
    //    As part of this following direct resources can be modified
    //      a. LPF
    //      b. WRED
    //      c. METER
    //      d. REGISTER
    // 2. So, if any of the data fields that are to be modified is part of the
    // action spec
    //    the above mentioned direct resources get a free ride.
    // 3. If there are no action parameters to be modified, the resources need
    // to
    // be modified using
    //     the set_resource API.
    // 4. For direct counter resource, pipe_mgr_mat_ent_direct_stat_set is the
    // API
    // to be used.
    // 5. For modifying TTL, a separate API to set the ttl is used.
    direct_resource_found = false;
  }
  tableEntryModSetExpForField(table,
                              ent_hdl,
                              index,
                              direct_resource_found,
                              action_spec_found,
                              ttl_found,
                              direct_counter_found,
                              ttl_value_ground_truth);
}

// This function iterates over all the fields of every entry and modifies the
// fields one at a time and then sets necessary expectations on the low level
// pipe mgr APIs
bf_status_t BfRtMatchActionTableTest::tableEntryModIterateFields(
    const BfRtTable &table, const std::map<pipe_ent_hdl_t, int> &entry_hdls) {
  // For every entry added in the table, change every field in the entry data
  // one by one and verify

  // Do one keyAllocate and dataAllocate and reset across uses
  std::unique_ptr<BfRtTableKey> table_key_mod;
  EXPECT_SUCCESS(table.keyAllocate(&table_key_mod));

  std::unique_ptr<BfRtTableData> table_data_mod;
  EXPECT_SUCCESS(table.dataAllocate(&table_data_mod));

  for (const auto &iter : entry_hdls) {
    pipe_ent_hdl_t ent_hdl = iter.first;
    int n = iter.second;

    auto &single_random_data = random_data[n];

    // Today bf-rt and the lower level pipe-mgr does not support only one
    // field from a group of fields of the same type to be modified alone.
    // What this means is that, if we have a counter spec with bytes and
    // packets and if allocate data with the field id of just the bytes,
    // we cant expect the value for packets to remain as it was. Thus, to test
    // modifies, we will first group fields in the data objects into different
    // categories (action param, counter spec, etc.). If a data has action
    // params and a counter spec, then we need to do a data allocate with all
    // fields which are action params then modify them one by one. With this,
    // we can expect the counter spec to remain unchanged
    std::map<fieldDestination, std::vector<bf_rt_id_t>> destination_types;
    for (const auto &field_id : single_random_data.fieldIdListGet()) {
      const BfRtTableDataField *table_data_field = nullptr;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
            field_id, single_random_data.actionIdGet(), &table_data_field));
      } else {
        EXPECT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
            field_id, &table_data_field));
      }
      auto field_types = table_data_field->getTypes();
      fieldDestination field_destination =
          BfRtTableDataField::getDataFieldDestination(field_types);
      destination_types[field_destination].push_back(field_id);
    }

    // Form key object from random key
    formTableKey_from_idx(table, n, table_key_mod.get());

    // Now iterate over all the destination types and modify each field within
    // the type one by one
    for (const auto &it : destination_types) {
      // Here we are modifying the fields one by one.
      // The data object needs to be reset with the action_id only if
      // action_spec
      // needs to be changed. Else it needs to be reset with the non-action_id
      // API version
      bool is_action_param = it.first == bfrt::fieldDestination::ACTION_SPEC;
      if (single_random_data.actionIdGet() && is_action_param) {
        EXPECT_SUCCESS(table.dataReset(
            it.second, single_random_data.actionIdGet(), table_data_mod.get()));
      } else {
        EXPECT_SUCCESS(table.dataReset(it.second, table_data_mod.get()));
      }
      // Now set all the fields in this group in the table data object
      tableDataSetHelper(
          table, single_random_data, *table_data_mod.get(), it.second);

      // Now iterate and modify every field
      for (const auto &field_id : it.second) {
        const BfRtTableDataField *table_data_field = nullptr;
        if (single_random_data.actionIdGet()) {
          EXPECT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
              field_id, single_random_data.actionIdGet(), &table_data_field));
        } else {
          EXPECT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
              field_id, &table_data_field));
        }
        auto field_types = table_data_field->getTypes();
        // Call the random data API to modify the value of this particular
        // field only
        if (*field_types.begin() == DataFieldType::TTL) {
          single_random_data.regenerateTTL(field_id, MIN_TTL, MAX_TTL);
        } else {
          single_random_data.regenerate(field_id);
        }
        const auto generic_data_field =
            single_random_data.dataFieldGet(field_id);
        // Now update the table data object to reflect the change in the
        // field val in the random data object
        tableDataSetHelper(
            table, single_random_data, *table_data_mod.get(), {field_id});
        // Form the new expected action spec
        ActionSpec *mod_as = new ActionSpec(single_random_data, it.second);
        // Assign this to the unique pointer so that its accessible in the
        // subroutines
        // that this function calls
        exp_as_vec[n].reset(mod_as);

        // Now we need to identify the type of the field id and set expectation
        // only on the API corresponding to that type. In addition, we need to
        // make sure that no other API gets invoked
        tableEntryModIterateFieldsSetExpForFieldId(
            table, ent_hdl, n, field_id, table_data_mod.get());

        // After setting expectation on the relevant pipe mgr modify API, set
        // an expectation on the entry handle function
        EXPECT_CALL(*pipe_mgr_obj,
                    pipeMgrMatchSpecToEntHdl(_, _, _, _, _, false))
            .Times(1)
            .WillOnce(WithArgs<4>(Invoke([&](pipe_mat_ent_hdl_t *mat_ent_hdl) {
              *mat_ent_hdl = ent_hdl;
              return PIPE_SUCCESS;
            })))
            .RetiresOnSaturation();

        // Finally call the bfrt table entry modify API
        EXPECT_SUCCESS(table.tableEntryMod(getDefaultSession(),
                                           getDefaultBfRtTarget(),
                                           *table_key_mod,
                                           *table_data_mod));
      }
    }
  }

  return BF_SUCCESS;
}

void BfRtTableTest::formTableKey_from_idx(const BfRtTable &table,
                                          const int &index,
                                          BfRtTableKey *table_key) {
  BF_RT_ASSERT(static_cast<uint32_t>(index) < random_keys.size());
  auto &single_random_key = random_keys[index];
  ASSERT_SUCCESS(table.keyReset(table_key));

  tableKeySetHelper(single_random_key, *table_key);
  return;
}

void BfRtTableTest::formTableData_from_idx(const BfRtTable &table,
                                           const int &index,
                                           BfRtTableData *table_data) {
  BF_RT_ASSERT(static_cast<uint32_t>(index) < random_data.size());
  auto &single_random_data = random_data[index];

  auto action_id = single_random_data.actionIdGet();

  if (action_id) {
    ASSERT_SUCCESS(table.dataReset(action_id, table_data));
  } else {
    ASSERT_SUCCESS(table.dataReset(table_data));
  }
  tableDataSetHelper(table, single_random_data, *table_data);
  return;
}

// This function swaps data objects between the pre-added entries in the table
// The swapping pattern is as follows:
// 0 <-> (n-1), 1 <-> (n-2), ....
bf_status_t BfRtMatchActionTableTest::tableEntryModSwapActionData(
    const uint32_t &num_entries,
    const BfRtTable &table,
    const std::map<pipe_ent_hdl_t, int> &entry_hdls) {
  // In this, we need to iterate over all the fields in the random data object
  // that is going to be assigned to a particular entry. Depending on all the
  // different types of fields that are there in the random data object,
  // we need to set expectations on the relevant APIs and then call the table
  // entry modify function

  // Do one keyAllocate and dataAllocate
  std::unique_ptr<BfRtTableKey> table_key_mod;
  EXPECT_SUCCESS(table.keyAllocate(&table_key_mod));

  std::unique_ptr<BfRtTableData> table_data_mod;
  EXPECT_SUCCESS(table.dataAllocate(&table_data_mod));

  for (auto iter = entry_hdls.begin(); iter != entry_hdls.end(); iter++) {
    const pipe_ent_hdl_t ent_hdl = iter->first;
    int index = num_entries - 1 - iter->second;
    // Iterate over all the fields in the random data object and figure out
    // all the relevant field destinations
    bool direct_resource_found = false;
    bool action_spec_found = false;
    bool ttl_found = false;
    bool direct_counter_found = false;
    const auto &single_random_data = random_data[index];
    uint64_t ttl_value_ground_truth;
    for (const auto &field_id : single_random_data.fieldIdListGet()) {
      std::set<DataFieldType> field_types;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(
            static_cast<const BfRtTableObj &>(table).dataFieldTypeGet(
                field_id, single_random_data.actionIdGet(), &field_types));
      } else {
        EXPECT_SUCCESS(
            static_cast<const BfRtTableObj &>(table).dataFieldTypeGet(
                field_id, &field_types));
      }
      auto field_destination =
          BfRtTableDataField::getDataFieldDestination(field_types);
      const auto &generic_data_field =
          single_random_data.dataFieldGet(field_id);
      switch (field_destination) {
        case fieldDestination::DIRECT_LPF:
        case fieldDestination::DIRECT_METER:
        case fieldDestination::DIRECT_WRED:
        case fieldDestination::DIRECT_REGISTER:
          direct_resource_found = true;
          break;
        case fieldDestination::ACTION_SPEC:
          action_spec_found = true;
          break;
        case fieldDestination::TTL:
          generic_data_field.getValue(&ttl_value_ground_truth);
          ttl_found = true;
          break;
        case fieldDestination::DIRECT_COUNTER:
          direct_counter_found = true;
          break;
        default:
          BF_RT_ASSERT(0);
      }
    }

    int key_idx = iter->second;
    int data_idx = index;

    formTableKey_from_idx(table, key_idx, table_key_mod.get());
    formTableData_from_idx(table, data_idx, table_data_mod.get());

    bf_rt_id_t action_id = 0;
    auto bf_status = table_data_mod->actionIdGet(&action_id);
    if (bf_status != BF_SUCCESS) {
      return bf_status;
    }
    if (action_id) {
      // Indicates that we have an action with no action params (no-op). Even
      // then we need to progrma this action to the lower level pipe mgr.
      // Hence set action_spec_found to true
      action_spec_found = true;
    }

    if (action_spec_found) {
      // Pipe-mgr exposes different APIs to modify different parts of the data
      // 1. To modify any part of the action spec, pipe_mgr_mat_ent_set_action
      // is
      // the API to use
      //    As part of this following direct resources can be modified
      //      a. LPF
      //      b. WRED
      //      c. METER
      //      d. REGISTER
      // 2. So, if any of the data fields that are to be modified is part of the
      // action spec
      //    the above mentioned direct resources get a free ride.
      // 3. If there are no action parameters to be modified, the resources need
      // to
      // be modified using
      //     the set_resource API.
      // 4. For direct counter resource, pipe_mgr_mat_ent_direct_stat_set is the
      // API
      // to be used.
      // 5. For modifying TTL, a separate API to set the ttl is used.
      direct_resource_found = false;
    }

    // Now set expectations for all the relevant field_destinations
    tableEntryModSetExpForField(table,
                                ent_hdl,
                                index,
                                direct_resource_found,
                                action_spec_found,
                                ttl_found,
                                direct_counter_found,
                                ttl_value_ground_truth);

    // After setting expectation on the relevant pipe mgr modify API, set
    // an expectation on the entry handle function
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatchSpecToEntHdl(_, _, _, _, _, false))
        .Times(1)
        .WillOnce(WithArgs<4>(Invoke([&](pipe_mat_ent_hdl_t *mat_ent_hdl) {
          *mat_ent_hdl = ent_hdl;
          return PIPE_SUCCESS;
        })))
        .RetiresOnSaturation();

    // Finally call the bfrt table entry modify API
    EXPECT_SUCCESS(table.tableEntryMod(getDefaultSession(),
                                       getDefaultBfRtTarget(),
                                       *table_key_mod,
                                       *table_data_mod));
  }
  return BF_SUCCESS;
}

// Given a entry, set expectations on the appropriate backend pipe mgr APIs
void BfRtMatchActionTableTest::tableEntryGetSetExpForEntry(
    const BfRtTable &table,
    const GenericData &single_random_data,
    const int &times) {
  // BF-RT calls different APIs on pipe mgr backend depending on the types
  // of fields present in the data object. Thus, to set proper expectations
  // categorize all the present fields into a known set of destinations
  // and set expectations accordingly
  std::set<fieldDestination> destination_types;
  for (const auto &field_id : single_random_data.fieldIdListGet()) {
    const BfRtTableDataField *table_data_field = nullptr;
    EXPECT_SUCCESS(static_cast<const BfRtTableObj &>(table).getDataField(
        field_id, single_random_data.actionIdGet(), &table_data_field));
    auto field_types = table_data_field->getTypes();
    fieldDestination field_destination =
        BfRtTableDataField::getDataFieldDestination(field_types);
    destination_types.insert(field_destination);
  }

  // Currently BF-RT allows 2 types of data allocates.
  // 1. One way to allocate data is without mentioning the action id. If the
  //    user wants to use this, then he/she cannot mention a partial list
  //    of field ids. It either has to be an exhaustive list (containing all
  //    the common data fields and the action params if any; the user will
  //    need to know in this case what action is already programmed in the
  //    device, because how else will the user be able to specify the action
  //    params of that action id) OR the user can proceed without mentioning
  //    any field id list. The UT takes the later approach.
  // 2. The second way to allocate data is by mentioning the action id. In
  //    this case, the user has the liberty to provide either a partial list
  //    or an exhaustive list OR no list at all. The necessary actions will be
  //    taken only on the fields provided in the list (if the list is
  //    provided) or on all the fiels if the list is not provided.

  // Since we are allocating table data without providing a list or an action
  // id, we expect BF-RT to always read the action spec from pipe mgr. Thus
  // set an expectation invariably
  if (times == 1) {
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetEntry(_, _, _, _, _, _, _, _, _, _))
        .Times(times)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrGetEntry))
        .RetiresOnSaturation();
  } else {
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrGetNextEntries(_, _, _, _, _, _, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrGetNextEntries))
        .RetiresOnSaturation();
  }

  // pipeMgrGetEntry now take care of all possible table resources and fields
  // there is nothing else to do here.
}

// Test cases for Match Action Table
/*
 * --- Completed
 * - Simple table entry add with unique match specs
 * - Simple table entry get (reading each entry one by one)
 * - Simple table entry iterator (reading exactly as many entries as present in
 *the table)
 *
 * --- TODO Planned
 * - Try adding entry with the same match spec ?
 * - Try reading more entries than actually present on the table by using the
 *iterator
 * - Simple table entry delete
 * - Simple table entry modify
 */

TEST_P(BfRtMatchActionTableTest, EntryAdd) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryAdd");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));
  }
}

TEST_P(BfRtMatchActionTableTest, EntryGet) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryGet");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Do one key and data allocate and reset them across uses
    std::unique_ptr<BfRtTableKey> table_key;
    EXPECT_SUCCESS(table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    EXPECT_SUCCESS(table.dataAllocate(&table_data));

    // Now iterate over the ordered_set and read each entry
    for (auto iter = entry_hdl_list.begin(); iter != entry_hdl_list.end();
         iter++) {
      const auto &ent_hdl = iter->first;
      int m = iter->second;

      auto &single_random_data = random_data[m];

      // Form key object from random_key
      formTableKey_from_idx(table, m, table_key.get());
      ASSERT_SUCCESS(table.dataReset(table_data.get()));

      std::unique_ptr<BfRtTableData> table_data_expected;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(table.dataAllocate(single_random_data.actionIdGet(),
                                          &table_data_expected));
      } else {
        EXPECT_SUCCESS(table.dataAllocate(&table_data_expected));
      }
      tableDataSetHelper(table, single_random_data, *table_data_expected);

      EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatchSpecToEntHdl(_, _, _, _, _, true))
          .Times(1)
          .WillOnce(WithArgs<4>(Invoke([&](pipe_mat_ent_hdl_t *mat_ent_hdl) {
            *mat_ent_hdl = ent_hdl;
            return PIPE_SUCCESS;
          })))
          .RetiresOnSaturation();

      // Set correct expectations on the relevant pipe mgr APIs
      tableEntryGetSetExpForEntry(table, single_random_data, 1);

      EXPECT_SUCCESS(table.tableEntryGet(getDefaultSession(),
                                         getDefaultBfRtTarget(),
                                         *table_key,
                                         getDefaultTableReadFlag(),
                                         table_data.get()));

      compareDataObjects(*table_data_expected, *table_data);
    }
  }
}

TEST_P(BfRtMatchActionTableTest, EntryGetIterator) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    const auto &table = *d_tables[iter];
    auto &entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    table,
                                    entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryGetIterator");

    // Do one keyAllocate and dataAllocate to compare with what is received from
    // the table iterator. Reset across use.
    std::unique_ptr<BfRtTableKey> table_key_expected;
    EXPECT_SUCCESS(table.keyAllocate(&table_key_expected));

    std::unique_ptr<BfRtTableData> table_data_expected;
    EXPECT_SUCCESS(table.dataAllocate(&table_data_expected));

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    if (entry_hdl_list.size() == 0) continue;

    // Now read the first and subsequent 'N' entries
    auto first_entry_hdl = entry_hdl_list.begin()->first;
    int n = entry_hdl_list.begin()->second;
    std::unique_ptr<BfRtTableKey> first_key;
    EXPECT_SUCCESS(table.keyAllocate(&first_key));
    std::unique_ptr<BfRtTableData> first_data;
    EXPECT_SUCCESS(table.dataAllocate(&first_data));
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetFirstEntryHandle(_, _, _, _))
        .Times(1)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrGetFirstEntryHandle))
        .RetiresOnSaturation();

    auto &single_random_key = random_keys[n];
    auto &single_random_data = random_data[n];

    // Set correct expectations on the relevant pipe mgr APIs
    tableEntryGetSetExpForEntry(table, single_random_data, 1);

    EXPECT_SUCCESS(table.tableEntryGetFirst(getDefaultSession(),
                                            getDefaultBfRtTarget(),
                                            getDefaultTableReadFlag(),
                                            first_key.get(),
                                            first_data.get()));
    EXPECT_SUCCESS(table.keyReset(table_key_expected.get()));

    formTableKey_from_idx(table, n, table_key_expected.get());
    formTableData_from_idx(table, n, table_data_expected.get());

    compareKeyObjects(*table_key_expected, *first_key);
    compareDataObjects(*table_data_expected, *first_data);

    // Now remove first entry from the ordered set and proceed with reading
    // the rest of the entries
    entry_hdl_list.erase(first_entry_hdl);
    if (entry_hdl_list.size() == 0) continue;

    // Get the next "N" entries
    std::vector<std::pair<BfRtTableKey *, BfRtTableData *>> next_entries;
    uint32_t next_entries_count = entry_hdl_list.size();
    std::vector<std::unique_ptr<BfRtTableKey>> next_keys(next_entries_count);
    std::vector<std::unique_ptr<BfRtTableData>> next_data(next_entries_count);
    for (uint32_t i = 0; i < next_entries_count; i++) {
      EXPECT_SUCCESS(table.keyAllocate(&next_keys[i]));
      EXPECT_SUCCESS(table.dataAllocate(&next_data[i]));
      next_entries.push_back(
          std::make_pair(next_keys[i].get(), next_data[i].get()));
    }

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrMatchSpecToEntHdl(_, _, _, _, _, false))
        .Times(1)
        .WillOnce(WithArgs<4>(Invoke([&](pipe_mat_ent_hdl_t *mat_ent_hdl) {
          *mat_ent_hdl = first_entry_hdl;
          return PIPE_SUCCESS;
        })))
        .RetiresOnSaturation();

    // Set correct expectations on the relevant pipe mgr APIs
    // Here, the field destinations are the same for all the enties in the
    // Match Direct table. We require the random data object to get hold
    // of the field destinations. As all the entries will have the same
    // destinations, we can simply pass in the single_random_data (which
    // corresponds to 0th random data) here as well
    tableEntryGetSetExpForEntry(table, single_random_data, next_entries_count);
    uint32_t num_returned;
    EXPECT_SUCCESS(table.tableEntryGetNext_n(getDefaultSession(),
                                             getDefaultBfRtTarget(),
                                             *first_key,
                                             next_entries_count,
                                             getDefaultTableReadFlag(),
                                             &next_entries,
                                             &num_returned));
    EXPECT_EQ(num_returned, next_entries_count) << "Num entries returned from "
                                                   "iterator don't match the "
                                                   "expected value\n";
    // Here we are relying on the implicit order in which pipe mgr returns us
    // installed entries. This order is not guaranteed but for the time being
    // we can assume that it is constant and that pipe mgr is going to return
    // us entries in the order in which they were added. Thus we assume that
    // the entries returned would be in this order of entry handles.
    // 1, 2, 3, etc..
    int counter = 0;
    for (const auto &p : entry_hdl_list) {
      auto arr_idx = p.second;
      formTableKey_from_idx(table, arr_idx, table_key_expected.get());
      formTableData_from_idx(table, arr_idx, table_data_expected.get());

      compareKeyObjects(*table_key_expected, *next_entries[counter].first);
      compareDataObjects(*table_data_expected, *next_entries[counter].second);
      counter++;
    }
  }
}

TEST_P(BfRtMatchActionTableTest, EntryMod) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryMod");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Now modify the entries. There are 2 ways in which we are going to test
    // modifies.
    // 1. In the first way, we are going to swap the data object between
    //    entries.
    // 2. In the second way, we are going to iterate over all the fields in the
    //    random data object and modify them one by one. While doing so we are
    //    going to set expectations on the backend pipe mgr API relevant to
    //    that field. In addition we need to ensure none of the other APIs are
    //    are called

    // 1.
    EXPECT_SUCCESS(
        tableEntryModSwapActionData(num_entries, table, entry_hdl_list));

    // 2.
    EXPECT_SUCCESS(tableEntryModIterateFields(table, entry_hdl_list));
  }
}

TEST_P(BfRtMatchActionTableTest, EntryDel) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryDel");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Do one keyAllocate and reset across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(table.keyAllocate(&table_key));

    bool is_const_table = false;
    table.tableIsConst(&is_const_table);

    // Now delete all the entries
    for (uint32_t m = 0; m < num_entries; m++) {
      // Create match spec matchers
      auto ms_matcher = BfRtMatchersFactory::makeMatchSpecMatcher(
          exp_ms_vec[m]->getPipeMatchSpec());

      if (!is_const_table) {
        EXPECT_CALL(*pipe_mgr_obj,
                    pipeMgrMatEntDelByMatchSpec(_, _, _, ms_matcher, _))
            .Times(1)
            .RetiresOnSaturation();
      }

      formTableKey_from_idx(table, m, table_key.get());
      // Delete entry from the table
      auto sts = table.tableEntryDel(
          getDefaultSession(), getDefaultBfRtTarget(), *table_key);
      if (!is_const_table) {
        EXPECT_EQ(sts, BF_SUCCESS)
            << "Table entry delete failed for entry : " << m
            << " Expected status : " << 0 << " Received status : " << sts
            << "\n";
      } else {
        EXPECT_EQ(sts, BF_INVALID_ARG)
            << "Table entry del succeeded when we "
               "expected it to fail for const table : "
            << m << " Expected status : " << BF_INVALID_ARG
            << " Received status : " << sts << "\n";
      }
    }
  }
}

void BfRtActionTableTest::initialTestSetup(const BfRtTable &table,
                                           const BfRtEntryGen &entry_generator,
                                           const uint32_t &num_entries) {
  // Call Base's initial setup
  BfRtTableTest::initialTestSetup(table, entry_generator, num_entries);

  // Get the randomly generator key and data objects
  tableRandomKeyGet(table, entry_generator, num_entries, &random_keys);
  tableRandomDataGet(table, entry_generator, num_entries, &random_data);

  // Form the 'ground truth' copies of the action spec to compare
  tableExpectedActionSpecGet(table, random_data, &exp_as_vec);
}

bf_status_t BfRtActionTableTest::tableEntryAdd(
    const uint32_t &num_entries,
    const BfRtTable &table,
    std::map<pipe_ent_hdl_t, int> *entry_hdls) {
  // - Allocate 'num_entries' number of key and data objects
  // - Get 'num_entries' number of randomly generated key and data objects
  //   from the Generator
  // - For all the key and data objects, iterate over all the key and data
  //   fields respectively and set their values
  // - For all the key and data objects, form the 'ground truth' copy of the
  //   action spec
  // - Create action spec matchers using the 'ground_truth' copies
  // - Set expectations on pipeMgrAdtEntAdd

  // Do one keyAllocate and dataAllocate
  std::unique_ptr<BfRtTableKey> table_key;
  EXPECT_SUCCESS(table.keyAllocate(&table_key));

  std::unique_ptr<BfRtTableData> table_data;
  EXPECT_SUCCESS(table.dataAllocate(&table_data));

  for (uint32_t n = 0; n < num_entries; n++) {
    pipe_adt_ent_hdl_t adt_ent_hdl = n + 1;
    // Create action spec matchers
    const pipe_action_spec_t *as = exp_as_vec[n]->getPipeActionSpec();
    auto as_matcher = BfRtMatchersFactory::makeActionSpecMatcher(as);

    // From table key and data from index
    formTableKey_from_idx(table, n, table_key.get());
    formTableData_from_idx(table, n, table_data.get());

    EXPECT_CALL(
        *pipe_mgr_obj,
        pipeMgrAdtEntAdd(_,
                         _,
                         _,
                         static_cast<const BfRtTableObj &>(table).getActFnHdl(
                             random_data[n].actionIdGet()),
                         _,
                         as_matcher,
                         _,
                         _))
        .Times(1)
        .WillOnce(DoAll(
            WithArgs<6>(
                Invoke([&](pipe_adt_ent_hdl_t *hdl) { *hdl = adt_ent_hdl; })),
            InvokeWithoutArgs([&]() {
              Entry mt(nullptr,
                       exp_as_vec[n]->getPipeActionSpec(),
                       static_cast<const BfRtTableObj &>(table).getActFnHdl(
                           random_data[n].actionIdGet()),
                       0,
                       &table);
              pipe_mgr_obj->getMockIPipeMgrIntfHelper().addEntryToDatabase(
                  adt_ent_hdl, &mt);
              return PIPE_SUCCESS;
            })))
        .RetiresOnSaturation();

    // Add entry in the table
    auto sts = table.tableEntryAdd(
        getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data);
    EXPECT_EQ(sts, BF_SUCCESS)
        << " Table entry add failed for entry : " << n
        << " Expected status : " << 0 << " Received status : " << sts << "\n";
    if (sts == BF_SUCCESS) {
      // Add the enrtry handle only in case of success
      if (entry_hdls->find(adt_ent_hdl) != entry_hdls->end()) {
        // This indicates that we are trying to add the same entry handle
        // twice.
        // This indicates a bug in the unit test logic
        BF_RT_ASSERT(0);
      }
      entry_hdls->insert(std::make_pair(adt_ent_hdl, n));
    }
  }
  return BF_SUCCESS;
}

// Test cases for Action Table
/*
 * --- Completed
 * - Simple table entry add with unique table keys
 * - Simple table entry get (reading each entry one by one)
 * - Simple table entry iterator (reading exactly as many entries as present in
 *the table)
 *
 * --- TODO Planned
 * - Try adding entry with the same table key ?
 * - Try reading more entries than actually present on the table by using the
 *iterator
 * - Simple table entry delete
 * - Simple table entry modify
 */

TEST_P(BfRtActionTableTest, EntryAdd) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryAdd");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));
  }
}

TEST_P(BfRtActionTableTest, DISABLED_EntryGet) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryGet");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Do one keyAllocate and dataAllocate and reset across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(table.dataAllocate(&table_data));

    // Now iterate over the ordered_set and read each entry
    for (auto iter = entry_hdl_list.begin(); iter != entry_hdl_list.end();
         iter++) {
      const auto &adt_ent_hdl = iter->first;
      int m = iter->second;

      EXPECT_CALL(*pipe_mgr_obj,
                  pipeMgrGetActionDataEntry(_, _, adt_ent_hdl, _, _, _))
          .Times(1)
          .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                           &MockIPipeMgrIntfHelper::pipeMgrGetActionDataEntry))
          .RetiresOnSaturation();

      // Fill in the key from random_key
      formTableKey_from_idx(table, m, table_key.get());

      // Just reset the data
      ASSERT_SUCCESS(table.dataReset(table_data.get()));

      // Do data Allocate for the expected data object
      auto &single_random_data = random_data[m];
      std::unique_ptr<BfRtTableData> table_data_expected;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(table.dataAllocate(single_random_data.actionIdGet(),
                                          &table_data_expected));
      } else {
        EXPECT_SUCCESS(table.dataAllocate(&table_data_expected));
      }
      tableDataSetHelper(table, single_random_data, *table_data_expected);

      EXPECT_SUCCESS(table.tableEntryGet(getDefaultSession(),
                                         getDefaultBfRtTarget(),
                                         *table_key,
                                         getDefaultTableReadFlag(),
                                         table_data.get()));

      compareDataObjects(*table_data_expected, *table_data);
    }
  }
}

TEST_P(BfRtActionTableTest, DISABLED_EntryGetIterator) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    table,
                                    entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryGetIterator");

    // Gather all the member ids allocated and store the corresponding index
    // in the key array
    std::unordered_map<
        uint64_t /* member id */,
        uint32_t /* index in random_key and random_data vectors */>
        mbr_id_to_idx;
    ASSERT_EQ(random_keys[0].fieldIdListGet().size(), 1);
    const auto mbr_field_id = random_keys[0].fieldIdListGet()[0];
    for (uint32_t i = 0; i < random_keys.size(); i++) {
      uint64_t member_id;
      const GenericKeyField &generic_key_field =
          random_keys[i].keyFieldGet(mbr_field_id);
      generic_key_field.getValue(&member_id);
      mbr_id_to_idx.insert(std::make_pair(member_id, i));
    }

    // Do one keyAllocate and dataAllocate to contain expected key and data
    // objects used to compare what is received from the table iteration
    std::unique_ptr<BfRtTableKey> table_key_expected;
    ASSERT_SUCCESS(table.keyAllocate(&table_key_expected));

    std::unique_ptr<BfRtTableData> table_data_expected;
    ASSERT_SUCCESS(table.dataAllocate(&table_data_expected));

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    if (entry_hdl_list.size() == 0) continue;

    // Now read the first and subsequent 'N' entries
    auto first_adt_entry_hdl = entry_hdl_list.begin()->first;
    std::unique_ptr<BfRtTableKey> first_key;
    EXPECT_SUCCESS(table.keyAllocate(&first_key));
    std::unique_ptr<BfRtTableData> first_data;
    EXPECT_SUCCESS(table.dataAllocate(&first_data));
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetActionDataEntry(_, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrGetActionDataEntry))
        .RetiresOnSaturation();

    // BF-RT is free to return any of the installed entries as the first entry
    // in the table. Hence we need to see the member id in the first key that
    // is returned and then use the key and the data at the corresponding index
    // in our global vectors to compare against the returned values.
    EXPECT_SUCCESS(table.tableEntryGetFirst(getDefaultSession(),
                                            getDefaultBfRtTarget(),
                                            getDefaultTableReadFlag(),
                                            first_key.get(),
                                            first_data.get()));
    uint64_t member_id;
    EXPECT_SUCCESS(first_key->getValue(mbr_field_id, &member_id));
    EXPECT_NE(mbr_id_to_idx.find(member_id), mbr_id_to_idx.end());
    uint32_t first_mbr_index = mbr_id_to_idx.at(member_id);
    // Form expected key and data from the member index
    formTableKey_from_idx(table, first_mbr_index, table_key_expected.get());
    formTableData_from_idx(table, first_mbr_index, table_data_expected.get());

    compareKeyObjects(*table_key_expected, *first_key);
    compareDataObjects(*table_data_expected, *first_data);

    // Now remove first entry from the ordered set and proceed with reading
    // the rest of the entries
    entry_hdl_list.erase(first_adt_entry_hdl);
    if (entry_hdl_list.size() == 0) continue;

    // Get the next "N" entries
    std::vector<std::pair<BfRtTableKey *, BfRtTableData *>> next_entries;
    uint32_t next_entries_count = entry_hdl_list.size();
    std::vector<std::unique_ptr<BfRtTableKey>> next_keys(next_entries_count);
    std::vector<std::unique_ptr<BfRtTableData>> next_data(next_entries_count);
    for (uint32_t i = 0; i < next_entries_count; i++) {
      EXPECT_SUCCESS(table.keyAllocate(&next_keys[i]));
      EXPECT_SUCCESS(table.dataAllocate(&next_data[i]));
      next_entries.push_back(
          std::make_pair(next_keys[i].get(), next_data[i].get()));
    }

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetActionDataEntry(_, _, _, _, _, _))
        .Times(AtLeast(1))  // FIXME should be Times(next_entries_count)
        .WillRepeatedly(
            Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                   &MockIPipeMgrIntfHelper::pipeMgrGetActionDataEntry))
        .RetiresOnSaturation();

    uint32_t num_returned;
    EXPECT_SUCCESS(table.tableEntryGetNext_n(getDefaultSession(),
                                             getDefaultBfRtTarget(),
                                             *first_key,
                                             next_entries_count,
                                             getDefaultTableReadFlag(),
                                             &next_entries,
                                             &num_returned));
    EXPECT_EQ(num_returned, next_entries_count) << "Num entries returned from "
                                                   "iterator don't match the "
                                                   "expected value\n";
    // Here we are relying on the implicit order in which pipe mgr returns us
    // installed entries. This order is not guaranteed but for the time being
    // we can assume that it is constant and that pipe mgr is going to return
    // us entries in the order in which they were added. Thus we assume that
    // the entries returned would be in this order of entry handles.
    // 1, 2, 3, etc..
    int counter = 0;
    for (const auto &p : entry_hdl_list) {
      auto arr_idx = p.second;
      uint64_t member_id_tmp;
      EXPECT_SUCCESS(
          next_entries[counter].first->getValue(mbr_field_id, &member_id_tmp));
      EXPECT_NE(mbr_id_to_idx.find(member_id_tmp), mbr_id_to_idx.end());
      uint32_t index = mbr_id_to_idx.at(member_id_tmp);

      formTableKey_from_idx(table, index, table_key_expected.get());
      formTableData_from_idx(table, index, table_data_expected.get());

      compareKeyObjects(*table_key_expected, *next_entries[counter].first);
      compareDataObjects(*table_data_expected, *next_entries[counter].second);
      counter++;
    }
  }
}

TEST_P(BfRtActionTableTest, DISABLED_EntryMod) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryMod");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Do one keyAllocate and dataAllocate and reset across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(table.dataAllocate(&table_data));

    // Now modify the entries. For modifying the entries we just swap the
    // actions between the entries. (n-1) <-> 0, (n-2) <-> 1, ...
    // This assumes the fact that the actions being random are going to
    // be different among the entries
    for (auto iter = entry_hdl_list.begin(); iter != entry_hdl_list.end();
         iter++) {
      pipe_ent_hdl_t adt_ent_hdl = iter->first;
      int s = iter->second;
      // Create match and action spec matchers
      const pipe_action_spec_t *as =
          exp_as_vec[num_entries - 1 - s]->getPipeActionSpec();
      auto as_matcher = BfRtMatchersFactory::makeActionSpecMatcher(as);

      EXPECT_CALL(
          *pipe_mgr_obj,
          pipeMgrAdtEntSet(_,
                           _,
                           _,
                           adt_ent_hdl,
                           static_cast<const BfRtTableObj &>(table).getActFnHdl(
                               random_data[num_entries - 1 - s].actionIdGet()),
                           as_matcher,
                           _))
          .Times(1)
          .RetiresOnSaturation();

      // Form Tablekey and data
      formTableKey_from_idx(table, s, table_key.get());
      formTableData_from_idx(table, num_entries - 1 - s, table_data.get());

      // Modify entry in the table
      auto sts = table.tableEntryMod(
          getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data);
      EXPECT_EQ(sts, BF_SUCCESS)
          << "Table entry mod failed for entry : " << s
          << " Expected status : " << 0 << " Received status : " << sts << "\n";
    }
  }
}

TEST_P(BfRtActionTableTest, DISABLED_EntryDel) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < d_tables.size(); n++) {
    const auto &table = *d_tables[n];
    auto &entry_generator = *(entry_generator_map[d_tables[n]]);

    BfRtTableScopeGuard scope_guard(
        *this, table, entry_generator, num_entries, pipe_mgr_obj, "EntryDel");

    // Add entries in the table
    std::map<pipe_ent_hdl_t, int> entry_hdl_list;
    ASSERT_SUCCESS(tableEntryAdd(num_entries, table, &entry_hdl_list));

    // Do one keyAllocate and reuse across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(table.keyAllocate(&table_key));

    // Do one dataAllocate. No need of resetting it, since we delete the entry
    // and expect the entry Get API to not return success
    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(table.dataAllocate(&table_data));

    // Now delete all the entries
    for (auto iter = entry_hdl_list.begin(); iter != entry_hdl_list.end();
         iter++) {
      pipe_ent_hdl_t adt_ent_hdl = iter->first;
      int n1 = iter->second;

      EXPECT_CALL(*pipe_mgr_obj, pipeMgrAdtEntDel(_, _, _, adt_ent_hdl, _))
          .Times(1)
          .RetiresOnSaturation();

      // Form table key from random key
      formTableKey_from_idx(table, n1, table_key.get());

      // Delete entry from the table
      auto sts = table.tableEntryDel(
          getDefaultSession(), getDefaultBfRtTarget(), *table_key);
      EXPECT_EQ(sts, BF_SUCCESS)
          << "Table entry delete failed for entry : " << n1
          << " Expected status : " << 0 << " Received status : " << sts << "\n";

      // Since BF-RT itself stores state for action tables, we need to ensure
      // that that state gets updated properly. Thus try to read the entry
      // and expect it to fail
      std::unique_ptr<BfRtTableData> table_data_tmp;
      EXPECT_SUCCESS(table.dataAllocate(&table_data_tmp));
      EXPECT_EQ(table.tableEntryGet(getDefaultSession(),
                                    getDefaultBfRtTarget(),
                                    *table_key,
                                    getDefaultTableReadFlag(),
                                    table_data_tmp.get()),
                BF_OBJECT_NOT_FOUND);
    }
  }
}

void BfRtSelectorTableTest::initialTestSetup(
    const BfRtTable &table,
    const BfRtEntryGen &entry_generator,
    const uint32_t &num_entries) {
  // Call Parent's initial setup
  BfRtTableTest::initialTestSetup(table, entry_generator, num_entries);

  // Get the randomly generator key and data objects
  tableRandomKeyGet(table, entry_generator, num_entries, &random_keys);
  tableRandomDataGet(table, entry_generator, num_entries, &random_data);
}

void BfRtSelectorTableTest::addEntriesInActProfTable(
    const BfRtTable &act_tbl, std::unordered_set<uint32_t> *seen_mbr_ids) {
  // A simple way to ensure that all the randomly generated mbrs (as part of
  // different groups) are present in the corresponding action profile table
  // is to iterate over all the members in all
  // groups generated by the random generator and then install them in the
  // corresponding action profile table

  // Now this function can be called multiple times (everytime we regeneate
  // data objects and want to add all the unseen member ids in the act prof
  // table). Thus we need to ensure that we install only the new set of unseen
  // member ids.
  std::unordered_set<uint32_t> new_mbr_ids;
  for (const auto &single_random_data : random_data) {
    const auto group_mbrs = single_random_data.getGroupMbrs();
    for (const auto group_mbr : group_mbrs) {
      if (seen_mbr_ids->find(group_mbr) == seen_mbr_ids->end()) {
        new_mbr_ids.insert(group_mbr);
        seen_mbr_ids->insert(group_mbr);
      }
    }
  }
  const uint32_t act_prof_num_entries = new_mbr_ids.size();
  auto &act_table_entry_generator = *(entry_generator_map[&act_tbl]);
  std::vector<GenericKey> local_random_keys;
  std::vector<GenericData> local_random_data;

  // Do one keyAllocate and dataAllocate and reset across uses
  std::unique_ptr<BfRtTableKey> table_key;
  EXPECT_SUCCESS(act_tbl.keyAllocate(&table_key));

  std::unique_ptr<BfRtTableData> table_data;
  EXPECT_SUCCESS(act_tbl.dataAllocate(&table_data));

  tableRandomKeyGet(act_tbl,
                    act_table_entry_generator,
                    act_prof_num_entries,
                    &local_random_keys);
  tableRandomDataGet(act_tbl,
                     act_table_entry_generator,
                     act_prof_num_entries,
                     &local_random_data);
  // At this point, random keys would have randomly generated mbr ids.
  // Thus we need to overrwrite them with the ones which we want
  uint32_t k = 0;
  for (auto iter = new_mbr_ids.begin(); iter != new_mbr_ids.end();
       iter++, k++) {
    local_random_keys[k].setActionMbrId(*iter);
    uint32_t temp = local_random_keys[k].getActionMbrId();
    BF_RT_ASSERT(temp == *iter);
  }

  uint32_t n = 0;
  for (auto iter = new_mbr_ids.begin(); iter != new_mbr_ids.end();
       iter++, n++) {
    pipe_adt_ent_hdl_t adt_ent_hdl = *iter;
    EXPECT_CALL(*pipe_mgr_obj, pipeMgrAdtEntAdd(_, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(WithArgs<6>(Invoke([&](pipe_adt_ent_hdl_t *hdl) {
          *hdl = adt_ent_hdl;
          return PIPE_SUCCESS;
        })))
        .RetiresOnSaturation();

    // Form table key and table data from random key and data respectively
    auto &single_random_key = local_random_keys[n];
    ASSERT_SUCCESS(act_tbl.keyReset(table_key.get()));
    tableKeySetHelper(single_random_key, *table_key);

    auto &single_random_data = local_random_data[n];
    ASSERT_SUCCESS(
        act_tbl.dataReset(single_random_data.actionIdGet(), table_data.get()));
    tableDataSetHelper(act_tbl, single_random_data, *table_data);

    EXPECT_SUCCESS(act_tbl.tableEntryAdd(
        getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data));

    // Maintain a mapping between action member id and the adt_ent_hdl
    // This is what is maintained by bfrt
    mbr_id_to_adt_ent_hdl_map.insert(std::make_pair(*iter, adt_ent_hdl));
  }
}

bf_status_t BfRtSelectorTableTest::tableEntryAdd(
    const uint32_t &num_entries,
    const BfRtTable &table,
    std::map<pipe_sel_grp_hdl_t, int> *group_hdls) {
  // Do one keyAllocate and dataAllocate and reset across uses
  std::unique_ptr<BfRtTableKey> table_key;
  EXPECT_SUCCESS(table.keyAllocate(&table_key));

  std::unique_ptr<BfRtTableData> table_data;
  EXPECT_SUCCESS(table.dataAllocate(&table_data));

  for (uint32_t n = 0; n < num_entries; n++) {
    pipe_sel_grp_hdl_t sel_grp_hdl = n + 1;
    const auto &single_random_data = random_data[n];
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrSelGrpAdd(
                    _, _, _, _, single_random_data.getMaxGroupSize(), _, _, _))
        .Times(1)
        .WillOnce(DoAll(
            WithArgs<6>(Invoke(
                [&](pipe_sel_grp_hdl_t *grp_hdl) { *grp_hdl = sel_grp_hdl; })),
            InvokeWithoutArgs([&]() {
              SelGroupEntry new_entry(single_random_data.getMaxGroupSize());
              pipe_mgr_obj->getMockIPipeMgrIntfHelper().addSelGroupToDatabase(
                  sel_grp_hdl, &new_entry);
              return PIPE_SUCCESS;
            })))
        .RetiresOnSaturation();

    // Get the member ids and sts in this random data
    const auto mbr_ids = single_random_data.getGroupMbrs();
    const auto mbr_sts = single_random_data.getGroupMbrSts();
    const auto adt_ent_hdls = getPipeAdtEntHdlsFromActMbrIds(mbr_ids);
    auto mbr_id_matcher =
        BfRtMatchersFactory::makeGroupMbrIdMatcher(adt_ent_hdls);
    auto mbr_sts_matcher = BfRtMatchersFactory::makeGroupMbrStsMatcher(mbr_sts);
    EXPECT_CALL(*pipe_mgr_obj,
                pipeMgrSelGrpMbrsSet(_,
                                     _,
                                     _,
                                     sel_grp_hdl,
                                     single_random_data.getGroupMbrCount(),
                                     mbr_id_matcher,
                                     mbr_sts_matcher,
                                     _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs([&]() {
          pipe_mgr_obj->getMockIPipeMgrIntfHelper().addActMbrsToSelGroup(
              sel_grp_hdl, adt_ent_hdls, mbr_sts);
          return PIPE_SUCCESS;
        }))
        .RetiresOnSaturation();

    // Form table key and table data from random key and data respectively
    formTableKey_from_idx(table, n, table_key.get());
    formTableData_from_idx(table, n, table_data.get());

    // Add entry in the table
    auto sts = table.tableEntryAdd(
        getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data);
    EXPECT_EQ(sts, BF_SUCCESS)
        << " Table entry add failed for entry : " << n
        << " Expected status : " << 0 << " Received status : " << sts << "\n";
    if (sts == BF_SUCCESS) {
      // Add the enrtry handle only in case of success
      if (group_hdls->find(sel_grp_hdl) != group_hdls->end()) {
        // This indicates that we are trying to add the same entry handle twice.
        // This indicates a bug in the unit test logic
        BF_RT_ASSERT(0);
      }
      group_hdls->insert(std::make_pair(sel_grp_hdl, n));
    }
  }
  return BF_SUCCESS;
}

// Test Cases for Selector Table

TEST_P(BfRtSelectorTableTest, DISABLED_EntryAdd) {
  const uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    auto &sel_table = *d_tables[iter];
    auto &sel_table_entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    sel_table,
                                    sel_table_entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryAdd");
    // This set keeps track of all the member ids which we have installed in
    // in the action profile corresponding to this selector table
    std::unordered_set<uint32_t> seen_mbr_ids;
    // Before we try to add any entry in the selector table, we need to ensure
    // that those members have been added in the corresponding act_prof table.
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    // Once we have ensured that all the action member ids have been added in
    // the action profile table, add entries in the selector table
    std::map<pipe_sel_grp_hdl_t, int> group_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, sel_table, &group_hdl_list));
  }
}

TEST_P(BfRtSelectorTableTest, DISABLED_EntryGet) {
  const uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    auto &sel_table = *d_tables[iter];
    auto &sel_table_entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    sel_table,
                                    sel_table_entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryGet");
    // This set keeps track of all the member ids which we have installed in
    // in the action profile corresponding to this selector table
    std::unordered_set<uint32_t> seen_mbr_ids;
    // Before we try to add any entry in the selector table, we need to ensure
    // that those members have been added in the corresponding act_prof table.
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    // Once we have ensured that all the action member ids have been added in
    // the action profile table, add entries in the selector table
    std::map<pipe_sel_grp_hdl_t, int> group_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, sel_table, &group_hdl_list));

    // Do keyAllocate and dataAllocate once to hold the key and data and reset
    // across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(sel_table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(sel_table.dataAllocate(&table_data));

    // Now iterate over the ordered_set and read each entry
    for (auto iter_2 = group_hdl_list.begin(); iter_2 != group_hdl_list.end();
         iter_2++) {
      const auto &sel_grp_hdl = iter_2->first;
      int index = iter_2->second;

      auto &single_random_data = random_data[index];

      // form Keyobject from random key
      formTableKey_from_idx(sel_table, index, table_key.get());
      ASSERT_SUCCESS(sel_table.dataReset(table_data.get()));

      std::unique_ptr<BfRtTableData> table_data_expected;
      EXPECT_SUCCESS(sel_table.dataAllocate(&table_data_expected));
      tableDataSetHelper(sel_table, single_random_data, *table_data_expected);

      EXPECT_CALL(*pipe_mgr_obj,
                  pipeMgrGetSelGrpMbrCount(_, _, _, sel_grp_hdl, _))
          .Times(1)
          .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                           &MockIPipeMgrIntfHelper::pipeMgrGetSelGrpMbrCount))
          .RetiresOnSaturation();

      EXPECT_CALL(*pipe_mgr_obj,
                  pipeMgrSelGrpMbrsGet(_,
                                       _,
                                       _,
                                       sel_grp_hdl,
                                       random_data[index].getGroupMbrCount(),
                                       _,
                                       _,
                                       _,
                                       _))
          .Times(1)
          .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                           &MockIPipeMgrIntfHelper::pipeMgrSelGrpMbrsGet))
          .RetiresOnSaturation();

      EXPECT_SUCCESS(sel_table.tableEntryGet(getDefaultSession(),
                                             getDefaultBfRtTarget(),
                                             *table_key,
                                             getDefaultTableReadFlag(),
                                             table_data.get()));

      compareDataObjects(*table_data_expected, *table_data);
    }
  }
}

TEST_P(BfRtSelectorTableTest, DISABLED_EntryGetIterator) {
  const uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    auto &sel_table = *d_tables[iter];
    auto &sel_table_entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    sel_table,
                                    sel_table_entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryGetIterator");

    // Gather all the group ids allocated and store the corresponding index
    // in the key array
    std::unordered_map<
        uint64_t /* group id */,
        uint32_t /* index in random_key and random_data vectors */>
        grp_id_to_idx;
    ASSERT_EQ(random_keys[0].fieldIdListGet().size(), 1);
    const auto grp_field_id = random_keys[0].fieldIdListGet()[0];
    for (uint32_t i = 0; i < random_keys.size(); i++) {
      uint64_t group_id;
      const GenericKeyField &generic_key_field =
          random_keys[i].keyFieldGet(grp_field_id);
      generic_key_field.getValue(&group_id);
      grp_id_to_idx.insert(std::make_pair(group_id, i));
    }

    // This set keeps track of all the member ids which we have installed in
    // in the action profile corresponding to this selector table
    std::unordered_set<uint32_t> seen_mbr_ids;
    // Add entries in the table
    // Before we try to add any entry in the selector table, we need to ensure
    // that those members have been added in the corresponding act_prof table.
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    // Once we have ensured that all the action member ids have been added in
    // the action profile table, add entries in the selector table
    std::map<pipe_sel_grp_hdl_t, int> group_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, sel_table, &group_hdl_list));

    // Do one keyAllocate and dataAllocate to compare with what is received from
    // the table iterator. Reset across use.
    std::unique_ptr<BfRtTableKey> table_key_expected;
    EXPECT_SUCCESS(sel_table.keyAllocate(&table_key_expected));

    std::unique_ptr<BfRtTableData> table_data_expected;
    EXPECT_SUCCESS(sel_table.dataAllocate(&table_data_expected));

    if (group_hdl_list.size() == 0) continue;

    // Now read the first and the subsequent 'N' entries
    auto first_sel_grp_hdl = group_hdl_list.begin()->first;
    std::unique_ptr<BfRtTableKey> first_key;
    EXPECT_SUCCESS(sel_table.keyAllocate(&first_key));
    std::unique_ptr<BfRtTableData> first_data;
    EXPECT_SUCCESS(sel_table.dataAllocate(&first_data));

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetSelGrpMbrCount(_, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrGetSelGrpMbrCount))
        .RetiresOnSaturation();

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrSelGrpMbrsGet(_, _, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                         &MockIPipeMgrIntfHelper::pipeMgrSelGrpMbrsGet))
        .RetiresOnSaturation();

    // BF-RT is free to return any of the installed entries as the first entry
    // in the table. Hence we need to see the group id in the first key that
    // is returned and then use the key and the data at the corresponding index
    // in our global vectors to compare against the returned values.
    EXPECT_SUCCESS(sel_table.tableEntryGetFirst(getDefaultSession(),
                                                getDefaultBfRtTarget(),
                                                getDefaultTableReadFlag(),
                                                first_key.get(),
                                                first_data.get()));
    uint64_t group_id;
    EXPECT_SUCCESS(first_key->getValue(grp_field_id, &group_id));
    EXPECT_NE(grp_id_to_idx.find(group_id), grp_id_to_idx.end());
    uint32_t first_grp_index = grp_id_to_idx.at(group_id);

    // Form expected key and data from the grp index
    formTableKey_from_idx(sel_table, first_grp_index, table_key_expected.get());
    formTableData_from_idx(
        sel_table, first_grp_index, table_data_expected.get());

    compareKeyObjects(*table_key_expected, *first_key);
    compareDataObjects(*table_data_expected, *first_data);

    // Now remove first entry from the ordered set and proceed with reading
    // the rest of the entries
    group_hdl_list.erase(first_sel_grp_hdl);
    if (group_hdl_list.size() == 0) continue;

    // Get the next "N" entries
    std::vector<std::pair<BfRtTableKey *, BfRtTableData *>> next_entries;
    uint32_t next_entries_count = num_entries - 1;
    std::vector<std::unique_ptr<BfRtTableKey>> next_keys(next_entries_count);
    std::vector<std::unique_ptr<BfRtTableData>> next_data(next_entries_count);
    for (uint32_t i = 0; i < next_entries_count; i++) {
      EXPECT_SUCCESS(sel_table.keyAllocate(&next_keys[i]));
      EXPECT_SUCCESS(sel_table.dataAllocate(&next_data[i]));
      next_entries.push_back(
          std::make_pair(next_keys[i].get(), next_data[i].get()));
    }

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrGetSelGrpMbrCount(_, _, _, _, _))
        .Times(next_entries_count)
        .WillRepeatedly(
            Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                   &MockIPipeMgrIntfHelper::pipeMgrGetSelGrpMbrCount))
        .RetiresOnSaturation();

    EXPECT_CALL(*pipe_mgr_obj, pipeMgrSelGrpMbrsGet(_, _, _, _, _, _, _, _, _))
        .Times(next_entries_count)
        .WillRepeatedly(Invoke(&pipe_mgr_obj->getMockIPipeMgrIntfHelper(),
                               &MockIPipeMgrIntfHelper::pipeMgrSelGrpMbrsGet))
        .RetiresOnSaturation();

    uint32_t num_returned;
    EXPECT_SUCCESS(sel_table.tableEntryGetNext_n(getDefaultSession(),
                                                 getDefaultBfRtTarget(),
                                                 *first_key,
                                                 next_entries_count,
                                                 getDefaultTableReadFlag(),
                                                 &next_entries,
                                                 &num_returned));
    EXPECT_EQ(num_returned, next_entries_count) << "Num entries returned from "
                                                   "iterator don't match the "
                                                   "expected value\n";
    for (uint64_t i = 0; i < next_entries_count; i++) {
      uint64_t group_id_2;
      EXPECT_SUCCESS(
          next_entries[i].first->getValue(grp_field_id, &group_id_2));
      EXPECT_NE(grp_id_to_idx.find(group_id_2), grp_id_to_idx.end());
      uint32_t index = grp_id_to_idx.at(group_id_2);
      // Form expected key and data from the group id
      formTableKey_from_idx(sel_table, index, table_key_expected.get());
      formTableData_from_idx(sel_table, index, table_data_expected.get());
      compareKeyObjects(*table_key_expected, *next_entries[i].first);
      compareDataObjects(*table_data_expected, *next_entries[i].second);
    }
  }
}

TEST_P(BfRtSelectorTableTest, DISABLED_EntryMod) {
  const uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    auto &sel_table = *d_tables[iter];
    auto &sel_table_entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    sel_table,
                                    sel_table_entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryMod");

    // Add entries in the table
    // This set keeps track of all the member ids which we have installed in
    // in the action profile corresponding to this selector table
    std::unordered_set<uint32_t> seen_mbr_ids;
    // Before we try to add any entry in the selector table, we need to ensure
    // that those members have been added in the corresponding act_prof table.
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    // Once we have ensured that all the action member ids have been added in
    // the action profile table, add entries in the selector table
    std::map<pipe_sel_grp_hdl_t, int> group_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, sel_table, &group_hdl_list));

    // Now modify the entries. Her we simply cant swap the data between the
    // entries because different entries will have data of different max group
    // size and we cannot maodify the max group size once we have created
    // the group. So just regenerate each data object (which keeps the max
    // group size constant and just modifies the values and the count of the
    // members)

    // First regenerate all the data objects
    for (auto iter_3 = group_hdl_list.begin(); iter_3 != group_hdl_list.end();
         iter_3++) {
      int index = iter_3->second;

      auto &single_random_data = random_data[index];
      // Modify the random data
      single_random_data.regenerate();
    }

    // Do one keyAllocate and dataAllocate and reset across uses
    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(sel_table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(sel_table.dataAllocate(&table_data));

    // Now that all the data objects have been regenerated, add all the
    // members in the corresponding action profile table so that they can
    // be successfully added in the selector table
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    for (auto iter_3 = group_hdl_list.begin(); iter_3 != group_hdl_list.end();
         iter_3++) {
      pipe_sel_grp_hdl_t sel_grp_hdl = iter_3->first;
      int index = iter_3->second;

      auto &single_random_key = random_keys[index];
      auto &single_random_data = random_data[index];
      // Get the member ids and sts in this random data
      const auto mbr_ids = single_random_data.getGroupMbrs();
      const auto mbr_sts = single_random_data.getGroupMbrSts();
      const auto adt_ent_hdls = getPipeAdtEntHdlsFromActMbrIds(mbr_ids);
      auto mbr_id_matcher =
          BfRtMatchersFactory::makeGroupMbrIdMatcher(adt_ent_hdls);
      auto mbr_sts_matcher =
          BfRtMatchersFactory::makeGroupMbrStsMatcher(mbr_sts);
      EXPECT_CALL(*pipe_mgr_obj,
                  pipeMgrSelGrpMbrsSet(_,
                                       _,
                                       _,
                                       sel_grp_hdl,
                                       single_random_data.getGroupMbrCount(),
                                       mbr_id_matcher,
                                       mbr_sts_matcher,
                                       _))
          .Times(1)
          .RetiresOnSaturation();

      // form TableKey and data from the index
      formTableKey_from_idx(sel_table, index, table_key.get());
      formTableData_from_idx(sel_table, index, table_data.get());

      // Modify entry in the table
      auto sts = sel_table.tableEntryMod(
          getDefaultSession(), getDefaultBfRtTarget(), *table_key, *table_data);
      EXPECT_EQ(sts, BF_SUCCESS)
          << "Table entry mod failed for entry : " << index
          << " Expected status : " << 0 << " Received status : " << sts << "\n";
    }
  }
}

TEST_P(BfRtSelectorTableTest, DISABLED_EntryDel) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t iter = 0; iter < d_tables.size(); iter++) {
    auto &sel_table = *d_tables[iter];
    auto &sel_table_entry_generator = *(entry_generator_map[d_tables[iter]]);

    BfRtTableScopeGuard scope_guard(*this,
                                    sel_table,
                                    sel_table_entry_generator,
                                    num_entries,
                                    pipe_mgr_obj,
                                    "EntryDel");

    // Add entries in the table
    // This set keeps track of all the member ids which we have installed in
    // in the action profile corresponding to this selector table
    std::unordered_set<uint32_t> seen_mbr_ids;
    // Before we try to add any entry in the selector table, we need to ensure
    // that those members have been added in the corresponding act_prof table.
    addEntriesInActProfTable(*sel_to_act_tbl[&sel_table], &seen_mbr_ids);

    // Once we have ensured that all the action member ids have been added in
    // the action profile table, add entries in the selector table
    std::map<pipe_sel_grp_hdl_t, int> group_hdl_list;
    EXPECT_SUCCESS(tableEntryAdd(num_entries, sel_table, &group_hdl_list));

    // Do one keyAllocate and dataAllocate and reset across uses. No need to
    // reset data object, since we delete entries in this test and expect a
    // OBJECT_NOT_FOUND error

    std::unique_ptr<BfRtTableKey> table_key;
    ASSERT_SUCCESS(sel_table.keyAllocate(&table_key));

    std::unique_ptr<BfRtTableData> table_data;
    ASSERT_SUCCESS(sel_table.dataAllocate(&table_data));

    for (auto iter_4 = group_hdl_list.begin(); iter_4 != group_hdl_list.end();
         iter_4++) {
      pipe_sel_grp_hdl_t sel_grp_hdl = iter_4->first;
      int index = iter_4->second;

      EXPECT_CALL(*pipe_mgr_obj, pipeMgrSelGrpDel(_, _, _, sel_grp_hdl, _))
          .Times(1)
          .RetiresOnSaturation();

      // form table key based on the index
      formTableKey_from_idx(sel_table, index, table_key.get());

      // Delete entry from the table
      auto sts = sel_table.tableEntryDel(
          getDefaultSession(), getDefaultBfRtTarget(), *table_key);
      EXPECT_EQ(sts, BF_SUCCESS)
          << "Table entry delete failed for entry : " << index
          << " Expected status : " << 0 << " Received status : " << sts << "\n";

      // Since BF-RT itself stores state for selector tables, we need to ensure
      // that that state gets updated properly. Thus try to read the entry
      // and expect it to fail
      EXPECT_EQ(sel_table.tableEntryGet(getDefaultSession(),
                                        getDefaultBfRtTarget(),
                                        *table_key,
                                        getDefaultTableReadFlag(),
                                        table_data.get()),
                BF_OBJECT_NOT_FOUND);
    }
  }
}

//
//-----------------------------------------------------------------------
// BF-RT TM Queue table test
void BfRtTMQueueTableTest::initialTestSetup(const BfRtTable &table,
                                            const BfRtEntryGen &entry_generator,
                                            const uint32_t &num_entries) {
  // Clear out all the vectors
  random_keys.clear();
  random_data.clear();

  // Get the randomly generator key and data objects
  tableRandomKeyGet(table, entry_generator, num_entries, &random_keys);
  tableRandomDataGet(table, entry_generator, num_entries, &random_data);
}

bf_status_t BfRtTMQueueTableTest::tableTest(const uint32_t &num_entries,
                                            const BfRtTable &table) {
  std::string table_name;
  table.tableNameGet(&table_name);
  std::unique_ptr<BfRtTableKey> table_key;

  // Do one keyAllocate and dataAllocate and reset across uses
  auto sts = table.keyAllocate(&table_key);
  EXPECT_EQ(sts, BF_SUCCESS)
      << "Key Allocate failed for table : " << table_name.c_str()
      << " Expected status : " << 0 << " Received status : " << sts << "\n";
  BF_RT_ASSERT(sts == BF_SUCCESS);

  // Data Allocate is done without action-ID here
  std::unique_ptr<BfRtTableData> table_data;
  sts = table.dataAllocate(&table_data);
  EXPECT_EQ(sts, BF_SUCCESS)
      << "Data Allocate failed for table : " << table_name.c_str()
      << " Expected status : " << 0 << " Received status : " << sts << "\n";
  return BF_SUCCESS;
}
// TODO: implement it
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BfRtTMQueueTableTest);

void BfRtPREMulticastNodeTableTest::initialTestSetup(
    const BfRtTable &table,
    const BfRtEntryGen &entry_generator,
    const uint32_t &num_entries) {
  // Clear out all the vectors
  random_keys.clear();
  random_data.clear();

  // Get the randomly generator key and data objects
  tableRandomKeyGet(table, entry_generator, num_entries, &random_keys);
  tableRandomDataGet(table, entry_generator, num_entries, &random_data);
}

TEST_P(BfRtPREMulticastNodeTableTest, EntryAllocateGet) {
  uint32_t num_entries = std::get<2>(GetParam());

  for (uint32_t n = 0; n < this->d_tables.size(); n++) {
    const auto &table = *this->d_tables[n];
    auto &entry_generator = *(entry_generator_map[this->d_tables[n]]);
    std::string table_name;
    EXPECT_SUCCESS(table.tableNameGet(&table_name));
    std::cout << ">>>>> Testing "
              << "Entry Add"
              << " for Table " << table_name << " <<<<<<\n";
    initialTestSetup(table, entry_generator, num_entries);

    // - Allocate 'num_entries' number of key and data objects
    // - Get 'num_entries' number of randomly generated key and data objects
    //   from the Generator
    // - For all the key and data objects, iterate over all the key and data
    //   fields respectively and set their values

    std::unique_ptr<BfRtTableKey> tb_key;
    std::unique_ptr<BfRtTableData> tb_data;

    // Do one keyAllocate and dataAllocate and reset across uses
    auto sts = table.keyAllocate(&tb_key);
    EXPECT_EQ(sts, BF_SUCCESS)
        << "Key Allocate failed for table : " << table_name.c_str()
        << " Expected status : " << 0 << " Received status : " << sts << "\n";
    BF_RT_ASSERT(sts == BF_SUCCESS);

    sts = table.dataAllocate(&tb_data);
    EXPECT_EQ(sts, BF_SUCCESS)
        << "Data Allocate failed for table : " << table_name.c_str()
        << " Expected status : " << 0 << " Received status : " << sts << "\n";

    for (uint32_t m = 0; m < 1; m++) {
      // Get Key object from random key
      formTableKey_from_idx(table, m, tb_key.get());

      // Get data object from random data
      formTableData_from_idx(table, m, tb_data.get());

      /*************Testing of Add Entry**************/
      uint64_t node_id = 0;
      bf_rt_id_t node_key_id;
      sts = table.keyFieldIdGet("$MULTICAST_NODE_ID", &node_key_id);
      EXPECT_EQ(sts, BF_SUCCESS);

      sts = tb_key->getValue(node_key_id, &node_id);
      EXPECT_EQ(sts, BF_SUCCESS);
    }
    EXPECT_SUCCESS(BF_SUCCESS);
  }
}

}  // namespace bfrt_test
}  // namespace bfrt
