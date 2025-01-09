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
#include <tofino_regs/tofino.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include "pipe_mgr_drv.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_tof_mirror_buffer.h"
#include <netinet/in.h>

#define PIPE_MGR_TOF_MIRROR_MAX_TRUNC_SIZE \
  (63 * 240)  // upto 63 entries of 240B
              // is supported.

// S_FLD is just an indirection to get around concatenation problem
#define S_FLD(_s, _f) _s._f

#define mirror_buf_glb_grp_reg_wr(_name)                                       \
  static pipe_status_t mirror_buf_glb_grp_reg_wr_##_name(                      \
      pipe_sess_hdl_t sess_hdl,                                                \
      bf_dev_id_t dev_id,                                                      \
      pipe_bitmap_t pipes,                                                     \
      uint32_t data) {                                                         \
    uint32_t addr;                                                             \
    rmt_dev_info_t *_dev_info_ = pipe_mgr_get_dev_info(dev_id);                \
    if (!_dev_info_) {                                                         \
      LOG_ERROR(                                                               \
          "%s:%d Failed to get device %d info", __func__, __LINE__, dev_id);   \
      return PIPE_INVALID_ARG;                                                 \
    }                                                                          \
                                                                               \
    addr = offsetof(                                                           \
        Tofino,                                                                \
        S_FLD(pipes[0].deparser.mirror.mir_buf_regs.mir_glb_group, _name));    \
                                                                               \
    pipe_instr_write_reg_t instr;                                              \
    construct_instr_reg_write(dev_id, &instr, addr, data);                     \
    uint32_t dprsr_stage = ~0;                                                 \
    lld_err_t lld_sts;                                                         \
    if (LLD_OK != (lld_sts = lld_sku_get_dprsr_stage(dev_id, &dprsr_stage))) { \
      LOG_ERROR("%s:%d Failed to get deprsr stage for dev %d, sts %d",         \
                __func__,                                                      \
                __LINE__,                                                      \
                dev_id,                                                        \
                lld_sts);                                                      \
      return PIPE_INVALID_ARG;                                                 \
    }                                                                          \
    return pipe_mgr_drv_ilist_add(&sess_hdl,                                   \
                                  _dev_info_,                                  \
                                  &pipes,                                      \
                                  dprsr_stage,                                 \
                                  (uint8_t *)&instr,                           \
                                  sizeof(instr));                              \
  }

#define mirror_buf_norm_desc_grp_reg_wr(_name)                                 \
  static pipe_status_t mirror_buf_norm_desc_grp_reg_wr_##_name(                \
      pipe_sess_hdl_t sess_hdl,                                                \
      bf_dev_id_t dev_id,                                                      \
      pipe_bitmap_t pipes,                                                     \
      uint16_t sid,                                                            \
      uint32_t data) {                                                         \
    uint32_t addr;                                                             \
    rmt_dev_info_t *_dev_info_ = pipe_mgr_get_dev_info(dev_id);                \
    if (!_dev_info_) {                                                         \
      LOG_ERROR(                                                               \
          "%s:%d Failed to get device %d info", __func__, __LINE__, dev_id);   \
      return PIPE_INVALID_ARG;                                                 \
    }                                                                          \
                                                                               \
    addr = offsetof(                                                           \
        Tofino,                                                                \
        S_FLD(pipes[0].deparser.mirror.mir_buf_desc.norm_desc_grp[sid],        \
              _name));                                                         \
                                                                               \
    pipe_instr_write_reg_t instr;                                              \
    construct_instr_reg_write(dev_id, &instr, addr, data);                     \
    uint32_t dprsr_stage = ~0;                                                 \
    lld_err_t lld_sts;                                                         \
    if (LLD_OK != (lld_sts = lld_sku_get_dprsr_stage(dev_id, &dprsr_stage))) { \
      LOG_ERROR("%s:%d Failed to get deprsr stage for dev %d, sts %d",         \
                __func__,                                                      \
                __LINE__,                                                      \
                dev_id,                                                        \
                lld_sts);                                                      \
      return PIPE_INVALID_ARG;                                                 \
    }                                                                          \
    return pipe_mgr_drv_ilist_add(&sess_hdl,                                   \
                                  _dev_info_,                                  \
                                  &pipes,                                      \
                                  dprsr_stage,                                 \
                                  (uint8_t *)&instr,                           \
                                  sizeof(instr));                              \
  }

#define mirror_buf_norm_desc_grp_reg_rd(_name)                                \
  static pipe_status_t mirror_buf_norm_desc_grp_reg_rd_##_name(               \
      pipe_sess_hdl_t sess_hdl,                                               \
      bf_dev_id_t dev_id,                                                     \
      bf_dev_pipe_t pipe_id,                                                  \
      uint16_t sid,                                                           \
      uint32_t *data) {                                                       \
    uint32_t addr;                                                            \
                                                                              \
    addr = offsetof(                                                          \
        Tofino,                                                               \
        S_FLD(pipes[pipe_id].deparser.mirror.mir_buf_desc.norm_desc_grp[sid], \
              _name));                                                        \
                                                                              \
    return pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, addr, data);                \
  }

#define mirror_buf_coal_desc_grp_reg_wr(_name)                               \
  static pipe_status_t mirror_buf_coal_desc_grp_reg_wr_##_name(              \
      pipe_sess_hdl_t sess_hdl,                                              \
      bf_dev_id_t dev_id,                                                    \
      pipe_bitmap_t pipes,                                                   \
      uint8_t cid,                                                           \
      uint32_t data) {                                                       \
    uint32_t addr;                                                           \
    rmt_dev_info_t *_dev_info_ = pipe_mgr_get_dev_info(dev_id);              \
    if (!_dev_info_) {                                                       \
      LOG_ERROR(                                                             \
          "%s:%d Failed to get device %d info", __func__, __LINE__, dev_id); \
      return PIPE_INVALID_ARG;                                               \
    }                                                                        \
                                                                             \
    addr = offsetof(                                                         \
        Tofino,                                                              \
        S_FLD(pipes[0].deparser.mirror.mir_buf_regs.coal_desc_grp[cid],      \
              _name));                                                       \
                                                                             \
    pipe_instr_write_reg_t instr;                                            \
    construct_instr_reg_write(dev_id, &instr, addr, data);                   \
    uint32_t deprsr_stage = ~0;                                              \
    lld_err_t lld_sts;                                                       \
    if (LLD_OK !=                                                            \
        (lld_sts = lld_sku_get_dprsr_stage(dev_id, &deprsr_stage))) {        \
      LOG_ERROR("%s:%d Failed to get deprsr stage for dev %d, sts %d",       \
                __func__,                                                    \
                __LINE__,                                                    \
                dev_id,                                                      \
                lld_sts);                                                    \
      return PIPE_INVALID_ARG;                                               \
    }                                                                        \
    return pipe_mgr_drv_ilist_add(&sess_hdl,                                 \
                                  _dev_info_,                                \
                                  &pipes,                                    \
                                  deprsr_stage,                              \
                                  (uint8_t *)&instr,                         \
                                  sizeof(instr));                            \
  }

#define mirror_buf_coal_desc_grp_reg_rd(_name)                                \
  static pipe_status_t mirror_buf_coal_desc_grp_reg_rd_##_name(               \
      pipe_sess_hdl_t sess_hdl,                                               \
      bf_dev_id_t dev_id,                                                     \
      bf_dev_pipe_t pipe_id,                                                  \
      uint8_t cid,                                                            \
      uint32_t *data) {                                                       \
    uint32_t addr;                                                            \
                                                                              \
    addr = offsetof(                                                          \
        Tofino,                                                               \
        S_FLD(pipes[pipe_id].deparser.mirror.mir_buf_regs.coal_desc_grp[cid], \
              _name));                                                        \
                                                                              \
    return pipe_mgr_drv_reg_rd(&sess_hdl, dev_id, addr, data);                \
  }

// clang-format off
// create register write functions for global group
mirror_buf_glb_grp_reg_wr(glb_ctrl)
mirror_buf_glb_grp_reg_wr(mir_watermark_drop)
mirror_buf_glb_grp_reg_wr(neg_mirr_ctrl)
// mirror_buf_glb_grp_reg_wr(neg_mirr_wtmk)
mirror_buf_glb_grp_reg_wr(coalescing_basetime)
mirror_buf_glb_grp_reg_wr(coalescing_baseid)
// mirror_buf_glb_grp_reg_wr(session_fifonum)
mirror_buf_glb_grp_reg_wr(norm_bank_entries)
mirror_buf_glb_grp_reg_wr(tot_bank_entries)
// mirror_buf_glb_grp_reg_wr(mir_int_stat)
// mirror_buf_glb_grp_reg_wr(mir_int_en)
// mirror_buf_glb_grp_reg_wr(mir_int_pri)
// mirror_buf_glb_grp_reg_wr(mir_int_dual_inj)
// mirror_buf_glb_grp_reg_wr(mir_int_sngl_inj)
// mirror_buf_glb_grp_reg_wr(mir_addr_err_dbuf)
// mirror_buf_glb_grp_reg_wr(mir_addr_err_idesc)
// mirror_buf_glb_grp_reg_wr(mir_addr_err_edesc)
// mirror_buf_glb_grp_reg_wr(mir_addr_err_odesc)
// mirror_buf_glb_grp_reg_wr(mir_addr_err_ptrff)
// mirror_buf_glb_grp_reg_wr(disable_mir_err)
// mirror_buf_glb_grp_reg_wr(stall_drop_stat)
// mirror_buf_glb_grp_reg_wr(ingr_pktdrop)
// mirror_buf_glb_grp_reg_wr(egr_pktdrop)
// mirror_buf_glb_grp_reg_wr(neg_pktdrop)
// mirror_buf_glb_grp_reg_wr(coal_pktdrop[0x8])
// mirror_buf_glb_grp_reg_wr(coal_cred_used[0x8])
// mirror_buf_glb_grp_reg_wr(negm_cred_used)
// mirror_buf_glb_grp_reg_wr(dft_csr)
mirror_buf_glb_grp_reg_wr(min_bcnt)

// create register write functions for norm_desc group
mirror_buf_norm_desc_grp_reg_wr(session_meta0)
mirror_buf_norm_desc_grp_reg_wr(session_meta1)
mirror_buf_norm_desc_grp_reg_wr(session_meta2)
mirror_buf_norm_desc_grp_reg_wr(session_meta3)
mirror_buf_norm_desc_grp_reg_wr(session_meta4)
mirror_buf_norm_desc_grp_reg_wr(session_ctrl)
// create register read functions for norm_desc group
mirror_buf_norm_desc_grp_reg_rd(session_meta0)
mirror_buf_norm_desc_grp_reg_rd(session_meta1)
mirror_buf_norm_desc_grp_reg_rd(session_meta2)
mirror_buf_norm_desc_grp_reg_rd(session_meta3)
mirror_buf_norm_desc_grp_reg_rd(session_meta4)
mirror_buf_norm_desc_grp_reg_rd(session_ctrl)

// create register write functions for coal_desc group
mirror_buf_coal_desc_grp_reg_wr(coal_ctrl0)
mirror_buf_coal_desc_grp_reg_wr(coal_ctrl1)
mirror_buf_coal_desc_grp_reg_wr(coal_ctrl2)
mirror_buf_coal_desc_grp_reg_wr(coal_pkt_header0)
mirror_buf_coal_desc_grp_reg_wr(coal_pkt_header1)
mirror_buf_coal_desc_grp_reg_wr(coal_pkt_header2)
mirror_buf_coal_desc_grp_reg_wr(coal_pkt_header3)
// create register read functions for coal_desc group
mirror_buf_coal_desc_grp_reg_rd(coal_ctrl0)
mirror_buf_coal_desc_grp_reg_rd(coal_ctrl1)
mirror_buf_coal_desc_grp_reg_rd(coal_ctrl2)
mirror_buf_coal_desc_grp_reg_rd(coal_pkt_header0)
mirror_buf_coal_desc_grp_reg_rd(coal_pkt_header1)
mirror_buf_coal_desc_grp_reg_rd(coal_pkt_header2)
mirror_buf_coal_desc_grp_reg_rd(coal_pkt_header3)

pipe_status_t pipe_mgr_tof_mirror_buf_init_session(pipe_sess_hdl_t sess_hdl,
                                                   bf_mirror_id_t sid,
                                                   bf_dev_id_t dev_id,
                                                   pipe_bitmap_t pbm) {
  // clang-format on
  pipe_status_t sts = PIPE_SUCCESS;

  sts = mirror_buf_norm_desc_grp_reg_wr_session_ctrl(
      sess_hdl, dev_id, pbm, sid, 0);
  sts |= mirror_buf_norm_desc_grp_reg_wr_session_meta0(
      sess_hdl, dev_id, pbm, sid, 0);
  sts |= mirror_buf_norm_desc_grp_reg_wr_session_meta1(
      sess_hdl, dev_id, pbm, sid, 0);
  sts |= mirror_buf_norm_desc_grp_reg_wr_session_meta2(
      sess_hdl, dev_id, pbm, sid, 0);
  sts |= mirror_buf_norm_desc_grp_reg_wr_session_meta3(
      sess_hdl, dev_id, pbm, sid, 0);
  sts |= mirror_buf_norm_desc_grp_reg_wr_session_meta4(
      sess_hdl, dev_id, pbm, sid, 0);

  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("Dev %d failed to initialize mirr sess_tbl for sid %d status %s",
              dev_id,
              sid,
              pipe_str_err(sts));
    return sts;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_mirror_buf_init(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id) {
  pipe_status_t sts = PIPE_SUCCESS;
  uint32_t data;
  uint32_t value = 0;
  pipe_bitmap_t all_pipes;
  uint32_t i = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Failed to get device %d info", __func__, __LINE__, dev_id);
    return PIPE_INVALID_ARG;
  }

  PIPE_BITMAP_INIT(&all_pipes, PIPE_BMP_SIZE);
  for (i = 0; i < pipe_mgr_get_num_active_pipes(dev_id); i++) {
    PIPE_BITMAP_SET(&all_pipes, i);
  }

  /* Check for invalid coal_sessions_num and coal_min */
  if (dev_info->coal_mirror_enable &&
      (dev_info->coal_sessions_num > PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX ||
       dev_info->coal_min > 32)) {
    return PIPE_INVALID_ARG;
  }

  /* TOF-2269, Program norm_bank_entries_norm_entries register before enabling
   * mirror to take effect */
  if (dev_info->coal_mirror_enable) {
    data = 0;
    value = 256 - dev_info->coal_sessions_num * dev_info->coal_min;
    value = (value << 9) | value;
  } else {
    data = 0;
    value = 0x20100;  // 256 entries each for bank0 and bank1
  }

  setp_mir_buf_regs_norm_bank_entries_norm_entries(
      &data, value);  // 256 entries each for bank0 and bank1
  sts = mirror_buf_glb_grp_reg_wr_norm_bank_entries(
      sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    // cannot re-program registers as register writes are failing
    return sts;
  }

  // setup all the global configuration registers with
  // appropriate (reset) values
  // global enable bits are not set, need to set them explicitly
  data = 0;
  setp_mir_buf_regs_glb_ctrl_ingr_ena(&data, 1);
  setp_mir_buf_regs_glb_ctrl_egr_ena(&data, 1);
  setp_mir_buf_regs_glb_ctrl_coalescing_ena(&data, 1);
  setp_mir_buf_regs_glb_ctrl_neg_mirror_ena(&data, 1);
  sts = mirror_buf_glb_grp_reg_wr_glb_ctrl(sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    // cannot re-program registers as register writes are failing
    return sts;
  }




  sts = mirror_buf_glb_grp_reg_wr_mir_watermark_drop(
      sess_hdl, dev_id, all_pipes, 0x18201820);
  if (sts != PIPE_SUCCESS) {
    // cannot re-program registers as register writes are failing
    return sts;
  }

  data = 0;
  setp_mir_buf_regs_tot_bank_entries_tot_entries(
      &data, 0x20100);  // 256 entries each for bank0 and bank1
  sts = mirror_buf_glb_grp_reg_wr_tot_bank_entries(
      sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    // cannot re-program registers as register writes are failing
    return sts;
  }

  // Mirror global register's min pkt size should be set to 16B - this is
  // needed for port mode transition issue workaround
  data = 16;
  sts = mirror_buf_glb_grp_reg_wr_min_bcnt(sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }

  data = 0;
  setp_mir_buf_regs_coalescing_basetime_coal_basetime(
      &data,
      (uint32_t)pipe_mgr_usec_to_bps_clock(
          dev_id, PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME));
  sts = mirror_buf_glb_grp_reg_wr_coalescing_basetime(
      sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  data = 0;
  setp_mir_buf_regs_coalescing_baseid_coal_sid(
      &data, PIPE_MGR_TOF_MIRROR_COAL_BASE_SID);
  sts = mirror_buf_glb_grp_reg_wr_coalescing_baseid(
      sess_hdl, dev_id, all_pipes, data);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  data = 0;
  setp_mir_buf_regs_neg_mirr_ctrl_negmir_sid(&data,
                                             PIPE_MGR_TOF_MIRROR_NEG_SID);
  // use defaults for the rest of the fields
  setp_mir_buf_regs_neg_mirr_ctrl_negmir_max_entries(&data, 0x20);
  setp_mir_buf_regs_neg_mirr_ctrl_negmir_min_entries(&data, 0x08);
  sts = mirror_buf_glb_grp_reg_wr_neg_mirr_ctrl(
      sess_hdl, dev_id, all_pipes, data);
  return sts;
}

bool pipe_mgr_tof_mirror_buf_sid_is_coalescing(uint16_t sid) {
  return ((sid & ~(PIPE_MGR_TOF_MIRROR_COAL_SESSION_MAX - 1)) ==
          PIPE_MGR_TOF_MIRROR_COAL_BASE_SID);
}

static uint8_t pipe_mgr_tof_mirror_buf_sid_to_coal_idx(uint16_t sid) {
  PIPE_MGR_DBGCHK(pipe_mgr_tof_mirror_buf_sid_is_coalescing(sid));
  return (uint8_t)(sid - PIPE_MGR_TOF_MIRROR_COAL_BASE_SID);
}

pipe_status_t pipe_mgr_tof_mirror_buf_coal_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pipes,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool enable) {
  pipe_status_t sts;
  uint8_t cid;
  uint32_t ctrl0, ctrl1, header, ctrl2;
  ctrl0 = ctrl1 = header = ctrl2 = 0;
  uint32_t timeout_n;
  int min_pkt_len = 0, mps_drop = 0;

  LOG_TRACE("%s for sid : %d", __func__, sid);

  if (!pipe_mgr_tof_mirror_buf_sid_is_coalescing(sid)) {
    return PIPE_INVALID_ARG;
  }
  /* Check for invalid header lengths */
  if (s_info->header_len > 4) {
    return PIPE_INVALID_ARG;
  }
  /* Check for invalid timeouts,
     timeout is 8 bit in ASIC but programmed in base units of 100us every tick
   */
  if (s_info->timeout_usec > 25500) {
    return PIPE_INVALID_ARG;
  }

  if ((s_info->extract_len & 0x03) || (s_info->extract_len > 240)) {
    // must be multiple of 4 and cannot be more than 3 cells
    // Max 80B (1 cell) when multiple ports feed into the same session
    return PIPE_INVALID_ARG;
  }
  cid = pipe_mgr_tof_mirror_buf_sid_to_coal_idx(sid);

  // disable the session during update
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_ctrl0(
      sess_hdl, dev_id, pipes, cid, ctrl0);
  if (PIPE_SUCCESS != sts) return sts;

  /* ASIC puts the coal pkt headers in the reverse order:
     (coal_pkt_hdr3, coal_pkt_hdr2, coal_pkt_hdr1, coal_pkt_hdr0)
     Reverse the pkt header programming so that user sees right ordering
  */
  /* Always start from src arr index 3, and start from header_(4-hd_len) in
   * HW to ensure hdr len 2 and 3 are correct
   */
  int hdr_idx = 3;
  header = 0;
  if (s_info->header_len >= 4) {
    setp_mir_buf_regs_coal_pkt_header0_coal_pkt_hdr0(
        &header, htonl(s_info->header[hdr_idx--]));
  }
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_pkt_header0(
      sess_hdl, dev_id, pipes, cid, header);
  if (PIPE_SUCCESS != sts) return sts;
  header = 0;
  if (s_info->header_len >= 3) {
    setp_mir_buf_regs_coal_pkt_header1_coal_pkt_hdr1(
        &header, htonl(s_info->header[hdr_idx--]));
  }
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_pkt_header1(
      sess_hdl, dev_id, pipes, cid, header);
  if (PIPE_SUCCESS != sts) return sts;

  header = 0;
  if (s_info->header_len >= 2) {
    setp_mir_buf_regs_coal_pkt_header2_coal_pkt_hdr2(
        &header, htonl(s_info->header[hdr_idx--]));
  }
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_pkt_header2(
      sess_hdl, dev_id, pipes, cid, header);
  if (PIPE_SUCCESS != sts) return sts;
  header = 0;
  if (s_info->header_len >= 1) {
    setp_mir_buf_regs_coal_pkt_header3_coal_pkt_hdr3(
        &header, htonl(s_info->header[hdr_idx]));
  }
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_pkt_header3(
      sess_hdl, dev_id, pipes, cid, header);
  if (PIPE_SUCCESS != sts) return sts;

  // write control regs at the end.

  /* Write coal_ctrl2 - Set mps-drop = (Max Payload of switch - 240)
     Use max payload specified by user. 240 is cell size
  */
  mps_drop = s_info->max_pkt_len - 240;
  // sanity check
  if (mps_drop < (int)s_info->extract_len) {
    mps_drop = s_info->extract_len;
  }

  setp_mir_buf_regs_coal_ctrl2_coal_mps_drop(&ctrl2, mps_drop);
  sts = mirror_buf_coal_desc_grp_reg_wr_coal_ctrl2(
      sess_hdl, dev_id, pipes, cid, ctrl2);
  if (PIPE_SUCCESS != sts) return sts;

  /* Write coal_ctrl1 */
  setp_mir_buf_regs_coal_ctrl1_coal_extract_length(&ctrl1, s_info->extract_len);
  setp_mir_buf_regs_coal_ctrl1_coal_sflow_type(&ctrl1,
                                               s_info->extract_len_from_p4);
  /* Coalescing mode - A0 or B0. Force to A0 mode for now  */
  int coal_mode = 0;
  setp_mir_buf_regs_coal_ctrl1_coal_ext_mode(&ctrl1, coal_mode);

  // program coal min/max credit for coalescing mirror.
  // Set coal min to 16 to avoid sending smaller coal pkts
  setp_mir_buf_regs_coal_ctrl1_coal_min(&ctrl1, 16);
  setp_mir_buf_regs_coal_ctrl1_coal_max(&ctrl1, 56);

  sts = mirror_buf_coal_desc_grp_reg_wr_coal_ctrl1(
      sess_hdl, dev_id, pipes, cid, ctrl1);
  if (PIPE_SUCCESS != sts) return sts;

  // enable the session
  if (enable) {
    setp_mir_buf_regs_coal_ctrl0_coal_ena(&ctrl0, 1);
  } else {
    setp_mir_buf_regs_coal_ctrl0_coal_ena(&ctrl0, 0);
  }
  setp_mir_buf_regs_coal_ctrl0_coal_pkthdr_length(&ctrl0,
                                                  s_info->header_len * 4);
  // convert timeout in terms of base unit
  // minimum 1 base unit
  timeout_n =
      (s_info->timeout_usec + (PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME - 1)) /
      PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME;
  setp_mir_buf_regs_coal_ctrl0_coal_timeout(&ctrl0, timeout_n);

  // min_pkt_len is used as a threshold to start sending out coal-mirrored
  // copy before the next sample can overrun it.
  // Set the min_pkt_len to max_pkt - (3*240). Set min pkt size slightly
  // smaller to ensure ASIC has a cycle to close the packet and that the coal
  // pkt is less than the max-pkt-len. 240 is cell size.
  min_pkt_len = s_info->max_pkt_len - (3 * 240);
  // sanity check
  if (min_pkt_len < (int)s_info->extract_len) {
    min_pkt_len = s_info->extract_len;
  }
  setp_mir_buf_regs_coal_ctrl0_coal_minpkt_size(&ctrl0, min_pkt_len);
  setp_mir_buf_regs_coal_ctrl0_coal_vid(&ctrl0, s_info->ver);

  sts = mirror_buf_coal_desc_grp_reg_wr_coal_ctrl0(
      sess_hdl, dev_id, pipes, cid, ctrl0);
  if (PIPE_SUCCESS != sts) return sts;
  LOG_TRACE("Write Ctrl [%d]: ctrl0:0x%x, ctrl1:0x%x", sid, ctrl0, ctrl1);

  return sts;
}

pipe_status_t pipe_mgr_tof_mirror_buf_coal_session_read(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *s_info,
    bool *enable,
    bool *session_valid) {
  pipe_status_t sts;
  uint8_t cid;
  uint32_t ctrl0 = 0, ctrl1 = 0, ctrl2 = 0;
  uint32_t header0 = 0, header1 = 0, header2 = 0, header3 = 0;

  LOG_TRACE("%s for sid : %d", __func__, sid);

  if (!pipe_mgr_tof_mirror_buf_sid_is_coalescing(sid)) {
    return PIPE_INVALID_ARG;
  }
  *session_valid = false;
  cid = pipe_mgr_tof_mirror_buf_sid_to_coal_idx(sid);

  sts = mirror_buf_coal_desc_grp_reg_rd_coal_ctrl0(
      sess_hdl, dev_id, pipe_id, cid, &ctrl0);
  if (PIPE_SUCCESS != sts) return sts;
  *enable = getp_mir_buf_regs_coal_ctrl0_coal_ena(&ctrl0) ? true : false;
  s_info->header_len =
      getp_mir_buf_regs_coal_ctrl0_coal_pkthdr_length(&ctrl0) / 4;
  s_info->timeout_usec = (getp_mir_buf_regs_coal_ctrl0_coal_timeout(&ctrl0) *
                          PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME) -
                         PIPE_MGR_TOF_MIRROR_COAL_DEF_BASE_TIME + 1;
  s_info->extract_len = getp_mir_buf_regs_coal_ctrl0_coal_minpkt_size(&ctrl0);
  s_info->ver = getp_mir_buf_regs_coal_ctrl0_coal_vid(&ctrl0);

  int hdr_idx = 3;
  if (s_info->header_len >= 4) {
    sts = mirror_buf_coal_desc_grp_reg_rd_coal_pkt_header0(
        sess_hdl, dev_id, pipe_id, cid, &header0);
    if (PIPE_SUCCESS != sts) return sts;
    s_info->header[hdr_idx--] =
        getp_mir_buf_regs_coal_pkt_header0_coal_pkt_hdr0(&header0);
  }

  if (s_info->header_len >= 3) {
    sts = mirror_buf_coal_desc_grp_reg_rd_coal_pkt_header1(
        sess_hdl, dev_id, pipe_id, cid, &header1);
    if (PIPE_SUCCESS != sts) return sts;
    s_info->header[hdr_idx--] =
        getp_mir_buf_regs_coal_pkt_header1_coal_pkt_hdr1(&header1);
  }
  if (s_info->header_len >= 2) {
    sts = mirror_buf_coal_desc_grp_reg_rd_coal_pkt_header2(
        sess_hdl, dev_id, pipe_id, cid, &header2);
    if (PIPE_SUCCESS != sts) return sts;
    s_info->header[hdr_idx--] =
        getp_mir_buf_regs_coal_pkt_header2_coal_pkt_hdr2(&header2);
  }

  if (s_info->header_len >= 1) {
    sts = mirror_buf_coal_desc_grp_reg_rd_coal_pkt_header3(
        sess_hdl, dev_id, pipe_id, cid, &header3);
    if (PIPE_SUCCESS != sts) return sts;
    s_info->header[hdr_idx] =
        getp_mir_buf_regs_coal_pkt_header3_coal_pkt_hdr3(&header3);
  }

  LOG_TRACE("Read Header [%d]: hdr0:0x%x, hdr1:0x%x, hdr2:0x%x, hdr3:0x%x",
            sid,
            header0,
            header1,
            header2,
            header3);

  sts = mirror_buf_coal_desc_grp_reg_rd_coal_ctrl2(
      sess_hdl, dev_id, pipe_id, cid, &ctrl2);
  if (PIPE_SUCCESS != sts) return sts;

  sts = mirror_buf_coal_desc_grp_reg_rd_coal_ctrl1(
      sess_hdl, dev_id, pipe_id, cid, &ctrl1);
  if (PIPE_SUCCESS != sts) return sts;
  s_info->extract_len =
      getp_mir_buf_regs_coal_ctrl1_coal_extract_length(&ctrl1);
  s_info->extract_len_from_p4 =
      getp_mir_buf_regs_coal_ctrl1_coal_sflow_type(&ctrl1);

  LOG_TRACE("Read Ctrl [%d]: ctrl0:0x%x, ctrl1:0x%x, ctrl2:0x%x",
            sid,
            ctrl0,
            ctrl1,
            ctrl2);

  if (ctrl0 || ctrl1) {
    *session_valid = true;
  }

  return sts;
}

pipe_status_t pipe_mgr_tof_mirror_buf_norm_session_update(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t pipes,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool enable_ing,
    bool enable_egr) {
  pipe_status_t sts;
  uint32_t meta0, meta1, meta2, meta3, meta4, ctrl;
  uint32_t log_port, trunc_size, phy_pipe_mask = 0;
  bf_dev_pipe_t phy_pipe = 0;

  LOG_TRACE("%s for sid:%d, type:%d, dir:%d",
            __func__,
            sid,
            session_info->mirror_type,
            session_info->dir);

  if (pipe_mgr_tof_mirror_buf_sid_is_coalescing(sid)) {
    if (session_info->dir != BF_DIR_EGRESS) {
      LOG_ERROR(
          "Mirror session 0x%x on device %d is a coalescing session and can "
          "only be enabled for egress.",
          sid,
          dev_id);
      return PIPE_INVALID_ARG;
    }
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  // Get the egress port.
  // (Destination ucast_egress_port should use logical pipe# for it
  // to work correctly on skew parts.(not all pipe parts)
  log_port = (session_info->u.mirror_session_meta.ucast_egress_port & 0x1FF);

  /* Convert the multicast pipe mask to the set of all physical pipes.  Note
   * this means the pipe mask will be "all or none" meaning all pipes set or no
   * pipes set; this does not allow a subset of pipes to be programmed. */
  if (session_info->u.mirror_session_meta.pipe_mask) {
    uint32_t n_pipes = pipe_mgr_get_num_active_pipes(dev_id);
    for (uint32_t k = 0; k < n_pipes; k++) {
      if (~session_info->u.mirror_session_meta.pipe_mask & (1u << k)) continue;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, k, &phy_pipe);
      phy_pipe_mask |= (1 << phy_pipe);
    }
  }
  meta0 =
      ((session_info->u.mirror_session_meta.ingress_cos & 0x7) |
       (session_info->u.mirror_session_meta.ucast_egress_port_v
        << 3) | /* egress port valid */
       (log_port << 4) |
       ((session_info->u.mirror_session_meta.egress_port_queue & 0x1F) << 13) |
       ((session_info->u.mirror_session_meta.packet_color & 0x3) << 18) |
       ((phy_pipe_mask & 0xF) << 20) |
       ((session_info->u.mirror_session_meta.level1_mcast_hash & 0xFF) << 24));

  meta1 =
      (((session_info->u.mirror_session_meta.level1_mcast_hash >> 8) & 0x1F) |
       ((session_info->u.mirror_session_meta.level2_mcast_hash & 0x1FFF) << 5) |
       ((session_info->u.mirror_session_meta.mcast_grp_a & 0x3FFFu) << 18));

  meta2 =
      (((session_info->u.mirror_session_meta.mcast_grp_a >> 14) & 0x3) |
       (session_info->u.mirror_session_meta.mcast_grp_a_v ? (0x1 << 2) : 0) |
       ((session_info->u.mirror_session_meta.mcast_grp_b & 0xFFFF) << 3) |
       (session_info->u.mirror_session_meta.mcast_grp_b_v ? (0x1 << 19) : 0) |
       ((uint32_t)session_info->u.mirror_session_meta.mcast_l1_xid & 0xFFF)
           << 20);

  meta3 =
      ((session_info->u.mirror_session_meta.mcast_l1_xid >> 12) & 0xF) |
      ((uint32_t)session_info->u.mirror_session_meta.mcast_l2_xid << 4) |
      ((uint32_t)session_info->u.mirror_session_meta.mcast_rid << 13) |
      ((session_info->u.mirror_session_meta.icos_for_copy_to_cpu & 0x3) << 30);

  meta4 = (((session_info->u.mirror_session_meta.icos_for_copy_to_cpu >> 2) &
            0x01) |
           (session_info->u.mirror_session_meta.copy_to_cpu ? (0x1 << 1) : 0) |
           ((session_info->u.mirror_session_meta.deflect_on_drop & 0x1) << 2));

  // egress enable for E2E and COAL_E2E
  ctrl = 0;
  setp_mir_buf_desc_session_ctrl_norm_ingr_ena(&ctrl, enable_ing);
  setp_mir_buf_desc_session_ctrl_norm_egr_ena(&ctrl, enable_egr);

  // If norm_trunc_size is set to value greater than
  // supported truncated size, mirror doesn't work well.
  // Set trunc size to maximum allowable value that HW supports.
  if (session_info->max_pkt_len > PIPE_MGR_TOF_MIRROR_MAX_TRUNC_SIZE) {
    trunc_size = PIPE_MGR_TOF_MIRROR_MAX_TRUNC_SIZE;
  } else {
    trunc_size = session_info->max_pkt_len;
  }
  setp_mir_buf_desc_session_ctrl_norm_trunc_size(&ctrl, trunc_size);
  // Each entry allow upto 240Bytes of pkt payload (including pkt header).
  // Set number of entries to 56 per HW team recommendation. This limit
  // prevents whole mirror buffer being used by single session.
  setp_mir_buf_desc_session_ctrl_norm_max_entries(&ctrl, 56);

  if (enable_ing || enable_egr) {
    LOG_TRACE("Write meta [%d]:meta0-4:0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
              sid,
              meta0,
              meta1,
              meta2,
              meta3,
              meta4);
    if (!pipe_mgr_is_device_locked(dev_id)) {
      /* Disable the session while the metaX registers are being programmed so
       * no packets are mirrored with a half-way configuration. */
      sts = mirror_buf_norm_desc_grp_reg_wr_session_ctrl(
          sess_hdl, dev_id, pipes, sid, 0);
      if (PIPE_SUCCESS != sts) return sts;
    }
    sts = mirror_buf_norm_desc_grp_reg_wr_session_meta0(
        sess_hdl, dev_id, pipes, sid, meta0);
    if (PIPE_SUCCESS != sts) return sts;
    sts = mirror_buf_norm_desc_grp_reg_wr_session_meta1(
        sess_hdl, dev_id, pipes, sid, meta1);
    if (PIPE_SUCCESS != sts) return sts;
    sts = mirror_buf_norm_desc_grp_reg_wr_session_meta2(
        sess_hdl, dev_id, pipes, sid, meta2);
    if (PIPE_SUCCESS != sts) return sts;
    sts = mirror_buf_norm_desc_grp_reg_wr_session_meta3(
        sess_hdl, dev_id, pipes, sid, meta3);
    if (PIPE_SUCCESS != sts) return sts;
    sts = mirror_buf_norm_desc_grp_reg_wr_session_meta4(
        sess_hdl, dev_id, pipes, sid, meta4);
    if (PIPE_SUCCESS != sts) return sts;
  }
  LOG_TRACE("Write Ctrl [%d]:ctrl:0x%x", sid, ctrl);
  sts = mirror_buf_norm_desc_grp_reg_wr_session_ctrl(
      sess_hdl, dev_id, pipes, sid, ctrl);
  if (PIPE_SUCCESS != sts) return sts;

  return sts;
}

pipe_status_t pipe_mgr_tof_mirror_buf_norm_session_read(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    uint16_t sid,
    pipe_mgr_mirror_session_info_t *session_info,
    bool *enable_ing,
    bool *enable_egr,
    bool *session_valid) {
  pipe_status_t sts;
  uint32_t meta0 = 0, meta1 = 0, meta2 = 0, meta3 = 0, meta4 = 0, ctrl = 0;
  uint32_t trunc_size, phy_pipe_mask = 0;
  uint32_t log_pipe_list = 0;
  bf_dev_pipe_t log_pipe = 0;

  LOG_TRACE("%s for sid:%d, pipe:%d", __func__, sid, pipe_id);

  if (!session_info) return PIPE_INVALID_ARG;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  *session_valid = false;
  /* Read control */
  sts = mirror_buf_norm_desc_grp_reg_rd_session_ctrl(
      sess_hdl, dev_id, pipe_id, sid, &ctrl);
  if (PIPE_SUCCESS != sts) return sts;
  LOG_TRACE("Read Ctrl [%d]:ctrl:0x%x", sid, ctrl);

  *enable_ing = getp_mir_buf_desc_session_ctrl_norm_ingr_ena(&ctrl);
  *enable_egr = getp_mir_buf_desc_session_ctrl_norm_egr_ena(&ctrl);
  trunc_size = getp_mir_buf_desc_session_ctrl_norm_trunc_size(&ctrl);
  session_info->max_pkt_len = trunc_size;

  /* Read session meta */
  sts = mirror_buf_norm_desc_grp_reg_rd_session_meta0(
      sess_hdl, dev_id, pipe_id, sid, &meta0);
  if (PIPE_SUCCESS != sts) return sts;

  session_info->u.mirror_session_meta.ingress_cos = meta0 & 0x7;
  session_info->u.mirror_session_meta.ucast_egress_port_v =
      (meta0 >> 3) & 0x1; /* egress port valid */
  session_info->u.mirror_session_meta.ucast_egress_port = (meta0 >> 4) & 0x1FF;
  session_info->u.mirror_session_meta.egress_port_queue = (meta0 >> 13) & 0x1F;
  session_info->u.mirror_session_meta.packet_color = (meta0 >> 18) & 0x3;
  phy_pipe_mask = (meta0 >> 20) & 0xF;

  /* ASiC has HW pipe mask programmed, convert that to logical pipe list */
  for (uint32_t k = 0; k < 4; k++) {
    if (~phy_pipe_mask & (1u << k)) {
      pipe_mgr_map_phy_pipe_id_to_log_pipe_id(dev_id, k, &log_pipe);
      log_pipe_list |= (1 << log_pipe);
    }
  }
  session_info->u.mirror_session_meta.pipe_mask = ~log_pipe_list;
  session_info->u.mirror_session_meta.level1_mcast_hash = (meta0 >> 24) & 0xFF;

  sts = mirror_buf_norm_desc_grp_reg_rd_session_meta1(
      sess_hdl, dev_id, pipe_id, sid, &meta1);
  if (PIPE_SUCCESS != sts) return sts;
  session_info->u.mirror_session_meta.level1_mcast_hash |= (meta1 & 0x1F) << 8;
  session_info->u.mirror_session_meta.level2_mcast_hash = (meta1 >> 5) & 0x1FFF;
  session_info->u.mirror_session_meta.mcast_grp_a = (meta1 >> 18) & 0x3FFFu;

  sts = mirror_buf_norm_desc_grp_reg_rd_session_meta2(
      sess_hdl, dev_id, pipe_id, sid, &meta2);
  if (PIPE_SUCCESS != sts) return sts;

  session_info->u.mirror_session_meta.mcast_grp_a |= (meta2 & 0x3) << 14;
  session_info->u.mirror_session_meta.mcast_grp_a_v =
      ((meta2 >> 2) & 0x1) ? true : false;
  session_info->u.mirror_session_meta.mcast_grp_b = (meta2 >> 3) & 0xFFFF;
  session_info->u.mirror_session_meta.mcast_grp_b_v =
      ((meta2 >> 19) & 0x1) ? true : false;
  session_info->u.mirror_session_meta.mcast_l1_xid = (meta2 >> 20) & 0xFFF;

  sts = mirror_buf_norm_desc_grp_reg_rd_session_meta3(
      sess_hdl, dev_id, pipe_id, sid, &meta3);
  if (PIPE_SUCCESS != sts) return sts;
  session_info->u.mirror_session_meta.mcast_l1_xid = (meta3 & 0xF) << 12;
  session_info->u.mirror_session_meta.mcast_l2_xid = (meta3 >> 4) & 0x1FF;
  session_info->u.mirror_session_meta.mcast_rid = (meta3 >> 13) & 0xFFFF;
  session_info->u.mirror_session_meta.icos_for_copy_to_cpu =
      (meta3 >> 30) & 0x3;

  sts = mirror_buf_norm_desc_grp_reg_rd_session_meta4(
      sess_hdl, dev_id, pipe_id, sid, &meta4);
  if (PIPE_SUCCESS != sts) return sts;
  session_info->u.mirror_session_meta.icos_for_copy_to_cpu = (meta4 & 0x01)
                                                             << 2;
  session_info->u.mirror_session_meta.copy_to_cpu =
      ((meta4 >> 1) & 0x1) ? true : false;
  session_info->u.mirror_session_meta.deflect_on_drop = (meta4 >> 2) & 0x1;

  LOG_TRACE("Read meta [%d]:meta0-4:0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
            sid,
            meta0,
            meta1,
            meta2,
            meta3,
            meta4);

  /* if there is non-zero data in the session, assume session is valid */
  if (ctrl || meta0 || meta1 || meta2 || meta3 || meta4) {
    *session_valid = true;
  }

  return sts;
}

bool pipe_mgr_tof_ha_mirror_buf_cfg_compare(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t log_pipe,
    bf_mirror_id_t sid,
    pipe_mgr_mirror_session_info_t *sess_info,
    pipe_mgr_mirror_session_info_t *hw_sess_info) {
  pipe_mgr_mirror_tof_session_entry_t *meta = &sess_info->u.mirror_session_meta;
  pipe_mgr_mirror_tof_session_entry_t *hw_meta =
      &hw_sess_info->u.mirror_session_meta;

  if ((meta->ingress_cos != hw_meta->ingress_cos) ||
      (meta->ucast_egress_port_v != hw_meta->ucast_egress_port_v) ||
      (meta->ucast_egress_port != hw_meta->ucast_egress_port) ||
      (meta->egress_port_queue != hw_meta->egress_port_queue) ||
      (meta->packet_color != hw_meta->packet_color) ||
      (meta->level1_mcast_hash != hw_meta->level1_mcast_hash)) {
    LOG_TRACE(
        "Mirror[sid=%d]. log-Pipe %d, Config meta0 does not match (dev %d)",
        sid,
        log_pipe,
        dev_info->dev_id);
    return false;
  }

  if ((meta->level2_mcast_hash != hw_meta->level2_mcast_hash) ||
      (meta->mcast_grp_a != hw_meta->mcast_grp_a) ||
      (meta->mcast_grp_a_v != hw_meta->mcast_grp_a_v) ||
      (meta->mcast_grp_b != hw_meta->mcast_grp_b) ||
      (meta->mcast_grp_b_v != hw_meta->mcast_grp_b_v) ||
      (meta->mcast_l1_xid != hw_meta->mcast_l1_xid)) {
    LOG_TRACE("Mirror[sid=%d]. log-Pipe %d, Config meta1 does not match ",
              sid,
              log_pipe);
    return false;
  }

  if ((meta->mcast_grp_a_v != hw_meta->mcast_grp_a_v) ||
      (meta->mcast_grp_b != hw_meta->mcast_grp_b) ||
      (meta->mcast_grp_b_v != hw_meta->mcast_grp_b_v) ||
      (meta->mcast_l1_xid != hw_meta->mcast_l1_xid)) {
    LOG_TRACE("Mirror[sid=%d]. log-Pipe %d, Config meta2 does not match ",
              sid,
              log_pipe);
    return false;
  }

  if ((meta->mcast_l2_xid != hw_meta->mcast_l2_xid) ||
      (meta->mcast_rid != hw_meta->mcast_rid) ||
      (meta->engress_bypass != hw_meta->engress_bypass) ||
      (meta->icos_for_copy_to_cpu != hw_meta->icos_for_copy_to_cpu)) {
    LOG_TRACE("Mirror[sid=%d]. log-Pipe %d, Config meta3 does not match ",
              sid,
              log_pipe);
    return false;
  }

  if ((meta->copy_to_cpu != hw_meta->copy_to_cpu) ||
      (meta->deflect_on_drop != hw_meta->deflect_on_drop)) {
    LOG_TRACE("Mirror[sid=%d]. log-Pipe %d, Config meta4 does not match ",
              sid,
              log_pipe);
    return false;
  }

  return true;
}
