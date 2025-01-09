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


#include <t2na.p4>
#include "includes/defines.h"
#include "includes/p4_table_sizes.h"

#ifndef TOFINO2M
#define LAST_STAGE 19
#else
#define LAST_STAGE 11
#endif

/* Tofino-2 divides the 280 PHVs first by width (32, 16, or 8 bits) and then,
 * within a given width, by group.  Each group contains 12 normal containers,
 * four mocha containers, and four dark containers.  There are four groups of
 * 32-bit containers, four groups of 8-bit containers, and six groups of 16-bit
 * containers.
 * The mocha and dark PHVs will be tested half on ingress and half on egress
 * (each gress will get two 32-bit groups, two 8-bit groups, and three 16-bit
 * groups).  This results in eight 32-bit containers, eight 8-bit containers and
 * 12 16-bit containers needed to cover the mocha or dark PHVs assigned to a
 * gress.  The following header/structure represents that set of containers and
 * will be instantiated in various places to test the PHV data path. */
header phv_group_h {
  bit<32> w0;
  bit<8>  clot_a;
  bit<32> w1;
  bit<8>  clot_b;
  bit<32> w2;
  bit<32> w3;
  bit<32> w4;
  bit<32> w5;
  bit<32> w6;
  bit<32> w7;
  bit<8>  clot_c;

  bit<8>  b0;
  bit<8>  b1;
  bit<8>  b2;
  bit<8>  clot_d;
  bit<8>  b3;
  bit<8>  b4;
  bit<8>  b5;
  bit<8>  b6;
  bit<8>  b7;

  bit<16> h0;
  bit<16> h1;
  bit<16> h2;
  bit<8>  clot_e;
  bit<16> h3;
  bit<16> h4;
  bit<16> h5;
  bit<16> h6;
  bit<16> h7;
  bit<8>  clot_f;
  bit<16> h8;
  bit<16> h9;
  bit<8>  clot_g;
  bit<16> h10;
  bit<16> h11;
}
struct phv_group_t {
  bit<32> w0;
  bit<32> w1;
  bit<32> w2;
  bit<32> w3;
  bit<32> w4;
  bit<32> w5;
  bit<32> w6;
  bit<32> w7;

  bit<8>  b0;
  bit<8>  b1;
  bit<8>  b2;
  bit<8>  b3;
  bit<8>  b4;
  bit<8>  b5;
  bit<8>  b6;
  bit<8>  b7;

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
}

/* For compability with the control plane we define a specifically named header
 * with a 16-bit field.  This will be used in a match table to control the
 * packet's forwarding destination.  It will also have a 14B padding field
 * representing the Ethernet header added by the control plane. */
header test_data_t {
  bit<(14*8)> ethernet;
  bit<16> pkt_ctrl;
}

header dummy_h {
  bit<32> f;
}


/* Ingress headers:
 *  16B test_data_t
 *  71B phv_group_h (64B plus 7B of clot fields)
 *  71B phv_group_h
 * 158B Total */
struct i_header_t {
  test_data_t itestdata;

  /* We will parse packet data into a group of mocha containers, carry it
   * through the pipeline, and deparse it.  This data should not change so the
   * input packet should equal the output packet. */
  phv_group_h imocha;

  /* Since dark containers are only available within the MAU pipeline (not from
   * the parser or deparser) we will parse 64B of packet into normal containers
   * and use them to initialize a set of dark containers in the first stage.
   * The last stage will reset the normal containers with the values from the
   * dark containers in the last stage so the deparser can put the fields back
   * into the packet.
   * This "dark_proxy" header represents those normal containers. */
  phv_group_h idark_proxy;
}

/* Egress headers:
 *  16B test_data_t
 *  71B phv_group_h
 *  71B phv_group_h
 *   4B dummy_h (Ignored, never parsed/deparsed)
 * 158B Total */
struct e_header_t {
  test_data_t etestdata;

  /* We will parse packet data into a group of mocha containers, carry it
   * through the pipeline, and deparse it.  This data should not change so the
   * input packet should equal the output packet. */
  phv_group_h emocha;

  /* Since dark containers are only available within the MAU pipeline (not from
   * the parser or deparser) we will parse 64B of packet into normal containers
   * and use them to initialize a set of dark containers in the first stage.
   * The last stage will reset the normal containers with the values from the
   * dark containers in the last stage so the deparser can put the fields back
   * into the packet.
   * This "dark_proxy" header represents those normal containers. */
  phv_group_h edark_proxy;

  /* A dummy header which will never be parsed or deparsed which provides a
   * scratch space in the egress pipeline.  The P4 compiler thinks it is
   * possible the header is deparsed but the control plane will never program
   * table actions to do this. */
  dummy_h dummy;
}

struct metadata_t {
  /* The following metadata structure represents the dark PHVs allocated to one
   * gress and will be used to carry the packet data parsed and deparsed with
   * the "dark_proxy" header through the MAU pipeline. */
  phv_group_t dark;
}

/* Use annotations to guide the compiler into the allocation we require. */
#define CONTAINER_SZ_TYPE(gress, full_name, sz, type) \
  @pa_container_size(gress, #full_name, sz) \
  @pa_container_type(gress, #full_name, type) \
  @do_not_use_clot(gress, #full_name)
#define PER_GRESS_CONTAINERS(gress, name_prefix, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w0,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w1,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w2,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w3,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w4,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w5,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w6,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w7,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b0,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b1,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b2,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b3,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b4,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b5,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b6,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b7,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h0,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h1,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h2,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h3,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h4,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h5,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h6,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h7,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h8,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h9,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h10, 16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h11, 16, type)

PER_GRESS_CONTAINERS("ingress", hdr.imocha, "mocha")
PER_GRESS_CONTAINERS("egress",  hdr.emocha, "mocha")

PER_GRESS_CONTAINERS("ingress", hdr.idark_proxy, "normal")
PER_GRESS_CONTAINERS("egress",  hdr.edark_proxy, "normal")

PER_GRESS_CONTAINERS("ingress", md.dark, "dark")
PER_GRESS_CONTAINERS("egress",  md.dark, "dark")


/*******************************************************************************
*                     ____                _
*                    |  _ \ __ _ _ __ ___(_)_ __   __ _
*                    | |_) / _` | '__/ __| | '_ \ / _` |
*                    |  __/ (_| | |  \__ \ | | | | (_| |
*                    |_|   \__,_|_|  |___/_|_| |_|\__, |
*                                                 |___/
*
*******************************************************************************/

parser IPrsr(packet_in pkt,
             out i_header_t hdr,
             out metadata_t md,
             out ingress_intrinsic_metadata_t intr_md) {

  state start {
    pkt.extract(intr_md);

    /* We are not interested in the Phase0/PortMetadata. */
    pkt.advance(PORT_METADATA_SIZE);

    pkt.extract(hdr.itestdata);

    /* Extract bytes from the packet into the mocha containers assigned to the
     * ingress. */
    pkt.extract(hdr.imocha);

    /* Extract bytes from the packet into the normal containers assigned to the
     * ingress so they can later be moved into dark containers. */
    pkt.extract(hdr.idark_proxy);

    transition accept;
  }
}

@egress_intrinsic_metadata_opt
parser EPrsr(packet_in pkt,
             out e_header_t hdr,
             out metadata_t md,
             out egress_intrinsic_metadata_t intr_md) {
  state start {
    /* We are not interested in any of the egress intrinsic MD however we must
     * "extract" it anyways.  The compiler can disable the optional fields so
     * we cannot simply skip over a fixed number of bits. */
    pkt.extract(intr_md);

    pkt.extract(hdr.etestdata);

    /* Extract bytes from the packet into the mocha containers assigned to the
     * egress. */
    pkt.extract(hdr.emocha);

    /* Extract bytes from the packet into the normal containers assigned to the
     * egress so they can later be moved into dark containers. */
    pkt.extract(hdr.edark_proxy);

    transition accept;
  }
}


/*******************************************************************************
*                 ____                            _
*                |  _ \  ___ _ __   __ _ _ __ ___(_)_ __   __ _
*                | | | |/ _ \ '_ \ / _` | '__/ __| | '_ \ / _` |
*                | |_| |  __/ |_) | (_| | |  \__ \ | | | | (_| |
*                |____/ \___| .__/ \__,_|_|  |___/_|_| |_|\__, |
*                           |_|                           |___/ 
*******************************************************************************/

control IDprsr(packet_out pkt,
               inout i_header_t hdr,
               in metadata_t md,
               in ingress_intrinsic_metadata_for_deparser_t dprsr_md) {
  apply { pkt.emit(hdr); }
}

control EDprsr(packet_out pkt,
               inout e_header_t hdr,
               in metadata_t md,
               in egress_intrinsic_metadata_for_deparser_t intr_dprs_md) {
  apply { pkt.emit(hdr); }
}


/*******************************************************************************
*        ___                                 __  __    _   _   _
*       |_ _|_ __   __ _ _ __ ___  ___ ___  |  \/  |  / \ | | | |___
*        | || '_ \ / _` | '__/ _ \/ __/ __| | |\/| | / _ \| | | / __|
*        | || | | | (_| | | |  __/\__ \__ \ | |  | |/ ___ \ |_| \__ \
*       |___|_| |_|\__, |_|  \___||___/___/ |_|  |_/_/   \_\___/|___/
*                  |___/
*******************************************************************************/
@disable_reserved_i2e_drop_implementation
@gfm_parity_enable
control Ing(inout i_header_t hdr,
            inout metadata_t md,
            in ingress_intrinsic_metadata_t intr_md,
            in ingress_intrinsic_metadata_from_parser_t intr_prsr_md,
            inout ingress_intrinsic_metadata_for_deparser_t intr_dprsr_md,
            inout ingress_intrinsic_metadata_for_tm_t intr_tm_md) {
  @name(".cntPkt") DirectCounter<bit<32>>(CounterType_t.PACKETS) cntPkt;
  @name(".override_eg_port") action override_eg_port(PortId_t port) {
    intr_tm_md.ucast_egress_port = port;
    cntPkt.count();
  }

  @name(".override_eg_port_to_cpu")
  action override_eg_port_to_cpu(PortId_t port) {
    intr_tm_md.ucast_egress_port = port;
    cntPkt.count();
  }

  @stage(0) @name(".dst_override") table dst_override {
    actions = {
      override_eg_port;
      override_eg_port_to_cpu;
    }
    key = {
      intr_md.ingress_port: exact @name("ig_intr_md.ingress_port");
      hdr.itestdata.pkt_ctrl: range @name("testdata.pkt_ctrl");
    }
    size = DST_OVERRIDE_TABLE_SIZE;
    counters = cntPkt;
  }


  action init_dark_containers() {
    md.dark.w0 = hdr.idark_proxy.w0;
    md.dark.w1 = hdr.idark_proxy.w1;
    md.dark.w2 = hdr.idark_proxy.w2;
    md.dark.w3 = hdr.idark_proxy.w3;
    md.dark.w4 = hdr.idark_proxy.w4;
    md.dark.w5 = hdr.idark_proxy.w5;
    md.dark.w6 = hdr.idark_proxy.w6;
    md.dark.w7 = hdr.idark_proxy.w7;

    md.dark.b0 = hdr.idark_proxy.b0;
    md.dark.b1 = hdr.idark_proxy.b1;
    md.dark.b2 = hdr.idark_proxy.b2;
    md.dark.b3 = hdr.idark_proxy.b3;
    md.dark.b4 = hdr.idark_proxy.b4;
    md.dark.b5 = hdr.idark_proxy.b5;
    md.dark.b6 = hdr.idark_proxy.b6;
    md.dark.b7 = hdr.idark_proxy.b7;

    md.dark.h0 = hdr.idark_proxy.h0;
    md.dark.h1 = hdr.idark_proxy.h1;
    md.dark.h2 = hdr.idark_proxy.h2;
    md.dark.h3 = hdr.idark_proxy.h3;
    md.dark.h4 = hdr.idark_proxy.h4;
    md.dark.h5 = hdr.idark_proxy.h5;
    md.dark.h6 = hdr.idark_proxy.h6;
    md.dark.h7 = hdr.idark_proxy.h7;
    md.dark.h8 = hdr.idark_proxy.h8;
    md.dark.h9 = hdr.idark_proxy.h9;
    md.dark.h10 = hdr.idark_proxy.h10;
    md.dark.h11 = hdr.idark_proxy.h11;
  }
  @stage(0)
  table dark_init {
    actions = {init_dark_containers; }
    size = 1;
    const default_action = init_dark_containers;
  }

  action restore_dark_containers() {
    hdr.idark_proxy.w0 = md.dark.w0;
    hdr.idark_proxy.w1 = md.dark.w1;
    hdr.idark_proxy.w2 = md.dark.w2;
    hdr.idark_proxy.w3 = md.dark.w3;
    hdr.idark_proxy.w4 = md.dark.w4;
    hdr.idark_proxy.w5 = md.dark.w5;
    hdr.idark_proxy.w6 = md.dark.w6;
    hdr.idark_proxy.w7 = md.dark.w7;

    hdr.idark_proxy.b0 = md.dark.b0;
    hdr.idark_proxy.b1 = md.dark.b1;
    hdr.idark_proxy.b2 = md.dark.b2;
    hdr.idark_proxy.b3 = md.dark.b3;
    hdr.idark_proxy.b4 = md.dark.b4;
    hdr.idark_proxy.b5 = md.dark.b5;
    hdr.idark_proxy.b6 = md.dark.b6;
    hdr.idark_proxy.b7 = md.dark.b7;

    hdr.idark_proxy.h0 = md.dark.h0;
    hdr.idark_proxy.h1 = md.dark.h1;
    hdr.idark_proxy.h2 = md.dark.h2;
    hdr.idark_proxy.h3 = md.dark.h3;
    hdr.idark_proxy.h4 = md.dark.h4;
    hdr.idark_proxy.h5 = md.dark.h5;
    hdr.idark_proxy.h6 = md.dark.h6;
    hdr.idark_proxy.h7 = md.dark.h7;
    hdr.idark_proxy.h8 = md.dark.h8;
    hdr.idark_proxy.h9 = md.dark.h9;
    hdr.idark_proxy.h10 = md.dark.h10;
    hdr.idark_proxy.h11 = md.dark.h11;
  }
  @stage(LAST_STAGE)
  table dark_restore {
    actions = {restore_dark_containers; }
    size = 1;
    const default_action = restore_dark_containers;
  }

  apply {
    dark_init.apply();
    dst_override.apply();
    dark_restore.apply();
  }
}


/*******************************************************************************
*           _____                          __  __    _   _   _
*          | ____|__ _ _ __ ___  ___ ___  |  \/  |  / \ | | | |___
*          |  _| / _` | '__/ _ \/ __/ __| | |\/| | / _ \| | | / __|
*          | |__| (_| | | |  __/\__ \__ \ | |  | |/ ___ \ |_| \__ \
*          |_____\__, |_|  \___||___/___/ |_|  |_/_/   \_\___/|___/
*                |___/
*******************************************************************************/
@disable_egress_mirror_io_select_initialization
control Egr(inout e_header_t hdr,
            inout metadata_t md,
            in egress_intrinsic_metadata_t intr_md,
            in egress_intrinsic_metadata_from_parser_t intr_md_from_prsr,
            inout egress_intrinsic_metadata_for_deparser_t intr_dprsr_md,
            inout egress_intrinsic_metadata_for_output_port_t intr_oport_md) {
  action init_dark_containers() {
    md.dark.w0 = hdr.edark_proxy.w0;
    md.dark.w1 = hdr.edark_proxy.w1;
    md.dark.w2 = hdr.edark_proxy.w2;
    md.dark.w3 = hdr.edark_proxy.w3;
    md.dark.w4 = hdr.edark_proxy.w4;
    md.dark.w5 = hdr.edark_proxy.w5;
    md.dark.w6 = hdr.edark_proxy.w6;
    md.dark.w7 = hdr.edark_proxy.w7;

    md.dark.b0 = hdr.edark_proxy.b0;
    md.dark.b1 = hdr.edark_proxy.b1;
    md.dark.b2 = hdr.edark_proxy.b2;
    md.dark.b3 = hdr.edark_proxy.b3;
    md.dark.b4 = hdr.edark_proxy.b4;
    md.dark.b5 = hdr.edark_proxy.b5;
    md.dark.b6 = hdr.edark_proxy.b6;
    md.dark.b7 = hdr.edark_proxy.b7;

    md.dark.h0 = hdr.edark_proxy.h0;
    md.dark.h1 = hdr.edark_proxy.h1;
    md.dark.h2 = hdr.edark_proxy.h2;
    md.dark.h3 = hdr.edark_proxy.h3;
    md.dark.h4 = hdr.edark_proxy.h4;
    md.dark.h5 = hdr.edark_proxy.h5;
    md.dark.h6 = hdr.edark_proxy.h6;
    md.dark.h7 = hdr.edark_proxy.h7;
    md.dark.h8 = hdr.edark_proxy.h8;
    md.dark.h9 = hdr.edark_proxy.h9;
    md.dark.h10 = hdr.edark_proxy.h10;
    md.dark.h11 = hdr.edark_proxy.h11;
  }
  @stage(0)
  table dark_init {
    actions = {init_dark_containers; }
    size = 1;
    const default_action = init_dark_containers;
  }

  action restore_dark_containers() {
    hdr.edark_proxy.w0 = md.dark.w0;
    hdr.edark_proxy.w1 = md.dark.w1;
    hdr.edark_proxy.w2 = md.dark.w2;
    hdr.edark_proxy.w3 = md.dark.w3;
    hdr.edark_proxy.w4 = md.dark.w4;
    hdr.edark_proxy.w5 = md.dark.w5;
    hdr.edark_proxy.w6 = md.dark.w6;
    hdr.edark_proxy.w7 = md.dark.w7;

    hdr.edark_proxy.b0 = md.dark.b0;
    hdr.edark_proxy.b1 = md.dark.b1;
    hdr.edark_proxy.b2 = md.dark.b2;
    hdr.edark_proxy.b3 = md.dark.b3;
    hdr.edark_proxy.b4 = md.dark.b4;
    hdr.edark_proxy.b5 = md.dark.b5;
    hdr.edark_proxy.b6 = md.dark.b6;
    hdr.edark_proxy.b7 = md.dark.b7;

    hdr.edark_proxy.h0 = md.dark.h0;
    hdr.edark_proxy.h1 = md.dark.h1;
    hdr.edark_proxy.h2 = md.dark.h2;
    hdr.edark_proxy.h3 = md.dark.h3;
    hdr.edark_proxy.h4 = md.dark.h4;
    hdr.edark_proxy.h5 = md.dark.h5;
    hdr.edark_proxy.h6 = md.dark.h6;
    hdr.edark_proxy.h7 = md.dark.h7;
    hdr.edark_proxy.h8 = md.dark.h8;
    hdr.edark_proxy.h9 = md.dark.h9;
    hdr.edark_proxy.h10 = md.dark.h10;
    hdr.edark_proxy.h11 = md.dark.h11;
  }
  @stage(LAST_STAGE)
  table dark_restore {
    actions = {restore_dark_containers; }
    size = 1;
    const default_action = restore_dark_containers;
  }

  action dummy() { hdr.dummy.setValid(); }

#define gfm_tbls(stg_id)                                             \
  @ways(1) /* Single way to reduce power */                          \
  @dynamic_table_key_masks(1) /* DKM to disable ghost bits */        \
  @stage(stg_id) table gfm_check_a_##stg_id##_stage {                \
    key = { hdr.edark_proxy.w0  : exact;                             \
            hdr.edark_proxy.w1  : exact;                             \
            hdr.edark_proxy.w2  : exact;                             \
            hdr.edark_proxy.w3  : exact;                             \
            hdr.edark_proxy.w4  : exact;                             \
            hdr.edark_proxy.w5  : exact;                             \
            hdr.edark_proxy.w6  : exact;                             \
            hdr.edark_proxy.w7  : exact;                             \
            hdr.edark_proxy.b0  : exact;                             \
            hdr.edark_proxy.b1  : exact;                             \
            hdr.edark_proxy.b2  : exact;                             \
            hdr.edark_proxy.b3  : exact;                             \
            hdr.edark_proxy.b4  : exact;                             \
            hdr.edark_proxy.b5  : exact;                             \
            hdr.edark_proxy.b6  : exact;                             \
            hdr.edark_proxy.b7  : exact;                             \
            hdr.edark_proxy.h0  : exact;                             \
            hdr.edark_proxy.h1  : exact;                             \
            hdr.edark_proxy.h2  : exact;                             \
            hdr.edark_proxy.h3  : exact;                             \
            hdr.edark_proxy.h4  : exact;                             \
            hdr.edark_proxy.h5  : exact;                             \
            hdr.edark_proxy.h6  : exact;                             \
            hdr.edark_proxy.h7  : exact;                             \
            hdr.edark_proxy.h8  : exact;                             \
            hdr.edark_proxy.h9  : exact;                             \
            hdr.edark_proxy.h10 : exact;                             \
            hdr.edark_proxy.h11 : exact;                             \
    }                                                                \
    actions = { dummy; NoAction; }                                   \
    size = 1024;                                                     \
    default_action = NoAction();                                     \
  }                                                                  \
  @ways(1) /* Single way to reduce power */                          \
  @dynamic_table_key_masks(1) /* DKM to disable ghost bits */        \
  @stage(stg_id) table gfm_check_b_##stg_id##_stage {                \
    key = { hdr.emocha.w0  : exact;                                  \
            hdr.emocha.w1  : exact;                                  \
            hdr.emocha.w2  : exact;                                  \
            hdr.emocha.w3  : exact;                                  \
            hdr.emocha.w4  : exact;                                  \
            hdr.emocha.w5  : exact;                                  \
            hdr.emocha.w6  : exact;                                  \
            hdr.emocha.w7  : exact;                                  \
            hdr.emocha.b0  : exact;                                  \
            hdr.emocha.b1  : exact;                                  \
            hdr.emocha.b2  : exact;                                  \
            hdr.emocha.b3  : exact;                                  \
            hdr.emocha.b4  : exact;                                  \
            hdr.emocha.b5  : exact;                                  \
            hdr.emocha.b6  : exact;                                  \
            hdr.emocha.b7  : exact;                                  \
            hdr.emocha.h0  : exact;                                  \
            hdr.emocha.h1  : exact;                                  \
            hdr.emocha.h2  : exact;                                  \
            hdr.emocha.h3  : exact;                                  \
            hdr.emocha.h4  : exact;                                  \
            hdr.emocha.h5  : exact;                                  \
            hdr.emocha.h6  : exact;                                  \
            hdr.emocha.h7  : exact;                                  \
            hdr.emocha.h8  : exact;                                  \
            hdr.emocha.h9  : exact;                                  \
            hdr.emocha.h10 : exact;                                  \
            hdr.emocha.h11 : exact;                                  \
    }                                                                \
    actions = { dummy; NoAction; }                                   \
    size = 1024;                                                     \
    default_action = NoAction();                                     \
  }

  gfm_tbls(0)
  gfm_tbls(1)
  gfm_tbls(2)
  gfm_tbls(3)
  gfm_tbls(4)
  gfm_tbls(5)
  gfm_tbls(6)
  gfm_tbls(7)
  gfm_tbls(8)
  gfm_tbls(9)
  gfm_tbls(10)
  gfm_tbls(11)
#ifndef TOFINO2M
  gfm_tbls(12)
  gfm_tbls(13)
  gfm_tbls(14)
  gfm_tbls(15)
  gfm_tbls(16)
  gfm_tbls(17)
  gfm_tbls(18)
  gfm_tbls(19)
#endif

  apply {
    dark_init.apply();

    switch(gfm_check_a_0_stage.apply().action_run) { NoAction: { gfm_check_b_0_stage.apply(); } }
    switch(gfm_check_a_1_stage.apply().action_run) { NoAction: { gfm_check_b_1_stage.apply(); } }
    switch(gfm_check_a_2_stage.apply().action_run) { NoAction: { gfm_check_b_2_stage.apply(); } }
    switch(gfm_check_a_3_stage.apply().action_run) { NoAction: { gfm_check_b_3_stage.apply(); } }
    switch(gfm_check_a_4_stage.apply().action_run) { NoAction: { gfm_check_b_4_stage.apply(); } }
    switch(gfm_check_a_5_stage.apply().action_run) { NoAction: { gfm_check_b_5_stage.apply(); } }
    switch(gfm_check_a_6_stage.apply().action_run) { NoAction: { gfm_check_b_6_stage.apply(); } }
    switch(gfm_check_a_7_stage.apply().action_run) { NoAction: { gfm_check_b_7_stage.apply(); } }
    switch(gfm_check_a_8_stage.apply().action_run) { NoAction: { gfm_check_b_8_stage.apply(); } }
    switch(gfm_check_a_9_stage.apply().action_run) { NoAction: { gfm_check_b_9_stage.apply(); } }
    switch(gfm_check_a_10_stage.apply().action_run) { NoAction: { gfm_check_b_10_stage.apply(); } }
    switch(gfm_check_a_11_stage.apply().action_run) { NoAction: { gfm_check_b_11_stage.apply(); } }
#ifndef TOFINO2M
    switch(gfm_check_a_12_stage.apply().action_run) { NoAction: { gfm_check_b_12_stage.apply(); } }
    switch(gfm_check_a_13_stage.apply().action_run) { NoAction: { gfm_check_b_13_stage.apply(); } }
    switch(gfm_check_a_14_stage.apply().action_run) { NoAction: { gfm_check_b_14_stage.apply(); } }
    switch(gfm_check_a_15_stage.apply().action_run) { NoAction: { gfm_check_b_15_stage.apply(); } }
    switch(gfm_check_a_16_stage.apply().action_run) { NoAction: { gfm_check_b_16_stage.apply(); } }
    switch(gfm_check_a_17_stage.apply().action_run) { NoAction: { gfm_check_b_17_stage.apply(); } }
    switch(gfm_check_a_18_stage.apply().action_run) { NoAction: { gfm_check_b_18_stage.apply(); } }
    switch(gfm_check_a_19_stage.apply().action_run) { NoAction: { gfm_check_b_19_stage.apply(); } }
#endif

    dark_restore.apply();
  }
}

Pipeline(IPrsr(), Ing(), IDprsr(), EPrsr(), Egr(), EDprsr()) pipeline_profile;
Switch(pipeline_profile) main;
