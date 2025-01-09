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


#ifndef lld_dr_descriptors_h
#define lld_dr_descriptors_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// Free Memory DRs
#define format_dr_msg_fm_wd0(wd0, addr, sz) \
  wd0 = (uint64_t)((addr & ~(0xff)) | (sz & 0xf));

#define extract_dr_msg_fm_wd0(wd0, addr, sz) \
  addr = BITS64(wd0, 63, 4) << 4;            \
  sz = BITS64(wd0, 3, 0);

#define convert_free_memory_sz(sz) (1 << (8 + sz))

// RX DRs
typedef struct dr_msg_rx_s {
  uint64_t wd0;
  uint64_t address_data;
} dr_msg_rx_t;

typedef enum {
  rx_m_type_lrt = 0,
  rx_m_type_idle,
  rx_m_type_reserved1,
  rx_m_type_learn,
  rx_m_type_pkt,
  rx_m_type_reserved2,
  rx_m_type_reserved3,
  rx_m_type_diag,
  rx_m_type_num
} dr_rx_msg_type;

#define extract_dr_msg_rx_wd0(wd0, data_sz, attr, status, type, e, s) \
  data_sz = BITS64(wd0, 63, 32);                                      \
  attr = BITS64(wd0, 31, 7);                                          \
  status = BITS64(wd0, 6, 5);                                         \
  type = BITS64(wd0, 4, 2);                                           \
  e = BITS64(wd0, 1, 1);                                              \
  s = wd0 & 1;

// TX DRs

#define format_dr_msg_tx_ilist_wd0(wd0, data_sz, list_typ, rsp_sz, type, e, s) \
  wd0 = ((uint64_t)data_sz << 32ull) | (rsp_sz << 6ull) | (list_typ << 5ull) | \
        (type << 2ull) | (e << 1ull) | s

#define format_dr_msg_tx_wd0(wd0, data_sz, attr, type, e, s)             \
  wd0 = ((uint64_t)data_sz << 32ull) | (attr << 5ull) | (type << 2ull) | \
        (e << 1ull) | s

#define extract_dr_msg_tx_wd0(wd0, data_sz, attr, type, e, s) \
  data_sz = BITS64(wd0, 63, 32);                              \
  attr = BITS64(wd0, 31, 5);                                  \
  type = BITS64(wd0, 4, 2);                                   \
  e = BITS64(wd0, 1, 1);                                      \
  s = wd0 & 1;

typedef struct dr_msg_tx_s {
  uint64_t wd0;
  uint64_t source_address;
  uint64_t destination_address;
  uint64_t message_id;
} dr_msg_tx_t;

typedef enum {
  tx_m_type_mac_stat = 0,
  tx_m_type_il,
  tx_m_type_reserved1,
  tx_m_type_wr_blk,
  tx_m_type_rd_blk,
  tx_m_type_que_wr_list,
  tx_m_type_pkt,
  tx_m_type_mac_wr_blk,
  tx_m_type_que_rd_blk = 4,
} dr_tx_msg_type;

// Completion DRs
#define extract_dr_msg_cmp_wd0(wd0, data_sz, attr, status, type, e, s) \
  data_sz = BITS64(wd0, 63, 32);                                       \
  attr = BITS64(wd0, 31, 7);                                           \
  status = BITS64(wd0, 6, 5);                                          \
  type = BITS64(wd0, 4, 2);                                            \
  e = BITS64(wd0, 1, 1);                                               \
  s = wd0 & 1;

#define extract_tof2_dr_msg_cmp_wd0(wd0, timestamp, attr, status, type, e, s) \
  timestamp = BITS64(wd0, 63, 24);                                            \
  attr = BITS64(wd0, 23, 7);                                                  \
  status = BITS64(wd0, 6, 5);                                                 \
  type = BITS64(wd0, 4, 2);                                                   \
  e = BITS64(wd0, 1, 1);                                                      \
  s = wd0 & 1;

#define extract_tof3_dr_msg_cmp_wd0(wd0, timestamp, attr, status, type, e, s) \
  extract_tof2_dr_msg_cmp_wd0(wd0, timestamp, attr, status, type, e, s);





typedef struct dr_msg_cmp_s {
  uint64_t wd0;
  uint64_t message_id;
} dr_msg_cmp_t;

// special handling for metadata
#define format_metadata_wd0(wd0) wd0 = (0x3FFFFFF << 5) | (1 << 2)

#define format_metadata_wd2(wd2, tbl, num_ops) \
  wd2 = ((uint64)tbl << 63) | (num_ops)

#define is_metadata_msg_tx_wd0(wd0) \
  (((wd0 << (63 - 31)) >> (63 - 31 + 5)) == 0x3FFFFFF)

#define extract_metadata_tx_wd2(wd2, table, num_ops) \
  table = wd2 >> 63LL;                               \
  num_ops = wd2 & 0xFFFFFFFFLL;

typedef struct metadata_msg_tx_s {
  uint64_t wd0;
  uint64_t metadata_address;
  uint64_t table_type_num_ops;  // bit 63, 0=lrt, 1=idle, 31:0 count of ops
  uint64_t message_id;
} metadata_msg_tx_t;

typedef union dr_descr_value_u {
  dr_msg_rx_t dr_msg_rx_v;
  dr_msg_cmp_t dr_msg_cmp_v;
  uint64_t raw_v[2];
} dr_descr_value_t;

#ifdef __cplusplus
}
#endif /* C++ */

#endif
