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

#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include <../bf_rt_common/bf_rt_info_impl.hpp>
#include <../bf_rt_common/bf_rt_cjson.hpp>
#include <../bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <../bf_rt_common/bf_rt_table_impl.hpp>

#include "bf_rt_info_test.hpp"

// using ::testing::WithParamInterface;
namespace bfrt {
namespace bfrt_test {

namespace {
void verify_string_choices(
    const std::string &table_name,
    const std::vector<std::reference_wrapper<const std::string>> &choices,
    std::set<std::string> *exp_choices) {
  for (const auto &iter : choices) {
    if (exp_choices->find(iter) != exp_choices->end()) {
      exp_choices->erase(iter);
    }
  }
  if (exp_choices->size() != 0) {
    EXPECT_EQ(exp_choices->size(), 0)
        << "All expected choices were not read for table " << table_name
        << "\n";
    std::cout << "The choices that were not read are \n";
    for (const auto &iter : *exp_choices) {
      std::cout << iter << "\n";
    }
    std::cout << "\n";
  }
}
}  // Anonymous namespace

// Testing whether pipeline profiles have been added correctly
TEST_P(BfRtInfoTest, TestProgramPipelineProfiles) {
  PipelineProfInfoVec prof_vec;
  auto status = bfrtInfo->bfRtInfoPipelineInfoGet(&prof_vec);
  if (program_name == "tna_32q_2pipe") {
    EXPECT_EQ(prof_vec[0].first.get(), "pipeline_profile_a");
    EXPECT_EQ(prof_vec[1].first.get(), "pipeline_profile_b");

    EXPECT_THAT(prof_vec[0].second.get(), ::testing::ElementsAre(0, 3));
    EXPECT_THAT(prof_vec[1].second.get(), ::testing::ElementsAre(1, 2));
  } else if (program_name == "tna_counter") {
    EXPECT_EQ(prof_vec[0].first.get(), "pipe");
    EXPECT_THAT(prof_vec[0].second.get(), ::testing::ElementsAre(0, 1, 2, 3));
  }
}

TEST_P(BfRtFixedInfoTest, TestTableDataFieldStringChoices) {
  std::vector<const BfRtTable *> table_vec;
  ASSERT_EQ(bfrtFixedInfo->bfrtInfoGetTables(&table_vec), BF_SUCCESS);
  for (const auto *table : table_vec) {
    bf_rt_id_t table_id;
    std::string table_name;
    ASSERT_EQ(table->tableIdGet(&table_id), BF_SUCCESS);
    ASSERT_EQ(table->tableNameGet(&table_name), BF_SUCCESS);

    if (/* table_name == "tm.port.group_cfg" || */ /* Without a key. */
        table_name == "tm.port.group") {
      // Check if key fields has been parsed corectly
      bf_rt_id_t key_field_id;
      ASSERT_EQ(table->keyFieldIdGet("pg_id", &key_field_id), BF_SUCCESS);
      ASSERT_EQ(key_field_id, 1);
    }

    if (table_name == "tm.port.group_cfg") {
      // Verify fields without choices.
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t field_id;

      ASSERT_EQ(table->dataFieldIdGet("ingress_qid_max", &field_id),
                BF_SUCCESS);
      // Try to get choices for a field without choices (should fail).
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                BF_INVALID_ARG)
          << "Falsely giving choices for field " << field_id
          << " which is not a string\n";

      ASSERT_EQ(table->dataFieldIdGet("pg_queues", &field_id), BF_SUCCESS);
      // Try to get choices for a field without choices (should fail).
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                BF_INVALID_ARG)
          << "Falsely giving choices for field " << field_id
          << " which is not a string\n";
    }

    if (table_name == "tm.port.group") {
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t action_id;
      bf_rt_id_t field_id;
      std::set<std::string> vf_names_common{"pg_dev_ports", "port_queue_count"};
      //---
      ASSERT_EQ(table->actionIdGet("seq", &action_id), BF_SUCCESS);
      // Fields without choices
      for (const auto &fld_n : vf_names_common) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
      //---
      ASSERT_EQ(table->actionIdGet("even", &action_id), BF_SUCCESS);
      // Fields without choices
      for (const auto &fld_n : vf_names_common) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
      //---
      ASSERT_EQ(table->actionIdGet("crop", &action_id), BF_SUCCESS);
      // Fields without choices
      for (const auto &fld_n : vf_names_common) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
      //---
      ASSERT_EQ(table->actionIdGet("map", &action_id), BF_SUCCESS);
      // Fields without choices
      for (const auto &fld_n : vf_names_common) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
      auto j = GetParam();
      int pg_ports = 4;  // TF1, TF3
      if (std::find(j.begin(), j.end(), "bf_rt_tm_tf2.json") != j.end()) {
        pg_ports = 8;  // TF2
      }
      std::stringstream f_name;
      for (int i = 0; i < pg_ports; i++) {
        f_name.str("");
        f_name << "ingress_qid_map_" << (int)(i);
        ASSERT_EQ(table->dataFieldIdGet(f_name.str(), action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "falsely giving choices for field " << field_id
            << " which is not a string\n";

        f_name.str("");
        f_name << "egress_qid_queues_" << (int)(i);
        ASSERT_EQ(table->dataFieldIdGet(f_name.str(), action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
    }

    if (table_name == "tm.queue.cfg" || table_name == "tm.queue.map" ||
        table_name == "tm.queue.color" || table_name == "tm.queue.buffer") {
      // Check if key fields has been parsed corectly
      bf_rt_id_t key_field_id;
      ASSERT_EQ(table->keyFieldIdGet("pg_id", &key_field_id), BF_SUCCESS);
      ASSERT_EQ(key_field_id, 1);

      ASSERT_EQ(table->keyFieldIdGet("pg_queue", &key_field_id), BF_SUCCESS);
      ASSERT_EQ(key_field_id, 2);
    }

    if (table_name == "tm.queue.cfg") {
      // Verify fields without choices.
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t field_id;

      ASSERT_EQ(table->dataFieldIdGet("mirror_drop_destination", &field_id),
                BF_SUCCESS);
      // Try to get choices for a field without choices (should fail).
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                BF_INVALID_ARG)
          << "Falsely giving choices for field " << field_id
          << " which is not a string\n";

      ASSERT_EQ(table->dataFieldIdGet("pfc_cos", &field_id), BF_SUCCESS);
      // Try to get choices for a field without choices (should fail).
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                BF_INVALID_ARG)
          << "Falsely giving choices for field " << field_id
          << " which is not a string\n";
    }

    if (table_name == "tm.queue.map") {
      // Verify fields without choices.
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t field_id;

      std::set<std::string> vf_names{"dev_port",
                                     "queue_nr",
                                     "ingress_qid_max",
                                     "ingress_qid_count",
                                     "ingress_qid_list"};

      for (const auto &fld_n : vf_names) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, &field_id), BF_SUCCESS);
        // Try to get choices for a field without choices (should fail).
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
    }

    if (table_name == "tm.queue.color") {
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t field_id;
      std::set<std::string> color_limit_choices{
          "12.5%", "25%", "37.5%", "50%", "62.5%", "75%", "87.5%", "100%"};

      // Verify fields with choices.
      std::set<std::string> vc_names{"drop_limit_yellow",
                                     "drop_limit_red",
                                     "hysteresis_yellow",
                                     "hysteresis_red"};
      for (const auto &fld_n : vc_names) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";
        verify_string_choices(table_name, choices, &color_limit_choices);
      }
      // Verify fields without choices.
      std::set<std::string> vf_names{"drop_enable"};

      auto j = GetParam();
      if (std::find(j.begin(), j.end(), "bf_rt_tm_tf2.json") != j.end() ||
          std::find(j.begin(), j.end(), "bf_rt_tm_tf3.json") != j.end()) {
        vf_names.insert("drop_visible");
      }
      for (const auto &fld_n : vf_names) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_INVALID_ARG)
            << "falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
    }

    if (table_name == "tm.queue.buffer") {
      std::vector<std::reference_wrapper<const std::string>> choices;
      bf_rt_id_t action_id;
      bf_rt_id_t field_id;

      std::set<std::string> pool_choices{
          "EG_APP_POOL_0", "EG_APP_POOL_1", "EG_APP_POOL_2", "EG_APP_POOL_3"};

      std::set<std::string> baf_choices{"1.5%",
                                        "3%",
                                        "6%",
                                        "11%",
                                        "20%",
                                        "33%",
                                        "50%",
                                        "66%",
                                        "80%",
                                        "DISABLE"};

      std::set<std::string> vf_names_pool{"guaranteed_cells",
                                          "hysteresis_cells",
                                          "tail_drop_enable",
                                          "pool_max_cells"};

      std::set<std::string> vf_names_buffer{
          "guaranteed_cells", "hysteresis_cells", "tail_drop_enable"};
      //---
      ASSERT_EQ(table->actionIdGet("shared_pool", &action_id), BF_SUCCESS);

      // Fields without choices
      for (const auto &fld_n : vf_names_pool) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }

      // Verify fields with choices.
      ASSERT_EQ(table->dataFieldIdGet("dynamic_baf", action_id, &field_id),
                BF_SUCCESS);
      EXPECT_EQ(
          table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
          BF_SUCCESS)
          << "Unable to get allowed choices for field " << field_id << "\n";
      verify_string_choices(table_name, choices, &baf_choices);

      ASSERT_EQ(table->dataFieldIdGet("pool_id", action_id, &field_id),
                BF_SUCCESS);
      EXPECT_EQ(
          table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
          BF_SUCCESS)
          << "Unable to get allowed choices for field " << field_id << "\n";
      verify_string_choices(table_name, choices, &pool_choices);

      //---
      ASSERT_EQ(table->actionIdGet("buffer_only", &action_id), BF_SUCCESS);
      // Fieldas without choices
      for (const auto &fld_n : vf_names_buffer) {
        ASSERT_EQ(table->dataFieldIdGet(fld_n, action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
      }
    }

    if (table_name == "tm.ppg") {
      // Check if key fields dev_port and ppg_id has been parsed corectly
      bf_rt_id_t key_field_id;
      ASSERT_EQ(table->keyFieldIdGet("$dev_port", &key_field_id), BF_SUCCESS);
      ASSERT_EQ(key_field_id, 1);

      ASSERT_EQ(table->keyFieldIdGet("$ppg_id", &key_field_id), BF_SUCCESS);
      ASSERT_EQ(key_field_id, 2);

      // Verify if Pool Id choices for data field app_pool_id match
      std::vector<std::reference_wrapper<const std::string>> pool_choices;
      std::set<std::string> exp_pool_choices{"BF_TM_IG_APP_POOL_0",
                                             "BF_TM_IG_APP_POOL_1",
                                             "BF_TM_IG_APP_POOL_2",
                                             "BF_TM_IG_APP_POOL_3"};

      bf_rt_id_t field_id = 7;
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &pool_choices),
                BF_SUCCESS)
          << "Unable to get allowed choices for field " << field_id << "\n";
      verify_string_choices(table_name, pool_choices, &exp_pool_choices);

      // Verify if the possible values of Dynamic BAF value are parsed correctly
      std::vector<std::reference_wrapper<const std::string>> baf_choices;
      std::set<std::string> exp_baf_choices{"BF_TM_PPG_BAF_1_POINT_5_PERCENT",
                                            "BF_TM_PPG_BAF_3_PERCENT",
                                            "BF_TM_PPG_BAF_6_PERCENT",
                                            "BF_TM_PPG_BAF_11_PERCENT",
                                            "BF_TM_PPG_BAF_20_PERCENT",
                                            "BF_TM_PPG_BAF_33_PERCENT",
                                            "BF_TM_PPG_BAF_50_PERCENT",
                                            "BF_TM_PPG_BAF_66_PERCENT",
                                            "BF_TM_PPG_BAF_80_PERCENT",
                                            "BF_TM_PPG_BAF_DISABLE"};
      field_id = 9;
      EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &baf_choices),
                BF_SUCCESS)
          << "Unable to get allowed choices for field " << field_id << "\n";
      verify_string_choices(table_name, baf_choices, &exp_baf_choices);
    }

    if (table_name == "$PORT") {
      {
        std::vector<std::reference_wrapper<const std::string>> choices;
        std::set<std::string> exp_choices{"BF_SPEED_NONE",
                                          "BF_SPEED_1G",
                                          "BF_SPEED_10G",
                                          "BF_SPEED_25G",
                                          "BF_SPEED_40G",
                                          "BF_SPEED_50G",
                                          "BF_SPEED_100G",
                                          "BF_SPEED_200G",
                                          "BF_SPEED_400G"};

        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$N_LANES", &field_id), BF_SUCCESS);
        // Try to get choices for a field which is not a string. So this should
        // fail
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
        // Finally get the choices correctly
        ASSERT_EQ(table->dataFieldIdGet("$SPEED", &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";

        // Verify if we read all the correct choices
        verify_string_choices(table_name, choices, &exp_choices);
      }  // $SPEED

      {
        std::vector<std::reference_wrapper<const std::string>> choices;
        std::set<std::string> exp_choices{"BF_FEC_TYP_NONE",
                                          "BF_FEC_TYP_FIRECODE",
                                          "BF_FEC_TYP_REED_SOLOMON",
                                          "BF_FEC_TYP_FC",
                                          "BF_FEC_TYP_RS"};

        // Get the choices correctly
        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$FEC", &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";

        // Verify if we read all the correct choices
        verify_string_choices(table_name, choices, &exp_choices);
      }  // $FEC

      {
        std::vector<std::reference_wrapper<const std::string>> choices;
        std::set<std::string> exp_choices{"BF_LPBK_NONE",
                                          "BF_LPBK_MAC_NEAR",
                                          "BF_LPBK_MAC_FAR",
                                          "BF_LPBK_PCS_NEAR",
                                          "BF_LPBK_SERDES_NEAR",
                                          "BF_LPBK_SERDES_FAR"};

        // Get the choices correctly
        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$LOOPBACK_MODE", &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";

        // Verify if we read all the correct choices
        verify_string_choices(table_name, choices, &exp_choices);
      }  // $LOOPBACK_MODE

      {
        std::vector<std::reference_wrapper<const std::string>> choices;
        std::set<std::string> exp_choices{"PM_PORT_DIR_DEFAULT",
                                          "PM_PORT_DIR_TX_ONLY",
                                          "PM_PORT_DIR_RX_ONLY"};

        // Get the choices correctly
        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$PORT_DIR", &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";

        // Verify if we read all the correct choices
        verify_string_choices(table_name, choices, &exp_choices);
      }  // $PORT_DIR

      {
        std::vector<std::reference_wrapper<const std::string>> choices;
        std::set<std::string> exp_choices{"BF_MEDIA_TYPE_COPPER",
                                          "BF_MEDIA_TYPE_OPTICAL"};

        // Get the choices correctly
        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$MEDIA_TYPE", &field_id), BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";

        // Verify if we read all the correct choices
        verify_string_choices(table_name, choices, &exp_choices);
      }  // $MEDIA_TYPE
    }
  }
}

TEST_P(BfRtInfoTest, TestTableKeyFieldStringChoices) {
  if (program_name != "tna_idletimeout") {
    return;
  }
  std::vector<const BfRtTable *> table_vec;
  ASSERT_EQ(bfrtInfo->bfrtInfoGetTables(&table_vec), BF_SUCCESS);
  for (const auto *table : table_vec) {
    bf_rt_id_t table_id;
    std::string table_name;
    ASSERT_EQ(table->tableIdGet(&table_id), BF_SUCCESS);
    ASSERT_EQ(table->tableNameGet(&table_name), BF_SUCCESS);

    std::vector<std::reference_wrapper<const std::string>> choices;
    std::set<std::string> exp_choices;
    if (table_name == "pipe0.$SNAPSHOT_EGRESS_LIVENESS") {
      bf_rt_id_t field_id;
      ASSERT_EQ(
          table->keyFieldIdGet("$SNAPSHOT_LIVENESS_FIELD_NAME", &field_id),
          BF_SUCCESS);
      // Try to get the choices correctly
      EXPECT_EQ(table->keyFieldAllowedChoicesGet(field_id, &choices),
                BF_SUCCESS)
          << "Unable to get allowed choices for field " << field_id
          << " for table " << table_name << "\n";
      // Since this table does not have choices published in the bfrt json
      // the returned vector should be empty
      EXPECT_EQ(choices.size(), 0)
          << "Falsely giving choices for a field " << field_id
          << " which does not exist for table " << table_name << "\n";
    } else if (table_name == "pipe0.$SNAPSHOT_INGRESS") {
      bf_rt_id_t field_id;
      ASSERT_EQ(table->keyFieldIdGet("$SNAPSHOT_TRIGGER_STAGE", &field_id),
                BF_SUCCESS);
      // Try to get choices for an incorrect field. Since the field
      // is not a string this should fail
      EXPECT_EQ(table->keyFieldAllowedChoicesGet(field_id, &choices),
                BF_INVALID_ARG)
          << "Falsely giving choices for field " << field_id
          << " which is not a string\n";
    }
    // Verify if we read all the correct choices
    verify_string_choices(table_name, choices, &exp_choices);
  }
}

TEST_P(BfRtInfoTest, TestTableDataFieldStringChoices) {
  if (program_name != "tna_meter_lpf_wred" &&
      program_name != "tna_idletimeout") {
    return;
  }
  std::vector<const BfRtTable *> table_vec;
  ASSERT_EQ(bfrtInfo->bfrtInfoGetTables(&table_vec), BF_SUCCESS);
  for (const auto *table : table_vec) {
    bf_rt_id_t table_id;
    std::string table_name;
    ASSERT_EQ(table->tableIdGet(&table_id), BF_SUCCESS);
    ASSERT_EQ(table->tableNameGet(&table_name), BF_SUCCESS);

    std::vector<std::reference_wrapper<const std::string>> choices;
    std::set<std::string> exp_choices;
    if (program_name == "tna_idletimeout") {
      if (table_name == "SwitchIngress.dmac") {
        // Set the expected values
        exp_choices.insert("ENTRY_IDLE");
        exp_choices.insert("ENTRY_ACTIVE");

        bf_rt_id_t field_id;
        bf_rt_id_t action_id;
        ASSERT_EQ(table->actionIdGet("SwitchIngress.hit", &action_id),
                  BF_SUCCESS);
        ASSERT_EQ(table->dataFieldIdGet("$ENTRY_TTL", action_id, &field_id),
                  BF_SUCCESS);
        // Try to get choices for a field which is not a string. So this should
        // fail
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
        // Try to get choices for a correct field but dont mention the action
        // id. Since the
        // the table has an action this should fail
        ASSERT_EQ(
            table->dataFieldIdGet("$ENTRY_HIT_STATE", action_id, &field_id),
            BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_NOT_SUPPORTED)
            << "Falsely giving choices for field " << field_id
            << " even when no action id is mentioned\n";
        // Finally get the choices correctly
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id
            << " with action id " << action_id << "\n";
      } else if (table_name == "SwitchIngress.dmac_indirect") {
        // Set the expected values
        exp_choices.insert("ENTRY_IDLE");
        exp_choices.insert("ENTRY_ACTIVE");

        bf_rt_id_t field_id;
        ASSERT_EQ(table->dataFieldIdGet("$ENTRY_TTL", &field_id), BF_SUCCESS);
        // Try to get choices for a field which is not a string. So this should
        // fail
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
        // Try to get choices for a correct field but mention the action id.
        // Since the
        // the table does not have an action this should fail
        ASSERT_EQ(table->dataFieldIdGet("$ENTRY_HIT_STATE", &field_id),
                  BF_SUCCESS);
        bf_rt_id_t action_id = 1;
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_NOT_SUPPORTED)
            << "Falsely giving choices for field " << field_id
            << " when action id is mentioned\n";
        // Finally get the choices correctly
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";
      }
    } else if (program_name == "tna_meter_lpf_wred") {
      if (table_name == "SwitchIngress.direct_lpf_match_tbl") {
        // Set the expected values
        exp_choices.insert("RATE");
        exp_choices.insert("SAMPLE");

        bf_rt_id_t field_id;
        bf_rt_id_t action_id;
        ASSERT_EQ(
            table->actionIdGet("SwitchIngress.set_rate_direct", &action_id),
            BF_SUCCESS);
        ASSERT_EQ(table->dataFieldIdGet(
                      "$LPF_SPEC_GAIN_TIME_CONSTANT_NS", action_id, &field_id),
                  BF_SUCCESS);
        // Try to get choices for a field which is not a string. So this should
        // fail
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
        // Try to get choices for a correct field but dont mention the action
        // id. Since the
        // the table has an action this should fail
        ASSERT_EQ(table->dataFieldIdGet("$LPF_SPEC_TYPE", action_id, &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_NOT_SUPPORTED)
            << "Falsely giving choices for field " << field_id
            << " even when no action id is mentioned\n";
        // Finally get the choices correctly
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id
            << " with action id " << action_id << "\n";
      } else if (table_name == "SwitchIngress.simple_lpf") {
        // Set the expected values
        exp_choices.insert("RATE");
        exp_choices.insert("SAMPLE");

        bf_rt_id_t field_id;
        ASSERT_EQ(
            table->dataFieldIdGet("$LPF_SPEC_GAIN_TIME_CONSTANT_NS", &field_id),
            BF_SUCCESS);
        // Try to get choices for a field which is not a string. So this should
        // fail
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_INVALID_ARG)
            << "Falsely giving choices for field " << field_id
            << " which is not a string\n";
        // Try to get choices for a correct field but mention the action id.
        // Since the
        // the table does not have an action this should fail
        bf_rt_id_t action_id = 1;
        ASSERT_EQ(table->dataFieldIdGet("$LPF_SPEC_TYPE", &field_id),
                  BF_SUCCESS);
        EXPECT_EQ(
            table->dataFieldAllowedChoicesGet(field_id, action_id, &choices),
            BF_NOT_SUPPORTED)
            << "Falsely giving choices for field " << field_id
            << " when action id is mentioned\n";
        // Finally get the choices correctly
        EXPECT_EQ(table->dataFieldAllowedChoicesGet(field_id, &choices),
                  BF_SUCCESS)
            << "Unable to get allowed choices for field " << field_id << "\n";
      }
    }
    // Verify if we read all the correct choices
    verify_string_choices(table_name, choices, &exp_choices);
  }
}

// Testing whether BfRt can handle non fully resolved names if sufficiently
// unique
TEST_P(BfRtInfoTest, TestNameUniqueness) {
  if (program_name == "tna_32q_2pipe") {
    const BfRtTable *table;
    // Looking just for forward table should fail
    EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet("forward", &table),
              BF_OBJECT_NOT_FOUND);
    // Looking for SwitchEgress_a.forward or SwitchEgress_b.forward
    // should pass
    EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet(
                  "pipeline_profile_a.SwitchEgress_a.forward", &table),
              BF_SUCCESS);
    EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet(
                  "pipeline_profile_b.SwitchEgress_b.forward", &table),
              BF_SUCCESS);
    // Looking for pipeline_profile_a.SwitchEgress_a.forward should pass

    // FIXME Commenting out the following until the time when the corresponding
    // changes in the compiler are merged
    // EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet(
    //              "pipeline_profile_a.SwitchEgress_a.forward", &table),
    //          BF_SUCCESS);

    // Same checks for learn obj
    const BfRtLearn *learn;
    // FIXME Commenting out the following until the time when the corresponding
    // changes in the compiler are merged
    // EXPECT_EQ(bfrtInfo->bfrtLearnFromNameGet(
    //              "pipeline_profile_b.SwitchIngressDeparser_b.digest",
    //              &learn),
    //          BF_SUCCESS);
    EXPECT_EQ(bfrtInfo->bfrtLearnFromNameGet(
                  "pipeline_profile_b.SwitchIngressDeparser_b.digest", &learn),
              BF_SUCCESS);
  } else if (program_name == "tna_counter") {
    const BfRtTable *table;
    // Looking just for forward table should pass
    EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet("forward", &table), BF_SUCCESS);
    // Looking for pipe.SwitchIngress.forward should pass
    EXPECT_EQ(bfrtInfo->bfrtTableFromNameGet("SwitchIngress.forward", &table),
              BF_SUCCESS);
  } else {
    return;
  }
}

// Testing whether we have formed the table dependency graph
// correctly
TEST_P(BfRtInfoTest, TestTableDependencies) {
  std::vector<const BfRtTable *> table_vec;
  ASSERT_EQ(bfrtInfo->bfrtInfoGetTables(&table_vec), BF_SUCCESS);
  for (const auto *table : table_vec) {
    bf_rt_id_t table_id;
    std::string table_name;
    ASSERT_EQ(table->tableIdGet(&table_id), BF_SUCCESS);
    ASSERT_EQ(table->tableNameGet(&table_name), BF_SUCCESS);

    if (program_name == "tna_action_selector") {
      // Check the tables that this table depends on
      {
        std::vector<bf_rt_id_t> depends_on_list;
        ASSERT_EQ(bfrtInfo->bfrtInfoTablesThisTableDependsOnGet(
                      table_id, &depends_on_list),
                  BF_SUCCESS);
        if (table_name == "SwitchIngress.forward") {
          EXPECT_EQ(depends_on_list.size(), 2);
          bool action_table_found = false;
          bool selector_table_found = false;
          for (const auto &iter : depends_on_list) {
            const BfRtTable *temp_table;
            EXPECT_EQ(bfrtInfo->bfrtTableFromIdGet(iter, &temp_table),
                      BF_SUCCESS);
            std::string temp_table_name;
            EXPECT_EQ(temp_table->tableNameGet(&temp_table_name), BF_SUCCESS);
            if (temp_table_name == "SwitchIngress.action_selector") {
              action_table_found = true;
            }
            if (temp_table_name == "SwitchIngress.action_selector_sel") {
              selector_table_found = true;
            }
          }
          EXPECT_EQ(action_table_found, true);
          EXPECT_EQ(selector_table_found, true);
        } else if (table_name == "SwitchIngress.action_selector") {
          EXPECT_EQ(depends_on_list.size(), 0);
        } else if (table_name == "SwitchIngress.action_selector_sel") {
          EXPECT_EQ(depends_on_list.size(), 1);
          const BfRtTable *temp_table;
          EXPECT_EQ(
              bfrtInfo->bfrtTableFromIdGet(depends_on_list[0], &temp_table),
              BF_SUCCESS);
          std::string temp_table_name;
          EXPECT_EQ(temp_table->tableNameGet(&temp_table_name), BF_SUCCESS);
          EXPECT_EQ(temp_table_name, "SwitchIngress.action_selector");
        }
      }

      // Check the tables that depend on this table
      {
        std::vector<bf_rt_id_t> tables_depend_on_this_table_list;
        EXPECT_EQ(bfrtInfo->bfrtInfoTablesDependentOnThisTableGet(
                      table_id, &tables_depend_on_this_table_list),
                  BF_SUCCESS)
            << "Failed for table id " << table_id << "\n";
        if (table_name == "SwitchIngress.action_selector") {
          EXPECT_EQ(tables_depend_on_this_table_list.size(), 2);
          bool match_table_found = false;
          bool selector_table_found = false;
          for (const auto &iter : tables_depend_on_this_table_list) {
            const BfRtTable *temp_table;
            EXPECT_EQ(bfrtInfo->bfrtTableFromIdGet(iter, &temp_table),
                      BF_SUCCESS);
            std::string temp_table_name;
            EXPECT_EQ(temp_table->tableNameGet(&temp_table_name), BF_SUCCESS);
            if (temp_table_name == "SwitchIngress.forward") {
              match_table_found = true;
            }
            if (temp_table_name == "SwitchIngress.action_selector_sel") {
              selector_table_found = true;
            }
          }
          EXPECT_EQ(match_table_found, true);
          EXPECT_EQ(selector_table_found, true);
        } else if (table_name == "SwitchIngress.forward") {
          EXPECT_EQ(tables_depend_on_this_table_list.size(), 0);
        } else if (table_name == "SwitchIngress.action_selector_sel") {
          EXPECT_EQ(tables_depend_on_this_table_list.size(), 1);
          const BfRtTable *temp_table;
          EXPECT_EQ(bfrtInfo->bfrtTableFromIdGet(
                        tables_depend_on_this_table_list[0], &temp_table),
                    BF_SUCCESS);
          std::string temp_table_name;
          EXPECT_EQ(temp_table->tableNameGet(&temp_table_name), BF_SUCCESS);
          EXPECT_EQ(temp_table_name, "SwitchIngress.forward");
        }
      }
    }  // tna_action_selector
    else if (program_name == "tna_counter") {
      // Check the tables that this table depends on
      {
        std::vector<bf_rt_id_t> depends_on_list;
        ASSERT_EQ(bfrtInfo->bfrtInfoTablesThisTableDependsOnGet(
                      table_id, &depends_on_list),
                  BF_SUCCESS);
        if (table_name == "SwitchIngress.forward") {
          EXPECT_EQ(depends_on_list.size(), 0);
        } else if (table_name == "SwitchIngress.indirect_counter") {
          EXPECT_EQ(depends_on_list.size(), 0);
        }
      }

      // Check the tables that depend on this table
      {
        std::vector<bf_rt_id_t> tables_depend_on_this_table_list;
        EXPECT_EQ(bfrtInfo->bfrtInfoTablesDependentOnThisTableGet(
                      table_id, &tables_depend_on_this_table_list),
                  BF_SUCCESS)
            << "Failed for table id " << table_id << "\n";
        if (table_name == "SwitchIngress.indirect_counter") {
          EXPECT_EQ(tables_depend_on_this_table_list.size(), 0);
        } else if (table_name == "SwitchIngress.forward") {
          EXPECT_EQ(tables_depend_on_this_table_list.size(), 0);
        }
      }
    }  // tna_counter
  }
}

// Testing whether for a corresponding table object
// was created for every bf-rt.json entry
TEST_P(BfRtInfoTest, TestTableNumbersIsCorrect) {
  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);
  Cjson tables_cjson = (*bfrt_root_cjson)["tables"];
  auto bf_rt_json_num = tables_cjson.array_size();

  uint32_t optimized_out = 0;
  // Loop over bf-rt.json and check for the table-objs
  for (const auto &table_cjson : tables_cjson.getCjsonChildVec()) {
    std::string table_name = (*table_cjson)["name"];
    const BfRtTable *table = nullptr;
    status = bfrtInfo->bfrtTableFromNameGet(table_name, &table);
    EXPECT_TRUE(status == BF_SUCCESS || status == BF_INVALID_ARG)
        << "Failed to get table from name " << table_name << " with status "
        << status << "\n";
    if (status == BF_INVALID_ARG) {
      optimized_out++;
    }
    // check the name of the table
    if (status != BF_SUCCESS) {
      continue;
    }
    std::string name;
    status = table->tableNameGet(&name);
    EXPECT_EQ(status, BF_SUCCESS);
    EXPECT_EQ(table_name, name);
    // check using IDs too
    bf_rt_id_t table_id = (*table_cjson)["id"];
    status = bfrtInfo->bfrtTableFromIdGet(table_id, &table);
    EXPECT_EQ(status, BF_SUCCESS);
  }
  EXPECT_EQ(bf_rt_json_num - optimized_out, vec.size())
      << "Number of tables parsed is wrong";
}

// Testing whether for a corresponding learn object
// was created for every bf-rt.json entry
TEST_P(BfRtInfoTest, TestLearnObjectNumbersIsCorrect) {
  std::vector<const BfRtLearn *> vec;
  auto status = bfrtInfo->bfrtInfoGetLearns(&vec);
  Cjson learn_cjson = (*bfrt_root_cjson)["learn_filters"];

  auto bf_rt_json_num = learn_cjson.array_size();
  EXPECT_EQ(bf_rt_json_num, vec.size())
      << "Number of learn filters parsed is wrong";

  // Loop over bf-rt.json and check for the learn-objs
  for (const auto &learn_cjson_each : learn_cjson.getCjsonChildVec()) {
    std::string learn_name = (*learn_cjson_each)["name"];
    const BfRtLearn *learn = nullptr;
    status = bfrtInfo->bfrtLearnFromNameGet(learn_name, &learn);
    EXPECT_TRUE(status == BF_SUCCESS)
        << "Failed to get learn object " << learn_name
        << " with status: " << status << "\n";
    // check the name of the learn obj
    std::string name;
    status = learn->learnNameGet(&name);
    EXPECT_EQ(status, BF_SUCCESS);
    EXPECT_EQ(learn_name, name);
    // check using IDs too
    bf_rt_id_t learn_id = (*learn_cjson_each)["id"];
    status = bfrtInfo->bfrtLearnFromIdGet(learn_id, &learn);
    EXPECT_EQ(status, BF_SUCCESS);
  }
}

// Testing for count of Action Profiles
TEST_P(BfRtInfoTest, TestActionProfileCountIsCorrect) {
  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);

  Cjson tables_cjson = (*bfrt_root_cjson)["tables"];
  auto bf_rt_json_num = tables_cjson.array_size();

  // Loop over bf-rt.json and check for the table-objs
  for (const auto &table_cjson : tables_cjson.getCjsonChildVec()) {
    std::string table_name = (*table_cjson)["name"];
    const BfRtTable *table_intf = nullptr;
    status = bfrtInfo->bfrtTableFromNameGet(table_name, &table_intf);
    if (status != BF_SUCCESS) {
      continue;
    }
    const BfRtTableObj *table = dynamic_cast<const BfRtTableObj *>(table_intf);

    std::vector<bf_rt_id_t> action_id_list;
    status = table->actionIdListGet(&action_id_list);
    if (!((*table_cjson)["action_specs"].exists())) {
      // action profile won't be there for tables other than match and
      // action tables so not supported for others.
      // It can also be absent for match tables without action ID
      BfRtTable::TableType table_type;
      table->tableTypeGet(&table_type);
      if (table_type == BfRtTable::TableType::MATCH_DIRECT) {
        EXPECT_EQ(status, BF_OBJECT_NOT_FOUND)
            << "Check failed for table " << table_name;
      } else {
        EXPECT_EQ(status, BF_NOT_SUPPORTED)
            << "Check failed for table " << table_name;
      }
    } else {
      EXPECT_EQ(action_id_list.size(),
                (*table_cjson)["action_specs"].array_size())
          << "Check failed for table " << table_name;
      EXPECT_EQ(status, BF_SUCCESS);
    }
  }
}

// Testing for count of Key Fields
TEST_P(BfRtInfoTest, TestKeyFieldCountIsCorrect) {
  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);

  Cjson tables_cjson = (*bfrt_root_cjson)["tables"];
  auto bf_rt_json_num = tables_cjson.array_size();

  // Loop over bf-rt.json and check for the table-objs
  for (const auto &table_cjson : tables_cjson.getCjsonChildVec()) {
    std::string table_name = (*table_cjson)["name"];
    const BfRtTable *table_intf = nullptr;
    status = bfrtInfo->bfrtTableFromNameGet(table_name, &table_intf);
    if (status != BF_SUCCESS) {
      continue;
    }
    const BfRtTableObj *table = dynamic_cast<const BfRtTableObj *>(table_intf);

    std::vector<bf_rt_id_t> field_id_list;
    status = table->keyFieldIdListGet(&field_id_list);

    EXPECT_EQ(status, BF_SUCCESS) << "Check failed for table " << table_name;
    EXPECT_EQ(field_id_list.size(), (*table_cjson)["key"].array_size())
        << "Check failed for table " << table_name;
  }
}

// Testing for count of Data Fields
TEST_P(BfRtInfoTest, TestDataFieldCountIsCorrect) {
  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);

  Cjson tables_cjson = (*bfrt_root_cjson)["tables"];
  auto bf_rt_json_num = tables_cjson.array_size();

  // Loop over bf-rt.json table-objs
  for (const auto &table_cjson : tables_cjson.getCjsonChildVec()) {
    std::string table_name = (*table_cjson)["name"];
    const BfRtTable *table_intf = nullptr;
    status = bfrtInfo->bfrtTableFromNameGet(table_name, &table_intf);
    if (status != BF_SUCCESS) {
      // Table numbers is already being tested in another test
      continue;
    }
    const BfRtTableObj *table = dynamic_cast<const BfRtTableObj *>(table_intf);

    // Get the count of the common data fields
    uint32_t common_data_count = (*table_cjson)["data"].array_size();
    // iterate over all the action profiles and verify count
    for (auto const &action_spec :
         (*table_cjson)["action_specs"].getCjsonChildVec()) {
      bf_rt_id_t action_id = (*action_spec)["id"];
      std::vector<bf_rt_id_t> field_id_list;
      status = table->dataFieldIdListGet(action_id, &field_id_list);
      EXPECT_EQ(status, BF_SUCCESS) << "Check failed for table " << table_name;
      EXPECT_EQ(field_id_list.size(),
                (*action_spec)["data"].array_size() + common_data_count)
          << "Check failed for table " << table_name;
    }
  }
}

// Testing for parsing correctness of table operations
TEST_P(BfRtInfoTest, TestTableOperationsParsingIsCorrect) {
  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);

  Cjson tables_cjson = (*bfrt_root_cjson)["tables"];
  auto bf_rt_json_num = tables_cjson.array_size();

  // Loop over bf-rt.json and check for the table-objs
  for (const auto &table_cjson : tables_cjson.getCjsonChildVec()) {
    std::string table_name = (*table_cjson)["name"];
    const BfRtTable *table_intf = nullptr;
    status = bfrtInfo->bfrtTableFromNameGet(table_name, &table_intf);
    if (status != BF_SUCCESS) {
      continue;
    }
    const BfRtTableObj *table = dynamic_cast<const BfRtTableObj *>(table_intf);

    std::vector<std::string> operations_v =
        (*table_cjson)["supported_operations"].getCjsonChildStringVec();

    BfRtTable::TableType table_type;
    status = table->tableTypeGet(&table_type);
    EXPECT_EQ(status, BF_SUCCESS)
        << "Unable to get table type for table " << table_name;

    switch (table_type) {
      case BfRtTable::TableType::REGISTER: {
        EXPECT_NE(std::find(operations_v.begin(), operations_v.end(), "Sync"),
                  operations_v.end())
            << "Check failed for table " << table_name;
        std::unique_ptr<BfRtTableOperations> table_ops;
        status = table->operationsAllocate(TableOperationsType::REGISTER_SYNC,
                                           &table_ops);
        EXPECT_EQ(status, BF_SUCCESS)
            << "Check failed for table " << table_name;
        status = table->operationsAllocate(TableOperationsType::COUNTER_SYNC,
                                           &table_ops);
        EXPECT_EQ(status, BF_NOT_SUPPORTED)
            << "Check failed for table " << table_name;
        break;
      }
      case BfRtTable::TableType::COUNTER: {
        EXPECT_NE(std::find(operations_v.begin(), operations_v.end(), "Sync"),
                  operations_v.end())
            << "Check failed for table " << table_name;
        std::unique_ptr<BfRtTableOperations> table_ops;
        status = table->operationsAllocate(TableOperationsType::COUNTER_SYNC,
                                           &table_ops);
        EXPECT_EQ(status, BF_SUCCESS)
            << "Check failed for table " << table_name;
        status = table->operationsAllocate(TableOperationsType::REGISTER_SYNC,
                                           &table_ops);
        EXPECT_EQ(status, BF_NOT_SUPPORTED)
            << "Check failed for table " << table_name;
        break;
      }
      case BfRtTable::TableType::MATCH_DIRECT:
      case BfRtTable::TableType::MATCH_INDIRECT:
      case BfRtTable::TableType::MATCH_INDIRECT_SELECTOR: {
        bool counter_present = false, register_present = false,
             hit_state_present = false;
        if (std::find(operations_v.begin(),
                      operations_v.end(),
                      "SyncRegisters") != operations_v.end()) {
          register_present = true;
        }
        if (std::find(operations_v.begin(),
                      operations_v.end(),
                      "SyncCounters") != operations_v.end()) {
          counter_present = true;
        }
        if (std::find(operations_v.begin(),
                      operations_v.end(),
                      "UpdateHitState") != operations_v.end()) {
          hit_state_present = true;
        }
        if (counter_present) {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(TableOperationsType::COUNTER_SYNC,
                                             &table_ops);
          EXPECT_EQ(status, BF_SUCCESS)
              << "Check failed for table " << table_name;
        } else {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(TableOperationsType::COUNTER_SYNC,
                                             &table_ops);
          EXPECT_EQ(status, BF_NOT_SUPPORTED)
              << "Check failed for table " << table_name;
        }
        if (register_present) {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(TableOperationsType::REGISTER_SYNC,
                                             &table_ops);
          EXPECT_EQ(status, BF_SUCCESS)
              << "Check failed for table " << table_name;
        } else {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(TableOperationsType::REGISTER_SYNC,
                                             &table_ops);
          EXPECT_EQ(status, BF_NOT_SUPPORTED)
              << "Check failed for table " << table_name;
        }
        if (hit_state_present) {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(
              TableOperationsType::HIT_STATUS_UPDATE, &table_ops);
          EXPECT_EQ(status, BF_SUCCESS)
              << "Check failed for table " << table_name;
        } else {
          std::unique_ptr<BfRtTableOperations> table_ops;
          status = table->operationsAllocate(
              TableOperationsType::HIT_STATUS_UPDATE, &table_ops);
          EXPECT_EQ(status, BF_NOT_SUPPORTED)
              << "Check failed for table " << table_name;
        }
        break;
      }
      default:
        break;
    }
  }
}

// Testing whether Param type is correct or not.
// Also tests whether the pipe_mgr initialisaztion is
// correct.
TEST_P(BfRtInfoTestActionProfile, TestTableActionProfileParamType) {
  auto *pipe = MockIPipeMgrIntf::getInstance();
  auto *pipe_2 = PipeMgrIntf::getInstance();
  ASSERT_EQ(pipe, reinterpret_cast<MockIPipeMgrIntf *>(pipe_2));

  std::vector<const BfRtTable *> vec;
  auto status = bfrtInfo->bfrtInfoGetTables(&vec);
  auto paramMap = std::get<4>(GetParam());
  std::string delim = "#";
  for (auto const &action_param_pair : paramMap) {
    // split the string using # as separator
    std::string s = action_param_pair.first;
    std::string action_name = s.substr(0, s.find(delim));
    std::string data_name = s.substr(s.find(delim) + 1);
    std::cout << "ACTION: " << action_name << " data: " << data_name
              << std::endl;
    for (auto const &item : vec) {
      auto table = dynamic_cast<const BfRtTableObj *>(item);
      // find action_id
      bf_rt_id_t action_id;
      status = table->actionIdGet(action_name, &action_id);
      if (status != BF_SUCCESS) {
        continue;
      }
      // find data_field ID
      bf_rt_id_t field_id;
      status = table->dataFieldIdGet(data_name, action_id, &field_id);

      // Find the param type of the data
      std::set<DataFieldType> ret_type;
      status = table->dataFieldTypeGet(field_id, action_id, &ret_type);

      if (status == BF_SUCCESS) {
        EXPECT_EQ(ret_type, action_param_pair.second);
      }
      break;
    }
  }
}

}  // namespace bfrt_test
}  // namespace bfrt
