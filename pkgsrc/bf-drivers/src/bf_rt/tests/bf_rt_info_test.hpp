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


#ifndef _BF_RT_INFO_TEST_HPP
#define _BF_RT_INFO_TEST_HPP

#include <stdio.h>
#include <assert.h>
#include <sched.h>
#include <string.h>

#include <memory>
#include <map>
#include <tuple>

/* bf_rt_includes */
#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_info.hpp>
#include <../bf_rt_common/bf_rt_table_data_impl.hpp>

#include "bf_rt_pipe_mgr_intf_mock.hpp"

namespace bfrt {
namespace bfrt_test {

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

  obj->bfrtInfo = nullptr;
  std::vector<std::string> bf_rt_json_file_path_vect;
  bf_rt_json_file_path_vect.push_back(filePath1);
  auto *pipe_mgr_obj = MockIPipeMgrIntf::getInstance();

  // Set the action on call to pipeMgrTblHdlPipeMaskGet
  ON_CALL(*pipe_mgr_obj, pipeMgrTblHdlPipeMaskGet(_, _, _, _))
      .WillByDefault(WithArgs<3>(Invoke([](uint32_t *mask) {
        *mask = 0;
        return PIPE_SUCCESS;
      })));

  ProgramConfig program_config(
      obj->program_name, bf_rt_json_file_path_vect, p4_pipelines);
  obj->bfrtInfo = BfRtInfoImpl::makeBfRtInfo(0 /* Device id */, program_config);

  ASSERT_NE(obj->bfrtInfo, nullptr) << "Failed to open json files";
}
}  // Anonymous namespace

class BfRtInfoTest : public ::testing::TestWithParam<
                         std::tuple<std::string,
                                    std::vector<std::string>,
                                    std::vector<std::vector<bf_dev_pipe_t>>,
                                    std::vector<std::string>,
                                    std::string>> {
 public:
  BfRtInfoTest() : bfrtInfo(nullptr){};
  virtual void SetUp() {
    auto t = this->GetParam();
    program_name = std::get<4>(t);
    setupInternal<BfRtInfoTest>(this);
    auto root = Cjson::createCjsonFromFile(
        getTestJsonFileContent(std::get<0>(t), program_name));
    bfrt_root_cjson = std::unique_ptr<Cjson>(new Cjson(root));
  }
  virtual void TearDown() {
    bfrtInfo.reset(nullptr);
    bfrt_root_cjson.reset(nullptr);
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const BfRtInfo> bfrtInfo;
  std::unique_ptr<const BfRtInfo> bfrtFixedInfo;
  std::unique_ptr<Cjson> bfrt_root_cjson;
  std::string program_name;
};

// tuple of (bf_rt_json_path,
// vector of context_json_path,
// vector of pipe_scope vectors,
// vector of profile_names,
// program_name)
INSTANTIATE_TEST_CASE_P(
    BfRtJsonAndContextJson,
    BfRtInfoTest,
    ::testing::Values(
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_counter"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "switch"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{
                            "pipeline_profile_a/context.json",
                            "pipeline_profile_b/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 3}, {1, 2}},
                        std::vector<std::string>{"pipeline_profile_a",
                                                 "pipeline_profile_b"},
                        "tna_32q_2pipe"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_action_selector"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_idletimeout"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        "tna_meter_lpf_wred")));

class BfRtFixedInfoTest
    : public ::testing::TestWithParam<std::vector<std::string>> {
 public:
  BfRtFixedInfoTest() : bfrtFixedInfo(nullptr){};
  virtual void SetUp() {
    std::vector<std::string> filenames = this->GetParam();
    std::vector<std::string> bf_rt_json_file_path_vect;
    for (const auto &iter : filenames) {
      std::string temp_file_path =
          std::string(BFRT_SHARED_INSTALLDIR) + std::string("/") + iter;
      bf_rt_json_file_path_vect.push_back(temp_file_path);
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
        program_name, bf_rt_json_file_path_vect, p4_pipelines);
    bfrtFixedInfo =
        BfRtInfoImpl::makeBfRtInfo(0 /* Device id */, program_config);
  }
  virtual void TearDown() {
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const BfRtInfo> bfrtFixedInfo;
  std::string program_name{"$SHARED"};
};

INSTANTIATE_TEST_CASE_P(BfRtPortsJson,
                        BfRtFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "bf_rt_port_tf1.json"}));

INSTANTIATE_TEST_CASE_P(BfRtPorts2Json,
                        BfRtFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "bf_rt_port_tf2.json"}));

INSTANTIATE_TEST_CASE_P(BfRtTmTofJson,
                        BfRtFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "bf_rt_tm_tf1.json"}));

INSTANTIATE_TEST_CASE_P(BfRtTmTof2Json,
                        BfRtFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "bf_rt_tm_tf2.json"}));

INSTANTIATE_TEST_CASE_P(BfRtTmTof3Json,
                        BfRtFixedInfoTest,
                        ::testing::Values(std::vector<std::string>{
                            "bf_rt_tm_tf3.json"}));








using ActionParamTypeMap =
    std::unordered_map<std::string, std::set<DataFieldType>>;

class BfRtInfoTestActionProfile
    : public ::testing::TestWithParam<
          std::tuple<std::string,
                     std::vector<std::string>,
                     std::vector<std::vector<bf_dev_pipe_t>>,
                     std::vector<std::string>,
                     ActionParamTypeMap,
                     std::string>> {
 public:
  BfRtInfoTestActionProfile(){};
  void SetUp() {
    auto t = this->GetParam();
    program_name = std::get<5>(t);
    setupInternal<BfRtInfoTestActionProfile>(this);
  }
  void TearDown() {
    std::unique_ptr<IPipeMgrIntf> &unique_ptr_mock =
        MockIPipeMgrIntf::getInstanceToUniquePtr();
    unique_ptr_mock.reset();
  }
  std::unique_ptr<const BfRtInfo> bfrtInfo;
  std::string program_name;
};

// For this test each pair of json file needs a ActionParamTypeMap object to be
// sent along. Add as needed
// map(action_name#data_name -> set of types) for BfRtInfoTestActionProfile
// In this map # acts as delimiter
ActionParamTypeMap switch_paramMapBfRt(
    {{"SwitchIngress.ingress_port_mapping."
      "port_mirror.set_mirror_id#session_id",
      std::set<DataFieldType>({ACTION_PARAM})}});
ActionParamTypeMap tna_counter_paramMapBfRt(
    {{"SwitchIngress.hit_dst#port",
      std::set<DataFieldType>({ACTION_PARAM, COUNTER_INDEX})}});

INSTANTIATE_TEST_CASE_P(
    BfRtJsonAndContextJson,
    BfRtInfoTestActionProfile,
    ::testing::Values(
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        switch_paramMapBfRt,
                        "switch"),
        std::make_tuple("bf-rt.json",
                        std::vector<std::string>{"pipe/context.json"},
                        std::vector<std::vector<bf_dev_pipe_t>>{{0, 1, 2, 3}},
                        std::vector<std::string>{"pipe"},
                        tna_counter_paramMapBfRt,
                        "tna_counter")));

}  // namespace bfrt_test
}  // namespace bfrt

#endif
