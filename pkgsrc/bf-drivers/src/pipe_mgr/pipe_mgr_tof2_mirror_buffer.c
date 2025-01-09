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
#include <tof2_regs/tof2_reg_drv.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include "pipe_mgr_drv.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_tof2_mirror_buffer.h"

#define S_FLD(_s, _f) _s._f
#define MIRROR_REG_ADDR(_name) \
  offsetof(tof2_reg, S_FLD(pipes[0].pardereg.mirreg.mirror, _name))

typedef struct pipe_mgr_tof2_coal_id_info_t {
  uint16_t sid[4];  // 0-255 or 0xFFFF for invalid
} pipe_mgr_tof2_coal_id_info_t;

static pipe_mgr_tof2_coal_id_info_t
    coal_info[PIPE_MGR_NUM_DEVICES][PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX];

static inline void tof2_mirror_buffer_poll_to_drain_sync(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe,
    uint16_t sid,
    rmt_dev_info_t *dev_info);

static inline pipe_status_t tof2_mirror_buf_tbl_selector_set_by_pipe(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable);

static bool pipe_mgr_tof2_is_war_applicable(bf_dev_id_t dev_id) {
  /* The WAR is not applicable for the below scenarios. Both of the below
   * scenarios aligns if the pipe devices is locked.
   * 1. INIT MODE: At intialization there no replayed mirror sessions, hence
   *    no configs mirror APIs will get called. All default configs are handled
   *    by the API "pipe_mgr_tof2_mirror_buf_init" which sets default 0 values
   *    in sessions tbl for all sessions. This API can get called if DMA is
   *    created during INIT. But as no replayed nodes are present, we are
   *    safe. TODO: Put some check to avoid WAR in INIT mode.
   * 2. FAST-RECONFIG: This can get called from FAST-RECONFIG DMA setup during
   *    warm-init-end or during device-add. */
  return !pipe_mgr_is_device_locked(dev_id);
}

static pipe_status_t pipe_mgr_tof2_mirror_buf_reg_wr(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     pipe_bitmap_t pipes,
                                                     uint32_t addr,
                                                     uint32_t data) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  pipe_instr_write_reg_t instr;
  uint32_t stage = ~0;
  pipe_status_t sts;
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
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

static pipe_status_t pipe_mgr_tof2_mirror_buf_reg_wr_pcie(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, uint32_t addr, uint32_t val) {
  pipe_status_t sts = PIPE_SUCCESS;

  sts = pipe_mgr_drv_reg_wr(&sess_hdl, dev_id, 0, addr, val);
  if (sts != PIPE_SUCCESS)
    LOG_ERROR(
        "Dev:%d mirr pcie reg write failed addr:%x data:%d", dev_id, addr, val);
  return sts;
}

pipe_status_t pipe_mgr_tof2_mirror_buf_init_session(pipe_sess_hdl_t shdl,
                                                    bf_mirror_id_t sid,
                                                    bf_dev_id_t dev_id,
                                                    pipe_bitmap_t pbm) {
  pipe_status_t sts;
  for (int slice = 0; slice < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; ++slice) {
    sts = pipe_mgr_tof2_mirror_buf_reg_wr(
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
  sts = pipe_mgr_tof2_mirror_buf_reg_wr(
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

pipe_status_t pipe_mgr_tof2_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id) {
  /* During hitless HA do not reprogram the mirroring tables. */
  if (pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    return PIPE_SUCCESS;
  }

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
  for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
    for (k = 0; k < PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX; k++) {
      pipe_mgr_tof2_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          all_pipes,
          MIRROR_REG_ADDR(slice_mem[i].coal_sess_cfg.entry[k]),
          data);
    }
  }
  data = 0;
  for (k = 0; k < PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX; k++) {
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_0_4),
        data);
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_1_4),
        data);
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_2_4),
        data);
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        all_pipes,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[k].coal_hdr_tbl_3_4),
        data);
  }
  for (j = 0; j < PIPE_MGR_TOF2_MIRROR_SESSION_MAX; j++) {
    pipe_mgr_tof2_mirror_buf_reg_wr(
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
    setp_tof2_mirror_slice_ctrl_logic_pipe(&data, j);
    setp_tof2_mirror_slice_ctrl_cnt_en(&data, 1);
    setp_tof2_mirror_slice_ctrl_ll_reset(&data, 0);
    for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
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
  for (int i = 0; i < PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX; ++i) {
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
  for (int i = 0; i < PIPE_MGR_TOF2_MIRROR_COAL_SESSION_MAX; ++i) {
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

static inline void pipe_mgr_tof2_mirror_sess_cfg_entry_fill(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint16_t sid,
    uint16_t coal_sid,
    pipe_mgr_mirror_session_info_t *s_info,
    uint32_t data[1],
    bool enable) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  if (s_info->dir == BF_DIR_INGRESS || s_info->dir == BF_DIR_BOTH) {
    setp_tof2_mirror_sess_entry_r_ingr_en(&data[0], 1);
  }
  if (s_info->dir == BF_DIR_EGRESS || s_info->dir == BF_DIR_BOTH) {
    setp_tof2_mirror_sess_entry_r_egr_en(&data[0], 1);
  }
  /* The pkt_len field is only 14 bits but max_pkt_len is a 16 bit value.  If
   * we are requesting something with the upper most bits set then ensure all
   * lower bits are also set. */
  /* Tofino-2 includes the Ethernet FCS in the truncation size while Tofino-1
   * does not.  So truncating a 200 byte packet to 128 bytes on Tofino-1 will
   * create a 132 byte packet (128 bytes of header+payload, 4 bytes of FCS)
   * but Tofino-2 will create a 128 byte packet (124 bytes of header+payload,
   * 4 bytes of FCS).  Increase the requested size by four so the two chips
   * behave the same. */
  if ((0x3FFF - 4) < s_info->max_pkt_len) {
    setp_tof2_mirror_sess_entry_r_pkt_len(&data[0], 0xFFFF);
  } else {
    setp_tof2_mirror_sess_entry_r_pkt_len(&data[0], s_info->max_pkt_len + 4);
    /* Tofino-2's mirror block can only truncate on four byte boundaries, log
     * a warning if the requested size is not a multiple of four. */
    if (s_info->max_pkt_len & 3) {
      LOG_WARN(
          "Dev %d pipe-sess:%x mirror session %d: Requested truncation size "
          "(max_pkt_len) "
          "of %d is not a multiple of four, using a size of %d",
          sess_hdl,
          dev_id,
          sid,
          s_info->max_pkt_len,
          s_info->max_pkt_len & 0xFFFC);
    }
  }
  if (s_info->mirror_type == BF_MIRROR_TYPE_COAL) {
    setp_tof2_mirror_sess_entry_r_coal_num(&data[0], coal_sid);
    setp_tof2_mirror_sess_entry_r_coal_en(&data[0], enable);
  } else {
    setp_tof2_mirror_sess_entry_r_coal_num(&data[0], s_info->pri);
  }

  /*slice_mem.sess_cfg: enable,coal_enble. */

  setp_tof2_mirror_sess_entry_r_sess_en(&data[0], 1);
}

static inline void pipe_mgr_tof2_mirr_s2p_sess_cfg_words_fill(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    uint32_t data[5]) {
  bf_dev_id_t dev_id = dev_info->dev_id;

  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
  lld_sku_get_chip_part_revision_number(dev_id, &rev);
  /* For Tofino2-A0 program the physical pipe mask, otherwise program the
   * logical. */
  uint32_t pipe_vec = 0;
  if (s_info->u.mirror_session_entry.pipe_vec) {
    uint32_t n_pipes = pipe_mgr_get_num_active_pipes(dev_id);
    if (rev == BF_SKU_CHIP_PART_REV_A0) {
      for (uint32_t k = 0; k < n_pipes; k++) {
        bf_dev_pipe_t phy_pipe = 0;
        if (~s_info->u.mirror_session_entry.pipe_vec & (1u << k)) continue;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, k, &phy_pipe);
        pipe_vec |= (1 << phy_pipe);
      }
    } else {
      pipe_vec = s_info->u.mirror_session_entry.pipe_vec & ((1 << n_pipes) - 1);
    }
  }

  /* For Tofino2-A0 program the physical ucast port, otherwise program the
   * logical. */
  uint32_t epipe_port = 0;
  if (s_info->u.mirror_session_entry.epipe_port_v) {
    epipe_port = s_info->u.mirror_session_entry.epipe_port;
    if (rev == BF_SKU_CHIP_PART_REV_A0) {
      bf_dev_pipe_t port_pipe = dev_info->dev_cfg.dev_port_to_pipe(epipe_port);
      bf_dev_port_t loc_port =
          dev_info->dev_cfg.dev_port_to_local_port(epipe_port);
      bf_dev_pipe_t phy_pipe = port_pipe;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, port_pipe, &phy_pipe);
      epipe_port = dev_info->dev_cfg.make_dev_port(phy_pipe, loc_port);
    }
  }
  // s2p_regs.sess_entry_word0
  data[0] = 0;
  setp_tof2_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(
      &data[0], s_info->u.mirror_session_entry.hash_cfg_flag_p << 1);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(
      &data[0], s_info->u.mirror_session_entry.hash_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_icos_cfg_f(
      &data[0], s_info->u.mirror_session_entry.icos_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_dod_cfg_f(
      &data[0], s_info->u.mirror_session_entry.dod_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_c2c_cfg_f(
      &data[0], s_info->u.mirror_session_entry.c2c_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_mc_cfg_f(
      &data[0], s_info->u.mirror_session_entry.mc_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_epipe_cfg_f(
      &data[0], s_info->u.mirror_session_entry.epipe_cfg_flag);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_yid_f(
      &data[0], s_info->u.mirror_session_entry.mcast_l2_xid);
  setp_tof2_mirr_s2p_sess_cfg_word0_r_xid_f(
      &data[0], s_info->u.mirror_session_entry.mcast_l1_xid);

  // s2p_regs.sess_entry_word1
  data[1] = 0;
  setp_tof2_mirr_s2p_sess_cfg_word1_r_mcid1_id_f(
      &data[1], s_info->u.mirror_session_entry.mcid1);
  setp_tof2_mirr_s2p_sess_cfg_word1_r_mcid2_id_f(
      &data[1], s_info->u.mirror_session_entry.mcid2);

  // s2p_regs.sess_entry_word2
  data[2] = 0;
  setp_tof2_mirr_s2p_sess_cfg_word2_r_epipe_port_f(&data[2], epipe_port);
  setp_tof2_mirr_s2p_sess_cfg_word2_r_epipe_port_vld_f(
      &data[2], s_info->u.mirror_session_entry.epipe_port_v);
  setp_tof2_mirr_s2p_sess_cfg_word2_r_def_on_drop_f(
      &data[2], s_info->u.mirror_session_entry.deflect_on_drop);
  setp_tof2_mirr_s2p_sess_cfg_word2_r_rid_f(
      &data[2], s_info->u.mirror_session_entry.mcast_rid);

  // s2p_regs.sess_entry_word3
  data[3] = 0;
  setp_tof2_mirr_s2p_sess_cfg_word3_r_hash1_f(
      &data[3], s_info->u.mirror_session_entry.hash1);
  setp_tof2_mirr_s2p_sess_cfg_word3_r_hash2_f(
      &data[3], s_info->u.mirror_session_entry.hash2);
  setp_tof2_mirr_s2p_sess_cfg_word3_r_pipe_vec_f(&data[3], pipe_vec);
  setp_tof2_mirr_s2p_sess_cfg_word3_r_egress_bypass_f(
      &data[3], s_info->u.mirror_session_entry.egr_bypass_flag);

  // s2p_regs.sess_entry_word4
  data[4] = 0;
  setp_tof2_mirr_s2p_sess_cfg_word4_r_icos_f(
      &data[4], s_info->u.mirror_session_entry.icos);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_color_f(
      &data[4], s_info->u.mirror_session_entry.color);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_mcid1_vld_f(
      &data[4], s_info->u.mirror_session_entry.mcid1_vld);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_mcid2_vld_f(
      &data[4], s_info->u.mirror_session_entry.mcid2_vld);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_c2c_cos_f(
      &data[4], s_info->u.mirror_session_entry.c2c_cos);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_c2c_vld_f(
      &data[4], s_info->u.mirror_session_entry.c2c_vld);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_yid_tbl_sel_f(
      &data[4], s_info->u.mirror_session_entry.yid_tbl_sel);
  setp_tof2_mirr_s2p_sess_cfg_word4_r_eport_qid_f(
      &data[4], s_info->u.mirror_session_entry.eport_qid);

  LOG_TRACE(
      "Sess:%d mirr_sess:%d s2p_reg entry-0: %d entry-1: %d entry-2: %d "
      "entry-3: %d entry-4: %d",
      sess_hdl,
      sid,
      data[0],
      data[1],
      data[2],
      data[3],
      data[4]);
}

static pipe_status_t tof2_mirror_buf_norm_session_set_dma(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_bitmap_t pbm,
    uint16_t sid,
    uint16_t coal_sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  int i;
  uint32_t data[5];

  bf_dev_id_t dev_id = dev_info->dev_id;

  /* If mirror is enabled, then fill the session config data locally from
   * the session info. */
  if (enable)
    pipe_mgr_tof2_mirr_s2p_sess_cfg_words_fill(
        sess_hdl, dev_info, sid, s_info, &data[0]);
  else {




    memset(data, 0, sizeof(uint32_t) * 5);
  }

  pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
                                  dev_id,
                                  pbm,
                                  MIRROR_REG_ADDR(s2p_regs.sess_entry_word0),
                                  data[0]);
  pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
                                  dev_id,
                                  pbm,
                                  MIRROR_REG_ADDR(s2p_regs.sess_entry_word1),
                                  data[1]);
  pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
                                  dev_id,
                                  pbm,
                                  MIRROR_REG_ADDR(s2p_regs.sess_entry_word2),
                                  data[2]);
  pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
                                  dev_id,
                                  pbm,
                                  MIRROR_REG_ADDR(s2p_regs.sess_entry_word3),
                                  data[3]);
  pipe_mgr_tof2_mirror_buf_reg_wr(sess_hdl,
                                  dev_id,
                                  pbm,
                                  MIRROR_REG_ADDR(s2p_regs.sess_entry_word4),
                                  data[4]);
  pipe_mgr_tof2_mirror_buf_reg_wr(
      sess_hdl, dev_id, pbm, MIRROR_REG_ADDR(s2p_sess.tbl0[sid]), 0);

  data[0] = 0;
  pipe_mgr_tof2_mirror_sess_cfg_entry_fill(
      sess_hdl, dev_info, sid, coal_sid, s_info, &data[0], enable);
  for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(slice_mem[i].sess_cfg.entry[sid]),
        data[0]);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t tof2_mirror_buf_norm_session_set(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pbm,
    uint16_t sid,
    uint16_t coal_sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  int i;
  uint32_t data[5] = {0};
  pipe_status_t sts = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Failed to get device info, dev %d ", __func__, dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Skip the war if the mirror session is configured during init or
   * during fast reconfig. */
  if (!pipe_mgr_tof2_is_war_applicable(dev_id)) {
    return tof2_mirror_buf_norm_session_set_dma(
        sess_hdl, dev_info, pbm, sid, coal_sid, s_info, enable);
  }











  /* Step-1: Flush if there any pending DMA writes to register. This is done to
   * avoid out of order writes with PCIE register writes. All the reg
   * writes moving ahead are PCIE writes. */
  if (pipe_mgr_sess_in_batch(sess_hdl)) {
    sts = pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Mirror pending flush fails with \"%s\", session %u at %s:%d",
                pipe_str_err(sts),
                sess_hdl,
                __func__,
                __LINE__);
      return sts;
    }
    /* Sync all HW register writes. */
    pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);
  }

  bool is_sw_model = false;
  bf_drv_device_type_get(dev_id, &is_sw_model);

  for (unsigned int log_pipe = 0; log_pipe < dev_info->num_active_pipes;
       ++log_pipe) {
    if (!PIPE_BITMAP_GET(&pbm, log_pipe)) continue;

    bf_dev_pipe_t phy_pipe = 0;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    /* Step-2: Write dprsr config registers via PCIe to disable traffic..  */
    tof2_mirror_buf_tbl_selector_set_by_pipe(
        sess_hdl, dev_info, log_pipe, s_info, false);

    /* Step-3: Wait for traffic to drain
     * NOTE: In model, we skip draining of queues. */
    if (!is_sw_model)
      tof2_mirror_buffer_poll_to_drain_sync(
          sess_hdl, dev_id, log_pipe, sid, dev_info);

    /* If mirror is enabled, then fill the session config data locally from
     * the session info. */
    if (enable)
      pipe_mgr_tof2_mirr_s2p_sess_cfg_words_fill(
          sess_hdl, dev_info, sid, s_info, &data[0]);
    else {




      memset(data, 0, sizeof(uint32_t) * 5);
    }

    /* Step-4: Write to the s2p_sess registers for each of the word using PCIE
     * mode. */
    uint32_t addr;
    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word0);
    sts = pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[0]);

    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word1);
    sts |=
        pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[1]);

    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word2);
    sts |=
        pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[2]);

    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word3);
    sts |=
        pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[3]);

    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word4);
    sts |=
        pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[4]);

    addr = offsetof(tof2_reg,
                    pipes[phy_pipe].pardereg.mirreg.mirror.s2p_sess.tbl0[sid]);
    sts |= pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, 0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "Dev:%d pipe:%d mirr_sess_entry reg_wr failed", dev_id, log_pipe);
      return PIPE_UNEXPECTED;
    }

    /* Step-5: Renable the mirror traffic. */
    tof2_mirror_buf_tbl_selector_set_by_pipe(
        sess_hdl, dev_info, log_pipe, s_info, true);

    /* Fill the mirror sess entry register and write using PCIE. */
    data[0] = 0;
    pipe_mgr_tof2_mirror_sess_cfg_entry_fill(
        sess_hdl, dev_info, sid, coal_sid, s_info, &data[0], enable);

    for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      addr = offsetof(tof2_reg,
                      pipes[phy_pipe]
                          .pardereg.mirreg.mirror.slice_mem[i]
                          .sess_cfg.entry[sid]);
      sts |=
          pipe_mgr_tof2_mirror_buf_reg_wr_pcie(sess_hdl, dev_id, addr, data[0]);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("Dev:%d pipe:%d mirr_entry reg_wr failed", dev_id, log_pipe);
        return PIPE_UNEXPECTED;
      }
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_session_reset(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          uint16_t sid,
                                          bf_dev_pipe_t pipe_id) {
  (void)shdl;
  free_coal_id(dev_info->dev_id, sid, pipe_id);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_mirror_buf_coal_session_update(
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
    setp_tof2_mirror_coal_sess_entry_r_pri(&data, s_info->pri);
    setp_tof2_mirror_coal_sess_entry_r_coal_mode(&data, s_info->coal_mode);
    if (s_info->extract_len_from_p4 == 1)
      setp_tof2_mirror_coal_sess_entry_r_len_cfg(&data,
                                                 s_info->extract_len_from_p4);
    else
      setp_tof2_mirror_coal_sess_entry_r_sample_pkt_len(
          &data,
          ((s_info->extract_len / 4) +
           ((s_info->extract_len % 4 == 0) ? 0 : 1)));
    setp_tof2_mirror_coal_sess_entry_r_coal_hdr(&data,
                                                (s_info->header_len - 8) / 4);
    for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof2_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          pbm,
          MIRROR_REG_ADDR(slice_mem[i].coal_sess_cfg.entry[coal_id]),
          data);
    }
    data = 0;
    setp_tof2_s2p_coal_hdr_r_coal_hdr_tbl_0_4_compiler_flag(
        &data, s_info->header[0] & 0xfful);
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_0_4),
        data);
    data = 0;
    if (s_info->header_len >= 12) {
      // configure s2p_coal.coal_hdl_tbl: header[4]
      setp_tof2_s2p_coal_hdr_r_coal_hdr_tbl_1_4_user_0_3(&data,
                                                         s_info->header[1]);
    }
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_1_4),
        data);
    data = 0;
    if ((s_info->header_len == 16) || (s_info->coal_mode)) {
      // configure s2p_coal.coal_hdl_tbl: header[4]
      setp_tof2_s2p_coal_hdr_r_coal_hdr_tbl_2_4_user_4_7(&data,
                                                         s_info->header[2]);
    }
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_2_4),
        data);
    data = 0;
    setp_tof2_s2p_coal_hdr_r_coal_hdr_tbl_3_4_seq_num(&data, s_info->header[3]);
    pipe_mgr_tof2_mirror_buf_reg_wr(
        sess_hdl,
        dev_id,
        pbm,
        MIRROR_REG_ADDR(s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_3_4),
        data);
    // slice_mem.coal_to_cfg: timeout_usec
    if (s_info->timeout_usec != 0) {
      data = 0;
      timeout_n = (s_info->timeout_usec +
                   (PIPE_MGR_TOF2_MIRROR_COAL_DEF_BASE_TIME - 1)) /
                  PIPE_MGR_TOF2_MIRROR_COAL_DEF_BASE_TIME;
      setp_tof2_mirror_coal_to_entry_r_coal_timeout(&data, timeout_n);
      for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
        pipe_mgr_tof2_mirror_buf_reg_wr(
            sess_hdl,
            dev_id,
            pbm,
            MIRROR_REG_ADDR(slice_mem[i].coal_to_cfg.entry[coal_id]),
            data);
      }
    }
    /**norm cfg**/
    sts = tof2_mirror_buf_norm_session_set(
        sess_hdl, dev_id, pbm, sid, coal_id, s_info, enable);
    if (sts != PIPE_SUCCESS) {
      if (new_coal_id) free_coal_id(dev_id, sid, pipe_id);
      return sts;
    }
  } else {
    data = 0;
    setp_tof2_mirror_sess_entry_r_sess_en(&data, 0);
    setp_tof2_mirror_sess_entry_r_coal_en(&data, 0);
    for (i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      pipe_mgr_tof2_mirror_buf_reg_wr(
          sess_hdl,
          dev_id,
          pbm,
          MIRROR_REG_ADDR(slice_mem[i].sess_cfg.entry[sid]),
          data);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_mirror_buf_norm_session_update(
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
  sts = tof2_mirror_buf_norm_session_set(
      sess_hdl, dev_id, pbm, sid, 0, session_info, (enable_ing || enable_egr));
  if (sts == PIPE_SUCCESS && coal_id != -1) free_coal_id(dev_id, sid, pipe_id);
  return sts;
}

pipe_status_t pipe_mgr_tof2_mirror_buf_session_read(
    pipe_sess_hdl_t shdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t phy_pipe,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool *enable_ing,
    bool *enable_egr,
    bool *session_valid) {
  uint32_t addr;
  /* The following hold the configuration read from the hardware, we will read
   * the hardware first and decode to our SW state afterwards. */
  uint32_t slice_data = 0;
  uint32_t sess_data[6] = {0, 0, 0, 0, 0, 0};
  bool is_coal = false;
  uint32_t coal_sess_cfg = 0, coal_hdr[4] = {0, 0, 0, 0}, coal_to_cfg = 0;

  /* First read s2p_sess.tbl0[sid] to load the session configuration into the
   * sess_entry_word registers, the data read back from s2p_sess.tbl0 can be
   * ignored.  Then read the actual session configuration from the five
   * sess_entry_word registers. */
  uint32_t sess_addrs[6] = {
      offsetof(tof2_reg,
               pipes[phy_pipe].pardereg.mirreg.mirror.s2p_sess.tbl0[sid]),
      offsetof(
          tof2_reg,
          pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word0),
      offsetof(
          tof2_reg,
          pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word1),
      offsetof(
          tof2_reg,
          pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word2),
      offsetof(
          tof2_reg,
          pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word3),
      offsetof(
          tof2_reg,
          pipes[phy_pipe].pardereg.mirreg.mirror.s2p_regs.sess_entry_word4)};
  for (size_t i = 0; i < sizeof sess_addrs / sizeof sess_addrs[0]; ++i) {
    pipe_mgr_drv_reg_rd(&shdl, dev_id, sess_addrs[i], &sess_data[i]);
  }
  /* Read control info from slice_mem[i].sess_cfg.entry[sid] */
  for (int i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
    uint32_t x;
    addr = offsetof(tof2_reg,
                    pipes[phy_pipe]
                        .pardereg.mirreg.mirror.slice_mem[i]
                        .sess_cfg.entry[sid]);
    pipe_mgr_drv_reg_rd(&shdl, dev_id, addr, &x);
    if (i) {
      if (x != slice_data) {
        LOG_ERROR(
            "Dev %d phy_pipe %d session %d: mirror sess_cfg not identical "
            "across slices, slice0 %x slice%d %x",
            dev_id,
            phy_pipe,
            sid,
            slice_data,
            i,
            x);
        return PIPE_UNEXPECTED;
      }
    } else {
      slice_data = x;
    }
  }

  is_coal = getp_tof2_mirror_sess_entry_r_coal_en(&slice_data);
  if (is_coal) {
    /* This is a coalescing session, there is additional configuration which
     * must be read.
     *   slice_mem[i].coal_sess_cfg.entry[coal_id]
     *   slice_mem[i].coal_to_cfg.entry[coal_id]
     *   s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_0_4
     *   s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_1_4
     *   s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_2_4
     *   s2p_coal.coal_hdr_tbl[coal_id].coal_hdr_tbl_3_4 */
    int coal_id = getp_tof2_mirror_sess_entry_r_coal_num(&slice_data);
    /* The coal_sess_cfg should be identical across all slices, read each
     * slice and confirm. */
    for (int i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      uint32_t x;
      addr = offsetof(tof2_reg,
                      pipes[phy_pipe]
                          .pardereg.mirreg.mirror.slice_mem[i]
                          .coal_sess_cfg.entry[coal_id]);
      pipe_mgr_drv_reg_rd(&shdl, dev_id, addr, &x);
      if (i) {
        if (coal_sess_cfg != x) {
          LOG_ERROR(
              "Dev %d phy_pipe %d session %d/%d: mirror coal_sess_cfg not "
              "identical across slices, slice0 %x slice%d %x",
              dev_id,
              phy_pipe,
              sid,
              coal_id,
              coal_sess_cfg,
              i,
              x);
          return PIPE_UNEXPECTED;
        }
      } else {
        coal_sess_cfg = x;
      }
    }

    /* The coal_to_cfg should be identical across all slices, read each slice
     * and confirm. */
    for (int i = 0; i < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; i++) {
      uint32_t x;
      addr = offsetof(tof2_reg,
                      pipes[phy_pipe]
                          .pardereg.mirreg.mirror.slice_mem[i]
                          .coal_to_cfg.entry[coal_id]);
      pipe_mgr_drv_reg_rd(&shdl, dev_id, addr, &x);
      if (i) {
        if (coal_to_cfg != x) {
          LOG_ERROR(
              "Dev %d phy_pipe %d session %d/%d: mirror coal_to_cfg not "
              "identical across slices, slice0 %x slice%d %x",
              dev_id,
              phy_pipe,
              sid,
              coal_id,
              coal_to_cfg,
              i,
              x);
          return PIPE_UNEXPECTED;
        }
      } else {
        coal_to_cfg = x;
      }
    }

    /* The coalescing header is a wide 16 byte register, read each section. */
    addr = offsetof(
        tof2_reg,
        pipes[phy_pipe].pardereg.mirreg.mirror.s2p_coal.coal_hdr_tbl[coal_id]);
    for (int i = 0; i < 4; ++i) {
      pipe_mgr_drv_reg_rd(&shdl, dev_id, addr + 4 * i, coal_hdr + i);
    }
  }

  /* Hardware reads are done, now decode into our SW structure. */
  // uint32_t slice_data = 0;
  // uint32_t sess_data[6] = {0, 0, 0, 0, 0, 0};
  // bool is_coal = false;
  // uint32_t coal_sess_cfg = 0, coal_hdr[4] = {0, 0, 0, 0}, coal_to_cfg = 0;
  bool enabled = getp_tof2_mirror_sess_entry_r_sess_en(&slice_data);
  bool ing = getp_tof2_mirror_sess_entry_r_ingr_en(&slice_data);
  bool egr = getp_tof2_mirror_sess_entry_r_egr_en(&slice_data);
  uint16_t pkt_len = getp_tof2_mirror_sess_entry_r_pkt_len(&slice_data);
  int coal_id_or_pri = getp_tof2_mirror_sess_entry_r_coal_num(&slice_data);

  if (!slice_data && !sess_data[1] && !sess_data[2] && !sess_data[3] &&
      !sess_data[4] && !sess_data[5]) {
    *session_valid = false;
    return PIPE_SUCCESS;
  } else {
    *session_valid = true;
    *enable_ing = ing && enabled;
    *enable_egr = egr && enabled;
    s_info->max_pkt_len = pkt_len;
    if (ing && egr)
      s_info->dir = BF_DIR_BOTH;
    else if (ing)
      s_info->dir = BF_DIR_INGRESS;
    else if (egr)
      s_info->dir = BF_DIR_EGRESS;
    else
      s_info->dir = BF_DIR_NONE;
    s_info->mirror_type = is_coal ? BF_MIRROR_TYPE_COAL : BF_MIRROR_TYPE_NORM;
  }
  /* Decode word0 */
  s_info->u.mirror_session_entry.hash_cfg_flag_p =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(&sess_data[1]) >> 1;
  s_info->u.mirror_session_entry.hash_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_hash_cfg_f(&sess_data[1]) & 1;
  s_info->u.mirror_session_entry.icos_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_icos_cfg_f(&sess_data[1]);
  s_info->u.mirror_session_entry.dod_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_dod_cfg_f(&sess_data[1]);
  s_info->u.mirror_session_entry.c2c_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_c2c_cfg_f(&sess_data[1]);
  s_info->u.mirror_session_entry.mc_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_mc_cfg_f(&sess_data[1]);
  s_info->u.mirror_session_entry.epipe_cfg_flag =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_epipe_cfg_f(&sess_data[1]);
  s_info->u.mirror_session_entry.mcast_l2_xid =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_yid_f(&sess_data[1]);
  s_info->u.mirror_session_entry.mcast_l1_xid =
      getp_tof2_mirr_s2p_sess_cfg_word0_r_xid_f(&sess_data[1]);

  /* Decode word1 */
  s_info->u.mirror_session_entry.mcid1 =
      getp_tof2_mirr_s2p_sess_cfg_word1_r_mcid1_id_f(&sess_data[2]);
  s_info->u.mirror_session_entry.mcid2 =
      getp_tof2_mirr_s2p_sess_cfg_word1_r_mcid2_id_f(&sess_data[2]);

  /* Decode word2 */
  s_info->u.mirror_session_entry.epipe_port_v =
      getp_tof2_mirr_s2p_sess_cfg_word2_r_epipe_port_vld_f(&sess_data[3]);
  s_info->u.mirror_session_entry.epipe_port =
      getp_tof2_mirr_s2p_sess_cfg_word2_r_epipe_port_f(&sess_data[3]);
  s_info->u.mirror_session_entry.deflect_on_drop =
      getp_tof2_mirr_s2p_sess_cfg_word2_r_def_on_drop_f(&sess_data[3]);
  s_info->u.mirror_session_entry.mcast_rid =
      getp_tof2_mirr_s2p_sess_cfg_word2_r_rid_f(&sess_data[3]);

  /* Decode word3 */
  s_info->u.mirror_session_entry.hash1 =
      getp_tof2_mirr_s2p_sess_cfg_word3_r_hash1_f(&sess_data[4]);
  s_info->u.mirror_session_entry.hash2 =
      getp_tof2_mirr_s2p_sess_cfg_word3_r_hash2_f(&sess_data[4]);
  s_info->u.mirror_session_entry.pipe_vec =
      getp_tof2_mirr_s2p_sess_cfg_word3_r_pipe_vec_f(&sess_data[4]);
  s_info->u.mirror_session_entry.egr_bypass_flag =
      getp_tof2_mirr_s2p_sess_cfg_word3_r_egress_bypass_f(&sess_data[4]);

  /* Decode word4 */
  s_info->u.mirror_session_entry.icos =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_icos_f(&sess_data[5]);
  s_info->u.mirror_session_entry.color =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_color_f(&sess_data[5]);
  s_info->u.mirror_session_entry.mcid1_vld =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_mcid1_vld_f(&sess_data[5]);
  s_info->u.mirror_session_entry.mcid2_vld =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_mcid2_vld_f(&sess_data[5]);
  s_info->u.mirror_session_entry.c2c_cos =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_c2c_cos_f(&sess_data[5]);
  s_info->u.mirror_session_entry.c2c_vld =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_c2c_vld_f(&sess_data[5]);
  s_info->u.mirror_session_entry.yid_tbl_sel =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_yid_tbl_sel_f(&sess_data[5]);
  s_info->u.mirror_session_entry.eport_qid =
      getp_tof2_mirr_s2p_sess_cfg_word4_r_eport_qid_f(&sess_data[5]);

  if (!is_coal) {
    s_info->pri = coal_id_or_pri;
  } else {
    s_info->pri = getp_tof2_mirror_coal_sess_entry_r_pri(&coal_sess_cfg);
    s_info->coal_mode =
        getp_tof2_mirror_coal_sess_entry_r_coal_mode(&coal_sess_cfg);
    s_info->extract_len_from_p4 =
        getp_tof2_mirror_coal_sess_entry_r_len_cfg(&coal_sess_cfg);
    s_info->extract_len =
        getp_tof2_mirror_coal_sess_entry_r_sample_pkt_len(&coal_sess_cfg) * 4;
    s_info->header_len =
        getp_tof2_mirror_coal_sess_entry_r_coal_hdr(&coal_sess_cfg) * 4 + 8;

    s_info->header[0] = coal_hdr[0];
    s_info->header[1] = coal_hdr[1];
    s_info->header[2] = coal_hdr[2];
    s_info->header[3] = coal_hdr[3];

    s_info->timeout_usec = coal_to_cfg;
  }
  return PIPE_SUCCESS;
}

bool pipe_mgr_tof2_ha_mirror_buf_cfg_compare(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    bf_mirror_id_t sid,
    pipe_mgr_mirror_session_info_t *sess_info,
    pipe_mgr_mirror_session_info_t *hw_sess_info) {
  pipe_mgr_mirror_tof2_session_entry_t *meta =
      &sess_info->u.mirror_session_entry;
  pipe_mgr_mirror_tof2_session_entry_t *hw_meta =
      &hw_sess_info->u.mirror_session_entry;

  if ((meta->hash_cfg_flag != hw_meta->hash_cfg_flag) ||
      (meta->hash_cfg_flag_p != hw_meta->hash_cfg_flag_p) ||
      (meta->icos_cfg_flag != hw_meta->icos_cfg_flag) ||
      (meta->dod_cfg_flag != hw_meta->dod_cfg_flag) ||
      (meta->c2c_cfg_flag != hw_meta->c2c_cfg_flag) ||
      (meta->mc_cfg_flag != hw_meta->mc_cfg_flag) ||
      (meta->epipe_cfg_flag != hw_meta->epipe_cfg_flag) ||
      (meta->mcast_l2_xid != hw_meta->mcast_l2_xid) ||
      (meta->mcast_l1_xid != hw_meta->mcast_l1_xid)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta0 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }
  if ((meta->mcid1 != hw_meta->mcid1) || (meta->mcid2 != hw_meta->mcid2)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta1 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }
  if ((meta->epipe_port != hw_meta->epipe_port) ||
      (meta->epipe_port_v != hw_meta->epipe_port_v) ||
      (meta->deflect_on_drop != hw_meta->deflect_on_drop) ||
      (meta->mcast_rid != hw_meta->mcast_rid)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta2 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }
  if ((meta->hash1 != hw_meta->hash1) || (meta->hash2 != hw_meta->hash2) ||
      (meta->pipe_vec != hw_meta->pipe_vec) ||
      (meta->egr_bypass_flag != hw_meta->egr_bypass_flag)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta3 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }
  if ((meta->icos != hw_meta->icos) || (meta->color != hw_meta->color) ||
      (meta->mcid1_vld != hw_meta->mcid1_vld) ||
      (meta->mcid2_vld != hw_meta->mcid2_vld) ||
      (meta->c2c_cos != hw_meta->c2c_cos) ||
      (meta->c2c_vld != hw_meta->c2c_vld) ||
      (meta->yid_tbl_sel != hw_meta->yid_tbl_sel) ||
      (meta->eport_qid != hw_meta->eport_qid)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta4 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }
  return true;
}

static inline pipe_status_t tof2_mirror_buf_tbl_selector_set_by_pipe(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  uint32_t ingr_addr, egr_addr;
  uint32_t ingr_data = 0;
  uint32_t egr_data = 0;
  pipe_status_t sts = PIPE_SUCCESS;

  bf_dev_pipe_t phy_pipe = 0;
  bf_dev_id_t dev_id = dev_info->dev_id;

  sts = pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev:%d mirr sess:%x log_pipe:%x log-phy conv failed",
              dev_id,
              sess_hdl,
              log_pipe);
    PIPE_MGR_ASSERT(0);
    return sts;
  }

  ingr_addr = offsetof(
      tof2_reg,
      pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.ipp.ingr.m_mirr_sel);
  egr_addr = offsetof(
      tof2_reg,
      pipes[phy_pipe].pardereg.dprsrreg.dprsrreg.inp.ipp.egr.m_mirr_sel);

  /* Save the original register values while disabling the mirr_sel reg.
   * Restore the saved value while re-enabling the sessions. */
  if (!enable) {
    pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, ingr_addr, &ingr_data);
    pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, egr_addr, &egr_data);

    s_info->old_ingr_sel_val[log_pipe] = ingr_data;
    s_info->old_egr_sel_val[log_pipe] = egr_data;

    setp_tof2_dprsr_input_ingress_only_g_m_mirr_sel_disable(&ingr_data, 1);
    setp_tof2_dprsr_input_egress_only_g_m_mirr_sel_disable(&egr_data, 1);
  } else {
    ingr_data = s_info->old_ingr_sel_val[log_pipe];
    egr_data = s_info->old_egr_sel_val[log_pipe];
  }

  sts = pipe_mgr_tof2_mirror_buf_reg_wr_pcie(
      sess_hdl, dev_id, ingr_addr, ingr_data);
  sts |= pipe_mgr_tof2_mirror_buf_reg_wr_pcie(
      sess_hdl, dev_id, egr_addr, egr_data);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev:%d pipe:%d mirror_sel write failed", dev_id, phy_pipe);
    return sts;
  }
  return sts;
}

static inline void tof2_mirror_buffer_poll_to_drain_sync(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t log_pipe,
    uint16_t sid,
    rmt_dev_info_t *dev_info) {
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

  /* Iterate through each buffer register to get the usage count and poll
   * on it until the usage count reduces to zero. This is blocking poll.
   */
  for (int slice = 0; slice < PIPE_MGR_TOF2_MIRROR_SLICE_MAX; slice++) {
    for (int idx = 0; idx < PIPE_MGR_TOF2_MIRROR_BUFFER_USAGE_PER_SLICE_MAX;
         idx++) {
      uint32_t usage_cnt = 0, loop_cnt = 256;
      uint32_t addrs = offsetof(tof2_reg,
                                pipes[phy_pipe]
                                    .pardereg.mirreg.mirror.slice_regs[slice]
                                    .dbuff_cnt[idx]);
      /* Due to pre-cached 4 entries from buffer free list, we consider 4
       * as the default empty value instead of zero.
       * TODO: Check if we can read register in non blocking way. */
      do {
        pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, addrs, &usage_cnt);
      } while ((usage_cnt > 4) && --loop_cnt);

      if (!loop_cnt && (usage_cnt > 4))
        LOG_ERROR(
            "Dev:%d Mirr Sess:%d pipe:%d  slice:%d usage_count:%d was not zero",
            dev_info->dev_id,
            sid,
            log_pipe,
            idx,
            usage_cnt - 4);
    }
  }
}
