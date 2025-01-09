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


#ifndef _PIPE_MGR_MIRROR_BUFFER_COMM_H_
#define _PIPE_MGR_MIRROR_BUFFER_COMM_H_
/* internal session info */
typedef struct pipe_mgr_mirror_tof3_session_entry_ {
  // word0
  uint32_t hash_cfg_flag : 1;   /*!< 1-hash from word3 configuration*/
  uint32_t hash_cfg_flag_p : 1; /*!< valid when hash_cfg_flag set to 1
                                 * 1-word3 hash1, 0-word3 hash2*/
  uint32_t icos_cfg_flag : 1;   /*!< 1-icos from word4 configuration*/
  uint32_t dod_cfg_flag : 1; /*!< 1-deflect_on_drop from word2 configuration*/
  uint32_t c2c_cfg_flag : 1; /*!< cope to cpu cfg
                              * 1-it may take c2c_cos/c2c_vld from word4
                              * configuration*/
  uint32_t mc_cfg_flag : 1;  /*!< multicast cfg
                              * 1-it may take mcid1/2_vld/pipe_vec
                              * from word4/3 configuration*/
  uint32_t
      epipe_cfg_flag : 1; /*!< 1-epipe_cfg dynamic from word2 configuration*/
  uint32_t rsv_1 : 9;     // filling gap
  uint32_t mcast_l1_xid : 16;
  // word1
  uint16_t mcid1;
  uint16_t mcid2;
  // word2
  uint32_t epipe_port : 11;
  uint32_t rsv_2 : 3;  // filling gap
  uint32_t epipe_port_v : 1;
  uint32_t deflect_on_drop : 1;
  uint32_t mcast_rid : 16;
  // word3
  uint32_t hash1 : 13;
  uint32_t hash2 : 13;
  uint32_t tm_vec : 4;
  uint32_t rsv_3 : 1;           // filling gap
  uint32_t egr_bypass_flag : 1; /*!< 1-engress pipeline will be bypassed*/
  // word4
  uint32_t icos : 3;
#define TOF3_MIRROR_ICOS 7
  uint32_t color : 2;
  uint32_t mcid1_vld : 1;
  uint32_t mcid2_vld : 1;
  uint32_t c2c_cos : 3;
#define TOF3_MIRROR_ICOS_C2C 7
  uint32_t c2c_vld : 1;
  uint32_t yid_tbl_sel : 1;
  uint32_t eport_qid : 7;
#define TOF3_MIRROR_EGR_PORT_Q 127
  uint32_t rsv_4 : 3;  // filling gap
  uint32_t mcast_l2_xid : 10;
  // miscellaneous
  uint8_t pipe_vec;
} pipe_mgr_mirror_tof3_session_entry_t;

/* internal session info */
typedef struct pipe_mgr_mirror_tof2_session_entry_ {
  // word0
  uint32_t hash_cfg_flag : 1;   /*!< 1-hash from word3 configuration*/
  uint32_t hash_cfg_flag_p : 1; /*!< valid when hash_cfg_flag set to 1
                                 * 1-word3 hash1, 0-word3 hash2*/
  uint32_t icos_cfg_flag : 1;   /*!< 1-icos from word4 configuration*/
  uint32_t dod_cfg_flag : 1; /*!< 1-deflect_on_drop from word2 configuration*/
  uint32_t c2c_cfg_flag : 1; /*!< cope to cpu cfg
                              * 1-it may take c2c_cos/c2c_vld from word4
                              * configuration*/
  uint32_t mc_cfg_flag : 1;  /*!< multicast cfg
                              * 1-it may take mcid1/2_vld/pipe_vec
                              * from word4/3 configuration*/
  uint32_t
      epipe_cfg_flag : 1; /*!< 1-epipe_cfg dynamic from word2 configuration*/
  uint32_t mcast_l2_xid : 9;
  uint32_t mcast_l1_xid : 16;
  // word1
  uint16_t mcid1;
  uint16_t mcid2;
  // word2
  uint32_t epipe_port : 9;
  uint32_t rsv : 5;  // filling gap
  uint32_t epipe_port_v : 1;
  uint32_t deflect_on_drop : 1;
  uint32_t mcast_rid : 16;
  // word3
  uint32_t hash1 : 13;
  uint32_t hash2 : 13;
  uint32_t pipe_vec : 5; /*!, [3:0] pip_vector, [4] if pipe has 400G port*/
  uint32_t egr_bypass_flag : 1; /*!< 1-engress pipeline will be bypassed*/
  // word4
  uint32_t icos : 3;
#define TOF2_MIRROR_ICOS 7
  uint32_t color : 2;
  uint32_t mcid1_vld : 1;
  uint32_t mcid2_vld : 1;
  uint32_t c2c_cos : 3;
#define TOF2_MIRROR_ICOS_C2C 7
  uint32_t c2c_vld : 1;
  uint32_t yid_tbl_sel : 1;
  uint32_t eport_qid : 7;
#define TOF2_MIRROR_EGR_PORT_Q 127
} pipe_mgr_mirror_tof2_session_entry_t;

typedef struct pipe_mgr_mirror_tof_session_entry_ {
  // meta0
  uint32_t ingress_cos : 3; /*!< cos assigned to mirrored copy */
#define TOF_MIRROR_ICOS 7
  uint32_t ucast_egress_port_v : 1; /*!< mirror destination port - valid */
  uint32_t ucast_egress_port : 9;
  uint32_t egress_port_queue : 5; /*!< mirror destination port queue */
#define TOF_MIRROR_EGR_PORT_Q 31
  uint32_t packet_color : 2;       /*!< color used for mirrored copy */
  uint32_t pipe_mask : 4;          /*!< output pipes - used with mcast */
  uint32_t level1_mcast_hash : 13; /*!< outer hash value - to select path */
  // meta1
  uint32_t level2_mcast_hash : 13; /*!< inner hash value - to select path */
  uint32_t mcast_grp_a : 16;       /*!< multicast group id - outer */
  // meta2
  uint32_t mcast_grp_a_v : 1; /*!< multicast group id outer - valid */
  uint32_t mcast_grp_b : 16;  /*!< multicast group id - inner */
  uint32_t mcast_grp_b_v : 1; /*!< multicast group id inner - valid */
  uint32_t mcast_l1_xid : 16;
  // meta3
  uint32_t mcast_l2_xid : 9;
  uint32_t mcast_rid : 16;
  uint32_t engress_bypass : 1;
  uint32_t icos_for_copy_to_cpu : 3; /*!< cos for packet to cpu */
#define TOF_MIRROR_ICOS_C2C 7
  // meta4
  uint32_t copy_to_cpu : 1;     /*!< copy mirrored packet to cpu */
  uint32_t deflect_on_drop : 1; /*!< TM can defelct the packet incase of drop */
} pipe_mgr_mirror_tof_session_entry_t;

typedef struct pipe_mgr_mirror_session_info {
  bf_mirror_type_e mirror_type; /*!< Mirror type */
  bf_mirror_direction_e dir;    /*!< Mirror direction */
  // parameters that tofino needs per norm mirroring session
  uint16_t max_pkt_len; /*!< Maximum packet len of a mirrored copy */
  // coalescing session parameters
  // valid/used only if type is coalesing
  uint32_t
      header[4];         /*!< Tofino:Internal Header upto 16Bytes, added to
                          *   coalesced packet. This header is
                          *   removed/processed by egress parser
                          *   Two ms-bytes of header[0] are reserved
                          *   for internal use
                          *   Tofino2: compiler flag [7:0]
                          *   user_defined bytes [95:32], 8 bytes
                          *   sequence number [127:96], next seq numb for coal session
                          */
  uint32_t header_len;   /*!< Length of the internal header,
                          *   in terms of words
                          *   Tofino2: 8B/12B/16B
                          */
  uint32_t timeout_usec; /*!< N*BaseTime, after which coalesced packet
                          *  is schedule for transmittion
                          */
  uint32_t extract_len;  /*!< Number of bytes extracted from each packet sample
                          *   Tofino: including a sample_hdr
                          *   max 80B or 240B, see Tofino documentation
                          *   Tofino2: only include 4B chunk_hdr
                          *   max 1024B
                          */
  uint32_t ver;
  bool extract_len_from_p4; /*!< Extraction length provided source
                             * Tofino: P4 program, This overrides the default
                             * Tofino2: 0-MAU, 1-session cfg
                             */
  // only for tofino2
  bool coal_mode; /*!< coalescing mode, 1-tofino coalescing mode*/
  bool pri;       /*!< mirror pri, 0-high, 1-low*/
  union {
    // only for tofino
    pipe_mgr_mirror_tof_session_entry_t mirror_session_meta;
    // only for tofino2
    pipe_mgr_mirror_tof2_session_entry_t mirror_session_entry;
    // only for tofino3
    pipe_mgr_mirror_tof3_session_entry_t mirror_session_tof3_entry;
  } u;
  uint32_t
      old_ingr_sel_val[PIPE_MGR_MAX_PIPES]; /* Old mirr_sel ingress value. */
  uint32_t
      old_egr_sel_val[PIPE_MGR_MAX_PIPES]; /* Old mirr_sel ingress value. */
} pipe_mgr_mirror_session_info_t;

#endif  // _PIPE_MGR_MIRROR_BUFFER_COMM_H_
