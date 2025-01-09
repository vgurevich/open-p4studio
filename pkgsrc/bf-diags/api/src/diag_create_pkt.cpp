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
#include <fstream>
#include <string>
#include "crafter.h"
#include "diag_create_pkt.h"
#include "diag_pd.h"

/* Collapse namespaces */
using namespace std;
using namespace Crafter;

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t DIAG_CPU_SRC_MAC[DIAG_PKT_MAC_SIZE] = {
    0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xEE};

int diag_create_pkt(bf_diag_sess_hdl_t sess_hdl,
                    int pkt_size,
                    uint8_t *buffer,
                    uint32_t pkt_num,
                    bool min_pkt_size) {
  bool use_fixed_pkt_contents = is_diag_use_fixed_pkt_contents(sess_hdl);
  bool use_fixed_pattern = is_diag_use_fixed_pattern(sess_hdl);
  bool use_fixed_payload = is_diag_use_fixed_payload(sess_hdl);
  bool use_full_pkt = is_diag_use_packet_full(sess_hdl);
  Packet packet;

#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
  /* Set the interface */
  string SrcMac = "";
  string DstMac = "";
  string DstMacEnd = "00:02:00:00:01";
  string separator = ":";
  Ethernet ether_header;
  static uint32_t qid = 0;
  static uint32_t diag_pkt_counter = 0;
  static uint8_t traffic_class = 100;
  static uint8_t hop_limit = 64;
  static uint32_t tcp_seq_no = 0x1a5b216e;
  static uint32_t tcp_ack_no = 0x2b1c1378;
  static uint16_t tcp_window_size = 0x521b;
  static uint16_t tcp_urgent = 0x8ce3;
  static uint8_t tcp_reserved = 0;

  char smac_byte[100];
  for (int i = 0; i < DIAG_PKT_MAC_SIZE; i++) {
    snprintf(smac_byte, 100, "%x", DIAG_CPU_SRC_MAC[i]);
    SrcMac = SrcMac + string(smac_byte);
    if (i < (DIAG_PKT_MAC_SIZE - 1)) {
      SrcMac = SrcMac + separator;
    }
  }
  /* The rx_pkt DR is determined by the first byte of the packet,
     spraying the packets equally on all queues
  */
  if (!use_fixed_pkt_contents) {
    qid = (qid + 1) % 8;
  }
  char dmac_msb[100];
  snprintf(dmac_msb, 100, "%x", qid);
  DstMac = dmac_msb + separator + DstMacEnd;
  ether_header.SetDestinationMAC(DstMac);
  ether_header.SetSourceMAC(SrcMac);

  /* Generate a random seed of 16 bits max */
  uint16_t random_seed = 0;
  /* Generate a seed value only for non-fixed packets */
  if ((!use_fixed_pkt_contents) && (!use_fixed_pattern) &&
      (!use_fixed_payload) && (!use_full_pkt)) {
    random_seed = (time(NULL) + diag_pkt_counter) % 65535;
    /* Zero seed implies a fixed payload, make sure seed is not zero */
    if (random_seed == 0) {
      random_seed = 1;
    }
    diag_pkt_counter++;
  }
  if (use_fixed_payload) {
    uint32_t fixed_payload_cnt = 0;
    /* Put the pattern number in the seed if there are fixed payloads,
       For one payload pattern the seed will be zero.
     */
    diag_fixed_payload_cnt_get(sess_hdl, &fixed_payload_cnt);
    random_seed = pkt_num % fixed_payload_cnt;
  }
  if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ALL) {
    cout << endl << "Random seed in send is: " << random_seed << endl;
  }

  /* Create a TCP header */
  TCP tcp_header;

  /* Set the source and destination ports */
  tcp_header.SetSrcPort(0);
  tcp_header.SetDstPort(62346);

  /* Set some TCP flags */
  tcp_header.SetFlags(TCP::SYN | TCP::CWR | TCP::ECE);

  IPv6 ipv6_header;
  IP ip_header;
  if (!min_pkt_size) {
    /* Add random data in tcp header fields */
    if (!use_fixed_pkt_contents) {
      tcp_seq_no = tcp_seq_no + 1001;
      tcp_ack_no = tcp_ack_no + 5031;
      tcp_window_size = tcp_window_size + 1;
      tcp_urgent = tcp_urgent + 1;
      tcp_reserved = (tcp_reserved + 1) % 7;
    }
    tcp_header.SetSeqNumber(tcp_seq_no);
    tcp_header.SetAckNumber(tcp_seq_no);
    tcp_header.SetWindowsSize(tcp_window_size);
    tcp_header.SetUrgPointer(tcp_urgent);
    tcp_header.SetReserved(tcp_reserved);

    if ((tcp_window_size % 3) == 0) {
      tcp_header.SetFlags(TCP::SYN | TCP::CWR | TCP::ECE);
    } else if ((tcp_window_size % 2) == 0) {
      tcp_header.SetFlags(TCP::RST | TCP::FIN);
    } else {
      tcp_header.SetFlags(TCP::CWR | TCP::PSH);
    }

    /* Add random data in ipv6 header fields */
    if (!use_fixed_pkt_contents) {
      traffic_class = traffic_class + 1;
      hop_limit = hop_limit + 1;
    }

    /* Create an IPv6 header */

    /* Get the IP address associated to the interface */
    string SrcIP = "0000::0000";
    string DstIP = "0000::0000";

    /* Set the Source and Destination IP address */
    ipv6_header.SetTrafficClass(traffic_class);
    ipv6_header.SetSourceIP(SrcIP);
    ipv6_header.SetDestinationIP(DstIP);
    ipv6_header.SetVersion(6);

#ifdef DIAG_POWER_ENABLE
    ipv6_header.SetHopLimit(hop_limit);
#else
    ipv6_header.SetHopLimit(DIAG_PKT_TTL);
#endif

    /* Encode the seed in Flow label field */
    ipv6_header.SetFlowLabel(random_seed);
  } else {
    /* Create an IPv4 header */

    /* Get the IP address associated to the interface */
    string SrcIP = "192.168.0.1";
    string DstIP = "192.168.0.2";

    /* Set the Source and Destination IP address */
    ip_header.SetSourceIP(SrcIP);
    ip_header.SetDestinationIP(DstIP);
    ip_header.SetTTL(DIAG_PKT_TTL);

    /* Encode the seed in the IP identification field */
    ip_header.SetIdentification(random_seed);
  }

  if (pkt_size < (DIAG_PKT_HEADER_SIZE)) {
    cout << endl << "Packet size specified is less than header size " << endl;
    return -1;
  }

#ifdef DIAG_MAU_BUS_STRESS_ENABLE
  /* Create mau_bus_stress header */
  RawLayer mau_bus_stress_header;
  uint8_t mau_bus_stress_bytes[DIAG_MAX_PKT_SIZE];
  /* Zero out mau_bus_stress header */
  memset(mau_bus_stress_bytes, 0, sizeof(mau_bus_stress_bytes));
  mau_bus_stress_header.SetPayload(mau_bus_stress_bytes,
                                   DIAG_MAU_BUS_STRESS_HDR_SIZE);
#endif
#ifdef DIAG_PARDE_STRAIN
  /* Create parde_strain header */
  RawLayer parde_strain_header;
  uint8_t parde_strain_bytes[DIAG_MAX_PKT_SIZE];
  /* Zero out parde_strain header */
  memset(parde_strain_bytes, 0, sizeof(parde_strain_bytes));
  parde_strain_header.SetPayload(parde_strain_bytes,
                                 DIAG_PARDE_STRAIN_HDR_SIZE);
#endif

  /* Create a payload */
  RawLayer raw_header;
  uint8_t raw_bytes[DIAG_MAX_PKT_SIZE];
  memset(raw_bytes, 0, sizeof(raw_bytes));

  diag_create_payload(
      sess_hdl, pkt_size - DIAG_PKT_HEADER_SIZE, random_seed, &raw_bytes[0]);
  raw_header.SetPayload(raw_bytes, pkt_size - DIAG_PKT_HEADER_SIZE);

  if (!min_pkt_size) {
    if (use_full_pkt) {
      packet = raw_header;
    } else {
/* Create an ipv6 packet... */
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
      packet = ether_header / ipv6_header / tcp_header / mau_bus_stress_header /
               raw_header;
#elif defined(DIAG_PARDE_STRAIN)
      packet = ether_header / ipv6_header / tcp_header / parde_strain_header /
               raw_header;
#else
      packet = ether_header / ipv6_header / tcp_header / raw_header;
#endif
    }
  } else {
    if (use_full_pkt) {
      packet = raw_header;
    } else {
/* Create an ipv4 packet... */
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
      packet = ether_header / ip_header / tcp_header / mau_bus_stress_header /
               raw_header;
#elif defined(DIAG_PARDE_STRAIN)
      packet = ether_header / ip_header / tcp_header / parde_strain_header /
               raw_header;
#else
      packet = ether_header / ip_header / tcp_header / raw_header;
#endif
    }
  }

#else  // DIAG_PHV_STRESS_ENABLE || DIAG_PHV_FLOP_TEST

  static uint32_t diag_pkt_counter = 0;

#ifndef DIAG_PATTERN_SHIFT_ENABLE
  /* Generate a random seed of 16 bits max */
  uint16_t random_seed = 0;
  /* Generate a seed value only for non-fixed packets */
  if ((!use_fixed_pkt_contents) && (!use_fixed_pattern) &&
      (!use_fixed_payload)) {
    random_seed = (time(NULL) + diag_pkt_counter) % 65535;
    /* Zero seed implies a fixed payload, make sure seed is not zero */
    if (random_seed == 0) {
      random_seed = 1;
    }
    diag_pkt_counter++;
  }
  if (use_fixed_payload) {
    uint32_t fixed_payload_cnt = 0;
    /* Put the pattern number in the seed if there are fixed payloads,
       For one payload pattern the seed will be zero.
     */
    diag_fixed_payload_cnt_get(sess_hdl, &fixed_payload_cnt);
    random_seed = pkt_num % fixed_payload_cnt;
  }
  /* Create the entire packet */
  RawLayer raw_header;
  uint8_t raw_bytes[DIAG_MAX_PKT_SIZE];
  memset(raw_bytes, 0, sizeof(raw_bytes));
  diag_create_payload(sess_hdl, pkt_size, random_seed, &raw_bytes[0]);
#if defined(TOFINO2) || defined(TOFINO3)
#if !defined(DIAG_PHV_FLOP_CONFIG_3)
  // Set the payload data size in the ethertype and set the top bit of the
  // ethertype so the packet can pass through an Ethernet MAC without triggering
  // a length error.
  int phv_stress_data_size = pkt_size - DIAG_ETHERNET_HDR_SIZE;
  raw_bytes[DIAG_ETHERNET_ETHER_TYPE] =
      0x80 | ((phv_stress_data_size >> 8) & 0x7f);
  raw_bytes[DIAG_ETHERNET_ETHER_TYPE + 1] = phv_stress_data_size & 0xff;
#endif
#else
  // In this mode, we want to ensure that the ethertype is a known good value
  // otherwise the comira MAC will interpret it as an error and drop the packet
  raw_bytes[DIAG_ETHERNET_ETHER_TYPE] = 0XFF;
  raw_bytes[DIAG_ETHERNET_ETHER_TYPE + 1] = 0XFF;
#endif
  raw_header.SetPayload(raw_bytes, pkt_size);

  packet = raw_header;
#else   // DIAG_PATTERN_SHIFT_ENABLE
  // In this compile mode, the composition of the pkt is as follows:
  // 240B (Random HEADER) + 1926B (Fixed PAYLOAD read from the file) + 24B (pkt
  // ids) + 4B CRC
  // The CRC is added by the PCIE interface internally so we won't consider it.
  // Thus the pkt size passed to this function would be (240 + 1926 + 24 = 2190)
  // We form the 240B and 1926B payload separately and then copy them into the
  // pkt buffer

  /* Always Generate a random seed of 16 bits max for the header */
  uint16_t random_seed = 0;
  random_seed = (time(NULL) + diag_pkt_counter) % 65535;
  /* Zero seed implies a fixed payload, make sure seed is not zero */
  if (random_seed == 0) {
    random_seed = 1;
  }
  /* Create random header of 240 Bytes */
  RawLayer raw_header;
  uint8_t raw_header_bytes[240];
  memset(raw_header_bytes, 0, sizeof(raw_header_bytes));
  unsigned int local_seed = 0;
  local_seed = (unsigned int)random_seed;
  for (int i = 0; i < 240; i++) {
    raw_header_bytes[i] = rand_r(&local_seed) % 256;
  }
  // In this mode, we want to ensure that the ethertype is a known good value
  // otherwise the comira MAC will interpret it as an error and drop the packet
  raw_header_bytes[DIAG_ETHERNET_ETHER_TYPE] = 0XFF;
  raw_header_bytes[DIAG_ETHERNET_ETHER_TYPE + 1] = 0XFF;
  raw_header.SetPayload(raw_header_bytes, 240);

  // Reset the random seed to 0
  random_seed = 0;
  if ((!use_fixed_pkt_contents) && (!use_fixed_pattern) &&
      (!use_fixed_payload)) {
    random_seed = (time(NULL) + diag_pkt_counter) % 65535;
    /* Zero seed implies a fixed payload, make sure seed is not zero */
    if (random_seed == 0) {
      random_seed = 1;
    }
  }
  diag_pkt_counter++;
  /* Create 1926 Byte payload read from file */
  if (use_fixed_payload) {
    uint32_t fixed_payload_cnt = 0;
    /* Put the pattern number in the seed if there are fixed payloads,
       For one payload pattern the seed will be zero.
     */
    diag_fixed_payload_cnt_get(sess_hdl, &fixed_payload_cnt);
    random_seed = pkt_num % fixed_payload_cnt;
  }
  RawLayer raw_payload;
  uint8_t raw_payload_bytes[1926];
  memset(raw_payload_bytes, 0, sizeof(raw_payload_bytes));
  diag_create_payload(sess_hdl, 1926, random_seed, &raw_payload_bytes[0]);
  raw_payload.SetPayload(raw_payload_bytes, 1926);

  packet = raw_header / raw_payload;
#endif  // DIAG_PATTERN_SHIFT_ENABLE

#endif  // DIAG_PHV_STRESS_ENABLE || DIAG_PHV_FLOP_TEST

  /* Copy packet */
  memcpy(buffer, packet.GetRawPtr(), packet.GetSize());

  return 0;
}

int diag_create_payload(bf_diag_sess_hdl_t sess_hdl,
                        int size,
                        uint16_t random_seed,
                        uint8_t *raw_bytes) {
  uint8_t pat_a = 0, pat_b = 0, start_pat = 0;
  unsigned int local_seed = 0;
  uint32_t pattern_len = 0, start_pat_len = 0;
  bool use_pat_a = false;
  char *fixed_payload = NULL, *pkt_full = NULL;
  bool use_fixed_pkt_contents = false, use_fixed_pattern = false;
  bool use_fixed_payload = false, use_random_flip_payload = false;
  bool use_full_pkt = false;
  int ret = 0;

  if (size < 0) {
    return 0;
  }

  /* Set the seed */
  local_seed = (unsigned int)random_seed;
  use_fixed_payload = is_diag_use_fixed_payload(sess_hdl);
  use_full_pkt = is_diag_use_packet_full(sess_hdl);
  use_random_flip_payload = is_diag_use_random_flip_payload(sess_hdl);

  /* If seed passed is zero, create fixed data pattern */
  if (random_seed == 0) {
    use_fixed_pkt_contents = is_diag_use_fixed_pkt_contents(sess_hdl);
    use_fixed_pattern = is_diag_use_fixed_pattern(sess_hdl);

    if (use_fixed_pattern) {
      ret = diag_payload_data_patterns_get(
          sess_hdl, &start_pat, &start_pat_len, &pat_a, &pat_b, &pattern_len);
      if (ret != 0) {
        return 0;
      }
      if (pattern_len == 0) {
        return 0;
      }
    }
  }
  if (use_fixed_payload) {
    /* Seed will be zero if there is only one fixed payload.
      Seed can be non-zero if there are multiple fixed payloads.
    */
    ret = diag_fixed_payload_get(sess_hdl, random_seed, &fixed_payload);
    if (ret != 0) {
      return 0;
    }
  }
  if (use_full_pkt) {
    /* Seed will be zero if there is only one fixed payload.
      Seed can be non-zero if there are multiple fixed payloads.
    */
    ret = diag_packet_full_get(sess_hdl, &pkt_full);
    if (ret != 0) {
      return 0;
    }
  }

  for (int i = 0; i < size; i++) {
    if (use_fixed_pattern) {
      if ((start_pat_len != 0) && (i < start_pat_len)) {
        raw_bytes[i] = start_pat;
      } else {
        if (((i - start_pat_len) % pattern_len) == 0) {
          use_pat_a = (!use_pat_a);
        }
        if (use_pat_a) {
          raw_bytes[i] = pat_a;
        } else {
          raw_bytes[i] = pat_b;
        }
      }
    } else if (use_fixed_pkt_contents) {
      raw_bytes[i] = 0xa;
    } else if (use_fixed_payload) {
      raw_bytes[i] = fixed_payload[i];
    } else if (use_full_pkt) {
      raw_bytes[i] = pkt_full[i];
    } else if (use_random_flip_payload) {
      /* 16 bytes random data then 16 bytes of flipped data */
      int grp = i / 16;
      if ((grp % 2) == 0) {
        raw_bytes[i] = rand_r(&local_seed) % 256;
      } else {
        raw_bytes[i] = ~raw_bytes[(grp - 1) * 16 + i % 16];
      }
    } else {
      raw_bytes[i] = rand_r(&local_seed) % 256;
    }
  }
  return 0;
}

#ifdef __cplusplus
}  // extern "C"
#endif
