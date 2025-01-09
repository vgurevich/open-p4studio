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

#ifndef _BF_RT_MATCHERS_HPP_
#define _BF_RT_MATCHERS_HPP_

#include <gmock/gmock.h>

#include <iostream>

namespace bfrt {
namespace bfrt_test {

using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

class IsSuccessMatcher : public MatcherInterface<bf_status_t> {
 public:
  // This function compares the expected status with the actual one
  bool MatchAndExplain(bf_status_t status,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;
};

class MatchSpecMatcher : public MatcherInterface<pipe_tbl_match_spec_t *> {
 public:
  MatchSpecMatcher(const pipe_tbl_match_spec_t *ms) : correct_ms(ms) {}

  // This function compares the expected match spec with the actual one
  bool MatchAndExplain(pipe_tbl_match_spec_t *ms,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  const pipe_tbl_match_spec_t *correct_ms{nullptr};
};

template <class T>
class ActionSpecMatcher : public MatcherInterface<T> {
 public:
  ActionSpecMatcher(T as) : correct_as(as) {}

  // This function compares the expected action spec with the actual one
  bool MatchAndExplain(T as, MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  T correct_as{0};
};

// Explicityle instantiate the above templates
template class ActionSpecMatcher<const pipe_action_spec_t *>;
template class ActionSpecMatcher<pipe_action_spec_t *>;

class CounterSpecMatcher : public MatcherInterface<pipe_stat_data_t *> {
 public:
  CounterSpecMatcher(pipe_stat_data_t *cs) : correct_cs(cs){};

  // This function compares the expeced counter spec with the actual one
  bool MatchAndExplain(pipe_stat_data_t *cs,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  pipe_stat_data_t *correct_cs{nullptr};
};

class ResourceSpecArrayMatcher : public MatcherInterface<pipe_res_spec_t *> {
 public:
  ResourceSpecArrayMatcher(pipe_res_spec_t *rs, const int &resource_count)
      : correct_rs(rs), correct_count(resource_count){};

  // This function compares the expeced resource spec array with the actual one
  bool MatchAndExplain(pipe_res_spec_t *rs,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  // FIXME should we be saving the ground truth resource count and only verify
  // the resources for that count OR should I verify the entire array of
  // resources? There's a risk of false negatives with verifying the entire
  // array beyond the resource count as the default initializations might
  // vary between the pipe mgr and the unit test ground truth generation logic
  pipe_res_spec_t *correct_rs{nullptr};
  const int correct_count{0};
};

class GroupMbrIdsMatcher : public MatcherInterface<pipe_adt_ent_hdl_t *> {
 public:
  GroupMbrIdsMatcher(const std::vector<pipe_adt_ent_hdl_t> &ent_hdl_vec)
      : correct_ent_hdl_vec(ent_hdl_vec) {}

  bool MatchAndExplain(pipe_adt_ent_hdl_t *ent_hdls,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  std::vector<pipe_adt_ent_hdl_t> correct_ent_hdl_vec;
};

class GroupMbrStsMatcher : public MatcherInterface<bool *> {
 public:
  GroupMbrStsMatcher(const std::vector<bool> &ent_hdl_sts_vec)
      : correct_ent_hdl_sts_vec(ent_hdl_sts_vec) {}

  bool MatchAndExplain(bool *ent_hdl_sts,
                       MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;
  void DescribeNegationTo(std::ostream *os) const override;

 private:
  std::vector<bool> correct_ent_hdl_sts_vec;
};

}  // namespace bfrt_test
}  // namespace bfrt
#endif  // _BF_RT_MATCHERS_HPP_
