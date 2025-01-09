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

// Standard includes
#include <iostream>
#include <unordered_map>
#include <memory>
#include <utility>
#include <mutex>
#include <vector>
#include <algorithm>

#include <string.h>

// Local includes
#include "diag_common.h"
#include "diag_pd.h"
#include "diag_create_pkt.h"
#include "diag_pkt_database.h"

// namespace diag {
#define MIN_REPEAT_THRESH_FOR_STRONG_SUSPECT_FOR_HOLD 3
#define HOLD_BYTE_INTERVAL 80

// Wrapper helper struct to store the pkt size and data. This ensures proper
// cleanup of the data
struct PktWrapper {
 public:
  PktWrapper(const int &size,
             const bf_diag_sess_hdl_t &hdl,
             const bf_dev_port_t &port)
      : pkt_size(size), sess_hdl(hdl), dev_port(port) {
    pkt = new uint8_t[size];
    if (pkt == nullptr) {
      DIAG_PRINT("Malloc failed\n");
      DIAG_ASSERT(0);
    }
  }
  ~PktWrapper() { delete[] pkt; }
  uint32_t pkt_size;
  uint8_t *pkt;
  bf_diag_sess_hdl_t sess_hdl;
  bf_dev_port_t dev_port;

  // Vectors to store all the bit locations that experience flips
  std::vector<uint32_t> flips_1_to_0_vec;
  std::vector<uint32_t> flips_0_to_1_vec;
};

// should this be static, how about using anonymous namespace?
static std::unordered_map<pkt_id_type, std::unique_ptr<PktWrapper>>
    g_pkt_database;
static std::mutex mtx;
static pkt_id_type g_pkt_id = 0;

namespace {
void diag_encode_sess_port_in_pkt_id(const bf_diag_sess_hdl_t sess_hdl,
                                     const bf_dev_port_t dev_port,
                                     pkt_id_type *pkt_id) {
  // pkt_id is a 64 bit u_int. Use the most significant 4 bytes to encode
  // the sess_hdl and the port with 2 bytes for each in that order
  if (DIAG_MIN_PKT_SIZE_ENABLED) {
    // for min size pkt, make pkt_id 2 bytes and save bytes
    *pkt_id = *pkt_id & 0xffffff;
    *pkt_id |= ((static_cast<pkt_id_type>(sess_hdl) & 0xffff) << 32);
    *pkt_id |= ((static_cast<pkt_id_type>(dev_port) & 0xffff) << 16);
  } else {
    *pkt_id = *pkt_id & 0xffffffff;
    *pkt_id |= ((static_cast<pkt_id_type>(sess_hdl) & 0xffff) << 48);
    *pkt_id |= ((static_cast<pkt_id_type>(dev_port) & 0xffff) << 32);
  }
}

void diag_decode_sess_port_from_pkt_id(const pkt_id_type pkt_id,
                                       bf_diag_sess_hdl_t *sess_hdl,
                                       bf_dev_port_t *dev_port) {
  if (DIAG_MIN_PKT_SIZE_ENABLED) {
    *sess_hdl = (pkt_id >> 32) & 0xffff;
    *dev_port = (pkt_id >> 16) & 0xffff;
  } else {
    *sess_hdl = (pkt_id >> 48) & 0xffff;
    *dev_port = (pkt_id >> 32) & 0xffff;
  }
}

inline uint32_t diag_get_header_size() {
// The header size of the packet is determined by the mode in which the
// SLT has been compiled.
#if defined(DIAG_PHV_STRESS_ENABLE)
  return 378;
#elif defined(DIAG_PHV_FLOP_CONFIG_1)
  // 14 (Eth) + 2 (Override) + 48W + 64H +64B (PHVs)
  return 400;
#elif defined(DIAG_PHV_FLOP_CONFIG_2)
  // 14 (Eth) + 2 (Override) + 16W + 32H + 16B (PHVs)
  return 160;
#elif defined(DIAG_PHV_FLOP_CONFIG_3)
  return DIAG_PKT_HEADER_SIZE;
#elif defined(DIAG_PATTERN_SHIFT_ENABLE)
  return 240;
#else
  return DIAG_PKT_HEADER_SIZE;
#endif
}

void diag_log_bit_flip_in_pkt(const pkt_id_type &pkt_id,
                              const uint32_t &byte_offset,
                              const bool &is_1_to_0_flip) {
  // Get the pkt data for the pkt_id
  auto &pkt_wrapper = g_pkt_database.at(pkt_id);

  // If the flip is from 1->0 and if it is not in the header, we don't
  // log it because it's not the regular setup issue
  if (is_1_to_0_flip && (byte_offset >= diag_get_header_size())) {
    return;
  }

  // Get the vectors for logging bit flips
  auto &vec_1_to_0 = pkt_wrapper.get()->flips_1_to_0_vec;
  auto &vec_0_to_1 = pkt_wrapper.get()->flips_0_to_1_vec;

  // Sanity check. The absolute bit offset should not already be present in
  // either of the maps. That would mean that we are adding the same bit
  // twice, which would indicate an error in our compare logic. Assert in
  // that case
  if (is_1_to_0_flip) {
    if (std::find(vec_1_to_0.begin(), vec_1_to_0.end(), byte_offset) !=
        vec_1_to_0.end()) {
      DIAG_PRINT(
          "Trying to add the same byte offset (%d) twice for pkt_id (%lu) for "
          "a 1 to 0 flip. Something is wrong in the pkt compare logic. "
          "Asserting\n",
          byte_offset,
          pkt_id);
      DIAG_ASSERT(0);
    }
  } else {
    if (std::find(vec_0_to_1.begin(), vec_0_to_1.end(), byte_offset) !=
        vec_0_to_1.end()) {
      DIAG_PRINT(
          "Trying to add the same byte offset (%d) twice for pkt_id (%lu) for "
          "a 0 to 1 flip. Something is wrong in the pkt compare logic. "
          "Asserting\n",
          byte_offset,
          pkt_id);
      DIAG_ASSERT(0);
    }
  }

  if (is_1_to_0_flip) {
    vec_1_to_0.push_back(byte_offset);
  } else {
    vec_0_to_1.push_back(byte_offset);
  }
}

void diag_classify_suspicion_for_setup(const pkt_id_type &pkt_id,
                                       bool *is_weak_suspect) {
  // Get the pkt data for the pkt_id
  auto &pkt_wrapper = g_pkt_database.at(pkt_id);

  // If there is even a single 1->0 flip, it is a weak suspect for setup
  if (pkt_wrapper.get()->flips_1_to_0_vec.size()) {
    *is_weak_suspect = true;
  } else {
    *is_weak_suspect = false;
  }
}

void diag_classify_suspicion_for_hold(
    const pkt_id_type &pkt_id,
    bool *is_weak_suspect,
    bool *is_strong_suspect,
    std::vector<std::vector<uint32_t>> *ss_hold_bit_flips_index_vec) {
  std::vector<uint32_t> bit_index;

  *is_strong_suspect = false;
  *is_weak_suspect = false;

  // Get the pkt data for the pkt_id
  auto &pkt_wrapper = g_pkt_database.at(pkt_id);

  auto &vec_0_to_1 = pkt_wrapper.get()->flips_0_to_1_vec;

  while (vec_0_to_1.size()) {
    std::vector<uint32_t> temp;
    int counter = 0;
    auto first = vec_0_to_1.front();
    for (auto iter = vec_0_to_1.begin(); iter != vec_0_to_1.end();) {
      // If the bit flips are on the required (80 byte) boundary, add the
      // bit index to a temp vector
      if ((((*iter) - first) % (HOLD_BYTE_INTERVAL)) == 0) {
        counter++;
        temp.push_back(*iter);
      }
      if ((*iter) == first) {
        iter = vec_0_to_1.erase(iter);
      } else {
        iter++;
      }
    }
    if (counter >= MIN_REPEAT_THRESH_FOR_STRONG_SUSPECT_FOR_HOLD) {
      *is_strong_suspect = true;
      ss_hold_bit_flips_index_vec->push_back(temp);
      // Remove the bit flips that have been accounted for in here
      // from the main vector so we don't go over them again.
      std::for_each(temp.begin(), temp.end(), [&](const int ele) {
        for (auto iter = vec_0_to_1.begin(); iter != vec_0_to_1.end();) {
          if ((*iter) == ele) {
            iter = vec_0_to_1.erase(iter);
          } else {
            iter++;
          }
        }
      });
    } else {
      *is_weak_suspect = true;
    }
  }
}

void diag_classify_suspicion(
    const pkt_id_type &pkt_id,
    bool *is_weak_suspect_for_setup,
    bool *is_weak_suspect_for_hold,
    bool *is_strong_suspect_for_hold,
    std::vector<std::vector<uint32_t>> *ss_hold_bit_flips_index_vec) {
  // Get the pkt data for the pkt_id
  auto &pkt_wrapper = g_pkt_database.at(pkt_id);

  // Classify suspicion level for setup
  diag_classify_suspicion_for_setup(pkt_id, is_weak_suspect_for_setup);

  // Clasify suspicion level for hold
  diag_classify_suspicion_for_hold(pkt_id,
                                   is_weak_suspect_for_hold,
                                   is_strong_suspect_for_hold,
                                   ss_hold_bit_flips_index_vec);

  // SS_Hold takes precedence over S_Hold
  if (*is_strong_suspect_for_hold) {
    *is_weak_suspect_for_hold = false;
  }
}

void diag_get_overall_pkt_failure_type(const pkt_id_type &pkt_id,
                                       const bool &is_weak_suspect_for_setup,
                                       const bool &is_strong_suspect_for_setup,
                                       const bool &is_weak_suspect_for_hold,
                                       const bool &is_strong_suspect_for_hold,
                                       const bool &is_payload_setup,
                                       diag_slt_failure_type_e *failure_type,
                                       std::string *failure_name) {
  if (/* Condition that needs to be true */ is_weak_suspect_for_setup &&
      !(/* Conditions that need to be false */ is_strong_suspect_for_setup ||
        is_weak_suspect_for_hold || is_strong_suspect_for_hold ||
        is_payload_setup)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_S_SETUP;
    *failure_name = "S_Setup";
  } else if (is_strong_suspect_for_setup &&
             !(is_weak_suspect_for_hold || is_strong_suspect_for_hold ||
               is_payload_setup)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_SS_SETUP;
    *failure_name = "SS_Setup";
  } else if (is_weak_suspect_for_hold &&
             !(is_weak_suspect_for_setup || is_strong_suspect_for_setup ||
               is_strong_suspect_for_hold || is_payload_setup)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_S_HOLD;
    *failure_name = "S_Hold";
  } else if (is_strong_suspect_for_hold &&
             !(is_weak_suspect_for_setup || is_strong_suspect_for_setup ||
               is_payload_setup)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_SS_HOLD;
    *failure_name = "SS_Hold";
  } else if (is_payload_setup &&
             !(is_weak_suspect_for_setup || is_strong_suspect_for_setup ||
               is_weak_suspect_for_hold || is_strong_suspect_for_hold)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP;
    *failure_name = "Payload_Setup";
  } else if ((is_weak_suspect_for_setup || is_strong_suspect_for_setup) &&
             (is_weak_suspect_for_hold || is_strong_suspect_for_hold) &&
             is_payload_setup) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_MIXED;
    *failure_name = "Mixed (";
    *failure_name += is_weak_suspect_for_setup ? "S_Setup + " : "SS_Setup + ";
    *failure_name += is_weak_suspect_for_hold ? "S_Hold + " : "SS_Hold + ";
    *failure_name += "Payload_Setup)";
  } else if ((is_weak_suspect_for_setup || is_strong_suspect_for_setup) &&
             is_payload_setup) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_MIXED;
    *failure_name = "Mixed (";
    *failure_name += is_weak_suspect_for_setup ? "S_Setup + " : "SS_Setup + ";
    *failure_name += "Payload_Setup)";
  } else if ((is_weak_suspect_for_hold || is_strong_suspect_for_hold) &&
             is_payload_setup) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_MIXED;
    *failure_name = "Mixed (";
    *failure_name += is_weak_suspect_for_hold ? "S_Hold + " : "SS_Hold + ";
    *failure_name += "Payload_Setup)";
  } else if ((is_weak_suspect_for_setup || is_strong_suspect_for_setup) &&
             (is_weak_suspect_for_hold || is_strong_suspect_for_hold)) {
    *failure_type = DIAG_SLT_FAILURE_TYPE_MIXED;
    *failure_name = "Mixed (";
    *failure_name += is_weak_suspect_for_setup ? "S_Setup + " : "SS_Setup + ";
    *failure_name += is_weak_suspect_for_hold ? "S_Hold" : "SS_Hold";
    *failure_name += ")";
  } else {
    *failure_type = DIAG_SLT_FAILURE_TYPE_NO_FAILURE;
    *failure_name += "No Corruptions";
  }
}

void diag_print_pkt_classification_summary(const pkt_id_type &pkt_id,
                                           const bf_diag_sess_hdl_t &sess_hdl,
                                           const bf_dev_port_t &dev_port,
                                           const uint32_t &size,
                                           const std::string &failure_name,
                                           const uint64_t &num_bytes_mismatch,
                                           const uint64_t &num_bits_mismatch) {
  DIAG_PRINT(
      "pkt_id %8lx : Summary : Session : %d : Port : %-3d : Pkt "
      "size : %d\n",
      pkt_id,
      sess_hdl,
      dev_port,
      size);
  DIAG_PRINT("pkt_id %8lx : Suspected for               : %s\n",
             pkt_id,
             failure_name.c_str());
  DIAG_PRINT("pkt_id %8lx : Total bytes that mismatched : %-4lu\n",
             pkt_id,
             num_bytes_mismatch);
  DIAG_PRINT("pkt_id %8lx : Total bits that mismatched  : %-5lu\n",
             pkt_id,
             num_bits_mismatch);
}

}  // anonymous namespace

// Add a pkt to the central database
int diag_add_pkt_to_db(const pkt_id_type pkt_id,
                       const uint32_t pkt_size,
                       const uint8_t *pkt_buf) {
  std::lock_guard<std::mutex> lock(mtx);

  // pkt_id should not be present in the map. Assert otherwise
  if (g_pkt_database.find(pkt_id) != g_pkt_database.end()) {
    DIAG_PRINT("Pkt id %lx already present in the map\n", pkt_id);
    DIAG_ASSERT(0);
  }

  bf_diag_sess_hdl_t hdl;
  bf_dev_port_t port;
  diag_decode_sess_port_from_pkt_id(pkt_id, &hdl, &port);
  PktWrapper *pkt_wrapper = new PktWrapper(pkt_size, hdl, port);
  if (pkt_wrapper == nullptr) {
    DIAG_PRINT(
        "Malloc failed for pkt_id %lx sess %x port %d \n", pkt_id, hdl, port);
    return -1;
  }
  memcpy(pkt_wrapper->pkt, pkt_buf, pkt_size);
  std::unique_ptr<PktWrapper> ptr(pkt_wrapper);
  g_pkt_database[pkt_id] = std::move(ptr);

  return 0;
}

// Retrieve pkt from the central database
int diag_get_pkt_from_db(const pkt_id_type pkt_id,
                         const uint32_t pkt_size,
                         uint32_t *cached_pkt_size,
                         uint8_t *pkt_buf) {
  std::lock_guard<std::mutex> lock(mtx);

  // Get the pkt data corresponding to the pkt_id
  if (g_pkt_database.find(pkt_id) == g_pkt_database.end()) {
    DIAG_PRINT("Pkt id %lx not present in the map\n", pkt_id);
    return -1;
  }

  const auto &pkt_wrapper = g_pkt_database.at(pkt_id);
  *cached_pkt_size = pkt_wrapper.get()->pkt_size;
#if !defined(DIAG_PATTERN_SHIFT_ENABLE) && !defined(DIAG_PHV_FLOP_CONFIG_3)
  // Have the below check only in cases when DIAG_PATTERN_SHIFT_ENABLE is
  // turned off as in this mode, the dataplane adds upto 31 bytes to
  // to the packet after the payload of 240 bytes.
  if (*cached_pkt_size != pkt_size) {
    DIAG_PRINT(
        "Pkt size (%d) decoded for pkt_id %lx not matching up with the pkt "
        "size (%d) passed in the params\n",
        *cached_pkt_size,
        pkt_id,
        pkt_size);
    return -1;
  }
#endif  // DIAG_PATTERN_SHIFT_ENABLE
  const auto decoded_pkt_raw_data = pkt_wrapper.get()->pkt;
  memcpy(pkt_buf, decoded_pkt_raw_data, pkt_size);

  return 0;
}

// Encode pkt_id at the tail of the packet
pkt_id_type diag_encode_pkt_id(const bf_diag_sess_hdl_t sess_hdl,
                               const bf_dev_port_t dev_port,
                               const uint32_t pkt_size,
                               uint8_t *pkt_buf) {
  std::lock_guard<std::mutex> lock(mtx);

  // We are going to encode the same pkt_id consecutive locations in the
  // payload
  uint8_t total_bytes_req = DIAG_PKT_ID_ENCODE_SIZE;
  if ((pkt_size - DIAG_PKT_HEADER_SIZE) < total_bytes_req) {
    DIAG_PRINT("Pkt size (%d) not sufficient to encode pkt_id \n", pkt_size);
    return PKT_DATABASE_ERR;
  }

  auto encoded_pkt_id = g_pkt_id;

  // Encode the sess hdl and port in the pkt_id
  diag_encode_sess_port_in_pkt_id(sess_hdl, dev_port, &encoded_pkt_id);

  // We start from the tail of the packet and encode consecutive copies of the
  // pkt_id
  int diag_min_size_off = DIAG_MIN_PKT_SIZE_ENABLED ? 2 : 0;
  for (int i = 0; i < DIAG_PKT_ID_REPLICATION_FACTOR; i++) {
    uint8_t *ptr_1 =
        &pkt_buf[pkt_size -
                 ((i + 1) * (sizeof(pkt_id_type) - diag_min_size_off))];
    uint8_t *ptr_2 = (uint8_t *)&encoded_pkt_id;
    // FIXME need to handle endianness properly
    for (int j = 0; j < (sizeof(pkt_id_type) - diag_min_size_off); j++) {
      ptr_1[j] = ptr_2[j];
    }
  }

  // Increment the pkt_id before exiting
  g_pkt_id++;

  return encoded_pkt_id;
}

// Decode pkt_id, sess_hdl, and dev_port from the tail of the packet
pkt_id_type diag_decode_pkt_id(const uint32_t pkt_size,
                               const uint8_t *pkt_buf,
                               pkt_id_type *pkt_id,
                               bf_diag_sess_hdl_t *sess_hdl,
                               bf_dev_port_t *dev_port) {
  std::lock_guard<std::mutex> lock(mtx);

  uint8_t total_bytes_req = DIAG_PKT_ID_ENCODE_SIZE;
  if ((pkt_size - DIAG_PKT_HEADER_SIZE) < total_bytes_req) {
    // DIAG_PRINT("Pkt size (%d) not sufficient to decode pkt_id\n", pkt_size);
    *pkt_id = PKT_DATABASE_ERR;
    return PKT_DATABASE_ERR;
  }

  // We start from the tail of the packet and decode consecutive copies of the
  // pkt_id
  pkt_id_type pkt_id_arr[PKT_ID_REPLICATION_FACTOR_DEF] = {0};
  int diag_min_size_off = DIAG_MIN_PKT_SIZE_ENABLED ? 2 : 0;
  for (int i = 0; i < DIAG_PKT_ID_REPLICATION_FACTOR; i++) {
    const uint8_t *ptr_1 =
        &pkt_buf[pkt_size -
                 ((i + 1) * (sizeof(pkt_id_type) - diag_min_size_off))];
    uint8_t *ptr_2 = (uint8_t *)&pkt_id_arr[i];
    // FIXME need to handle endianness properly
    for (int j = 0; j < (sizeof(pkt_id_type) - diag_min_size_off); j++) {
      ptr_2[j] = ptr_1[j];
    }
  }

  for (int i = 0; i < (DIAG_PKT_ID_REPLICATION_FACTOR - 1); i++) {
    if (pkt_id_arr[i] != pkt_id_arr[i + 1]) {
#if 0
      DIAG_PRINT(
          "Mismatch in decoded pkt_ids (%d:%lx %d:%lx) from the payload\n",
          i,
          pkt_id_arr[i],
          i + 1,
          pkt_id_arr[i + 1]);
#endif
      *pkt_id = PKT_DATABASE_ERR;
      return PKT_DATABASE_ERR;
    }
  }

  *pkt_id = pkt_id_arr[0];
  diag_decode_sess_port_from_pkt_id(*pkt_id, sess_hdl, dev_port);

  return 0;
}

// Cleanup the pkt database
void diag_cleanup_pkt_database(const bf_diag_sess_hdl_t sess_hdl) {
  std::lock_guard<std::mutex> lock(mtx);

  DIAG_PRINT("Cleaning up pkt database for session %d\n", sess_hdl);

  // Go through the entire map and remove all packets which belong to this
  // particular session
  for (auto item = g_pkt_database.begin(); item != g_pkt_database.end();) {
    pkt_id_type pkt_id = item->first;
    bf_diag_sess_hdl_t decoded_sess_hdl;
    bf_dev_port_t dev_port;
    diag_decode_sess_port_from_pkt_id(pkt_id, &decoded_sess_hdl, &dev_port);
    if (sess_hdl == decoded_sess_hdl) {
      item = g_pkt_database.erase(item);
    } else {
      item++;
    }
  }
}

/* Compare received pkt with the replica in the database */
bf_status_t diag_compare_pkt_with_database(
    const uint32_t size,
    const uint8_t *received_pkt,
    const pkt_id_type pkt_id,
    const bf_diag_sess_hdl_t sess_hdl,
    const bf_dev_port_t ingress_port,
    uint8_t *exp_bytes,
    uint32_t *cached_pkt_size,
    uint64_t *num_bytes_mismatch,
    uint64_t *num_bits_mismatch,
    uint64_t *num_1_to_0_flips,
    uint64_t *num_0_to_1_flips,
    diag_slt_failure_type_e *failure_type) {
  uint32_t i = 0, j = 0;
  *num_bytes_mismatch = 0;
  *num_bits_mismatch = 0;
  *num_1_to_0_flips = 0;
  *num_0_to_1_flips = 0;
  *cached_pkt_size = 0;
  uint8_t bits_mismatch = 0;
  uint32_t num_bytes_added_by_dataplane = 0;
  uint8_t flips_1_to_0 = 0;
  uint8_t flips_0_to_1 = 0;
  bool is_1_to_0_flip = false;
  int max_print_per_packet = 0;
  bool is_suspect_for_setup = false;
  bool is_strong_suspect_for_setup = false;
  bool is_suspect_for_hold = false;
  bool is_suspect_for_unknown = false;
  bool is_suspect_for_setup_payload = false;

  bool pkt_is_payload_setup = false;
  bool pkt_is_strong_suspect_for_setup = false;

  // Lets be optimistic and set the failure_type to none
  *failure_type = DIAG_SLT_FAILURE_TYPE_NO_FAILURE;

  // Create a copy of the received packet
  uint8_t buf[DIAG_MAX_PKT_SIZE];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, received_pkt, size);

  /* Retrieve the cached pkt from the pkt_id */
  if (diag_get_pkt_from_db(pkt_id, size, cached_pkt_size, exp_bytes) != 0) {
    DIAG_PRINT(
        ">>>>>>>>> Unable to retrieve pkt for pkt_id %lx from the "
        "database "
        "<<<<<<<<<\n",
        pkt_id);
    return BF_UNEXPECTED;
  }

  // Safety check
  // Recieved pkt size can be larger than cached pkt size if we are in
  // DIAG_PATTERN_SHIFT_ENABLE  mode. For all other cases, it will be
  // equal.
  if (size < *cached_pkt_size) {
    DIAG_PRINT(
        ">>>>>>>>> Received pkt size (%d) is smaller than the sent "
        "one (%d) "
        "<<<<<<<<<\n",
        size,
        *cached_pkt_size);
    return BF_UNEXPECTED;
  }

#if defined(DIAG_PATTERN_SHIFT_ENABLE) || defined(DIAG_PHV_FLOP_CONFIG_3)
  // In this mode, the dataplane would have added (size-cached_pkt_size)
  // number of bytes in the packet after the header of 240 Bytes. To keep
  // rest of the comparison algo the same, lets overwrite the bytes added
  // by the dataplane with the bytes which were there in the packet from
  // from the beginning.
  int start_of_payload = 0;
#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  start_of_payload = 240;
#elif defined(DIAG_PHV_FLOP_CONFIG_3)
  start_of_payload = 125;
#endif
  if (size > *cached_pkt_size) {
    num_bytes_added_by_dataplane = size - *cached_pkt_size;
    for (i = start_of_payload; i < *cached_pkt_size; i++) {
      buf[i] = buf[i + num_bytes_added_by_dataplane];
    }
  }
#endif  // DIAG_PATTERN_SHIFT_ENABLE

  max_print_per_packet = 0;
  /* Compare the cached packet with the received packet */
  for (i = 0; i < *cached_pkt_size; i++) {
#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
    if ((i == DIAG_TCP_SRC_PORT_BYTE) || (i == DIAG_TCP_SRC_PORT_BYTE + 1)) {
      /* Don't compare these bytes because the dataplane would have filled
         in these values */
      continue;
    } else {
      break;
    }
#endif
    if (exp_bytes[i] != buf[i]) {
      (*num_bytes_mismatch)++;

      bits_mismatch = 0;
      flips_1_to_0 = 0;
      flips_0_to_1 = 0;
      // Now there can be multiple flips in a byte
      for (j = 0; j < 8; j++) {
        is_1_to_0_flip = false;
        is_suspect_for_setup = false;
        is_strong_suspect_for_setup = false;
        is_suspect_for_hold = false;
        is_suspect_for_setup_payload = false;
        if (((buf[i] >> j) & 0x01) != ((exp_bytes[i] >> j) & 0x01)) {
          bits_mismatch++;
          if (((buf[i] >> j) & 0x01) == 0) {
            flips_1_to_0++;
            is_1_to_0_flip = true;
            // For this to be a suspect for setup issue, this byte needs to be
            // within the header
            if (i < diag_get_header_size()) {
              // if the bit that flipped is bit 1, then this is a strong suspect
              // for setup
              if (j == 1) {
                is_strong_suspect_for_setup = true;
                is_suspect_for_setup = false;
                pkt_is_strong_suspect_for_setup = true;
              } else {
                is_suspect_for_setup = true;
                is_strong_suspect_for_setup = false;
              }
            } else {
              is_suspect_for_setup_payload = true;
              pkt_is_payload_setup = true;
            }
          } else {
            is_1_to_0_flip = false;
            flips_0_to_1++;
            // Hold issue can occur in the header as well as the payload. So
            // mark it as a suspect for hold always
            is_suspect_for_hold = true;
          }
          DIAG_PRINT(
              "BIT_FLIP pkt_id %8lx : Exp Byte: 0x%-2x : Rcv Byte: 0x%-2x : "
              "Byte Offset: %-4d : Bit Offset: %-1d : %-4s : %-9s\n",
              pkt_id,
              exp_bytes[i],
              buf[i],
              i,
              j,
              is_1_to_0_flip ? "1->0" : "0->1",
              is_suspect_for_setup
                  ? "S_Setup"
                  : is_strong_suspect_for_setup
                        ? "SS_Setup"
                        : is_suspect_for_hold
                              ? "S_Hold"
                              : is_suspect_for_setup_payload ? "Payload_Setup"
                                                             : "Unknown");
        }
      }
      (*num_bits_mismatch) = (*num_bits_mismatch) + bits_mismatch;
      (*num_1_to_0_flips) = (*num_1_to_0_flips) + (flips_1_to_0);
      (*num_0_to_1_flips) = (*num_0_to_1_flips) + (flips_0_to_1);
      // If (1->0 flips + 0->1 flips) is greater than 1, that that indicaes
      // that the byte suffered from multibit corruption.
      if ((flips_1_to_0 + flips_0_to_1) > 1) {
        // Before we proceed with doing any analytics on this byte/packet
        // make sure that the byte does not have any multibit corruptions
        // If there are multiple bit corruptions within a byte, then the
        // overall failure type of the packet is unknown
        *failure_type = DIAG_SLT_FAILURE_TYPE_UNKNOWN;
      } else {
        // We try to classify the packets only if the failure mode is known
        // Log the bit flips details so that we can categorize this pkt later
        diag_log_bit_flip_in_pkt(pkt_id, i, flips_1_to_0 == 1);
      }
    }
  }

  if (*failure_type == DIAG_SLT_FAILURE_TYPE_UNKNOWN) {
    diag_print_pkt_classification_summary(pkt_id,
                                          sess_hdl,
                                          ingress_port,
                                          size + DIAG_PKT_CRC_HEADER_SIZE,
                                          "Unknown",
                                          *num_bytes_mismatch,
                                          *num_bits_mismatch);
    return BF_UNEXPECTED;
  }

  if (*num_bytes_mismatch) {
    // Finally see if the packet can be categorized as a strong suspect for
    // setup
    // or hold issue
    bool is_weak_suspect_for_setup = false;
    bool is_strong_suspect_for_setup = false;
    bool is_weak_suspect_for_hold = false;
    bool is_strong_suspect_for_hold = false;
    std::vector<std::vector<uint32_t>> hold_bit_flips_vec;
    diag_classify_suspicion(pkt_id,
                            &is_weak_suspect_for_setup,
                            &is_weak_suspect_for_hold,
                            &is_strong_suspect_for_hold,
                            &hold_bit_flips_vec);

    // Check if any byte in the packet was classified as a strong suspect for
    // for setup. If yes, then is_weak_suspect_for_setup needs to be false
    if (pkt_is_strong_suspect_for_setup) {
      is_strong_suspect_for_setup = true;
      is_weak_suspect_for_setup = false;
    }

    // Sanity check. hold_bit_flips_vec should be non-empty only if the packet
    // is a strong suspect for hold. Else we have messed up the classification
    // logic
    if ((!is_strong_suspect_for_hold && hold_bit_flips_vec.size()) ||
        (is_strong_suspect_for_hold && (!hold_bit_flips_vec.size()))) {
      DIAG_PRINT(
          "BIT_FLIP pkt_id %8lx : Strong hold suspicion logic messed up : "
          "strong_suspect (%d) bit_flip_vec size (%lu). Asserting\n",
          pkt_id,
          is_strong_suspect_for_hold,
          hold_bit_flips_vec.size());
      DIAG_ASSERT(0);
    }

    // Determine the final failure type for the packet
    std::string failure_name;
    diag_get_overall_pkt_failure_type(pkt_id,
                                      is_weak_suspect_for_setup,
                                      is_strong_suspect_for_setup,
                                      is_weak_suspect_for_hold,
                                      is_strong_suspect_for_hold,
                                      pkt_is_payload_setup,
                                      failure_type,
                                      &failure_name);

    diag_print_pkt_classification_summary(pkt_id,
                                          sess_hdl,
                                          ingress_port,
                                          size + DIAG_PKT_CRC_HEADER_SIZE,
                                          failure_name,
                                          *num_bytes_mismatch,
                                          *num_bits_mismatch);

    if (is_strong_suspect_for_hold) {
      DIAG_PRINT(
          "BIT_FLIP pkt_id %8lx : 80 byte boundary failures found in the "
          "following bytes (<byte_offset:exp_byte:recv_byte>):\n",
          pkt_id);
      std::for_each(
          hold_bit_flips_vec.begin(),
          hold_bit_flips_vec.end(),
          [&](const std::vector<uint32_t> vec) {
            DIAG_PRINT("BIT_FLIP pkt_id %8lx : ", pkt_id);
            for (auto iter = vec.begin(); iter != vec.end(); iter++) {
              DIAG_PRINT(
                  "<%-4d:%-2x:%-2x> ", *iter, exp_bytes[*iter], buf[*iter]);
            }
            DIAG_PRINT("\n");
          });
    }
    return BF_UNEXPECTED;
  } else {
    // No bit flips in the packet
    *failure_type = DIAG_SLT_FAILURE_TYPE_NO_FAILURE;
  }

  // Keep the compiler happy
  (void)num_bytes_added_by_dataplane;
  return BF_SUCCESS;
}

//} // namespace diag
