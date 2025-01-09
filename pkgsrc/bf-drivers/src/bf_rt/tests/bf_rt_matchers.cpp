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


#ifdef __cplusplus
extern "C" {
#endif

#include <pipe_mgr/pipe_mgr_intf.h>

#ifdef __cplusplus
}
#endif

#include "bf_rt_matchers.hpp"

namespace bfrt {
namespace bfrt_test {

bool IsSuccessMatcher::MatchAndExplain(bf_status_t sts,
                                       MatchResultListener *listener) const {
  if (sts != BF_SUCCESS) {
    *listener << "status is " << sts << " instead of BF_SUCCESS(0)";
    return false;
  }
  return true;
}

void IsSuccessMatcher::DescribeTo(std::ostream *os) const {
  *os << "is BF_SUCCESS";
}

void IsSuccessMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is not BF_SUCCESS";
}

bool MatchSpecMatcher::MatchAndExplain(pipe_tbl_match_spec_t *ms,
                                       MatchResultListener *listener) const {
  bool ret = true;
  if (ms->num_match_bytes != correct_ms->num_match_bytes) {
    *listener << "Incorrect num_match_bytes : \n received : "
              << ms->num_match_bytes
              << "\n expected : " << correct_ms->num_match_bytes << "\n";
    ret = false;
  }

  if (memcmp(ms->match_mask_bits,
             correct_ms->match_mask_bits,
             correct_ms->num_match_bytes)) {
    *listener << "Incorrect match mask bits : ";
    *listener << "\n received : ";
    for (uint32_t i = 0; i < ms->num_match_bytes; i++) {
      *listener << static_cast<int>(ms->match_mask_bits[i]) << " ";
    }
    *listener << "\n expected : ";
    for (uint32_t i = 0; i < correct_ms->num_match_bytes; i++) {
      *listener << static_cast<int>(correct_ms->match_mask_bits[i]) << " ";
    }
    *listener << "\n";
    ret = false;
  }
  if (memcmp(ms->match_value_bits,
             correct_ms->match_value_bits,
             correct_ms->num_match_bytes)) {
    *listener << "Incorrect match value bits : ";
    *listener << "\n received : ";
    for (uint32_t i = 0; i < ms->num_match_bytes; i++) {
      *listener << static_cast<int>(ms->match_value_bits[i]) << " ";
    }
    *listener << "\n expected : ";
    for (uint32_t i = 0; i < correct_ms->num_match_bytes; i++) {
      *listener << static_cast<int>(correct_ms->match_value_bits[i]) << " ";
    }
    *listener << "\n";
    ret = false;
  }
  if (ms->partition_index != correct_ms->partition_index) {
    *listener << "Incorrect parition index : \n received : "
              << ms->partition_index
              << "\n expected : " << correct_ms->partition_index << "\n";
    ret = false;
  }
  if (ms->num_valid_match_bits != correct_ms->num_valid_match_bits) {
    *listener << "Incorrect num valid bits : \n received : "
              << ms->num_valid_match_bits
              << "\n expected : " << correct_ms->num_valid_match_bits << "\n";
    ret = false;
  }
  if (ms->priority != correct_ms->priority) {
    *listener << "incorrect priority : \n received : " << ms->priority
              << "\n expected : " << correct_ms->priority << "\n";
    ret = false;
  }

  return ret;
}

void MatchSpecMatcher::DescribeTo(std::ostream *os) const {
  *os << "is correct match spec";
}

void MatchSpecMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect match spec";
}

template <class T>
bool ActionSpecMatcher<T>::MatchAndExplain(
    T as, MatchResultListener *listener) const {
  bool ret = true;
  if (as->pipe_action_datatype_bmap != correct_as->pipe_action_datatype_bmap) {
    *listener << "Incorrect action data type bmap : \n received : "
              << static_cast<int>(as->pipe_action_datatype_bmap)
              << "\n expected : "
              << static_cast<int>(correct_as->pipe_action_datatype_bmap)
              << "\n";
    ret = false;
  }
  if (IS_ACTION_SPEC_ACT_DATA(correct_as)) {
    if (as->act_data.num_action_data_bytes !=
        correct_as->act_data.num_action_data_bytes) {
      *listener << "Incorrect num action data bytes : \n received : "
                << as->act_data.num_action_data_bytes << "\n expected : "
                << correct_as->act_data.num_action_data_bytes << "\n";
      ret = false;
    }
    if (as->act_data.num_valid_action_data_bits !=
        correct_as->act_data.num_valid_action_data_bits) {
      *listener << "Incorrect num valid action data bits : \n received : "
                << as->act_data.num_valid_action_data_bits << "\n expected : "
                << correct_as->act_data.num_valid_action_data_bits << "\n";
      ret = false;
    }
    if (memcmp(as->act_data.action_data_bits,
               correct_as->act_data.action_data_bits,
               correct_as->act_data.num_action_data_bytes)) {
      *listener << "Incorrect action data bits : ";
      *listener << "\n received : ";
      for (uint32_t i = 0; i < as->act_data.num_action_data_bytes; i++) {
        *listener << static_cast<int>(as->act_data.action_data_bits[i]) << " ";
      }
      *listener << "\n expected : ";
      for (uint32_t i = 0; i < correct_as->act_data.num_action_data_bytes;
           i++) {
        *listener << static_cast<int>(correct_as->act_data.action_data_bits[i])
                  << " ";
      }
      *listener << "\n";
      ret = false;
    }
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(correct_as)) {
    if (as->adt_ent_hdl != correct_as->adt_ent_hdl) {
      *listener << "Incorrect adt ent hdl : \n received : " << as->adt_ent_hdl
                << "\n expected : " << correct_as->adt_ent_hdl << "\n";
      ret = false;
    }
  } else if (IS_ACTION_SPEC_SEL_GRP(correct_as)) {
    if (as->sel_grp_hdl != correct_as->sel_grp_hdl) {
      *listener << "Incorrect sel grp hdl : \n received : " << as->sel_grp_hdl
                << "\n expected : " << correct_as->sel_grp_hdl << "\n";
      ret = false;
    }
  }

  if (as->resource_count != correct_as->resource_count) {
    *listener << "Incorrect resource count : \n received : "
              << as->resource_count
              << "\n expected : " << correct_as->resource_count << "\n";
    ret = false;
  }
  if (correct_as->resource_count) {
    if (memcmp(as->resources,
               correct_as->resources,
               sizeof(pipe_res_spec_t) * correct_as->resource_count)) {
      *listener << "Incorrect resource spec(s)\n";
      ret = false;
    }
  }
  return ret;
}

template <class T>
void ActionSpecMatcher<T>::DescribeTo(std::ostream *os) const {
  *os << "is correct action spec";
}

template <class T>
void ActionSpecMatcher<T>::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect action spec";
}

bool CounterSpecMatcher::MatchAndExplain(pipe_stat_data_t *cs,
                                         MatchResultListener *listener) const {
  bool ret = true;
  if (cs->bytes != correct_cs->bytes) {
    *listener << "Incorrect number of bytes : \n received : " << cs->bytes
              << "\n expected : " << correct_cs->bytes << "\n";
    ret = false;
  }
  if (cs->packets != correct_cs->packets) {
    *listener << "Incorrect number of packets : \n received : " << cs->packets
              << "\n expected : " << correct_cs->packets << "\n";
    ret = false;
  }
  return ret;
}

void CounterSpecMatcher::DescribeTo(std::ostream *os) const {
  *os << "is correct counter spec";
}

void CounterSpecMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect counter spec";
}

bool ResourceSpecArrayMatcher::MatchAndExplain(
    pipe_res_spec_t *rs, MatchResultListener *listener) const {
  bool ret = true;
  // TODO Add a more detailed comparison for each of the resource specs and
  // log failures
  if (memcmp(rs, correct_rs, sizeof(pipe_res_spec_t) * correct_count)) {
    *listener << "Incorrect resource spec(s)\n";
    ret = false;
  }
  return ret;
}

void ResourceSpecArrayMatcher::DescribeTo(std::ostream *os) const {
  *os << "is correct resource spec";
}

void ResourceSpecArrayMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect resource spec";
}

bool GroupMbrIdsMatcher::MatchAndExplain(pipe_adt_ent_hdl_t *ent_hdls,
                                         MatchResultListener *listener) const {
  bool ret = true;
  for (uint32_t i = 0; i < correct_ent_hdl_vec.size(); i++) {
    if (correct_ent_hdl_vec[i] != ent_hdls[i]) {
      *listener << "Incorrect adt entry handle at index " << i
                << "\n expected : " << correct_ent_hdl_vec[i]
                << "\n received : " << ent_hdls[i] << "\n";
      ret = false;
    }
  }
  return ret;
}

void GroupMbrIdsMatcher::DescribeTo(std::ostream *os) const {
  *os << "is correct adt entry handles in selector group";
}

void GroupMbrIdsMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect adt entry handles in selector group";
}

bool GroupMbrStsMatcher::MatchAndExplain(bool *ent_hdl_sts,
                                         MatchResultListener *listener) const {
  bool ret = true;
  for (uint32_t i = 0; i < correct_ent_hdl_sts_vec.size(); i++) {
    if (correct_ent_hdl_sts_vec[i] != ent_hdl_sts[i]) {
      *listener << "Incorrect adt entry handle status at index " << i
                << "\n expected : " << correct_ent_hdl_sts_vec[i]
                << "\n received : " << ent_hdl_sts[i] << "\n";
      ret = false;
    }
  }
  return ret;
}

void GroupMbrStsMatcher::DescribeTo(std::ostream *os) const {
  *os << "is correct adt entry handle status in selector group";
}

void GroupMbrStsMatcher::DescribeNegationTo(std::ostream *os) const {
  *os << "is incorrect adt entry handle status in selector group";
}

}  // namespace bfrt_test
}  // namespace bfrt
