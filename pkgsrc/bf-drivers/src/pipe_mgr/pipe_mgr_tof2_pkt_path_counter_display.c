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


#define __STDC_FORMAT_MACROS
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <bf_types/bf_types.h>
#include <lld/lld_reg_if.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include "pipe_mgr_tof2_pkt_path_counter_display.h"
typedef struct pipe_mgr_pktcnt_disp_s {
  uint32_t pktcnt;
  uint8_t cnt_numb;  // number of 32bits in the pktcnt.
  bf_packetpath_counter_type_en counter_type;
  char *pktcnt_name;
  char *pktcnt_desc;
} pipe_mgr_pktcnt_disp_t;

// _pgport1 = _pgport/8 _pgport2 = _pgport%8
#define PIPE_MGR_IPB_CNT_SUB(_pipe, _pgport, _pgport1, _pgport2)              \
  {                                                                           \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_deparser_drop_pkt     \
                    .chnl_deparser_drop_pkt_0_2),                             \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_deparser_drop_pkt",                                              \
       "IPB: Deparser drop count"},                                           \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_wsch_discard_pkt      \
                    .chnl_wsch_discard_pkt_0_2),                              \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_wsch_discard_pkt",                                               \
       "IPB: IPB internal drop count"},                                       \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_wsch_trunc_pkt        \
                    .chnl_wsch_trunc_pkt_0_2),                                \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                                \
       "chnl_wsch_trunc_pkt",                                                 \
       "IPB: IPB truncation count"},                                          \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_drop_trunc_pkt        \
                    .chnl_drop_trunc_pkt_0_2),                                \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                                \
       "chnl_drop_trunc_pkt",                                                 \
       "IPB: IPB drop truncation count"},                                     \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_resubmit_discard_pkt  \
                    .chnl_resubmit_discard_pkt_0_2),                          \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_resubmit_discard_pkt",                                           \
       "IPB: Deparser resubmit drop count"},                                  \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_parser_discard_pkt    \
                    .chnl_parser_discard_pkt_0_2),                            \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_parser_discard_pkt",                                             \
       "IPB: Parser drop packet count"},                                      \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]                  \
                    .ipbreg.chan##_pgport2##_group.chnl_resubmit_received_pkt \
                    .chnl_resubmit_received_pkt_0_2),                         \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_resubmit_received_pkt",                                          \
       "IPB: Resubmit packet count"},                                         \
  },
#define pipe_mgr_ipb_pktcnt_size 7  // ipb

// _pgport1 = _pgport/8 _pgport2 = _pgport%8
#define PIPE_MGR_IPB_SP_CNT_SUB(_pipe, _pgport, _pgport1, _pgport2)       \
  {                                                                       \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_parser_send_pkt   \
                    .chnl_parser_send_pkt_0_3),                           \
       3,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_parser_send_pkt",                                            \
       "IPB: parser send pkt counter"},                                   \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_parser_send_pkt   \
                    .chnl_parser_send_pkt_1_3),                           \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_parser_send_pkt",                                            \
       "IPB: err pkt counter"},                                           \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_deparser_send_pkt \
                    .chnl_deparser_send_pkt_0_3),                         \
       3,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_deparser_send_pkt",                                          \
       "IPB: deparser send pkt counter"},                                 \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_deparser_send_pkt \
                    .chnl_deparser_send_pkt_1_3),                         \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_deparser_send_pkt",                                          \
       "IPB: err pkt counter"},                                           \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_macs_received_pkt \
                    .chnl_macs_received_pkt_0_3),                         \
       3,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_macs_received_pkt",                                          \
       "IPB: MAC receive pkt counter"},                                   \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport1]              \
                    .ipbreg.chan##_pgport2##_group.chnl_macs_received_pkt \
                    .chnl_macs_received_pkt_1_3),                         \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_macs_received_pkt",                                          \
       "IPB: MAC receive err pkt counter"},                               \
  },

#define pipe_mgr_ipb_sp_pktcnt_size 6  // ipb special

#define PIPE_MGR_IPRSR_CNT_SUB(_pipe, _pgport)                         \
  {                                                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .hdr_byte_cnt[_pgport % 8 % 2]                     \
                    .hdr_byte_cnt_0_2),                                \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "hdr_byte_cnt",                                                 \
       "IPRSR: Header byte count"},                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_rx_cnt[_pgport % 8 % 2]                       \
                    .pkt_rx_cnt_0_2),                                  \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                         \
       "pkt_rx_cnt",                                                   \
       "IPRSR: Paclet RX count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_tx_cnt[_pgport % 8 % 2]                       \
                    .pkt_tx_cnt_0_2),                                  \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                         \
       "pkt_tx_cnt",                                                   \
       "IPRSR: Packet TX count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_drop_cnt[_pgport % 8 % 2]                     \
                    .pkt_drop_cnt_0_2),                                \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                          \
       "pkt_drop_cnt",                                                 \
       "IPRSR: Packet drop count"},                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2), \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "no_tcam_match_err_cnt",                                        \
       "IPRSR: No TCAM match error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .partial_hdr_err_cnt.partial_hdr_err_cnt_0_2),     \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                         \
       "partial_hdr_err_cnt",                                          \
       "IPRSR: Partial header error count"},                           \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .ctr_range_err_cnt.ctr_range_err_cnt_0_2),         \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "ctr_range_err_cnt",                                            \
       "IPRSR: Counter range error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),   \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "timeout_iter_err_cnt",                                         \
       "IPRSR: Timeout iteration error count"},                        \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2), \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "timeout_cycle_err_cnt",                                        \
       "IPRSR: Timeout cycle error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .src_ext_err_cnt.src_ext_err_cnt_0_2),             \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "src_ext_err_cnt",                                              \
       "IPRSR: Source extract error count"},                           \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .phv_owner_err_cnt.phv_owner_err_cnt_0_2),         \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "phv_owner_err_cnt",                                            \
       "IPRSR: PHV owner error ocunt"},                                \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .multi_wr_err_cnt.multi_wr_err_cnt_0_2),           \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "multi_wr_err_cnt",                                             \
       "IPRSR: Multi write error count"},                              \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .fcs_err_cnt.fcs_err_cnt_0_2),                     \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "fcs_err_cnt",                                                  \
       "IPRSR: FCS error count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.ipbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .csum_err_cnt.csum_err_cnt_0_2),                   \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "csum_err_cnt",                                                 \
       "IPRSR: Checksum error count"},                                 \
  },
#define pipe_mgr_iprsr_pktcnt_size 14  // iprsr pipe0-3, mac0-8, chnl0-7

#define PIPE_MGR_EPB_CNT_SUB(_pipe, _pgport, _pgport1, _pgport2)          \
  {                                                                       \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_parser_send_pkt   \
                    .chnl_parser_send_pkt_0_3),                           \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_parser_send_pkt",                                            \
       "EPB: wrap around counter"},                                       \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_parser_send_pkt   \
                    .chnl_parser_send_pkt_2_3),                           \
       1,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_parser_send_pkt",                                            \
       "EPB: saturating counter"},                                        \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_deparser_send_pkt \
                    .chnl_deparser_send_pkt_0_3),                         \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_deparser_send_pkt",                                          \
       "EPB: wrap around counter"},                                       \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_deparser_send_pkt \
                    .chnl_deparser_send_pkt_2_3),                         \
       1,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_deparser_send_pkt",                                          \
       "EPB: saturating counter"},                                        \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_warp_send_pkt     \
                    .chnl_warp_send_pkt_0_3),                             \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_warp_send_pkt",                                              \
       "EPB: WARP packet count"},                                         \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_warp_send_pkt     \
                    .chnl_warp_send_pkt_2_3),                             \
       1,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_warp_send_pkt",                                              \
       "EPB: WARP packet count"},                                         \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_p2s_received_pkt  \
                    .chnl_p2s_received_pkt_0_3),                          \
       2,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_p2s_received_pkt",                                           \
       "EPB: P2S packet count"},                                          \
      {offsetof(tof2_reg,                                                 \
                pipes[_pipe]                                              \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport1]              \
                    .epbreg.chan##_pgport2##_group.chnl_p2s_received_pkt  \
                    .chnl_p2s_received_pkt_2_3),                          \
       1,                                                                 \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                            \
       "chnl_p2s_received_pkt",                                           \
       "EPB: P2S packet count"},                                          \
  },
#define pipe_mgr_epb_pktcnt_size 8  // epb

#define PIPE_MGR_EPRSR_CNT_SUB(_pipe, _pgport)                         \
  {                                                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .hdr_byte_cnt[_pgport % 8 % 2]                     \
                    .hdr_byte_cnt_0_2),                                \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,                       \
       "hdr_byte_cnt",                                                 \
       "EPRSR: Header byte count"},                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_rx_cnt[_pgport % 8 % 2]                       \
                    .pkt_rx_cnt_0_2),                                  \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                         \
       "pkt_rx_cnt",                                                   \
       "EPRSR: Paclet RX count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_tx_cnt[_pgport % 8 % 2]                       \
                    .pkt_tx_cnt_0_2),                                  \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                         \
       "pkt_tx_cnt",                                                   \
       "EPRSR: Packet TX count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .pkt_drop_cnt[_pgport % 8 % 2]                     \
                    .pkt_drop_cnt_0_2),                                \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                          \
       "pkt_drop_cnt",                                                 \
       "EPRSR: Packet drop count"},                                    \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2), \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "no_tcam_match_err_cnt",                                        \
       "EPRSR: No TCAM match error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .partial_hdr_err_cnt.partial_hdr_err_cnt_0_2),     \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "partial_hdr_err_cnt",                                          \
       "EPRSR: Partial header error count"},                           \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .ctr_range_err_cnt.ctr_range_err_cnt_0_2),         \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "ctr_range_err_cnt",                                            \
       "EPRSR: Counter range error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),   \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "timeout_iter_err_cnt",                                         \
       "EPRSR: Timeout iteration error count"},                        \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2), \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "timeout_cycle_err_cnt",                                        \
       "EPRSR: Timeout cycle error count"},                            \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .src_ext_err_cnt.src_ext_err_cnt_0_2),             \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "src_ext_err_cnt",                                              \
       "EPRSR: Source extract error count"},                           \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .phv_owner_err_cnt.phv_owner_err_cnt_0_2),         \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "phv_owner_err_cnt",                                            \
       "EPRSR: PHV owner error ocunt"},                                \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .multi_wr_err_cnt.multi_wr_err_cnt_0_2),           \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "multi_wr_err_cnt",                                             \
       "EPRSR: Multi write error count"},                              \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .fcs_err_cnt.fcs_err_cnt_0_2),                     \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "fcs_err_cnt",                                                  \
       "EPRSR: FCS error count"},                                      \
      {offsetof(tof2_reg,                                              \
                pipes[_pipe]                                           \
                    .pardereg.pgstnreg.epbprsr4reg[_pgport / 8]        \
                    .prsr[_pgport % 8 / 2]                             \
                    .csum_err_cnt.csum_err_cnt_0_2),                   \
       2,                                                              \
       BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,                       \
       "csum_err_cnt",                                                 \
       "EPRSR: Checksum error count"},                                 \
  },
#define pipe_mgr_eprsr_pktcnt_size 14  // eprsr

#define pipe_mgr_pmarb_pktcnt_size 8
pipe_mgr_pktcnt_disp_t pipe_mgr_pmarb_pktcnt[4][pipe_mgr_pmarb_pktcnt_size] = {
    {
        {offsetof(
             tof2_reg,
             pipes[0]
                 .pardereg.pgstnreg.parbreg.left.i_phv_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[0]
                 .pardereg.pgstnreg.parbreg.left.i_eop_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_eop_count",
         "PMARB: Ingress EOP count"},
        {offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.parbreg.left.i_norm_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_phv_count",
         "PMARB: Ingress normal PHV count"},
        {offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.parbreg.left.i_norm_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_eop_count",
         "PMARB: Ingress normal EOP count"},
        {offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.parbreg.left.i_resub_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit PHV count"},
        {offsetof(tof2_reg,
                  pipes[0]
                      .pardereg.pgstnreg.parbreg.left.i_resub_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit EOP count"},
        {offsetof(
             tof2_reg,
             pipes[0]
                 .pardereg.pgstnreg.parbreg.right.e_phv_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_phv_count",
         "PMARB: Egress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[0]
                 .pardereg.pgstnreg.parbreg.right.e_eop_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_eop_count",
         "PMARB: Egress EOP count"},
    },
    {
        {offsetof(
             tof2_reg,
             pipes[1]
                 .pardereg.pgstnreg.parbreg.left.i_phv_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[1]
                 .pardereg.pgstnreg.parbreg.left.i_eop_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress EOP count"},
        {offsetof(tof2_reg,
                  pipes[1]
                      .pardereg.pgstnreg.parbreg.left.i_norm_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_phv_count",
         "PMARB: Ingress normal PHV count"},
        {offsetof(tof2_reg,
                  pipes[1]
                      .pardereg.pgstnreg.parbreg.left.i_norm_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_eop_count",
         "PMARB: Ingress normal EOP count"},
        {offsetof(tof2_reg,
                  pipes[1]
                      .pardereg.pgstnreg.parbreg.left.i_resub_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit PHV count"},
        {offsetof(tof2_reg,
                  pipes[1]
                      .pardereg.pgstnreg.parbreg.left.i_resub_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit EOP count"},
        {offsetof(
             tof2_reg,
             pipes[1]
                 .pardereg.pgstnreg.parbreg.right.e_phv_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_phv_count",
         "PMARB: Egress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[1]
                 .pardereg.pgstnreg.parbreg.right.e_eop_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_eop_count",
         "PMARB: Egress EOP count"},
    },
    {
        {offsetof(
             tof2_reg,
             pipes[2]
                 .pardereg.pgstnreg.parbreg.left.i_phv_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[2]
                 .pardereg.pgstnreg.parbreg.left.i_eop_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress EOP count"},
        {offsetof(tof2_reg,
                  pipes[2]
                      .pardereg.pgstnreg.parbreg.left.i_norm_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_phv_count",
         "PMARB: Ingress normal PHV count"},
        {offsetof(tof2_reg,
                  pipes[2]
                      .pardereg.pgstnreg.parbreg.left.i_norm_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_eop_count",
         "PMARB: Ingress normal EOP count"},
        {offsetof(tof2_reg,
                  pipes[2]
                      .pardereg.pgstnreg.parbreg.left.i_resub_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit PHV count"},
        {offsetof(tof2_reg,
                  pipes[2]
                      .pardereg.pgstnreg.parbreg.left.i_resub_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit EOP count"},
        {offsetof(
             tof2_reg,
             pipes[2]
                 .pardereg.pgstnreg.parbreg.right.e_phv_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_phv_count",
         "PMARB: Egress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[2]
                 .pardereg.pgstnreg.parbreg.right.e_eop_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_eop_count",
         "PMARB: Egress EOP count"},
    },
    {
        {offsetof(
             tof2_reg,
             pipes[3]
                 .pardereg.pgstnreg.parbreg.left.i_phv_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[3]
                 .pardereg.pgstnreg.parbreg.left.i_eop_count.i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_phv_count",
         "PMARB: Ingress EOP count"},
        {offsetof(tof2_reg,
                  pipes[3]
                      .pardereg.pgstnreg.parbreg.left.i_norm_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_phv_count",
         "PMARB: Ingress normal PHV count"},
        {offsetof(tof2_reg,
                  pipes[3]
                      .pardereg.pgstnreg.parbreg.left.i_norm_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_norm_eop_count",
         "PMARB: Ingress normal EOP count"},
        {offsetof(tof2_reg,
                  pipes[3]
                      .pardereg.pgstnreg.parbreg.left.i_resub_phv_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit PHV count"},
        {offsetof(tof2_reg,
                  pipes[3]
                      .pardereg.pgstnreg.parbreg.left.i_resub_eop_count
                      .i_phv_count_0_2),
         2,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "i_resub_phv_count",
         "PMARB: Ingress resubmit EOP count"},
        {offsetof(
             tof2_reg,
             pipes[3]
                 .pardereg.pgstnreg.parbreg.right.e_phv_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_phv_count",
         "PMARB: Egress PHV count"},
        {offsetof(
             tof2_reg,
             pipes[3]
                 .pardereg.pgstnreg.parbreg.right.e_eop_count.i_phv_count_0_2),
         1,
         BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
         "e_eop_count",
         "PMARB: Egress EOP count"},
    },
};  // pmarb pipe0-3

#define TOF2_IDPRSR_PERF_CNTR(slice, idx, chan)          \
  {offsetof(tof2_reg,                                    \
            pipes[0]                                     \
                .pardereg.dprsrreg.dprsrreg.ho_i[slice]  \
                .out_ingr.perf_pkt[idx]                  \
                .perf_pkt_0_2),                          \
   2,                                                    \
   BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,               \
   "ho_i[" #slice "].out_ingr.perf_pkt[" #idx "]",       \
   "IDeparser: Channel " chan " Pkts"},                  \
  {                                                      \
    offsetof(tof2_reg,                                   \
             pipes[0]                                    \
                 .pardereg.dprsrreg.dprsrreg.ho_i[slice] \
                 .out_ingr.perf_byt[idx]                 \
                 .perf_byt_0_2),                         \
        2, BF_PACKETPATH_COUNTER_TYPE_BYTECOUNT,         \
        "ho_i[" #slice "].out_ingr.perf_byt[" #idx "]",  \
        "IDeparser: Channel " chan " Bytes"              \
  }
pipe_mgr_pktcnt_disp_t pipe_mgr_idprsr_pktcnt[] = {
    {offsetof(tof2_reg,
              pipes[0]
                  .pardereg.dprsrreg.dprsrreg.inp.ipp.main_i.cnt_i_phv
                  .cnt_i_phv_0_2),
     2,
     BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
     "inp.ipp.main_i.cnt_i_phv",
     "IDeparser: PHV count"},
    {offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "inp.icr.cfg48[0]",
     "IDeparser: Drops"},
    {offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "inp.icr.cfg48[1]",
     "IDeparser: Resubmit"},
    {offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "inp.icr.cfg48[2]",
     "IDeparser: Forward"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[0].hir.h.cfg48[0]",
     "IDeparser: Pkt Channel 0-1, 8-23"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[1].hir.h.cfg48[0]",
     "IDeparser: Pkt Channel 2-3, 24-39"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[2].hir.h.cfg48[0]",
     "IDeparser: Pkt Channel 4-5, 40-55"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[3].hir.h.cfg48[0]",
     "IDeparser: Pkt Channel 6-7, 56-71"},

    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].hir.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_i[0].hir.h.cfg48[1]",
     "IDeparser: Drop Channel 0-1, 8-23"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].hir.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_i[1].hir.h.cfg48[1]",
     "IDeparser: Drop Channel 2-3, 24-39"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].hir.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_i[2].hir.h.cfg48[1]",
     "IDeparser: Drop Channel 4-5, 40-55"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].hir.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_i[3].hir.h.cfg48[1]",
     "IDeparser: Drop Channel 6-7, 56-71"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[0].out_ingr.cfg48[0]",
     "IDeparser: Channel 0-1, 8-23 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[1].out_ingr.cfg48[0]",
     "IDeparser: Channel 2-3, 24-39 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[2].out_ingr.cfg48[0]",
     "IDeparser: Channel 4-5, 40-55 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[3].out_ingr.cfg48[0]",
     "IDeparser: Channel 6-7, 56-71 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[0].out_ingr.cfg48[1]",
     "IDeparser: Channel 0-1 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[1].out_ingr.cfg48[1]",
     "IDeparser: Channel 2-3 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[2].out_ingr.cfg48[1]",
     "IDeparser: Channel 4-5 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[3].out_ingr.cfg48[1]",
     "IDeparser: Channel 6-7 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[0].out_ingr.cfg48[2]",
     "IDeparser: Channel 8-15 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0].out_ingr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[0].out_ingr.cfg48[3]",
     "IDeparser: Channel 16-23 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[1].out_ingr.cfg48[2]",
     "IDeparser: Channel 24-31 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1].out_ingr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[1].out_ingr.cfg48[3]",
     "IDeparser: Channel 32-39 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[2].out_ingr.cfg48[2]",
     "IDeparser: Channel 40-47 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[2].out_ingr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[2].out_ingr.cfg48[3]",
     "IDeparser: Channel 48-55 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[3].out_ingr.cfg48[2]",
     "IDeparser: Channel 56-63 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[3].out_ingr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_i[3].out_ingr.cfg48[3]",
     "IDeparser: Channel 64-71 EOP"},
    TOF2_IDPRSR_PERF_CNTR(0, 16, " 0"),
    TOF2_IDPRSR_PERF_CNTR(0, 17, " 1"),
    TOF2_IDPRSR_PERF_CNTR(1, 16, " 2"),
    TOF2_IDPRSR_PERF_CNTR(1, 17, " 3"),
    TOF2_IDPRSR_PERF_CNTR(2, 16, " 4"),
    TOF2_IDPRSR_PERF_CNTR(2, 17, " 5"),
    TOF2_IDPRSR_PERF_CNTR(3, 16, " 6"),
    TOF2_IDPRSR_PERF_CNTR(3, 17, " 7"),
    TOF2_IDPRSR_PERF_CNTR(0, 0, " 8"),
    TOF2_IDPRSR_PERF_CNTR(0, 1, " 9"),
    TOF2_IDPRSR_PERF_CNTR(0, 2, "10"),
    TOF2_IDPRSR_PERF_CNTR(0, 3, "11"),
    TOF2_IDPRSR_PERF_CNTR(0, 4, "12"),
    TOF2_IDPRSR_PERF_CNTR(0, 5, "13"),
    TOF2_IDPRSR_PERF_CNTR(0, 6, "14"),
    TOF2_IDPRSR_PERF_CNTR(0, 7, "15"),
    TOF2_IDPRSR_PERF_CNTR(0, 8, "16"),
    TOF2_IDPRSR_PERF_CNTR(0, 9, "17"),
    TOF2_IDPRSR_PERF_CNTR(0, 10, "18"),
    TOF2_IDPRSR_PERF_CNTR(0, 11, "19"),
    TOF2_IDPRSR_PERF_CNTR(0, 12, "20"),
    TOF2_IDPRSR_PERF_CNTR(0, 13, "21"),
    TOF2_IDPRSR_PERF_CNTR(0, 14, "22"),
    TOF2_IDPRSR_PERF_CNTR(0, 15, "23"),
    TOF2_IDPRSR_PERF_CNTR(1, 0, "24"),
    TOF2_IDPRSR_PERF_CNTR(1, 1, "25"),
    TOF2_IDPRSR_PERF_CNTR(1, 2, "26"),
    TOF2_IDPRSR_PERF_CNTR(1, 3, "27"),
    TOF2_IDPRSR_PERF_CNTR(1, 4, "28"),
    TOF2_IDPRSR_PERF_CNTR(1, 5, "29"),
    TOF2_IDPRSR_PERF_CNTR(1, 6, "30"),
    TOF2_IDPRSR_PERF_CNTR(1, 7, "31"),
    TOF2_IDPRSR_PERF_CNTR(1, 8, "32"),
    TOF2_IDPRSR_PERF_CNTR(1, 9, "33"),
    TOF2_IDPRSR_PERF_CNTR(1, 10, "34"),
    TOF2_IDPRSR_PERF_CNTR(1, 11, "35"),
    TOF2_IDPRSR_PERF_CNTR(1, 12, "36"),
    TOF2_IDPRSR_PERF_CNTR(1, 13, "37"),
    TOF2_IDPRSR_PERF_CNTR(1, 14, "38"),
    TOF2_IDPRSR_PERF_CNTR(1, 15, "39"),
    TOF2_IDPRSR_PERF_CNTR(2, 0, "40"),
    TOF2_IDPRSR_PERF_CNTR(2, 1, "41"),
    TOF2_IDPRSR_PERF_CNTR(2, 2, "42"),
    TOF2_IDPRSR_PERF_CNTR(2, 3, "43"),
    TOF2_IDPRSR_PERF_CNTR(2, 4, "44"),
    TOF2_IDPRSR_PERF_CNTR(2, 5, "45"),
    TOF2_IDPRSR_PERF_CNTR(2, 6, "46"),
    TOF2_IDPRSR_PERF_CNTR(2, 7, "47"),
    TOF2_IDPRSR_PERF_CNTR(2, 8, "48"),
    TOF2_IDPRSR_PERF_CNTR(2, 9, "49"),
    TOF2_IDPRSR_PERF_CNTR(2, 10, "50"),
    TOF2_IDPRSR_PERF_CNTR(2, 11, "51"),
    TOF2_IDPRSR_PERF_CNTR(2, 12, "52"),
    TOF2_IDPRSR_PERF_CNTR(2, 13, "53"),
    TOF2_IDPRSR_PERF_CNTR(2, 14, "54"),
    TOF2_IDPRSR_PERF_CNTR(2, 15, "55"),
    TOF2_IDPRSR_PERF_CNTR(3, 0, "56"),
    TOF2_IDPRSR_PERF_CNTR(3, 1, "57"),
    TOF2_IDPRSR_PERF_CNTR(3, 2, "58"),
    TOF2_IDPRSR_PERF_CNTR(3, 3, "59"),
    TOF2_IDPRSR_PERF_CNTR(3, 4, "60"),
    TOF2_IDPRSR_PERF_CNTR(3, 5, "61"),
    TOF2_IDPRSR_PERF_CNTR(3, 6, "62"),
    TOF2_IDPRSR_PERF_CNTR(3, 7, "63"),
    TOF2_IDPRSR_PERF_CNTR(3, 8, "64"),
    TOF2_IDPRSR_PERF_CNTR(3, 9, "65"),
    TOF2_IDPRSR_PERF_CNTR(3, 10, "66"),
    TOF2_IDPRSR_PERF_CNTR(3, 11, "67"),
    TOF2_IDPRSR_PERF_CNTR(3, 12, "68"),
    TOF2_IDPRSR_PERF_CNTR(3, 13, "69"),
    TOF2_IDPRSR_PERF_CNTR(3, 14, "70"),
    TOF2_IDPRSR_PERF_CNTR(3, 15, "71"),
};

#define TOF2_EDPRSR_PERF_CNTR(slice, idx, chan)          \
  {offsetof(tof2_reg,                                    \
            pipes[0]                                     \
                .pardereg.dprsrreg.dprsrreg.ho_e[slice]  \
                .out_egr.perf_pkt[idx]                   \
                .perf_pkt_0_2),                          \
   2,                                                    \
   BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,               \
   "ho_e[" #slice "].out_egr.perf_pkt[" #idx "]",        \
   "EDeparser: Channel " chan " Pkts"},                  \
  {                                                      \
    offsetof(tof2_reg,                                   \
             pipes[0]                                    \
                 .pardereg.dprsrreg.dprsrreg.ho_e[slice] \
                 .out_egr.perf_byt[idx]                  \
                 .perf_byt_0_2),                         \
        2, BF_PACKETPATH_COUNTER_TYPE_BYTECOUNT,         \
        "ho_e[" #slice "].out_egr.perf_byt[" #idx "]",   \
        "EDeparser: Channel " chan " Bytes"              \
  }
pipe_mgr_pktcnt_disp_t pipe_mgr_edprsr_pktcnt[] = {
    {offsetof(tof2_reg,
              pipes[0]
                  .pardereg.dprsrreg.dprsrreg.inp.ipp.main_e.cnt_i_phv
                  .cnt_i_phv_0_2),
     2,
     BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
     "cnt_i_phv",
     "EDeparser: Egress PHV count"},
    {offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.inp.icr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "inp.icr.cfg48[3]",
     "EDeparser: Egress Drops"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[0].her.h.cfg48[0]",
     "EDeparser: Pkt Channel 0-1, 8-23"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[1].her.h.cfg48[0]",
     "EDeparser: Pkt Channel 2-3, 24-39"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[2].her.h.cfg48[0]",
     "EDeparser: Pkt Channel 4-5, 40-55"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[3].her.h.cfg48[0]",
     "EDeparser: Pkt Channel 6-7, 56-71"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].her.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_e[0].her.h.cfg48[1]",
     "EDeparser: Drop Channel 0-1, 8-23"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].her.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_e[1].her.h.cfg48[1]",
     "EDeparser: Drop Channel 2-3, 24-39"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].her.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_e[2].her.h.cfg48[1]",
     "EDeparser: Drop Channel 4-5, 40-55"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].her.h.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
     "ho_e[3].her.h.cfg48[1]",
     "EDeparser: Drop Channel 6-7, 56-71"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[0].out_egr.cfg48[0]",
     "EDeparser: Channel 0-1, 8-23 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[1].out_egr.cfg48[0]",
     "EDeparser: Channel 2-3, 24-39 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[2].out_egr.cfg48[0]",
     "EDeparser: Channel 4-5, 40-55 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.cfg48[0]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[3].out_egr.cfg48[0]",
     "EDeparser: Channel 6-7, 56-71 Sent to Mirror"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[0].out_egr.cfg48[1]",
     "EDeparser: Channel 0-1 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[1].out_egr.cfg48[1]",
     "EDeparser: Channel 2-3 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[2].out_egr.cfg48[1]",
     "EDeparser: Channel 4-5 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.cfg48[1]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[3].out_egr.cfg48[1]",
     "EDeparser: Channel 6-7 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[0].out_egr.cfg48[2]",
     "EDeparser: Channel 8-15 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0].out_egr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[0].out_egr.cfg48[3]",
     "EDeparser: Channel 16-23 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[1].out_egr.cfg48[2]",
     "EDeparser: Channel 24-31 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1].out_egr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[1].out_egr.cfg48[3]",
     "EDeparser: Channel 32-39 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[2].out_egr.cfg48[2]",
     "EDeparser: Channel 40-47 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[2].out_egr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[2].out_egr.cfg48[3]",
     "EDeparser: Channel 48-55 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.cfg48[2]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[3].out_egr.cfg48[2]",
     "EDeparser: Channel 56-63 EOP"},
    {offsetof(tof2_reg,
              pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[3].out_egr.cfg48[3]),
     2,
     BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,
     "ho_e[3].out_egr.cfg48[3]",
     "EDeparser: Channel 64-71 EOP"},
    TOF2_EDPRSR_PERF_CNTR(0, 16, " 0"),
    TOF2_EDPRSR_PERF_CNTR(0, 17, " 1"),
    TOF2_EDPRSR_PERF_CNTR(1, 16, " 2"),
    TOF2_EDPRSR_PERF_CNTR(1, 17, " 3"),
    TOF2_EDPRSR_PERF_CNTR(2, 16, " 4"),
    TOF2_EDPRSR_PERF_CNTR(2, 17, " 5"),
    TOF2_EDPRSR_PERF_CNTR(3, 16, " 6"),
    TOF2_EDPRSR_PERF_CNTR(3, 17, " 7"),
    TOF2_EDPRSR_PERF_CNTR(0, 0, " 8"),
    TOF2_EDPRSR_PERF_CNTR(0, 1, " 9"),
    TOF2_EDPRSR_PERF_CNTR(0, 2, "10"),
    TOF2_EDPRSR_PERF_CNTR(0, 3, "11"),
    TOF2_EDPRSR_PERF_CNTR(0, 4, "12"),
    TOF2_EDPRSR_PERF_CNTR(0, 5, "13"),
    TOF2_EDPRSR_PERF_CNTR(0, 6, "14"),
    TOF2_EDPRSR_PERF_CNTR(0, 7, "15"),
    TOF2_EDPRSR_PERF_CNTR(0, 8, "16"),
    TOF2_EDPRSR_PERF_CNTR(0, 9, "17"),
    TOF2_EDPRSR_PERF_CNTR(0, 10, "18"),
    TOF2_EDPRSR_PERF_CNTR(0, 11, "19"),
    TOF2_EDPRSR_PERF_CNTR(0, 12, "20"),
    TOF2_EDPRSR_PERF_CNTR(0, 13, "21"),
    TOF2_EDPRSR_PERF_CNTR(0, 14, "22"),
    TOF2_EDPRSR_PERF_CNTR(0, 15, "23"),
    TOF2_EDPRSR_PERF_CNTR(1, 0, "24"),
    TOF2_EDPRSR_PERF_CNTR(1, 1, "25"),
    TOF2_EDPRSR_PERF_CNTR(1, 2, "26"),
    TOF2_EDPRSR_PERF_CNTR(1, 3, "27"),
    TOF2_EDPRSR_PERF_CNTR(1, 4, "28"),
    TOF2_EDPRSR_PERF_CNTR(1, 5, "29"),
    TOF2_EDPRSR_PERF_CNTR(1, 6, "30"),
    TOF2_EDPRSR_PERF_CNTR(1, 7, "31"),
    TOF2_EDPRSR_PERF_CNTR(1, 8, "32"),
    TOF2_EDPRSR_PERF_CNTR(1, 9, "33"),
    TOF2_EDPRSR_PERF_CNTR(1, 10, "34"),
    TOF2_EDPRSR_PERF_CNTR(1, 11, "35"),
    TOF2_EDPRSR_PERF_CNTR(1, 12, "36"),
    TOF2_EDPRSR_PERF_CNTR(1, 13, "37"),
    TOF2_EDPRSR_PERF_CNTR(1, 14, "38"),
    TOF2_EDPRSR_PERF_CNTR(1, 15, "39"),
    TOF2_EDPRSR_PERF_CNTR(2, 0, "40"),
    TOF2_EDPRSR_PERF_CNTR(2, 1, "41"),
    TOF2_EDPRSR_PERF_CNTR(2, 2, "42"),
    TOF2_EDPRSR_PERF_CNTR(2, 3, "43"),
    TOF2_EDPRSR_PERF_CNTR(2, 4, "44"),
    TOF2_EDPRSR_PERF_CNTR(2, 5, "45"),
    TOF2_EDPRSR_PERF_CNTR(2, 6, "46"),
    TOF2_EDPRSR_PERF_CNTR(2, 7, "47"),
    TOF2_EDPRSR_PERF_CNTR(2, 8, "48"),
    TOF2_EDPRSR_PERF_CNTR(2, 9, "49"),
    TOF2_EDPRSR_PERF_CNTR(2, 10, "50"),
    TOF2_EDPRSR_PERF_CNTR(2, 11, "51"),
    TOF2_EDPRSR_PERF_CNTR(2, 12, "52"),
    TOF2_EDPRSR_PERF_CNTR(2, 13, "53"),
    TOF2_EDPRSR_PERF_CNTR(2, 14, "54"),
    TOF2_EDPRSR_PERF_CNTR(2, 15, "55"),
    TOF2_EDPRSR_PERF_CNTR(3, 0, "56"),
    TOF2_EDPRSR_PERF_CNTR(3, 1, "57"),
    TOF2_EDPRSR_PERF_CNTR(3, 2, "58"),
    TOF2_EDPRSR_PERF_CNTR(3, 3, "59"),
    TOF2_EDPRSR_PERF_CNTR(3, 4, "60"),
    TOF2_EDPRSR_PERF_CNTR(3, 5, "61"),
    TOF2_EDPRSR_PERF_CNTR(3, 6, "62"),
    TOF2_EDPRSR_PERF_CNTR(3, 7, "63"),
    TOF2_EDPRSR_PERF_CNTR(3, 8, "64"),
    TOF2_EDPRSR_PERF_CNTR(3, 9, "65"),
    TOF2_EDPRSR_PERF_CNTR(3, 10, "66"),
    TOF2_EDPRSR_PERF_CNTR(3, 11, "67"),
    TOF2_EDPRSR_PERF_CNTR(3, 12, "68"),
    TOF2_EDPRSR_PERF_CNTR(3, 13, "69"),
    TOF2_EDPRSR_PERF_CNTR(3, 14, "70"),
    TOF2_EDPRSR_PERF_CNTR(3, 15, "71"),
};

#define PIPE_MGR_EBUF_CNT_SUB(_pipe, _pgport)                                 \
  {                                                                           \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_0_14),                           \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: dprsr rcv pkt"},                                                \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_2_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: dprsr err pkt"},                                                \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_3_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: dprsr runt pkt"},                                               \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_4_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: dprsr ct timeout pkt"},                                         \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_5_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_pktnum",                                                         \
       "Ebuf: dprsr drp pkt"},                                                \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_6_14),                           \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: warp rcv pkt"},                                                 \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_8_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: warp err pkt"},                                                 \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_9_14),                           \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: warp runt pkt"},                                                \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_10_14),                          \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                                 \
       "chnl_pktnum",                                                         \
       "Ebuf: warp drp pkt"},                                                 \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_11_14),                          \
       2,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: mac xmt pkt"},                                                  \
      {offsetof(tof2_reg,                                                     \
                pipes[_pipe]                                                  \
                    .pardereg.pgstnreg.ebuf900reg[(_pgport - 8) / 16]         \
                    .ebuf400reg[(_pgport - 8) / 8 - 2 * ((_pgport - 8) / 16)] \
                    .chan_group[_pgport % 8]                                  \
                    .chnl_pktnum.chnl_pktnum_13_14),                          \
       1,                                                                     \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                                \
       "chnl_pktnum",                                                         \
       "Ebuf: mac err pkt"},                                                  \
  },
// ebuf when pg_port8-71

#define PIPE_MGR_EBUF1_CNT_SUB(_pipe, _pgport)                 \
  {                                                            \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_0_14),            \
       2,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                 \
       "chnl_pktnum",                                          \
       "Ebuf: dprsr rcv pkt"},                                 \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_2_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                 \
       "chnl_pktnum",                                          \
       "Ebuf: dprsr err pkt"},                                 \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_3_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                 \
       "chnl_pktnum",                                          \
       "Ebuf: dprsr runt pkt"},                                \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_4_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                 \
       "chnl_pktnum",                                          \
       "Ebuf: dprsr ct timeout pkt"},                          \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_5_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                  \
       "chnl_pktnum",                                          \
       "Ebuf: dprsr drp pkt"},                                 \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_6_14),            \
       2,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                 \
       "chnl_pktnum",                                          \
       "Ebuf: warp rcv pkt"},                                  \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_8_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                 \
       "chnl_pktnum",                                          \
       "Ebuf: warp err pkt"},                                  \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_9_14),            \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                 \
       "chnl_pktnum",                                          \
       "Ebuf: warp runt pkt"},                                 \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_10_14),           \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,                  \
       "chnl_pktnum",                                          \
       "Ebuf: warp drp pkt"},                                  \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_11_14),           \
       2,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                 \
       "chnl_pktnum",                                          \
       "Ebuf: mac xmt pkt"},                                   \
      {offsetof(tof2_reg,                                      \
                pipes[_pipe]                                   \
                    .pardereg.pgstnreg.ebuf900reg[_pgport / 2] \
                    .ebuf100reg.chan_group[_pgport % 2]        \
                    .chnl_pktnum.chnl_pktnum_13_14),           \
       1,                                                      \
       BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,                 \
       "chnl_pktnum",                                          \
       "Ebuf: mac err pkt"},                                   \
  },
// ebuf when pg_port0-7
#define pipe_mgr_ebuf_pktcnt_size 11

#define PIPE_MGR_S2P_CNT_SUB(_pipe, _pgport)                             \
  {                                                                      \
      {offsetof(tof2_reg,                                                \
                pipes[_pipe].pardereg.pgstnreg.s2preg.pkt_ctr[_pgport]), \
       1,                                                                \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                           \
       "pkt_ctr",                                                        \
       "S2P: Packet counter"},                                           \
      {offsetof(tof2_reg,                                                \
                pipes[_pipe]                                             \
                    .pardereg.pgstnreg.s2preg.byte_ctr[_pgport]          \
                    .byte_ctr_0_2),                                      \
       2,                                                                \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                           \
       "byte_ctr",                                                       \
       "S2P: Byte counter"},                                             \
  },
#define pipe_mgr_s2p_pktcnt_size 2

#define PIPE_MGR_P2S_CNT_SUB(_pipe, _pgport)                             \
  {                                                                      \
      {offsetof(tof2_reg,                                                \
                pipes[_pipe].pardereg.pgstnreg.p2sreg.pkt_ctr[_pgport]), \
       1,                                                                \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                           \
       "pkt_ctr",                                                        \
       "P2S: Packet counter"},                                           \
      {offsetof(tof2_reg,                                                \
                pipes[_pipe]                                             \
                    .pardereg.pgstnreg.p2sreg.byte_ctr[_pgport]          \
                    .byte_ctr_0_2),                                      \
       2,                                                                \
       BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT,                           \
       "byte_ctr",                                                       \
       "P2S: Byte counter"},                                             \
  },
#define pipe_mgr_p2s_pktcnt_size 2

/* clang-format off */
#define DUP(_name)\
  {PIPE_MGR_##_name##_CNT_SUB(0,0)\
   PIPE_MGR_##_name##_CNT_SUB(0,1)\
   PIPE_MGR_##_name##_CNT_SUB(0,2)\
   PIPE_MGR_##_name##_CNT_SUB(0,3)\
   PIPE_MGR_##_name##_CNT_SUB(0,4)\
   PIPE_MGR_##_name##_CNT_SUB(0,5)\
   PIPE_MGR_##_name##_CNT_SUB(0,6)\
   PIPE_MGR_##_name##_CNT_SUB(0,7)\
   PIPE_MGR_##_name##_CNT_SUB(0,8)\
   PIPE_MGR_##_name##_CNT_SUB(0,9)\
   PIPE_MGR_##_name##_CNT_SUB(0,10)\
   PIPE_MGR_##_name##_CNT_SUB(0,11)\
   PIPE_MGR_##_name##_CNT_SUB(0,12)\
   PIPE_MGR_##_name##_CNT_SUB(0,13)\
   PIPE_MGR_##_name##_CNT_SUB(0,14)\
   PIPE_MGR_##_name##_CNT_SUB(0,15)\
   PIPE_MGR_##_name##_CNT_SUB(0,16)\
   PIPE_MGR_##_name##_CNT_SUB(0,17)\
   PIPE_MGR_##_name##_CNT_SUB(0,18)\
   PIPE_MGR_##_name##_CNT_SUB(0,19)\
   PIPE_MGR_##_name##_CNT_SUB(0,20)\
   PIPE_MGR_##_name##_CNT_SUB(0,21)\
   PIPE_MGR_##_name##_CNT_SUB(0,22)\
   PIPE_MGR_##_name##_CNT_SUB(0,23)\
   PIPE_MGR_##_name##_CNT_SUB(0,24)\
   PIPE_MGR_##_name##_CNT_SUB(0,25)\
   PIPE_MGR_##_name##_CNT_SUB(0,26)\
   PIPE_MGR_##_name##_CNT_SUB(0,27)\
   PIPE_MGR_##_name##_CNT_SUB(0,28)\
   PIPE_MGR_##_name##_CNT_SUB(0,29)\
   PIPE_MGR_##_name##_CNT_SUB(0,30)\
   PIPE_MGR_##_name##_CNT_SUB(0,31)\
   PIPE_MGR_##_name##_CNT_SUB(0,32)\
   PIPE_MGR_##_name##_CNT_SUB(0,33)\
   PIPE_MGR_##_name##_CNT_SUB(0,34)\
   PIPE_MGR_##_name##_CNT_SUB(0,35)\
   PIPE_MGR_##_name##_CNT_SUB(0,36)\
   PIPE_MGR_##_name##_CNT_SUB(0,37)\
   PIPE_MGR_##_name##_CNT_SUB(0,38)\
   PIPE_MGR_##_name##_CNT_SUB(0,39)\
   PIPE_MGR_##_name##_CNT_SUB(0,40)\
   PIPE_MGR_##_name##_CNT_SUB(0,41)\
   PIPE_MGR_##_name##_CNT_SUB(0,42)\
   PIPE_MGR_##_name##_CNT_SUB(0,43)\
   PIPE_MGR_##_name##_CNT_SUB(0,44)\
   PIPE_MGR_##_name##_CNT_SUB(0,45)\
   PIPE_MGR_##_name##_CNT_SUB(0,46)\
   PIPE_MGR_##_name##_CNT_SUB(0,47)\
   PIPE_MGR_##_name##_CNT_SUB(0,48)\
   PIPE_MGR_##_name##_CNT_SUB(0,49)\
   PIPE_MGR_##_name##_CNT_SUB(0,50)\
   PIPE_MGR_##_name##_CNT_SUB(0,51)\
   PIPE_MGR_##_name##_CNT_SUB(0,52)\
   PIPE_MGR_##_name##_CNT_SUB(0,53)\
   PIPE_MGR_##_name##_CNT_SUB(0,54)\
   PIPE_MGR_##_name##_CNT_SUB(0,55)\
   PIPE_MGR_##_name##_CNT_SUB(0,56)\
   PIPE_MGR_##_name##_CNT_SUB(0,57)\
   PIPE_MGR_##_name##_CNT_SUB(0,58)\
   PIPE_MGR_##_name##_CNT_SUB(0,59)\
   PIPE_MGR_##_name##_CNT_SUB(0,60)\
   PIPE_MGR_##_name##_CNT_SUB(0,61)\
   PIPE_MGR_##_name##_CNT_SUB(0,62)\
   PIPE_MGR_##_name##_CNT_SUB(0,63)\
   PIPE_MGR_##_name##_CNT_SUB(0,64)\
   PIPE_MGR_##_name##_CNT_SUB(0,65)\
   PIPE_MGR_##_name##_CNT_SUB(0,66)\
   PIPE_MGR_##_name##_CNT_SUB(0,67)\
   PIPE_MGR_##_name##_CNT_SUB(0,68)\
   PIPE_MGR_##_name##_CNT_SUB(0,69)\
   PIPE_MGR_##_name##_CNT_SUB(0,70)\
   PIPE_MGR_##_name##_CNT_SUB(0,71)},\
  {PIPE_MGR_##_name##_CNT_SUB(1,0)\
   PIPE_MGR_##_name##_CNT_SUB(1,1)\
   PIPE_MGR_##_name##_CNT_SUB(1,2)\
   PIPE_MGR_##_name##_CNT_SUB(1,3)\
   PIPE_MGR_##_name##_CNT_SUB(1,4)\
   PIPE_MGR_##_name##_CNT_SUB(1,5)\
   PIPE_MGR_##_name##_CNT_SUB(1,6)\
   PIPE_MGR_##_name##_CNT_SUB(1,7)\
   PIPE_MGR_##_name##_CNT_SUB(1,8)\
   PIPE_MGR_##_name##_CNT_SUB(1,9)\
   PIPE_MGR_##_name##_CNT_SUB(1,10)\
   PIPE_MGR_##_name##_CNT_SUB(1,11)\
   PIPE_MGR_##_name##_CNT_SUB(1,12)\
   PIPE_MGR_##_name##_CNT_SUB(1,13)\
   PIPE_MGR_##_name##_CNT_SUB(1,14)\
   PIPE_MGR_##_name##_CNT_SUB(1,15)\
   PIPE_MGR_##_name##_CNT_SUB(1,16)\
   PIPE_MGR_##_name##_CNT_SUB(1,17)\
   PIPE_MGR_##_name##_CNT_SUB(1,18)\
   PIPE_MGR_##_name##_CNT_SUB(1,19)\
   PIPE_MGR_##_name##_CNT_SUB(1,20)\
   PIPE_MGR_##_name##_CNT_SUB(1,21)\
   PIPE_MGR_##_name##_CNT_SUB(1,22)\
   PIPE_MGR_##_name##_CNT_SUB(1,23)\
   PIPE_MGR_##_name##_CNT_SUB(1,24)\
   PIPE_MGR_##_name##_CNT_SUB(1,25)\
   PIPE_MGR_##_name##_CNT_SUB(1,26)\
   PIPE_MGR_##_name##_CNT_SUB(1,27)\
   PIPE_MGR_##_name##_CNT_SUB(1,28)\
   PIPE_MGR_##_name##_CNT_SUB(1,29)\
   PIPE_MGR_##_name##_CNT_SUB(1,30)\
   PIPE_MGR_##_name##_CNT_SUB(1,31)\
   PIPE_MGR_##_name##_CNT_SUB(1,32)\
   PIPE_MGR_##_name##_CNT_SUB(1,33)\
   PIPE_MGR_##_name##_CNT_SUB(1,34)\
   PIPE_MGR_##_name##_CNT_SUB(1,35)\
   PIPE_MGR_##_name##_CNT_SUB(1,36)\
   PIPE_MGR_##_name##_CNT_SUB(1,37)\
   PIPE_MGR_##_name##_CNT_SUB(1,38)\
   PIPE_MGR_##_name##_CNT_SUB(1,39)\
   PIPE_MGR_##_name##_CNT_SUB(1,40)\
   PIPE_MGR_##_name##_CNT_SUB(1,41)\
   PIPE_MGR_##_name##_CNT_SUB(1,42)\
   PIPE_MGR_##_name##_CNT_SUB(1,43)\
   PIPE_MGR_##_name##_CNT_SUB(1,44)\
   PIPE_MGR_##_name##_CNT_SUB(1,45)\
   PIPE_MGR_##_name##_CNT_SUB(1,46)\
   PIPE_MGR_##_name##_CNT_SUB(1,47)\
   PIPE_MGR_##_name##_CNT_SUB(1,48)\
   PIPE_MGR_##_name##_CNT_SUB(1,49)\
   PIPE_MGR_##_name##_CNT_SUB(1,50)\
   PIPE_MGR_##_name##_CNT_SUB(1,51)\
   PIPE_MGR_##_name##_CNT_SUB(1,52)\
   PIPE_MGR_##_name##_CNT_SUB(1,53)\
   PIPE_MGR_##_name##_CNT_SUB(1,54)\
   PIPE_MGR_##_name##_CNT_SUB(1,55)\
   PIPE_MGR_##_name##_CNT_SUB(1,56)\
   PIPE_MGR_##_name##_CNT_SUB(1,57)\
   PIPE_MGR_##_name##_CNT_SUB(1,58)\
   PIPE_MGR_##_name##_CNT_SUB(1,59)\
   PIPE_MGR_##_name##_CNT_SUB(1,60)\
   PIPE_MGR_##_name##_CNT_SUB(1,61)\
   PIPE_MGR_##_name##_CNT_SUB(1,62)\
   PIPE_MGR_##_name##_CNT_SUB(1,63)\
   PIPE_MGR_##_name##_CNT_SUB(1,64)\
   PIPE_MGR_##_name##_CNT_SUB(1,65)\
   PIPE_MGR_##_name##_CNT_SUB(1,66)\
   PIPE_MGR_##_name##_CNT_SUB(1,67)\
   PIPE_MGR_##_name##_CNT_SUB(1,68)\
   PIPE_MGR_##_name##_CNT_SUB(1,69)\
   PIPE_MGR_##_name##_CNT_SUB(1,70)\
   PIPE_MGR_##_name##_CNT_SUB(1,71)},\
  {PIPE_MGR_##_name##_CNT_SUB(2,0)\
   PIPE_MGR_##_name##_CNT_SUB(2,1)\
   PIPE_MGR_##_name##_CNT_SUB(2,2)\
   PIPE_MGR_##_name##_CNT_SUB(2,3)\
   PIPE_MGR_##_name##_CNT_SUB(2,4)\
   PIPE_MGR_##_name##_CNT_SUB(2,5)\
   PIPE_MGR_##_name##_CNT_SUB(2,6)\
   PIPE_MGR_##_name##_CNT_SUB(2,7)\
   PIPE_MGR_##_name##_CNT_SUB(2,8)\
   PIPE_MGR_##_name##_CNT_SUB(2,9)\
   PIPE_MGR_##_name##_CNT_SUB(2,10)\
   PIPE_MGR_##_name##_CNT_SUB(2,11)\
   PIPE_MGR_##_name##_CNT_SUB(2,12)\
   PIPE_MGR_##_name##_CNT_SUB(2,13)\
   PIPE_MGR_##_name##_CNT_SUB(2,14)\
   PIPE_MGR_##_name##_CNT_SUB(2,15)\
   PIPE_MGR_##_name##_CNT_SUB(2,16)\
   PIPE_MGR_##_name##_CNT_SUB(2,17)\
   PIPE_MGR_##_name##_CNT_SUB(2,18)\
   PIPE_MGR_##_name##_CNT_SUB(2,19)\
   PIPE_MGR_##_name##_CNT_SUB(2,20)\
   PIPE_MGR_##_name##_CNT_SUB(2,21)\
   PIPE_MGR_##_name##_CNT_SUB(2,22)\
   PIPE_MGR_##_name##_CNT_SUB(2,23)\
   PIPE_MGR_##_name##_CNT_SUB(2,24)\
   PIPE_MGR_##_name##_CNT_SUB(2,25)\
   PIPE_MGR_##_name##_CNT_SUB(2,26)\
   PIPE_MGR_##_name##_CNT_SUB(2,27)\
   PIPE_MGR_##_name##_CNT_SUB(2,28)\
   PIPE_MGR_##_name##_CNT_SUB(2,29)\
   PIPE_MGR_##_name##_CNT_SUB(2,30)\
   PIPE_MGR_##_name##_CNT_SUB(2,31)\
   PIPE_MGR_##_name##_CNT_SUB(2,32)\
   PIPE_MGR_##_name##_CNT_SUB(2,33)\
   PIPE_MGR_##_name##_CNT_SUB(2,34)\
   PIPE_MGR_##_name##_CNT_SUB(2,35)\
   PIPE_MGR_##_name##_CNT_SUB(2,36)\
   PIPE_MGR_##_name##_CNT_SUB(2,37)\
   PIPE_MGR_##_name##_CNT_SUB(2,38)\
   PIPE_MGR_##_name##_CNT_SUB(2,39)\
   PIPE_MGR_##_name##_CNT_SUB(2,40)\
   PIPE_MGR_##_name##_CNT_SUB(2,41)\
   PIPE_MGR_##_name##_CNT_SUB(2,42)\
   PIPE_MGR_##_name##_CNT_SUB(2,43)\
   PIPE_MGR_##_name##_CNT_SUB(2,44)\
   PIPE_MGR_##_name##_CNT_SUB(2,45)\
   PIPE_MGR_##_name##_CNT_SUB(2,46)\
   PIPE_MGR_##_name##_CNT_SUB(2,47)\
   PIPE_MGR_##_name##_CNT_SUB(2,48)\
   PIPE_MGR_##_name##_CNT_SUB(2,49)\
   PIPE_MGR_##_name##_CNT_SUB(2,50)\
   PIPE_MGR_##_name##_CNT_SUB(2,51)\
   PIPE_MGR_##_name##_CNT_SUB(2,52)\
   PIPE_MGR_##_name##_CNT_SUB(2,53)\
   PIPE_MGR_##_name##_CNT_SUB(2,54)\
   PIPE_MGR_##_name##_CNT_SUB(2,55)\
   PIPE_MGR_##_name##_CNT_SUB(2,56)\
   PIPE_MGR_##_name##_CNT_SUB(2,57)\
   PIPE_MGR_##_name##_CNT_SUB(2,58)\
   PIPE_MGR_##_name##_CNT_SUB(2,59)\
   PIPE_MGR_##_name##_CNT_SUB(2,60)\
   PIPE_MGR_##_name##_CNT_SUB(2,61)\
   PIPE_MGR_##_name##_CNT_SUB(2,62)\
   PIPE_MGR_##_name##_CNT_SUB(2,63)\
   PIPE_MGR_##_name##_CNT_SUB(2,64)\
   PIPE_MGR_##_name##_CNT_SUB(2,65)\
   PIPE_MGR_##_name##_CNT_SUB(2,66)\
   PIPE_MGR_##_name##_CNT_SUB(2,67)\
   PIPE_MGR_##_name##_CNT_SUB(2,68)\
   PIPE_MGR_##_name##_CNT_SUB(2,69)\
   PIPE_MGR_##_name##_CNT_SUB(2,70)\
   PIPE_MGR_##_name##_CNT_SUB(2,71)},\
  {PIPE_MGR_##_name##_CNT_SUB(3,0)\
   PIPE_MGR_##_name##_CNT_SUB(3,1)\
   PIPE_MGR_##_name##_CNT_SUB(3,2)\
   PIPE_MGR_##_name##_CNT_SUB(3,3)\
   PIPE_MGR_##_name##_CNT_SUB(3,4)\
   PIPE_MGR_##_name##_CNT_SUB(3,5)\
   PIPE_MGR_##_name##_CNT_SUB(3,6)\
   PIPE_MGR_##_name##_CNT_SUB(3,7)\
   PIPE_MGR_##_name##_CNT_SUB(3,8)\
   PIPE_MGR_##_name##_CNT_SUB(3,9)\
   PIPE_MGR_##_name##_CNT_SUB(3,10)\
   PIPE_MGR_##_name##_CNT_SUB(3,11)\
   PIPE_MGR_##_name##_CNT_SUB(3,12)\
   PIPE_MGR_##_name##_CNT_SUB(3,13)\
   PIPE_MGR_##_name##_CNT_SUB(3,14)\
   PIPE_MGR_##_name##_CNT_SUB(3,15)\
   PIPE_MGR_##_name##_CNT_SUB(3,16)\
   PIPE_MGR_##_name##_CNT_SUB(3,17)\
   PIPE_MGR_##_name##_CNT_SUB(3,18)\
   PIPE_MGR_##_name##_CNT_SUB(3,19)\
   PIPE_MGR_##_name##_CNT_SUB(3,20)\
   PIPE_MGR_##_name##_CNT_SUB(3,21)\
   PIPE_MGR_##_name##_CNT_SUB(3,22)\
   PIPE_MGR_##_name##_CNT_SUB(3,23)\
   PIPE_MGR_##_name##_CNT_SUB(3,24)\
   PIPE_MGR_##_name##_CNT_SUB(3,25)\
   PIPE_MGR_##_name##_CNT_SUB(3,26)\
   PIPE_MGR_##_name##_CNT_SUB(3,27)\
   PIPE_MGR_##_name##_CNT_SUB(3,28)\
   PIPE_MGR_##_name##_CNT_SUB(3,29)\
   PIPE_MGR_##_name##_CNT_SUB(3,30)\
   PIPE_MGR_##_name##_CNT_SUB(3,31)\
   PIPE_MGR_##_name##_CNT_SUB(3,32)\
   PIPE_MGR_##_name##_CNT_SUB(3,33)\
   PIPE_MGR_##_name##_CNT_SUB(3,34)\
   PIPE_MGR_##_name##_CNT_SUB(3,35)\
   PIPE_MGR_##_name##_CNT_SUB(3,36)\
   PIPE_MGR_##_name##_CNT_SUB(3,37)\
   PIPE_MGR_##_name##_CNT_SUB(3,38)\
   PIPE_MGR_##_name##_CNT_SUB(3,39)\
   PIPE_MGR_##_name##_CNT_SUB(3,40)\
   PIPE_MGR_##_name##_CNT_SUB(3,41)\
   PIPE_MGR_##_name##_CNT_SUB(3,42)\
   PIPE_MGR_##_name##_CNT_SUB(3,43)\
   PIPE_MGR_##_name##_CNT_SUB(3,44)\
   PIPE_MGR_##_name##_CNT_SUB(3,45)\
   PIPE_MGR_##_name##_CNT_SUB(3,46)\
   PIPE_MGR_##_name##_CNT_SUB(3,47)\
   PIPE_MGR_##_name##_CNT_SUB(3,48)\
   PIPE_MGR_##_name##_CNT_SUB(3,49)\
   PIPE_MGR_##_name##_CNT_SUB(3,50)\
   PIPE_MGR_##_name##_CNT_SUB(3,51)\
   PIPE_MGR_##_name##_CNT_SUB(3,52)\
   PIPE_MGR_##_name##_CNT_SUB(3,53)\
   PIPE_MGR_##_name##_CNT_SUB(3,54)\
   PIPE_MGR_##_name##_CNT_SUB(3,55)\
   PIPE_MGR_##_name##_CNT_SUB(3,56)\
   PIPE_MGR_##_name##_CNT_SUB(3,57)\
   PIPE_MGR_##_name##_CNT_SUB(3,58)\
   PIPE_MGR_##_name##_CNT_SUB(3,59)\
   PIPE_MGR_##_name##_CNT_SUB(3,60)\
   PIPE_MGR_##_name##_CNT_SUB(3,61)\
   PIPE_MGR_##_name##_CNT_SUB(3,62)\
   PIPE_MGR_##_name##_CNT_SUB(3,63)\
   PIPE_MGR_##_name##_CNT_SUB(3,64)\
   PIPE_MGR_##_name##_CNT_SUB(3,65)\
   PIPE_MGR_##_name##_CNT_SUB(3,66)\
   PIPE_MGR_##_name##_CNT_SUB(3,67)\
   PIPE_MGR_##_name##_CNT_SUB(3,68)\
   PIPE_MGR_##_name##_CNT_SUB(3,69)\
   PIPE_MGR_##_name##_CNT_SUB(3,70)\
   PIPE_MGR_##_name##_CNT_SUB(3,71)},
/* clang-format on */
#define PIPE_MGR_P2S_CNT                                                   \
  pipe_mgr_pktcnt_disp_t pipe_mgr_p2s_pktcnt[4][72]                        \
                                            [pipe_mgr_p2s_pktcnt_size] = { \
                                                DUP(P2S)};  // p2s

#define PIPE_MGR_S2P_CNT                                                   \
  pipe_mgr_pktcnt_disp_t pipe_mgr_s2p_pktcnt[4][72]                        \
                                            [pipe_mgr_s2p_pktcnt_size] = { \
                                                DUP(S2P)};  // s2p

#define PIPE_MGR_IPRSR_CNT                                                     \
  pipe_mgr_pktcnt_disp_t pipe_mgr_iprsr_pktcnt[4][72]                          \
                                              [pipe_mgr_iprsr_pktcnt_size] = { \
                                                  DUP(IPRSR)};  // iprsr

#define PIPE_MGR_EPRSR_CNT                                                     \
  pipe_mgr_pktcnt_disp_t pipe_mgr_eprsr_pktcnt[4][72]                          \
                                              [pipe_mgr_eprsr_pktcnt_size] = { \
                                                  DUP(EPRSR)};  // eprsr

/* clang-format off */
#define DUP_EBUF(_name)\
  {PIPE_MGR_##_name##1_CNT_SUB(0,0)\
   PIPE_MGR_##_name##1_CNT_SUB(0,1)\
   PIPE_MGR_##_name##1_CNT_SUB(0,2)\
   PIPE_MGR_##_name##1_CNT_SUB(0,3)\
   PIPE_MGR_##_name##1_CNT_SUB(0,4)\
   PIPE_MGR_##_name##1_CNT_SUB(0,5)\
   PIPE_MGR_##_name##1_CNT_SUB(0,6)\
   PIPE_MGR_##_name##1_CNT_SUB(0,7)\
   PIPE_MGR_##_name##_CNT_SUB(0,8)\
   PIPE_MGR_##_name##_CNT_SUB(0,9)\
   PIPE_MGR_##_name##_CNT_SUB(0,10)\
   PIPE_MGR_##_name##_CNT_SUB(0,11)\
   PIPE_MGR_##_name##_CNT_SUB(0,12)\
   PIPE_MGR_##_name##_CNT_SUB(0,13)\
   PIPE_MGR_##_name##_CNT_SUB(0,14)\
   PIPE_MGR_##_name##_CNT_SUB(0,15)\
   PIPE_MGR_##_name##_CNT_SUB(0,16)\
   PIPE_MGR_##_name##_CNT_SUB(0,17)\
   PIPE_MGR_##_name##_CNT_SUB(0,18)\
   PIPE_MGR_##_name##_CNT_SUB(0,19)\
   PIPE_MGR_##_name##_CNT_SUB(0,20)\
   PIPE_MGR_##_name##_CNT_SUB(0,21)\
   PIPE_MGR_##_name##_CNT_SUB(0,22)\
   PIPE_MGR_##_name##_CNT_SUB(0,23)\
   PIPE_MGR_##_name##_CNT_SUB(0,24)\
   PIPE_MGR_##_name##_CNT_SUB(0,25)\
   PIPE_MGR_##_name##_CNT_SUB(0,26)\
   PIPE_MGR_##_name##_CNT_SUB(0,27)\
   PIPE_MGR_##_name##_CNT_SUB(0,28)\
   PIPE_MGR_##_name##_CNT_SUB(0,29)\
   PIPE_MGR_##_name##_CNT_SUB(0,30)\
   PIPE_MGR_##_name##_CNT_SUB(0,31)\
   PIPE_MGR_##_name##_CNT_SUB(0,32)\
   PIPE_MGR_##_name##_CNT_SUB(0,33)\
   PIPE_MGR_##_name##_CNT_SUB(0,34)\
   PIPE_MGR_##_name##_CNT_SUB(0,35)\
   PIPE_MGR_##_name##_CNT_SUB(0,36)\
   PIPE_MGR_##_name##_CNT_SUB(0,37)\
   PIPE_MGR_##_name##_CNT_SUB(0,38)\
   PIPE_MGR_##_name##_CNT_SUB(0,39)\
   PIPE_MGR_##_name##_CNT_SUB(0,40)\
   PIPE_MGR_##_name##_CNT_SUB(0,41)\
   PIPE_MGR_##_name##_CNT_SUB(0,42)\
   PIPE_MGR_##_name##_CNT_SUB(0,43)\
   PIPE_MGR_##_name##_CNT_SUB(0,44)\
   PIPE_MGR_##_name##_CNT_SUB(0,45)\
   PIPE_MGR_##_name##_CNT_SUB(0,46)\
   PIPE_MGR_##_name##_CNT_SUB(0,47)\
   PIPE_MGR_##_name##_CNT_SUB(0,48)\
   PIPE_MGR_##_name##_CNT_SUB(0,49)\
   PIPE_MGR_##_name##_CNT_SUB(0,50)\
   PIPE_MGR_##_name##_CNT_SUB(0,51)\
   PIPE_MGR_##_name##_CNT_SUB(0,52)\
   PIPE_MGR_##_name##_CNT_SUB(0,53)\
   PIPE_MGR_##_name##_CNT_SUB(0,54)\
   PIPE_MGR_##_name##_CNT_SUB(0,55)\
   PIPE_MGR_##_name##_CNT_SUB(0,56)\
   PIPE_MGR_##_name##_CNT_SUB(0,57)\
   PIPE_MGR_##_name##_CNT_SUB(0,58)\
   PIPE_MGR_##_name##_CNT_SUB(0,59)\
   PIPE_MGR_##_name##_CNT_SUB(0,60)\
   PIPE_MGR_##_name##_CNT_SUB(0,61)\
   PIPE_MGR_##_name##_CNT_SUB(0,62)\
   PIPE_MGR_##_name##_CNT_SUB(0,63)\
   PIPE_MGR_##_name##_CNT_SUB(0,64)\
   PIPE_MGR_##_name##_CNT_SUB(0,65)\
   PIPE_MGR_##_name##_CNT_SUB(0,66)\
   PIPE_MGR_##_name##_CNT_SUB(0,67)\
   PIPE_MGR_##_name##_CNT_SUB(0,68)\
   PIPE_MGR_##_name##_CNT_SUB(0,69)\
   PIPE_MGR_##_name##_CNT_SUB(0,70)\
   PIPE_MGR_##_name##_CNT_SUB(0,71)},\
  {PIPE_MGR_##_name##1_CNT_SUB(1,0)\
   PIPE_MGR_##_name##1_CNT_SUB(1,1)\
   PIPE_MGR_##_name##1_CNT_SUB(1,2)\
   PIPE_MGR_##_name##1_CNT_SUB(1,3)\
   PIPE_MGR_##_name##1_CNT_SUB(1,4)\
   PIPE_MGR_##_name##1_CNT_SUB(1,5)\
   PIPE_MGR_##_name##1_CNT_SUB(1,6)\
   PIPE_MGR_##_name##1_CNT_SUB(1,7)\
   PIPE_MGR_##_name##_CNT_SUB(1,8)\
   PIPE_MGR_##_name##_CNT_SUB(1,9)\
   PIPE_MGR_##_name##_CNT_SUB(1,10)\
   PIPE_MGR_##_name##_CNT_SUB(1,11)\
   PIPE_MGR_##_name##_CNT_SUB(1,12)\
   PIPE_MGR_##_name##_CNT_SUB(1,13)\
   PIPE_MGR_##_name##_CNT_SUB(1,14)\
   PIPE_MGR_##_name##_CNT_SUB(1,15)\
   PIPE_MGR_##_name##_CNT_SUB(1,16)\
   PIPE_MGR_##_name##_CNT_SUB(1,17)\
   PIPE_MGR_##_name##_CNT_SUB(1,18)\
   PIPE_MGR_##_name##_CNT_SUB(1,19)\
   PIPE_MGR_##_name##_CNT_SUB(1,20)\
   PIPE_MGR_##_name##_CNT_SUB(1,21)\
   PIPE_MGR_##_name##_CNT_SUB(1,22)\
   PIPE_MGR_##_name##_CNT_SUB(1,23)\
   PIPE_MGR_##_name##_CNT_SUB(1,24)\
   PIPE_MGR_##_name##_CNT_SUB(1,25)\
   PIPE_MGR_##_name##_CNT_SUB(1,26)\
   PIPE_MGR_##_name##_CNT_SUB(1,27)\
   PIPE_MGR_##_name##_CNT_SUB(1,28)\
   PIPE_MGR_##_name##_CNT_SUB(1,29)\
   PIPE_MGR_##_name##_CNT_SUB(1,30)\
   PIPE_MGR_##_name##_CNT_SUB(1,31)\
   PIPE_MGR_##_name##_CNT_SUB(1,32)\
   PIPE_MGR_##_name##_CNT_SUB(1,33)\
   PIPE_MGR_##_name##_CNT_SUB(1,34)\
   PIPE_MGR_##_name##_CNT_SUB(1,35)\
   PIPE_MGR_##_name##_CNT_SUB(1,36)\
   PIPE_MGR_##_name##_CNT_SUB(1,37)\
   PIPE_MGR_##_name##_CNT_SUB(1,38)\
   PIPE_MGR_##_name##_CNT_SUB(1,39)\
   PIPE_MGR_##_name##_CNT_SUB(1,40)\
   PIPE_MGR_##_name##_CNT_SUB(1,41)\
   PIPE_MGR_##_name##_CNT_SUB(1,42)\
   PIPE_MGR_##_name##_CNT_SUB(1,43)\
   PIPE_MGR_##_name##_CNT_SUB(1,44)\
   PIPE_MGR_##_name##_CNT_SUB(1,45)\
   PIPE_MGR_##_name##_CNT_SUB(1,46)\
   PIPE_MGR_##_name##_CNT_SUB(1,47)\
   PIPE_MGR_##_name##_CNT_SUB(1,48)\
   PIPE_MGR_##_name##_CNT_SUB(1,49)\
   PIPE_MGR_##_name##_CNT_SUB(1,50)\
   PIPE_MGR_##_name##_CNT_SUB(1,51)\
   PIPE_MGR_##_name##_CNT_SUB(1,52)\
   PIPE_MGR_##_name##_CNT_SUB(1,53)\
   PIPE_MGR_##_name##_CNT_SUB(1,54)\
   PIPE_MGR_##_name##_CNT_SUB(1,55)\
   PIPE_MGR_##_name##_CNT_SUB(1,56)\
   PIPE_MGR_##_name##_CNT_SUB(1,57)\
   PIPE_MGR_##_name##_CNT_SUB(1,58)\
   PIPE_MGR_##_name##_CNT_SUB(1,59)\
   PIPE_MGR_##_name##_CNT_SUB(1,60)\
   PIPE_MGR_##_name##_CNT_SUB(1,61)\
   PIPE_MGR_##_name##_CNT_SUB(1,62)\
   PIPE_MGR_##_name##_CNT_SUB(1,63)\
   PIPE_MGR_##_name##_CNT_SUB(1,64)\
   PIPE_MGR_##_name##_CNT_SUB(1,65)\
   PIPE_MGR_##_name##_CNT_SUB(1,66)\
   PIPE_MGR_##_name##_CNT_SUB(1,67)\
   PIPE_MGR_##_name##_CNT_SUB(1,68)\
   PIPE_MGR_##_name##_CNT_SUB(1,69)\
   PIPE_MGR_##_name##_CNT_SUB(1,70)\
   PIPE_MGR_##_name##_CNT_SUB(1,71)},\
  {PIPE_MGR_##_name##1_CNT_SUB(2,0)\
   PIPE_MGR_##_name##1_CNT_SUB(2,1)\
   PIPE_MGR_##_name##1_CNT_SUB(2,2)\
   PIPE_MGR_##_name##1_CNT_SUB(2,3)\
   PIPE_MGR_##_name##1_CNT_SUB(2,4)\
   PIPE_MGR_##_name##1_CNT_SUB(2,5)\
   PIPE_MGR_##_name##1_CNT_SUB(2,6)\
   PIPE_MGR_##_name##1_CNT_SUB(2,7)\
   PIPE_MGR_##_name##_CNT_SUB(2,8)\
   PIPE_MGR_##_name##_CNT_SUB(2,9)\
   PIPE_MGR_##_name##_CNT_SUB(2,10)\
   PIPE_MGR_##_name##_CNT_SUB(2,11)\
   PIPE_MGR_##_name##_CNT_SUB(2,12)\
   PIPE_MGR_##_name##_CNT_SUB(2,13)\
   PIPE_MGR_##_name##_CNT_SUB(2,14)\
   PIPE_MGR_##_name##_CNT_SUB(2,15)\
   PIPE_MGR_##_name##_CNT_SUB(2,16)\
   PIPE_MGR_##_name##_CNT_SUB(2,17)\
   PIPE_MGR_##_name##_CNT_SUB(2,18)\
   PIPE_MGR_##_name##_CNT_SUB(2,19)\
   PIPE_MGR_##_name##_CNT_SUB(2,20)\
   PIPE_MGR_##_name##_CNT_SUB(2,21)\
   PIPE_MGR_##_name##_CNT_SUB(2,22)\
   PIPE_MGR_##_name##_CNT_SUB(2,23)\
   PIPE_MGR_##_name##_CNT_SUB(2,24)\
   PIPE_MGR_##_name##_CNT_SUB(2,25)\
   PIPE_MGR_##_name##_CNT_SUB(2,26)\
   PIPE_MGR_##_name##_CNT_SUB(2,27)\
   PIPE_MGR_##_name##_CNT_SUB(2,28)\
   PIPE_MGR_##_name##_CNT_SUB(2,29)\
   PIPE_MGR_##_name##_CNT_SUB(2,30)\
   PIPE_MGR_##_name##_CNT_SUB(2,31)\
   PIPE_MGR_##_name##_CNT_SUB(2,32)\
   PIPE_MGR_##_name##_CNT_SUB(2,33)\
   PIPE_MGR_##_name##_CNT_SUB(2,34)\
   PIPE_MGR_##_name##_CNT_SUB(2,35)\
   PIPE_MGR_##_name##_CNT_SUB(2,36)\
   PIPE_MGR_##_name##_CNT_SUB(2,37)\
   PIPE_MGR_##_name##_CNT_SUB(2,38)\
   PIPE_MGR_##_name##_CNT_SUB(2,39)\
   PIPE_MGR_##_name##_CNT_SUB(2,40)\
   PIPE_MGR_##_name##_CNT_SUB(2,41)\
   PIPE_MGR_##_name##_CNT_SUB(2,42)\
   PIPE_MGR_##_name##_CNT_SUB(2,43)\
   PIPE_MGR_##_name##_CNT_SUB(2,44)\
   PIPE_MGR_##_name##_CNT_SUB(2,45)\
   PIPE_MGR_##_name##_CNT_SUB(2,46)\
   PIPE_MGR_##_name##_CNT_SUB(2,47)\
   PIPE_MGR_##_name##_CNT_SUB(2,48)\
   PIPE_MGR_##_name##_CNT_SUB(2,49)\
   PIPE_MGR_##_name##_CNT_SUB(2,50)\
   PIPE_MGR_##_name##_CNT_SUB(2,51)\
   PIPE_MGR_##_name##_CNT_SUB(2,52)\
   PIPE_MGR_##_name##_CNT_SUB(2,53)\
   PIPE_MGR_##_name##_CNT_SUB(2,54)\
   PIPE_MGR_##_name##_CNT_SUB(2,55)\
   PIPE_MGR_##_name##_CNT_SUB(2,56)\
   PIPE_MGR_##_name##_CNT_SUB(2,57)\
   PIPE_MGR_##_name##_CNT_SUB(2,58)\
   PIPE_MGR_##_name##_CNT_SUB(2,59)\
   PIPE_MGR_##_name##_CNT_SUB(2,60)\
   PIPE_MGR_##_name##_CNT_SUB(2,61)\
   PIPE_MGR_##_name##_CNT_SUB(2,62)\
   PIPE_MGR_##_name##_CNT_SUB(2,63)\
   PIPE_MGR_##_name##_CNT_SUB(2,64)\
   PIPE_MGR_##_name##_CNT_SUB(2,65)\
   PIPE_MGR_##_name##_CNT_SUB(2,66)\
   PIPE_MGR_##_name##_CNT_SUB(2,67)\
   PIPE_MGR_##_name##_CNT_SUB(2,68)\
   PIPE_MGR_##_name##_CNT_SUB(2,69)\
   PIPE_MGR_##_name##_CNT_SUB(2,70)\
   PIPE_MGR_##_name##_CNT_SUB(2,71)},\
  {PIPE_MGR_##_name##1_CNT_SUB(3,0)\
   PIPE_MGR_##_name##1_CNT_SUB(3,1)\
   PIPE_MGR_##_name##1_CNT_SUB(3,2)\
   PIPE_MGR_##_name##1_CNT_SUB(3,3)\
   PIPE_MGR_##_name##1_CNT_SUB(3,4)\
   PIPE_MGR_##_name##1_CNT_SUB(3,5)\
   PIPE_MGR_##_name##1_CNT_SUB(3,6)\
   PIPE_MGR_##_name##1_CNT_SUB(3,7)\
   PIPE_MGR_##_name##_CNT_SUB(3,8)\
   PIPE_MGR_##_name##_CNT_SUB(3,9)\
   PIPE_MGR_##_name##_CNT_SUB(3,10)\
   PIPE_MGR_##_name##_CNT_SUB(3,11)\
   PIPE_MGR_##_name##_CNT_SUB(3,12)\
   PIPE_MGR_##_name##_CNT_SUB(3,13)\
   PIPE_MGR_##_name##_CNT_SUB(3,14)\
   PIPE_MGR_##_name##_CNT_SUB(3,15)\
   PIPE_MGR_##_name##_CNT_SUB(3,16)\
   PIPE_MGR_##_name##_CNT_SUB(3,17)\
   PIPE_MGR_##_name##_CNT_SUB(3,18)\
   PIPE_MGR_##_name##_CNT_SUB(3,19)\
   PIPE_MGR_##_name##_CNT_SUB(3,20)\
   PIPE_MGR_##_name##_CNT_SUB(3,21)\
   PIPE_MGR_##_name##_CNT_SUB(3,22)\
   PIPE_MGR_##_name##_CNT_SUB(3,23)\
   PIPE_MGR_##_name##_CNT_SUB(3,24)\
   PIPE_MGR_##_name##_CNT_SUB(3,25)\
   PIPE_MGR_##_name##_CNT_SUB(3,26)\
   PIPE_MGR_##_name##_CNT_SUB(3,27)\
   PIPE_MGR_##_name##_CNT_SUB(3,28)\
   PIPE_MGR_##_name##_CNT_SUB(3,29)\
   PIPE_MGR_##_name##_CNT_SUB(3,30)\
   PIPE_MGR_##_name##_CNT_SUB(3,31)\
   PIPE_MGR_##_name##_CNT_SUB(3,32)\
   PIPE_MGR_##_name##_CNT_SUB(3,33)\
   PIPE_MGR_##_name##_CNT_SUB(3,34)\
   PIPE_MGR_##_name##_CNT_SUB(3,35)\
   PIPE_MGR_##_name##_CNT_SUB(3,36)\
   PIPE_MGR_##_name##_CNT_SUB(3,37)\
   PIPE_MGR_##_name##_CNT_SUB(3,38)\
   PIPE_MGR_##_name##_CNT_SUB(3,39)\
   PIPE_MGR_##_name##_CNT_SUB(3,40)\
   PIPE_MGR_##_name##_CNT_SUB(3,41)\
   PIPE_MGR_##_name##_CNT_SUB(3,42)\
   PIPE_MGR_##_name##_CNT_SUB(3,43)\
   PIPE_MGR_##_name##_CNT_SUB(3,44)\
   PIPE_MGR_##_name##_CNT_SUB(3,45)\
   PIPE_MGR_##_name##_CNT_SUB(3,46)\
   PIPE_MGR_##_name##_CNT_SUB(3,47)\
   PIPE_MGR_##_name##_CNT_SUB(3,48)\
   PIPE_MGR_##_name##_CNT_SUB(3,49)\
   PIPE_MGR_##_name##_CNT_SUB(3,50)\
   PIPE_MGR_##_name##_CNT_SUB(3,51)\
   PIPE_MGR_##_name##_CNT_SUB(3,52)\
   PIPE_MGR_##_name##_CNT_SUB(3,53)\
   PIPE_MGR_##_name##_CNT_SUB(3,54)\
   PIPE_MGR_##_name##_CNT_SUB(3,55)\
   PIPE_MGR_##_name##_CNT_SUB(3,56)\
   PIPE_MGR_##_name##_CNT_SUB(3,57)\
   PIPE_MGR_##_name##_CNT_SUB(3,58)\
   PIPE_MGR_##_name##_CNT_SUB(3,59)\
   PIPE_MGR_##_name##_CNT_SUB(3,60)\
   PIPE_MGR_##_name##_CNT_SUB(3,61)\
   PIPE_MGR_##_name##_CNT_SUB(3,62)\
   PIPE_MGR_##_name##_CNT_SUB(3,63)\
   PIPE_MGR_##_name##_CNT_SUB(3,64)\
   PIPE_MGR_##_name##_CNT_SUB(3,65)\
   PIPE_MGR_##_name##_CNT_SUB(3,66)\
   PIPE_MGR_##_name##_CNT_SUB(3,67)\
   PIPE_MGR_##_name##_CNT_SUB(3,68)\
   PIPE_MGR_##_name##_CNT_SUB(3,69)\
   PIPE_MGR_##_name##_CNT_SUB(3,70)\
   PIPE_MGR_##_name##_CNT_SUB(3,71)},
/* clang-format on */

#define PIPE_MGR_EBUF_CNT                                                    \
  pipe_mgr_pktcnt_disp_t pipe_mgr_ebuf_pktcnt[4][72]                         \
                                             [pipe_mgr_ebuf_pktcnt_size] = { \
                                                 DUP_EBUF(EBUF)};  // ebuf

/* clang-format off */
#define DUP_SP(_name)\
{PIPE_MGR_##_name##_CNT_SUB(0, 0, 0, 0)\
PIPE_MGR_##_name##_CNT_SUB(0, 1, 0, 1)\
PIPE_MGR_##_name##_CNT_SUB(0, 2, 0, 2)\
PIPE_MGR_##_name##_CNT_SUB(0, 3, 0, 3)\
PIPE_MGR_##_name##_CNT_SUB(0, 4, 0, 4)\
PIPE_MGR_##_name##_CNT_SUB(0, 5, 0, 5)\
PIPE_MGR_##_name##_CNT_SUB(0, 6, 0, 6)\
PIPE_MGR_##_name##_CNT_SUB(0, 7, 0, 7)\
PIPE_MGR_##_name##_CNT_SUB(0, 8, 1, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 9, 1, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 10, 1, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 11, 1, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 12, 1, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 13, 1, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 14, 1, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 15, 1, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 16, 2, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 17, 2, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 18, 2, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 19, 2, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 20, 2, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 21, 2, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 22, 2, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 23, 2, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 24, 3, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 25, 3, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 26, 3, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 27, 3, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 28, 3, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 29, 3, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 30, 3, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 31, 3, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 32, 4, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 33, 4, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 34, 4, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 35, 4, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 36, 4, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 37, 4, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 38, 4, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 39, 4, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 40, 5, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 41, 5, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 42, 5, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 43, 5, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 44, 5, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 45, 5, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 46, 5, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 47, 5, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 48, 6, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 49, 6, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 50, 6, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 51, 6, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 52, 6, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 53, 6, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 54, 6, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 55, 6, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 56, 7, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 57, 7, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 58, 7, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 59, 7, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 60, 7, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 61, 7, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 62, 7, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 63, 7, 7) \
PIPE_MGR_##_name##_CNT_SUB(0, 64, 8, 0) \
PIPE_MGR_##_name##_CNT_SUB(0, 65, 8, 1) \
PIPE_MGR_##_name##_CNT_SUB(0, 66, 8, 2) \
PIPE_MGR_##_name##_CNT_SUB(0, 67, 8, 3) \
PIPE_MGR_##_name##_CNT_SUB(0, 68, 8, 4) \
PIPE_MGR_##_name##_CNT_SUB(0, 69, 8, 5) \
PIPE_MGR_##_name##_CNT_SUB(0, 70, 8, 6) \
PIPE_MGR_##_name##_CNT_SUB(0, 71, 8, 7)}, \
{PIPE_MGR_##_name##_CNT_SUB(1, 0, 0, 0)\
PIPE_MGR_##_name##_CNT_SUB(1, 1, 0, 1)\
PIPE_MGR_##_name##_CNT_SUB(1, 2, 0, 2)\
PIPE_MGR_##_name##_CNT_SUB(1, 3, 0, 3)\
PIPE_MGR_##_name##_CNT_SUB(1, 4, 0, 4)\
PIPE_MGR_##_name##_CNT_SUB(1, 5, 0, 5)\
PIPE_MGR_##_name##_CNT_SUB(1, 6, 0, 6)\
PIPE_MGR_##_name##_CNT_SUB(1, 7, 0, 7)\
PIPE_MGR_##_name##_CNT_SUB(1, 8, 1, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 9, 1, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 10, 1, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 11, 1, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 12, 1, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 13, 1, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 14, 1, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 15, 1, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 16, 2, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 17, 2, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 18, 2, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 19, 2, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 20, 2, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 21, 2, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 22, 2, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 23, 2, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 24, 3, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 25, 3, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 26, 3, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 27, 3, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 28, 3, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 29, 3, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 30, 3, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 31, 3, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 32, 4, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 33, 4, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 34, 4, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 35, 4, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 36, 4, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 37, 4, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 38, 4, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 39, 4, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 40, 5, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 41, 5, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 42, 5, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 43, 5, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 44, 5, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 45, 5, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 46, 5, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 47, 5, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 48, 6, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 49, 6, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 50, 6, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 51, 6, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 52, 6, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 53, 6, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 54, 6, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 55, 6, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 56, 7, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 57, 7, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 58, 7, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 59, 7, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 60, 7, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 61, 7, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 62, 7, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 63, 7, 7) \
PIPE_MGR_##_name##_CNT_SUB(1, 64, 8, 0) \
PIPE_MGR_##_name##_CNT_SUB(1, 65, 8, 1) \
PIPE_MGR_##_name##_CNT_SUB(1, 66, 8, 2) \
PIPE_MGR_##_name##_CNT_SUB(1, 67, 8, 3) \
PIPE_MGR_##_name##_CNT_SUB(1, 68, 8, 4) \
PIPE_MGR_##_name##_CNT_SUB(1, 69, 8, 5) \
PIPE_MGR_##_name##_CNT_SUB(1, 70, 8, 6) \
PIPE_MGR_##_name##_CNT_SUB(1, 71, 8, 7)}, \
{PIPE_MGR_##_name##_CNT_SUB(2, 0, 0, 0)\
PIPE_MGR_##_name##_CNT_SUB(2, 1, 0, 1)\
PIPE_MGR_##_name##_CNT_SUB(2, 2, 0, 2)\
PIPE_MGR_##_name##_CNT_SUB(2, 3, 0, 3)\
PIPE_MGR_##_name##_CNT_SUB(2, 4, 0, 4)\
PIPE_MGR_##_name##_CNT_SUB(2, 5, 0, 5)\
PIPE_MGR_##_name##_CNT_SUB(2, 6, 0, 6)\
PIPE_MGR_##_name##_CNT_SUB(2, 7, 0, 7)\
PIPE_MGR_##_name##_CNT_SUB(2, 8, 1, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 9, 1, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 10, 1, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 11, 1, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 12, 1, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 13, 1, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 14, 1, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 15, 1, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 16, 2, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 17, 2, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 18, 2, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 19, 2, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 20, 2, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 21, 2, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 22, 2, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 23, 2, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 24, 3, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 25, 3, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 26, 3, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 27, 3, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 28, 3, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 29, 3, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 30, 3, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 31, 3, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 32, 4, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 33, 4, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 34, 4, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 35, 4, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 36, 4, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 37, 4, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 38, 4, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 39, 4, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 40, 5, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 41, 5, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 42, 5, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 43, 5, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 44, 5, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 45, 5, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 46, 5, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 47, 5, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 48, 6, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 49, 6, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 50, 6, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 51, 6, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 52, 6, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 53, 6, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 54, 6, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 55, 6, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 56, 7, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 57, 7, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 58, 7, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 59, 7, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 60, 7, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 61, 7, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 62, 7, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 63, 7, 7) \
PIPE_MGR_##_name##_CNT_SUB(2, 64, 8, 0) \
PIPE_MGR_##_name##_CNT_SUB(2, 65, 8, 1) \
PIPE_MGR_##_name##_CNT_SUB(2, 66, 8, 2) \
PIPE_MGR_##_name##_CNT_SUB(2, 67, 8, 3) \
PIPE_MGR_##_name##_CNT_SUB(2, 68, 8, 4) \
PIPE_MGR_##_name##_CNT_SUB(2, 69, 8, 5) \
PIPE_MGR_##_name##_CNT_SUB(2, 70, 8, 6) \
PIPE_MGR_##_name##_CNT_SUB(2, 71, 8, 7)}, \
{PIPE_MGR_##_name##_CNT_SUB(3, 0, 0, 0)\
PIPE_MGR_##_name##_CNT_SUB(3, 1, 0, 1)\
PIPE_MGR_##_name##_CNT_SUB(3, 2, 0, 2)\
PIPE_MGR_##_name##_CNT_SUB(3, 3, 0, 3)\
PIPE_MGR_##_name##_CNT_SUB(3, 4, 0, 4)\
PIPE_MGR_##_name##_CNT_SUB(3, 5, 0, 5)\
PIPE_MGR_##_name##_CNT_SUB(3, 6, 0, 6)\
PIPE_MGR_##_name##_CNT_SUB(3, 7, 0, 7)\
PIPE_MGR_##_name##_CNT_SUB(3, 8, 1, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 9, 1, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 10, 1, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 11, 1, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 12, 1, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 13, 1, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 14, 1, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 15, 1, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 16, 2, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 17, 2, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 18, 2, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 19, 2, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 20, 2, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 21, 2, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 22, 2, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 23, 2, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 24, 3, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 25, 3, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 26, 3, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 27, 3, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 28, 3, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 29, 3, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 30, 3, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 31, 3, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 32, 4, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 33, 4, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 34, 4, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 35, 4, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 36, 4, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 37, 4, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 38, 4, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 39, 4, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 40, 5, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 41, 5, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 42, 5, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 43, 5, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 44, 5, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 45, 5, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 46, 5, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 47, 5, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 48, 6, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 49, 6, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 50, 6, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 51, 6, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 52, 6, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 53, 6, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 54, 6, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 55, 6, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 56, 7, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 57, 7, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 58, 7, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 59, 7, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 60, 7, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 61, 7, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 62, 7, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 63, 7, 7) \
PIPE_MGR_##_name##_CNT_SUB(3, 64, 8, 0) \
PIPE_MGR_##_name##_CNT_SUB(3, 65, 8, 1) \
PIPE_MGR_##_name##_CNT_SUB(3, 66, 8, 2) \
PIPE_MGR_##_name##_CNT_SUB(3, 67, 8, 3) \
PIPE_MGR_##_name##_CNT_SUB(3, 68, 8, 4) \
PIPE_MGR_##_name##_CNT_SUB(3, 69, 8, 5) \
PIPE_MGR_##_name##_CNT_SUB(3, 70, 8, 6) \
PIPE_MGR_##_name##_CNT_SUB(3, 71, 8, 7)},

/* clang-format on */

#define PIPE_MGR_IPB_CNT                                                   \
  pipe_mgr_pktcnt_disp_t pipe_mgr_ipb_pktcnt[4][72]                        \
                                            [pipe_mgr_ipb_pktcnt_size] = { \
                                                DUP_SP(IPB)};  // ipb
#define PIPE_MGR_IPB_SP_CNT                                          \
  pipe_mgr_pktcnt_disp_t                                             \
      pipe_mgr_ipb_sp_pktcnt[4][72][pipe_mgr_ipb_sp_pktcnt_size] = { \
          DUP_SP(IPB_SP)};  // ipb special

#define PIPE_MGR_EPB_CNT                                                   \
  pipe_mgr_pktcnt_disp_t pipe_mgr_epb_pktcnt[4][72]                        \
                                            [pipe_mgr_epb_pktcnt_size] = { \
                                                DUP_SP(EPB)};  // epb

PIPE_MGR_P2S_CNT
PIPE_MGR_S2P_CNT
PIPE_MGR_EBUF_CNT
PIPE_MGR_IPRSR_CNT
PIPE_MGR_EPRSR_CNT
PIPE_MGR_IPB_CNT
PIPE_MGR_IPB_SP_CNT
PIPE_MGR_EPB_CNT

#define GET_STRUCT_SIZE(_name) pipe_mgr_##_name##_pktcnt_size
#define PG_PORT_MAX 71
#define PIPE_MAX 4
#define HLINE                                                                  \
  "__________________________________________________________________________" \
  "_________________________"

/* functions */
#if DVM_CONFIG_INCLUDE_UCLI == 1
static void pipe_mgr_tof2_pkt_path_cnt_struct_walk(
    ucli_context_t *uc,
    int hex,
    bool non_zero,
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t *cnt_p) {
  int i;
  uint32_t val;
  uint64_t cnt = 0, cnt_tmp;
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");
  for (i = 0; i < cnt_size; i++) {
    lld_read_register(devid, cnt_p[i].pktcnt, &val);
    cnt = val;
    if (cnt_p[i].cnt_numb > 1) {
      lld_read_register(devid, (cnt_p[i].pktcnt + 0x4ul), &val);
      cnt_tmp = val;
      cnt = cnt + (cnt_tmp << 32);
    }
    if (non_zero && (cnt == 0)) {
      continue;
    }
    aim_printf(&uc->pvs, "%-45s", cnt_p[i].pktcnt_desc);
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", cnt_p[i].pktcnt_name);
    aim_printf(&uc->pvs, "\n");
  }
  return;
}
static void pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
    ucli_context_t *uc,
    int hex,
    bool non_zero,
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t **cnt_p,
    int range) {
  int i, j;
  uint32_t val1, val2;
  uint64_t cnt_tmp;
  uint64_t cnt = 0;
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");
  for (i = 0; i < cnt_size; i++) {
    cnt = 0;
    for (j = 0; j < range; j++) {
      lld_read_register(devid, cnt_p[j][i].pktcnt, &val1);
      if (cnt_p[j][i].cnt_numb > 1) {
        lld_read_register(devid, (cnt_p[j][i].pktcnt + 0x4ul), &val2);
        cnt_tmp = val2;
        cnt = (uint64_t)(cnt + ((cnt_tmp << 32) | val1));
      } else {
        cnt = (uint64_t)(cnt + val1);
      }
    }
    if (non_zero && (cnt == 0)) {
      continue;
    }
    aim_printf(&uc->pvs, "%-45s", cnt_p[0][i].pktcnt_desc);
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", cnt_p[0][i].pktcnt_name);
    aim_printf(&uc->pvs, "\n");
  }
  return;
}
static void pipe_mgr_tof2_ipb_sp_pkt_path_cnt_struct_array_walk(
    ucli_context_t *uc,
    int hex,
    bool non_zero,
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t **cnt_p,
    int range) {
  int i, j;
  uint32_t val1, val2, val3;
  uint64_t cnt_tmp;
  uint64_t cnt = 0, err_cnt = 0;
  for (i = 0; i < cnt_size; (i = i + 2)) {
    cnt = 0;
    err_cnt = 0;
    for (j = 0; j < range; j++) {
      lld_read_register(devid, cnt_p[j][i].pktcnt, &val1);
      lld_read_register(devid, (cnt_p[j][i].pktcnt + 0x4ul), &val2);
      lld_read_register(devid, (cnt_p[j][i].pktcnt + 0x8ul), &val3);
      cnt_tmp = val2;
      cnt = (uint64_t)(cnt + (((cnt_tmp & 0xf) << 32) | val1));
      cnt_tmp = val3;
      err_cnt = (uint64_t)(
          err_cnt + (((val2 >> 4) & 0xfffffff) | ((cnt_tmp & 0xff) << 28)));
    }
    if ((!non_zero) || (non_zero && (cnt != 0))) {
      aim_printf(&uc->pvs, "%-45s", cnt_p[0][i].pktcnt_desc);
      if (!hex) {
        aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
      } else {
        aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
      }
      aim_printf(&uc->pvs, "%-30s", cnt_p[0][i].pktcnt_name);
      aim_printf(&uc->pvs, "\n");
    }
    if ((!non_zero) || (non_zero && (err_cnt != 0))) {
      aim_printf(&uc->pvs, "%-45s", cnt_p[0][i + 1].pktcnt_desc);
      if (!hex) {
        aim_printf(&uc->pvs, "%-30" PRIu64, err_cnt);
      } else {
        aim_printf(&uc->pvs, "%-30" PRIx64, err_cnt);
      }
      aim_printf(&uc->pvs, "%-30s", cnt_p[0][i + 1].pktcnt_name);
      aim_printf(&uc->pvs, "\n");
    }
  }
  return;
}

void pipe_mgr_tof2_pkt_path_display_iprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bool non_zero,
                                                  bf_dev_id_t devid,
                                                  int pipe,
                                                  int pg_port,
                                                  int pg_port_end) {
  int cnt_size;
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_iprsr_pktcnt[pipe][pg_port + i];
  }
  aim_printf(
      &uc->pvs, "%-49s", "iprsr-counter                                ");
  cnt_size = GET_STRUCT_SIZE(iprsr);
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_eprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bool non_zero,
                                                  bf_dev_id_t devid,
                                                  int pipe,
                                                  int pg_port,
                                                  int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(eprsr);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_eprsr_pktcnt[pipe][pg_port + i];
  }
  aim_printf(
      &uc->pvs, "%-49s", "eprsr-counter                                ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_ipb_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(ipb);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  pipe_mgr_pktcnt_disp_t *cnt_p_sp[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_ipb_pktcnt[pipe][pg_port + i];
    cnt_p_sp[i] = pipe_mgr_ipb_sp_pktcnt[pipe][pg_port + i];
  }
  aim_printf(
      &uc->pvs, "%-49s", "ipb-counter                                  ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  pipe_mgr_tof2_ipb_sp_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, GET_STRUCT_SIZE(ipb_sp), cnt_p_sp, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_epb_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(epb);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_epb_pktcnt[pipe][pg_port + i];
  }
  aim_printf(
      &uc->pvs, "%-49s", "epb-counter                                  ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_ebuf_counter(ucli_context_t *uc,
                                                 int hex,
                                                 bool non_zero,
                                                 bf_dev_id_t devid,
                                                 int pipe,
                                                 int pg_port,
                                                 int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(ebuf);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_ebuf_pktcnt[pipe][pg_port + i];
  }
  aim_printf(
      &uc->pvs, "%-49s", "ebuf-counter                                 ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_s2p_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(s2p);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_s2p_pktcnt[pipe][pg_port + i];
  }

  /* Poke the sample register before reading the counters. */
  lld_write_register(
      devid,
      offsetof(tof2_reg, pipes[pipe].pardereg.pgstnreg.s2preg.ctr_sample),
      1);

  aim_printf(
      &uc->pvs, "%-49s", "s2p-counter                                  ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_p2s_counter(ucli_context_t *uc,
                                                int hex,
                                                bool non_zero,
                                                bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(p2s);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_p2s_pktcnt[pipe][pg_port + i];
  }

  /* Poke the sample register before reading the counters. */
  lld_write_register(
      devid,
      offsetof(tof2_reg, pipes[pipe].pardereg.pgstnreg.p2sreg.ctr_sample),
      1);

  aim_printf(
      &uc->pvs, "%-49s", "p2s-counter                                  ");
  pipe_mgr_tof2_pkt_path_cnt_struct_array_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p, range);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_idprsr_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe) {
  aim_printf(
      &uc->pvs, "%-49s", "idprsr-counter                               ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  int count = 0;
  bf_packetpath_counter_t *r =
      pipe_mgr_tof2_pkt_path_idprsr_read_counter(dev_info, pipe, &count);
  if (!r) return;

  for (int i = 0; i < count; ++i) {
    if (non_zero && (r[i].value == 0)) {
      continue;
    }
    aim_printf(&uc->pvs, "%-45s", r[i].description);
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, r[i].value);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, r[i].value);
    }
    aim_printf(&uc->pvs, "%-30s", pipe_mgr_idprsr_pktcnt[i].pktcnt_name);
    aim_printf(&uc->pvs, "\n");
  }

  PIPE_MGR_FREE(r);

  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_edprsr_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe) {
  aim_printf(
      &uc->pvs, "%-49s", "edprsr-counter                               ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  int count = 0;
  bf_packetpath_counter_t *r =
      pipe_mgr_tof2_pkt_path_edprsr_read_counter(dev_info, pipe, &count);
  if (!r) return;

  for (int i = 0; i < count; ++i) {
    if (non_zero && (r[i].value == 0)) {
      continue;
    }
    aim_printf(&uc->pvs, "%-45s", r[i].description);
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, r[i].value);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, r[i].value);
    }
    aim_printf(&uc->pvs, "%-30s", pipe_mgr_edprsr_pktcnt[i].pktcnt_name);
    aim_printf(&uc->pvs, "\n");
  }

  PIPE_MGR_FREE(r);

  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_pmarb_counter(
    ucli_context_t *uc, int hex, bool non_zero, bf_dev_id_t devid, int pipe) {
  int cnt_size = GET_STRUCT_SIZE(pmarb);
  pipe_mgr_pktcnt_disp_t *cnt_p = pipe_mgr_pmarb_pktcnt[pipe];
  aim_printf(
      &uc->pvs, "%-49s", "pmarb-counter                                ");
  pipe_mgr_tof2_pkt_path_cnt_struct_walk(
      uc, hex, non_zero, devid, cnt_size, cnt_p);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}
void pipe_mgr_tof2_pkt_path_display_pipe_counter(ucli_context_t *uc,
                                                 int hex,
                                                 bf_dev_id_t devid,
                                                 int pipe) {
  pipe_mgr_tof2_pkt_path_display_idprsr_counter(uc, hex, false, devid, pipe);
  pipe_mgr_tof2_pkt_path_display_edprsr_counter(uc, hex, false, devid, pipe);
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof2_nonzero_chip_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid) {
  // all learn counter
  // FIXME
  (void)uc;
  (void)devid;
}
void pipe_mgr_tof2_per_chip_counter_cli(ucli_context_t *uc, bf_dev_id_t devid) {
  // all learn counter
  // FIXME
  (void)uc;
  (void)devid;
}
void pipe_mgr_tof2_nonzero_pipe_counter_cli(ucli_context_t *uc,
                                            bf_dev_id_t devid,
                                            int pipe) {
  pipe_mgr_tof2_pkt_path_display_idprsr_counter(uc, true, true, devid, pipe);
  pipe_mgr_tof2_pkt_path_display_edprsr_counter(uc, true, true, devid, pipe);
  pipe_mgr_tof2_pkt_path_display_pmarb_counter(uc, true, true, devid, pipe);
}
void pipe_mgr_tof2_pipe_counter_cli(ucli_context_t *uc,
                                    bf_dev_id_t devid,
                                    int pipe) {
  pipe_mgr_tof2_pkt_path_display_idprsr_counter(uc, true, false, devid, pipe);
  pipe_mgr_tof2_pkt_path_display_edprsr_counter(uc, true, false, devid, pipe);
  pipe_mgr_tof2_pkt_path_display_pmarb_counter(uc, true, false, devid, pipe);
}
void pipe_mgr_tof2_nonzero_pipe_and_port_counter_cli(
    ucli_context_t *uc, bf_dev_id_t devid, int pipe, int port, int port_end) {
  pipe_mgr_tof2_pkt_path_display_iprsr_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_eprsr_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_ipb_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_epb_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_ebuf_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_s2p_counter(
      uc, true, true, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_p2s_counter(
      uc, true, true, devid, pipe, port, port_end);
}
void pipe_mgr_tof2_pipe_and_port_counter_cli(
    ucli_context_t *uc, bf_dev_id_t devid, int pipe, int port, int port_end) {
  pipe_mgr_tof2_pkt_path_display_iprsr_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_eprsr_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_ipb_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_epb_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_ebuf_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_s2p_counter(
      uc, true, false, devid, pipe, port, port_end);
  pipe_mgr_tof2_pkt_path_display_p2s_counter(
      uc, true, false, devid, pipe, port, port_end);
}

#endif
static void tof2_pkt_path_do_read(bf_dev_id_t dev_id,
                                  int count,
                                  uint32_t offset,
                                  pipe_mgr_pktcnt_disp_t *src,
                                  bf_packetpath_counter_t *dst) {
  for (int i = 0; i < count; ++i) {
    dst[i].description_index = 0;
    dst[i].description = src[i].pktcnt_desc;
    dst[i].counter_type = src[i].counter_type;
    dst[i].value = 0;
    for (int j = 0; j < src[i].cnt_numb; ++j) {
      uint32_t x = 0;
      uint64_t y = 0;
      lld_read_register(dev_id, src[i].pktcnt + offset + 4 * j, &x);
      y = x;
      dst[i].value += y << (32 * j);
    }
  }
}
static void tof2_pkt_path_do_clear(bf_dev_id_t dev_id,
                                   int count,
                                   uint32_t offset,
                                   pipe_mgr_pktcnt_disp_t *src) {
  for (int i = 0; i < count; ++i)
    for (int j = 0; j < src[i].cnt_numb; ++j)
      lld_write_register(dev_id, src[i].pktcnt + offset + 4 * j, 0);
}
static void pipe_mgr_tof2_pkt_path_clear_cnt_struct_walk(
    bf_dev_id_t devid, int cnt_size, pipe_mgr_pktcnt_disp_t *cnt_p) {
  int i;
  for (i = 0; i < cnt_size; i++) {
    lld_write_register(devid, cnt_p[i].pktcnt, 0);
    if (cnt_p[i].cnt_numb > 1) {
      lld_write_register(devid, (cnt_p[i].pktcnt + 0x4ul), 0);
    }
  }
  return;
}
static void pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t **cnt_p,
    int range) {
  int i, j;
  for (i = 0; i < cnt_size; i++) {
    for (j = 0; j < range; j++) {
      lld_write_register(devid, cnt_p[j][i].pktcnt, 0);
      if (cnt_p[j][i].cnt_numb > 1) {
        lld_write_register(devid, (cnt_p[j][i].pktcnt + 0x4ul), 0);
      }
    }
  }
  return;
}
static void pipe_mgr_tof2_ipb_sp_pkt_path_clear_cnt_struct_array_walk(
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t **cnt_p,
    int range) {
  int i, j;
  for (i = 0; i < cnt_size; (i = i + 2)) {
    for (j = 0; j < range; j++) {
      lld_write_register(devid, cnt_p[j][i].pktcnt, 0);
      lld_write_register(devid, (cnt_p[j][i].pktcnt + 0x4ul), 0);
      lld_write_register(devid, (cnt_p[j][i].pktcnt + 0x8ul), 0);
    }
  }
  return;
}
void pipe_mgr_tof2_pkt_path_clear_iprsr_counter(bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(iprsr);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_iprsr_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_eprsr_counter(bf_dev_id_t devid,
                                                int pipe,
                                                int pg_port,
                                                int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(eprsr);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_eprsr_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_ipb_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(ipb);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  pipe_mgr_pktcnt_disp_t *cnt_p_sp[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_ipb_pktcnt[pipe][pg_port + i];
    cnt_p_sp[i] = pipe_mgr_ipb_sp_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
  pipe_mgr_tof2_ipb_sp_pkt_path_clear_cnt_struct_array_walk(
      devid, GET_STRUCT_SIZE(ipb_sp), cnt_p_sp, range);
}
void pipe_mgr_tof2_pkt_path_clear_epb_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(epb);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_epb_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_ebuf_counter(bf_dev_id_t devid,
                                               int pipe,
                                               int pg_port,
                                               int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(ebuf);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_ebuf_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_s2p_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(s2p);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_s2p_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_p2s_counter(bf_dev_id_t devid,
                                              int pipe,
                                              int pg_port,
                                              int pg_port_end) {
  int cnt_size = GET_STRUCT_SIZE(p2s);
  int i;
  int range = pg_port_end ? (pg_port_end - pg_port) : 1;
  pipe_mgr_pktcnt_disp_t *cnt_p[8];
  if ((pipe >= 4) || (range > 8) || (range < 1)) return;
  if (pg_port + range > TOF2_NUM_PORTS_PER_PIPE) return;
  for (i = 0; i < range; i++) {
    cnt_p[i] = pipe_mgr_p2s_pktcnt[pipe][pg_port + i];
  }
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_array_walk(
      devid, cnt_size, cnt_p, range);
}
void pipe_mgr_tof2_pkt_path_clear_idprsr_counter(rmt_dev_info_t *dev_info,
                                                 int pipe) {
  uint32_t offset = dev_info->dev_cfg.dir_addr_set_pipe_id(0, pipe);
  /* Watch out, some of the counters cannot be written!  We need to update the
   * pipe_mgr_pktcnt_disp_t struct to indicate this, for now we are just using
   * a smaller size. */
  int c = 28;
  tof2_pkt_path_do_clear(dev_info->dev_id, c, offset, pipe_mgr_idprsr_pktcnt);
}
void pipe_mgr_tof2_pkt_path_clear_edprsr_counter(rmt_dev_info_t *dev_info,
                                                 int pipe) {
  uint32_t offset = dev_info->dev_cfg.dir_addr_set_pipe_id(0, pipe);
  /* Watch out, some of the counters cannot be written!  We need to update the
   * pipe_mgr_pktcnt_disp_t struct to indicate this, for now we are just using
   * a smaller size. */
  int c = 26;
  tof2_pkt_path_do_clear(dev_info->dev_id, c, offset, pipe_mgr_edprsr_pktcnt);
}
void pipe_mgr_tof2_pkt_path_clear_pmarb_counter(bf_dev_id_t devid, int pipe) {
  int cnt_size = GET_STRUCT_SIZE(pmarb);
  pipe_mgr_pktcnt_disp_t *cnt_p = pipe_mgr_pmarb_pktcnt[pipe];
  pipe_mgr_tof2_pkt_path_clear_cnt_struct_walk(devid, cnt_size, cnt_p);
}
void pipe_mgr_tof2_pkt_path_clear_pipe_counter(bf_dev_id_t devid, int pipe) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  pipe_mgr_tof2_pkt_path_clear_idprsr_counter(dev_info, pipe);
  pipe_mgr_tof2_pkt_path_clear_edprsr_counter(dev_info, pipe);
}
void pipe_mgr_tof2_pkt_path_clear_all_counter(bf_dev_id_t devid, int pipe) {
  int pg_port;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return;
  }
  for (pg_port = 0; pg_port < 72; pg_port++) {
    pipe_mgr_tof2_pkt_path_clear_iprsr_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_eprsr_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_ipb_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_epb_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_ebuf_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_s2p_counter(devid, pipe, pg_port, 0);
    pipe_mgr_tof2_pkt_path_clear_p2s_counter(devid, pipe, pg_port, 0);
  }
  pipe_mgr_tof2_pkt_path_clear_idprsr_counter(dev_info, pipe);
  pipe_mgr_tof2_pkt_path_clear_edprsr_counter(dev_info, pipe);
  pipe_mgr_tof2_pkt_path_clear_pmarb_counter(devid, pipe);
}
/*read*/
static void pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t *cnt_p,
    bf_packetpath_counter_t *r) {
  int i;
  uint32_t val;
  uint64_t cnt = 0, cnt_tmp;
  for (i = 0; i < cnt_size; i++) {
    lld_read_register(devid, cnt_p[i].pktcnt, &val);
    cnt = val;
    if (cnt_p[i].cnt_numb > 1) {
      lld_read_register(devid, (cnt_p[i].pktcnt + 0x4ul), &val);
      cnt_tmp = val;
      cnt = cnt + (cnt_tmp << 32);
    }
    r[i].description_index = 0xffffffff;
    r[i].description = cnt_p[i].pktcnt_desc;
    r[i].value = cnt;
    r[i].counter_type = cnt_p[i].counter_type;
  }
  return;
}
static void pipe_mgr_tof2_ipb_sp_pkt_path_cnt_struct_read_walk(
    bf_dev_id_t devid,
    int cnt_size,
    pipe_mgr_pktcnt_disp_t *cnt_p,
    bf_packetpath_counter_t *r) {
  int i;
  uint32_t val1, val2, val3;
  uint64_t cnt, err_cnt, cnt_tmp;
  for (i = 0; i < cnt_size; (i = i + 2)) {
    lld_read_register(devid, cnt_p[i].pktcnt, &val1);
    lld_read_register(devid, (cnt_p[i].pktcnt + 0x4ul), &val2);
    lld_read_register(devid, (cnt_p[i].pktcnt + 0x8ul), &val3);
    cnt_tmp = val2;
    cnt = (((cnt_tmp & 0xf) << 32) | val1);
    cnt_tmp = val3;
    err_cnt = ((val2 >> 4) & 0xfffffff) | ((cnt_tmp & 0xff) << 28);
    r[i].description_index = 0xffffffff;
    r[i].description = cnt_p[i].pktcnt_desc;
    r[i].value = cnt;
    r[i].counter_type = cnt_p[i].counter_type;
    r[i + 1].description_index = 0xffffffff;
    r[i + 1].description = cnt_p[i + 1].pktcnt_desc;
    r[i + 1].value = err_cnt;
    r[i + 1].counter_type = cnt_p[i + 1].counter_type;
  }
  return;
}

bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_iprsr_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p;
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(iprsr);
  int i = 0;
  if (count == NULL) return NULL;
  counter_head =
      bf_sys_calloc(cnt_size * port_numb, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size * port_numb);
  while (i < port_numb) {
    cnt_p = pipe_mgr_iprsr_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size)));
    i++;
  }
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_idprsr_read_counter(
    rmt_dev_info_t *dev_info, int pipe, int *count) {
  int c = sizeof pipe_mgr_idprsr_pktcnt / sizeof pipe_mgr_idprsr_pktcnt[0];
  bf_packetpath_counter_t *r =
      bf_sys_calloc(c, sizeof(bf_packetpath_counter_t));
  if (!r) return NULL;

  /* Poke the sample register for idprsr counters. */
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 2; ++j)
      lld_write_register(dev_info->dev_id,
                         offsetof(tof2_reg,
                                  pipes[pipe]
                                      .pardereg.dprsrreg.dprsrreg.ho_i[i]
                                      .out_ingr.perf_probe),
                         j);

  uint32_t offset = dev_info->dev_cfg.dir_addr_set_pipe_id(0, pipe);
  tof2_pkt_path_do_read(dev_info->dev_id, c, offset, pipe_mgr_idprsr_pktcnt, r);
  if (count) *count = c;
  return r;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_epb_ebuf_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p;
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(ebuf);
  int cnt_size1 = GET_STRUCT_SIZE(epb);
  int cnt_size_all = cnt_size + cnt_size1;
  int i = 0;
  if (count == NULL) return NULL;
  counter_head =
      bf_sys_calloc(cnt_size_all * port_numb, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size_all * port_numb);
  while (i < port_numb) {
    cnt_p = pipe_mgr_ebuf_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size_all)));
    cnt_p = pipe_mgr_epb_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid,
        cnt_size1,
        cnt_p,
        (counter_head + (i * cnt_size_all + cnt_size)));
    i++;
  }
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_eprsr_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p;
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(eprsr);
  int i = 0;
  if (count == NULL) return NULL;
  counter_head =
      bf_sys_calloc(cnt_size * port_numb, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size * port_numb);
  while (i < port_numb) {
    cnt_p = pipe_mgr_eprsr_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size)));
    i++;
  }
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_edprsr_read_counter(
    rmt_dev_info_t *dev_info, int pipe, int *count) {
  int c = sizeof pipe_mgr_edprsr_pktcnt / sizeof pipe_mgr_edprsr_pktcnt[0];
  bf_packetpath_counter_t *r =
      bf_sys_calloc(c, sizeof(bf_packetpath_counter_t));
  if (!r) return NULL;

  /* Poke the sample register for edprsr counters. */
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 2; ++j)
      lld_write_register(dev_info->dev_id,
                         offsetof(tof2_reg,
                                  pipes[pipe]
                                      .pardereg.dprsrreg.dprsrreg.ho_e[i]
                                      .out_egr.perf_probe),
                         j);

  uint32_t offset = dev_info->dev_cfg.dir_addr_set_pipe_id(0, pipe);
  tof2_pkt_path_do_read(dev_info->dev_id, c, offset, pipe_mgr_edprsr_pktcnt, r);
  if (count) *count = c;
  return r;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_pmarb_read_counter(
    bf_dev_id_t devid, int pipe, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p = pipe_mgr_pmarb_pktcnt[pipe];
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(pmarb);
  if (count == NULL) return NULL;
  counter_head = bf_sys_calloc(cnt_size, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
      devid, cnt_size, cnt_p, counter_head);
  *count = cnt_size;
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_s2p_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p;
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(s2p);
  int i = 0;
  if (count == NULL) return NULL;
  counter_head =
      bf_sys_calloc(cnt_size * port_numb, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size * port_numb);
  while (i < port_numb) {
    cnt_p = pipe_mgr_s2p_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size)));
    i++;
  }
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_p2s_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(p2s);
  int i = 0;
  if (count == NULL) return NULL;
  counter_head =
      bf_sys_calloc(cnt_size * port_numb, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size * port_numb);
  while (i < port_numb) {
    pipe_mgr_pktcnt_disp_t *cnt_p = pipe_mgr_p2s_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size)));
    i++;
  }
  return counter_head;
}
bf_packetpath_counter_t *pipe_mgr_tof2_pkt_path_ipb_read_counter(
    bf_dev_id_t devid, int pipe, int pg_port, int port_numb, int *count) {
  pipe_mgr_pktcnt_disp_t *cnt_p;
  bf_packetpath_counter_t *counter_head = NULL;
  int cnt_size = GET_STRUCT_SIZE(ipb);
  int sp_cnt_size = GET_STRUCT_SIZE(ipb_sp);
  int cnt_size_all = cnt_size + sp_cnt_size;
  int i = 0;
  if (count == NULL) return NULL;
  counter_head = bf_sys_calloc((cnt_size_all * port_numb),
                               sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  *count = (cnt_size_all * port_numb);
  while (i < port_numb) {
    cnt_p = pipe_mgr_ipb_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_pkt_path_cnt_struct_read_walk(
        devid, cnt_size, cnt_p, (counter_head + (i * cnt_size_all)));
    cnt_p = pipe_mgr_ipb_sp_pktcnt[pipe][pg_port + i];
    pipe_mgr_tof2_ipb_sp_pkt_path_cnt_struct_read_walk(
        devid,
        sp_cnt_size,
        cnt_p,
        (counter_head + (i * cnt_size_all + cnt_size)));
    i++;
  }
  return counter_head;
}
