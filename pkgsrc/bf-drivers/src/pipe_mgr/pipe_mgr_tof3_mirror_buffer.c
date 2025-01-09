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


#include <dvm/bf_drv_intf.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include "pipe_mgr_drv.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_tof3_mirror_buffer.h"

#define S_FLD(_s, _f) _s._f
#define MIRROR_REG_ADDR(_name) \
  offsetof(tof3_reg, S_FLD(pipes[0].pardereg.mirreg.mirror, _name))

typedef struct pipe_mgr_tof3_coal_id_info_t {
  uint16_t sid[4];  // 0-255 or 0xFFFF for invalid
} pipe_mgr_tof3_coal_id_info_t;

static pipe_mgr_tof3_coal_id_info_t
    coal_info[PIPE_MGR_NUM_DEVICES][PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX];

static pipe_status_t pipe_mgr_tof3_mirror_buf_reg_wr(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     pipe_bitmap_t pipes,
                                                     uint32_t addr,
                                                     uint32_t data) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  pipe_instr_write_reg_t instr;
  uint32_t stage = ~0;
  pipe_status_t sts;

  if (!dev_info) {
    LOG_ERROR("%s:%d Invalid device %d for mirror session %d",
              __func__,
              __LINE__,
              dev_id,
              sess_hdl);
    return PIPE_INVALID_ARG;
  }

  construct_instr_reg_write(dev_id, &instr, addr, data);
  stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  sts = pipe_mgr_drv_ilist_add(&sess_hdl,
                               dev_info,
                               &pipes,
                               stage,
                               (uint8_t *)&instr,
                               sizeof(pipe_instr_write_reg_t));
  return sts;
}

pipe_status_t pipe_mgr_tof3_mirror_buf_init_session(pipe_sess_hdl_t shdl,
                                                    bf_mirror_id_t sid,
                                                    bf_dev_id_t dev_id,
                                                    pipe_bitmap_t pbm) {
  pipe_status_t sts;
  for (int slice = 0; slice < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; ++slice) {
    sts = pipe_mgr_tof3_mirror_buf_reg_wr(
        shdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(slice_mem[slice].sess_cfg.entry[sid]),
        0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "Dev %d failed to initialize mirr sess_cfg for slice %d sid %d "
          "status %s",
          dev_id,
          slice,
          sid,
          pipe_str_err(sts));
      return sts;
    }
  }
  sts = pipe_mgr_tof3_mirror_buf_reg_wr(
      shdl, dev_id, pbm, MIRROR_REG_ADDR(s2p_sess.tbl0[sid]), 0);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d failed to initialize mirr sess_tbl for sid %d status %s",
              dev_id,
              sid,
              pipe_str_err(sts));
    return sts;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id) {
  // reset registers
  // configure any registers that fix value for users here
  uint32_t data;
  pipe_bitmap_t all_pipes;
  int i, j, k;
  uint32_t n_pipes = pipe_mgr_get_num_active_pipes(dev_id);
  PIPE_BITMAP_INIT(&all_pipes, PIPE_BMP_SIZE);
  for (i = 0; i < (int)n_pipes; i++) {
    PIPE_BITMAP_SET(&all_pipes, i);
  }
  // write reg coal_sess_cfg to all 0
  data = 0;
  for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
    for (k = 0; k < PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX; k++) {
      pipe_mgr_tof3_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          all_pipes,
          MIRROR_REG_ADDR(slice_mem[i].coal_sess_cfg.entry[k]),
          data);
    }
  }
  data = 0;
  for (k = 0; k < PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX; k++) {
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_0_4),
        data);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_1_4),
        data);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_2_4),
        data);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_3_4),
        data);
  }
  for (j = 0; j < PIPE_MGR_TOF3_MIRROR_SESSION_MAX; j++) {
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl, dev_id, all_pipes, MIRROR_REG_ADDR(s2p_sess.tbl0[j]), data);
  }
  // init coalesing id allocator
  for (size_t x = 0; x < sizeof coal_info[dev_id] / sizeof coal_info[dev_id][0];
       ++x) {
    for (size_t y = 0; y < sizeof coal_info[dev_id][x].sid /
                               sizeof coal_info[dev_id][x].sid[0];
         ++y) {
      coal_info[dev_id][x].sid[y] = 0xFFFF;
    }
  }

  // slice_regs.slice_ctr
  data = 0;
  for (j = 0; j < (int)n_pipes; j++) {
    PIPE_BITMAP_INIT(&all_pipes, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&all_pipes, j);
    setp_tof3_mirror_slice_ctrl_logic_pipe(&data, j);
    setp_tof3_mirror_slice_ctrl_cnt_en(&data, 1);
    setp_tof3_mirror_slice_ctrl_ll_reset(&data, 0);
    for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                      dev_id,
                                      all_pipes,
                                      MIRROR_REG_ADDR(slice_regs[i].slice_ctr),
                                      data);
    }
  }
  return PIPE_SUCCESS;
}

static int sid_to_coal_id(bf_dev_id_t dev_id,
                          uint16_t sid,
                          bf_dev_pipe_t pipe_id) {
  for (int i = 0; i < PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX; ++i) {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      if (coal_info[dev_id][i].sid[0] == sid &&
          coal_info[dev_id][i].sid[1] == sid &&
          coal_info[dev_id][i].sid[2] == sid &&
          coal_info[dev_id][i].sid[3] == sid)
        return i;
    } else {
      if (coal_info[dev_id][i].sid[pipe_id] == sid) return i;
    }
  }
  return -1;
}

static int alloc_coal_id(bf_dev_id_t dev_id,
                         uint16_t sid,
                         bf_dev_pipe_t pipe_id) {
  /* If allocating an ID for all pipes then just take the first one that is free
   * in all pipes.  When allocating for a specific pipe try to find an ID that
   * is already used in another pipe. */
  int first_free_all = -1;
  for (int i = 0; i < PIPE_MGR_TOF3_MIRROR_COAL_SESSION_MAX; ++i) {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      if (coal_info[dev_id][i].sid[0] == 0xFFFF &&
          coal_info[dev_id][i].sid[1] == 0xFFFF &&
          coal_info[dev_id][i].sid[2] == 0xFFFF &&
          coal_info[dev_id][i].sid[3] == 0xFFFF) {
        coal_info[dev_id][i].sid[0] = sid;
        coal_info[dev_id][i].sid[1] = sid;
        coal_info[dev_id][i].sid[2] = sid;
        coal_info[dev_id][i].sid[3] = sid;
        return i;
      }
    } else {
      if (coal_info[dev_id][i].sid[0] == 0xFFFF &&
          coal_info[dev_id][i].sid[1] == 0xFFFF &&
          coal_info[dev_id][i].sid[2] == 0xFFFF &&
          coal_info[dev_id][i].sid[3] == 0xFFFF) {
        if (first_free_all == -1) first_free_all = i;
      } else if (coal_info[dev_id][i].sid[pipe_id] == 0xFFFF) {
        coal_info[dev_id][i].sid[pipe_id] = sid;
        return i;
      }
    }
  }
  if (pipe_id != BF_DEV_PIPE_ALL && first_free_all != -1) {
    coal_info[dev_id][first_free_all].sid[pipe_id] = sid;
    return first_free_all;
  }
  return -1;
}

static void free_coal_id(bf_dev_id_t dev_id,
                         uint16_t sid,
                         bf_dev_pipe_t pipe_id) {
  int i = sid_to_coal_id(dev_id, sid, pipe_id);
  if (i == -1) return;
  if (pipe_id == BF_DEV_PIPE_ALL) {
    coal_info[dev_id][i].sid[0] = 0xFFFF;
    coal_info[dev_id][i].sid[1] = 0xFFFF;
    coal_info[dev_id][i].sid[2] = 0xFFFF;
    coal_info[dev_id][i].sid[3] = 0xFFFF;
  } else {
    coal_info[dev_id][i].sid[pipe_id] = 0xFFFF;
  }
}

static pipe_status_t tof3_mirror_buf_norm_session_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    uint16_t coal_sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  int i;
  uint32_t data;
  if (enable) {
    /* For Tofino3 program the logical ucast port */
    uint32_t epipe_port = 0;
    if (s_info->u.mirror_session_tof3_entry.epipe_port_v) {
      epipe_port = s_info->u.mirror_session_tof3_entry.epipe_port;
    }
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
    if (!dev_info) {
      LOG_ERROR("%s:%d Invalid device %d for mirror session %d",
                __func__,
                __LINE__,
                dev_id,
                sid);
      return PIPE_INVALID_ARG;
    }
    /* tm-vec calculation, contains die-id */
    uint32_t tm_vec_uc = 0, tm_vec_mc = 0, tm_vec_c2c = 0;
    uint32_t tm_vec = 0;
    if (s_info->u.mirror_session_tof3_entry.epipe_port_v) {
      bf_dev_pipe_t log_pipe = dev_info->dev_cfg.dev_port_to_pipe(epipe_port);
      /* Pipes 0-3 are on die 0 and pipes 4-7 are on die 1 */
      if (log_pipe <= 3) {
        tm_vec_uc = 0x1;
      } else {
        tm_vec_uc = 0x2;
      }
    }

    /* Mcast to supply local-remote die info, set local only for now
     * TBD: How these mirror c2c/mg1/mg2 TVTs are different than the
     * usual TVT for C2C/MGID1/MGID2 ? */
    if (s_info->u.mirror_session_tof3_entry.c2c_vld) {
      tm_vec_c2c = 0x1;
    }

    if ((s_info->u.mirror_session_tof3_entry.mcid1_vld) ||
        (s_info->u.mirror_session_tof3_entry.mcid2_vld)) {
      tm_vec_mc = 0x1;
    }

    if (tm_vec_c2c && !s_info->u.mirror_session_tof3_entry.mcid1_vld &&
        !s_info->u.mirror_session_tof3_entry.mcid2_vld) {
      s_info->u.mirror_session_tof3_entry.mcid1 = 0;
      s_info->u.mirror_session_tof3_entry.mcid1_vld = 1;
    }

    tm_vec = tm_vec_uc | tm_vec_mc | tm_vec_c2c;

    // s2p_regs.sess_entry_word0-4, s2p_sess.tbl0-1
    data = 0;
    setp_tof3_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.hash_cfg_flag_p << 1);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.hash_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_icos_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.icos_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_dod_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.dod_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_c2c_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.c2c_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_mc_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.mc_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_epipe_cfg_f(
        &data, s_info->u.mirror_session_tof3_entry.epipe_cfg_flag);
    setp_tof3_mirr_s2p_sess_cfg_word0_r_xid_f(
        &data, s_info->u.mirror_session_tof3_entry.mcast_l1_xid);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word0),
                                    data);
    data = 0;
    setp_tof3_mirr_s2p_sess_cfg_word1_r_mcid1_id_f(
        &data, s_info->u.mirror_session_tof3_entry.mcid1);
    setp_tof3_mirr_s2p_sess_cfg_word1_r_mcid2_id_f(
        &data, s_info->u.mirror_session_tof3_entry.mcid2);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word1),
                                    data);
    data = 0;
    setp_tof3_mirr_s2p_sess_cfg_word2_r_epipe_port_f(&data, epipe_port);
    setp_tof3_mirr_s2p_sess_cfg_word2_r_epipe_port_vld_f(
        &data, s_info->u.mirror_session_tof3_entry.epipe_port_v);
    setp_tof3_mirr_s2p_sess_cfg_word2_r_def_on_drop_f(
        &data, s_info->u.mirror_session_tof3_entry.deflect_on_drop);
    setp_tof3_mirr_s2p_sess_cfg_word2_r_rid_f(
        &data, s_info->u.mirror_session_tof3_entry.mcast_rid);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word2),
                                    data);
    data = 0;
    setp_tof3_mirr_s2p_sess_cfg_word3_r_hash1_f(
        &data, s_info->u.mirror_session_tof3_entry.hash1);
    setp_tof3_mirr_s2p_sess_cfg_word3_r_hash2_f(
        &data, s_info->u.mirror_session_tof3_entry.hash2);
    setp_tof3_mirr_s2p_sess_cfg_word3_r_tm_vec_f(&data, tm_vec);
    setp_tof3_mirr_s2p_sess_cfg_word3_r_egress_bypass_f(
        &data, s_info->u.mirror_session_tof3_entry.egr_bypass_flag);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word3),
                                    data);
    data = 0;
    setp_tof3_mirr_s2p_sess_cfg_word4_r_icos_f(
        &data, s_info->u.mirror_session_tof3_entry.icos);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_color_f(
        &data, s_info->u.mirror_session_tof3_entry.color);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_mcid1_vld_f(
        &data, s_info->u.mirror_session_tof3_entry.mcid1_vld);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_mcid2_vld_f(
        &data, s_info->u.mirror_session_tof3_entry.mcid2_vld);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_c2c_cos_f(
        &data, s_info->u.mirror_session_tof3_entry.c2c_cos);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_c2c_vld_f(
        &data, s_info->u.mirror_session_tof3_entry.c2c_vld);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_yid_tbl_sel_f(
        &data, s_info->u.mirror_session_tof3_entry.yid_tbl_sel);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_eport_qid_f(
        &data, s_info->u.mirror_session_tof3_entry.eport_qid);
    setp_tof3_mirr_s2p_sess_cfg_word4_r_yid_f(
        &data, s_info->u.mirror_session_tof3_entry.mcast_l2_xid);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word4),
                                    data);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl, dev_id, pbm, MIRROR_REG_ADDR(s2p_sess.tbl0[sid]), 0);
    // slice_mem.sess_cfg: egr_en, ingr_en, max_pkt_len, coal_sid
  } else {
    /* We don't disable the enable flag of the session rather we
     * zeroes out the mirror session configs such that even if
     * the session is enabled, packets with zero configs followed
     * by SOP gets dropped at TM */
    data = 0;
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word0),
                                    data);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word1),
                                    data);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word2),
                                    data);
    setp_tof3_mirr_s2p_sess_cfg_word3_r_tm_vec_f(&data, 1);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word3),
                                    data);
    pipe_mgr_tof3_mirror_buf_reg_wr(sess_hdl,
                                    dev_id,
                                    pbm,
                                    MIRROR_REG_ADDR(s2p_regs.sess_entry_word4),
                                    data);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl, dev_id, pbm, MIRROR_REG_ADDR(s2p_sess.tbl0[sid]), 0);
  }
  data = 0;
  if (s_info->dir == BF_DIR_INGRESS || s_info->dir == BF_DIR_BOTH) {
    setp_tof3_mirror_sess_entry_r_ingr_en(&data, 1);
  }
  if (s_info->dir == BF_DIR_EGRESS || s_info->dir == BF_DIR_BOTH) {
    setp_tof3_mirror_sess_entry_r_egr_en(&data, 1);
  }
  /* The pkt_len field is only 14 bits but max_pkt_len is a 16 bit value.  If
   * we are requesting something with the upper most bits set then ensure all
   * lower bits are also set. */
  /* Tofino-3 includes the Ethernet FCS in the truncation size while Tofino-1
   * does not.  So truncating a 200 byte packet to 128 bytes on Tofino-1 will
   * create a 132 byte packet (128 bytes of header+payload, 4 bytes of FCS) but
   * Tofino-3 will create a 128 byte packet (124 bytes of header+payload, 4
   * bytes of FCS).  Increase the requested size by four so the two chips
   * behave the same. */
  if ((0x3FFF - 4) < s_info->max_pkt_len) {
    setp_tof3_mirror_sess_entry_r_pkt_len(&data, 0xFFFF);
  } else {
    setp_tof3_mirror_sess_entry_r_pkt_len(&data, s_info->max_pkt_len + 4);
    /* Tofino-3's mirror block can only truncate on four byte boundaries, log a
     * warning if the requested size is not a multiple of four. */
    if (s_info->max_pkt_len & 3) {
      LOG_WARN(
          "Dev %d mirror session %d: Requested truncation size (max_pkt_len) "
          "of %d is not a multiple of four, using a size of %d",
          dev_id,
          sid,
          s_info->max_pkt_len,
          s_info->max_pkt_len & 0xFFFC);
    }
  }
  if (s_info->mirror_type == BF_MIRROR_TYPE_COAL) {
    setp_tof3_mirror_sess_entry_r_coal_num(&data, coal_sid);
    setp_tof3_mirror_sess_entry_r_coal_en(&data, enable);
  } else {
    setp_tof3_mirror_sess_entry_r_coal_num(&data, s_info->pri);
  }
  // slice_mem.sess_cfg: enable,coal_enble
  setp_tof3_mirror_sess_entry_r_sess_en(&data, 1);
  for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(slice_mem[i].sess_cfg.entry[sid]),
        data);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_session_reset(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          uint16_t sid,
                                          bf_dev_pipe_t pipe_id) {
  (void)shdl;
  free_coal_id(dev_info->dev_id, sid, pipe_id);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_mirror_buf_coal_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  // check sid-coal already existing, if no, check current numb < 16
  uint32_t data;
  int i;
  pipe_status_t sts;
  int coal_id = sid_to_coal_id(dev_id, sid, pipe_id);
  bool new_coal_id = false;
  /* If the session does not have a coalescing id try to get one. */
  if (coal_id == -1) {
    new_coal_id = true;
    coal_id = alloc_coal_id(dev_id, sid, pipe_id);
    if (coal_id == -1) {
      LOG_ERROR(
          "Device %d pipe %x cannot mirror session %d as coalescing; all "
          "coalescing IDs are in use",
          dev_id,
          pipe_id,
          sid);
      return PIPE_INVALID_ARG;
    }
  }

  // configure if enable
  if (enable) {
    uint32_t timeout_n;
    /**coal cfg**/
    // slice_mem.coal_sess_cfg: header_len, extract_len_from_p4, coal_mode,
    // extract_len, pri
    data = 0;
    setp_tof3_mirror_coal_sess_entry_r_pri(&data, s_info->pri);
    setp_tof3_mirror_coal_sess_entry_r_coal_mode(&data, s_info->coal_mode);
    if (s_info->extract_len_from_p4 == 1)
      setp_tof3_mirror_coal_sess_entry_r_len_cfg(&data,
                                                 s_info->extract_len_from_p4);
    else
      setp_tof3_mirror_coal_sess_entry_r_sample_pkt_len(
          &data,
          ((s_info->extract_len / 4) +
           ((s_info->extract_len % 4 == 0) ? 0 : 1)));
    setp_tof3_mirror_coal_sess_entry_r_coal_hdr(&data,
                                                (s_info->header_len - 8) / 4);
    for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof3_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          pbm,
          MIRROR_REG_ADDR(slice_mem[i].coal_sess_cfg.entry[coal_id]),
          data);
    }
    data = 0;
    setp_tof3_s2p_coal_hdr_r_coal_hdr_tbl_0_4_compiler_flag(
        &data, s_info->header[0] & 0xfful);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_0_4),
        data);
    data = 0;
    if (s_info->header_len >= 12) {
      // configure s2p_coal.coal_hdl_tbl: header[4]
      setp_tof3_s2p_coal_hdr_r_coal_hdr_tbl_1_4_user_0_3(&data,
                                                         s_info->header[1]);
    }
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_1_4),
        data);
    data = 0;
    if ((s_info->header_len == 16) || (s_info->coal_mode)) {
      // configure s2p_coal.coal_hdl_tbl: header[4]
      setp_tof3_s2p_coal_hdr_r_coal_hdr_tbl_2_4_user_4_7(&data,
                                                         s_info->header[2]);
    }
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_2_4),
        data);
    data = 0;
    setp_tof3_s2p_coal_hdr_r_coal_hdr_tbl_3_4_seq_num(&data, s_info->header[3]);
    pipe_mgr_tof3_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_3_4),
        data);
    // slice_mem.coal_to_cfg: timeout_usec
    if (s_info->timeout_usec != 0) {
      data = 0;
      timeout_n = (s_info->timeout_usec +
                   (PIPE_MGR_TOF3_MIRROR_COAL_DEF_BASE_TIME - 1)) /
                  PIPE_MGR_TOF3_MIRROR_COAL_DEF_BASE_TIME;
      setp_tof3_mirror_coal_to_entry_r_coal_timeout(&data, timeout_n);
      for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
        pipe_mgr_tof3_mirror_buf_reg_wr(
            sess_hdl,
            dev_id,
            pbm,
            MIRROR_REG_ADDR(slice_mem[i].coal_to_cfg.entry[coal_id]),
            data);
      }
    }
    /**norm cfg**/
    sts = tof3_mirror_buf_norm_session_set(
        sess_hdl, dev_id, pbm, sid, coal_id, s_info, enable);
    if (sts != PIPE_SUCCESS) {
      if (new_coal_id) free_coal_id(dev_id, sid, pipe_id);
      return sts;
    }
  } else {
    data = 0;
    setp_tof3_mirror_sess_entry_r_sess_en(&data, 0);
    setp_tof3_mirror_sess_entry_r_coal_en(&data, 0);
    for (i = 0; i < PIPE_MGR_TOF3_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof3_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          pbm,
          MIRROR_REG_ADDR(slice_mem[i].sess_cfg.entry[sid]),
          data);
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_mirror_buf_norm_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr) {
  /* Incase this is a modification of an existing session, check if it is
   * currently a coalescing session. */
  int coal_id = sid_to_coal_id(dev_id, sid, pipe_id);
  pipe_status_t sts;
  sts = tof3_mirror_buf_norm_session_set(
      sess_hdl, dev_id, pbm, sid, 0, session_info, (enable_ing || enable_egr));
  if (sts == PIPE_SUCCESS && coal_id != -1) free_coal_id(dev_id, sid, pipe_id);
  return sts;
}
