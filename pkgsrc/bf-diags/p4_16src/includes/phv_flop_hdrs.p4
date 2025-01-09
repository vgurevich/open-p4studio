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



#ifndef _PHV_FLOP_HDRS_
#define _PHV_FLOP_HDRS_

#ifdef DIAG_PHV_FLOP_CONFIG_1
  #define DIAG_DUMMY_OUTPUT
#endif

//#define DIAG_H95_ENABLE

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

// We use this header in place of ethernet_t in the egress pipeline when
// DIAG_PHV_CONFIG_2 is in effect. It coerces the Ethernet header into
// fullword PHVs, freeing up halfword PHVs for testing.
header othernet_t {
    bit<32> x1;
    bit<32> x2;
    bit<32> x3;
    bit<32> x4;
}

header testdata_t {
    // Packet control field. Used internally by the diag framework.
    // Equivalent to the hdr.tcp.dstPort field in non-PHV tests.
    bit<16> pkt_ctrl;
}

#ifdef DIAG_PHV_FLOP_CONFIG_1
  @phv_limit(-T,-B16-31,-W16-31)
  // 48 fullword PHVs (16 untested)
  //   W0-15 = hdr.flopdata.w0-15
  //  *W16-31 = unused (egress)
  //   W32-63 = hdr.flopdata.w32-63
  // 64 halfword PHVs (32 untested)
  //   H0-H15 = hdr.flopdata.h0-15
  //  *H16 = egress::hdr.dummy.nothing
  //  *H17 = egress::hdr.dummy.$valid
  //  *H18-31 = unused (egress)
  //   H32-47 = hdr.flopdata.h32-47
  //   H48-63 = hdr.flopdata.h48-63
  //   H64-79 = hdr.flopdata.h64-79
  //  *H80-91 = overhead
  //     ig_intr_md_for_tm
  //     hdr.ethernet
  //     ig_intr_md
  //     hdr.testdata
  //     ingress::hdr POV
  //  *H92-95 = unused
  // 48 byte PHVs (16 untested)
  //   B0-15 = hdr.flopdata.b0-b15
  //  *B16-31 = unused (egress)
  //   B32-47 = hdr.flopdata.b32-47
  //   B48-63 = hdr.flopdata.b48-63
  // We suppress Tagalong PHVs (-T) to induce the compiler
  // to use regular PHVs for all our test data.
#else
  @phv_limit(-TB16-31,-TH16-31,-TW16-31,-H32-H79)
  // 16 fullword PHVs
  //   W0-W15 = flopdata.w16-31 (ingress)
  //   W16-W31 = flopdata.w16-31 (egress)
  //   W32-38 = overhead
  //   W39-63 = unused
  // 32 halfword PHVs
  //   H0-H15 = flopdata.h16-31 (ingress)
  //   H16-31 = flopdata.h16-31 (egress)
  //   H32-63 = unused (reserved)
  //   H64-79 = flopdata.h80-95 (ingress)
  //   H80-95 = flopdata.h80-95 (egress)
  // 16 byte PHVs
  //   B0-15 = flopdata.b16-31 (ingress)
  //   B16-31 = flopdata.b16-31 (egress)
  //   B32-48 = overhead
  //   B49-55 = unused
  //   B56 = overhead
  //   B57-63 = unused
  // The configuration has been tuned to induce the compiler
  // to use Tagalong PHVs on ingress but not on egress.
#endif

/************************/
/*  INGRESS CONTAINERS  */
/************************/

/* 32-BIT CONTAINERS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 1 */
  @pa_container_size("ingress", "hdr.flopdata.w0", 32)
  @pa_container_size("ingress", "hdr.flopdata.w1", 32)
  @pa_container_size("ingress", "hdr.flopdata.w2", 32)
  @pa_container_size("ingress", "hdr.flopdata.w3", 32)
  @pa_container_size("ingress", "hdr.flopdata.w4", 32)
  @pa_container_size("ingress", "hdr.flopdata.w5", 32)
  @pa_container_size("ingress", "hdr.flopdata.w6", 32)
  @pa_container_size("ingress", "hdr.flopdata.w7", 32)

  @pa_container_size("ingress", "hdr.flopdata.w8", 32)
  @pa_container_size("ingress", "hdr.flopdata.w9", 32)
  @pa_container_size("ingress", "hdr.flopdata.w10", 32)
  @pa_container_size("ingress", "hdr.flopdata.w11", 32)
  @pa_container_size("ingress", "hdr.flopdata.w12", 32)
  @pa_container_size("ingress", "hdr.flopdata.w13", 32)
  @pa_container_size("ingress", "hdr.flopdata.w14", 32)
  @pa_container_size("ingress", "hdr.flopdata.w15", 32)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
  /* group 2 */
  @pa_container_size("ingress", "hdr.flopdata.w16", 32)
  @pa_container_size("ingress", "hdr.flopdata.w17", 32)
  @pa_container_size("ingress", "hdr.flopdata.w18", 32)
  @pa_container_size("ingress", "hdr.flopdata.w19", 32)
  @pa_container_size("ingress", "hdr.flopdata.w20", 32)
  @pa_container_size("ingress", "hdr.flopdata.w21", 32)
  @pa_container_size("ingress", "hdr.flopdata.w22", 32)
  @pa_container_size("ingress", "hdr.flopdata.w23", 32)

  @pa_container_size("ingress", "hdr.flopdata.w24", 32)
  @pa_container_size("ingress", "hdr.flopdata.w25", 32)
  @pa_container_size("ingress", "hdr.flopdata.w26", 32)
  @pa_container_size("ingress", "hdr.flopdata.w27", 32)
  @pa_container_size("ingress", "hdr.flopdata.w28", 32)
  @pa_container_size("ingress", "hdr.flopdata.w29", 32)
  @pa_container_size("ingress", "hdr.flopdata.w30", 32)
  @pa_container_size("ingress", "hdr.flopdata.w31", 32)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 3 */
  @pa_container_size("ingress", "hdr.flopdata.w32", 32)
  @pa_container_size("ingress", "hdr.flopdata.w33", 32)
  @pa_container_size("ingress", "hdr.flopdata.w34", 32)
  @pa_container_size("ingress", "hdr.flopdata.w35", 32)
  @pa_container_size("ingress", "hdr.flopdata.w36", 32)
  @pa_container_size("ingress", "hdr.flopdata.w37", 32)
  @pa_container_size("ingress", "hdr.flopdata.w38", 32)
  @pa_container_size("ingress", "hdr.flopdata.w39", 32)

  @pa_container_size("ingress", "hdr.flopdata.w40", 32)
  @pa_container_size("ingress", "hdr.flopdata.w41", 32)
  @pa_container_size("ingress", "hdr.flopdata.w42", 32)
  @pa_container_size("ingress", "hdr.flopdata.w43", 32)
  @pa_container_size("ingress", "hdr.flopdata.w44", 32)
  @pa_container_size("ingress", "hdr.flopdata.w45", 32)
  @pa_container_size("ingress", "hdr.flopdata.w46", 32)
  @pa_container_size("ingress", "hdr.flopdata.w47", 32)

  /* group 4 */
  @pa_container_size("ingress", "hdr.flopdata.w48", 32)
  @pa_container_size("ingress", "hdr.flopdata.w49", 32)
  @pa_container_size("ingress", "hdr.flopdata.w50", 32)
  @pa_container_size("ingress", "hdr.flopdata.w51", 32)
  @pa_container_size("ingress", "hdr.flopdata.w52", 32)
  @pa_container_size("ingress", "hdr.flopdata.w53", 32)
  @pa_container_size("ingress", "hdr.flopdata.w54", 32)
  @pa_container_size("ingress", "hdr.flopdata.w55", 32)

  @pa_container_size("ingress", "hdr.flopdata.w56", 32)
  @pa_container_size("ingress", "hdr.flopdata.w57", 32)
  @pa_container_size("ingress", "hdr.flopdata.w58", 32)
  @pa_container_size("ingress", "hdr.flopdata.w59", 32)
  @pa_container_size("ingress", "hdr.flopdata.w60", 32)
  @pa_container_size("ingress", "hdr.flopdata.w61", 32)
  @pa_container_size("ingress", "hdr.flopdata.w62", 32)
  @pa_container_size("ingress", "hdr.flopdata.w63", 32)
#endif


/* 16-BIT CONTAINERS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 1 */
  @pa_container_size("ingress", "hdr.flopdata.h0", 16)
  @pa_container_size("ingress", "hdr.flopdata.h1", 16)
  @pa_container_size("ingress", "hdr.flopdata.h2", 16)
  @pa_container_size("ingress", "hdr.flopdata.h3", 16)
  @pa_container_size("ingress", "hdr.flopdata.h4", 16)
  @pa_container_size("ingress", "hdr.flopdata.h5", 16)
  @pa_container_size("ingress", "hdr.flopdata.h6", 16)
  @pa_container_size("ingress", "hdr.flopdata.h7", 16)

  @pa_container_size("ingress", "hdr.flopdata.h8", 16)
  @pa_container_size("ingress", "hdr.flopdata.h9", 16)
  @pa_container_size("ingress", "hdr.flopdata.h10", 16)
  @pa_container_size("ingress", "hdr.flopdata.h11", 16)
  @pa_container_size("ingress", "hdr.flopdata.h12", 16)
  @pa_container_size("ingress", "hdr.flopdata.h13", 16)
  @pa_container_size("ingress", "hdr.flopdata.h14", 16)
  @pa_container_size("ingress", "hdr.flopdata.h15", 16)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
  /* group 2 */
  @pa_container_size("ingress", "hdr.flopdata.h16", 16)
  @pa_container_size("ingress", "hdr.flopdata.h17", 16)
  @pa_container_size("ingress", "hdr.flopdata.h18", 16)
  @pa_container_size("ingress", "hdr.flopdata.h19", 16)
  @pa_container_size("ingress", "hdr.flopdata.h20", 16)
  @pa_container_size("ingress", "hdr.flopdata.h21", 16)
  @pa_container_size("ingress", "hdr.flopdata.h22", 16)
  @pa_container_size("ingress", "hdr.flopdata.h23", 16)

  @pa_container_size("ingress", "hdr.flopdata.h24", 16)
  @pa_container_size("ingress", "hdr.flopdata.h25", 16)
  @pa_container_size("ingress", "hdr.flopdata.h26", 16)
  @pa_container_size("ingress", "hdr.flopdata.h27", 16)
  @pa_container_size("ingress", "hdr.flopdata.h28", 16)
  @pa_container_size("ingress", "hdr.flopdata.h29", 16)
  @pa_container_size("ingress", "hdr.flopdata.h30", 16)
  @pa_container_size("ingress", "hdr.flopdata.h31", 16)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 3 */
  @pa_container_size("ingress", "hdr.flopdata.h32", 16)
  @pa_container_size("ingress", "hdr.flopdata.h33", 16)
  @pa_container_size("ingress", "hdr.flopdata.h34", 16)
  @pa_container_size("ingress", "hdr.flopdata.h35", 16)
  @pa_container_size("ingress", "hdr.flopdata.h36", 16)
  @pa_container_size("ingress", "hdr.flopdata.h37", 16)
  @pa_container_size("ingress", "hdr.flopdata.h38", 16)
  @pa_container_size("ingress", "hdr.flopdata.h39", 16)

  @pa_container_size("ingress", "hdr.flopdata.h40", 16)
  @pa_container_size("ingress", "hdr.flopdata.h41", 16)
  @pa_container_size("ingress", "hdr.flopdata.h42", 16)
  @pa_container_size("ingress", "hdr.flopdata.h43", 16)
  @pa_container_size("ingress", "hdr.flopdata.h44", 16)
  @pa_container_size("ingress", "hdr.flopdata.h45", 16)
  @pa_container_size("ingress", "hdr.flopdata.h46", 16)
  @pa_container_size("ingress", "hdr.flopdata.h47", 16)

  /* group 4 */
  @pa_container_size("ingress", "hdr.flopdata.h48", 16)
  @pa_container_size("ingress", "hdr.flopdata.h49", 16)
  @pa_container_size("ingress", "hdr.flopdata.h50", 16)
  @pa_container_size("ingress", "hdr.flopdata.h51", 16)
  @pa_container_size("ingress", "hdr.flopdata.h52", 16)
  @pa_container_size("ingress", "hdr.flopdata.h53", 16)
  @pa_container_size("ingress", "hdr.flopdata.h54", 16)
  @pa_container_size("ingress", "hdr.flopdata.h55", 16)

  @pa_container_size("ingress", "hdr.flopdata.h56", 16)
  @pa_container_size("ingress", "hdr.flopdata.h57", 16)
  @pa_container_size("ingress", "hdr.flopdata.h58", 16)
  @pa_container_size("ingress", "hdr.flopdata.h59", 16)
  @pa_container_size("ingress", "hdr.flopdata.h60", 16)
  @pa_container_size("ingress", "hdr.flopdata.h61", 16)
  @pa_container_size("ingress", "hdr.flopdata.h62", 16)
  @pa_container_size("ingress", "hdr.flopdata.h63", 16)

  /* group 5 */
  @pa_container_size("ingress", "hdr.flopdata.h64", 16)
  @pa_container_size("ingress", "hdr.flopdata.h65", 16)
  @pa_container_size("ingress", "hdr.flopdata.h66", 16)
  @pa_container_size("ingress", "hdr.flopdata.h67", 16)
  @pa_container_size("ingress", "hdr.flopdata.h68", 16)
  @pa_container_size("ingress", "hdr.flopdata.h69", 16)
  @pa_container_size("ingress", "hdr.flopdata.h70", 16)
  @pa_container_size("ingress", "hdr.flopdata.h71", 16)

  @pa_container_size("ingress", "hdr.flopdata.h72", 16)
  @pa_container_size("ingress", "hdr.flopdata.h73", 16)
  @pa_container_size("ingress", "hdr.flopdata.h74", 16)
  @pa_container_size("ingress", "hdr.flopdata.h75", 16)
  @pa_container_size("ingress", "hdr.flopdata.h76", 16)
  @pa_container_size("ingress", "hdr.flopdata.h77", 16)
  @pa_container_size("ingress", "hdr.flopdata.h78", 16)
  @pa_container_size("ingress", "hdr.flopdata.h79", 16)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
  /* group 6 */
  @pa_container_size("ingress", "hdr.flopdata.h80", 16)
  @pa_container_size("ingress", "hdr.flopdata.h81", 16)
  @pa_container_size("ingress", "hdr.flopdata.h82", 16)
  @pa_container_size("ingress", "hdr.flopdata.h83", 16)
  @pa_container_size("ingress", "hdr.flopdata.h84", 16)
  @pa_container_size("ingress", "hdr.flopdata.h85", 16)
  @pa_container_size("ingress", "hdr.flopdata.h86", 16)
  @pa_container_size("ingress", "hdr.flopdata.h87", 16)

  @pa_container_size("ingress", "hdr.flopdata.h88", 16)
  @pa_container_size("ingress", "hdr.flopdata.h89", 16)
  @pa_container_size("ingress", "hdr.flopdata.h90", 16)
  @pa_container_size("ingress", "hdr.flopdata.h91", 16)
  @pa_container_size("ingress", "hdr.flopdata.h92", 16)
  @pa_container_size("ingress", "hdr.flopdata.h93", 16)
  @pa_container_size("ingress", "hdr.flopdata.h94", 16)
#ifdef DIAG_H95_ENABLE
  @pa_container_size("ingress", "hdr.flopdata.h95", 16)
#endif
#endif

/* 8-BIT CONTAINERS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 1 */
  @pa_container_size("ingress", "hdr.flopdata.b0", 8)
  @pa_container_size("ingress", "hdr.flopdata.b1", 8)
  @pa_container_size("ingress", "hdr.flopdata.b2", 8)
  @pa_container_size("ingress", "hdr.flopdata.b3", 8)
  @pa_container_size("ingress", "hdr.flopdata.b4", 8)
  @pa_container_size("ingress", "hdr.flopdata.b5", 8)
  @pa_container_size("ingress", "hdr.flopdata.b6", 8)
  @pa_container_size("ingress", "hdr.flopdata.b7", 8)

  @pa_container_size("ingress", "hdr.flopdata.b8", 8)
  @pa_container_size("ingress", "hdr.flopdata.b9", 8)
  @pa_container_size("ingress", "hdr.flopdata.b10", 8)
  @pa_container_size("ingress", "hdr.flopdata.b11", 8)
  @pa_container_size("ingress", "hdr.flopdata.b12", 8)
  @pa_container_size("ingress", "hdr.flopdata.b13", 8)
  @pa_container_size("ingress", "hdr.flopdata.b14", 8)
  @pa_container_size("ingress", "hdr.flopdata.b15", 8)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
  /* group 2 */
  @pa_container_size("ingress", "hdr.flopdata.b16", 32)
  @pa_container_size("ingress", "hdr.flopdata.b17", 32)
  @pa_container_size("ingress", "hdr.flopdata.b18", 32)
  @pa_container_size("ingress", "hdr.flopdata.b19", 32)
  @pa_container_size("ingress", "hdr.flopdata.b20", 32)
  @pa_container_size("ingress", "hdr.flopdata.b21", 32)
  @pa_container_size("ingress", "hdr.flopdata.b22", 32)
  @pa_container_size("ingress", "hdr.flopdata.b23", 32)

  @pa_container_size("ingress", "hdr.flopdata.b24", 32)
  @pa_container_size("ingress", "hdr.flopdata.b25", 32)
  @pa_container_size("ingress", "hdr.flopdata.b26", 32)
  @pa_container_size("ingress", "hdr.flopdata.b27", 32)
  @pa_container_size("ingress", "hdr.flopdata.b28", 32)
  @pa_container_size("ingress", "hdr.flopdata.b29", 32)
  @pa_container_size("ingress", "hdr.flopdata.b30", 32)
  @pa_container_size("ingress", "hdr.flopdata.b31", 32)
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
  /* group 3 */
  @pa_container_size("ingress", "hdr.flopdata.b32", 8)
  @pa_container_size("ingress", "hdr.flopdata.b33", 8)
  @pa_container_size("ingress", "hdr.flopdata.b34", 8)
  @pa_container_size("ingress", "hdr.flopdata.b35", 8)
  @pa_container_size("ingress", "hdr.flopdata.b36", 8)
  @pa_container_size("ingress", "hdr.flopdata.b37", 8)
  @pa_container_size("ingress", "hdr.flopdata.b38", 8)
  @pa_container_size("ingress", "hdr.flopdata.b39", 8)

  @pa_container_size("ingress", "hdr.flopdata.b40", 8)
  @pa_container_size("ingress", "hdr.flopdata.b41", 8)
  @pa_container_size("ingress", "hdr.flopdata.b42", 8)
  @pa_container_size("ingress", "hdr.flopdata.b43", 8)
  @pa_container_size("ingress", "hdr.flopdata.b44", 8)
  @pa_container_size("ingress", "hdr.flopdata.b45", 8)
  @pa_container_size("ingress", "hdr.flopdata.b46", 8)
  @pa_container_size("ingress", "hdr.flopdata.b47", 8)

  /* group 4 */
  @pa_container_size("ingress", "hdr.flopdata.b48", 8)
  @pa_container_size("ingress", "hdr.flopdata.b49", 8)
  @pa_container_size("ingress", "hdr.flopdata.b50", 8)
  @pa_container_size("ingress", "hdr.flopdata.b51", 8)
  @pa_container_size("ingress", "hdr.flopdata.b52", 8)
  @pa_container_size("ingress", "hdr.flopdata.b53", 8)
  @pa_container_size("ingress", "hdr.flopdata.b54", 8)
  @pa_container_size("ingress", "hdr.flopdata.b55", 8)

  @pa_container_size("ingress", "hdr.flopdata.b56", 8)
  @pa_container_size("ingress", "hdr.flopdata.b57", 8)
  @pa_container_size("ingress", "hdr.flopdata.b58", 8)
  @pa_container_size("ingress", "hdr.flopdata.b59", 8)
  @pa_container_size("ingress", "hdr.flopdata.b60", 8)
  @pa_container_size("ingress", "hdr.flopdata.b61", 8)
  @pa_container_size("ingress", "hdr.flopdata.b62", 8)
  @pa_container_size("ingress", "hdr.flopdata.b63", 8)
#endif

/***********************/
/*  EGRESS CONTAINERS  */
/***********************/

#ifdef DIAG_PHV_FLOP_CONFIG_2
  /* word group 2 */
  @pa_container_size("egress", "hdr.flopdata.w16", 32)
  @pa_container_size("egress", "hdr.flopdata.w17", 32)
  @pa_container_size("egress", "hdr.flopdata.w18", 32)
  @pa_container_size("egress", "hdr.flopdata.w19", 32)
  @pa_container_size("egress", "hdr.flopdata.w20", 32)
  @pa_container_size("egress", "hdr.flopdata.w21", 32)
  @pa_container_size("egress", "hdr.flopdata.w22", 32)
  @pa_container_size("egress", "hdr.flopdata.w23", 32)

  @pa_container_size("egress", "hdr.flopdata.w24", 32)
  @pa_container_size("egress", "hdr.flopdata.w25", 32)
  @pa_container_size("egress", "hdr.flopdata.w26", 32)
  @pa_container_size("egress", "hdr.flopdata.w27", 32)
  @pa_container_size("egress", "hdr.flopdata.w28", 32)
  @pa_container_size("egress", "hdr.flopdata.w29", 32)
  @pa_container_size("egress", "hdr.flopdata.w30", 32)
  @pa_container_size("egress", "hdr.flopdata.w31", 32)

  /* halfword group 2 */
  @pa_container_size("egress", "hdr.flopdata.h16", 16)
  @pa_container_size("egress", "hdr.flopdata.h17", 16)
  @pa_container_size("egress", "hdr.flopdata.h18", 16)
  @pa_container_size("egress", "hdr.flopdata.h19", 16)
  @pa_container_size("egress", "hdr.flopdata.h20", 16)
  @pa_container_size("egress", "hdr.flopdata.h21", 16)
  @pa_container_size("egress", "hdr.flopdata.h22", 16)
  @pa_container_size("egress", "hdr.flopdata.h23", 16)

  @pa_container_size("egress", "hdr.flopdata.h24", 16)
  @pa_container_size("egress", "hdr.flopdata.h25", 16)
  @pa_container_size("egress", "hdr.flopdata.h26", 16)
  @pa_container_size("egress", "hdr.flopdata.h27", 16)
  @pa_container_size("egress", "hdr.flopdata.h28", 16)
  @pa_container_size("egress", "hdr.flopdata.h29", 16)
  @pa_container_size("egress", "hdr.flopdata.h30", 16)
  @pa_container_size("egress", "hdr.flopdata.h31", 16)

  /* byte group 2 */
  @pa_container_size("egress", "hdr.flopdata.b16", 8)
  @pa_container_size("egress", "hdr.flopdata.b17", 8)
  @pa_container_size("egress", "hdr.flopdata.b18", 8)
  @pa_container_size("egress", "hdr.flopdata.b19", 8)
  @pa_container_size("egress", "hdr.flopdata.b20", 8)
  @pa_container_size("egress", "hdr.flopdata.b21", 8)
  @pa_container_size("egress", "hdr.flopdata.b22", 8)
  @pa_container_size("egress", "hdr.flopdata.b23", 8)

  @pa_container_size("egress", "hdr.flopdata.b24", 8)
  @pa_container_size("egress", "hdr.flopdata.b25", 8)
  @pa_container_size("egress", "hdr.flopdata.b26", 8)
  @pa_container_size("egress", "hdr.flopdata.b27", 8)
  @pa_container_size("egress", "hdr.flopdata.b28", 8)
  @pa_container_size("egress", "hdr.flopdata.b29", 8)
  @pa_container_size("egress", "hdr.flopdata.b30", 8)
  @pa_container_size("egress", "hdr.flopdata.b31", 8)

  /* group 6 */
  @pa_container_size("egress", "hdr.flopdata.h80", 16)
  @pa_container_size("egress", "hdr.flopdata.h81", 16)
  @pa_container_size("egress", "hdr.flopdata.h82", 16)
  @pa_container_size("egress", "hdr.flopdata.h83", 16)
  @pa_container_size("egress", "hdr.flopdata.h84", 16)
  @pa_container_size("egress", "hdr.flopdata.h85", 16)
  @pa_container_size("egress", "hdr.flopdata.h86", 16)
  @pa_container_size("egress", "hdr.flopdata.h87", 16)

  @pa_container_size("egress", "hdr.flopdata.h88", 16)
  @pa_container_size("egress", "hdr.flopdata.h89", 16)
  @pa_container_size("egress", "hdr.flopdata.h90", 16)
  @pa_container_size("egress", "hdr.flopdata.h91", 16)
  @pa_container_size("egress", "hdr.flopdata.h92", 16)
  @pa_container_size("egress", "hdr.flopdata.h93", 16)
  @pa_container_size("egress", "hdr.flopdata.h94", 16)
#ifdef DIAG_H95_ENABLE
  @pa_container_size("egress", "hdr.flopdata.h95", 16)
#endif
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
  @pa_container_size("ingress", "ig_intr_md.ingress_port", 16)
#else
  @pa_container_size("ingress", "ig_intr_md.ingress_port", 32)
  @pa_container_size("ingress", "ig_intr_md_for_tm.ucast_egress_port", 32)
  @pa_container_size("ingress", "hdr.ethernet.$valid", 32)
  @pa_container_size("egress", "hdr.othernet.$valid", 32)
#endif

header phv_flop_hdr_t {

/* 32-BIT FIELDS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 1 */
    bit<32> w0;
    bit<32> w1;
    bit<32> w2;
    bit<32> w3;
    bit<32> w4;
    bit<32> w5;
    bit<32> w6;
    bit<32> w7;

    bit<32> w8;
    bit<32> w9;
    bit<32> w10;
    bit<32> w11;
    bit<32> w12;
    bit<32> w13;
    bit<32> w14;
    bit<32> w15;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
    /* group 2 */
    bit<32> w16;
    bit<32> w17;
    bit<32> w18;
    bit<32> w19;
    bit<32> w20;
    bit<32> w21;
    bit<32> w22;
    bit<32> w23;

    bit<32> w24;
    bit<32> w25;
    bit<32> w26;
    bit<32> w27;
    bit<32> w28;
    bit<32> w29;
    bit<32> w30;
    bit<32> w31;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 3 */
    bit<32> w32;
    bit<32> w33;
    bit<32> w34;
    bit<32> w35;
    bit<32> w36;
    bit<32> w37;
    bit<32> w38;
    bit<32> w39;

    bit<32> w40;
    bit<32> w41;
    bit<32> w42;
    bit<32> w43;
    bit<32> w44;
    bit<32> w45;
    bit<32> w46;
    bit<32> w47;

    /* group 4 */
    bit<32> w48;
    bit<32> w49;
    bit<32> w50;
    bit<32> w51;
    bit<32> w52;
    bit<32> w53;
    bit<32> w54;
    bit<32> w55;

    bit<32> w56;
    bit<32> w57;
    bit<32> w58;
    bit<32> w59;
    bit<32> w60;
    bit<32> w61;
    bit<32> w62;
    bit<32> w63;
#endif

/* 16-BIT FIELDS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 1 */
    bit<16> h0;
    bit<16> h1;
    bit<16> h2;
    bit<16> h3;
    bit<16> h4;
    bit<16> h5;
    bit<16> h6;
    bit<16> h7;

    bit<16> h8;
    bit<16> h9;
    bit<16> h10;
    bit<16> h11;
    bit<16> h12;
    bit<16> h13;
    bit<16> h14;
    bit<16> h15;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
    /* group 2 */
    bit<16> h16;
    bit<16> h17;
    bit<16> h18;
    bit<16> h19;
    bit<16> h20;
    bit<16> h21;
    bit<16> h22;
    bit<16> h23;

    bit<16> h24;
    bit<16> h25;
    bit<16> h26;
    bit<16> h27;
    bit<16> h28;
    bit<16> h29;
    bit<16> h30;
    bit<16> h31;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 3 */
    bit<16> h32;
    bit<16> h33;
    bit<16> h34;
    bit<16> h35;
    bit<16> h36;
    bit<16> h37;
    bit<16> h38;
    bit<16> h39;

    bit<16> h40;
    bit<16> h41;
    bit<16> h42;
    bit<16> h43;
    bit<16> h44;
    bit<16> h45;
    bit<16> h46;
    bit<16> h47;

    /* group 4 */
    bit<16> h48;
    bit<16> h49;
    bit<16> h50;
    bit<16> h51;
    bit<16> h52;
    bit<16> h53;
    bit<16> h54;
    bit<16> h55;

    bit<16> h56;
    bit<16> h57;
    bit<16> h58;
    bit<16> h59;
    bit<16> h60;
    bit<16> h61;
    bit<16> h62;
    bit<16> h63;

    /* group 5 */
    bit<16> h64;
    bit<16> h65;
    bit<16> h66;
    bit<16> h67;
    bit<16> h68;
    bit<16> h69;
    bit<16> h70;
    bit<16> h71;

    bit<16> h72;
    bit<16> h73;
    bit<16> h74;
    bit<16> h75;
    bit<16> h76;
    bit<16> h77;
    bit<16> h78;
    bit<16> h79;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
    /* group 6 */
    bit<16> h80;
    bit<16> h81;
    bit<16> h82;
    bit<16> h83;
    bit<16> h84;
    bit<16> h85;
    bit<16> h86;
    bit<16> h87;

    bit<16> h88;
    bit<16> h89;
    bit<16> h90;
    bit<16> h91;
    bit<16> h92;
    bit<16> h93;
    bit<16> h94;
#ifdef DIAG_H95_ENABLE
    bit<16> h95;
#endif
#endif

/* 8-BIT FIELDS */

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 1 */
    bit<8> b0;
    bit<8> b1;
    bit<8> b2;
    bit<8> b3;
    bit<8> b4;
    bit<8> b5;
    bit<8> b6;
    bit<8> b7;

    bit<8> b8;
    bit<8> b9;
    bit<8> b10;
    bit<8> b11;
    bit<8> b12;
    bit<8> b13;
    bit<8> b14;
    bit<8> b15;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_2
    /* group 2 */
    bit<8> b16;
    bit<8> b17;
    bit<8> b18;
    bit<8> b19;
    bit<8> b20;
    bit<8> b21;
    bit<8> b22;
    bit<8> b23;

    bit<8> b24;
    bit<8> b25;
    bit<8> b26;
    bit<8> b27;
    bit<8> b28;
    bit<8> b29;
    bit<8> b30;
    bit<8> b31;
#endif

#ifdef DIAG_PHV_FLOP_CONFIG_1
    /* group 3 */
    bit<8> b32;
    bit<8> b33;
    bit<8> b34;
    bit<8> b35;
    bit<8> b36;
    bit<8> b37;
    bit<8> b38;
    bit<8> b39;

    bit<8> b40;
    bit<8> b41;
    bit<8> b42;
    bit<8> b43;
    bit<8> b44;
    bit<8> b45;
    bit<8> b46;
    bit<8> b47;

    /* group 4 */
    bit<8> b48;
    bit<8> b49;
    bit<8> b50;
    bit<8> b51;
    bit<8> b52;
    bit<8> b53;
    bit<8> b54;
    bit<8> b55;

    bit<8> b56;
    bit<8> b57;
    bit<8> b58;
    bit<8> b59;
    bit<8> b60;
    bit<8> b61;
    bit<8> b62;
    bit<8> b63;
#endif

} // phv_flop_hdr_t

struct header_t {
    @name(".ethernet") ethernet_t ethernet;
    @name(".othernet") othernet_t othernet;
    @name(".testdata") testdata_t testdata;
    @name(".flopdata") phv_flop_hdr_t flopdata;
}

#ifdef DIAG_DUMMY_OUTPUT
// The p4_pd_diag_eg_snapshot_trig_spec structure is based on the output
// of the Egress stage. We must appear to output something, even if we
// always bypass the Egress stage, or the structure will be empty and
// we'll get a bunch of compiler errors.
//
// This is an effective no-op because the dummy header is never valid.
// It consumes a PHV; therefore we suppress it when we can.
header dummy_t {
#ifdef DIAG_PHV_FLOP_CONFIG_1
    bit<16> nothing;
#else
    bit<32> nothing;
#endif
}
#endif

struct empty_header_t {
#ifdef DIAG_DUMMY_OUTPUT
    dummy_t dummy;
#endif
}

struct empty_metadata_t {}

#endif // _PHV_FLOP_HDRS_
