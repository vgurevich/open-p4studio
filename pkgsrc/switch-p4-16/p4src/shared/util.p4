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



#include "types.p4"

#ifdef FOLDED_SWITCH_PIPELINE

//-------------------------------------------------------------------------------------------------
// Pack qid and icos into the same byte
//-------------------------------------------------------------------------------------------------
control BridgedMDPack(inout switch_header_t hdr, inout switch_local_metadata_t local_md)() {

    action set_bridged_md_fields() {
        hdr.fp_bridged_md.qid_icos[7:5] = local_md.qos.icos;
        hdr.fp_bridged_md.qid_icos[4:0] = local_md.qos.qid;
	hdr.fp_bridged_md.ingress_bd_tt_myip[12:0]  = local_md.bd;
	hdr.fp_bridged_md.ingress_bd_tt_myip[13:13] = (bit<1>)local_md.tunnel.terminate;
	hdr.fp_bridged_md.ingress_bd_tt_myip[15:14] = local_md.flags.myip;
    }

    table bridged_md_pack {
        actions = {
            set_bridged_md_fields;
        }

        const default_action = set_bridged_md_fields();
        size = 256;
    }

    apply {
        bridged_md_pack.apply();
    }
}

//-------------------------------------------------------------------------------------------------
// Set drop reason for internal bridged_md in folded pipeline
//-------------------------------------------------------------------------------------------------
control SetIG0DropReason(inout switch_header_t hdr, in switch_local_metadata_t local_md) {

    action update_drop_reason(switch_drop_reason_t drop_reason) {
        hdr.fp_bridged_md.drop_reason = drop_reason;
    }

    action copy_from_l2_drop_reason() {
        hdr.fp_bridged_md.drop_reason = local_md.l2_drop_reason;
    }

    action copy_from_drop_reason() {
        hdr.fp_bridged_md.drop_reason = local_md.drop_reason;
    }

    table drop_reason_update {
        key = {
                local_md.l2_drop_reason : ternary;
                local_md.flags.port_vlan_miss : ternary;
                local_md.stp.state_ : ternary;
        }
        actions = {
            update_drop_reason;
            copy_from_l2_drop_reason;
            copy_from_drop_reason;
        }
        default_action = copy_from_drop_reason();
        size = 512;

        const entries = {
	    // Previous L2 drops
            (SWITCH_DROP_REASON_SRC_MAC_ZERO,            _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_DST_MAC_ZERO,            _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_SRC_MAC_MULTICAST,       _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO,      _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO,      _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST, _, _) : copy_from_l2_drop_reason();
            (SWITCH_DROP_REASON_DMAC_RESERVED,           _, _) : copy_from_l2_drop_reason();
	    // additional L2 drop reasons
            (0,  true,                         _)   : update_drop_reason(SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS);
            (0, false, SWITCH_STP_STATE_LEARNING)   : update_drop_reason(SWITCH_DROP_REASON_STP_STATE_LEARNING);
            (0, false, SWITCH_STP_STATE_BLOCKING)   : update_drop_reason(SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING);
	    // previous non-L2 drops
            (0, false, SWITCH_STP_STATE_FORWARDING) : copy_from_drop_reason();
        }
    }

    apply {
        drop_reason_update.apply();
    }
}

#else
// Bridged metadata fields for Egress pipeline.
action add_bridged_md(
        inout switch_bridged_metadata_h bridged_md, in switch_local_metadata_t local_md) {
    bridged_md.setValid();
    bridged_md.src = SWITCH_PKT_SRC_BRIDGED;
    bridged_md.base.ingress_port = local_md.ingress_port;
    bridged_md.base.ingress_port_lag_index = local_md.ingress_port_lag_index;
    bridged_md.base.ingress_bd = local_md.bd;
    bridged_md.base.nexthop = local_md.nexthop;
    bridged_md.base.pkt_type = local_md.lkp.pkt_type;
    bridged_md.base.routed = local_md.flags.routed;
    bridged_md.base.bypass_egress = local_md.flags.bypass_egress;
#if defined(PTP_ENABLE)
    bridged_md.base.capture_ts = local_md.flags.capture_ts;
#endif
    bridged_md.base.cpu_reason = local_md.cpu_reason;
    bridged_md.base.timestamp = local_md.timestamp;
    bridged_md.base.tc = local_md.qos.tc;
    bridged_md.base.qid = local_md.qos.qid;
    bridged_md.base.color = local_md.qos.color;
    bridged_md.base.vrf = local_md.vrf;

#if defined(EGRESS_IP_ACL_ENABLE) || defined(EGRESS_MIRROR_ACL_ENABLE)
    bridged_md.acl.l4_src_port = local_md.lkp.l4_src_port;
    bridged_md.acl.l4_dst_port = local_md.lkp.l4_dst_port;
#ifdef ACL_USER_META_ENABLE
    bridged_md.acl.user_metadata = local_md.user_metadata;
#endif // ACL_USER_META_ENABLE
#if defined(EGRESS_ACL_PORT_RANGE_ENABLE) && !defined(L4_PORT_EGRESS_LOU_ENABLE)
    bridged_md.acl.l4_src_port_label = local_md.l4_src_port_label;
    bridged_md.acl.l4_dst_port_label = local_md.l4_dst_port_label;
#endif // EGRESS_ACL_PORT_RANGE_ENABLE && !L4_PORT_EGRESS_LOU_ENABLE
    bridged_md.acl.tcp_flags = local_md.lkp.tcp_flags;
#elif defined(DTEL_FLOW_REPORT_ENABLE)
    bridged_md.acl.tcp_flags = local_md.lkp.tcp_flags;
#endif // EGRESS_IP_ACL_ENABLE || EGRESS_MIRROR_ACL_ENABLE

#ifdef TUNNEL_ENABLE
    bridged_md.tunnel.tunnel_nexthop = local_md.tunnel_nexthop;
#if defined(VXLAN_ENABLE) && !defined(DTEL_ENABLE)
    bridged_md.tunnel.hash = local_md.lag_hash[15:0];
#endif /* VXLAN_ENABLE && !DTEL_ENABLE */
#ifdef MPLS_ENABLE
    bridged_md.tunnel.mpls_pop_count = local_md.tunnel.mpls_pop_count;
#endif
#ifdef TUNNEL_TTL_MODE_ENABLE
    bridged_md.tunnel.ttl_mode = local_md.tunnel.ttl_mode;
#endif /* TUNNEL_TTL_MODE_ENABLE */
#ifdef TUNNEL_ECN_RFC_6040_ENABLE
    bridged_md.tunnel.ecn_mode = local_md.tunnel.ecn_mode;
#endif /* TUNNEL_ECN_RFC_6040_ENABLE */
#ifdef TUNNEL_QOS_MODE_ENABLE
    bridged_md.tunnel.qos_mode = local_md.tunnel.qos_mode;
#endif /* TUNNEL_QOS_MODE_ENABLE */
    bridged_md.tunnel.terminate = local_md.tunnel.terminate;
#endif

#ifdef DTEL_ENABLE
    bridged_md.dtel.report_type = local_md.dtel.report_type;
    bridged_md.dtel.session_id = local_md.dtel.session_id;
    bridged_md.dtel.hash = local_md.lag_hash;
    bridged_md.dtel.egress_port = local_md.egress_port;
#endif
#ifdef SFC_ENABLE
    bridged_md.sfc = {local_md.sfc.type,
                      local_md.sfc.queue_register_idx};
#endif
#ifdef BFD_OFFLOAD_ENABLE
    bridged_md.base.bfd_pkt_tx = local_md.bfd.pkt_tx;
    bridged_md.base.bfd_tx_mult = local_md.bfd.tx_mult;
#endif
}

action set_ig_intr_md(in switch_local_metadata_t local_md,
                      inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
                      inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {
    ig_intr_md_for_tm.mcast_grp_b = local_md.multicast.id;
// Set PRE hash values
    ig_intr_md_for_tm.level2_mcast_hash = local_md.lag_hash[28:16];
    ig_intr_md_for_tm.rid = local_md.bd;

#ifdef QOS_ENABLE
    ig_intr_md_for_tm.qid = local_md.qos.qid;
    ig_intr_md_for_tm.ingress_cos = local_md.qos.icos;
#endif
}

#ifdef INGRESS_ACL_ACTION_MIRROR_OUT_ENABLE
action copy_mirror_to_bridged_md(
        inout switch_bridged_metadata_h bridged_md, inout switch_local_metadata_t local_md) {
    bridged_md.mirror.type = local_md.mirror.type;
    bridged_md.mirror.src = local_md.mirror.src;
    bridged_md.mirror.session_id = local_md.mirror.session_id;
    bridged_md.mirror.meter_index = local_md.mirror.meter_index;

    // reset local mirror type if mirror src is egress,
    // so that Ingress Mirror control block is skipped
    local_md.mirror.type = SWITCH_MIRROR_TYPE_INVALID;
    // resetting below are not must, can ignore as well
    local_md.mirror.src = SWITCH_PKT_SRC_BRIDGED;
    local_md.mirror.session_id = 0;
    local_md.mirror.meter_index = 0;
}

#endif
control SetEgIntrMd(inout switch_header_t hdr,
                    in switch_local_metadata_t local_md,
                    inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
                    inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    apply {
#if __TARGET_TOFINO__ == 2
#if TOFINO2_PADDING_ENABLE
      if (local_md.pkt_src == SWITCH_PKT_SRC_BRIDGED) {
          if (local_md.pkt_length >= MIN_SIZE) {
              hdr.pad.setInvalid();
              /* Truncation will not be enabled */
          } else {
              /* Padding will be emitted (setValid() was done in the parser */
              eg_intr_md_for_dprsr.mtu_trunc_len = MIN_SIZE;
          }
      }
#endif /* TOFINO2_PADDING_ENABLE */
#endif /* __TARGET_TOFINO__ == 2 */

#ifdef PTP_ENABLE
        eg_intr_md_for_oport.capture_tstamp_on_tx = (bit<1>)local_md.flags.capture_ts;
#endif
#ifdef MIRROR_ENABLE
        if (local_md.mirror.type != SWITCH_MIRROR_TYPE_INVALID) {
#if __TARGET_TOFINO__ == 1
            eg_intr_md_for_dprsr.mirror_type = (bit<3>) local_md.mirror.type;
#else
            eg_intr_md_for_dprsr.mirror_type = (bit<4>) local_md.mirror.type;
            if (local_md.mirror.src == SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT) {
                eg_intr_md_for_dprsr.mirror_io_select = 0;
            } else {
                eg_intr_md_for_dprsr.mirror_io_select = 1;
            }
#endif
        }
#endif
    }
}
#endif /* FOLDED_SWITCH_PIPELINE */
