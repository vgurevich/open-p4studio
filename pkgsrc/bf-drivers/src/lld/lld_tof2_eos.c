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


/**
 * @file lld_ind_reg_if_tof2.c
 * \brief Details indirect register access APIs
 *
 */

/**
 * @addtogroup lld-reg-api
 * @{
 * This is a description of some APIs.
 */

#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#define bf_sys_assert()
#endif

#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_reg_if.h>
#include "lld.h"
#include "lld_map.h"
#include "lld_log.h"
#include "lld_dev.h"
//#include <lld/lld_sku.h>
#include <tof2_regs/tof2_reg_drv.h>

extern void ucli_log(char *fmt, ...);
extern char *get_full_reg_path_name(bf_dev_id_t dev_id, uint32_t offset);
extern bool eos_verbose;

static void lld_tof2_eos_audit_verify_0(bf_dev_id_t dev_id, uint32_t addr);
static void lld_tof2_eos_audit_verify_exp(bf_dev_id_t dev_id,
                                          uint32_t addr,
                                          uint32_t exp);

/********************************************************************
 * lld_tof2_eos_dprsr_audit
 *
 * Debug cmd handler for dprsr end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_dprsr_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;

  // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inp_->icr_->intr_->stat_,
  // "dprsr_inp_intr");
  addr = offsetof(
      tof2_reg, pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inp_->icr_->intr_b_->stat_,
  // "dprsr_inp_intr");
  addr =
      offsetof(tof2_reg,
               pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  for (int i = 0; i < 4; i++) {
    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inpslice_[i]->intr_->stat_,"dprsr_inpslice_intr");
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inpslice[i].intr.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->hir_->h_->stat_,"dprsr_hdr_ingress_intr");
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_i[i].hir.h.intr.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->her_->h_->stat_,"dprsr_hdr_egress_intr");
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_e[i].her.h.intr.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->out_ingr_->intr_0_->stat_,"dprsr_out_ingress_intr0");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_i[i]
                        .out_ingr.intr_0.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->out_ingr_->intr_1_->stat_,"dprsr_out_ingress_intr1");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_i[i]
                        .out_ingr.intr_1.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->out_egr_->intr_0_->stat_,"dprsr_out_egress_intr0");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_e[i]
                        .out_egr.intr_0.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->out_egr_->intr_1_->stat_,"dprsr_out_egress_intr1");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_e[i]
                        .out_egr.intr_1.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }
}

/********************************************************************
 * lld_tof2_eos_ebuf_audit
 *
 * Debug cmd handler for ebuf end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_ebuf_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
  // uint32_t d;
  uint32_t expt_val_31_0;
  // uint32_t expt_val_32_63;

  expt_val_31_0 = 0x2a;
  // expt_val_32_63 = 0;
  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 8; k++) {
        // m_ebuf_reg[i]->ebuf400reg_[j]->chan_group_[k]->chnl_fifo_stat_->Read();
        // d =
        // m_ebuf_reg[i]->ebuf400reg_[j]->chan_group_[k]->chnl_fifo_stat_->GetData(0);
        // printf("The value read for
        // ebuf_reg[%d]->ebuf400reg[%d]->chan_group[%d] :%d\n", i, j, k, d);
        addr = offsetof(tof2_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ebuf900reg[i]
                            .ebuf400reg[j]
                            .chan_group[k]
                            .chnl_fifo_stat);
        lld_tof2_eos_audit_verify_exp(dev_id, addr, expt_val_31_0);

        // if(d != expt_val_31_0) {
        // bfdv_error("EBUF audit: EOS check failed. Read Value for 31:0 bits =
        // %d , Expected Value = 0x2a for
        // ebuf_reg[%d]->ebuf400reg[%d]->chan_group[%d]", d, i, j, k);
        //}

        // d =
        // m_ebuf_reg[i]->ebuf400reg_[j]->chan_group_[k]->chnl_fifo_stat_->GetData(1);
        // printf("The value read for
        // ebuf_reg[%d]->ebuf400reg[%d]->chan_group[%d] :%d\n", i, j, k, d);
        // if(d != expt_val_32_63) {
        // bfdv_error("EBUF audit: EOS check failed. Read for 63:32 bits Value =
        // %d , Expected Value = 0 for
        // ebuf_reg[%d]->ebuf400reg[%d]->chan_group[%d]", d, i, j, k);
        //}
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int k = 0; k < 2; k++) {
      // m_ebuf_reg[i]->ebuf100reg_->chan_group_[k]->chnl_fifo_stat_->Read();
      // d =
      // m_ebuf_reg[i]->ebuf100reg_->chan_group_[k]->chnl_fifo_stat_->GetData(0);
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ebuf900reg[i]
                          .ebuf100reg.chan_group[k]
                          .chnl_fifo_stat);
      lld_tof2_eos_audit_verify_exp(dev_id, addr, expt_val_31_0);

      // if(d != expt_val_31_0) {
      // bfdv_error("EBUF audit: EOS check failed. Read Value for 31:0 bits =
      // 0x%h , Expected Value = 0x2a for
      // ebuf_reg[%d]->ebuf100reg->chan_group[%d]", d, i, k);
      //}

      // m_ebuf_reg[i]->ebuf100reg_->chan_group_[k]->chnl_fifo_stat_->Read();
      // d =
      // m_ebuf_reg[i]->ebuf100reg_->chan_group_[k]->chnl_fifo_stat_->GetData(1);

      // if(d != expt_val_32_63) {
      // bfdv_error("EBUF audit: EOS check failed. Read for 63:32 bits Value =
      // 0x%h , Expected Value = 0 for
      // ebuf_reg[%d]->ebuf100reg->chan_group[%d]", d, i, k);
      //}
    }
  }

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      // EBUF_REG_READ_N_CHK_FOR_ZERO(m_ebuf_reg[i]->ebuf400reg_[j]->glb_group_->intr_stat_,"ebuf900_ebuf400_intr_stat");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ebuf900reg[i]
                          .ebuf400reg[j]
                          .glb_group.intr_stat);
      lld_tof2_eos_audit_verify_0(dev_id, addr);
    }
  }

  for (int i = 0; i < 4; i++) {
    // EBUF_REG_READ_N_CHK_FOR_ZERO(m_ebuf_reg[i]->ebuf100reg_->glb_group_->intr_stat_,"ebuf900_ebuf100_intr_stat");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ebuf900reg[i]
                        .ebuf100reg.glb_group.intr_stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }
}

/********************************************************************
 * lld_tof2_eos_epb_audit
 *
 * Debug cmd handler for epb end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_epb_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);
  // EPB_REG_READ_N_CHK_FOR_ZERO(EPB_REGS_GLB_GROUP->stat_, "epb_intr");
  for (int i = 0; i < 9; i++) {
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.epbprsr4reg[i]
                        .epbreg.glb_group.intr_stat.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    // long reg
    lld_tof2_eos_audit_verify_0(dev_id, addr + 4);
  }
}

/********************************************************************
 * lld_tof2_eos_ibuf_audit
 *
 * Debug cmd handler for ibuf end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_ibuf_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
  (void)dev_id;
  (void)phys_pipe;
  (void)addr;
#if 0
  for (int chan_id=0; chan_id<4; chan_id++) {
    m_chan_group[chan_id]->chnl_ctrl_->Read();
    m_chan_group[chan_id]->chnl_afull_->Read();
    m_chan_group[chan_id]->chnl_tx_xoff_->Read();
    m_chan_group[chan_id]->chnl_drop_->Read();
    m_chan_group[chan_id]->chnl_metadata_fix_->Read();
    m_chan_group[chan_id]->chnl_metadata_fix2_->Read();
    m_chan_group[chan_id]->chnl_pktnum0_->Read();
    m_chan_group[chan_id]->chnl_pktnum1_->Read();
    m_chan_group[chan_id]->chnl_ptr_fifo_min_max_->Read();
    m_chan_group[chan_id]->chnl_recirc_fifo_min_max_->Read();
    m_chan_group[chan_id]->chnl_deparser_drop_pkt_->Read();
    m_chan_group[chan_id]->chnl_wsch_discard_pkt_->Read();
    m_chan_group[chan_id]->chnl_wsch_trunc_pkt_->Read();
    m_chan_group[chan_id]->chnl_recirc_discard_pkt_->Read();
    m_chan_group[chan_id]->chnl_parser_discard_pkt_->Read();
    m_chan_group[chan_id]->chnl_parser_send_pkt_->Read();
    m_chan_group[chan_id]->chnl_deparser_send_pkt_->Read();
    m_chan_group[chan_id]->chnl_macs_received_pkt_->Read();
    m_chan_group[chan_id]->chnl_recirc_received_pkt_->Read();
  }

  IBUF_REGS_GLB_GROUP->glb_ctrl_->Read();
  IBUF_REGS_GLB_GROUP->glb_parser_maxbyte_->Read();
  IBUF_REGS_GLB_GROUP->glb_meta_avail_->Read();
  IBUF_REGS_GLB_GROUP->bank_watermark_afull_->Read();
  IBUF_REGS_GLB_GROUP->bank_watermark_tx_xoff_->Read();
  IBUF_REGS_GLB_GROUP->bank_watermark_drop_->Read();
  IBUF_REGS_GLB_GROUP->int_stat_->Read();
  IBUF_REGS_GLB_GROUP->int_en_->Read();
  IBUF_REGS_GLB_GROUP->int_pri_->Read();
  IBUF_REGS_GLB_GROUP->int_inj_->Read();
  IBUF_REGS_GLB_GROUP->glb_err_addr0_->Read();
  IBUF_REGS_GLB_GROUP->glb_err_addr1_->Read();
  IBUF_REGS_GLB_GROUP->glb_err_addr2_->Read();

  m_sknobs_name = m_name+".ibuf_unit_tb";
  if (sknobs_get_value((char*)m_sknobs_name.c_str(), 0)) {
    IBUF_REGS_GLB_GROUP->indmfree_cnt_->Read();
    for (int i=0; i<16; i++) {
      IBUF_REGS_GLB_GROUP->indmfree_reg_[i]->Read();
    }
    IBUF_REGS_GLB_GROUP->freelist_cnt_[0]->Read();
    IBUF_REGS_GLB_GROUP->freelist_cnt_[1]->Read();
    IBUF_REGS_GLB_GROUP->freelist_cnt_[2]->Read();
    IBUF_REGS_GLB_GROUP->freelist_cnt_[3]->Read();
    IBUF_REGS_GLB_GROUP->dft_csr_->Read();
    IBUF_REGS_GLB_GROUP->tim_off_->Read();
    for (int i=0; i<64; i++) {
      IBUF_REGS_GLB_GROUP->freelist_reg_[i]->Read();
    }
  }
#endif  // 0
}

/********************************************************************
 * lld_tof2_eos_ipb_audit
 *
 * Debug cmd handler for ipb end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_ipb_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
#define NUM_IPB 9

  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);
  for (int i = 0; i < 9; i++) {
    // IPB_REG_READ_N_CHK_FOR_BC(IPB_REGS_GLB_GROUP->glb_status_,
    // "free_list_used_cnt",3705461980);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.glb_group.glb_status);
    lld_tof2_eos_audit_verify_exp(dev_id, addr, 3705461980);
  }
  for (int i = 0; i < 8; i++) {
    // IPB_REG_READ_N_CHK_FOR_ZERO(IPB_REGS_GLB_GROUP->stat_, "ipb_intr");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.glb_group.intr_stat.stat);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    // wide reg
    lld_tof2_eos_audit_verify_0(dev_id, addr + 4);
  }

  for (int i = 0; i < NUM_IPB; i++) {
    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_deparser_drop_pkt_,"chnl_dprs_drop_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_deparser_drop_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_wsch_trunc_pkt_,"chnl_wsch_trunc_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_wsch_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_wsch_discard_pkt_,"chnl_wsch_discard_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_wsch_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_resubmit_discard_pkt_,"chnl_resubmit_discard_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_resubmit_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_parser_discard_pkt_,"chnl_parser_discard_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_parser_discard_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_drop_trunc_pkt_,"chnl_drop_trunc_pkt");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_drop_trunc_pkt);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_metanum_,"chnl_metanum");
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_metanum);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }
}

/********************************************************************
 * lld_tof2_eos_mau_audit
 *
 * Debug cmd handler for mau end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_mau_audit(bf_dev_id_t dev_id,
                            uint32_t phys_pipe,
                            uint32_t stage) {
  uint32_t addr;
  // char s[100];
  // uint32_t d;

#define PHYSICAL_UNITRAM_ROWS 8
#define PHYSICAL_UNITRAMS_PER_ROW 12

  // cout << "This is " << get_name() << " Audit\n";
  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->intr_status_mau_imem_,
  // "intr_status_mau_imem");
  addr =
      offsetof(tof2_reg, pipes[phys_pipe].mau[stage].dp.intr_status_mau_imem);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->intr_status_mau_snapshot_,
  // "intr_status_mau_snapshot");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].mau[stage].dp.intr_status_mau_snapshot);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  //  MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->xbar_hash_->hash_->intr_status_mau_gfm_hash_,
  //  "intr_status_mau_gfm_hash");
  // addr = offsetof(tof2_reg,
  // pipes[phys_pipe].mau[stage].dp.intr_status_mau_gfm_hash);
  // lld_tof2_eos_audit_verify_0(dev_id, addr);

  if (0) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->intr_status_mau_tcam_array_,
    // "intr_status_mau_tcam_array");
    addr = offsetof(
        tof2_reg, pipes[phys_pipe].mau[stage].tcams.intr_status_mau_tcam_array);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->intr_status_mau_ad_,
    // "intr_status_mau_ad");
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].mau[stage].rams.match.adrdist.intr_status_mau_ad);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->intr_status_mau_cfg_,
  // "intr_status_mau_cfg");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].mau[stage].cfg_regs.intr_status_mau_cfg);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  for (int prn = 0; prn < PHYSICAL_UNITRAM_ROWS; prn++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->intr_status_mau_unit_ram_row_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.array.row[prn]
                        .intr_status_mau_unit_ram_row);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->intr_status_mau_adrmux_row_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .adrmux.intr_status_mau_adrmux_row);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->i2portctl_->intr_status_mau_synth2port_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .i2portctl.intr_status_mau_synth2port);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->i2portctl_->mau_synth2port_errlog_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .i2portctl.mau_synth2port_errlog);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    for (int i = 0; i < 6; i++) {
      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->mapram_sbe_errlog_[i],
      // s);
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.map_alu.row[prn]
                          .adrmux.mapram_sbe_errlog[i]);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->mapram_mbe_errlog_[i],
      // s);
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.map_alu.row[prn]
                          .adrmux.mapram_mbe_errlog[i]);
      lld_tof2_eos_audit_verify_0(dev_id, addr);
    }

    for (int pcn = 0; pcn < PHYSICAL_UNITRAMS_PER_ROW; pcn++) {
      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->ram_[pcn]->unit_ram_sbe_errlog_,
      // s);
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.array.row[prn]
                          .ram[pcn]
                          .unit_ram_sbe_errlog);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->ram_[pcn]->unit_ram_sbe_errlog_,
      // s);
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.array.row[prn]
                          .ram[pcn]
                          .unit_ram_mbe_errlog);
      lld_tof2_eos_audit_verify_0(dev_id, addr);
    }
  }

  for (int alu = 0; alu < 4; alu++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->stats_wrap_[alu]->stats_->intr_status_mau_stats_alu_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.stats_wrap[alu]
                        .stats.intr_status_mau_stats_alu);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->meter_group_[alu]->selector_->intr_status_mau_selector_alu_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.meter_group[alu]
                        .selector.intr_status_mau_selector_alu);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->meter_group_[alu]->selector_->mau_selector_alu_errlog_,
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.meter_group[alu]
                        .selector.mau_selector_alu_errlog);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }

  for (int i = 0; i < 4; i++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->tcam_logical_channel_errlog_lo_[i],
    // s);
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].mau[stage].tcams.tcam_logical_channel_errlog_lo[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->tcam_logical_channel_errlog_hi_[i],
    // s);
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].mau[stage].tcams.tcam_logical_channel_errlog_hi[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->deferred_stats_parity_errlog_[i],
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.match.adrdist.deferred_stats_parity_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->def_meter_sbe_errlog_[i],
    // s);
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].mau[stage].rams.match.adrdist.def_meter_sbe_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_creq_errlog_[i],
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_creq_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_cresp_errlog_[i],
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_cresp_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_sreq_errlog_[i],
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_sreq_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }
  for (int i = 0; i < 12; i++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->tcam_sbe_errlog_[i],
    // s);
    addr = offsetof(tof2_reg,
                    pipes[phys_pipe].mau[stage].tcams.tcam_sbe_errlog[i]);
    lld_tof2_eos_audit_verify_0(dev_id, addr);
  }
  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->imem_sbe_errlog_,
  // "imem_sbe_errlog");
  addr = offsetof(tof2_reg, pipes[phys_pipe].mau[stage].dp.imem_sbe_errlog);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->idletime_slip_errlog_,
  // "idletime_slip_errlog");
  addr = offsetof(
      tof2_reg,
      pipes[phys_pipe].mau[stage].rams.match.adrdist.idletime_slip_errlog);
  lld_tof2_eos_audit_verify_0(dev_id, addr);
}

/********************************************************************
 * lld_tof2_eos_mirror_audit
 *
 * Debug cmd handler for mirror end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_mirror_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
  // uint32_t d;

  // cout << "Inside MIRROR AUDIT function" << endl;

  // MIRROR_REG_READ_N_CHK_FOR_ZERO(m_mirror_reg->s2p_regs_->intr_->stat_,
  // "mirr_s2p_intr");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].pardereg.mirreg.mirror.s2p_regs.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // MIRROR_REG_READ_N_CHK_FOR_VALUE(m_mirror_reg->s2p_regs_->s2p_credit_cfg_,
  // "mirr_s2p_credit", 0x30);
  addr =
      offsetof(tof2_reg,
               pipes[phys_pipe].pardereg.mirreg.mirror.s2p_regs.s2p_credit_cfg);
  lld_tof2_eos_audit_verify_exp(dev_id, addr, 0x30);

  for (int i = 0; i < 4; i++) {
    uint32_t got;

    // MIRROR_REG_READ_N_CHK_FOR_TWO_VALUES(m_mirror_reg->slice_regs_[i]->intr_->stat_,
    // "mirr_slice_intr", 0x0, 0x100); //TODO: bit8 could be 1
    // MIRROR_REG_READ_N_CHK_FOR_MULT_VALUES(m_mirror_reg->slice_regs_[i]->intr_->stat_,
    // "mirr_slice_intr", 0x0, 0x4, 0x8, 0xc, 0x100, 0x104, 0x108, 0x10c);
    // //TODO: bit2, bit3, bit8 could be 1
    addr = offsetof(
        tof2_reg,
        pipes[phys_pipe].pardereg.mirreg.mirror.slice_regs[i].intr.stat);
    lld_read_register(dev_id, addr, &got);
    switch (got) {
      case 0:
      case 4:
      case 8:
      case 0xC:
      case 0x100:
      case 0x104:
      case 0x108:
      case 0x10C:
        break;
      default:
        ucli_log("got=%08x : exp=0/4/8/C/100/104/108/10C : %08x : %s\n",
                 got,
                 addr,
                 get_full_reg_path_name(dev_id, addr));
        break;
    }
    // for (int j=0; j<5; j++) {
    //  MIRROR_REG_READ_N_CHK_FOR_VALUE(m_mirror_reg->slice_regs_[i]->dbuff_cnt_[j],
    //  "mirr_slice_dbuff_cnt", 0x4);
    //  MIRROR_REG_READ_N_CHK_FOR_RANGE(m_mirror_reg->slice_regs_[i]->dbuff_wm_[j],
    //  "mirr_slice_dbuff_cnt", 0x4, 0x7f);
    //}
    // for (int q=0; q<54; q++) {
    //  MIRROR_REG_READ_N_CHK_FOR_ZERO(m_mirror_reg->slice_regs_[i]->qdepth_[q],
    //  "mirr_slice_qdepth");
    //}
  }
}

/********************************************************************
 * lld_tof2_eos_prsr_audit
 *
 * Debug cmd handler for prsr end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_prsr_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr, mac, prsr, thrd;
// uint32_t d;
// cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);

//#define NUM_PRSR_TYPES 2
#define NUM_MACS 8
#define NUM_PRSRS 4
#define NUM_THREADS 2

  for (mac = 0; mac < 9; mac++) {
    for (prsr = 0; prsr < 4; prsr++) {
      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->intr_->stat_,
      // "parser_intr");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .intr.stat);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      for (thrd = 0; thrd < NUM_THREADS; thrd++) {
        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->pkt_drop_cnt_[thrd],
        // "pkt_drop_cnt");
        addr = offsetof(tof2_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .pkt_drop_cnt[thrd]);
        lld_tof2_eos_audit_verify_0(dev_id, addr);
      }

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->no_tcam_match_err_cnt_,
      // "no_tcam_match_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .no_tcam_match_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->partial_hdr_err_cnt_,
      // "partial_hdr_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .partial_hdr_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->ctr_range_err_cnt_,
      // "ctr_range_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .ctr_range_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_iter_err_cnt_,
      // "timeout_iter_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .timeout_iter_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_cycle_err_cnt_,
      // "timeout_cycle_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .timeout_cycle_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->src_ext_err_cnt_,
      // "src_ext_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .src_ext_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->phv_owner_err_cnt_,
      // "phv_owner_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .phv_owner_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->multi_wr_err_cnt_,
      // "multi_wr_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .multi_wr_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_sbe_cnt_,
      // "aram_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .aram_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_mbe_cnt_,
      // "aram_mbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .aram_mbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->fcs_err_cnt_,
      // "fcs_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .fcs_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_err_cnt_,
      // "csum_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->tcam_par_err_cnt_,
      // "tcam_par_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .tcam_par_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_sbe_cnt_,
      // "csum_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_mbe_cnt_,
      // "csum_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ipbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);
    }
  }
  for (mac = 0; mac < 9; mac++) {
    for (prsr = 0; prsr < 4; prsr++) {
      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->intr_->stat_,
      // "parser_intr");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .intr.stat);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      for (thrd = 0; thrd < NUM_THREADS; thrd++) {
        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->pkt_drop_cnt_[thrd],
        // "pkt_drop_cnt");
        addr = offsetof(tof2_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.epbprsr4reg[mac]
                            .prsr[prsr]
                            .pkt_drop_cnt[thrd]);
        lld_tof2_eos_audit_verify_0(dev_id, addr);
      }

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->no_tcam_match_err_cnt_,
      // "no_tcam_match_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .no_tcam_match_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->partial_hdr_err_cnt_,
      // "partial_hdr_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .partial_hdr_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->ctr_range_err_cnt_,
      // "ctr_range_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .ctr_range_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_iter_err_cnt_,
      // "timeout_iter_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .timeout_iter_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_cycle_err_cnt_,
      // "timeout_cycle_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .timeout_cycle_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->src_ext_err_cnt_,
      // "src_ext_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .src_ext_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->phv_owner_err_cnt_,
      // "phv_owner_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .phv_owner_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->multi_wr_err_cnt_,
      // "multi_wr_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .multi_wr_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_sbe_cnt_,
      // "aram_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .aram_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_mbe_cnt_,
      // "aram_mbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .aram_mbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->fcs_err_cnt_,
      // "fcs_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .fcs_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_err_cnt_,
      // "csum_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->tcam_par_err_cnt_,
      // "tcam_par_err_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .tcam_par_err_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_sbe_cnt_,
      // "csum_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);

      // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_mbe_cnt_,
      // "csum_sbe_cnt");
      addr = offsetof(tof2_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.epbprsr4reg[mac]
                          .prsr[prsr]
                          .csum_sbe_cnt);
      lld_tof2_eos_audit_verify_0(dev_id, addr);
    }
  }
  // PRSR_REG_READ_N_CHK_FOR_ZERO(PMERGE_LL0_REGS_RSPEC->intr_->stat_,"pmerge_ll_intr");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll0.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // PRSR_REG_READ_N_CHK_FOR_ZERO(PMERGE_LR0_REGS_RSPEC->intr_->stat_,"pmerge_lr_intr");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr0.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);
}

/********************************************************************
 * lld_tof2_eos_s2p_audit
 *
 * Debug cmd handler for s2p end-of-sim checks
 ********************************************************************/
void lld_tof2_eos_s2p_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
  uint32_t addr;
  // uint32_t d;

  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);
  // S2P_REG_READ_N_CHK_FOR_ZERO(m_s2p_reg->intr_->stat_, "s2p_reg_intr");
  addr = offsetof(tof2_reg,
                  pipes[phys_pipe].pardereg.mirreg.mirror.s2p_regs.intr.stat);
  lld_tof2_eos_audit_verify_0(dev_id, addr);

  // m_s2p_reg->ctr_sample_->sample_=0x1;
  // m_s2p_reg->ctr_sample_->Write();

  addr =
      offsetof(tof2_reg, pipes[phys_pipe].pardereg.pgstnreg.s2preg.tm_cred_rd);
  lld_tof2_eos_audit_verify_exp(dev_id, addr, 0x88888888);
}

static void lld_tof2_eos_audit_verify_exp(bf_dev_id_t dev_id,
                                          uint32_t addr,
                                          uint32_t exp) {
  uint32_t got;

  lld_read_register(dev_id, addr, &got);
  if (got != exp) {
    ucli_log("got=%08x : exp=%08x : %08x : %s\n",
             got,
             exp,
             addr,
             get_full_reg_path_name(dev_id, addr));
  } else if (eos_verbose) {
    ucli_log("got=%08x :              : %08x : %s\n",
             got,
             addr,
             get_full_reg_path_name(dev_id, addr));
  }
}

static void lld_tof2_eos_audit_verify_0(bf_dev_id_t dev_id, uint32_t addr) {
  lld_tof2_eos_audit_verify_exp(dev_id, addr, 0);
}
