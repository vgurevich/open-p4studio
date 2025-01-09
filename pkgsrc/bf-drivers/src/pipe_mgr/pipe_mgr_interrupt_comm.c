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


#include <bf_types/bf_types.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_sm.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_select_tbl.h"
#include <pipe_mgr/pipe_mgr_intf.h>

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

/* Wrapper API for reg write */
pipe_status_t pipe_mgr_interrupt_write_register(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                uint32_t reg_addr,
                                                uint32_t reg_data) {
  return pipe_mgr_write_register(dev, subdev, reg_addr, reg_data);
}

/* Wrapper API for reg read */
int pipe_mgr_interrupt_read_register(bf_dev_id_t dev,
                                     uint32_t reg_addr,
                                     uint32_t *reg_data) {
  *reg_data = 0;
  if (pipe_mgr_is_device_locked(dev)) {
    return PIPE_INVALID_ARG;
  }
  return lld_subdev_read_register(dev, 0, reg_addr, reg_data);
}

/* Clear some bits in the enable register */
pipe_status_t pipe_mgr_interrupt_set_enable_val(bf_dev_id_t dev,
                                                bf_subdev_id_t subdev,
                                                uint32_t enable_hi_addr,
                                                uint32_t clear_val) {
  uint32_t enable_hi_val = 0;

  if (clear_val) {
    pipe_mgr_interrupt_read_register(dev, enable_hi_addr, &enable_hi_val);
    enable_hi_val &= ~(clear_val);
    pipe_mgr_interrupt_write_register(
        dev, subdev, enable_hi_addr, enable_hi_val);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_non_pipe_interrupt_set_enable_val(
    bf_dev_id_t dev,
    bf_subdev_id_t subdev,
    uint32_t enable_hi_addr,
    uint32_t clear_val) {
  uint32_t enable_hi_val = 0;

  if (clear_val) {
    pipe_mgr_interrupt_read_register(dev, enable_hi_addr, &enable_hi_val);
    enable_hi_val &= ~(clear_val);
    lld_subdev_write_register(dev, subdev, enable_hi_addr, enable_hi_val);
  }

  return PIPE_SUCCESS;
}

/* Acquire lock and rewrite sram or tcam data */
pipe_status_t pipe_mgr_intr_sram_tcam_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev,
                                                  pipe_tbl_hdl_t tbl_hdl,
                                                  uint64_t phy_addr) {
  int tbl_type = 0;

  if (tbl_hdl == 0) {
    return PIPE_SUCCESS;
  }

  tbl_type = PIPE_GET_HDL_TYPE(tbl_hdl);

  switch (tbl_type) {
    case PIPE_HDL_TYPE_MAT_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_tbl_access(sess_hdl, dev, tbl_hdl, true),
              pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr));
      break;
    case PIPE_HDL_TYPE_ADT_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_tbl_access(sess_hdl, dev, tbl_hdl, true),
              pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr));
      break;
    case PIPE_HDL_TYPE_SEL_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_tbl_access(sess_hdl, dev, tbl_hdl, true),
              pipe_mgr_sram_tcam_ecc_error_correct(dev, phy_addr));
      break;
    case PIPE_HDL_TYPE_STAT_TBL:
      // FIXME
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      break;
    default:
      break;
  }

  return PIPE_SUCCESS;
}

pipe_status_t s2p_ram_sbe_correct(pipe_status_t sess_hdl,
                                  bf_dev_id_t dev,
                                  pipe_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t log_pipe_id,
                                  dev_stage_t stage_id,
                                  int mem_offset) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = log_pipe_id;

  switch (PIPE_GET_HDL_TYPE(tbl_hdl)) {
    case PIPE_HDL_TYPE_SEL_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
              pipe_mgr_sel_sbe_correct(
                  dev, log_pipe_id, stage_id, tbl_hdl, mem_offset));
    case PIPE_HDL_TYPE_STAT_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
              pipe_mgr_stat_tbl_sbe_correct(
                  dev, log_pipe_id, stage_id, tbl_hdl, mem_offset));
    case PIPE_HDL_TYPE_METER_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
              pipe_mgr_meter_tbl_sbe_correct(
                  dev, log_pipe_id, stage_id, tbl_hdl, mem_offset));
    case PIPE_HDL_TYPE_STFUL_TBL:
      RMT_API(sess_hdl,
              0,
              pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
              pipe_mgr_stful_sbe_correct(
                  dev, log_pipe_id, stage_id, tbl_hdl, mem_offset));
    default:
      return PIPE_INVALID_ARG;
  }
}

/* Handle error in gfm */
uint32_t pipe_mgr_gfm_err_handle(bf_dev_id_t dev,
                                 bf_subdev_id_t subdev_id,
                                 uint32_t intr_address,
                                 uint32_t intr_status_val,
                                 uint32_t enable_hi_addr,
                                 uint32_t enable_lo_addr,
                                 void *userdata) {
  (void)enable_hi_addr;
  (void)enable_lo_addr;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  dev_stage_t stage = 0;
  bool ingress = false;
  (void)subdev_id;  // TBD Tf3-fix
  (void)enable_hi_addr;
  (void)enable_lo_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return PIPE_UNEXPECTED;
  }

  /* Get pipe and stage from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;
  stage = userdata_p->stage;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("gfm intr (dev %d, pipe %d, stage %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            stage,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // Status register:
  // pipes[].mau[].dp.intr_status_mau_gfm_hash

  for (bitpos = 0; bitpos < PIPE_MGR_INTR_GFM_NUM_ECC_ERR; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      ingress = (bitpos <= 7);

      /* Report event */
      BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                 dev,
                 pipe,
                 stage,
                 bitpos,
                 BF_ERR_TYPE_PARITY,
                 BF_ERR_BLK_GFM,
                 ingress ? BF_ERR_LOC_GFM_INGRESS : BF_ERR_LOC_GFM_EGRESS,
                 "GFM (Galois Field Matrix) parity error in group %d (%s)",
                 bitpos - (ingress ? 0 : 8),
                 ingress ? "Ingress" : "Egress");
      LOG_TRACE(
          "GFM (Galois Field Matrix) parity err in pipe %d, stage %d, group %d "
          "(%s)",
          pipe,
          stage,
          bitpos - (ingress ? 0 : 8),
          ingress ? "Ingress" : "Egress");

      /* Repair memory */
      pipe_mgr_write_gfm_from_shadow(
          pipe_mgr_ctx->int_ses_hndl, dev, pipe, stage, bitpos, ingress);
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(
      dev, subdev_id, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* log_pipe:0-3; entry 0-15; direction:0-ingress 1-egress
 * Only for Tof2
 */
pipe_status_t pipe_mgr_intr_mirr_tbl_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t dev,
                                                 bf_dev_pipe_t log_pipe,
                                                 uint32_t entry,
                                                 bool direction) {
  return pipe_mgr_write_mirrtbl_entry_from_shadow(
      sess_hdl, dev, log_pipe, entry, direction);
}

pipe_status_t pipe_mgr_intr_map_ram_idle_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     mem_id_t mem_id,
                                                     uint32_t mem_offset) {
  dev_target_t dev_tgt;
  dev_tgt.device_id = dev;
  dev_tgt.dev_pipe_id = pipe;

  RMT_API(sess_hdl,
          0,
          pipe_mgr_verify_pipe_tbl_access(sess_hdl, dev_tgt, tbl_hdl, true),
          pipe_mgr_mat_tbl_reset_idle(
              sess_hdl, dev, tbl_hdl, pipe, stage, mem_id, mem_offset));
}

bool tbl_is_s2p(pipe_tbl_hdl_t hdl) {
  pipe_hdl_type_t t = PIPE_GET_HDL_TYPE(hdl);
  return t == PIPE_HDL_TYPE_SEL_TBL || t == PIPE_HDL_TYPE_STAT_TBL ||
         t == PIPE_HDL_TYPE_METER_TBL || t == PIPE_HDL_TYPE_STFUL_TBL;
}

pipe_status_t pipe_mgr_log_err_evt(bf_error_sev_level_t severity,
                                   bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint8_t stage,
                                   uint64_t address,
                                   bf_error_type_t type,
                                   bf_error_block_t blk,
                                   bf_error_block_location_t loc,
                                   const char *format,
                                   ...) {
  int index = 0;
  char tmp_sr[PIPE_MGR_ERR_EVT_LOG_STR_LEN];
  va_list vargs;

  tmp_sr[0] = '\0';

  va_start(vargs, format);
  vsnprintf(tmp_sr, sizeof(tmp_sr), format, vargs);
  va_end(vargs);

  PIPE_MGR_LOCK(&PIPE_INTR_ERR_EVT_MTX(dev));
  /* Write to a new index */
  PIPE_INTR_ERR_EVT_FRONT(dev) =
      (PIPE_INTR_ERR_EVT_FRONT(dev) + 1) % PIPE_MGR_ERR_EVT_LOG_MAX;
  index = PIPE_INTR_ERR_EVT_FRONT(dev);

  PIPE_INTR_ERR_EVT_LOG(dev, index).valid = true;
  PIPE_INTR_ERR_EVT_LOG(dev, index).severity = severity;
  PIPE_INTR_ERR_EVT_LOG(dev, index).dev = dev;
  PIPE_INTR_ERR_EVT_LOG(dev, index).pipe = pipe;
  PIPE_INTR_ERR_EVT_LOG(dev, index).stage = stage;
  PIPE_INTR_ERR_EVT_LOG(dev, index).address = address;
  PIPE_INTR_ERR_EVT_LOG(dev, index).type = type;
  PIPE_INTR_ERR_EVT_LOG(dev, index).blk = blk;
  PIPE_INTR_ERR_EVT_LOG(dev, index).loc = loc;
  PIPE_MGR_MEMCPY(PIPE_INTR_ERR_EVT_LOG(dev, index).err_string,
                  tmp_sr,
                  PIPE_MGR_ERR_EVT_LOG_STR_LEN);
  PIPE_MGR_UNLOCK(&PIPE_INTR_ERR_EVT_MTX(dev));

  return PIPE_SUCCESS;
}

static char *pipe_intr_loc_to_string(bf_error_block_location_t loc,
                                     char *buf,
                                     int len) {
  switch (loc) {
    case BF_ERR_LOC_NONE:
      snprintf(buf, len, "None");
      break;
    case BF_ERR_LOC_MAU_IDLETIME:
      snprintf(buf, len, "MAU idletime error");
      break;
    case BF_ERR_LOC_MAU_STATEFUL_LOG:
      snprintf(buf, len, "MAU stateful log fifo error");
      break;
    case BF_ERR_LOC_SELECTOR_ALU_ST_MINMAX:
      snprintf(buf, len, "Selector ALU stateful min/max error");
      break;
    case BF_ERR_LOC_SELECTOR_ALU_DIV_BY0:
      snprintf(buf, len, "Selector ALU stateful divide by 0 error");
      break;
    case BF_ERR_LOC_SELECTOR_ALU_SALU_PRED:
      snprintf(buf, len, "Selector ALU stateful predication error");
      break;
    case BF_ERR_LOC_MIRR_POINTER_FIFO:
      snprintf(buf, len, "Mirror Pointer FIFO");
      break;
    case BF_ERR_LOC_MIRR_IG:
      snprintf(buf, len, "Mirror Ingress");
      break;
    case BF_ERR_LOC_MIRR_EG:
      snprintf(buf, len, "Mirror Egress");
      break;
    case BF_ERR_LOC_MIRR_OUT_DESC:
      snprintf(buf, len, "Mirror Out Desc");
      break;
    case BF_ERR_LOC_MIRR_DATA_BUFFER:
      snprintf(buf, len, "Mirror Data Buffer");
      break;
    case BF_ERR_LOC_MIRR_DROP_NEG:
      snprintf(buf, len, "Mirror Drop Negative");
      break;
    case BF_ERR_LOC_MIRR_DROP_COAL:
      snprintf(buf, len, "Mirror Drop Coalescing");
      break;
    case BF_ERR_LOC_MIRR_IG_DIS_SESS:
      snprintf(buf, len, "Mirror Ingress Disable session");
      break;
    case BF_ERR_LOC_MIRR_EG_DIS_SESS:
      snprintf(buf, len, "Mirror Egress Disable session");
      break;
    case BF_ERR_LOC_MIRR_OUT:
      snprintf(buf, len, "Mirror Out FIFO");
      break;
    case BF_ERR_LOC_MIRR_CRC12:
      snprintf(buf, len, "Mirror CRC12");
      break;
    case BF_ERR_LOC_MIRR_SESSION:
      snprintf(buf, len, "Mirror session");
      break;
    case BF_ERR_LOC_MIRR_S2P_CREDIT:
      snprintf(buf, len, "Mirror s2p credit");
      break;
    case BF_ERR_LOC_MIRR_IDPRSR_SOPEOP_MISMATCH:
      snprintf(buf, len, "Mirror idprsr sop eop mismatch");
      break;
    case BF_ERR_LOC_MIRR_EDPRSR_SOPEOP_MISMATCH:
      snprintf(buf, len, "Mirror edprsr sop eop mismatch");
      break;
    case BF_ERR_LOC_MIRR_DATA_MEM:
      snprintf(buf, len, "Mirror data memory");
      break;
    case BF_ERR_LOC_TM_PRE_FIFO:
      snprintf(buf, len, "TM-PRE FIFO");
      break;
    case BF_ERR_LOC_TM_PRE_MIT:
      snprintf(buf, len, "TM-PRE MIT");
      break;
    case BF_ERR_LOC_TM_PRE_LIT0_BM:
      snprintf(buf, len, "TM-PRE LIT0 BM");
      break;
    case BF_ERR_LOC_TM_PRE_LIT1_BM:
      snprintf(buf, len, "TM-PRE LIT1 BM");
      break;
    case BF_ERR_LOC_TM_PRE_LIT0_NP:
      snprintf(buf, len, "TM-PRE LIT0 NP");
      break;
    case BF_ERR_LOC_TM_PRE_LIT1_NP:
      snprintf(buf, len, "TM-PRE LIT1 NP");
      break;
    case BF_ERR_LOC_TM_PRE_PMT0:
      snprintf(buf, len, "TM-PRE PMT0");
      break;
    case BF_ERR_LOC_TM_PRE_PMT1:
      snprintf(buf, len, "TM-PRE PMT1");
      break;
    case BF_ERR_LOC_TM_PRE_RDM:
      snprintf(buf, len, "TM-PRE RDM");
      break;
    case BF_ERR_LOC_TM_PRE_BANKID_MEM:
      snprintf(buf, len, "TM-PRE bank id memory");
      break;
    case BF_ERR_LOC_TM_WAC_PPG_MAP:
      snprintf(buf, len, "TM-WAC PPG map");
      break;
    case BF_ERR_LOC_TM_WAC_DROP_CNT:
      snprintf(buf, len, "TM-WAC drop counter");
      break;
    case BF_ERR_LOC_TM_WAC_PFC_VIS:
      snprintf(buf, len, "TM-WAC PFC VIS");
      break;
    case BF_ERR_LOC_TM_WAC_SCH_FCR:
      snprintf(buf, len, "TM-WAC SCH FCR");
      break;
    case BF_ERR_LOC_TM_WAC_QID_MAP:
      snprintf(buf, len, "TM-WAC QID map");
      break;
    case BF_ERR_LOC_TM_WAC_WAC2QAC:
      snprintf(buf, len, "TM-WAC to QAC");
      break;
    case BF_ERR_LOC_TM_QAC_QUE_DROP:
      snprintf(buf, len, "TM-QAC queue drop");
      break;
    case BF_ERR_LOC_TM_QAC_PORT_DROP:
      snprintf(buf, len, "TM-QAC port drop");
      break;
    case BF_ERR_LOC_TM_QAC_QID_MAP:
      snprintf(buf, len, "TM-QAC qid map");
      break;
    case BF_ERR_LOC_TM_QAC_QAC2PRC:
      snprintf(buf, len, "TM-QAC QAC to PRC");
      break;
    case BF_ERR_LOC_TM_QAC_PRC2PSC:
      snprintf(buf, len, "TM-QAC PRC to PSC");
      break;
    case BF_ERR_LOC_TM_CLC_ENQ_FIFO:
      snprintf(buf, len, "TM-CLC ENQ FIFO");
      break;
    case BF_ERR_LOC_TM_CLC_QAC_FIFO:
      snprintf(buf, len, "TM-CLC QAC FIFO");
      break;
    case BF_ERR_LOC_TM_CLC_PH_FIFO:
      snprintf(buf, len, "TM-CLC PH FIFO");
      break;
    case BF_ERR_LOC_TM_CLC_QAC_PH_FIFO:
      snprintf(buf, len, "TM-CLC QAC PH FIFO");
      break;
    case BF_ERR_LOC_TM_PEX_CLM:
      snprintf(buf, len, "TM-PEX CLM");
      break;
    case BF_ERR_LOC_TM_PEX_PH_FIFO:
      snprintf(buf, len, "TM-PEX PH FIFO");
      break;
    case BF_ERR_LOC_TM_PEX_META_FIFO:
      snprintf(buf, len, "TM-PEX META FIFO");
      break;
    case BF_ERR_LOC_TM_PEX_PH_AFIFO:
      snprintf(buf, len, "TM-PEX PH AFIFO");
      break;
    case BF_ERR_LOC_TM_PEX_DISCARD_FIFO:
      snprintf(buf, len, "TM-PEX DISCARD FIFO");
      break;
    case BF_ERR_LOC_TM_QLC_QLM:
      snprintf(buf, len, "TM-QLC QLM");
      break;
    case BF_ERR_LOC_TM_QLC_SCHDEQ:
      snprintf(buf, len, "TM-QLC SCH DEQ");
      break;
    case BF_ERR_LOC_TM_QLC_PH_FIFO:
      snprintf(buf, len, "TM-QLC PH FIFO");
      break;
    case BF_ERR_LOC_TM_PRC_T3:
      snprintf(buf, len, "TM-PRC T3");
      break;
    case BF_ERR_LOC_TM_PSC_PSM:
      snprintf(buf, len, "TM-PSC PSM");
      break;
    case BF_ERR_LOC_TM_PSC_COMM:
      snprintf(buf, len, "TM-PSC comm");
      break;
    case BF_ERR_LOC_TM_CAA:
      snprintf(buf, len, "TM-CAA");
      break;
    case BF_ERR_LOC_TM_SCH_TDM:
      snprintf(buf, len, "TM-SCH TDM");
      break;
    case BF_ERR_LOC_TM_SCH_UPD_WAC:
      snprintf(buf, len, "TM-SCH upd WAC");
      break;
    case BF_ERR_LOC_TM_SCH_UPD_EDPRSR_ADVFC:
      snprintf(buf, len, "TM-SCH upd edprsr advfc");
      break;
    case BF_ERR_LOC_TM_SCH_Q_MINRATE:
      snprintf(buf, len, "TM-SCH queue minrate");
      break;
    case BF_ERR_LOC_TM_SCH_Q_EXCRATE:
      snprintf(buf, len, "TM-SCH queue excrate");
      break;
    case BF_ERR_LOC_TM_SCH_Q_MAXRATE:
      snprintf(buf, len, "TM-SCH queue maxrate");
      break;
    case BF_ERR_LOC_TM_SCH_L1_MINRATE:
      snprintf(buf, len, "TM-SCH L1 minrate");
      break;
    case BF_ERR_LOC_TM_SCH_L1_EXCRATE:
      snprintf(buf, len, "TM-SCH L1 excrate");
      break;
    case BF_ERR_LOC_TM_SCH_L1_MAXRATE:
      snprintf(buf, len, "TM-SCH L1 maxrate");
      break;
    case BF_ERR_LOC_TM_SCH_P_MAXRATE:
      snprintf(buf, len, "TM-SCH P maxrate");
      break;
    case BF_ERR_LOC_TM_SCH_UPD_PEX:
      snprintf(buf, len, "TM-SCH upd PEX");
      break;
    case BF_ERR_LOC_TM_SCH_UPD_EDPRSR:
      snprintf(buf, len, "TM-SCH upd edprsr");
      break;
    case BF_ERR_LOC_TM_SCH_PEX_CREDIT:
      snprintf(buf, len, "TM-SCH PEX credit");
      break;
    case BF_ERR_LOC_TM_SCH_PEX_MAC_CREDIT:
      snprintf(buf, len, "TM-SCH PEX MAC credit");
      break;
    case BF_ERR_LOC_TM_SCH_Q_WATCHDOG:
      snprintf(buf, len, "TM-SCH queue watchdog");
      break;
    case BF_ERR_LOC_PRSR_ACT_RAM:
      snprintf(buf, len, "Parser Action RAM");
      break;
    case BF_ERR_LOC_PRSR_INP_BUFF:
      snprintf(buf, len, "Parser input buffer");
      break;
    case BF_ERR_LOC_PRSR_OUT_FIFO:
      snprintf(buf, len, "Parser Output fifo");
      break;
    case BF_ERR_LOC_PRSR_TCAM_PARITY:
      snprintf(buf, len, "Parser Tcam parity");
      break;
    case BF_ERR_LOC_PRSR_CSUM:
      snprintf(buf, len, "Parser csum");
      break;
    case BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0:
      snprintf(buf, len, "Deparser Pipe Vector TBL0");
      break;
    case BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL1:
      snprintf(buf, len, "Deparser Pipe Vector TBL1");
      break;
    case BF_ERR_LOC_DEPRSR_MIRRTBL:
      snprintf(buf, len, "Deparser mirror table");
      break;
    case BF_ERR_LOC_DEPRSR_IPKT_MAC:
      snprintf(buf, len, "Deparser ipkt MAC");
      break;
    case BF_ERR_LOC_DEPRSR_CMD_FIFO:
      snprintf(buf, len, "Deparser cmd fifo");
      break;
    case BF_ERR_LOC_DEPRSR_CRED_ERR:
      snprintf(buf, len, "Deparser credit err");
      break;
    case BF_ERR_LOC_DEPRSR_PKTST:
      snprintf(buf, len, "Deparser packet st");
      break;
    case BF_ERR_LOC_DEPRSR_META_FIFO:
      snprintf(buf, len, "Deparser metadata fifo");
      break;
    case BF_ERR_LOC_DEPRSR_PKTHDR:
      snprintf(buf, len, "Deparser packet header");
      break;
    case BF_ERR_LOC_DEPRSR_MIRRHDR:
      snprintf(buf, len, "Deparser mirror header");
      break;
    case BF_ERR_LOC_DEPRSR_TMSCH:
      snprintf(buf, len, "Deparser TM sch");
      break;
    case BF_ERR_LOC_DEPRSR_DATAST:
      snprintf(buf, len, "Deparser data st");
      break;
    case BF_ERR_LOC_DEPRSR_PKTDATA:
      snprintf(buf, len, "Deparser packet data");
      break;
    case BF_ERR_LOC_DEPRSR_ARB_FIFO:
      snprintf(buf, len, "Deparser ARB FIFO");
      break;
    case BF_ERR_LOC_DEPRSR_CTL_CHAN:
      snprintf(buf, len, "Deparser ctl chan");
      break;
    case BF_ERR_LOC_PKTGEN_BUFFER:
      snprintf(buf, len, "Pktgen Buffer");
      break;
    case BF_ERR_LOC_PKTGEN_PFC:
      snprintf(buf, len, "Pktgen PFC");
      break;
    case BF_ERR_LOC_PKTGEN_TBC_FIFO:
      snprintf(buf, len, "Pktgen TBC FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO:
      snprintf(buf, len, "Pktgen ETH CPU port FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO:
      snprintf(buf, len, "Pktgen ebuf P0 FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO:
      snprintf(buf, len, "Pktgen ebuf P1 FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO:
      snprintf(buf, len, "Pktgen ebuf P2 FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO:
      snprintf(buf, len, "Pktgen ebuf P3 FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_APP_EVT:
      snprintf(buf, len, "Pktgen application event");
      break;
    case BF_ERR_LOC_PKTGEN_IPB_CHNL_SEQ:
      snprintf(buf, len, "Pktgen IPB channel sequence");
      break;
    case BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL:
      snprintf(buf, len, "Pktgen ETH CPU port and TBC same channel");
      break;
    case BF_ERR_LOC_PKTGEN_ETH_PORT_FIFO:
      snprintf(buf, len, "Pktgen ETH port FIFO");
      break;
    case BF_ERR_LOC_PKTGEN_PHASE0:
      snprintf(buf, len, "Pktgen PHASE0");
      break;
    case BF_ERR_LOC_GFM_INGRESS:
      snprintf(buf, len, "Galois Field Matrix Ingress");
      break;
    case BF_ERR_LOC_GFM_EGRESS:
      snprintf(buf, len, "Galois Field Matrix Egress");
      break;
    case BF_ERR_LOC_DMA_PBC:
      snprintf(buf, len, "DMA PBC");
      break;
    case BF_ERR_LOC_DMA_CBC:
      snprintf(buf, len, "DMA CBC");
      break;
    case BF_ERR_LOC_DMA_MBC:
      snprintf(buf, len, "DMA MBC");
      break;
    case BF_ERR_LOC_LFLTR_BFT_CLR:
      snprintf(buf, len, "Learn Filter Clear");
      break;
    case BF_ERR_LOC_LFLTR_BFT0:
      snprintf(buf, len, "Learn Filter Bloom Filter 0");
      break;
    case BF_ERR_LOC_LFLTR_BFT1:
      snprintf(buf, len, "Learn Filter Bloom Filter 1");
      break;
    case BF_ERR_LOC_LFLTR_LQT0:
      snprintf(buf, len, "Learn Filter Quanta 0");
      break;
    case BF_ERR_LOC_LFLTR_LQT1:
      snprintf(buf, len, "Learn Filter Quanta 1");
      break;
    case BF_ERR_LOC_LFLTR_LBUF:
      snprintf(buf, len, "Learn Filter Buffer");
      break;
    case BF_ERR_LOC_EBUF:
      snprintf(buf, len, "Egress buffer");
      break;
    default:
      snprintf(buf, len, "Unknown");
      break;
  }
  return buf;
}

static char *pipe_intr_blk_to_string(bf_error_block_t blk, char *buf, int len) {
  switch (blk) {
    case BF_ERR_BLK_NONE:
      snprintf(buf, len, "None");
      break;
    case BF_ERR_BLK_MAU:
      snprintf(buf, len, "MAU");
      break;
    case BF_ERR_BLK_TCAM:
      snprintf(buf, len, "Tcam");
      break;
    case BF_ERR_BLK_SRAM:
      snprintf(buf, len, "Sram");
      break;
    case BF_ERR_BLK_MAP_RAM:
      snprintf(buf, len, "Map-Ram");
      break;
    case BF_ERR_BLK_STATS:
      snprintf(buf, len, "Stats");
      break;
    case BF_ERR_BLK_METERS:
      snprintf(buf, len, "Meters");
      break;
    case BF_ERR_BLK_SYNTH2PORT:
      snprintf(buf, len, "Synth2Port");
      break;
    case BF_ERR_BLK_SELECTOR_ALU:
      snprintf(buf, len, "Selector-ALU");
      break;
    case BF_ERR_BLK_IMEM:
      snprintf(buf, len, "Imem");
      break;
    case BF_ERR_BLK_MIRROR:
      snprintf(buf, len, "Mirror");
      break;
    case BF_ERR_BLK_TM_PRE:
      snprintf(buf, len, "TM-PRE");
      break;
    case BF_ERR_BLK_TM_WAC:
      snprintf(buf, len, "TM-WAC");
      break;
    case BF_ERR_BLK_TM_QAC:
      snprintf(buf, len, "TM-QAC");
      break;
    case BF_ERR_BLK_TM_CLC:
      snprintf(buf, len, "TM-CLC");
      break;
    case BF_ERR_BLK_TM_PEX:
      snprintf(buf, len, "TM-PEX");
      break;
    case BF_ERR_BLK_TM_QLC:
      snprintf(buf, len, "TM-QLC");
      break;
    case BF_ERR_BLK_TM_PRC:
      snprintf(buf, len, "TM-PRC");
      break;
    case BF_ERR_BLK_TM_PSC:
      snprintf(buf, len, "TM-PSC");
      break;
    case BF_ERR_BLK_TM_CAA:
      snprintf(buf, len, "TM-CAA");
      break;
    case BF_ERR_BLK_TM_SCH:
      snprintf(buf, len, "TM-SCH");
      break;
    case BF_ERR_BLK_PRSR:
      snprintf(buf, len, "Parser");
      break;
    case BF_ERR_BLK_DEPRSR:
      snprintf(buf, len, "Deparser");
      break;
    case BF_ERR_BLK_PKTGEN:
      snprintf(buf, len, "Pktgen");
      break;
    case BF_ERR_BLK_GFM:
      snprintf(buf, len, "Galois Field Matrix");
      break;
    case BF_ERR_BLK_DMA:
      snprintf(buf, len, "DMA");
      break;
    case BF_ERR_BLK_LFLTR:
      snprintf(buf, len, "Learn Filter");
      break;
    case BF_ERR_BLK_EBUF:
      snprintf(buf, len, "Egress buffer");
      break;
    default:
      snprintf(buf, len, "Unknown");
      break;
  }
  return buf;
}

static char *pipe_intr_type_to_string(bf_error_type_t type,
                                      char *buf,
                                      int len) {
  switch (type) {
    case BF_ERR_TYPE_GENERIC:
      snprintf(buf, len, "Generic");
      break;
    case BF_ERR_TYPE_SINGLE_BIT_ECC:
      snprintf(buf, len, "Single-Bit ECC");
      break;
    case BF_ERR_TYPE_MULTI_BIT_ECC:
      snprintf(buf, len, "Multi-Bit ECC");
      break;
    case BF_ERR_TYPE_PARITY:
      snprintf(buf, len, "Parity");
      break;
    case BF_ERR_TYPE_OVERFLOW:
      snprintf(buf, len, "FIFO overflow");
      break;
    case BF_ERR_TYPE_UNDERFLOW:
      snprintf(buf, len, "FIFO underflow");
      break;
    case BF_ERR_TYPE_PKT_DROP:
      snprintf(buf, len, "Packet drop");
      break;
    default:
      snprintf(buf, len, "Unknown");
      break;
  }
  return buf;
}

static char *pipe_intr_severity_to_string(bf_error_sev_level_t sev,
                                          char *buf,
                                          int len) {
  switch (sev) {
    case BF_ERR_SEV_CORRECTABLE:
      snprintf(buf, len, "Correctable");
      break;
    case BF_ERR_SEV_NON_CORRECTABLE:
      snprintf(buf, len, "Non-Correctable");
      break;
    case BF_ERR_SEV_FATAL:
      snprintf(buf, len, "Fatal");
      break;
    default:
      snprintf(buf, len, "Unknown");
      break;
  }
  return buf;
}

void pipe_mgr_err_evt_log_dump_by_index(ucli_context_t *uc,
                                        bf_dev_id_t dev,
                                        int index,
                                        int intr_num) {
  char sev_str[100], type_str[100], blk_str[100], loc_str[100];

  if (index >= PIPE_MGR_ERR_EVT_LOG_MAX) {
    return;
  }
  pipe_mgr_err_evt_log_t *log = &(PIPE_INTR_ERR_EVT_LOG(dev, index));

  if (!(PIPE_INTR_ERR_EVT_LOG(dev, index).valid)) {
    return;
  }

  aim_printf(&uc->pvs, "------- Interrupt Num: %d --------\n", intr_num);
  aim_printf(&uc->pvs,
             " Block: %s, Block Location: %s\n",
             pipe_intr_blk_to_string(log->blk, blk_str, sizeof(blk_str)),
             pipe_intr_loc_to_string(log->loc, loc_str, sizeof(loc_str)));
  aim_printf(
      &uc->pvs,
      " Severity: %s, Type: %s\n",
      pipe_intr_severity_to_string(log->severity, sev_str, sizeof(sev_str)),
      pipe_intr_type_to_string(log->type, type_str, sizeof(type_str)));
  aim_printf(&uc->pvs,
             " Pipe: %d, Stage: %d, Address: 0x%" PRIx64 "\n",
             log->pipe,
             log->stage,
             log->address);
  aim_printf(&uc->pvs, " Error string: %s\n", log->err_string);
  return;
}
