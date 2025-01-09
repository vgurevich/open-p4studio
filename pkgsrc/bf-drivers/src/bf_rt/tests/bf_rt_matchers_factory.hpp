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

#ifndef _BF_RT_MATCHERS_FACTORY_HPP_
#define _BF_RT_MATCHERS_FACTORY_HPP_

#include "bf_rt_matchers.hpp"

namespace bfrt {
namespace bfrt_test {

using ::testing::MakeMatcher;
using ::testing::Matcher;

class BfRtMatchersFactory {
 public:
  static inline Matcher<bf_status_t> makeIsSuccessMatcher() {
    return MakeMatcher(new IsSuccessMatcher());
  }

  static inline Matcher<pipe_tbl_match_spec_t *> makeMatchSpecMatcher(
      const pipe_tbl_match_spec_t *t) {
    return MakeMatcher(new MatchSpecMatcher(t));
  }

  template <class T>
  static inline Matcher<T> makeActionSpecMatcher(T t) {
    return MakeMatcher(new ActionSpecMatcher<T>(t));
  }

  static inline Matcher<pipe_stat_data_t *> makeCounterSpecMatcher(
      pipe_stat_data_t *t) {
    return MakeMatcher(new CounterSpecMatcher(t));
  }

  static inline Matcher<pipe_res_spec_t *> makeResourceSpecArrayMatcher(
      pipe_res_spec_t *t, const int &resource_count) {
    // FIXME
    return MakeMatcher(new ResourceSpecArrayMatcher(t, resource_count));
  }

  static inline Matcher<pipe_adt_ent_hdl_t *> makeGroupMbrIdMatcher(
      const std::vector<pipe_adt_ent_hdl_t> &t) {
    return MakeMatcher(new GroupMbrIdsMatcher(t));
  }

  static inline Matcher<bool *> makeGroupMbrStsMatcher(
      const std::vector<bool> &t) {
    return MakeMatcher(new GroupMbrStsMatcher(t));
  }
};

// Explicitly instantiate the above function template
template Matcher<const pipe_action_spec_t *>
BfRtMatchersFactory::makeActionSpecMatcher<const pipe_action_spec_t *>(
    const pipe_action_spec_t *);

template Matcher<pipe_action_spec_t *>
BfRtMatchersFactory::makeActionSpecMatcher<pipe_action_spec_t *>(
    pipe_action_spec_t *);

}  // namespace bfrt_test
}  // namespace bfrt

#endif  // _BF_RT_MATCHERS_FACTORY_HPP_
