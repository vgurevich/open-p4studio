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

#ifndef S3_ID_GEN_H__
#define S3_ID_GEN_H__

#include <mutex>  // NOLINT(build/c++11)
#include <vector>

#include "bf_switch/bf_switch_types.h"


const int XORINDEX64[64] = {0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28,
                            16, 3,  61, 54, 58, 35, 52, 50, 42, 21, 44, 38, 32,
                            29, 23, 17, 11, 4,  62, 46, 55, 26, 59, 40, 36, 15,
                            53, 34, 51, 20, 43, 31, 22, 10, 45, 25, 39, 14, 33,
                            19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

namespace smi {
/*
 * Level-1 vector ->  64  64  64
 *                    /\
 *         buckets->64 64
 *                  /\
 * Level-2 10101111010100000101111110100001
 * Each of level-1 nodes represents 4k entries (64*64)
 * Find the first LSB not set in level-2. 2 in the above case.
 * If it is full, move onto next index of the bucket.
 *
 */
const uint32_t ID_GEN_ALLOCATOR_DEFAULT_SIZE = 2;
const uint32_t BIT_WIDTH = 64;
const uint32_t LEVEL2_ENTRIES_PER_BUCKET = BIT_WIDTH;
const uint32_t LEVEL2_ENTRIES_PER_BUCKET_BITS = 6;
const uint32_t LEVEL1_BUCKETS_PER_ENTRY = BIT_WIDTH;
const uint32_t LEVEL1_IDS_PER_ENTRY =
    LEVEL1_BUCKETS_PER_ENTRY * LEVEL2_ENTRIES_PER_BUCKET;
const uint32_t LEVEL1_IDS_PER_ENTRY_BITS = 12;

class idAllocator {
 private:
  std::vector<uint64_t> ids;
  std::mutex mtx;
  std::vector<uint64_t> bucket;

  typedef std::lock_guard<std::mutex> LOCK_ID;

  // Identical to switchapi except for 64 bit blocks
  // Pre-reserve 64 bit blocks in a hierarchical way and use as a bitmap

  void spaceAllocateForIds(uint val) {
    /* This function allocates space for id vector and its corresponding bucket.
   *
   * Default size of ids vector is 2.
   * Each entry in the ids corresponds to 4k ids.
   * Calculation: 64 bits * 64 = 4k.
   * Explanation: a single bit in ids maps to a unit of 64 bits of unsigned
              integer that resides in the bucket vector.
   * Total ids that can be allocated with the default size of 2 = 2*4k = 8k.
   * Total size of buckets  = 2*64 = 128
   * 128 buckets can hold 8k ids. (128 * 64)
   */
    ids.resize(val, 0UL);
    uint total_entries = val << LEVEL2_ENTRIES_PER_BUCKET_BITS;
    bucket.resize(total_entries, 0UL);
  }

 public:
  idAllocator() { spaceAllocateForIds(ID_GEN_ALLOCATOR_DEFAULT_SIZE); }
  idAllocator(uint32_t size) { spaceAllocateForIds(size); }
  // Alternative method to find first LSB set bit
  inline int bitScanForward(uint64_t bb) {
    const uint64_t debruijn64 = 0x03f79d71b4cb0a89UL;
    if (bb)
      return XORINDEX64[((bb ^ (bb - 1)) * debruijn64) >> 58];
    else
      return BIT_WIDTH;
  }

  uint32_t getIdFromBuckets(uint32_t bucket_group_index,
                            uint64_t &bucket_group_bit_map) {
    int level1_position = __builtin_ctzl(~bucket_group_bit_map);

    int bucket_index = (bucket_group_index << LEVEL2_ENTRIES_PER_BUCKET_BITS) +
                       level1_position;
    int level2_pos = __builtin_ctzl(~bucket[bucket_index]);
    bucket[bucket_index] |= 1UL << level2_pos;

    if (bucket[bucket_index] == 0xFFFFFFFFFFFFFFFFU)
      bucket_group_bit_map |= (1UL << level1_position);

    return ((bucket_group_index << LEVEL1_IDS_PER_ENTRY_BITS) +
            (level1_position << 6) + level2_pos + 1);
  }
  uint32_t allocate() {
    LOCK_ID guard(mtx);
    int index = 0;
    for (auto &&x : ids) {
      // if this index is full, continue to next
      if (x == 0xFFFFFFFFFFFFFFFF) {
        ++index;
        continue;
      }
      uint32_t ret = getIdFromBuckets(index, x);
      return ret;
    }
    /*
     * Default size of id vector is 2.
     * When a request to allocate ids is called, and suppose all the default 8k
     * ids are allocated,then here code tries to double the size of id vector
     * to 4 and telling to allocate id with old size(2). If it allocates id
     * with new size(4) it will lead to segmentation fault.
     * spaceAllocateForIds resizes the vector to accomodate the ids.
     */
    uint32_t size = ids.size();
    spaceAllocateForIds(ids.size() << 1);
    uint32_t ret = getIdFromBuckets(size, ids[size]);
    return ret;
  }
  int reserve(uint32_t val) {
    LOCK_ID guard(mtx);
    uint32_t level1_idx = (val - 1) >> LEVEL1_IDS_PER_ENTRY_BITS;
    uint32_t level2_idx = ((val - 1) & (LEVEL1_IDS_PER_ENTRY - 1)) >>
                          LEVEL2_ENTRIES_PER_BUCKET_BITS;
    int pos = (((val - 1) & (LEVEL1_IDS_PER_ENTRY - 1)) &
               (LEVEL2_ENTRIES_PER_BUCKET - 1));
    int bucket_index =
        (level1_idx << LEVEL2_ENTRIES_PER_BUCKET_BITS) + level2_idx;
    while (ids.size() < level1_idx) {
      spaceAllocateForIds(ids.size() << 1);
    }
    if (bucket[bucket_index] & (1UL << pos)) {
      return SWITCH_STATUS_RESOURCE_IN_USE;
    }
    bucket[bucket_index] |= (1UL << pos);
    if (bucket[bucket_index] == 0xFFFFFFFFFFFFFFFF) {
      ids[level1_idx] |= (1UL << level2_idx);
    }
    return SWITCH_STATUS_SUCCESS;
  }

  int release(uint32_t val) {
    LOCK_ID guard(mtx);
    uint32_t level1_idx = (val - 1) >> (LEVEL1_IDS_PER_ENTRY_BITS);
    uint32_t level2_idx = ((val - 1) & (LEVEL1_IDS_PER_ENTRY - 1)) >>
                          LEVEL2_ENTRIES_PER_BUCKET_BITS;
    int pos = (((val - 1) & (LEVEL1_IDS_PER_ENTRY - 1)) &
               (LEVEL2_ENTRIES_PER_BUCKET - 1));

    if (ids.size() < level1_idx) return SWITCH_STATUS_INVALID_PARAMETER;

    int bucket_index =
        (level1_idx << LEVEL2_ENTRIES_PER_BUCKET_BITS) + level2_idx;

    if ((bucket[bucket_index] & (1UL << pos)) == 0) {
      return SWITCH_STATUS_INVALID_PARAMETER;
    }
    bucket[bucket_index] &= ~(1UL << pos);
    ids[level1_idx] &= ~(1UL << level2_idx);
    return SWITCH_STATUS_SUCCESS;
  }

  uint32_t get_first() {
    LOCK_ID guard(mtx);
    uint32_t idx = 0;
    for (auto &entry : bucket) {
      if (entry == 0) {
        ++idx;
        continue;
      }
      uint32_t position = __builtin_ctzl(entry);
      uint32_t ret = (idx << LEVEL2_ENTRIES_PER_BUCKET_BITS) + position + 1;
      return ret;
    }
    return SWITCH_STATUS_SUCCESS;
  }

  void get_next_n(std::vector<uint32_t> &next_n, uint32_t first, uint32_t n) {
    LOCK_ID guard(mtx);
    uint32_t idx = (first - 1) >> LEVEL2_ENTRIES_PER_BUCKET_BITS;
    auto pos = (first - 1) & (LEVEL2_ENTRIES_PER_BUCKET - 1);
    uint32_t i = 0;
    ++pos;
    std::vector<uint64_t>::iterator nth = bucket.begin() + idx;
    for (auto it = nth; it != bucket.end(); it++) {
      if (*it != 0UL) {
        while (pos < BIT_WIDTH) {
          if (*it & (1UL << (pos))) {
            next_n.push_back((idx << LEVEL2_ENTRIES_PER_BUCKET_BITS) + pos + 1);
            ++i;
          }
          if (i == n) return;
          ++pos;
        }
      }
      pos = 0;
      ++idx;
      if (i == n) return;
    }
    return;
  }

  void get_all_in_use(std::vector<uint32_t> &in_use_ids) {
    LOCK_ID guard(mtx);
    uint32_t idx = 0;
    uint32_t pos = 0;
    for (auto it = bucket.begin(); it != bucket.end(); it++) {
      if (*it != 0UL) {
        while (pos < BIT_WIDTH) {
          if (*it & (1UL << (pos))) {
            uint32_t newpos = idx << LEVEL2_ENTRIES_PER_BUCKET_BITS;
            in_use_ids.push_back(newpos + pos + 1);
          }
          ++pos;
        }
      }
      pos = 0;
      ++idx;
    }
    return;
  }
};
}  // namespace smi
#endif  // S3_ID_GEN_H__
