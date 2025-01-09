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

#include <tofino/pdfixed/pd_conn_mgr.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pktgen_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <tofino/pdfixed/pd_mirror.h>

p4_pd_status_t p4_pd_mirror_session_update(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_mirror_session_info_t *mirr_sess_info,
    bool enable)

{
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;

  bf_mirror_session_info_t s_info;

  PIPE_MGR_MEMSET(&s_info, 0, sizeof(s_info));
  s_info.mirror_type =
      (bf_mirror_type_e)mirr_sess_info->type;  // bf values as same as pd
  s_info.dir =
      (bf_mirror_direction_e)mirr_sess_info->dir;  // bf values as same as pd
  /* The p4_pd_mirror_session_info_t does not have a pipe_mask instead if any
   * type of multicast is required (this includes c2c) then we set the mask to
   * all pipes. */
  if (mirr_sess_info->mcast_grp_a_v || mirr_sess_info->mcast_grp_b_v ||
      mirr_sess_info->c2c) {
    // Only if multicast mirroring, set pipe vector.
    s_info.pipe_mask = 0xF;
  } else {
    s_info.pipe_mask = 0x0;
  }
  s_info.ucast_egress_port = mirr_sess_info->egr_port;
  s_info.ucast_egress_port_v = mirr_sess_info->egr_port_v;
  s_info.egress_port_queue = mirr_sess_info->egr_port_queue;
  s_info.ingress_cos = mirr_sess_info->cos;
  s_info.packet_color = (bf_tm_color_t)mirr_sess_info->packet_color;
  s_info.level1_mcast_hash = mirr_sess_info->level1_mcast_hash;
  s_info.level2_mcast_hash = mirr_sess_info->level2_mcast_hash;
  s_info.mcast_grp_a = mirr_sess_info->mcast_grp_a;
  s_info.mcast_grp_a_v = mirr_sess_info->mcast_grp_a_v;
  s_info.mcast_grp_b = mirr_sess_info->mcast_grp_b;
  s_info.mcast_grp_b_v = mirr_sess_info->mcast_grp_b_v;
  s_info.mcast_l1_xid = mirr_sess_info->mcast_l1_xid;
  s_info.mcast_l2_xid = mirr_sess_info->mcast_l2_xid;
  s_info.mcast_rid = mirr_sess_info->mcast_rid;
  s_info.icos_for_copy_to_cpu = mirr_sess_info->cos;
  s_info.copy_to_cpu = mirr_sess_info->c2c;
  s_info.deflect_on_drop = 0;  // if set, it can cause -ve mirror loop
  s_info.max_pkt_len = mirr_sess_info->max_pkt_len ? mirr_sess_info->max_pkt_len
                                                   : (uint16_t)0xFFFF;
  // coalescing parameters
  // header[4]  initialization - compiler and hw use first 2 bytes of
  // this header in a special way, user can use rest 14
  if (s_info.mirror_type == BF_MIRROR_TYPE_COAL) {
    PIPE_MGR_MEMCPY(
        (uint8_t *)&s_info.header[0], (uint8_t *)mirr_sess_info->int_hdr, 16);
  }
  s_info.header_len = mirr_sess_info->int_hdr_len;
  s_info.timeout_usec = mirr_sess_info->timeout_usec;
  s_info.extract_len = mirr_sess_info->extract_len;
  // if api does not specify it, it comes from P4 program
  if (mirr_sess_info->extract_len == 0) {
    s_info.extract_len_from_p4 = true;
  }
  return bf_mirror_session_set(
      shdl, bf_dev_tgt, mirr_sess_info->id, &s_info, enable);
}

p4_pd_status_t p4_pd_mirror_session_create(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_mirror_session_info_t *mirr_sess_info)

{
  return p4_pd_mirror_session_update(
      shdl, dev_tgt, mirr_sess_info, true /*enable*/);
}

p4_pd_status_t p4_pd_mirror_session_delete(p4_pd_sess_hdl_t shdl,
                                           p4_pd_dev_target_t dev_tgt,
                                           p4_pd_mirror_id_t id) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_reset(shdl, bf_dev_tgt, id);
}

p4_pd_status_t p4_pd_mirror_session_disable(p4_pd_sess_hdl_t shdl,
                                            p4_pd_direction_t dir,
                                            p4_pd_dev_target_t dev_tgt,
                                            p4_pd_mirror_id_t id) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_disable(
      shdl, bf_dev_tgt, (bf_mirror_direction_e)dir, id);
}

p4_pd_status_t p4_pd_mirror_session_enable(p4_pd_sess_hdl_t shdl,
                                           p4_pd_direction_t dir,
                                           p4_pd_dev_target_t dev_tgt,
                                           p4_pd_mirror_id_t id) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_enable(
      shdl, bf_dev_tgt, (bf_mirror_direction_e)dir, id);
}

p4_pd_status_t p4_pd_mirror_session_meta_flag_update(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_mirror_id_t id,
    p4_pd_mirror_meta_flag_e flag,
    bool value) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_meta_flag_update(
      shdl, bf_dev_tgt, id, (bf_mirror_meta_flag_e)flag, value);
}

p4_pd_status_t p4_pd_mirror_session_priority_update(p4_pd_sess_hdl_t shdl,
                                                    p4_pd_dev_target_t dev_tgt,
                                                    p4_pd_mirror_id_t id,
                                                    bool value) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_priority_update(shdl, bf_dev_tgt, id, value);
}

p4_pd_status_t p4_pd_mirror_session_coal_mode_update(p4_pd_sess_hdl_t shdl,
                                                     p4_pd_dev_target_t dev_tgt,
                                                     p4_pd_mirror_id_t id,
                                                     bool value) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_coal_mode_update(shdl, bf_dev_tgt, id, value);
}
static void p4_pd_mirror_session_info_translate(
    p4_pd_mirror_session_info_t *mirr_sess_info,
    bf_mirror_session_info_t *s_info) {
  int i;
  mirr_sess_info->type = (p4_pd_mirror_type_e)s_info->mirror_type;
  mirr_sess_info->dir = (p4_pd_direction_t)s_info->dir;
  mirr_sess_info->egr_port = s_info->ucast_egress_port;
  mirr_sess_info->egr_port_v = s_info->ucast_egress_port_v;
  mirr_sess_info->egr_port_queue = s_info->egress_port_queue;
  mirr_sess_info->packet_color = (p4_pd_color_t)s_info->packet_color;
  mirr_sess_info->mcast_grp_a = s_info->mcast_grp_a;
  mirr_sess_info->mcast_grp_a_v = s_info->mcast_grp_a_v;
  mirr_sess_info->mcast_grp_b = s_info->mcast_grp_b;
  mirr_sess_info->mcast_grp_b_v = s_info->mcast_grp_b_v;
  mirr_sess_info->max_pkt_len =
      s_info->max_pkt_len == 0xFFFF ? 0 : s_info->max_pkt_len;
  mirr_sess_info->level1_mcast_hash = s_info->level1_mcast_hash;
  mirr_sess_info->level2_mcast_hash = s_info->level2_mcast_hash;
  mirr_sess_info->cos = s_info->ingress_cos;
  mirr_sess_info->c2c = s_info->copy_to_cpu;
  mirr_sess_info->extract_len = s_info->extract_len;
  mirr_sess_info->timeout_usec = s_info->timeout_usec;
  if (mirr_sess_info->int_hdr != NULL) {
    if (s_info->mirror_type == BF_MIRROR_TYPE_COAL) {
      for (i = 0; i < 4; i++) mirr_sess_info->int_hdr[i] = s_info->header[i];
      mirr_sess_info->int_hdr_len = s_info->header_len;
    } else {
      for (i = 0; i < 4; i++) mirr_sess_info->int_hdr[i] = 0;
      mirr_sess_info->int_hdr_len = 0;
    }
  } else {
    mirr_sess_info->int_hdr_len = 0;
  }
}
p4_pd_status_t p4_pd_mirror_session_get_first(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_mirror_session_info_t *first_mirr_sess_info,
    bf_dev_pipe_t *first_mirr_sess_pipe_id) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  bf_mirror_session_info_t s_info;
  pipe_status_t sts;
  bf_mirror_get_id_t bf_first;
  sts = bf_mirror_session_get_first(shdl, bf_dev_tgt, &s_info, &bf_first);
  if (sts != PIPE_SUCCESS) return sts;
  first_mirr_sess_info->id = bf_first.sid;
  p4_pd_mirror_session_info_translate(first_mirr_sess_info, &s_info);
  *first_mirr_sess_pipe_id =
      bf_first.pipe_id == BF_DEV_PIPE_ALL ? PD_DEV_PIPE_ALL : bf_first.pipe_id;
  return PIPE_SUCCESS;
}
p4_pd_status_t p4_pd_mirror_session_get_next(
    p4_pd_sess_hdl_t shdl,
    p4_pd_dev_target_t dev_tgt,
    p4_pd_mirror_id_t current_mirr_id,
    bf_dev_pipe_t current_mirr_sess_pipe_id,
    p4_pd_mirror_session_info_t *next_mirr_sess_info,
    bf_dev_pipe_t *next_mirr_sess_pipe_id) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  bf_mirror_get_id_t bf_current, bf_next;
  bf_current.sid = current_mirr_id;
  bf_current.pipe_id = current_mirr_sess_pipe_id == PD_DEV_PIPE_ALL
                           ? BF_DEV_PIPE_ALL
                           : current_mirr_sess_pipe_id;
  bf_mirror_session_info_t s_info;
  pipe_status_t sts;
  sts = bf_mirror_session_get_next(
      shdl, bf_dev_tgt, bf_current, &s_info, &bf_next);
  if (sts != PIPE_SUCCESS) return sts;
  next_mirr_sess_info->id = bf_next.sid;
  p4_pd_mirror_session_info_translate(next_mirr_sess_info, &s_info);
  *next_mirr_sess_pipe_id =
      bf_next.pipe_id == BF_DEV_PIPE_ALL ? PD_DEV_PIPE_ALL : bf_next.pipe_id;
  return PIPE_SUCCESS;
}
p4_pd_status_t p4_pd_mirror_session_pipe_vector_set(p4_pd_sess_hdl_t shdl,
                                                    p4_pd_dev_target_t dev_tgt,
                                                    p4_pd_mirror_id_t id,
                                                    int logical_pipe_vector) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_mcast_pipe_vector_set(
      shdl, bf_dev_tgt, id, logical_pipe_vector);
}

p4_pd_status_t p4_pd_mirror_session_pipe_vector_get(p4_pd_sess_hdl_t shdl,
                                                    p4_pd_dev_target_t dev_tgt,
                                                    p4_pd_mirror_id_t id,
                                                    int *logical_pipe_vector) {
  dev_target_t bf_dev_tgt;
  bf_dev_tgt.device_id = dev_tgt.device_id;
  bf_dev_tgt.dev_pipe_id = (dev_tgt.dev_pipe_id == PD_DEV_PIPE_ALL)
                               ? DEV_PIPE_ALL
                               : dev_tgt.dev_pipe_id;
  return bf_mirror_session_mcast_pipe_vector_get(
      shdl, bf_dev_tgt, id, logical_pipe_vector);
}
p4_pd_status_t p4_pd_mirror_session_max_session_id_get(
    p4_pd_sess_hdl_t shdl,
    bf_dev_id_t device_id,
    p4_pd_mirror_type_e mir_type,
    p4_pd_mirror_id_t *id) {
  return bf_mirror_max_mirror_sessions_get(
      shdl, device_id, (bf_mirror_type_e)mir_type, id);
}
p4_pd_status_t p4_pd_mirror_session_base_session_id_get(
    p4_pd_sess_hdl_t shdl,
    bf_dev_id_t device_id,
    p4_pd_mirror_type_e mir_type,
    p4_pd_mirror_id_t *id) {
  return bf_mirror_base_mirror_session_id_get(
      shdl, device_id, (bf_mirror_type_e)mir_type, id);
}
