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

#ifndef _TDI_TABLE_TEST_HPP_
#define _TDI_TABLE_TEST_HPP_

#include <bitset>
#include <unordered_set>

#include <tdi/tdi_init.hpp>

#include "tdi_pipe_mgr_intf_mock.hpp"
#include "tdi_tm_mgr_intf_mock.hpp"
#include "tdi_mc_mgr_intf_mock.hpp"
#include <tdi_common/tdi_init_impl.cpp>
#include <tdi_common/tdi_info_impl.hpp>
#include <tdi_common/tdi_table_impl.hpp>
#include <tdi_p4/tdi_p4_table_data_impl.hpp>
#include "tdi_matchers.hpp"
#include "tdi_matchers_factory.hpp"
#include "tdi_entry_gen.hpp"

namespace tdi {
namespace tdi_test {
namespace {
void packFieldIntoMatchSpecByteBuffer(const TdiTableKeyField &key_field,
                                      const size_t &size,
                                      const bool &do_masking,
                                      const uint8_t *field_buf,
                                      const uint8_t *field_mask_buf,
                                      uint8_t *match_spec_buf,
                                      uint8_t *match_mask_spec_buf) {
  // Get the start bit and the end bit of the concerned field
  size_t start_bit = key_field.getStartBit();
  size_t end_bit = start_bit + key_field.getSize() - 1;

  // Extract all the bits from field buf into the match spec byte buf between
  // the start bit and the end bit
  size_t j = 0;
  for (size_t i = start_bit; i <= end_bit; i++, j++) {
    uint8_t temp = (field_buf[size - (j / 8) - 1] >> (j % 8)) & 0x01;
    uint8_t temp_mask = 0x01;
    // This function is called even for EXM key fields for which the field mask
    // is defaulted to all zeros. So don't touch the temp_mask
    if (field_mask_buf != nullptr) {
      temp_mask = (field_mask_buf[size - (j / 8) - 1] >> (j % 8)) & 0x01;
      if (do_masking) {
        temp &= temp_mask;
      }
    }

    // Left shift the extracted bit into its correct location
    match_spec_buf[key_field.getParentFieldFullByteSize() - (i / 8) - 1] |=
        (temp << (i % 8));
    match_mask_spec_buf[key_field.getParentFieldFullByteSize() - (i / 8) - 1] |=
        (temp_mask << (i % 8));
  }
  // TODO: ideally we need to check whether the backend table is
  // ternary or not too. Need to find a way to implement a
  // mock pipeMgrTblIsTern
  if (!do_masking) {
    for (size_t index = 0; index < key_field.getParentFieldFullByteSize();
         index++) {
      match_mask_spec_buf[index] = 0xff;
    }
  }
}
}  // Anonymous namespace

#define MAX_TTL 0xffffffff
#define MIN_TTL 0
#define NUM_ENTRIES 100

#define ASSERT_SUCCESS(status) \
  ASSERT_THAT(status, TdiMatchersFactory::makeIsSuccessMatcher())
#define EXPECT_SUCCESS(status) \
  EXPECT_THAT(status, TdiMatchersFactory::makeIsSuccessMatcher())

using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::WithArgs;

class MatchSpec {
 public:
  MatchSpec(const GenericKey &random_key) {
    const auto &base_table = random_key.tableGet();
    const auto &table = static_cast<const TdiTableObj &>(base_table);

    // Allocate memory for the value and mask arrays
    size_t num_bytes = table.getKeySize().bytes;
    match_value_bits.reset(new uint8_t[num_bytes]());
    match_mask_bits.reset(new uint8_t[num_bytes]());
    pipe_ms.match_value_bits = match_value_bits.get();
    pipe_ms.match_mask_bits = match_mask_bits.get();
    pipe_ms.num_valid_match_bits = table.getKeySize().bits;
    pipe_ms.num_match_bytes = num_bytes;

    // Iterate over all the fields in the random key
    for (const auto &field_id : random_key.fieldIdListGet()) {
      const auto &generic_key_field = random_key.keyFieldGet(field_id);
      KeyFieldType key_type;
      EXPECT_SUCCESS(table.keyFieldTypeGet(field_id, &key_type));
      size_t bits_size;
      EXPECT_SUCCESS(table.keyFieldSizeGet(field_id, &bits_size));

      // convert size to bytes
      auto byte_size = (bits_size + 7) / 8;
      // The following check is to maintain sanity of the generator object
      EXPECT_EQ(byte_size, generic_key_field.sizeGet())
          << " Size of field : " << static_cast<int>(field_id)
          << " obtained from table : " << byte_size
          << " does not match the size from the generator : "
          << generic_key_field.sizeGet() << "\n";

      // Get the key field object from the table
      const TdiTableKeyField *table_key_field;
      EXPECT_SUCCESS(table.getKeyField(field_id, &table_key_field));
      switch (key_type) {
        case KeyFieldType::EXACT:
        case KeyFieldType::TERNARY:
        case KeyFieldType::RANGE: {
          std::string key_field_name;
          EXPECT_SUCCESS(table.keyFieldNameGet(field_id, &key_field_name));
          if (key_field_name == "$MATCH_PRIORITY") {
            uint32_t priority = 0;
            generic_key_field.getValue<uint32_t>(&priority);
            pipe_ms.priority = priority;
          } else {
            auto val_vec = generic_key_field.getValue();
            const auto &mask_vec = generic_key_field.getMask();
            if (key_type != KeyFieldType::RANGE) {
              for (uint32_t k = 0; k < val_vec.size(); k++) {
                val_vec[k] = val_vec[k] & mask_vec[k];
              }
            }

            if (table_key_field->isFieldSlice()) {
              packFieldIntoMatchSpecByteBuffer(
                  *table_key_field,
                  val_vec.size(),
                  key_type == KeyFieldType::RANGE ||
                          key_type == KeyFieldType::EXACT
                      ? false
                      : true,
                  val_vec.data(),
                  mask_vec.data(),
                  pipe_ms.match_value_bits + table_key_field->getOffset(),
                  pipe_ms.match_mask_bits + table_key_field->getOffset());
            } else {
              std::memcpy(
                  pipe_ms.match_value_bits + table_key_field->getOffset(),
                  val_vec.data(),
                  val_vec.size());
              std::memcpy(
                  pipe_ms.match_mask_bits + table_key_field->getOffset(),
                  mask_vec.data(),
                  val_vec.size());
            }
          }
          break;
        }
        case KeyFieldType::LPM: {
          auto val_vec = generic_key_field.getValue();
          uint16_t p_len = generic_key_field.getPrefixLen();
          EXPECT_LE(p_len, val_vec.size() * 8)
              << "Prefix length : " << p_len
              << " is greater than the size of the match field : "
              << val_vec.size() * 8
              << "for field : " << static_cast<int>(field_id) << "\n";
          pipe_ms.priority = bits_size - p_len;

          // Convert the prefix length to mask
          std::vector<std::bitset<8>> mask_vec(val_vec.size(), 0);
          for (auto &m : mask_vec) {
            if (p_len <= 0) {
              break;
            }
            for (int i = 7; i >= 0 && p_len > 0; i--) {
              m[i] = 1;
              p_len--;
            }
          }

          std::vector<uint8_t> temp_mask_vec;
          for (const auto &iter : mask_vec) {
            temp_mask_vec.push_back(iter.to_ulong());
          }
          for (uint32_t k = 0; k < val_vec.size(); k++) {
            val_vec[k] = val_vec[k] & temp_mask_vec[k];
          }
          if (table_key_field->isFieldSlice()) {
            packFieldIntoMatchSpecByteBuffer(
                *table_key_field,
                val_vec.size(),
                key_type == KeyFieldType::RANGE ? false : true,
                val_vec.data(),
                temp_mask_vec.data(),
                pipe_ms.match_value_bits + table_key_field->getOffset(),
                pipe_ms.match_mask_bits + table_key_field->getOffset());
          } else {
            std::memcpy(pipe_ms.match_value_bits + table_key_field->getOffset(),
                        val_vec.data(),
                        val_vec.size());
            std::memcpy(pipe_ms.match_mask_bits + table_key_field->getOffset(),
                        temp_mask_vec.data(),
                        val_vec.size());
          }
          break;
        }
        case KeyFieldType::INVALID:
        default:
          EXPECT_EQ(1, 0) << "Field : " << field_id << " Invalid key type : "
                          << static_cast<int>(key_type) << "\n";
      }
    }
  }

  pipe_tbl_match_spec_t *getPipeMatchSpec() { return &pipe_ms; }
  const pipe_tbl_match_spec_t *getPipeMatchSpec() const { return &pipe_ms; }

  std::unique_ptr<uint8_t> match_value_bits;
  std::unique_ptr<uint8_t> match_mask_bits;
  pipe_tbl_match_spec_t pipe_ms{0};
};

class ActionSpec {
 public:
  ActionSpec(const GenericData &single_random_data,
             const std::vector<tdi_id_t> &interested_fields) {
    const TdiTable &table = single_random_data.tableGet();
    const TdiTableObj &table_obj = static_cast<const TdiTableObj &>(table);

    auto action_id = single_random_data.actionIdGet();
    num_valid_action_data_bits_ = 0;
    action_data_bits = {};
    pipe_as = {0};

    TdiTable::TableType table_type;
    table_obj.tableTypeGet(&table_type);
    if (table_type == TdiTable::TableType::MATCH_DIRECT ||
        table_type == TdiTable::TableType::ACTION_PROFILE) {
      pipe_as.pipe_action_datatype_bmap |= PIPE_ACTION_DATA_TYPE;
    }

    for (const auto &field_id : interested_fields) {
      const auto &generic_data_field =
          single_random_data.dataFieldGet(field_id);
      const TdiTableDataField *table_data_field = nullptr;
      if (single_random_data.actionIdGet()) {
        EXPECT_SUCCESS(table_obj.getDataField(
            field_id, single_random_data.actionIdGet(), &table_data_field));
      } else {
        EXPECT_SUCCESS(table_obj.getDataField(field_id, &table_data_field));
      }
      auto types_vec = table_data_field->getTypes();
      size_t bits_field_size = table_data_field->getSize();
      auto byte_field_size = (bits_field_size + 7) / 8;

      EXPECT_EQ(byte_field_size, generic_data_field.sizeGet())
          << " Size of field : " << static_cast<int>(field_id)
          << " obtained from table : " << byte_field_size
          << " does not match the size from the generator : "
          << generic_data_field.sizeGet() << "\n";

      for (const auto &field_type : types_vec) {
        switch (field_type) {
          case (DataFieldType::ACTION_PARAM): {
            num_valid_action_data_bits_ += bits_field_size;
            auto data_arr = generic_data_field.getValue();
            action_data_bits.insert(
                action_data_bits.end(), data_arr.begin(), data_arr.end());
            break;
          }
          case (DataFieldType::ACTION_PARAM_OPTIMIZED_OUT): {
            num_valid_action_data_bits_ += bits_field_size;
            // Since this field has been optimized out by the compiler from the
            // context
            // json, tdi is going to send zeros for this field to pipe mgr.
            // Hence when
            // we read back this field, we are going to receive zeros. Hence we
            // do not
            // get the value from the generic_data_field to form the ground
            // truth copy
            // of this field
            std::vector<uint8_t> data_arr(byte_field_size, 0);
            action_data_bits.insert(
                action_data_bits.end(), data_arr.begin(), data_arr.end());
            break;
          }
          case (DataFieldType::COUNTER_INDEX):
          case (DataFieldType::REGISTER_INDEX):
          case (DataFieldType::METER_INDEX):
          case (DataFieldType::LPF_INDEX):
          case (DataFieldType::WRED_INDEX): {
            pipe_tbl_hdl_t res_hdl = table_obj.getResourceHdl(field_type);
            pipe_res_spec_t *res_spec;
            updateResourceSpec(res_hdl, &res_spec);
            res_spec->tbl_hdl = res_hdl;
            uint32_t tbl_idx = 0;
            generic_data_field.getValue<uint32_t>(&tbl_idx);
            res_spec->tbl_idx = tbl_idx;
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

            // For resource indices, we need to include dummy bytes (of value 0)
            // in the pipe action data byte array, as these indices appear as
            // p4_parameters in context json and pipe mgr uses this node to
            // construct the action spec. This requirement of dummy 0s is mainly
            // because of the HA constraints.
            // Now the resource index can also be an action param, in which case
            // it will be accounter for in the case DataFieldType::ACTION_PARAM.
            // So don't account for it here
            if (types_vec.size() > 1) break;
            num_valid_action_data_bits_ += bits_field_size;
            std::vector<uint8_t> data_arr(byte_field_size, 0);
            action_data_bits.insert(
                action_data_bits.end(), data_arr.begin(), data_arr.end());
            break;
          }
          case (DataFieldType::COUNTER_SPEC_BYTES):
          case (DataFieldType::COUNTER_SPEC_PACKETS): {
            pipe_tbl_hdl_t res_hdl = table_obj.getResourceHdl(field_type);
            pipe_res_spec_t *res_spec;
            updateResourceSpec(res_hdl, &res_spec);
            res_spec->tbl_hdl = res_hdl;
            res_spec->tbl_idx = 0;
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
            pipe_stat_data_t *counter_spec = &res_spec->data.counter;
            uint64_t val = 0;
            generic_data_field.getValue<uint64_t>(&val);
            if (field_type == DataFieldType::COUNTER_SPEC_BYTES) {
              counter_spec->bytes = val;
            } else {
              counter_spec->packets = val;
            }
            break;
          }
          case (DataFieldType::REGISTER_SPEC):
          case (DataFieldType::REGISTER_SPEC_HI):
          case (DataFieldType::REGISTER_SPEC_LO): {
            pipe_tbl_hdl_t res_hdl = table_obj.getResourceHdl(field_type);
            pipe_res_spec_t *res_spec;
            updateResourceSpec(res_hdl, &res_spec);
            res_spec->tbl_hdl = res_hdl;
            res_spec->tbl_idx = 0;
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
            break;
          }
          case (DataFieldType::METER_SPEC_CIR_PPS):
          case (DataFieldType::METER_SPEC_PIR_PPS):
          case (DataFieldType::METER_SPEC_CBS_PKTS):
          case (DataFieldType::METER_SPEC_PBS_PKTS):
          case (DataFieldType::METER_SPEC_CIR_KBPS):
          case (DataFieldType::METER_SPEC_PIR_KBPS):
          case (DataFieldType::METER_SPEC_CBS_KBITS):
          case (DataFieldType::METER_SPEC_PBS_KBITS): {
            pipe_tbl_hdl_t res_hdl = table_obj.getResourceHdl(field_type);
            pipe_res_spec_t *res_spec;
            updateResourceSpec(res_hdl, &res_spec);
            res_spec->tbl_hdl = res_hdl;
            res_spec->tbl_idx = 0;
            res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;

            uint64_t val = 0;
            generic_data_field.getValue<uint64_t>(&val);
            pipe_meter_spec_t *meter_spec = &res_spec->data.meter;
            if (field_type == DataFieldType::METER_SPEC_CIR_PPS) {
              meter_spec->cir.value.pps = val;
              meter_spec->cir.type = METER_RATE_TYPE_PPS;
            } else if (field_type == DataFieldType::METER_SPEC_PIR_PPS) {
              meter_spec->pir.value.pps = val;
              meter_spec->pir.type = METER_RATE_TYPE_PPS;
            } else if (field_type == DataFieldType::METER_SPEC_CBS_PKTS) {
              meter_spec->cburst = val;
            } else if (field_type == DataFieldType::METER_SPEC_PBS_PKTS) {
              meter_spec->pburst = val;
            } else if (field_type == DataFieldType::METER_SPEC_CIR_KBPS) {
              meter_spec->cir.value.kbps = val;
              meter_spec->cir.type = METER_RATE_TYPE_KBPS;
            } else if (field_type == DataFieldType::METER_SPEC_PIR_KBPS) {
              meter_spec->pir.value.kbps = val;
              meter_spec->pir.type = METER_RATE_TYPE_KBPS;
            } else if (field_type == DataFieldType::METER_SPEC_CBS_KBITS) {
              meter_spec->cburst = val;
            } else if (field_type == DataFieldType::METER_SPEC_PBS_KBITS) {
              meter_spec->pburst = val;
            } else {
              TDI_ASSERT(0);
            }
            break;
          }
          case DataFieldType::ACTION_MEMBER_ID: {
            pipe_as.pipe_action_datatype_bmap |= PIPE_ACTION_DATA_HDL_TYPE;
            uint32_t hdl = 0;
            generic_data_field.getValue<uint32_t>(&hdl);
            pipe_as.adt_ent_hdl = hdl;
            break;
          }
          case DataFieldType::SELECTOR_GROUP_ID: {
            pipe_as.pipe_action_datatype_bmap |= PIPE_SEL_GRP_HDL_TYPE;
            uint32_t hdl = 0;
            generic_data_field.getValue<uint32_t>(&hdl);
            pipe_as.sel_grp_hdl = hdl;
            break;
          }
          case DataFieldType::TTL: {
            // Since we don't store TTL in the action spec, nothing to do here
            break;
          }
          case DataFieldType::ENTRY_HIT_STATE: {
            // TODO : For the time being we are not testing this
            break;
          }
          case DataFieldType::LPF_SPEC_OUTPUT_SCALE_DOWN_FACTOR: {
            break;
          }
          case DataFieldType::WRED_SPEC_MIN_THRESHOLD:
          case DataFieldType::WRED_SPEC_MAX_THRESHOLD: {
            break;
          }
          default:
            EXPECT_EQ(0, 1)
                << "Invalid Datafield type for field : " << field_id << "\n";
        }
      }
    }

    // Assign the action_data_bits ptr
    // If action_id is 0, then allocate largest size of the action_data_bits
    // possible. Else just assign whatever was calculated above
    // Initialize the action data with all zeros
    if (!action_id) {
      pipe_as.act_data.num_valid_action_data_bits =
          table_obj.getMaxdataSzbits();
      action_data_bits = std::vector<uint8_t>(table_obj.getMaxdataSz(), 0);
      pipe_as.act_data.action_data_bits = action_data_bits.data();
      pipe_as.act_data.num_action_data_bytes = action_data_bits.size();
    } else {
      pipe_as.act_data.num_valid_action_data_bits = num_valid_action_data_bits_;
      pipe_as.act_data.action_data_bits = action_data_bits.data();
      pipe_as.act_data.num_action_data_bytes = action_data_bits.size();
    }
  }

  ActionSpec(const GenericData &single_random_data)
      : ActionSpec(single_random_data, single_random_data.fieldIdListGet()) {}

  pipe_res_spec_t *getResourceSpec(const pipe_tbl_hdl_t &tbl_hdl) {
    pipe_res_spec_t *res_spec = NULL;
    for (auto i = 0; i < pipe_as.resource_count; i++) {
      res_spec = &pipe_as.resources[i];
      if (res_spec->tbl_hdl == tbl_hdl) {
        break;
      } else {
        res_spec = NULL;
      }
    }
    return res_spec;
  }
  void updateResourceSpec(const pipe_tbl_hdl_t &tbl_hdl,
                          pipe_res_spec_t **res_spec) {
    *res_spec = getResourceSpec(tbl_hdl);
    if (!(*res_spec)) {
      *res_spec = &pipe_as.resources[pipe_as.resource_count++];
    }
  }

  pipe_action_spec_t *getPipeActionSpec() { return &pipe_as; }
  const pipe_action_spec_t *getPipeActionSpec() const { return &pipe_as; }

  std::vector<uint8_t> action_data_bits = {};
  size_t num_valid_action_data_bits_ = 0;
  pipe_action_spec_t pipe_as{0};
};

class TdiTableTest
    : public ::testing::TestWithParam<std::tuple<std::string,
                                                 std::vector<std::string>,
                                                 uint32_t,
                                                 std::string>> {
 public:
  TdiTableTest() {}

  ~TdiTableTest() {}

  void tdiDeviceAdd() {
    auto t = this->GetParam();
    std::string program_name = std::get<3>(t);
    std::string tdi_json_file_name = std::get<0>(t);
    std::string tdi_json_file_path =
        std::string(INSTALLDIR) +
        (program_name != "switch" ? "/tofinopd" : "") + std::string("/") +
        program_name + std::string("/") + tdi_json_file_name;

    std::vector<std::string> &context_json_file_names = std::get<1>(t);
    std::vector<std::string> context_json_file_paths;
    for (auto &filename : context_json_file_names) {
      std::string filepath = std::string(INSTALLDIR) +
                             (program_name != "switch" ? "/tofinopd" : "") +
                             std::string("/") + program_name +
                             std::string("/") + filename;
      context_json_file_paths.push_back(filepath);
    }

    // TODO For the time being we are creating a dummy dev_profile structure
    // to pass in to the device add call for tdi. This is guaranteed to only
    // work for single pipe and single pipe scope p4 programs. Later on, we
    // we need to change the input parameters of the test fixture to actaually
    // take the path of the conf file and then parse it and construct the
    // dev progfile structure.

    // Add a dummy device (with id 0) to tdi
    bf_device_profile_t dev_profile;
    dev_profile.num_p4_programs = 1;
    this->prog_name = program_name;
    std::strncpy(dev_profile.p4_programs[dev_id].prog_name,
                 prog_name.c_str(),
                 sizeof(dev_profile.p4_programs[dev_id].prog_name));
    dev_profile.p4_programs[dev_id].tdi_json_file =
        const_cast<char *>(tdi_json_file_path.c_str());
    dev_profile.p4_programs[dev_id].num_p4_pipelines =
        context_json_file_paths.size();
    std::string temp = std::string(TDI_SHARED_INSTALLDIR) + std::string("/");
    dev_profile.tdi_non_p4_json_dir_path = const_cast<char *>(temp.c_str());
    std::string dummy_binary_file_path = "dummy_binary_file_path";
    for (uint32_t i = 0; i < context_json_file_paths.size(); i++) {
      // Assign a name to the pipeline
      std::strncpy(
          dev_profile.p4_programs[dev_id].p4_pipelines[i].p4_pipeline_name,
          "pipe",
          sizeof(dev_profile.p4_programs[dev_id]
                     .p4_pipelines[i]
                     .p4_pipeline_name));
      dev_profile.p4_programs[dev_id].p4_pipelines[i].runtime_context_file =
          const_cast<char *>(context_json_file_paths[i].c_str());
      dev_profile.p4_programs[dev_id].p4_pipelines[i].cfg_file =
          const_cast<char *>(dummy_binary_file_path.c_str());
      dev_profile.p4_programs[dev_id].p4_pipelines[i].num_pipes_in_scope = 4;
    }

    bf_dev_family_t dev_family =
        TDI_DEV_FAMILY_TOFINO;  // We really dont care for this parameter
    ASSERT_SUCCESS(tdi_device_add(
        dev_id, dev_family, &dev_profile, nullptr, TDI_DEV_INIT_COLD));
  }

  void tdiDeviceRemove() { ASSERT_SUCCESS(tdi_device_remove(dev_id)); }

  virtual void SetUp() {
    // Get instance of the mock class
    pipe_mgr_obj = MockIPipeMgrIntf::getInstance();

    // Set the action on call to pipeMgrTblHdlPipeMaskGet
    ON_CALL(*pipe_mgr_obj, pipeMgrTblHdlPipeMaskGet(_, _, _, _))
        .WillByDefault(WithArgs<3>(Invoke([](uint32_t *mask) {
          *mask = 0;
          return PIPE_SUCCESS;
        })));

    // Before we carry on with the test, add the device to tdi
    tdiDeviceAdd();

    // Once the device has been added, get and cache the tdi info obj
    ASSERT_SUCCESS(
        TdiDevMgr::getInstance().tdiInfoGet(dev_id, prog_name, &tdiInfo));
    ASSERT_NE(tdiInfo, nullptr) << "Failed to open json files";

    static_cast<const TdiInfoImpl *>(tdiInfo)->tdiInfoGetTables(&b_tables);

    // Setup default session and tdi_target and the table read flag
    session = TdiSession::sessionCreate();
    tdi_tgt.dev_id = dev_id;
    tdi_tgt.pipe_id = pipe_id;
    flag = TdiTable::TdiTableGetFlag::GET_FROM_SW;

    // Set the seed for the random generator
    auto seed = time(NULL);
    std::cout << "Seed used is : " << static_cast<uint64_t>(seed) << "\n";
    TdiEntryGen::setRandomSeed(seed);
  }
  virtual void TearDown() {
    // Remove the device from tdi
    tdiDeviceRemove();

    // Before we destroy the MockIPipeMgrIntf object down below, reset
    // the shared_ptr for the session. This is important as the destructor
    // of Session actually uses the MockIPipeMgrIntf object
    session.reset();

    // Destroy the mock object which forces gmock to verify whether all
    // expectations on the mock object have been satisfied or not
    // and generate a failure if not
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }

  // Do some common initial setup before running test for each table separately
  virtual void initialTestSetup(const TdiTable &table,
                                const TdiEntryGen &entry_generator,
                                const uint32_t &num_entries) {
    // Clear out all the vectors
    random_keys.clear();
    random_data.clear();
    exp_ms_vec.clear();
    exp_as_vec.clear();

    // So whenever we are starting on a new test, we want all the state in the
    // Mock helper class (entry_database) to be empty as we want to emulate
    // a clean slate
    pipe_mgr_obj->getMockIPipeMgrIntfHelper().databaseCleanup();
  }

  TdiSession &getDefaultSession() { return *session; }

  tdi_target_t &getDefaultTdiTarget() { return tdi_tgt; }

  void formTableKey_from_idx(const TdiTable &table,
                             const int &index,
                             TdiTableKey *table_key);

  void formTableData_from_idx(const TdiTable &table,
                              const int &index,
                              TdiTableData *table_data);

 protected:
  TdiTable::TdiTableGetFlag &getDefaultTableReadFlag() { return flag; }

  void getTablesOfAType(const TdiTable::TableType &table_type_arg,
                        std::vector<TdiTable *> *table_vec) {
    for (auto *table : b_tables) {
      TdiTable::TableType t_type;
      static_cast<const TdiTableObj *>(table)->tableTypeGet(&t_type);
      if (t_type == table_type_arg) {
        table_vec->push_back(table);
      }
    }
  }

  // Given a vector of random data, this will return a vector of correctly
  // formed action specs which we should expect on call to table entry add
  void tableExpectedActionSpecGet(
      const TdiTable &table,
      const std::vector<GenericData> &random_data,
      std::vector<std::unique_ptr<ActionSpec>> *as_vec);
  // Given a vector of random keys, this will return a vector of correctly
  // formed match specs which we should expect on call to table entry add
  void tableExpectedMatchSpecGet(
      const TdiTable &table,
      const std::vector<GenericKey> &random_keys,
      std::vector<std::unique_ptr<MatchSpec>> *ms_vec);
  // Given a random data, table data and a list of intersted fields, we get the
  // corresponding values from the random object and set those in the table data
  // object
  void tableDataSetHelper(const TdiTable &table,
                          const GenericData &random_data,
                          TdiTableData &table_data,
                          const std::vector<tdi_id_t> &interested_fields);
  // Given a random data and table data object, this will set the fields
  // in the table data object by iterating over the field ids in the
  // random data object
  void tableDataSetHelper(const TdiTable &table,
                          const GenericData &random_data,
                          TdiTableData &table_data);
  // Given a random key and table key object, this will set the fields
  // in the table key object by iterating over the field ids in the random
  // key object
  void tableKeySetHelper(const GenericKey &random_key, TdiTableKey &table_key);
  // This will return a vector of random data objects by quering them from
  // the generator
  void tableRandomDataGet(const TdiTable &table,
                          const TdiEntryGen &entry_generator,
                          const uint32_t &num_entries,
                          std::vector<GenericData> *random_data);
  // This will return a vector of random key objects by quering them from
  // the generator
  void tableRandomKeyGet(const TdiTable &table,
                         const TdiEntryGen &entry_generator,
                         const uint32_t &num_entries,
                         std::vector<GenericKey> *random_keys);

  // This compares 2 key objects
  void compareKeyObjects(const TdiTableKey &obj_1,
                         const TdiTableKey &obj_2) const;
  // This compares 2 data objects
  void compareDataObjects(const TdiTableData &obj_1,
                          const TdiTableData &obj_2) const;

  const TdiInfo *tdiInfo{nullptr};
  std::vector<TdiTable *> b_tables;
  MockIPipeMgrIntf *pipe_mgr_obj{nullptr};
  tdi_dev_id_t dev_id{0};
  bf_dev_pipe_t pipe_id{0xffff};
  std::string prog_name;
  std::shared_ptr<TdiSession> session;
  tdi_target_t tdi_tgt;
  TdiTable::TdiTableGetFlag flag{TdiTable::TdiTableGetFlag::GET_FROM_SW};
  TdiTable::TableType table_type{TdiTable::TableType::INVALID};

  std::unordered_map<const TdiTable *, std::unique_ptr<TdiEntryGen>>
      entry_generator_map;
  std::vector<GenericKey> random_keys;
  std::vector<GenericData> random_data;
  std::vector<std::unique_ptr<MatchSpec>> exp_ms_vec;
  std::vector<std::unique_ptr<ActionSpec>> exp_as_vec;
};  // TdiTableTest

class TdiMatchActionTableTest : public TdiTableTest {
 protected:
  TdiMatchActionTableTest() {}
  void SetUp() override {
    // Call SetUp of the parent
    TdiTableTest::SetUp();
    // Get all the match action direct tables
    getTablesOfAType(TdiTable::TableType::MATCH_DIRECT, &d_tables);
    // Set the table type
    table_type = TdiTable::TableType::MATCH_DIRECT;
    // Setup the Generator objects for all the tables
    for (const auto *table : d_tables) {
      entry_generator_map.insert(
          std::make_pair(table, TdiEntryGen::makeGenerator(table)));
    }
  }

  void initialTestSetup(const TdiTable &table,
                        const TdiEntryGen &entry_generator,
                        const uint32_t &num_entries) override;

  void getTTL(const TdiTable &table,
              GenericData &single_random_data,
              uint64_t *ttl_val);
  tdi_status_t tableEntryAdd(const uint32_t &num_entries,
                             const TdiTable &table,
                             std::map<pipe_ent_hdl_t, int> *entry_hdls);
  tdi_status_t tableEntryModSwapActionData(
      const uint32_t &num_entries,
      const TdiTable &table,
      const std::map<pipe_ent_hdl_t, int> &entry_hdls);
  tdi_status_t tableEntryModIterateFields(
      const TdiTable &table, const std::map<pipe_ent_hdl_t, int> &entry_hdls);
  void tableEntryModIterateFieldsSetExpForFieldId(
      const TdiTable &table,
      const pipe_ent_hdl_t &ent_hdl,
      const int &index,
      const tdi_id_t &field_id,
      const TdiTableData *table_data_mod);
  void tableEntryModSetExpForField(const TdiTable &table,
                                   const pipe_ent_hdl_t &ent_hdl,
                                   const int &index,
                                   const bool &direct_resource_found,
                                   const bool &action_spec_found,
                                   const bool &ttl_found,
                                   const bool &direct_counter_found,
                                   const uint64_t &ttl_value_ground_truth);
  void tableEntryGetSetExpForEntry(const TdiTable &table,
                                   const GenericData &single_random_data,
                                   const int &times);

  std::vector<TdiTable *> d_tables;
};  // TdiMatchActionTableTest

INSTANTIATE_TEST_CASE_P(MatchActionTableTNAExactMatchTestSuite,
                        TdiMatchActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_exact_match")));
INSTANTIATE_TEST_CASE_P(MatchActionTableTNACounterTestSuite,
                        TdiMatchActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_counter")));
INSTANTIATE_TEST_CASE_P(MatchActionTableSwitchTestSuite,
                        TdiMatchActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "switch")));

class TdiActionTableTest : public TdiTableTest {
 protected:
  TdiActionTableTest() {}
  void SetUp() override {
    // Call SetUp of the parent
    TdiTableTest::SetUp();
    // Get all the action profile tables
    getTablesOfAType(TdiTable::TableType::ACTION_PROFILE, &d_tables);
    // Set the table type
    table_type = TdiTable::TableType::ACTION_PROFILE;
    // Setup the Generator objects for all the tables
    for (const auto *table : d_tables) {
      entry_generator_map.insert(
          std::make_pair(table, TdiEntryGen::makeGenerator(table)));
    }
  }

  void initialTestSetup(const TdiTable &table,
                        const TdiEntryGen &entry_generator,
                        const uint32_t &num_entries) override;

  tdi_status_t tableEntryAdd(const uint32_t &num_entries,
                             const TdiTable &table,
                             std::map<pipe_ent_hdl_t, int> *entry_hdls);

  std::vector<TdiTable *> d_tables;
};  // TdiActionTableTest

INSTANTIATE_TEST_CASE_P(ActionTableTNAActionSelectorTestSuite,
                        TdiActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_action_selector")));
INSTANTIATE_TEST_CASE_P(ActionTableTNAActionProfileTestSuite,
                        TdiActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_action_profile")));
INSTANTIATE_TEST_CASE_P(ActionTableSwitchTestSuite,
                        TdiActionTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "switch")));

class TdiSelectorTableTest : public TdiTableTest {
 protected:
  TdiSelectorTableTest() {}
  void SetUp() override {
    // Call SetUp of the parent
    TdiTableTest::SetUp();
    // Get all the selector tables
    getTablesOfAType(TdiTable::TableType::SELECTOR, &d_tables);
    // Set the table type
    table_type = TdiTable::TableType::SELECTOR;
    // FIXME Fgure out this mess of const TdiTable * vs. non const TdiTable *
    for (const auto *sel_tbl : d_tables) {
      auto act_prof_tbl_id =
          static_cast<const TdiTableObj *>(sel_tbl)->getActProfId();
      const TdiTable *act_tbl = nullptr;
      // Get the action profile table linked with this selector table
      EXPECT_SUCCESS(tdiInfo->tdiTableFromIdGet(act_prof_tbl_id, &act_tbl));
      EXPECT_NE(act_tbl, nullptr);
      sel_to_act_tbl.insert(std::make_pair(sel_tbl, act_tbl));
    }
    // Setup the Generator objects for all the tables
    for (const auto *table : d_tables) {
      // Selector table
      entry_generator_map.insert(
          std::make_pair(table, TdiEntryGen::makeGenerator(table)));
      // Action Profile table
      entry_generator_map.insert(
          std::make_pair(sel_to_act_tbl[table],
                         TdiEntryGen::makeGenerator(sel_to_act_tbl[table])));
    }
  }

  void initialTestSetup(const TdiTable &table,
                        const TdiEntryGen &entry_generator,
                        const uint32_t &num_entries) override;

  void addEntriesInActProfTable(const TdiTable &act_tbl,
                                std::unordered_set<uint32_t> *seen_mbr_ids);

  tdi_status_t tableEntryAdd(const uint32_t &num_entries,
                             const TdiTable &table,
                             std::map<pipe_ent_hdl_t, int> *entry_hdls);

  std::vector<TdiTable *> d_tables;
  std::unordered_map<const TdiTable * /*Selector table*/,
                     const TdiTable * /*Action profile table*/>
      sel_to_act_tbl;
  std::unordered_map<tdi_id_t, pipe_adt_ent_hdl_t> mbr_id_to_adt_ent_hdl_map;

 protected:
  std::vector<pipe_adt_ent_hdl_t> getPipeAdtEntHdlsFromActMbrIds(
      const std::vector<tdi_id_t> &mbr_ids) const {
    std::vector<pipe_adt_ent_hdl_t> adt_ent_hdls;
    for (const auto &mbr_id : mbr_ids) {
      if (mbr_id_to_adt_ent_hdl_map.find(mbr_id) ==
          mbr_id_to_adt_ent_hdl_map.end()) {
        // Indicates that we have messed up somewhere
        TDI_ASSERT(0);
      }
      adt_ent_hdls.push_back(mbr_id_to_adt_ent_hdl_map.at(mbr_id));
    }
    return adt_ent_hdls;
  }
};  // TdiSelectorTableTest

INSTANTIATE_TEST_CASE_P(SelectorTableTNAActionSelectorTestSuite,
                        TdiSelectorTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_action_selector")));
INSTANTIATE_TEST_CASE_P(SelectorTableSwitchTestSuite,
                        TdiSelectorTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "switch")));

class TdiTMQueueTableTest : public TdiTableTest {
 public:
  TdiTMQueueTableTest(){};
  ~TdiTMQueueTableTest(){};
  void SetUp() override {
    // Call SetUp of the parent
    TdiTableTest::SetUp();

    // Get instance of the mock class
    traffic_mgr_obj = MockITrafficMgrIntf::getInstance();

    // Get all the Queue tables
    getTablesOfAType(TdiTable::TableType::TM_QUEUE_CFG, &d_tables);
    // Set the table type
    table_type = TdiTable::TableType::TM_QUEUE_CFG;
    // Setup the Generator objects for all the tables
    for (const auto *table : d_tables) {
      entry_generator_map.insert(
          std::make_pair(table, TdiEntryGen::makeGenerator(table)));
    }
  }

  void initialTestSetup(const TdiTable &table,
                        const TdiEntryGen &entry_generator,
                        const uint32_t &num_entries) override;

  virtual void TearDown() {
    // Call TearDown of the parent
    TdiTableTest::TearDown();

    // Destroy the mock object which forces gmock to verify whether all
    // expectations on the mock object have been satisfied or not
    // and generate a failure if not
    std::unique_ptr<ITrafficMgrIntf> &unique_ptr_mock =
        MockITrafficMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }

  tdi_status_t tableTest(const uint32_t &num_entries, const TdiTable &table);

 protected:
  MockITrafficMgrIntf *traffic_mgr_obj{nullptr};
  std::vector<TdiTable *> d_tables;
};

INSTANTIATE_TEST_CASE_P(TrafficMgrTestSuite,
                        TdiTMQueueTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_action_selector")));  // TODO write a p4
                                                       // program to test TM and
                                                       // use it here until then
                                                       // using
                                                       // tna_action_selector
                                                       // program.

class TdiPREMulticastNodeTableTest : public TdiTableTest {
 public:
  TdiPREMulticastNodeTableTest(){};
  ~TdiPREMulticastNodeTableTest(){};
  void SetUp() override {
    // Call SetUp of the parent
    TdiTableTest::SetUp();
    this->mc_mgr_obj = MockIMcMgrIntf::getInstance();
    getTablesOfAType(TdiTable::TableType::PRE_NODE, &this->d_tables);
    // Set the table type
    this->table_type = TdiTable::TableType::PRE_NODE;
    // Setup the Generator objects for all the tables
    for (const auto *table : this->d_tables) {
      this->entry_generator_map.insert(
          std::make_pair(table, TdiEntryGen::makeGenerator(table)));
    }
  }
  void TearDown() override {
    TdiTableTest::TearDown();
    // Destroy the mock object which forces gmock to verify whether all
    // expectations on the mock object have been satisfied or not
    // and generate a failure if not
    std::unique_ptr<IMcMgrIntf> &unique_ptr_mock =
        MockIMcMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }

  void initialTestSetup(const TdiTable &table,
                        const TdiEntryGen &entry_generator,
                        const uint32_t &num_entries) override;

 protected:
  MockIMcMgrIntf *mc_mgr_obj{nullptr};
  std::vector<TdiTable *> d_tables;
};

INSTANTIATE_TEST_CASE_P(PREMulticastNodeTableTestSuite,
                        TdiPREMulticastNodeTableTest,
                        ::testing::Values(std::make_tuple(
                            "tdi.json",
                            std::vector<std::string>{"pipe/context.json"},
                            NUM_ENTRIES,
                            "tna_multicast")));

// This class is mainly concerned with doing some initial setup for running
// any test on a particular table. In the destructor of this class, we focibly
// verify expectations on the mock object for that table.
class TdiTableScopeGuard {
 public:
  TdiTableScopeGuard(TdiTableTest &test_table,
                     const TdiTable &table,
                     TdiEntryGen &entry_gen,
                     const uint32_t &num_entries,
                     MockIPipeMgrIntf *pipe_mgr_obj,
                     const std::string &test_case_name)
      : test_table_(test_table),
        table_(table),
        entry_gen_(entry_gen),
        num_entries_(num_entries),
        pipe_mgr_obj_(pipe_mgr_obj),
        test_case_name_(test_case_name) {
    std::string table_name;
    EXPECT_SUCCESS(table.tableNameGet(&table_name));
    std::cout << ">>>>> Testing " << test_case_name << " for Table "
              << table_name << " <<<<<<\n";

    // For the time being, if the table has idle table attributes, we are
    // setting
    // the notify mode on it. In future, we need to add negative tests which
    // will include not
    // setting the attribute and expecting set val on ttl field to fail and
    // such other tests
    std::set<TableAttributesType> type_set;
    EXPECT_SUCCESS(table.tableAttributesSupported(&type_set));
    for (auto ts : type_set) {
      switch (ts) {
        case TableAttributesType::IDLE_TABLE_RUNTIME: {
          std::unique_ptr<TdiTableAttributes> tbl_attr;
          EXPECT_SUCCESS(table.attributeAllocate(
              ts, TableAttributesIdleTableMode::NOTIFY_MODE, &tbl_attr));
          tbl_attr->idleTableNotifyModeSet(
              true, 0, 1234, MAX_TTL, MIN_TTL, nullptr);
          EXPECT_SUCCESS(
              table.tableAttributesSet(test_table.getDefaultSession(),
                                       test_table.getDefaultTdiTarget(),
                                       *tbl_attr));
          break;
        }
        default:
          break;
      }
    }

    test_table.initialTestSetup(table, entry_gen, num_entries);
  }
  ~TdiTableScopeGuard() {
    // Force verification of all the expectations for this table before we move
    // onto the next one
    Mock::VerifyAndClearExpectations(pipe_mgr_obj_);
  }

 private:
  TdiTableTest &test_table_;
  const TdiTable &table_;
  TdiEntryGen &entry_gen_;
  const uint32_t num_entries_;
  MockIPipeMgrIntf *pipe_mgr_obj_{nullptr};
  std::string test_case_name_{""};
};  // TdiTableScopeGuard

}  // namespace tdi_test
}  // namespace tdi
#endif  // _TDI_TABLE_TEST_HPP_
