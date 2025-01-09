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

#ifndef _TDI_INFO_TEST_HPP
#define _TDI_INFO_TEST_HPP

#include <stdio.h>
#include <assert.h>
#include <sched.h>
#include <string.h>

#include <memory>
#include <map>
#include <tuple>

/* tdi_includes */
#include <tdi/tdi_common.h>
#include <tdi/tdi_info.hpp>
#include <../tdi_common/tdi_table_data_impl.hpp>

#include "tdi_pipe_mgr_intf_mock.hpp"

namespace tdi {
namespace tdi_test {

using ::testing::WithArgs;

namespace {
std::string getTestJsonFileContent(const std::string &filename,
                                   const std::string &program_name) {
  std::string temp_file_name =
      std::string(INSTALLDIR) + (program_name != "switch" ? "/tofinopd" : "") +
      std::string("/") + program_name + std::string("/") + filename;
  std::ifstream file(temp_file_name);
  if (file.fail()) {
    LOG_CRIT("Unable to find Json File %s", temp_file_name.c_str());
    return "";
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  return content;
}

template <class T>
void setupInternal(T *obj) {
  auto t = obj->GetParam();
  std::string filename1 = std::get<0>(t);
  std::vector<std::string> &context_json_filenames = std::get<1>(t);
  std::vector<std::vector<bf_dev_pipe_t>> &pipe_scope_vec = std::get<2>(t);
  std::vector<std::string> &pipeline_profile_vec = std::get<3>(t);
  std::string program_name = obj->program_name;

  std::string filePath1 =
      std::string(INSTALLDIR) + (program_name != "switch" ? "/tofinopd" : "") +
      std::string("/") + program_name + std::string("/") + filename1;
  std::string dummy_binary_path = "dummy_binary_file_path";
  std::vector<ProgramConfig::P4Pipeline> p4_pipelines;
  for (uint32_t i = 0; i < context_json_filenames.size(); i++) {
    std::string filePath = std::string(INSTALLDIR) +
                           (program_name != "switch" ? "/tofinopd" : "") +
                           std::string("/") + program_name + std::string("/") +
                           context_json_filenames[i];
    p4_pipelines.emplace_back(pipeline_profile_vec[i],
                              filePath,
                              dummy_binary_path,
                              pipe_scope_vec[i]);
  }

  obj->tdiInfo = nullptr;
  std::vector<std::string> tdi_json_file_path_vect;
  tdi_json_file_path_vect.push_back(filePath1);
  auto *pipe_mgr_obj = MockIPipeMgrIntf::getInstance();

  // Set the action on call to pipeMgrTblHdlPipeMaskGet
  ON_CALL(*pipe_mgr_obj, pipeMgrTblHdlPipeMaskGet(_, _, _, _))
      .WillByDefault(WithArgs<3>(Invoke([](uint32_t *mask) {
        *mask = 0;
        return PIPE_SUCCESS;
      })));

  ProgramConfig program_config(
      obj->program_name, tdi_json_file_path_vect, p4_pipelines);
  obj->tdiInfo = TdiInfoImpl::makeTdiInfo(0 /* Device id */, program_config);

  ASSERT_NE(obj->tdiInfo, nullptr) << "Failed to open json files";
}
}  // Anonymous namespace

class TdiInfoTest : public ::testing::TestWithParam<
                        std::tuple<std::string,
                                   std::vector<std::string>,
                                   std::vector<std::vector<bf_dev_pipe_t>>,
                                   std::vector<std::string>,
                                   std::string>> {
 public:
  TdiInfoTest() : tdiInfo(nullptr){};
  virtual void SetUp() {
    auto t = this->GetParam();
    program_name = std::get<4>(t);
    setupInternal<TdiInfoTest>(this);
    auto root = Cjson::createCjsonFromFile(
        getTestJsonFileContent(std::get<0>(t), program_name));
    tdi_root_cjson = std::unique_ptr<Cjson>(new Cjson(root));
  }
  virtual void TearDown() {
    tdiInfo.reset(nullptr);
    tdi_root_cjson.reset(nullptr);
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const TdiInfo> tdiInfo;
  std::unique_ptr<const TdiInfo> tdiFixedInfo;
  std::unique_ptr<Cjson> tdi_root_cjson;
  std::string program_name;
};

// tuple of (tdi_json_path,
// vector of context_json_path,
// vector of pipe_scope vectors,
// vector of profile_names,
// program_name)
INSTANTIATE_TEST_CASE_P(
    TdiJsonAndContextJson,
    TdiInfoTest,
    ::testing::Values(
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_counter"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "switch"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{
                            "pipeline_profile_a/context.json",
                            "pipeline_profile_b/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 3}, {1, 2}},
                        std::vector<std::string>{"pipeline_profile_a",
                                                 "pipeline_profile_b"},
                        "tna_32q_2pipe"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_action_selector"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_idletimeout"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_meter_lpf_wred")));

class TdiFixedInfoTest
    : public ::testing::TestWithParam<std::vector<std::string>> {
 public:
  TdiFixedInfoTest() : tdiFixedInfo(nullptr){};
  virtual void SetUp() {
    std::vector<std::string> filenames = this->GetParam();
    std::vector<std::string> tdi_json_file_path_vect;
    for (const auto &iter : filenames) {
      std::string temp_file_path =
          std::string(TDI_SHARED_INSTALLDIR) + std::string("/") + iter;
      tdi_json_file_path_vect.push_back(temp_file_path);
    }

    auto *pipe_mgr_obj = MockIPipeMgrIntf::getInstance();
    // Set the action on call to pipeMgrTblHdlPipeMaskGet
    ON_CALL(*pipe_mgr_obj, pipeMgrTblHdlPipeMaskGet(_, _, _, _))
        .WillByDefault(WithArgs<3>(Invoke([](uint32_t *mask) {
          *mask = 0;
          return PIPE_SUCCESS;
        })));
    std::vector<ProgramConfig::P4Pipeline> p4_pipelines;
    ProgramConfig program_config(
        program_name, tdi_json_file_path_vect, p4_pipelines);
    tdiFixedInfo = TdiInfoImpl::makeTdiInfo(0 /* Device id */, program_config);
  }
  virtual void TearDown() {
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const TdiInfo> tdiFixedInfo;
  std::string program_name{"$SHARED"};
};

INSTANTIATE_TEST_CASE_P(TdiPortsJson,
                        TdiFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "tdi_port_tf1.json"}));

INSTANTIATE_TEST_CASE_P(TdiPorts2Json,
                        TdiFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "tdi_port_tf2.json"}));

INSTANTIATE_TEST_CASE_P(TdiTmTofJson,
                        TdiFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "tdi_tm_tf1.json"}));

INSTANTIATE_TEST_CASE_P(TdiTmTof2Json,
                        TdiFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "tdi_tm_tf2.json"}));

INSTANTIATE_TEST_CASE_P(TdiTmTof3Json,
                        TdiFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "tdi_tm_tf3.json"}));

using ActionParamTypeMap =
    std::unordered_map<std::string, std::set<DataFieldType>>;

class TdiInfoTestActionProfile
    : public ::testing::TestWithParam<
          std::tuple<std::string,
                     std::vector<std::string>,
                     std::vector<std::vector<bf_dev_pipe_t>>,
                     std::vector<std::string>,
                     ActionParamTypeMap,
                     std::string>> {
 public:
  TdiInfoTestActionProfile(){};
  void SetUp() {
    auto t = this->GetParam();
    program_name = std::get<5>(t);
    setupInternal<TdiInfoTestActionProfile>(this);
  }
  void TearDown() {
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const TdiInfo> tdiInfo;
  std::string program_name;
};

// For this test each pair of json file needs a ActionParamTypeMap object to be
// sent along. Add as needed
// map(action_name#data_name -> set of types) for TdiInfoTestActionProfile
// In this map # acts as delimiter
ActionParamTypeMap switch_paramMapTdi(
    {{"SwitchIngress.ingress_port_mapping."
      "port_mirror.set_mirror_id#session_id",
      std::set<DataFieldType>({ACTION_PARAM})}});
ActionParamTypeMap tna_counter_paramMapTdi(
    {{"SwitchIngress.hit_dst#port",
      std::set<DataFieldType>({ACTION_PARAM, COUNTER_INDEX})}});

INSTANTIATE_TEST_CASE_P(
    TdiJsonAndContextJson,
    TdiInfoTestActionProfile,
    ::testing::Values(
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        switch_paramMapTdi,
                        "switch"),
        std::make_tuple("tdi.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        tna_counter_paramMapTdi,
                        "tna_counter")));

}  // namespace tdi_test
}  // namespace tdi

#endif
