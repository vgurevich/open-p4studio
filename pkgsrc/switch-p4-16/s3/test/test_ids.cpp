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
#include <chrono>
#include <cassert>
#include <random>

#include "bf_switch/bf_switch_types.h"
#include "../id_gen.h"

using namespace std;
using namespace std::chrono;
using namespace smi;

#define ITERS 1200000
void perf_test() {
  idAllocator alloc(1024);
  auto start = steady_clock::now();
  for (int i = 0; i < ITERS; i++) {
    alloc.allocate();
  }
  auto diff = duration_cast<milliseconds>(steady_clock::now() - start);
  std::cout << "Allocator for " << ITERS << ": " << diff.count() << " ms"
            << std::endl;

  return;
}

int main() {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<uint32_t> distribution(1, 320);
  std::vector<uint32_t> ids, temp_ids;
  uint32_t dice_roll = 0;
  idAllocator alloc(32);

  // Test1: repeated allocate and release. Make sure all IDs are correctly
  // released
  auto range = {65, 64, 63, 129, 128, 127};
  for (auto r : range) {
    for (int i = 0; i < r; i++) {
      ids.push_back(alloc.allocate());
    }
    for (auto id : ids) {
      assert(alloc.release(id) == SWITCH_STATUS_SUCCESS);
    }
    ids.clear();
    assert(alloc.allocate() == 1);
    assert(alloc.release(1) == SWITCH_STATUS_SUCCESS);
  }

  // Test2: Random alloc and release
  for (int i = 0; i < 3200; i++) {
    ids.push_back(alloc.allocate());
  }
  dice_roll = distribution(mt);
  assert(alloc.release(dice_roll) == SWITCH_STATUS_SUCCESS);
  assert(alloc.allocate() == dice_roll);
  dice_roll = distribution(mt);
  assert(alloc.release(dice_roll) == SWITCH_STATUS_SUCCESS);
  assert(alloc.allocate() == dice_roll);

  dice_roll = 0;
  for (int i = 0; dice_roll < 3200; i++) {
    std::uniform_int_distribution<uint32_t> dist(i, 3200);
    dice_roll = dist(mt);
    if (dice_roll == 3200) break;
    assert(alloc.release(dice_roll) == SWITCH_STATUS_SUCCESS);
    temp_ids.push_back(dice_roll);
    i = dice_roll;
    cout << i << " ";
  }
  cout << "\n";
  for (auto id : temp_ids) {
    (void)id;
    cout << alloc.allocate() << " ";
  }
  cout << "\n";
  assert(alloc.allocate() == 3201);
  ids.push_back(3201);
  for (auto id : ids) {
    assert(alloc.release(id) == SWITCH_STATUS_SUCCESS);
  }
  ids.clear();
  assert(alloc.allocate() == 1);
  assert(alloc.release(1) == SWITCH_STATUS_SUCCESS);

  // Test3: Invalid release
  // invalid release id
  assert(alloc.release(6400) == SWITCH_STATUS_INVALID_PARAMETER);
  assert(alloc.release(1) == SWITCH_STATUS_INVALID_PARAMETER);
  assert(alloc.release(63) == SWITCH_STATUS_INVALID_PARAMETER);
  assert(alloc.release(64) == SWITCH_STATUS_INVALID_PARAMETER);
  assert(alloc.release(65) == SWITCH_STATUS_INVALID_PARAMETER);
  assert(alloc.release(128) == SWITCH_STATUS_INVALID_PARAMETER);

  // Test4: reserve tests
  assert(alloc.reserve(10) == SWITCH_STATUS_SUCCESS);
  // return with under use message
  assert(alloc.reserve(10) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.release(10) == SWITCH_STATUS_SUCCESS);
  for (int i = 0; i < 320; i++) {
    ids.push_back(alloc.allocate());
  }
  // check word boundaries
  assert(alloc.reserve(1) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(63) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(64) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(65) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(127) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(128) == SWITCH_STATUS_RESOURCE_IN_USE);
  assert(alloc.reserve(129) == SWITCH_STATUS_RESOURCE_IN_USE);
  for (auto id : ids) {
    assert(alloc.release(id) == SWITCH_STATUS_SUCCESS);
  }
  ids.clear();

  // Test5: Out of bound reserve
  // out of bounds reserve, should extend vector and reserve
  assert(alloc.reserve(6400) == SWITCH_STATUS_SUCCESS);
  // return with under use message
  assert(alloc.reserve(6400) == SWITCH_STATUS_RESOURCE_IN_USE);
  // out of bounds reserve, should extend vector and reserve
  assert(alloc.reserve(64000) == SWITCH_STATUS_SUCCESS);
  assert(alloc.release(6400) == SWITCH_STATUS_SUCCESS);
  assert(alloc.release(64000) == SWITCH_STATUS_SUCCESS);

  // Test6: get_first tests
  assert(alloc.reserve(1) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 1);
  assert(alloc.release(1) == SWITCH_STATUS_SUCCESS);
  assert(alloc.reserve(32) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 32);
  assert(alloc.release(32) == SWITCH_STATUS_SUCCESS);
  assert(alloc.reserve(63) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 63);
  assert(alloc.release(63) == SWITCH_STATUS_SUCCESS);
  assert(alloc.reserve(64) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 64);
  assert(alloc.release(64) == SWITCH_STATUS_SUCCESS);
  assert(alloc.reserve(96) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 96);
  assert(alloc.reserve(91) == SWITCH_STATUS_SUCCESS);
  assert(alloc.get_first() == 91);
  assert(alloc.release(96) == SWITCH_STATUS_SUCCESS);
  assert(alloc.release(91) == SWITCH_STATUS_SUCCESS);

  // Test7: get_next tests
  int idx = 1;
  std::vector<uint32_t> gn1 = {61, 62, 63, 64, 65};
  for (auto i : gn1) {
    assert(alloc.reserve(i) == SWITCH_STATUS_SUCCESS);
  }
  assert(alloc.get_first() == 61);
  std::vector<uint32_t> next_n;
  alloc.get_next_n(next_n, 61, 4);
  for (auto i : next_n) {
    assert(i == gn1[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  for (auto i : gn1) {
    assert(alloc.release(i) == SWITCH_STATUS_SUCCESS);
  }
  next_n.clear();
  idx = 1;
  std::vector<uint32_t> gn2 = {64, 68, 69, 127, 128, 129, 130, 131};
  for (auto i : gn2) {
    assert(alloc.reserve(i) == SWITCH_STATUS_SUCCESS);
  }
  assert(alloc.get_first() == 64);
  alloc.get_next_n(next_n, 64, 4);
  for (auto i : next_n) {
    assert(i == gn2[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  next_n.clear();
  idx = 1;
  alloc.get_next_n(next_n, 64, 7);
  for (auto i : next_n) {
    assert(i == gn2[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  next_n.clear();
  idx = 1;
  alloc.get_next_n(next_n, 64, 20);
  for (auto i : next_n) {
    assert(i == gn2[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  for (auto i : gn2) {
    assert(alloc.release(i) == SWITCH_STATUS_SUCCESS);
  }
  next_n.clear();
  idx = 1;
  std::vector<uint32_t> gn3 = {1, 32, 63, 64, 65, 70, 71, 73, 74};
  for (auto i : gn3) {
    assert(alloc.reserve(i) == SWITCH_STATUS_SUCCESS);
  }
  assert(alloc.get_first() == 1);
  alloc.get_next_n(next_n, 1, 4);
  for (auto i : next_n) {
    assert(i == gn3[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  uint32_t back = next_n.back();
  next_n.clear();
  alloc.get_next_n(next_n, back, 4);
  for (auto i : next_n) {
    assert(i == gn3[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  back = next_n.back();
  next_n.clear();
  // size is 0 since we have got all from this index
  alloc.get_next_n(next_n, back, 4);
  assert(next_n.size() == 0);
  // push 4 more entries
  for (auto i : {80, 81, 82, 83}) {
    assert(alloc.reserve(i) == SWITCH_STATUS_SUCCESS);
  }
  gn3.push_back(80);
  gn3.push_back(81);
  gn3.push_back(82);
  gn3.push_back(83);
  alloc.get_next_n(next_n, back, 4);
  for (auto i : next_n) {
    assert(i == gn3[idx]);
    ++idx;
    cout << i << " ";
  }
  cout << "\n";
  for (auto i : gn3) {
    assert(alloc.release(i) == SWITCH_STATUS_SUCCESS);
  }

  // Test7: get_all_in_use tests
  // No ids in use
  std::vector<uint32_t> used_ids;
  alloc.get_all_in_use(used_ids);
  assert(used_ids.size() == 0);
  used_ids.clear();

  // Only one id from first pool in use
  std::vector<uint32_t> gall = {10};
  idx = 0;
  assert(alloc.reserve(10) == SWITCH_STATUS_SUCCESS);
  alloc.get_all_in_use(used_ids);
  for (auto id : used_ids) {
    assert(id == gall[idx]);
    ++idx;
    cout << id << " ";
  }
  for (auto i : gall) {
    assert(alloc.release(i) == SWITCH_STATUS_SUCCESS);
  }
  gall.clear();
  used_ids.clear();

  // Entire first pool in use
  for (int i = 0; i < 32; i++) {
    gall.push_back(alloc.allocate());
  }
  alloc.get_all_in_use(used_ids);
  idx = 0;
  for (auto id : used_ids) {
    assert(id == gall[idx]);
    ++idx;
  }
  used_ids.clear();

  // Entire first pool & one id from 2nd pool in use
  gall.push_back(alloc.allocate());
  idx = 0;
  alloc.get_all_in_use(used_ids);
  for (auto id : used_ids) {
    assert(id == gall[idx]);
    ++idx;
  }
  used_ids.clear();

  // Entire first pool & two ids from 2nd pool in use
  assert(alloc.reserve(63) == SWITCH_STATUS_SUCCESS);
  gall.push_back(63);
  idx = 0;
  alloc.get_all_in_use(used_ids);
  for (auto id : used_ids) {
    assert(id == gall[idx]);
    ++idx;
  }
  used_ids.clear();

  // Entire first pool & two ids from 2nd pool & entire fourth pool in use
  for (int i = 0; i < 32; i++) {
    int rid = 97 + i;
    alloc.reserve(rid);
    gall.push_back(rid);
  }
  idx = 0;
  alloc.get_all_in_use(used_ids);
  for (auto id : used_ids) {
    assert(id == gall[idx]);
    ++idx;
  }

  for (auto i : gall) {
    assert(alloc.release(i) == SWITCH_STATUS_SUCCESS);
  }

  // Ensure no ids in use
  used_ids.clear();
  alloc.get_all_in_use(used_ids);
  assert(used_ids.size() == 0);

  // Test8: to verify the resize of ids vector in allocate function
  idAllocator to_alloc;
  for (int i = 0; i < ITERS; i++) {
    to_alloc.allocate();
  }

  assert(to_alloc.release(9000) == SWITCH_STATUS_SUCCESS);
  assert(to_alloc.release(1) == SWITCH_STATUS_SUCCESS);
  // perf_test();
  return 0;
}
