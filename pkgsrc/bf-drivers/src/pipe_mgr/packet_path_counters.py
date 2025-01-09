################################################################################
 #  Copyright (C) 2024 Intel Corporation
 #
 #  Licensed under the Apache License, Version 2.0 (the "License");
 #  you may not use this file except in compliance with the License.
 #  You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 #  Unless required by applicable law or agreed to in writing,
 #  software distributed under the License is distributed on an "AS IS" BASIS,
 #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 #  See the License for the specific language governing permissions
 #  and limitations under the License.
 #
 #
 #  SPDX-License-Identifier: Apache-2.0
################################################################################

#!/ usr / bin / python

""" Python Code to generate packet path counters "C" Routines """

import os, re, sys
import pdb


PERPIPE = 1
NONPIPE = 0
DIRECT = 1
INDIRECT = 0

BF_CR_HEADER = (
"""
/*******************************************************************************
 *
 *
 *
 ******************************************************************************/

/*
 * This code is generated using  packet_path_counters.py ; Any edits to this
 * file will be lost.
 */

""")

PKT_PATH_INCLUDES = (
"""
#define __STDC_FORMAT_MACROS
#include <stddef.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <tofino_regs/tofino.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include "pipe_mgr_tof_pkt_path_counter_display.h"

""")

HLINE = (
"""
#define HLINE                                                                  \
  "__________________________________________________________________________" \
  "_________________________"

""")

PKT_PATH_H_FILE_DEF = (
"""
#ifndef __TOFINO_PKT_PATH_DISPLAY_H__
#define __TOFINO_PKT_PATH_DISPLAY_H__

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_tof_counters.h>

#if DVM_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#endif

""")


# Counter Table Schema
# (A,B,C,D,E, F, G)
#   where A = CounterName
#   where B = Indicate Per channel Counter or not. 
#             (1 => Per channel, 0 => Not Per channel)
#   where C = CounterWidth (Width specified as 1 => 32bit, 2 => 64bit)
#   where D = CounterDepth (If counter is array of registers or a table
#                           of values, this field indicates depth.)
#   where E = RegisterHierarchy
#   where F = RegisterName
#   where G = Counter-Type
#             0 => Packet Count
#             1 => Packet Drop Count
#             2 => Packet Error Count
#             3 => Internal Error Count (example: FIFO full count)
#             4 => Internal Count (example: counting PHVs)
#             5 => Status


#Counters should be listed in blocks of 4 channels..chan0, chan1, chan2, \
    #chan3.This order is required.
PKT_PATH_IBUF_COUNTERS = [
("IBUF: Pkts From MAC on Channel0",             1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_macs_received_pkt", 0),
("IBUF: Pkts From MAC on Channel1",             1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_macs_received_pkt", 0),
("IBUF: Pkts From MAC on Channel2",             1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_macs_received_pkt", 0),
("IBUF: Pkts From MAC on Channel3",             1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_macs_received_pkt", 0),
("IBUF: Pkts from channel0 sent to Parser",     1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_parser_send_pkt", 0),
("IBUF: Pkts from channel1 sent to Parser",     1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_parser_send_pkt" ,0),
("IBUF: Pkts from channel2 sent to Parser",     1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_parser_send_pkt", 0),
("IBUF: Pkts from channel3 sent to Parser",     1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_parser_send_pkt", 0),
("IBUF: Pkts from channel0 sent to DeParser",   1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_deparser_send_pkt", 0),
("IBUF: Pkts from channel1 sent to DeParser",   1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_deparser_send_pkt", 0),
("IBUF: Pkts from channel2 sent to DeParser",   1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_deparser_send_pkt", 0),
("IBUF: Pkts from channel3 sent to DeParser",   1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_deparser_send_pkt", 0),
("IBUF: Recirculation Pkts on channel0",        1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_recirc_received_pkt", 0),
("IBUF: Recirculation Pkts on channel1",        1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_recirc_received_pkt", 0),
("IBUF: Recirculation Pkts on channel2",        1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_recirc_received_pkt", 0),
("IBUF: Recirculation Pkts on channel3",        1, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_recirc_received_pkt", 0),
("IBUF: Total Pkts Dropped on channel0",        1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_deparser_drop_pkt", 1),
("IBUF: Total Pkts Dropped on channel1",        1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_deparser_drop_pkt", 1),
("IBUF: Total Pkts Dropped on channel2",        1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_deparser_drop_pkt" ,1),
("IBUF: Total Pkts Dropped on channel3",        1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_deparser_drop_pkt", 1),
("IBUF: Discarded Pkts in ibuf on channel0",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_wsch_discard_pkt", 1),
("IBUF: Discarded Pkts in ibuf on channel1",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_wsch_discard_pkt", 1),
("IBUF: Discarded Pkts in ibuf on channel2",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_wsch_discard_pkt", 1),
("IBUF: Discarded Pkts in ibuf on channel3",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_wsch_discard_pkt", 1),
("IBUF: Truncated Pkts on channel0",            1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_wsch_trunc_pkt", 2),
("IBUF: Truncated Pkts on channel1",            1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_wsch_trunc_pkt", 2),
("IBUF: Truncated Pkts on channel2",            1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_wsch_trunc_pkt", 2),
("IBUF: Truncated Pkts on channel3",            1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_wsch_trunc_pkt", 2),
("IBUF: Discarded Recirculated Pkts on ch0",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_recirc_discard_pkt", 1),
("IBUF: Discarded Recirculated Pkts on ch1",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_recirc_discard_pkt", 1),
("IBUF: Discarded Recirculated Pkts on ch2",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_recirc_discard_pkt", 1),
("IBUF: Discarded Recirculated Pkts on ch3",    1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_recirc_discard_pkt", 1),
("IBUF: Discarded Pkts in PRSR due to ibuf ch0 full or prsr req to drop ",  1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan0_group.chnl_parser_discard_pkt", 1),
("IBUF: Discarded Pkts in PRSR due to ibuf ch1 full or prsr req to drop ",  1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan1_group.chnl_parser_discard_pkt", 1),
("IBUF: Discarded Pkts in PRSR due to ibuf ch2 full or prsr req to drop ",  1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan2_group.chnl_parser_discard_pkt", 1),
("IBUF: Discarded Pkts in PRSR due to ibuf ch3 full or prsr req to drop ",  1, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.chan3_group.chnl_parser_discard_pkt", 1),
]

PKT_PATH_IPRSR_COUNTERS = [
("IPRSR: Total Pkts Dropped on Channel0",       1, 1, 1,   "pmarb.ibp18_reg.ibp_reg",  "prsr_reg.pkt_drop_cnt[0]", 1),
("IPRSR: Total Pkts Dropped on Channel1",       1, 1, 1,   "pmarb.ibp18_reg.ibp_reg",  "prsr_reg.pkt_drop_cnt[1]", 1),
("IPRSR: Total Pkts Dropped on Channel2",       1, 1, 1,   "pmarb.ibp18_reg.ibp_reg",  "prsr_reg.pkt_drop_cnt[2]", 1),
("IPRSR: Total Pkts Dropped on Channel3",       1, 1, 1,   "pmarb.ibp18_reg.ibp_reg",  "prsr_reg.pkt_drop_cnt[3]", 1),
("IPRSR: output fifo full counter",             0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.op_fifo_full_cnt", 3),
("IPRSR: output fifo stall counter",            0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.op_fifo_full_stall_cnt", 3),
("IPRSR: TCAM match Error",                     0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.no_tcam_match_err_cnt", 2),
("IPRSR: Partial Header Error",                 0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.partial_hdr_err_cnt", 2),
("IPRSR: Counter Range Error",                  0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.ctr_range_err_cnt", 2),
("IPRSR: Timeout or Excess state iteration Error", 0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.timeout_iter_err_cnt", 2),
("IPRSR: Timeout or Excess clock cycle",           0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.timeout_cycle_err_cnt", 2),
("IPRSR: Extraction source error counter",      0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.src_ext_err_cnt", 2),
("IPRSR: Container size error counter",         0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.dst_cont_err_cnt", 2),
("IPRSR: PHV owner error counter",              0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.phv_owner_err_cnt", 2),
("IPRSR: PHV multiple write error",             0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.multi_wr_err_cnt", 2),
("IPRSR: FCS error",                            0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.fcs_err_cnt", 2),
("IPRSR: Checksum error",                       0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.csum_err_cnt", 2),
("IPRSR: TCAM parity error",                    0, 2, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.tcam_par_err_cnt", 2),
]

PKT_PATH_IDPRSR_COUNTERS = [
("IDPRSR: header too long",                     0, 1, 1,   "deparser.inp.iir", "hdr_too_long_i", 2),
("IDPRSR: Valid PHV into Deparser",             0, 2, 1,   "deparser.inp.iir.main_i", "cnt_i_phv", 4),
("IDPRSR: Valid TPHV into Deparser",            0, 2, 1,   "deparser.inp.iir.main_i", "cnt_i_tphv", 4),
("IDPRSR: Packets into Deparser",               0, 2, 1,   "deparser.inp.iir.ingr", "cnt_i_read", 0),
("IDPRSR: Packets discarded ",                  0, 2, 1,   "deparser.inp.iir.ingr", "cnt_i_discard", 1),
("IDPRSR: Packets resubmitted ",                0, 2, 1,   "deparser.inp.iir.ingr", "cnt_i_resubmit", 0),
("IDPRSR: Learn Counter due to packets ",     0, 2, 1,   "deparser.inp.iir.ingr", "cnt_i_learn", 0),
("IDPRSR: Packets processed in output phase ",  0, 2, 1,   "deparser.out_ingr.regs", "cnt_pkts", 0),
("IDPRSR: CRC error on packets from Ibuf ",     0, 1, 72, "deparser.out_ingr.regs", "ctm_crc_err", 2),
("IDPRSR: Errored packets to TM ",              0, 1, 1,   "deparser.out_ingr.regs", "i_egr_pkt_err", 2),
("IDPRSR: Errored packets from Ibuf to iCTM",   0, 1, 1,   "deparser.out_ingr.regs", "i_ctm_pkt_err", 2),
("IDPRSR: Packets sent to TM",                  0, 2, 1,   "deparser.out_ingr.regs", "i_fwd_pkts", 0),
("IDPRSR: Packets discarded at TM interface ",  0, 2, 1,   "deparser.out_ingr.regs", "i_disc_pkts", 1),
("IDPRSR: Packets sent to Ingress mirror buffer", 0, 2, 1,   "deparser.out_ingr.regs", "i_mirr_pkts", 0),
]

PKT_PATH_EPB_COUNTERS = [
("EPB: Total Pkts bypassing Epipe on Channel ",            1, 2, 4,   "pmarb.ebp18_reg.egrNx_reg", "epb_disp_port_regs.egr_bypass_count", 0),
("EPB: Total Pkts sent to Epipe on Channel ",              1, 2, 4,   "pmarb.ebp18_reg.egrNx_reg", "epb_disp_port_regs.egr_pipe_count", 0),
]

PKT_PATH_EPRSR_COUNTERS = [
("EPRSR: output fifo full counter",             0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.op_fifo_full_cnt", 3),
("EPRSR: output fifo stall counter",            0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.op_fifo_full_stall_cnt", 3),
("EPRSR: Total Pkts Dropped on channel0",       1, 1, 1, "pmarb.ebp18_reg.ebp_reg",  "prsr_reg.pkt_drop_cnt[0]", 1),
("EPRSR: Total Pkts Dropped on channel1",       1, 1, 1, "pmarb.ebp18_reg.ebp_reg",  "prsr_reg.pkt_drop_cnt[1]", 1),
("EPRSR: Total Pkts Dropped on channel2",       1, 1, 1, "pmarb.ebp18_reg.ebp_reg",  "prsr_reg.pkt_drop_cnt[2]", 1),
("EPRSR: Total Pkts Dropped on channel3",       1, 1, 1, "pmarb.ebp18_reg.ebp_reg",  "prsr_reg.pkt_drop_cnt[3]", 1),
("EPRSR: TCAM match Error",                     0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.no_tcam_match_err_cnt", 2),
("EPRSR: Partial Header Error",                 0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.partial_hdr_err_cnt", 2),
("EPRSR: Counter Range Error",                  0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.ctr_range_err_cnt", 2),
("EPRSR: Timeout or Excess state iteration Error", 0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.timeout_iter_err_cnt", 2),
("EPRSR: Timeout or Excess clock cycle",           0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.timeout_cycle_err_cnt", 2),
("EPRSR: Extraction source error counter",      0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.src_ext_err_cnt", 2),
("EPRSR: Container size error counter",         0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.dst_cont_err_cnt", 2),
("EPRSR: PHV owner error counter",              0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.phv_owner_err_cnt", 2),
("EPRSR: PHV multiple write error",             0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.multi_wr_err_cnt", 2),
("EPRSR: FCS error",                            0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.fcs_err_cnt", 2),
("EPRSR: Checksum error",                       0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.csum_err_cnt", 2),
("EPRSR: TCAM parity error",                    0, 2, 1, "pmarb.ebp18_reg.ebp_reg", "prsr_reg.tcam_par_err_cnt", 2),
]

PKT_PATH_EDPRSR_COUNTERS = [
("EDPRSR: header too long",                     0, 1, 1,   "deparser.inp.ier", "hdr_too_long_e", 2),
("EDPRSR: Valid PHV into Deparser",             0, 2, 1,   "deparser.inp.ier.main_e", "cnt_i_phv", 4),
("EDPRSR: Valid TPHV into Deparser",            0, 2, 1,   "deparser.inp.ier.main_e", "cnt_i_tphv", 4),
("EDPRSR: Packets processed in output phase ",  0, 2, 1,   "deparser.out_egr.regs", "cnt_pkts", 0),
("EDPRSR: CRC error on packets from ebuf ",     0, 1, 72,  "deparser.out_egr.regs", "ctm_crc_err", 2),
("EDPRSR: Errored packets from deprsr to Ebuf ",0, 1, 1,   "deparser.out_egr.regs", "e_egr_pkt_err", 2),
("EDPRSR: Errored packets from Ebuf to eCTM ",  0, 1, 1,   "deparser.out_egr.regs", "e_ctm_pkt_err", 2),
("EDPRSR: Packets sent out of Egress Deparser", 0, 2, 1,   "deparser.out_egr.regs", "e_fwd_pkts", 0),
("EDPRSR: Packets discarded ",                  0, 2, 1,   "deparser.out_egr.regs", "e_disc_pkts", 1),
("EDPRSR: Packets sent to Egress mirror buffer",0, 2, 1,   "deparser.out_egr.regs", "e_mirr_pkts", 0),
]

PKT_PATH_PER_PORT_INTERRUPTS = [
("IBUF:  Interrupt Status",   0, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "ing_buf_regs.glb_group.int_stat", 5),
("IPRSR: Interrupt Status",   0, 1, 1,   "pmarb.ibp18_reg.ibp_reg", "prsr_reg.intr.status", 5),
("EPRSR: Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.ebp_reg", "epb_prsr_port_regs.int_stat", 5),
("EPRSR: Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.ebp_reg", "prsr_reg.intr.status", 5),
("EBUF:  Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.egrNx_reg", "ebuf_disp_regs.int_stat", 5),
("EBUF:  Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.egrNx_reg", "ebuf_fifo_regs.int_stat", 5),
("EPB:   Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.egrNx_reg", "epb_disp_port_regs.int_stat", 5),
("EPB:   Interrupt Status",   0, 1, 1,   "pmarb.ebp18_reg.egrNx_reg", "epb_dprs_regs.int_stat", 5),

]

PKT_PATH_PER_MODULE_INTERRUPTS = [
("IDPRSR:Interrupt Status",             0, 1, 1,   "deparser", "inp.icr.intr.status", 5),
("IDPRSR:Interrupt Status",             0, 1, 1,   "deparser", "hdr.hir.ingr.intr.status", 5),
("IDPRSR:Interrupt Status",             0, 1, 1,   "deparser", "out_ingr.regs.intr.status", 5),
("EDPRSR:Interrupt Status",             0, 1, 1,   "deparser", "out_egr.regs.intr.status", 5),
]

class pkt_path_counter:

  def __init__(self, _cfile, _hfile, _api=False):
    self.counter_index = 0
    self.counter_string_table = []
    self.enum_table = []
    self._api = _api
    self._cfile = open(_cfile, "w+")
    self._hfile = open(_hfile, "w+")
    self._cfile.write(BF_CR_HEADER)
    self._hfile.write(BF_CR_HEADER)
    self._hfile.write(PKT_PATH_H_FILE_DEF)
    self._cfile.write(PKT_PATH_INCLUDES)
    self._cfile.write(HLINE)

  def pkt_path_hfile_blankline(self):
    self._hfile.write("\n")

  def pkt_path_hfile_codegen(self, cline, indent=0):
    for i in xrange (indent):
      self._hfile.write(" ")
    self._hfile.write(cline)
    self._hfile.write("\n")

  def pkt_path_blankline(self):
    self._cfile.write("\n")

  def pkt_path_codegen(self, cline, indent=0):
    for i in xrange (indent):
      self._cfile.write(" ")
    self._cfile.write(cline)
    self._cfile.write("\n")

  def pkt_path_codegen_newline(self, indent=0):
    if (not self._api):
      self.pkt_path_codegen("aim_printf(&uc->pvs, \"\\n\");", indent)
    self._cfile.write("\n")


  def pkt_path_print_table_header(self, column1, fieldcnt, fieldlist):
    if (self._api):
      return
    tblstr = "aim_printf(&uc->pvs, \"%-" + str(len(column1)+4) + "s\", " + "\"" + column1 + "\");"
    self.pkt_path_codegen(tblstr, 2)
    k = len(column1)
    for f in xrange(fieldcnt):
      k += len(fieldlist[f * 2]) + 4
      tblstr = "aim_printf(&uc->pvs, \"%-" + str((fieldlist[f * 2 + 1])) + "s\", " + "\"" + fieldlist[f * 2] + "\");"
      self.pkt_path_codegen(tblstr, 2)
    self.pkt_path_codegen_newline(2)
    tblstr = "aim_printf(&uc->pvs, \"%.*s\", " + str(k + 10) + ", " + "HLINE" +  ");"
    self.pkt_path_codegen(tblstr, 2)
    self.pkt_path_codegen_newline(2)


  def pkt_path_print_counter_entry(self, indent, counter_name, regname, val, u64):
    if (self._api):
      return
    pstr = "aim_printf(&uc->pvs, \"%-45"  + "s\", " + "\"" + counter_name + "\"" + ");"
    self.pkt_path_codegen(pstr, indent)
    hexcheck = "if (!hex) { "
    self.pkt_path_codegen(hexcheck, indent)
    if (u64 > 0):
      pstr = "aim_printf(&uc->pvs, \"%-30"  + "\" PRIu64, " + val + ");"
    else:
      pstr = "aim_printf(&uc->pvs, \"%-30"  + "u\", " + val + ");"
    self.pkt_path_codegen(pstr, indent+2)
    hexcheck = "} else { "
    self.pkt_path_codegen(hexcheck, indent)
    if (u64 > 0):
      pstr = "aim_printf(&uc->pvs, \"%-30"  + "\" PRIx64, " + val + ");"
    else:
      pstr = "aim_printf(&uc->pvs, \"%-30"  + "x\", " + val + ");"
    self.pkt_path_codegen(pstr, indent+2)
    hexcheck = "} "
    self.pkt_path_codegen(hexcheck, indent)
    pstr = "aim_printf(&uc->pvs, \"%-30"  + "s\", " + "\"" + regname + "\"" + ");"
    self.pkt_path_codegen(pstr, indent)




  def pkt_path_codegen_open_function(self, tname, argu):
    self.pkt_path_hfile_codegen("#if DVM_CONFIG_INCLUDE_UCLI == 1")
    cline = "extern void " + "pipe_mgr_tof_pkt_path_display_" + tname + argu + ";"
    self.pkt_path_hfile_codegen(cline)
    self.pkt_path_hfile_codegen("#endif")
    cline = "void " + "pipe_mgr_tof_pkt_path_display_" + tname + argu
    self.pkt_path_codegen(cline)
    cline = "{"
    self.pkt_path_codegen(cline)
    self.pkt_path_blankline()

  def pkt_path_codegen_open_api(self, tname, argu):
    cline = "extern bf_packetpath_counter_t* " + "pipe_mgr_tof_pkt_path_" + tname + argu + ";"
    self.pkt_path_hfile_codegen(cline)
    cline = "bf_packetpath_counter_t* " + "pipe_mgr_tof_pkt_path_" + tname + argu
    self.pkt_path_codegen(cline)
    cline = "{"
    self.pkt_path_codegen(cline)
    self.pkt_path_blankline()


  def pkt_path_codegen_open_clear_function(self, tname, argu):
    cline = "extern void " + "pipe_mgr_tof_pkt_path_clear_" + tname + argu + ";"
    self.pkt_path_hfile_codegen(cline)
    cline = "void " + "pipe_mgr_tof_pkt_path_clear_" + tname + argu
    self.pkt_path_codegen(cline)

    cline = "{"
    self.pkt_path_codegen(cline)
    self.pkt_path_blankline()

  def pkt_path_codegen_declare_variable(self, vtype, var):
    cline = vtype + " " + var + ";"
    self.pkt_path_codegen(cline, 2)


  def pkt_path_codegen_close_function(self, tname, newlines=1):
    if (newlines):
      self.pkt_path_codegen_newline(2)
      self.pkt_path_codegen_newline(2)
      self.pkt_path_codegen_newline(2)
    cline = "}"
    self.pkt_path_codegen(cline)
    self.pkt_path_blankline()

  def pkt_path_codegen_call_function(self, tname, argu):
    if (not self._api):
      cline = "pipe_mgr_tof_pkt_path_display_" + tname + argu + ";"
    else:
      cline = "pipe_mgr_tof_pkt_path_" + tname + argu + ";"

    self.pkt_path_codegen(cline, 2)
    self.pkt_path_blankline()

  def pkt_path_codegen_call_clear_function(self, tname, argu):
    cline = "pipe_mgr_tof_pkt_path_clear_" + tname + argu + ";"
    self.pkt_path_codegen(cline, 2)
    self.pkt_path_blankline()

  def pkt_path_generate_counter_enum(self):
    self.enum_table.append("BF_MAX_PKT_PATH_COUNTER_INDEX")
    self.pkt_path_hfile_blankline()
    self.pkt_path_hfile_codegen("typedef enum pipe_mgr_tof_pkt_path_counter_enum_ {", 0)
    for enum in self.enum_table:
      self.pkt_path_hfile_codegen(enum + "," ,   2)
    self.pkt_path_hfile_codegen("} pipe_mgr_tof_pkt_path_counter_en;", 0)
    self.pkt_path_hfile_blankline()

  def pkt_path_generate_counter_string_table(self):
    self.pkt_path_blankline()
    self.pkt_path_codegen("static const char* pipe_mgr_tof_pkt_path_counter_strings[] = {", 0)
    for counter_str in self.counter_string_table:
      self.pkt_path_codegen("\"" + counter_str + "\", ", 2)
    self.pkt_path_codegen("};", 0)
    self.pkt_path_blankline()

  def pkt_path_generate_counter_description_read_function(self, dtype="get_counter_description"):
    self._api = True
    indent = 0
    cline = "extern const char* " + "pipe_mgr_tof_pkt_path_" + dtype + "(uint32_t description_index)" + ";"
    self.pkt_path_hfile_codegen(cline)
    cline = "const char* " + "pipe_mgr_tof_pkt_path_" + dtype + "(uint32_t description_index)"
    self.pkt_path_codegen(cline)
    cline = "{"
    self.pkt_path_codegen(cline)
    self.pkt_path_blankline()
    self.pkt_path_codegen("if (description_index >= BF_MAX_PKT_PATH_COUNTER_INDEX) return NULL;", 2)
    self.pkt_path_codegen("return (pipe_mgr_tof_pkt_path_counter_strings[description_index]);", 2)
    self.pkt_path_codegen_close_function("", 0)



  def pkt_path_generate_per_pipe_per_port_read_counter_api(self, modulename, countertbl, dtype="read_counter"):
    self._api = True
    indent = 0
    self.pkt_path_codegen_open_api(modulename, "_" + dtype + "(int devid, int p, int port, int chan_numb, int *count)")
    self.pkt_path_codegen_declare_variable("uint32_t", "val, num_counters")
    self.pkt_path_codegen_declare_variable("int", "pg_port")
    self.pkt_path_codegen_declare_variable("bf_packetpath_counter_t", "*counters, *counter_head = NULL")
    self.pkt_path_codegen_declare_variable("uint64_t", "cnt = 0")
    self.pkt_path_codegen_declare_variable("uint64_t", "reg_cnt = 0")
    self.pkt_path_blankline()

    indent += 2

    total_counters = 0
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      total_counters += cdepth
    self.pkt_path_codegen("if (!count) return NULL;", indent)
    self.pkt_path_codegen("num_counters = " + str(total_counters) + ";", indent)
    self.pkt_path_codegen("counter_head = bf_sys_calloc((num_counters * chan_numb), sizeof(bf_packetpath_counter_t));", indent)
    self.pkt_path_codegen("if (!counter_head) return NULL;", indent)
    self.pkt_path_codegen("counters = counter_head;", indent)
    self.pkt_path_blankline()
    self.pkt_path_codegen("num_counters = 0;", indent)
    self.pkt_path_blankline()
    self.pkt_path_codegen("for (pg_port = port; pg_port < (port + chan_numb); pg_port++) {", indent)
    indent += 2 

    tablerow = 0
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      indent = 0
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (chnl):
          checkport = "if ((pg_port % 4) == " + str(tablerow) + ") {"
          tablerow = (tablerow + 1) % 4
          self.pkt_path_codegen(checkport, indent + 2)
          indent += 2
        if (cwidth > 1):
          regstr = "cnt = 0;"
          self.pkt_path_codegen(regstr, indent + 2)
          for i in xrange (cwidth):
            regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + reg_array + "." + regn[index] + "_" + str(i) + "_" + str(cwidth)
            regstr = regstr + "), &val);"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "reg_cnt = val;"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "cnt = (reg_cnt << " + str (i * 32) + ") + cnt;"
            self.pkt_path_codegen(regstr, indent + 2)
        else:
          regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), &val);"
          self.pkt_path_codegen(regstr, indent + 2)
          self.pkt_path_codegen("cnt = val;", indent + 2)
  
        if (cdepth > 1):
          counterstr = cname + str(cd)
        else:
          counterstr = cname

        self.counter_string_table.append(counterstr)
        enum_str = counterstr 
        enum_str = enum_str.upper()
        enum_str = enum_str.replace(":", "")
        enum_str = enum_str.replace(" ", "_")
        self.enum_table.append(enum_str)
        self.pkt_path_codegen("counters->description_index = " + str(self.counter_index) + ";", indent + 2)
        self.pkt_path_codegen("counters->counter_type = " + str(counter_type) + ";", indent + 2);
        self.pkt_path_codegen("counters->value = cnt;", indent + 2)
        self.counter_index += 1

        self.pkt_path_codegen("counters++;", indent + 2)
        self.pkt_path_codegen("num_counters++;", indent + 2)

        if (chnl):
          checkport = "}"
          self.pkt_path_codegen(checkport, indent)
        self.pkt_path_codegen_newline(indent + 2)

    self.pkt_path_codegen("}", 2)
    self.pkt_path_codegen("*count = num_counters;", 2)
    self.pkt_path_codegen("return counter_head;", 2)
    self.pkt_path_codegen_close_function(modulename, 0)

  def pkt_path_generate_per_pipe_module_read_counter_api(self, modulename, countertbl, counters=1, dtype="counter"):
    self._api = True
    indent = 0
    self.pkt_path_codegen_open_api(modulename, "_" + dtype + "(int devid, int p, int *count)")
    self.pkt_path_codegen_declare_variable("uint32_t", "val, num_counters")
    self.pkt_path_codegen_declare_variable("bf_packetpath_counter_t", "*counters, *counter_head = NULL")
    self.pkt_path_codegen_declare_variable("uint64_t", "cnt = 0")
    self.pkt_path_codegen_declare_variable("uint64_t", "reg_cnt = 0")

    self.pkt_path_blankline()

    indent += 2

    total_counters = 0
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      total_counters += cdepth

    self.pkt_path_codegen("num_counters = " + str(total_counters) + ";", indent)
    self.pkt_path_codegen("counter_head = bf_sys_calloc(num_counters, sizeof(bf_packetpath_counter_t));", indent)
    self.pkt_path_codegen("if (!counter_head) return NULL;", indent)
    self.pkt_path_codegen("counters = counter_head;", indent)
    self.pkt_path_blankline()
    self.pkt_path_codegen("num_counters = 0;", indent)
    self.pkt_path_blankline()

    for cname, chnl, cwidth, cdepth, regpath, regname , counter_type in countertbl:
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (cwidth > 1):
          regstr = "cnt = 0;"
          self.pkt_path_codegen(regstr, indent + 2)
          for i in xrange (cwidth):
            regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + "." + regn[index] + "_" + str(i) + "_" + str(cwidth)
            regstr = regstr + "), &val);"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "reg_cnt = val;"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "cnt = (reg_cnt << " + str (i * 32) + ") + cnt;"
            self.pkt_path_codegen(regstr, indent + 2)
        else:
          regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), &val);"
          self.pkt_path_codegen(regstr, indent + 2)
          self.pkt_path_codegen("cnt = val;", indent + 2)

        if (cdepth > 1):
          counterstr = cname + str(cd)
        else:
          counterstr = cname

        self.counter_string_table.append(counterstr)
        enum_str = counterstr 
        enum_str = enum_str.upper()
        enum_str = enum_str.replace(":", "")
        enum_str = enum_str.replace(" ", "_")
        self.enum_table.append(enum_str)
        self.pkt_path_codegen("counters->description_index = " + str(self.counter_index) + ";", indent + 2)
        self.pkt_path_codegen("counters->counter_type = " + str(counter_type) + ";", indent + 2);
        self.pkt_path_codegen("counters->value = cnt;", indent + 2)
        self.counter_index += 1

        self.pkt_path_codegen("counters++;", indent + 2)
        self.pkt_path_codegen("num_counters++;", indent + 2)
        self.pkt_path_codegen_newline(indent + 2)

    self.pkt_path_codegen("*count = num_counters;", 2)
    self.pkt_path_codegen("return counter_head;", 2)
    self.pkt_path_codegen_close_function(modulename, 0)



  def pkt_path_generate_per_pipe_per_port_module_regs(self, modulename, countertbl, counters=1, dtype="counter"):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_function(modulename, "_" + dtype + "(ucli_context_t *uc, int hex, int devid, int p, int pg_port)")
    self.pkt_path_codegen_declare_variable("uint32_t", "val")
    if (counters):
      self.pkt_path_codegen_declare_variable("uint64_t", "cnt = 0")
      self.pkt_path_codegen_declare_variable("uint64_t", "reg_cnt = 0")
    self.pkt_path_blankline()
    indent += 2
    s = modulename + "-" + dtype
    s1 = " " * (45 - len(s))
    s = s + s1
    self.pkt_path_print_table_header(s, 2, ("Count", 30, "Register Name", 20))
    tablerow = 0
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      indent = 0
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (chnl):
          checkport = "if ((pg_port % 4) == " + str(tablerow) + ") {"
          tablerow = (tablerow + 1) % 4
          self.pkt_path_codegen(checkport, indent + 2)
          indent += 2
        if (cwidth > 1):
          regstr = "cnt = 0;"
          self.pkt_path_codegen(regstr, indent + 2)
          for i in xrange (cwidth):
            regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + reg_array + "." + regn[index] + "_" + str(i) + "_" + str(cwidth)
            regstr = regstr + "), &val);"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "reg_cnt = val;"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "cnt = (reg_cnt << " + str (i * 32) + ") + cnt;"
            self.pkt_path_codegen(regstr, indent + 2)
          self.pkt_path_print_counter_entry(indent + 2, cname, regname+reg_array, "cnt", 1)
        else:
          regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), &val);"
          self.pkt_path_codegen(regstr, indent + 2)
          if (counters):
            self.pkt_path_codegen("(void)cnt;", indent + 2)
          self.pkt_path_print_counter_entry(indent + 2, cname, regname+reg_array, "val", 0)
        self.pkt_path_codegen_newline(indent+2)

        if (chnl):
          checkport = "}"
          self.pkt_path_codegen(checkport, indent)

    self.pkt_path_codegen_close_function(modulename)

  def pkt_path_generate_per_pipe_module_regs(self, modulename, countertbl, counters=1, dtype="counter"):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_function(modulename, "_" + dtype + "(ucli_context_t *uc, int hex, int devid, int p)")
    self.pkt_path_codegen_declare_variable("uint32_t", "val")
    if (counters):
      self.pkt_path_codegen_declare_variable("uint64_t", "cnt = 0")
      self.pkt_path_codegen_declare_variable("uint64_t", "reg_cnt = 0")
    s = modulename + "-" + dtype
    s1 = " " * (45 - len(s))
    s = s + s1
    self.pkt_path_print_table_header(s, 2, ("Count", 30, "Register Name", 20))
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (cwidth > 1):
          regstr = "cnt = 0;"
          self.pkt_path_codegen(regstr, indent + 2)
          for i in xrange (cwidth):
            regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + "." + regn[index] + "_" + str(i) + "_" + str(cwidth)
            regstr = regstr + "), &val);"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "reg_cnt = val;"
            self.pkt_path_codegen(regstr, indent + 2)
            regstr = "cnt = (reg_cnt << " + str (i * 32) + ") + cnt;"
            self.pkt_path_codegen(regstr, indent + 2)
          self.pkt_path_print_counter_entry(indent + 2, cname, regname+reg_array, "cnt", 1)
        else:
          regstr = "lld_read_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), &val);"
          self.pkt_path_codegen(regstr, indent + 2)
          if (counters):
            self.pkt_path_codegen("(void)cnt;", indent + 2)
          self.pkt_path_print_counter_entry(indent + 2, cname, regname+reg_array, "val", 0)
        self.pkt_path_codegen_newline(indent+2)

    self.pkt_path_codegen_close_function(modulename)



  def pkt_path_generate_per_pipe_counters(self):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_function("pipe", "_counter(ucli_context_t *uc, int hex, int devid, int p)")
    self.pkt_path_codegen_call_function("idprsr", "_counter(uc, hex, devid, p)")
    self.pkt_path_codegen_call_function("edprsr", "_counter(uc, hex, devid, p)")
    self.pkt_path_codegen_close_function("pipe")


  def pkt_path_clear_per_pipe_per_port_module_counters(self, modulename, countertbl):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_clear_function(modulename, "_counter(int devid, int p, int pg_port)")
    indent += 2
    tablerow = 0
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (chnl):
          checkport = "if ((pg_port % 4) == " + str(tablerow) + ") {"
          tablerow = (tablerow + 1) % 4
          self.pkt_path_codegen(checkport, indent + 2)
          indent += 2
        if (cwidth > 1):
          for i in xrange (cwidth):
            regstr = "lld_write_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + reg_array + "." + regn[index] + "_" + str(cwidth - i - 1) + "_" + str(cwidth)
            regstr = regstr + "), 0);"
            self.pkt_path_codegen(regstr, indent + 2)
        else:
          regstr = "lld_write_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "[" + "pg_port/4" + "]."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), 0);"
          self.pkt_path_codegen(regstr, indent + 2)
        if (chnl):
          checkport = "}"
          self.pkt_path_codegen(checkport, indent)

    self.pkt_path_codegen_close_function(modulename, 0)

  def pkt_path_clear_per_pipe_module_counters(self, modulename, countertbl):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_clear_function(modulename, "_counter(int devid, int p)")
    indent += 2
    for cname, chnl, cwidth, cdepth, regpath, regname, counter_type in countertbl:
      for cd in range(0, cdepth):
        indent = 0
        if (cdepth > 1):
           reg_array = "[" + str(cd) + "]"
        else:
           reg_array = ""
        if (cwidth > 1):
          for i in xrange (cwidth):
            regstr = "lld_write_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
            regn = regname.split(".")
            r = len(regn)
            index = r - 1
            regstr = regstr + regname + reg_array + "." + regn[index] + "_" + str(cwidth - i - 1) + "_" + str(cwidth)
            regstr = regstr + "), 0);"
            self.pkt_path_codegen(regstr, indent + 2)
        else:
          regstr = "lld_write_register(devid," + " offsetof(Tofino, pipes[p]." + regpath + "."
          regstr = regstr + regname + reg_array
          regstr = regstr + "), 0);"
          self.pkt_path_codegen(regstr, indent + 2)
    self.pkt_path_codegen_close_function(modulename, 0)

  def pkt_path_clear_per_pipe_counters(self):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_clear_function("pipe", "_counter(int devid, int p)")
    self.pkt_path_codegen_call_clear_function("idprsr", "_counter(devid, p)")
    self.pkt_path_codegen_call_clear_function("edprsr", "_counter(devid, p)")
    self.pkt_path_codegen_close_function("pipe", 0)

  def pkt_path_clear_all_counters(self):
    self._api = False
    indent = 0
    self.pkt_path_codegen_open_clear_function("all", "_counter(int devid, int p)")
    for i in xrange (72):
      self.pkt_path_codegen_call_clear_function("ibuf", "_counter(devid, p, " + str(i) + ")")
      self.pkt_path_codegen_call_clear_function("iprsr", "_counter(devid, p, " + str(i) + ")")
      self.pkt_path_codegen_call_clear_function("eprsr", "_counter(devid, p, " + str(i) + ")")
#self.pkt_path_codegen_call_clear_function("ebuf", "_counter(devid, p)")
    self.pkt_path_codegen_call_clear_function("idprsr", "_counter(devid, p)")
    self.pkt_path_codegen_call_clear_function("edprsr", "_counter(devid, p)")
    self.pkt_path_codegen_close_function("all", 0)


# Disable the script
print("Current generation script is outdated")
print("Do not try to force it for existing files re-generation!")
exit(0)

if __name__ == "__main__":
  pcounter = pkt_path_counter("./pipe_mgr_tof_pkt_path_counter_display.c", "./pipe_mgr_tof_pkt_path_counter_display.h", False)

  pcounter.pkt_path_codegen("#if DVM_CONFIG_INCLUDE_UCLI == 1")
  #Functions to read counters and display (uses printf)
  pcounter.pkt_path_generate_per_pipe_per_port_module_regs("ibuf", PKT_PATH_IBUF_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_module_regs("iprsr", PKT_PATH_IPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_module_regs("idprsr", PKT_PATH_IDPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_module_regs("ebuf", PKT_PATH_EPB_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_module_regs("eprsr", PKT_PATH_EPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_module_regs("edprsr", PKT_PATH_EDPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_counters()

  #Generate functions to display interrupts
  pcounter.pkt_path_generate_per_pipe_per_port_module_regs("interrupts", PKT_PATH_PER_PORT_INTERRUPTS, counters=0, dtype="intr")
  pcounter.pkt_path_generate_per_pipe_module_regs("dprsr_interrupt", PKT_PATH_PER_MODULE_INTERRUPTS, counters=0, dtype="intr")
  pcounter.pkt_path_codegen("#endif") # If ucli is enabled or not.
  #Generate functions to clear counters 
  pcounter.pkt_path_clear_per_pipe_per_port_module_counters("ibuf", PKT_PATH_IBUF_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_per_port_module_counters("iprsr", PKT_PATH_IPRSR_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_module_counters("idprsr", PKT_PATH_IDPRSR_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_per_port_module_counters("ebuf", PKT_PATH_EPB_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_per_port_module_counters("eprsr", PKT_PATH_EPRSR_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_module_counters("edprsr", PKT_PATH_EDPRSR_COUNTERS)
  pcounter.pkt_path_clear_per_pipe_counters()
  pcounter.pkt_path_clear_all_counters()
  # pcounter.pkt_path_codegen("#endif") # If ucli is enabled or not.

  #Generate functions to read counters; Counters are read into array of structures
  pcounter.pkt_path_generate_per_pipe_per_port_read_counter_api("ibuf", PKT_PATH_IBUF_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_read_counter_api("iprsr", PKT_PATH_IPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_module_read_counter_api("idprsr", PKT_PATH_IDPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_read_counter_api("ebuf", PKT_PATH_EPB_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_per_port_read_counter_api("eprsr", PKT_PATH_EPRSR_COUNTERS)
  pcounter.pkt_path_generate_per_pipe_module_read_counter_api("edprsr", PKT_PATH_EDPRSR_COUNTERS)

  # Note -- following 2 functions should be called after generating all
  # code/apis except string-index to description api .

  #Generate functions to create counter enums and counter string table.
  pcounter.pkt_path_generate_counter_enum()
  pcounter.pkt_path_generate_counter_string_table()

  #Generate functions to get counter description using counter-index
  pcounter.pkt_path_generate_counter_description_read_function()

  pcounter.pkt_path_hfile_codegen("#endif")
