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


/*!
 * @file diag_create_pkt.h
 * @date
 *
 * Contains definitions of diag pkt send thread, recv callbacks
 *
 */
#ifndef _DIAG_CREATE_PKT_H
#define _DIAG_CREATE_PKT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "diags/bf_diag_api.h"
#include "diag_common.h"

#define DIAG_PKT_MAC_SIZE 6
#define DIAG_PKT_TTL 64
/* Ethernet: 14 bytes, IP :20 bytes, TCP: 20 bytes */
#define DIAG_ETHERNET_HDR_SIZE 14
#define DIAG_SRC_MAC_START_BYTE 6
#define DIAG_ETHERNET_ETHER_TYPE 12

/* IPv6 packet */
#define DIAG_IP_HDR_SIZE (DIAG_MIN_PKT_SIZE_ENABLED ? 20 : 40)
#define DIAG_IP_TTL_BYTE                                        \
  (DIAG_MIN_PKT_SIZE_ENABLED ? (DIAG_ETHERNET_HDR_SIZE + 9 - 1) \
                             : (DIAG_ETHERNET_HDR_SIZE + 8 - 1))
#define DIAG_IP_ID_BYTE                                         \
  (DIAG_MIN_PKT_SIZE_ENABLED ? (DIAG_ETHERNET_HDR_SIZE + 5 - 1) \
                             : (DIAG_ETHERNET_HDR_SIZE + 3 - 1))
#define DIAG_IP_ADDR_SIZE (DIAG_MIN_PKT_SIZE_ENABLED ? 4 : 8)
#define DIAG_IP_SRC_BYTE                                     \
  (DIAG_MIN_PKT_SIZE_ENABLED ? (DIAG_ETHERNET_HDR_SIZE + 12) \
                             : (DIAG_ETHERNET_HDR_SIZE + 8))
#define DIAG_IP_DST_BYTE                                   \
  (DIAG_MIN_PKT_SIZE_ENABLED                               \
       ? (DIAG_ETHERNET_HDR_SIZE + 12 + DIAG_IP_ADDR_SIZE) \
       : (DIAG_ETHERNET_HDR_SIZE + 8 + DIAG_IP_ADDR_SIZE))

#define DIAG_TCP_HDR_SIZE 20

#ifndef DIAG_MAU_BUS_STRESS_ENABLE
#define DIAG_MAU_BUS_STRESS_HDR_SIZE (0)
#else
#define DIAG_MAU_BUS_STRESS_HDR_SIZE (192 + 192)
#define DIAG_MAU_BUS_STRESS_HDR_START_OFFSET \
  (DIAG_ETHERNET_HDR_SIZE + DIAG_IP_HDR_SIZE + DIAG_TCP_HDR_SIZE)
#endif

#ifndef DIAG_PARDE_STRAIN
#define DIAG_PARDE_STRAIN_HDR_SIZE (0)
#else
#define DIAG_PARDE_STRAIN_HDR_SIZE (1)
#endif

#define DIAG_PKT_HEADER_SIZE                                       \
  (DIAG_ETHERNET_HDR_SIZE + DIAG_IP_HDR_SIZE + DIAG_TCP_HDR_SIZE + \
   DIAG_MAU_BUS_STRESS_HDR_SIZE)
#define DIAG_PKT_CRC_HEADER_SIZE 4
#define DIAG_PKT_SEED_SIZE 2
#define DIAG_TCP_SRC_PORT_BYTE (DIAG_ETHERNET_HDR_SIZE + DIAG_IP_HDR_SIZE)
#define DIAG_TCP_DST_PORT_BYTE (DIAG_TCP_SRC_PORT_BYTE + 2)
#define DIAG_ETHERNET_DST_MAC_BYTE 0
#define DIAG_ETHERNET_SRC_MAC_BYTE 6
#define PKT_ID_REPLICATION_FACTOR_DEF 3
#define DIAG_PKT_ID_ENCODE_SIZE                                       \
  (((int)sizeof(pkt_id_type) - (DIAG_MIN_PKT_SIZE_ENABLED ? 2 : 0)) * \
   DIAG_PKT_ID_REPLICATION_FACTOR)
#define DIAG_FULL_PKT_HDR_SIZE \
  (DIAG_PKT_HEADER_SIZE + DIAG_PKT_CRC_HEADER_SIZE + DIAG_PKT_ID_ENCODE_SIZE)
#define DIAG_MIN_PKT_SIZE \
  (((DIAG_FULL_PKT_HDR_SIZE) > 64) ? (DIAG_FULL_PKT_HDR_SIZE) : 64)

typedef enum {
  DIAG_PKT_LOG_OFF = 0,
  DIAG_PKT_LOG_ERR = 0x1,
  DIAG_PKT_LOG_ALL = 0x2,
} diag_pkt_log_level_e;

int diag_create_pkt(bf_diag_sess_hdl_t sess_hdl,
                    int pkt_size,
                    uint8_t *buffer,
                    uint32_t pkt_num,
                    bool min_pkt_size);
int diag_create_payload(bf_diag_sess_hdl_t sess_hdl,
                        int size,
                        uint16_t random_seed,
                        uint8_t *raw_bytes);
void diag_pkt_logs_val_set(uint32_t val);
uint32_t diag_pkt_log_val_get();
int diag_payload_data_patterns_set(bf_diag_sess_hdl_t sess_hdl,
                                   uint8_t s,
                                   uint32_t s_len,
                                   uint8_t a,
                                   uint8_t b,
                                   uint32_t pattern_len);
int diag_payload_data_patterns_set_def(bf_diag_sess_hdl_t sess_hdl);
int diag_payload_data_patterns_get(bf_diag_sess_hdl_t sess_hdl,
                                   uint8_t *s,
                                   uint32_t *s_len,
                                   uint8_t *a,
                                   uint8_t *b,
                                   uint32_t *pattern_len);
int diag_use_fixed_pattern_set(bf_diag_sess_hdl_t sess_hdl, bool val);
bool is_diag_use_fixed_pattern(bf_diag_sess_hdl_t sess_hdl);

int diag_payload_type_set(bf_diag_sess_hdl_t sess_hdl,
                          bf_diag_packet_payload_t val);
bool is_diag_use_fixed_payload(bf_diag_sess_hdl_t sess_hdl);
int diag_packet_full_type_set(bf_diag_sess_hdl_t sess_hdl,
                              bf_diag_packet_full_t val);
bool is_diag_use_packet_full(bf_diag_sess_hdl_t sess_hdl);
bool is_diag_use_random_flip_payload(bf_diag_sess_hdl_t sess_hdl);
int diag_fixed_payload_set(bf_diag_sess_hdl_t sess_hdl,
                           bool payload_file_valid,
                           const char *payload,
                           const char *payload_file_path);
int diag_fixed_payload_set_def(bf_diag_sess_hdl_t sess_hdl);
int diag_fixed_payload_cnt_get(bf_diag_sess_hdl_t sess_hdl,
                               uint32_t *pattern_cnt);
int diag_fixed_payload_get(bf_diag_sess_hdl_t sess_hdl,
                           uint32_t pattern_num,
                           char **payload);

int diag_packet_full_set(bf_diag_sess_hdl_t sess_hdl,
                         bool pkt_file_valid,
                         const char *pkt,
                         const char *pkt_file_path);
int diag_packet_full_set_def(bf_diag_sess_hdl_t sess_hdl);
int diag_packet_full_get(bf_diag_sess_hdl_t sess_hdl, char **full_pkt);

int diag_use_fixed_pkt_contents_set(bf_diag_sess_hdl_t sess_hdl, bool val);
bool is_diag_use_fixed_pkt_contents(bf_diag_sess_hdl_t sess_hdl);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
