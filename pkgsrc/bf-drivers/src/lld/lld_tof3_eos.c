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
#include <assert.h>
#include <inttypes.h>
#else
#define assert()
#endif

#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_reg_if.h>
#include "lld.h"
#include "lld_map.h"
#include "lld_log.h"
#include "lld_dev.h"
//#include <lld/lld_sku.h>
#include <tof3_regs/tof3_reg_drv.h>

extern void ucli_log(char *fmt, ...);
extern char *get_full_reg_path_name(bf_dev_id_t dev_id, uint32_t offset);
extern bool eos_verbose;

static void lld_tof3_eos_audit_verify_0(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        uint32_t addr);
static void lld_tof3_eos_audit_verify_64b_0(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id,
                                            uint32_t addr);
static void lld_tof3_eos_audit_verify_exp(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t addr,
                                          uint32_t exp);

/********************************************************************
 * lld_tof3_eos_dprsr_audit
 *
 * Debug cmd handler for dprsr end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_dprsr_audit(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t phys_pipe) {
  uint32_t addr;

  // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inp_->icr_->intr_->stat_,
  // "dprsr_inp_intr");
  addr = offsetof(
      tof3_reg, pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inp_->icr_->intr_b_->stat_,
  // "dprsr_inp_intr");
  addr =
      offsetof(tof3_reg,
               pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.intr_b.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  for (int i = 0; i < 4; i++) {
    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->inpslice_[i]->intr_->stat_,"dprsr_inpslice_intr");
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inpslice[i].intr.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->hir_->h_->stat_,"dprsr_hdr_ingress_intr");
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_i[i].hir.h.intr.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->her_->h_->stat_,"dprsr_hdr_egress_intr");
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_e[i].her.h.intr.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->out_ingr_->intr_0_->stat_,"dprsr_out_ingress_intr0");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_i[i]
                        .out_ingr.intr_0.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_i_[i]->out_ingr_->intr_1_->stat_,"dprsr_out_ingress_intr1");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_i[i]
                        .out_ingr.intr_1.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->out_egr_->intr_0_->stat_,"dprsr_out_egress_intr0");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_e[i]
                        .out_egr.intr_0.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // DPRSR_REG_READ_N_CHK_FOR_ZERO(m_dprsr0_reg->ho_e_[i]->out_egr_->intr_1_->stat_,"dprsr_out_egress_intr1");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_e[i]
                        .out_egr.intr_1.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  }

  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.inp.icr.input_status);
  lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x3f);

  for (int i = 0; i < 4; i++) {
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_i[i].hir.h.hdr_status);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x3ffff);

    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.dprsrreg.dprsrreg.ho_e[i].her.h.hdr_status);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x3ffff);

    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_i[i]
                        .out_ingr.output_status);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x3ffff);

    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.dprsrreg.dprsrreg.ho_e[i]
                        .out_egr.output_status);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x3ffff);
  }
}

/********************************************************************
 * lld_tof3_eos_ebuf_audit
 *
 * Debug cmd handler for ebuf end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_ebuf_audit(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t phys_pipe) {
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
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ebuf900reg[i]
                            .ebuf400reg[j]
                            .chan_group[k]
                            .chnl_fifo_stat);
        lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, expt_val_31_0);

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
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ebuf900reg[i]
                          .ebuf100reg.chan_group[k]
                          .chnl_fifo_stat);
      lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, expt_val_31_0);

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

#if 0
  /* Interrupts not enabled yet */
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 2; j++) {
      // EBUF_REG_READ_N_CHK_FOR_ZERO(m_ebuf_reg[i]->ebuf400reg_[j]->glb_group_->intr_stat_,"ebuf900_ebuf400_intr_stat");
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .pardereg.pgstnreg.ebuf900reg[i]
                          .ebuf400reg[j]
                          .glb_group.intr_stat);
      lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x10080);
    }
  }

  for (int i = 0; i < 4; i++) {
    // EBUF_REG_READ_N_CHK_FOR_ZERO(m_ebuf_reg[i]->ebuf100reg_->glb_group_->intr_stat_,"ebuf900_ebuf100_intr_stat");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ebuf900reg[i]
                        .ebuf100reg.glb_group.intr_stat);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x10080);
  }
#endif
}

/********************************************************************
 * lld_tof3_eos_ibuf_audit
 *
 * Debug cmd handler for ibuf end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_ibuf_audit(bf_dev_id_t dev_id, uint32_t phys_pipe) {
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
 * lld_tof3_eos_epb_audit
 *
 * Debug cmd handler for epb end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_epb_audit(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint32_t phys_pipe) {
  uint32_t addr;
  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);
  // EPB_REG_READ_N_CHK_FOR_ZERO(EPB_REGS_GLB_GROUP->stat_, "epb_intr");
  for (int i = 0; i < 9; i++) {
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.epbprsr4reg[i]
                        .epbreg.glb_group.intr_stat.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    // long reg
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr + 4);
  }
}

/********************************************************************
 * lld_tof3_eos_ipb_audit
 *
 * Debug cmd handler for ipb end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_ipb_audit(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint32_t phys_pipe) {
  uint32_t addr;
#define NUM_IPB 9

  // cfg_manager_db::get_cfg_manager()->set_ref_model_enabled(0);
  for (int i = 0; i < 9; i++) {
    // IPB_REG_READ_N_CHK_FOR_BC(IPB_REGS_GLB_GROUP->glb_status_,
    // "free_list_used_cnt",3705461980);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.glb_group.glb_status);
    lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x04020100);
  }
  for (int i = 0; i < 8; i++) {
    // IPB_REG_READ_N_CHK_FOR_ZERO(IPB_REGS_GLB_GROUP->stat_, "ipb_intr");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.glb_group.intr_stat.stat);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    // wide reg
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr + 4);
  }

  for (int i = 0; i < NUM_IPB; i++) {
    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_deparser_drop_pkt_,"chnl_dprs_drop_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_deparser_drop_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_wsch_trunc_pkt_,"chnl_wsch_trunc_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_wsch_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_wsch_discard_pkt_,"chnl_wsch_discard_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_wsch_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_resubmit_discard_pkt_,"chnl_resubmit_discard_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_resubmit_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_parser_discard_pkt_,"chnl_parser_discard_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_parser_discard_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_drop_trunc_pkt_,"chnl_drop_trunc_pkt");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_drop_trunc_pkt);
    lld_tof3_eos_audit_verify_64b_0(dev_id, subdev_id, addr);

    // IPB_REG_READ_N_CHK_FOR_ZERO(m_chan_group[chan_id]->chnl_metanum_,"chnl_metanum");
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan0_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan1_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan2_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan3_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan4_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan5_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan6_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.ipbprsr4reg[i]
                        .ipbreg.chan7_group.chnl_metanum);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  }
}

/********************************************************************
 * lld_tof3_eos_lfltr_audit
 *
 * Debug cmd handler for lfltr end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_lfltr_audit(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t phys_pipe) {
  uint64_t lq_sop_in, lq_eop_err_drop, lq_in, lq_dropped_state,
      lq_dropped_learned, lq_learned;
  uint32_t d, d_lo, d_hi, addr, addr_lo, addr_hi;
  uint32_t state0, state1, empty0, empty1, full0, full1;

  addr_lo = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_sop_in.lq_sop_in_0_2);
  addr_hi = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_sop_in.lq_sop_in_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_sop_in = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  addr_lo = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_eop_in_err.lq_eop_in_err_0_2);
  addr_hi = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_eop_in_err.lq_eop_in_err_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_eop_err_drop = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  addr_lo = offsetof(tof3_reg,
                     pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_in.lq_in_0_2);
  addr_hi = offsetof(tof3_reg,
                     pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_in.lq_in_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_in = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  addr_lo = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.lfltr_reg.ctrl.lq_dropped_state.lq_dropped_state_0_2);
  addr_hi = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.lfltr_reg.ctrl.lq_dropped_state.lq_dropped_state_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_dropped_state = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  addr_lo = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.lfltr_reg.ctrl.lq_dropped_learned.lq_dropped_learned_0_2);
  addr_hi = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.lfltr_reg.ctrl.lq_dropped_learned.lq_dropped_learned_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_dropped_learned = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  addr_lo = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_learned.lq_learned_0_2);
  addr_hi = offsetof(
      tof3_reg,
      pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lq_learned.lq_learned_1_2);
  lld_subdev_read_register(dev_id, subdev_id, addr_lo, &d_lo);
  lld_subdev_read_register(dev_id, subdev_id, addr_hi, &d_hi);
  lq_learned = (uint64_t)d_hi << 32ul | (uint64_t)d_lo;

  if (lq_sop_in != (lq_eop_err_drop + lq_in)) {
    ucli_log(
        "[ERROR]:: Required LFLTR_INP_BUF counters not accurate : lq_sop_in = "
        "lq_eop_err_drop + lq_in. Read counters:LQ_SOP_IN = %016" PRIx64
        " , LQ_EOP_ERROR_DROPPED = %016" PRIx64 " , LQ_IN = %016" PRIx64 "\n",
        lq_sop_in,
        lq_eop_err_drop,
        lq_in);
  }
  if (lq_in != (lq_dropped_state + lq_dropped_learned + lq_learned)) {
    ucli_log(
        "[ERROR]:: Required counters not accurate : lq_in = lq_dropped_state + "
        "lq_dropped_learned + lq_learned. Read counters:LQ_IN = %016" PRIx64
        " , LQ_DROPPED_LEARNED = %016" PRIx64
        " , LQ_DROPPED_STATE = %016" PRIx64 " , LQ_LEARNED = %016" PRIx64 "\n",
        lq_in,
        lq_dropped_learned,
        lq_dropped_state,
        lq_learned);
  }

  addr = offsetof(tof3_reg, pipes[phys_pipe].pardereg.lfltr_reg.ctrl.lqt_state);
  lld_subdev_read_register(dev_id, subdev_id, addr, &d);

  state0 = getp_tof3_lfltr_lqt_state_state0(&d);
  state1 = getp_tof3_lfltr_lqt_state_state1(&d);
  // if((m_lfltr_reg->ctrl_->lqt_state_->state0_ != 1) ||
  // (m_lfltr_reg->ctrl_->lqt_state_->state1_ != 1))
  if ((state0 != 1) || (state1 != 1)) {
    ucli_log(
        "[ERROR]:: Both LQT_STATEs should be in FILL(3'b001) : State0 = 0x%x "
        ":: State1 = 0x%x \n",
        state0,
        state1);
  }

  empty0 = getp_tof3_lfltr_lqt_state_empty0(&d);
  empty1 = getp_tof3_lfltr_lqt_state_empty1(&d);
  // if((m_lfltr_reg->ctrl_->lqt_state_->empty0_ != 1) ||
  // (m_lfltr_reg->ctrl_->lqt_state_->empty1_ != 1))
  if ((empty0 != 1) || (empty1 != 1)) {
    ucli_log(
        "[ERROR]:: Both LQT_STATEs->empty should be 1 : Empty0 = 0x%x :: "
        "Empty1 = 0x%x \n",
        empty0,
        empty1);
  }

  full0 = getp_tof3_lfltr_lqt_state_full0(&d);
  full1 = getp_tof3_lfltr_lqt_state_full1(&d);
  // if((m_lfltr_reg->ctrl_->lqt_state_->empty0_ != 1) ||
  // (m_lfltr_reg->ctrl_->lqt_state_->empty1_ != 1))
  if ((full0 != 0) || (full1 != 0)) {
    ucli_log(
        "[ERROR]:: Both LQT_STATEs->full should be 0 : Full0 = 0x%x :: Full1 = "
        "0x%x \n",
        full0,
        full1);
  }

#if 0
  /* Need to init lfltr, enable after that */
  addr =
      offsetof(tof3_reg, pipes[phys_pipe].pardereg.lfltr_reg.ctrl.creq_state);
  lld_subdev_read_register(dev_id, subdev_id, addr, &d);
  if (d != 0) {
    ucli_log(
        "[ERROR]:: CREQ_STATE should be IDLE (2'b00) : Detected CREQ_STATE = "
        "0x%x \n",
        d);
  }
#endif

#if 0
  /* Interrupts not enabled yet */
  addr = offsetof(tof3_reg, pipes[phys_pipe].pardereg.lfltr_reg.ctrl.intr_stat);
  lld_subdev_read_register(dev_id, subdev_id, addr, &d);
  if (d != 0) {
    ucli_log("[ERROR]:: LFLTR INTR_STAT not cleared! intr_stat : 0x%x \n", d);
  }
#endif
}

/********************************************************************
 * lld_tof3_eos_mau_audit
 *
 * Debug cmd handler for mau end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_mau_audit(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint32_t phys_pipe,
                            uint32_t stage) {
  uint32_t addr;
  // char s[100];
  // uint32_t d;

#define PHYSICAL_UNITRAM_ROWS 8
#define PHYSICAL_UNITRAMS_PER_ROW 12

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->intr_status_mau_imem_,
  // "intr_status_mau_imem");
  addr =
      offsetof(tof3_reg, pipes[phys_pipe].mau[stage].dp.intr_status_mau_imem);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->intr_status_mau_snapshot_,
  // "intr_status_mau_snapshot");
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].mau[stage].dp.intr_status_mau_snapshot);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // //
  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->xbar_hash_->hash_->intr_status_mau_gfm_hash_,
  // "intr_status_mau_gfm_hash");
  // addr = offsetof(tof3_reg,
  // pipes[phys_pipe].mau[stage].dp.intr_status_mau_gfm_hash);
  // lld_tof3_eos_audit_verify_0(dev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->intr_status_mau_tcam_array_,
  // "intr_status_mau_tcam_array");
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].mau[stage].tcams.intr_status_mau_tcam_array);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->intr_status_mau_ad_,
  // "intr_status_mau_ad");
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe].mau[stage].rams.match.adrdist.intr_status_mau_ad);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->intr_status_mau_cfg_,
  // "intr_status_mau_cfg");
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].mau[stage].cfg_regs.intr_status_mau_cfg);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  for (int prn = 0; prn < PHYSICAL_UNITRAM_ROWS; prn++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->intr_status_mau_unit_ram_row_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.array.row[prn]
                        .intr_status_mau_unit_ram_row);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->intr_status_mau_adrmux_row_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .adrmux.intr_status_mau_adrmux_row);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->i2portctl_->intr_status_mau_synth2port_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .i2portctl.intr_status_mau_synth2port);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->i2portctl_->mau_synth2port_errlog_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.row[prn]
                        .i2portctl.mau_synth2port_errlog);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    for (int i = 0; i < 6; i++) {
      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->mapram_sbe_errlog_[i],
      // s);
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.map_alu.row[prn]
                          .adrmux.mapram_sbe_errlog[i]);
      lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->row_[prn]->adrmux_->mapram_mbe_errlog_[i],
      // s);
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.map_alu.row[prn]
                          .adrmux.mapram_mbe_errlog[i]);
      lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    }

    for (int pcn = 0; pcn < PHYSICAL_UNITRAMS_PER_ROW; pcn++) {
      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->ram_[pcn]->unit_ram_sbe_errlog_,
      // s);
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.array.row[prn]
                          .ram[pcn]
                          .unit_ram_sbe_errlog);
      lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

      // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->array_->row_[prn]->ram_[pcn]->unit_ram_sbe_errlog_,
      // s);
      addr = offsetof(tof3_reg,
                      pipes[phys_pipe]
                          .mau[stage]
                          .rams.array.row[prn]
                          .ram[pcn]
                          .unit_ram_mbe_errlog);
      lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
    }
  }

  for (int alu = 0; alu < 4; alu++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->stats_wrap_[alu]->stats_->intr_status_mau_stats_alu_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.stats_wrap[alu]
                        .stats.intr_status_mau_stats_alu);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->meter_group_[alu]->selector_->intr_status_mau_selector_alu_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.meter_group[alu]
                        .selector.intr_status_mau_selector_alu);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->map_alu_->meter_group_[alu]->selector_->mau_selector_alu_errlog_,
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.map_alu.meter_group[alu]
                        .selector.mau_selector_alu_errlog);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  }

  for (int i = 0; i < 4; i++) {
    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->tcam_logical_channel_errlog_lo_[i],
    // s);
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].mau[stage].tcams.tcam_logical_channel_errlog_lo[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->tcams_->tcam_logical_channel_errlog_hi_[i],
    // s);
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].mau[stage].tcams.tcam_logical_channel_errlog_hi[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    addr = offsetof(tof3_reg,
                    pipes[phys_pipe].mau[stage].tcams.tcam_sbe_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->deferred_stats_parity_errlog_[i],
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .mau[stage]
                        .rams.match.adrdist.deferred_stats_parity_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->def_meter_sbe_errlog_[i],
    // s);
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].mau[stage].rams.match.adrdist.def_meter_sbe_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_creq_errlog_[i],
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_creq_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_cresp_errlog_[i],
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_cresp_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

    // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->cfg_regs_->pbs_sreq_errlog_[i],
    // s);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe].mau[stage].cfg_regs.pbs_sreq_errlog[i]);
    lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  }

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->dp_->imem_sbe_errlog_,
  // "imem_sbe_errlog");
  addr = offsetof(tof3_reg, pipes[phys_pipe].mau[stage].dp.imem_sbe_errlog);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  // MAU_REG_READ_N_CHK_FOR_ZERO(MAU_REGS->pipes_[0]->mau_[m_mau_inst]->rams_->match_->adrdist_->idletime_slip_errlog_,
  // "idletime_slip_errlog");
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe].mau[stage].rams.match.adrdist.idletime_slip_errlog);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
}

/********************************************************************
 * lld_tof3_eos_mirror_audit
 *
 * Debug cmd handler for mirror end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_mirror_audit(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               uint32_t phys_pipe) {
  uint32_t addr;
  // uint32_t d;

  //  MIRROR_REG_READ_N_CHK_FOR_ZERO(m_mirror_reg->s2p_regs_->intr_->stat_,
  //  "mirr_s2p_intr");
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.mirreg.mirror.s2p_regs.intr.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  //  MIRROR_REG_READ_N_CHK_FOR_VALUE(m_mirror_reg->s2p_regs_->s2p_credit_cfg_,
  //  "mirr_s2p_credit", 0x30);
  addr =
      offsetof(tof3_reg,
               pipes[phys_pipe].pardereg.mirreg.mirror.s2p_regs.s2p_credit_cfg);
  lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0x54);

  //  for (int i=0; i<4; i++) {
  //    MIRROR_REG_READ_N_CHK_FOR_MULT_VALUES(m_mirror_reg->slice_regs_[i]->intr_->stat_,
  //    "mirr_slice_intr", 0x0, 0x4, 0x8, 0xc, 0x100, 0x104, 0x108, 0x10c,
  //    0x200, 0x204, 0x208, 0x20c, 0x300, 0x304, 0x308, 0x30c); //TODO: bit2,
  //    bit3, bit8, bit9 could be 1  -- TODO: pass it as an array

  for (int i = 0; i < 4; i++) {
    uint32_t got;

    // MIRROR_REG_READ_N_CHK_FOR_TWO_VALUES(m_mirror_reg->slice_regs_[i]->intr_->stat_,
    // "mirr_slice_intr", 0x0, 0x100); //TODO: bit8 could be 1
    // MIRROR_REG_READ_N_CHK_FOR_MULT_VALUES(m_mirror_reg->slice_regs_[i]->intr_->stat_,
    // "mirr_slice_intr", 0x0, 0x4, 0x8, 0xc, 0x100, 0x104, 0x108, 0x10c);
    // //TODO: bit2, bit3, bit8 could be 1
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.mirreg.mirror.slice_regs[i].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &got);
    switch (got) {
      case 0:
      case 4:
      case 8:
      case 0xC:
      case 0x100:
      case 0x104:
      case 0x108:
      case 0x10C:
      case 0x200:
      case 0x204:
      case 0x208:
      case 0x20C:
      case 0x300:
      case 0x304:
      case 0x308:
      case 0x30C:
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
 * lld_tof3_eos_parrser_audit
 *
 * Debug cmd handler for prsr end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_parser_audit(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               uint32_t phys_pipe) {
  uint32_t addr, mac, prsr, prsr_typ, d;

#define NUM_PRSR_TYPES 2
#define NUM_MACS 8
#define NUM_PRSRS 4
#define NUM_THREADS 2

  for (mac = 0; mac < NUM_MACS; mac++) {
    for (prsr = 0; prsr < NUM_PRSRS; prsr++) {
      for (prsr_typ = 0; prsr_typ < NUM_PRSR_TYPES; prsr_typ++) {
        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->intr_->stat_,
        // "parser_intr");

        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .intr.stat);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        for (uint32_t chan_id = 0; chan_id < NUM_THREADS; chan_id++) {
          // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->pkt_drop_cnt_[chan_id],
          // "pkt_drop_cnt");
          addr = offsetof(tof3_reg,
                          pipes[phys_pipe]
                              .pardereg.pgstnreg.ipbprsr4reg[mac]
                              .prsr[prsr]
                              .pkt_drop_cnt[chan_id]
                              .pkt_drop_cnt_0_2);
          lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
          addr = offsetof(tof3_reg,
                          pipes[phys_pipe]
                              .pardereg.pgstnreg.ipbprsr4reg[mac]
                              .prsr[prsr]
                              .pkt_drop_cnt[chan_id]
                              .pkt_drop_cnt_1_2);
          lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        }

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->no_tcam_match_err_cnt_,
        // "no_tcam_match_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->partial_hdr_err_cnt_,
        // "partial_hdr_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .partial_hdr_err_cnt.partial_hdr_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .partial_hdr_err_cnt.partial_hdr_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->ctr_range_err_cnt_,
        // "ctr_range_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .ctr_range_err_cnt.ctr_range_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .ctr_range_err_cnt.ctr_range_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_iter_err_cnt_,
        // "timeout_iter_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .timeout_iter_err_cnt.timeout_iter_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .timeout_iter_err_cnt.timeout_iter_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->timeout_cycle_err_cnt_,
        // "timeout_cycle_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->src_ext_err_cnt_,
        // "src_ext_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .src_ext_err_cnt.src_ext_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .src_ext_err_cnt.src_ext_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->phv_owner_err_cnt_,
        // "phv_owner_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .phv_owner_err_cnt.phv_owner_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .phv_owner_err_cnt.phv_owner_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->multi_wr_err_cnt_,
        // "multi_wr_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .multi_wr_err_cnt.multi_wr_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .multi_wr_err_cnt.multi_wr_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_sbe_cnt_,
        // "aram_sbe_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .aram_sbe_cnt.aram_sbe_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .aram_sbe_cnt.aram_sbe_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->aram_mbe_cnt_,
        // "aram_mbe_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .aram_mbe_cnt.aram_mbe_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .aram_mbe_cnt.aram_mbe_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->fcs_err_cnt_,
        // "fcs_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .fcs_err_cnt.fcs_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .fcs_err_cnt.fcs_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_err_cnt_,
        // "csum_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_err_cnt.csum_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_err_cnt.csum_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->tcam_par_err_cnt_,
        // "tcam_par_err_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .tcam_par_err_cnt.tcam_par_err_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .tcam_par_err_cnt.tcam_par_err_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_sbe_cnt_,
        // "csum_sbe_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_sbe_cnt.csum_sbe_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_sbe_cnt.csum_sbe_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PRSR_REG_MAIN_RSPEC(prsr_type)->csum_mbe_cnt_,
        // "csum_sbe_cnt");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_mbe_cnt.csum_mbe_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_mbe_cnt.csum_mbe_cnt_1_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PMERGE_LL0_REGS_RSPEC->intr_->stat_,"pmerge_ll_intr");
        addr = offsetof(tof3_reg,
                        pipes[phys_pipe]
                            .pardereg.pgstnreg.ipbprsr4reg[mac]
                            .prsr[prsr]
                            .csum_mbe_cnt.csum_mbe_cnt_0_2);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PMERGE_LR0_REGS_RSPEC->intr_->stat_,"pmerge_lr_intr");
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll0.intr.stat);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PRSR_REG_READ_N_CHK_FOR_ZERO(PMERGE_LR0_REGS_RSPEC->intr_->stat_,"pmerge_lr_intr");
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr0.intr.stat);
        lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->sample_ = 0x1;
        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 1);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->sample_ = 0x1;
        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 1);

        // USLEEP(10); //wait for 10ns
        bf_sys_usleep(10);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_time_->Read();
        // d = PMERGE_LL1_REGS_RSPEC->i_ctr_time_->GetData(0);
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_time);
        lld_subdev_read_register(dev_id, subdev_id, addr, &d);

        // printf("perf_pkt counter value for ingress 1st pulse i_ctr_time_ =
        // %0d \n", d);
        // ucli_log(
        //    "perf_pkt counter value for ingress 1st pulse i_ctr_time_ = %0d
        //    \n",
        //    d);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_time_->Read();
        // d = PMERGE_LR1_REGS_RSPEC->e_ctr_time_->GetData(0);
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_time);
        lld_subdev_read_register(dev_id, subdev_id, addr, &d);

        // printf("perf_pkt counter value for egress 1st pulse e_ctr_time_ = %0d
        // \n", d);
        // ucli_log(
        //    "perf_pkt counter value for egress 1st pulse e_ctr_time_ = %0d
        //    \n",
        //    d);

        // USLEEP(10); //wait for 10ns
        bf_sys_usleep(10);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->sample_ = 0x0;
        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 0);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->sample_ = 0x0;
        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 0);

        // USLEEP(100); //wait for 100ns
        bf_sys_usleep(100);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->sample_ = 0x1;
        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 1);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->sample_ = 0x1;
        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 1);

        // USLEEP(10); //wait for 10ns
        bf_sys_usleep(10);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_time_->Read();
        // d = PMERGE_LL1_REGS_RSPEC->i_ctr_time_->GetData(0);
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_time);
        lld_subdev_read_register(dev_id, subdev_id, addr, &d);

        // printf("perf_pkt counter value for ingress 2nd pulse i_ctr_time_ =
        // %0d \n", d);
        // ucli_log(
        //    "perf_pkt counter value for ingress 2nd pulse i_ctr_time_ = %0d
        //    \n",
        //    d);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_time_->Read();
        // d = PMERGE_LR1_REGS_RSPEC->e_ctr_time_->GetData(0);
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_time);
        lld_subdev_read_register(dev_id, subdev_id, addr, &d);

        // printf("perf_pkt counter value for egress 2nd pulse e_ctr_time_ = %0d
        // \n", d);
        // ucli_log(
        //    "perf_pkt counter value for egress 2nd pulse e_ctr_time_ = %0d
        //    \n",
        //    d);

        // USLEEP(10); //wait for 10ns
        bf_sys_usleep(10);

        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->sample_ = 0x0;
        // PMERGE_LL1_REGS_RSPEC->i_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.ll1.i_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 0);

        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->sample_ = 0x0;
        // PMERGE_LR1_REGS_RSPEC->e_ctr_sample_->Write();
        addr = offsetof(
            tof3_reg,
            pipes[phys_pipe].pardereg.pgstnreg.pmergereg.lr1.e_ctr_sample);
        lld_subdev_write_register(dev_id, subdev_id, addr, 0);

        // USLEEP(100); //wait for 100ns
        bf_sys_usleep(100);
      }
    }
  }
}

/********************************************************************
 * lld_tof3_eos_party_pgr_audit
 *
 * Debug cmd handler for s2p end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_party_pgr_audit(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint32_t phys_pipe) {
  uint32_t addr;

  // intr_stat = PGR_REGS->pgr_common_->intr_stat_->GetData(0);
  // bfdv_debug(BFDV_INFO, "[INFO]::PGR INTR_STAT_0:0x%x \n", intr_stat);
  // intr_stat = PGR_REGS->pgr_common_->intr_stat_->GetData(1);
  // bfdv_debug(BFDV_INFO, "[INFO]::PGR INTR_STAT_1:0x%x \n", intr_stat);
  // intr_stat = PGR_REGS->pgr_common_->intr_stat_->GetData(2);
  // bfdv_debug(BFDV_INFO, "[INFO]::PGR INTR_STAT_2:0x%x \n", intr_stat);
  // intr_stat = PGR_REGS->pgr_common_->intr_stat_->GetData(3);
  // bfdv_debug(BFDV_INFO, "[INFO]::PGR INTR_STAT_3:0x%x \n", intr_stat);
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
  addr = offsetof(
      tof3_reg,
      pipes[phys_pipe]
          .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
}

/********************************************************************
 * lld_tof3_eos_s2p_audit
 *
 * Debug cmd handler for s2p end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_s2p_audit(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint32_t phys_pipe) {
  uint32_t addr;

  // if (!intr_test) {
  //  S2P_REG_READ_N_CHK_FOR_ZERO(m_s2p_reg_0->intr_->stat_, "s2p_reg_0_intr");
  //  S2P_REG_READ_N_CHK_FOR_ZERO(m_s2p_reg_1->intr_2_->stat_,
  //  "s2p_reg_1_intr");

  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.s2preg.reg_0.intr.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.s2preg.reg_1.intr_2.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
}

/* port rate to channel count */
static int port_rate_to_chan_cnt(uint32_t rate) {
  switch (rate) {
    case 7:
      return 8;
    case 6:
      return 4;
    case 5:
      return 2;
    case 4:
    case 3:
    case 2:
    case 1:
    case 0:
    default:
      return 1;
  }
}

/********************************************************************
 * lld_tof3_eos_p2s_audit
 *
 * Debug cmd handler for s2p end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_p2s_audit(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint32_t phys_pipe) {
  uint32_t addr;
  uint32_t d;
  uint8_t cred_amt[4];
  uint32_t default_cred[8];  //, default_cred_amt;

  // default value for 400G
  // default_cred_amt = 0x00010842;

  // write to counter sample before reading the packet and byte counters
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.ctr_sample);
  lld_subdev_write_register(dev_id, subdev_id, addr, 1);

  // credit check
  // for (int i = 0; i < 18; i++) {
  //  m_p2s_reg_0->epb_cred_wr_[i]->Read();
  //  for (int j = 0; j < 4; j++) {
  //    m_sknobs_name = m_name + ".epb_cred_wr[" + std::to_string(i) + "]" +
  //    ".amt[" + std::to_string(j) + "]";
  //    cred_amt[j] = ((default_cred_amt >> (j*5)) & 0x1F);
  //    cred_amt[j] = sknobs_get_dynamic_value((char*)m_sknobs_name.c_str(),
  //    cred_amt[j]);
  //    if(m_p2s_reg_0->epb_cred_wr_[i]->amt_[j] != cred_amt[j]) {
  //      bfdv_error("p2s_audit: epb_cred[%0x].amt[%0x] mismatch  0x%x(exp
  //      0x%x)\n",i, j, m_p2s_reg_0->epb_cred_wr_[i]->amt_[j] , cred_amt[j]);
  //    }
  //  }
  //}
  for (int i = 2; i < 18; i++) {
    // determine port rate (assumes all ports/chnls are the same rate)
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.port_rate_cfg[i / 2]);
    if (((i / 2) % 2) != 0) {
      continue;
    }
    lld_subdev_read_register(dev_id, subdev_id, addr, &d);
    // printf("p2s_audit: port_rate[%0x], val-> 0x%x \n", i, d);
    memset(&default_cred[0], 0, sizeof(default_cred));
    /* Port rate is same for all channels at a speed */
    for (int ch = 0; ch < 8; ch++) {
      uint32_t rate = ((d >> (3 * ch)) & 7);
      uint32_t ch_cnt = port_rate_to_chan_cnt(rate);
      // printf("   p2s_audit: rate %d, ch cnt %d\n", rate, ch_cnt);
      default_cred[ch] = ch_cnt * 3;
      ch += ch_cnt;
    }
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.epb_cred_wr[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &d);
    // printf("   p2s_audit: epb cr wr 0x%x\n", d);
    for (int j = 0; j < 4; j++) {
      if ((j % 2) != 0) {
        continue;
      }
      // cred_amt[j] = ((default_cred_amt >> (j * 5)) & 0x1F);
      cred_amt[j] = ((i & 1) == 0) ? default_cred[j] : default_cred[j + 4];
      if (((d >> (j * 5)) & 0x1f) != cred_amt[j]) {
        ucli_log("p2s_audit: epb_cred[%0x].amt[%0x] mismatch  0x%x(exp 0x%x)\n",
                 i,
                 j,
                 ((d >> (j * 5)) & 0x1f),
                 cred_amt[j]);
      }
    }
  }
  // default_tm_cred  = 0x30;

  for (int i = 0; i < 72; i++) {
    uint32_t pl, ph, bl, bh;
    // Packets
    // m_p2s_reg_0->pkt_ctr_[i]->Read();
    // d = m_p2s_reg_0->pkt_ctr_[i]->GetData(0);
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.pkt_ctr[i].pkt_ctr_0_2);
    lld_subdev_read_register(dev_id, subdev_id, addr, &pl);
    addr = offsetof(
        tof3_reg,
        pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.pkt_ctr[i].pkt_ctr_1_2);
    lld_subdev_read_register(dev_id, subdev_id, addr, &ph);

    // pkt_ctr[i] = ((static_cast<uint64_t>(d) & 0xffffffff) | (pkt_ctr[i] &
    // 0xf00000000));
    // d = m_p2s_reg_0->pkt_ctr_[i]->GetData(1);
    // pkt_ctr[i] = (((static_cast<uint64_t>(d) << 32) & 0xf00000000) |
    // (pkt_ctr[i] & 0xffffffff));

    // Byte
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.p2sreg.reg_0.byte_ctr[i]
                        .byte_ctr_0_2);
    lld_subdev_read_register(dev_id, subdev_id, addr, &bl);
    addr = offsetof(tof3_reg,
                    pipes[phys_pipe]
                        .pardereg.pgstnreg.p2sreg.reg_0.byte_ctr[i]
                        .byte_ctr_1_2);
    lld_subdev_read_register(dev_id, subdev_id, addr, &bh);
    // m_p2s_reg_0->byte_ctr_[i]->Read();
    // d = m_p2s_reg_0->byte_ctr_[i]->GetData(0);
    // byte_ctr[i] = ((static_cast<uint64_t>(d) & 0xffffffff) | (byte_ctr[i] &
    // 0xffff00000000));
    // d = m_p2s_reg_0->byte_ctr_[i]->GetData(1);
    // byte_ctr[i] = (((static_cast<uint64_t>(d) << 32) & 0xffff00000000) |
    // (byte_ctr[i] & 0xffffffff));

    if (eos_verbose) {
      ucli_log(
          "pipes[%d].pardereg.pgstnreg.p2sreg.reg_0.pkt_ctr: %u_%u : byte_ctr: "
          "%u_%u\n",
          phys_pipe,
          ph,
          pl,
          bh,
          bl);
    }
  }

  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_0.intr.stat);
  lld_subdev_read_register(dev_id, subdev_id, addr, &d);
  if ((d & 0xffff) != 0) {
    ucli_log("p2s_audit: non-zero p2s (reg_0) interrupt 0x%x\n", (d & 0xffff));
  }
  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.p2sreg.reg_1.intr.stat);
  lld_subdev_read_register(dev_id, subdev_id, addr, &d);
  if ((d & 0xffff) != 0) {
    ucli_log("p2s_audit: non-zero p2s (reg_1) interrupt 0x%x\n", (d & 0xffff));
  }
}

/********************************************************************
 * lld_tof3_eos_parb_audit
 *
 * Debug cmd handler for s2p end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_parb_audit(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t phys_pipe) {
  uint32_t addr;

  // pipes_[m_pipe_inst]->pardereg_->pgstnreg
  // PARB_REG_READ_N_CHK_FOR_ZERO(m_parb_regs->left_->intr_->stat_,"parb_left_intr");
  // PARB_REG_READ_N_CHK_FOR_ZERO(m_parb_regs->right_->intr_->stat_,"parb_right_intr");

  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.parbreg.left.intr.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);

  addr = offsetof(tof3_reg,
                  pipes[phys_pipe].pardereg.pgstnreg.parbreg.right.intr.stat);
  lld_tof3_eos_audit_verify_0(dev_id, subdev_id, addr);
}

static void lld_tof3_eos_audit_verify_exp(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t addr,
                                          uint32_t exp) {
  uint32_t got;

  lld_subdev_read_register(dev_id, subdev_id, addr, &got);
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

static void lld_tof3_eos_audit_verify_0(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        uint32_t addr) {
  lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0);
}

static void lld_tof3_eos_audit_verify_64b_0(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id,
                                            uint32_t addr) {
  lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr, 0);
  lld_tof3_eos_audit_verify_exp(dev_id, subdev_id, addr + 4, 0);
}

void lld_tof3_non_tm_eos_audit(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               uint32_t phys_pipe) {
  lld_tof3_eos_s2p_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_party_pgr_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_parser_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_parb_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_p2s_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_mirror_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_lfltr_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_ipb_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_epb_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_ebuf_audit(dev_id, subdev_id, phys_pipe);
  lld_tof3_eos_dprsr_audit(dev_id, subdev_id, phys_pipe);
}
