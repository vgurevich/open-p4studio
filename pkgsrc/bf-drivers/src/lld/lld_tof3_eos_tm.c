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
 * @file lld_ind_reg_if_tof3.c
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

// sknobs settings
#define m_sknobs_value_tm_prc_pipe_eos_check 1
extern uint32_t m_pipe_mask;
#define NumPipe 4
uint32_t tof3_psc_blk_dis[3] = {0};
uint32_t tof3_caa_blk_vld[0xC0] = {0};

extern void ucli_log(char *fmt, ...);
extern char *get_full_reg_path_name(bf_dev_id_t dev_id, uint32_t offset);

static void tm_ig_int_chk(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint32_t addr, data;
  // uint8_t clear = 0;
  uint32_t intr_stat;
  uint32_t intr_mask;
  // uint32_t write_mask;

  for (int pipe_num = 0; pipe_num < NumPipe; pipe_num++) {
    if (!((m_pipe_mask >> pipe_num) & 0x1)) {
      continue;
    }
    // WAC
    intr_mask = 0;

    // TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->stat_->GetData(0);
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_wac_top.wac_pipe[pipe_num].wac_reg.intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::WAC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::WAC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
    // CLC
    intr_mask = 0;
    // TM_REGS->get_clc_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_clc_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_clc_top.clc[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::CLC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::CLC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
    // PEX
    intr_mask = 0;
    // TM_REGS->get_pex_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_pex_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_pex_top.pex[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::CLC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::CLC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
  }
  // CAA
  intr_mask = 0;

  // TM_REGS->get_tm()->tm_caa_top_->intr_->stat_->Read();
  // intr_stat = TM_REGS->get_tm()->tm_caa_top_->intr_->stat_->GetData(0);

  addr = offsetof(tof3_reg, device_select.tm_top.tm_caa_top.intr.stat);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  intr_stat = data;

  if ((intr_stat & ~intr_mask) != 0) {
    ucli_log("[ERROR]::CAA Interrupt Status FAILED:0x%x \n", intr_stat);
  } else {
    ucli_log("[INFO]::CAA INT PASS:0x%x \n", intr_stat);
  }
}

static void tm_eg_int_chk(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          uint8_t clear) {
  uint32_t addr, data;
  uint32_t intr_stat;
  uint32_t intr_stat_mod;
  // uint32_t intr_inj;
  uint32_t intr_mask;
  // uint32_t write_mask;
  // uint32_t err_log;

  (void)clear;

  for (int pipe_num = 0; pipe_num < NumPipe; pipe_num++) {
    if (!((m_pipe_mask >> pipe_num) & 0x1)) {
      continue;
    }
    // QLC
    intr_mask = 0;
    // TM_REGS->get_qlc_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_qlc_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qlc_top.qlc[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::QLC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::QLC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
    // PRE

    // TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_pre_top.pre[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;
#if 0
    intr_mask = 0;
    intr_mask |=
        0x1 | 0xC | 0x200;  // fifo_ready/table0/1_ph_count/rdm_addr_hit

    if(ignore_pre_overflow) { intr_mask = intr_mask | 0x100; }
    if((ignore_pre_l1_max_exceed >> pipe_num) & 0x1) { intr_mask = intr_mask | 0x10; }
    if((ignore_pre_l2_max_exceed >> pipe_num) & 0x1) { intr_mask = intr_mask | 0x20; }
    if(((ignore_pre_illegal_l1 >> pipe_num) & 0x1) || ignore_mbe || ignore_mbe_pre) { intr_mask = intr_mask | 0x40; }
    if(((ignore_pre_illegal_l2 >> pipe_num) & 0x1) || ignore_mbe || ignore_mbe_pre) { intr_mask = intr_mask | 0x80; }

    if(clear && ~intr_test_en) {
      if (ignore_sbe | ignore_sbe_pre | ignore_all_be) {
        ucli_log( "[INFO]::PRE P%0d SBE Mask Generated!\n", pipe_num);
        // Backdoor read inj register
        TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->inj_->Read();
        intr_inj = TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->inj_->GetData(0);

        intr_mask |= ~(intr_inj & 0x40000000) | ignore_all_be; //fifo_mem_bankid
        intr_mask |= ~(intr_inj & 0x20000000) | ignore_all_be; //rdm_sbe
        intr_mask |= ~(intr_inj & 0x10000000) | ignore_all_be; //pmt1_sbe
        intr_mask |= ~(intr_inj & 0x08000000) | ignore_all_be; //pmt0_sbe
        intr_mask |= ~(intr_inj & 0x04000000) | ignore_all_be; //lit1_np_sbe
        intr_mask |= ~(intr_inj & 0x02000000) | ignore_all_be; //lit0_np_sbe
        intr_mask |= ~(intr_inj & 0x01000000) | ignore_all_be; //lit1_bm_sbe
        intr_mask |= ~(intr_inj & 0x00800000) | ignore_all_be; //lit0_bm_sbe
        intr_mask |= ~(intr_inj & 0x00400000) | ignore_all_be; //mit_sbe
        intr_mask |= ~(intr_inj & 0x00200000) | ignore_all_be; //fifo_sbe
      }
      if (ignore_mbe | ignore_mbe_pre | ignore_all_be) {
        ucli_log( "[INFO]::PRE P%0d MBE Mask Generated!\n", pipe_num);
        // Backdoor read inj register
        TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->inj_->Read();
        intr_inj = TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->inj_->GetData(0);

        intr_mask |= ~(intr_inj & 0x100000) | ignore_all_be; //fifo_mem_bankid
        intr_mask |= ~(intr_inj & 0x080000) | ignore_all_be; //rdm_mbe
        intr_mask |= ~(intr_inj & 0x040000) | ignore_all_be; //pmt1_mbe
        intr_mask |= ~(intr_inj & 0x020000) | ignore_all_be; //pmt0_mbe
        intr_mask |= ~(intr_inj & 0x010000) | ignore_all_be; //lit1_np_mbe
        intr_mask |= ~(intr_inj & 0x008000) | ignore_all_be; //lit0_np_mbe
        intr_mask |= ~(intr_inj & 0x004000) | ignore_all_be; //lit1_bm_mbe
        intr_mask |= ~(intr_inj & 0x002000) | ignore_all_be; //lit0_bm_mbe
        intr_mask |= ~(intr_inj & 0x001000) | ignore_all_be; //mit_mbe
        intr_mask |= ~(intr_inj & 0x000800) | ignore_all_be; //fifo_sbe
      }
      if ((ignore_pre_rdm_change >> pipe_num) & 0x1) {
        intr_mask |= 0x2; //rdm_change_done
      }
    }
    // Apply mask - XOR if both rd_data and mask set => clear to 0
    // rd_data_mask are bits that could be set so mask out
    if (ignore_all_be) {
      intr_stat_mod = intr_stat & ~intr_mask;
    } else {
      intr_stat_mod = (intr_stat ^ intr_mask) & ~intr_mask;
    }
#endif  // 0

    intr_stat_mod = intr_stat;
    // We can ignore 1101 bits in debug register for EOS. They are not errors
    if (intr_stat_mod != 0 && intr_stat_mod != 0xd) {
      ucli_log("[ERROR]::PRE P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat_mod);
    } else {
      ucli_log("[INFO]::PRE P%0d INT PASS:0x%x \n", pipe_num, intr_stat_mod);
    }

#if 0
    write_mask = TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->GetWriteMask(0);
    TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->SetData(0,write_mask);
    TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->Write();

    TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->Read();
    intr_stat = TM_REGS->get_pre_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);
    intr_stat_mod = intr_stat & ~intr_mask;

    if(clear) {
      if(intr_stat_mod != 0) {
        ucli_log( "[ERROR]::PRE P%0d Interrupt Status Clear FAILED:0x%x \n", pipe_num, intr_stat_mod);
      } else {
        ucli_log( "[INFO]::PRE P%0d INT Clear PASS:0x%x \n", pipe_num, intr_stat_mod);
      }
    }
#endif  // 0

    // PRC
    intr_mask = 0;

    // TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_prc_top.prc[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    // read twice? intentional?
    // TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::PRC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::PRC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }

    // QAC
    intr_mask = 0;

    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->GetData(0);

    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_qac_top.qac_pipe[pipe_num].qac_reg.intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

#if 0
    if (((ignore_qac_nodst >> pipe_num) & 0x1) || ignore_mbe_pre || ignore_mbe_sch || ignore_mbe_prc == 1 || ignore_mbe_pex) {
      intr_mask |= 0x400;  //debug_sts
    }
    if (ignore_sbe) {
      ucli_log( "[INFO]::QAC P%0d SBE Mask Generated!\n", pipe_num);
      intr_mask |= 0x155;  //sbe:prc2psc/qac2prc/qid_mapping/port_drop/queue_drop
    }
    if(ignore_mbe) {
      ucli_log( "[INFO]::QAC P%0d MBE Mask Generated!\n", pipe_num);
      intr_mask |= 0x6AA;  //mbe:debug_sts/prc2psc/qac2prc/qid_mapping/port_drop/queue_drop
    }

    TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->Read();
    intr_stat = TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->GetData(0);
#endif

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::QAC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::QAC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
#if 0
    if(clear) {
      if(intr_stat != 0) {
        // Read SBE Err Log Register
        if(intr_stat & 0x1) { //queue_drop sbe
          if (!intr_test_en) {
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_sbe_err_log_->Read();
            err_log = TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_sbe_err_log_->GetData(0);

            // Clear Specific Entry
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_queue_->entry_[err_log]->count_=0;
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_queue_->entry_[err_log]->Write();
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_sbe_err_log_->addr_=0;
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_sbe_err_log_->Write();
          }
        }
        if((intr_stat >> 1) & 0x1) { //queue_drop mbe
          if (!intr_test_en) {
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_mbe_err_log_->Read();
            err_log = TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_mbe_err_log_->GetData(0);

            // Clear Specific Entry
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_queue_->entry_[err_log]->count_=0;
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_queue_->entry_[err_log]->Write();
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_mbe_err_log_->addr_=0;
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_table_mbe_err_log_->Write();
          }
        }
        if((intr_stat >> 2) & 0x1) { //port_drop sbe
          if (!intr_test_en) {
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_sbe_err_log_->Read();
            err_log = TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_sbe_err_log_->GetData(0);

            // Clear Specific Entry
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_port_->entry_[err_log]->count_=0;
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_port_->entry_[err_log]->Write();
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_sbe_err_log_->addr_=0;
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_sbe_err_log_->Write();
          }
        }
        if((intr_stat >> 3) & 0x1) { //port_drop mbe
          if (!intr_test_en) {
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_mbe_err_log_->Read();
            err_log = TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_mbe_err_log_->GetData(0);

            // Clear Specific Entry
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_port_->entry_[err_log]->count_=0;
            TM_REGS->get_qac_pipe_mem_rspec(pipe_num)->csr_memory_qac_drop_count_port_->entry_[err_log]->Write();
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_mbe_err_log_->addr_=0;
            TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_cnt_table_mbe_err_log_->Write();
          }
        }
      }

      write_mask = TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->GetWriteMask(0);
      TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->SetData(0,write_mask);
      TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->stat_->Write();

      TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->Read();
      intr_stat = TM_REGS->get_prc_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);
      if((intr_stat & ~intr_mask) != 0) {
        ucli_log( "[ERROR]::QAC P%0d Interrupt Status Clear FAILED:0x%x \n", pipe_num, intr_stat);
      } else {
        ucli_log( "[INFO]::QAC P%0d INT Clear PASS:0x%x \n", pipe_num, intr_stat);
      }
    }
#endif  // 0

    // SCH
    intr_mask = 0;

#if 0
    if(ignore_sbe || ignore_sbe_sch || ignore_all_be) {
      ucli_log( "[INFO]::SCH P%0d SBE Mask Generated!\n", pipe_num);
      intr_mask |= 0x555555;  //upd_edprsr,upd_pex1/0,p_maxrate,l1_maxrate,l1_excrate,l1_minrate,q_maxrate_sbe,q_excrate_sbe,q_minrate,upd_wac,tdm_table
    }
    if(ignore_mbe || ignore_mbe_sch || ignore_all_be) {
      ucli_log( "[INFO]::SCH P%0d MBE Mask Generated!\n", pipe_num);
      intr_mask |= 0xAAAAAA;  //upd_edprsr,upd_pex1/0,p_maxrate,l1_maxrate,l1_excrate,l1_minrate,q_maxrate_sbe,q_excrate_sbe,q_minrate,upd_wac,tdm_table
    }
    if (ignore_sch_q_flush) {
      ucli_log( "[INFO]::SCH P%0d Q-Flush Mask Generated!\n", pipe_num);
      intr_mask |= 0x80000000;
    }
#endif

    // TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_sch_top.sch[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::SCH P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::SCH P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
#if 0
    if(clear) {
      write_mask = TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->GetWriteMask(0);
      TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->SetData(0,write_mask);
      TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->Write();

      TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->Read();
      intr_stat = TM_REGS->get_sch_pipe_rspec(pipe_num)->intr_->stat_->GetData(0);
      /* Comment the clear check since if any SBE/MBE error will be continuously scanned.
       * No way to stop scan - need to write to location w/ good ECC
      if(intr_stat != 0) {
        ucli_log( "[ERROR]::SCH P%0d Interrupt Status Clear FAILED:0x%x \n", pipe_num, intr_stat);
      } else {
        ucli_log( "[INFO]::SCH P%0d INT Clear PASS:0x%x \n", pipe_num, intr_stat);
      }
      */
    }
#endif

    // PSC
    intr_mask = 0;

    // TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->Read();
    // intr_stat =
    // TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->GetData(0);

    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_psc_top.psc[pipe_num].intr.stat);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    intr_stat = data;

#if 0
    if(ignore_sbe) {
      ucli_log( "[INFO]::PSC P%0d SBE Mask Generated!\n", pipe_num);
      intr_mask |= 0x1;  //psm_sbe
    }
    if(ignore_mbe || ignore_mbe_sch || ignore_all_be) {
      ucli_log( "[INFO]::PSC P%0d MBE Mask Generated!\n", pipe_num);
      intr_mask |= 0x2;  //psc_mbe
    }
#endif

    if ((intr_stat & ~intr_mask) != 0) {
      ucli_log("[ERROR]::PSC P%0d Interrupt Status FAILED:0x%x \n",
               pipe_num,
               intr_stat);
    } else {
      ucli_log("[INFO]::PSC P%0d INT PASS:0x%x \n", pipe_num, intr_stat);
    }
#if 0
    if(clear) {
      write_mask = TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->GetWriteMask(0);
      TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->SetData(0,write_mask);
      TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->Write();

      TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->Read();
      TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->intr_->stat_->GetData(0);

      if(intr_stat != 0) {
        ucli_log( "[ERROR]::SCH P%0d Interrupt Status Clear FAILED:0x%x \n", pipe_num, intr_stat);
      } else {
        ucli_log( "[INFO]::SCH P%0d INT Clear PASS:0x%x \n", pipe_num, intr_stat);
      }
    }
#endif
  }
  // PSC
  intr_mask = 0;
  // TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->Read();
  // intr_stat =
  // TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->GetData(0);

  addr =
      offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.intr.stat);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  intr_stat = data;

  intr_mask |= 0x1 | 0x8 | 0x10;  // pktdrop,overflow,underflow

#if 0
  if(ignore_sbe) {
    ucli_log( "[INFO]::PSC COMMON SBE Mask Generated!\n");
    intr_mask |= 0x2;  //linkmem
  }
  if(ignore_mbe) {
    ucli_log( "[INFO]::PSC COMMON MBE Mask Generated!\n");
    intr_mask |= 0x4;  //linkmem
  }
#endif

  if ((intr_stat & ~intr_mask) != 0) {
    ucli_log("[ERROR]::PSC COMMON Interrupt Status FAILED:0x%x \n", intr_stat);
  } else {
    ucli_log("[INFO]::PSC COMMON INT PASS:0x%x \n", intr_stat);
  }
#if 0
  if(clear) {
    write_mask = TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->GetWriteMask(0);
    TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->SetData(0,write_mask);
    TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->Write();

    TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->Read();
    TM_REGS->get_tm()->tm_psc_top_->psc_common_->intr_->stat_->GetData(0);

    if((intr_stat & ~intr_mask) != 0) {
      ucli_log( "[ERROR]::PSC Interrupt Status Clear FAILED:0x%x \n", intr_stat);
    } else {
      ucli_log( "[INFO]::PSC INT Clear PASS:0x%x \n", intr_stat);
    }
  }
#endif
}

static void tm_wac_common_eos_check(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  uint8_t ignore_ap_drop_st;
  uint8_t wm_cnt_buf = 64;

  cnt_size =
      tof3_reg_device_select_tm_top_tm_wac_top_wac_common_wac_common_wac_ap_cnt_cell_array_count;  // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_ap_cnt_cell_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_ap_cnt_cell_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_common.wac_common
                        .wac_ap_cnt_cell[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_wac_ap_cnt_cell_cnt(&data);
    // if
    // (TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_ap_cnt_cell_[i]->cnt_
    // != 0x0) {
    if (fld != 0x0) {
      ucli_log("[ERROR]::wac_global_ap_cnt non-zero:%0d for idx:%0d\n", fld, i);
    }
  }

  // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_hdr_cnt_cell_->Read();
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_hdr_cnt_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_wac_hdr_cnt_cell_cnt(&data);
  // if
  // (TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_hdr_cnt_cell_->cnt_
  // != 0x0) {
  if (fld != 0x0) {
    ucli_log("[ERROR]::wac_global_hdr_cnt non-zero:%0d\n", fld);
  }

  // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_dod_cnt_cell_->Read();
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_dod_cnt_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_wac_dod_cnt_cell_cnt(&data);
  // if
  // (TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_dod_cnt_cell_->cnt_
  // != 0x0) {
  if (fld != 0x0) {
    ucli_log("[ERROR]::wac_global_dod_cnt non-zero:%0d\n", fld);
  }

  // m_sknobs_name = "tb_tm.ignore_wac_ap_drop_st";
  ignore_ap_drop_st =
      false;  // sknobs_get_value((char*)m_sknobs_name.c_str(), 0);

  if (!ignore_ap_drop_st) {
    uint32_t drop_state0, drop_state1, drop_state2, drop_state3;
    // set according to test sknobs (?)
    uint32_t wac_glb_ap_grn_lmt[4] = {0, 0, 0, 0};
    uint32_t wac_glb_ap_yel_lmt[4] = {0, 0, 0, 0};
    uint32_t wac_glb_ap_red_lmt[4] = {0, 0, 0, 0};
    uint32_t wac_glb_hdr_lmt = 0;

    // grn
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_common
                        .wac_common_block_drop_st.drop_state_cell[0]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    drop_state0 = getp_tof3_wac_ap_drop_state_cell_drop_state(&data);
    for (int ap = 0; ap < 4; ap++) {
      if (wac_glb_ap_grn_lmt[ap] == 0) {
        if (((drop_state0 >> ap) & 1) != 1) {
          ucli_log(
              "wac_common_block grn_lmt=0. drop_state non-one:%0d for ap:%0d\n",
              drop_state0,
              ap);
        }
      } else {
        if (((drop_state0 >> ap) & 1) != 0) {
          ucli_log(
              "wac_common_block grn_lmt=0. drop_state non-zero:%0d for "
              "ap:%0d\n",
              drop_state0,
              ap);
        }
      }
    }
    // yel
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_common
                        .wac_common_block_drop_st.drop_state_cell[1]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    drop_state1 = getp_tof3_wac_ap_drop_state_cell_drop_state(&data);
    for (int ap = 0; ap < 4; ap++) {
      if (((drop_state0 >> ap) & 1) == 1) {
        if (((drop_state1 >> ap) & 1) != 1) {
          ucli_log(
              "wac_common_block grn drop. drop_state non-one:%0d for ap:%0d\n",
              drop_state1,
              ap);
        }
      } else {
        if (wac_glb_ap_yel_lmt[ap] == 0) {
          if (((drop_state1 >> ap) & 1) != 1) {
            ucli_log(
                "wac_common_block yel_lmt=0. drop_state non-one:%0d for "
                "ap:%0d\n",
                drop_state1,
                ap);
          }
        } else {
          if (((drop_state1 >> ap) & 1) != 0) {
            ucli_log(
                "wac_common_block yel_lmt!=0. drop_state non-zero:%0d for "
                "ap:%0d\n",
                drop_state1,
                ap);
          }
        }
      }
    }
    // red
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_common
                        .wac_common_block_drop_st.drop_state_cell[2]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    drop_state2 = getp_tof3_wac_ap_drop_state_cell_drop_state(&data);
    for (int ap = 0; ap < 4; ap++) {
      if ((((drop_state0 >> ap) & 1) == 1) ||
          (((drop_state1 >> ap) & 1) == 1)) {
        if (((drop_state2 >> ap) & 1) != 1) {
          ucli_log(
              "wac_common_block grn or yel drop. drop_state non-one:%0d for "
              "ap:%0d\n",
              drop_state2,
              ap);
        }
      } else {
        if (wac_glb_ap_red_lmt[ap] == 0) {
          if (((drop_state2 >> ap) & 1) != 1) {
            ucli_log(
                "wac_common_block red_lmt=0. drop_state non-one:%0d for "
                "ap:%0d\n",
                drop_state2,
                ap);
          }
        } else {
          if (((drop_state2 >> ap) & 1) != 0) {
            ucli_log(
                "wac_common_block red_lmt!=0. drop_state non-zero:%0d for "
                "ap:%0d\n",
                drop_state2,
                ap);
          }
        }
      }
    }
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_common
                        .wac_common_block_drop_st.drop_state_cell[3]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    drop_state3 = getp_tof3_wac_ap_drop_state_cell_drop_state(&data);
    if (wac_glb_hdr_lmt == 0) {
      if ((drop_state3 & 1) != 1) {
        ucli_log("wac_common_block glb_hdr_lmt=0. drop_state non-one:%0d\n",
                 drop_state3);
      }
    } else {
      if ((drop_state3 & 1) != 0) {
        ucli_log("wac_common_block glb_hdr_lmt!=0. drop_state non-zero:%0d\n",
                 drop_state3);
      }
    }
  }

  // Read Global Hdr WM
  // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_wm_hdr_cnt_cell_->Read();
  addr = offsetof(tof3_reg,
                  device_select.tm_top.tm_wac_top.wac_common.wac_common
                      .wac_wm_hdr_cnt_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  uint32_t hdr_cnt_cell = getp_tof3_wac_wm_hdr_cnt_cell_cnt(&data);

  // Read Global Hdr Limit
  // TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_hdr_limit_cell_->Read();
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_wac_top.wac_common.wac_common.wac_hdr_limit_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  uint32_t hdr_limit_cell = getp_tof3_wac_hdr_limit_cell_limit(&data);

  // Compare if (Limit + slop (64) < WM) => ERROR
  // if
  // (((TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_hdr_limit_cell_->limit_)
  // + wm_cnt_buf) <
  //    (TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_wm_hdr_cnt_cell_->cnt_))
  //    {
  //  ucli_log( "[ERROR]::wac_common_block WAC Global Hdr WM > than Hdr Limit!
  //  WM:%0d Limit:%0d\n",
  //    TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_wm_hdr_cnt_cell_->cnt_,
  //    TM_REGS->get_tm()->tm_wac_top_->wac_common_->wac_common_->wac_hdr_limit_cell_->limit_);
  //}
  if (((hdr_limit_cell) + wm_cnt_buf) < (hdr_cnt_cell)) {
    ucli_log(
        "[ERROR]::wac_common_block WAC Global Hdr WM > than Hdr Limit! WM:%0d "
        "Limit:%0d\n",
        hdr_cnt_cell,
        hdr_limit_cell);
  }

  ucli_log("[TM_EOS]:: WAC Common Check Done!\n");
}

#if 0
void tm_wac_drop_cnt_eos_check() {
uint32_t cnt_size;
uint64_t ppg_drop_total[4];
uint8_t drop_cnt_buf;

bool wac_clr_drop_cnt_chk;
bool wac_ppg_drop_cnt_chk;
bool wac_port_drop_cnt_chk;
bool wac_glb_hdr_drop_cnt_chk;

  m_sknobs_name = "tof3.tm.wac_clr_drop_cnt_chk";
  wac_clr_drop_cnt_chk = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
  m_sknobs_name = "tof3.tm.wac_ppg_drop_cnt_chk";
  wac_ppg_drop_cnt_chk = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
  m_sknobs_name = "tof3.tm.wac_port_drop_cnt_chk";
  wac_port_drop_cnt_chk = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
  m_sknobs_name = "tof3.tm.wac_glb_hdr_drop_cnt_chk";
  wac_glb_hdr_drop_cnt_chk = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
  m_sknobs_name = "tof3.tm.wac_drop_cnt_buf";
  drop_cnt_buf = sknobs_get_value((char*)m_sknobs_name.c_str(), 20);

  // PPG
  for (int pipe=0; pipe < NumPipe; pipe++) {
    if(!((m_pipe_mask >> pipe) & 0x1)) { continue; }
    ppg_drop_total[pipe] = 0;

    cnt_size = TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_.size();
    for(int i=0; i<cnt_size; i++) {
      m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_ppg"+std::to_string(i)+"_drop_cnt";
      m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0xABCD);

      TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_[i]->Read();
      ppg_drop_total[pipe] += TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_[i]->cnt_;

      if(wac_ppg_drop_cnt_chk == 1) {
        if((TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_[i]->cnt_ > (m_sknobs_value+drop_cnt_buf)) ||
           ((TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_[i]->cnt_+drop_cnt_buf) < m_sknobs_value) ) {
          ucli_log( "[ERROR]:: WAC_DROP_COUNT_PPG[%0d] CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", i, m_sknobs_value,
              TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_ppg_->drop_cnt_[i]->cnt_);
        }
      }
    }
  }
  // PORT
  if(wac_port_drop_cnt_chk) {
    for (int pipe=0; pipe < NumPipe; pipe++) {
      if(!((m_pipe_mask >> pipe) & 0x1)) { continue; }
      cnt_size = TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_port_->drop_cnt_.size();
      for(int i=0; i<cnt_size; i++) {
        m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_port"+std::to_string(i)+"_drop_cnt";
        m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);

        TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_port_->drop_cnt_[i]->Read();
        if((TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_port_->drop_cnt_[i]->cnt_ > (m_sknobs_value+drop_cnt_buf)) ||
           ((TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_port_->drop_cnt_[i]->cnt_+drop_cnt_buf) < m_sknobs_value) ) {
          ucli_log( "[ERROR]:: WAC_DROP_COUNT_PORT[%0d] CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", i, m_sknobs_value,
              TM_REGS->get_wac_pipe_mem_rspec(pipe)->csr_memory_wac_drop_count_port_->drop_cnt_[i]->cnt_);
        }
      }
    }
  }
  // AP
  for (int pipe=0; pipe < NumPipe; pipe++) {
    if(!((m_pipe_mask >> pipe) & 0x1)) { continue; }
    if(wac_clr_drop_cnt_chk == 1) {
      cnt_size = TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_green_drop_.size();
      for(int i=0; i<cnt_size; i++) {
        m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_ap"+std::to_string(i)+"_drop_cnt";
        m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);

        TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_green_drop_[i]->Read();
        if( (TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_green_drop_[i]->ctr32_ > (m_sknobs_value+drop_cnt_buf)) ||
           ((TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_green_drop_[i]->ctr32_+drop_cnt_buf) < m_sknobs_value)) {
          ucli_log( "[ERROR]:: WAC_AP_GRN_DROP_COUNT[%0d] CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", i, m_sknobs_value,
              TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_green_drop_[i]->ctr32_);
        }
      }

      cnt_size = TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_yel_drop_.size();
      for(int i=0; i<cnt_size; i++) {
        m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_yel"+std::to_string(i)+"_drop_cnt";
        m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);

        TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_yel_drop_[i]->Read();
        if( (TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_yel_drop_[i]->ctr32_ > (m_sknobs_value+drop_cnt_buf)) ||
           ((TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_yel_drop_[i]->ctr32_+drop_cnt_buf) < m_sknobs_value)) {
          ucli_log( "[ERROR]:: WAC_AP_YEL_DROP_COUNT[%0d] CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", i, m_sknobs_value,
              TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_yel_drop_[i]->ctr32_);
        }
      }
      cnt_size = TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_red_drop_.size();
      for(int i=0; i<cnt_size; i++) {
        m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_red"+std::to_string(i)+"_drop_cnt";
        m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);

        TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_red_drop_[i]->Read();
        if( (TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_red_drop_[i]->ctr32_ > (m_sknobs_value+drop_cnt_buf)) ||
           ((TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_red_drop_[i]->ctr32_+drop_cnt_buf) < m_sknobs_value)) {
          ucli_log( "[ERROR]:: WAC_AP_RED_DROP_COUNT[%0d] CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", i, m_sknobs_value,
              TM_REGS->get_wac_pipe_rspec(pipe)->wac_reg_->ap_red_drop_[i]->ctr32_);
        }
      }
    }
  }

  // GLB HDR
  if(wac_glb_hdr_drop_cnt_chk) {
    for (int pipe=0; pipe < NumPipe; pipe++) {
      if(!((m_pipe_mask >> pipe) & 0x1)) { continue; }
      m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe)+"_hdr_drop_cnt";
      m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
      if( (ppg_drop_total[pipe] > (m_sknobs_value+drop_cnt_buf)) || ((ppg_drop_total[pipe]+drop_cnt_buf) < m_sknobs_value) ) {
        ucli_log( "[ERROR]:: WAC_GLB_HDR_DROP_COUNT CNT MISMATCH! Exp:0x%0x Act:0x%0x\n", m_sknobs_value, ppg_drop_total[pipe]);
      }
    }
  }
}

void tm_init::tm_wac_pipe_eos_check(uint8_t pipe_num) {
uint32_t cnt_size;
  TM_INIT_FUNC_START(tm_wac_pipe_eos_check)
  ucli_log( "[TM_EOS]:: WAC Pipe%0d Check Starting...\n", pipe_num);
  // PPG_CNT min,shr,hdr
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_min_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_min_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_min_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_ppg_min_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                        TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_min_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_shr_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_shr_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_shr_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_ppg_shr_cnt:%0d for idx:%0d, pipe:%0d\n",
                           TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_shr_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_hdr_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_hdr_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_hdr_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_ppg_hdr_cnt:%0d for idx:%0d, pipe:%0d\n",
                           TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_hdr_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  // PORT_CNT min,shr,hdr
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_min_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_min_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_min_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_port_min_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_min_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_hdr_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_hdr_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_hdr_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_port_hdr_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_hdr_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_shr_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_shr_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_shr_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_port_shr_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_shr_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  // PG_CNT min,shr
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_min_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_min_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_min_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_pg_min_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_min_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }

  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_shr_cnt_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_shr_cnt_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_shr_cnt_->entry_[i]->cnt_ != 0x0) {
        ucli_log( "[ERROR]::wac_pg_shr_cnt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_shr_cnt_->entry_[i]->cnt_, i, pipe_num);
     }
  }
  // port_state
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_[i]->shr_lmt_ != 0x0) {
        ucli_log( "[ERROR]::wac_port_st shr_lmt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_[i]->shr_lmt_, i, pipe_num);
     }
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_[i]->hdr_lmt_ != 0x0) {
        ucli_log( "[ERROR]::wac_port_st hdr_lmt non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_port_st_->entry_[i]->hdr_lmt_, i, pipe_num);
     }
  }
  // ppg_drop_st
#ifdef _TOFINO_COSIM_
  m_sknobs_value = 0;
#else
  m_sknobs_name = "tb_tm.wac_drop_st_clear";
  m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
#endif

  if(!m_sknobs_value) {
    cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_drop_st_->entry_.size();
    for (int i=0; i < cnt_size; i++) {
       TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_drop_st_->entry_[i]->Read();
       if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_drop_st_->entry_[i]->drop_st_ != 0x0) {
          ucli_log( "[ERROR]::wac_ppg_drop_st non-zero:%0d for idx:%0d, pipe:%0d\n",
                   TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_ppg_drop_st_->entry_[i]->drop_st_, i, pipe_num);
       }
    }
  }

  // pg_drop_st
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_drop_st_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_drop_st_->entry_[i]->Read();
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_drop_st_->entry_[i]->drop_st_ != 0x0) {
        if (!(wac_pg_min_th[pipe_num][i] == 0 && TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_drop_st_->entry_[i]->drop_st_ == 0x1)) {
           ucli_log( "[ERROR]::wac_pg_drop_st non-zero:%0d for idx:%0d, pipe:%0d\n",
                    TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pg_drop_st_->entry_[i]->drop_st_, i, pipe_num);
        }
     }
  }
  // pfc state
  if (cfg_manager_db::get_cfg_manager()->get_backdoor() == front_door_e) {
    cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pfc_state_->pfc_state_.size();
    for (int i=0; i < cnt_size; i++) {
       TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pfc_state_->pfc_state_[i]->Read();
       if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pfc_state_->pfc_state_[i]->port_ppg_state_!= 0x0) {
          ucli_log( "[ERROR]::wac_pfc_st non-zero:%0d for idx:%0d, pipe:%0d\n",
                   TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_pfc_state_->pfc_state_[i]->port_ppg_state_, i, pipe_num);
       }
    }
  }
  // qacq state
  cnt_size = tm_regmodel_db::get_register_model()->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_.size();
  for (int i=0; i < cnt_size; i++) {
     TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->Read();
     //if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->nomin_ != 0x0) {
     //   ucli_log( "[ERROR]::wac_qacq_st nomin non-zero:%0d for idx:%0d, pipe:%0d\n",
     //            TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->nomin_ , i, pipe_num);
     //}
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->red_off_ != 0x0) {
        ucli_log( "[ERROR]::wac_qacq_st red_off_ non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->red_off_ , i, pipe_num);
     }
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->yel_off_ != 0x0) {
        ucli_log( "[ERROR]::wac_qacq_st yel_off_ non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->yel_off_ , i, pipe_num);
     }
     if (TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->gre_off_ != 0x0) {
        ucli_log( "[ERROR]::wac_qacq_st gre_off_ non-zero:%0d for idx:%0d, pipe:%0d\n",
                 TM_REGS->get_wac_pipe_mem_rspec(pipe_num)->csr_memory_wac_qacq_state_->entry_[i]->gre_off_ , i, pipe_num);
     }
  }

  TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_debug_register_->Read();
  if ((TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_debug_register_->debug_sts_ != 0) && ((fuse_pipe_dis & (1 << pipe_num)) == 0)){
       ucli_log( "[ERROR]::wac_debug register non-zero: debug_sts:0x%x\n",
                  TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_debug_register_->debug_sts_);
  }

  TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_drop_psc_full_->Read();
  if (TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_drop_psc_full_->ctr48_ != 0){
       ucli_log( "[ERROR]::wac_drop_psc_full register non-zero: ctr48:0x%x\n",
                  TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_drop_psc_full_->ctr48_);
  }

#ifndef _TOFINO_COSIM_
  if (fuse_pipe_dis != 0 && (fuse_pipe_dis & (1 << pipe_num)) == 0) {
     m_sknobs_name = "tof3.tm.wac"+std::to_string(pipe_num)+"_ctr_drop_fuse_pipe";
     m_sknobs_value = sknobs_get_value((char*)m_sknobs_name.c_str(), 0);
     TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->ctr_drop_fuse_pipe_->Read();
     if ((TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->ctr_drop_fuse_pipe_->ctr32_ == 0 && m_sknobs_value != 0) ||
         (TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->ctr_drop_fuse_pipe_->ctr32_ != 0 && m_sknobs_value == 0)){
          ucli_log( "[ERROR]::wac%0d_ctr_drop_fuse_pipe register:%0d  expected:%0d\n",
                     pipe_num, TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->ctr_drop_fuse_pipe_->ctr32_, m_sknobs_value);
     }
  }
#endif

  ucli_log( "[TM_EOS]:: WAC Pipe%0d Check Done!\n", pipe_num);
  TM_INIT_FUNC_END(tm_wac_pipe_eos_check)
}

#endif  // 0

static void tm_wac_buf_full_eos_check(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id) {
  uint32_t addr, data;
  ucli_log("[TM_EOS]:: WAC Buffer Full Check Starting...\n");

  for (int pipe_num = 0; pipe_num < 4; pipe_num++) {
    // TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_drop_buf_full_->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_wac_top.wac_pipe[pipe_num]
                        .wac_reg.wac_drop_buf_full);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    uint32_t c48_lo =
        getp_tof3_cnt48_inc_wac_drop_buf_full_0_2_ctr48_31_0(&data);
    lld_subdev_read_register(dev_id, subdev_id, addr + 4, &data);
    uint32_t c48_hi =
        getp_tof3_cnt48_inc_wac_drop_buf_full_1_2_ctr48_47_32(&data);

    // if
    // (TM_REGS->get_wac_pipe_rspec(pipe_num)->wac_reg_->wac_drop_buf_full_->ctr48_
    // != 0){
    if ((c48_lo | c48_hi) != 0) {
      ucli_log(
          "[ERROR]::wac_drop_buf_full register non-zero: ctr48:0x%08x_%08x\n",
          c48_hi,
          c48_lo);
    }
  }

  ucli_log("[TM_EOS]:: WAC Buffer Full Check Done!\n");
}

static void tm_caa_eos_check(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  uint32_t timeout_en = 0;
  uint32_t exp_blk_usect = 2;  // default if timeout_en=0 (also default)

  uint64_t enbl_val = 0;
  if (subdev_id > 1) {
    enbl_val = 0x0f0f0f0f0f0f0f0f;
  } else {
    enbl_val = 0x0f0f0f0f;
  }

  // blk_usecnt
  cnt_size =
      tof3_reg_device_select_tm_top_tm_caa_top_epipe_array_count;  // TM_REGS->get_tm()->tm_caa_top_->epipe_.size();
  addr = offsetof(tof3_reg, device_select.tm_top.tm_caa_top.ctrl);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  timeout_en = getp_tof3_caa_ctrl_r_timeout_en(&data);
  if (timeout_en) {
    exp_blk_usect = 0;
  }

  for (int i = 0; i < (int)cnt_size; i++) {
    if (!((enbl_val >> i) & 0x1ul)) {
      // No need to check non enabled blocks
      continue;
    }
    // TM_REGS->get_tm()->tm_caa_top_->epipe_[i]->blks_usecnt_->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_caa_top.epipe[i].blks_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_caa_epipe_blocks_usecnt_r_value(&data);

    // if(TM_REGS->get_tm()->tm_caa_top_->epipe_[i]->blks_usecnt_->value_ != 0)
    // {
    if (fld != exp_blk_usect) {
      ucli_log(
          "[ERROR]::caa pipe %d blks_usecnt: exp=0x%x : got=0x%x "
          "<timeout_en=%d>\n",
          i,
          exp_blk_usect,
          fld,
          timeout_en);
    }
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_caa_top.epipe[i].blks_max_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    ucli_log("[TM_EOS]::pipe %d blks_max_usecnt 0x%x\n", i, data);
  }

  cnt_size =
      tof3_reg_device_select_tm_top_tm_caa_top_block_array_count;  // TM_REGS->get_tm()->tm_caa_top_->block_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_tm()->tm_caa_top_->block_[i]->addr_usecnt_->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_caa_top.block[i].addr_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_caa_block_addr_usecnt_r_value(&data);
    if (i < 191) {
      // if(TM_REGS->get_tm()->tm_caa_top_->block_[i]->addr_usecnt_->value_ !=
      // 0) {
      if (fld != 0) {
        ucli_log("[ERROR]::caa block %d addr_usecnt non-zero: 0x%x\n", i, fld);
      }
    } else if ((tof3_caa_blk_vld[i / 32] >> (i % 32)) & 1) {
      // if(TM_REGS->get_tm()->tm_caa_top_->block_[i]->addr_usecnt_->value_ !=
      // 2) {
      if (fld != 2) {
        ucli_log("[ERROR]::caa block %d addr_usecnt non-two: 0x%x\n", i, fld);
      }
    }
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_caa_top.block[i].addr_max_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    ucli_log("[TM_EOS]:: icaa block %d addr_max_usecnt: %0dh\n", i, data);
  }
}

// CLC Common
static void tm_clc_common_eos_check(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id) {
  uint32_t addr, data;
  uint32_t cnt_size;

  ucli_log("[TM_EOS]:: CLC Common Checks Starting...\n");

  // qlc_pt_ct_cnt pipe0
  cnt_size =
      tof3_reg_device_select_tm_top_tm_clc_top_clc_common_qclc_pt_ct_cnt_pipe0_array_count;  // TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_.size();
  for (int grp = 0; grp < 1; grp++) {
    for (int i = 0; i < (int)cnt_size; i++) {
      // TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_[i]->Read();
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_clc_top.clc_common[grp]
                          .qclc_pt_ct_cnt_pipe0[i]);
      lld_subdev_read_register(dev_id, subdev_id, addr, &data);
      uint32_t cnt0 = getp_tof3_qclc_pt_ct_cnt_cnt0(&data);
      uint32_t cnt1 = getp_tof3_qclc_pt_ct_cnt_cnt1(&data);
      uint32_t cnt2 = getp_tof3_qclc_pt_ct_cnt_cnt2(&data);
      uint32_t cnt3 = getp_tof3_qclc_pt_ct_cnt_cnt3(&data);

      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_[i]->cnt0_!=
      // 0) {
      if (cnt0 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe0[%0d] non-zero cnt0_: %0d\n",
                 i,
                 cnt0);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_[i]->cnt1_!=
      // 0) {
      if (cnt1 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe0[%0d] non-zero cnt1_: %0d\n",
                 i,
                 cnt1);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_[i]->cnt2
      // != 0) {
      if (cnt2 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe0[%0d] non-zero cnt2_: %0d\n",
                 i,
                 cnt2);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe0_[i]->cnt3
      // != 0) {
      if (cnt3 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe0[%0d] non-zero cnt3_: %0d\n",
                 i,
                 cnt3);
      }
    }
    // qlc_pt_ct_cnt pipe1
    for (int i = 0; i < (int)cnt_size; i++) {
      // TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe1_[i]->Read();
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_clc_top.clc_common[grp]
                          .qclc_pt_ct_cnt_pipe1[i]);
      lld_subdev_read_register(dev_id, subdev_id, addr, &data);
      uint32_t cnt0 = getp_tof3_qclc_pt_ct_cnt_cnt0(&data);
      uint32_t cnt1 = getp_tof3_qclc_pt_ct_cnt_cnt1(&data);
      uint32_t cnt2 = getp_tof3_qclc_pt_ct_cnt_cnt2(&data);
      uint32_t cnt3 = getp_tof3_qclc_pt_ct_cnt_cnt3(&data);

      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe1_[i]->cnt0_!=
      // 0) {
      if (cnt0 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe1[%0d] non-zero cnt0_: %0d\n",
                 i,
                 cnt0);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe1_[i]->cnt1_!=
      // 0) {
      if (cnt1 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe1[%0d] non-zero cnt1_: %0d\n",
                 i,
                 cnt1);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe1_[i]->cnt2
      // != 0) {
      if (cnt2 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe1[%0d] non-zero cnt2_: %0d\n",
                 i,
                 cnt2);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe1_[i]->cnt3
      // != 0) {
      if (cnt3 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe1[%0d] non-zero cnt3_: %0d\n",
                 i,
                 cnt3);
      }
    }
    // qlc_pt_ct_cnt pipe2
    for (int i = 0; i < (int)cnt_size; i++) {
      // TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe2_[i]->Read();
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_clc_top.clc_common[grp]
                          .qclc_pt_ct_cnt_pipe2[i]);
      lld_subdev_read_register(dev_id, subdev_id, addr, &data);
      uint32_t cnt0 = getp_tof3_qclc_pt_ct_cnt_cnt0(&data);
      uint32_t cnt1 = getp_tof3_qclc_pt_ct_cnt_cnt1(&data);
      uint32_t cnt2 = getp_tof3_qclc_pt_ct_cnt_cnt2(&data);
      uint32_t cnt3 = getp_tof3_qclc_pt_ct_cnt_cnt3(&data);

      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe2_[i]->cnt0_!=
      // 0) {
      if (cnt0 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe2[%0d] non-zero cnt0_: %0d\n",
                 i,
                 cnt0);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe2_[i]->cnt1_!=
      // 0) {
      if (cnt1 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe2[%0d] non-zero cnt1_: %0d\n",
                 i,
                 cnt1);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe2_[i]->cnt2
      // != 0) {
      if (cnt2 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe2[%0d] non-zero cnt2_: %0d\n",
                 i,
                 cnt2);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe2_[i]->cnt3
      // != 0) {
      if (cnt3 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe2[%0d] non-zero cnt3_: %0d\n",
                 i,
                 cnt3);
      }
    }
    // qlc_pt_ct_cnt pipe3
    for (int i = 0; i < (int)cnt_size; i++) {
      // TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe3_[i]->Read();
      addr = offsetof(tof3_reg,
                      device_select.tm_top.tm_clc_top.clc_common[grp]
                          .qclc_pt_ct_cnt_pipe3[i]);
      lld_subdev_read_register(dev_id, subdev_id, addr, &data);
      uint32_t cnt0 = getp_tof3_qclc_pt_ct_cnt_cnt0(&data);
      uint32_t cnt1 = getp_tof3_qclc_pt_ct_cnt_cnt1(&data);
      uint32_t cnt2 = getp_tof3_qclc_pt_ct_cnt_cnt2(&data);
      uint32_t cnt3 = getp_tof3_qclc_pt_ct_cnt_cnt3(&data);

      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe3_[i]->cnt0_!=
      // 0) {
      if (cnt0 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe3[%0d] non-zero cnt0_: %0d\n",
                 i,
                 cnt0);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe3_[i]->cnt1_!=
      // 0) {
      if (cnt1 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe3[%0d] non-zero cnt1_: %0d\n",
                 i,
                 cnt1);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe3_[i]->cnt2
      // != 0) {
      if (cnt2 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe3[%0d] non-zero cnt2_: %0d\n",
                 i,
                 cnt2);
      }
      // if
      // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->qclc_pt_ct_cnt_pipe3_[i]->cnt3
      // != 0) {
      if (cnt3 != 0) {
        ucli_log("[ERROR]::qclc_pt_ct_cnt_pipe3[%0d] non-zero cnt3_: %0d\n",
                 i,
                 cnt3);
      }
    }

    // Check mc_tot_cnt
    // TM_REGS->get_tm()->tm_clc_top_->clc_common_->ct_tot_cnt_->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_clc_top.clc_common[grp].ct_tot_cnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    uint32_t uc_tot_cnt = getp_tof3_qclc_pt_tot_cnt_uc_tot_cnt(&data);
    uint32_t mc_tot_cnt = getp_tof3_qclc_pt_tot_cnt_mc_tot_cnt(&data);

    // if
    // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->ct_tot_cnt_->mc_tot_cnt_!=
    // 0) {
    if (mc_tot_cnt != 0) {
      ucli_log("[ERROR]::mc_ct_cnt non-zero cnt: %0d\n", mc_tot_cnt);
    }
    // Check uc_tot_cnt
    // if
    // (TM_REGS->get_tm()->tm_clc_top_->clc_common_->ct_tot_cnt_->uc_tot_cnt_!=
    // 0) {
    if (uc_tot_cnt != 0) {
      ucli_log("[ERROR]::uc_ct_cnt non-zero cnt: %0d\n", uc_tot_cnt);
    }
  }
  ucli_log("[TM_EOS]:: CLC Common Checks Done!\n");
}

static void tm_clc_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  ucli_log("[TM_EOS]:: CLC Pipe%0d Checks Starting...\n", pipe_num);
  // egress_port_ct_state
  cnt_size =
      tof3_reg_device_select_tm_top_tm_clc_top_clc_common_egress_port_ct_state_array_dim_1_count;  // TM_REGS->get_clc_pipe_rspec(pipe_num)->egress_port_ct_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_clc_pipe_rspec(pipe_num)->egress_port_ct_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_clc_top.clc_common[0]
                        .egress_port_ct_state[pipe_num][i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_port_ct_state_ct_state(&data);
    // if(TM_REGS->get_clc_pipe_rspec(pipe_num)->egress_port_ct_state_[i]->ct_state_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::egress_port_ct_state non-zero for pipe:%0d, i:%0d, state0: "
          "%0d\n",
          pipe_num,
          i,
          fld);
    }
  }
  // ingress_port_ct_state
  cnt_size =
      tof3_reg_device_select_tm_top_tm_clc_top_clc_common_ingress_port_ct_state_array_dim_1_count;  // TM_REGS->get_clc_pipe_rspec(pipe_num)->ingress_port_ct_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_clc_pipe_rspec(pipe_num)->ingress_port_ct_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_clc_top.clc_common[0]
                        .ingress_port_ct_state[pipe_num][i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_port_ct_state_ct_state(&data);
    // if(TM_REGS->get_clc_pipe_rspec(pipe_num)->ingress_port_ct_state_[i]->ct_state_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::ingress_port_ct_state non-zero for pipe:%0d, i:%0d, "
          "state0: %0d\n",
          pipe_num,
          i,
          fld);
    }
  }
  ucli_log("[TM_EOS]:: CLC Pipe%0d Checks Done!\n", pipe_num);
}

// PEX Pipe
static void tm_pex_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  (void)pipe_num;
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  // uint32_t lim;
  // uint32_t wm_comp;

  ucli_log("[TM_EOS]:: PEX Pipe%0d Check Starting...\n", pipe_num);
  cnt_size =
      tof3_reg_device_select_tm_top_tm_pex_top_pex_pt_state_array_count;  // TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_.size();

  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_pex_top.pex[pipe_num].pt_state[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);

    fld = getp_tof3_pex_pt_state_state0(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state0_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state0: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state1(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state1_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state1: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state2(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state2_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state2: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state3(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state3_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state3: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state4(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state4_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state4: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state5(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state5_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state5: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state6(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state6_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state6: %0d\n",
          pipe_num,
          i,
          fld);
    }
    fld = getp_tof3_pex_pt_state_state7(&data);
    // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->pt_state_[i]->state7_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::pex_pt_state non-zero for pipe:%0d, i:%0d, state7: %0d\n",
          pipe_num,
          i,
          fld);
    }
  }
  cnt_size =
      tof3_reg_device_select_tm_top_tm_pex_top_pex_q_empty_array_count;  // tm_regmodel_db::get_register_model()->get_pex_pipe_rspec(pipe_num)->q_empty_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_pex_pipe_rspec(pipe_num)->q_empty_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_pex_top.pex[pipe_num].q_empty[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);

    fld = getp_tof3_pex_q_empty_empty(&data);
    if (i % 3 == 2) {
      // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->q_empty_[i]->empty_ !=
      // 0x800000ff) {
      if (fld != 0x00000ff) {
        ucli_log(
            "[ERROR]::pex q_empty not 0x00000ff for pipe:%0d, i:%0d, "
            "empty_state: 0x%0x\n",
            pipe_num,
            i,
            fld);
      }
    } else {
      // if (TM_REGS->get_pex_pipe_rspec(pipe_num)->q_empty_[i]->empty_ !=
      // 0xffffffff) {
      if (fld != 0xffffffff) {
        ucli_log(
            "[ERROR]::pex q_empty not 0xffffffff for pipe:%0d, i:%0d, "
            "empty_state: 0x%0x\n",
            pipe_num,
            i,
            fld);
      }
    }
  }
  // debug register
  if (1) {  // cfg_manager_db::get_cfg_manager()->get_backdoor() ==
            // front_door_e) {
    // TM_REGS->get_pex_pipe_rspec(pipe_num)->pex_debug_register_->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_pex_top.pex[pipe_num].pex_debug_register);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);

    fld = getp_tof3_pex_debug_register_debug_sts(&data);
    // if(TM_REGS->get_pex_pipe_rspec(pipe_num)->pex_debug_register_->debug_sts_
    // != 0) {
    // Debug_sts :
    //"[31:10] - Reserved"
    //"[9] psc_discard_ph_fifo_empty[1]"
    //"[8] psc_discard_ph_fifo_empty[0]"
    //"[7] PH port/priority fifo empty"
    //"[6] psc_ph_afifo_empty"
    //"[5] pex_sch_pfc_fifo_empty"
    //"[4] pex_qlc_dis_cred_fifo_empty"
    //"[3] pex_sch_dis_cred_fifo0_empty"
    //"[2] pex_sch_dis_cred_fifo1_empty"
    //"[1] pex_sch_cred_fifo0_empty"
    //"[0] speed_cell_error";
    //(08/06/20) kbhushan - As per Mike Ferrera pex_sch_pfc_fifo_empty bit
    // should be masked since
    //                      pex->sch pfc updates are continuous, making that bit
    //                      indeterministic.

    if ((fld & 0xffffffdf) != 0x3de) {
      ucli_log("[ERROR]::pex_debug register non-zero: debug_sts:0x%x\n", fld);
    }
  }

  cnt_size =
      tof3_reg_device_select_tm_top_tm_pex_top_pex_pt_mgc_cred_array_count;  // tm_regmodel_db::get_register_model()->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_[i]->Read();
    addr = offsetof(
        tof3_reg, device_select.tm_top.tm_pex_top.pex[pipe_num].pt_mgc_cred[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    uint32_t data2, cred0, cred1, cred2, cred3;
    lld_subdev_read_register(dev_id, subdev_id, addr + 4, &data2);
    cred0 = getp_tof3_pex_mgc_cred_cnt_cred0(&data);
    cred1 = getp_tof3_pex_mgc_cred_cnt_cred1(&data);
    cred2 = getp_tof3_pex_mgc_cred_cnt_cred2(&data);
    cred3 = getp_tof3_pex_mgc_cred_cnt_cred3(&data);

    // ucli_log( "[TM_EOS]:: PEX Pipe%0d pt_epb_credit MAC:%0d, cred0: %0d,
    // cred1:%0d, cred2:%0d, cred3:%0d\n", pipe_num, i,
    //                      TM_REGS->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_[i]->cred0_,
    //                      TM_REGS->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_[i]->cred1_,
    //                      TM_REGS->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_[i]->cred2_,
    //                      TM_REGS->get_tm()->tm_pex_top_->pex_[pipe_num]->pt_epb_cred_[i]->cred3_);
    ucli_log(
        "[TM_EOS]:: PEX Pipe%0d pt_epb_credit MAC:%0d, cred0: %0d, cred1:%0d, "
        "cred2:%0d, cred3:%0d\n",
        pipe_num,
        i,
        cred0,
        cred1,
        cred2,
        cred3);
  }
  ucli_log("[TM_EOS]:: PEX Pipe%0d Check Done!\n", pipe_num);
}

static void tm_qac_common_eos_check(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id) {
  uint32_t addr, data, fld;

  ucli_log("[TM_EOS]:: QAC Common Check Starting...\n");
  // PRE FIFO per PIPE Cnt

  for (int fifo = 0; fifo < 4; fifo++) {
    // PIPE0
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe0_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_pkt_pipe0[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_pkt_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe0_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC0 PRE FIFO%0d Pkt Count not quiesced!  Actual=:0x%0x!\n",
          fifo,
          fld);
    }
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe0_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_cell_pipe0[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_cell_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe0_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC0 PRE FIFO%0d Cell Count not quiesced!  "
          "Actual=:0x%0x!\n",
          fifo,
          fld);
    }

    // PIPE1
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe1_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_pkt_pipe1[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_pkt_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe1_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC1 PRE FIFO%0d Pkt Count not quiesced!  Actual=:0x%0x!\n",
          fifo,
          fld);
    }
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe1_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_cell_pipe1[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_cell_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe1_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC1 PRE FIFO%0d Cell Count not quiesced!  "
          "Actual=:0x%0x!\n",
          fifo,
          fld);
    }

    // PIPE2
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe2_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_pkt_pipe2[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_pkt_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe2_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC2 PRE FIFO%0d Pkt Count not quiesced!  Actual=:0x%0x!\n",
          fifo,
          fld);
    }
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe2_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_cell_pipe2[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_cell_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe2_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC2 PRE FIFO%0d Cell Count not quiesced!  "
          "Actual=:0x%0x!\n",
          fifo,
          fld);
    }

    // PIPE3
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe3_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_pkt_pipe3[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_pkt_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_pkt_pipe3_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC3 PRE FIFO%0d Pkt Count not quiesced!  Actual=:0x%0x!\n",
          fifo,
          fld);
    }
    // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe3_[fifo]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_common.qac_common
                        .qac_pre_fifo_cnt_cell_pipe3[fifo]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qac_pre_fifo_cnt_cell_cnt_pkt(&data);

    // if
    // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_pre_fifo_cnt_cell_pipe3_[fifo]->cnt_pkt_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC3 PRE FIFO%0d Cell Count not quiesced!  "
          "Actual=:0x%0x!\n",
          fifo,
          fld);
    }
  }
  // DOD
  // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_dod_cnt_cell_->Read();
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_dod_cnt_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_qac_dod_cnt_cell_cnt(&data);

  // if
  // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_dod_cnt_cell_->cnt_
  // != 0) {
  if (fld != 0) {
    ucli_log("[ERROR]::QAC DOD Cell Count not quiesced!  Actual=:0x%0x!\n",
             fld);
  }

  // MC Pkt Cnt
  // TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_mcct_cnt_pkt_->Read();
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_qac_top.qac_common.qac_common.qac_dod_cnt_cell);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_qac_mcct_cnt_pkt_cnt_pkt(&data);

  // if
  // (TM_REGS->get_tm()->tm_qac_top_->qac_common_->qac_common_->qac_mcct_cnt_pkt_->cnt_pkt_
  // != 0) {
  if (fld != 0) {
    ucli_log("[ERROR]::QAC MC Pkt Count not quiesced!  Actual=:0x%0x!\n", fld);
  }
  ucli_log("[TM_EOS]:: QAC Common Check Done!\n");
}

static void tm_qac_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  // uint8_t dynamic_access;
  // uint8_t pex_qac_drv_en;
  uint32_t cnt_size;
  // uint32_t deviation;

  ucli_log("[TM_EOS]:: QAC Pipe%0d Queue Drop State Check!\n", pipe_num);
  cnt_size =
      tof3_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_queue_drop_state_array_count;  // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_pipe[pipe_num]
                        .qac_reg.queue_drop_state[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_queue_drop_state_st(&data);

    // if(TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_state_[i]->st_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC Pipe%0d Queue%0d Drop State Not Quiesced! Act:0x%0x \n",
          pipe_num,
          i,
          fld);
    }
  }
  cnt_size =
      tof3_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_queue_drop_yel_state_array_count;  // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_yel_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_yel_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_pipe[pipe_num]
                        .qac_reg.queue_drop_yel_state[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_queue_drop_state_st(&data);

    // if(TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_yel_state_[i]->st_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC Pipe%0d Queue%0d Drop yel State Not Quiesced! "
          "Act:0x%0x \n",
          pipe_num,
          i,
          fld);
    }
  }
  cnt_size =
      tof3_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_queue_drop_red_state_array_count;  // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_red_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_red_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_pipe[pipe_num]
                        .qac_reg.queue_drop_red_state[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_queue_drop_state_st(&data);

    // if(TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->queue_drop_red_state_[i]->st_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC Pipe%0d Queue%0d Drop red State Not Quiesced! "
          "Act:0x%0x\n",
          pipe_num,
          i,
          fld);
    }
  }
  cnt_size =
      tof3_reg_device_select_tm_top_tm_qac_top_qac_pipe_qac_reg_port_drop_state_array_count;  // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_state_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_state_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qac_top.qac_pipe[pipe_num]
                        .qac_reg.port_drop_state[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_port_drop_state_st(&data);

    // if(TM_REGS->get_qac_pipe_rspec(pipe_num)->qac_reg_->port_drop_state_[i]->st_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QAC Pipe%0d Port%0d Drop State Not Quiesced! Act:0x%0x\n",
          pipe_num,
          i,
          fld);
    }
  }
}

static void tm_psc_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  // Check PH used per pipe
  ucli_log("[TM_EOS]:: PSC Pipe%0d PH Check!\n", pipe_num);

  // TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->psc_ph_used_->Read();
  addr = offsetof(tof3_reg,
                  device_select.tm_top.tm_psc_top.psc[pipe_num].psc_ph_used);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_psc_ph_used_ph_used(&data);

  // if (TM_REGS->get_tm()->tm_psc_top_->psc_[pipe_num]->psc_ph_used_->ph_used_
  // != 0) {
  if (fld != 0) {
    ucli_log(
        "[ERROR]::PSC Pipe%0d PH UseCnt:%0d not quiesced!\n", pipe_num, fld);
  }
  // Only when pipe enabled
  addr = offsetof(
      tof3_reg,
      device_select.tm_top.tm_psc_top.psc_common.epipe[pipe_num].enable);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_psc_epipe_enable_r_value(&data);

  // if
  // (TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->enable_->value_
  // == 1) {
  if (fld == 1) {
    // Read Max UseCnt and clear
    ucli_log("[TM_EOS]:: PSC Pipe%0d Blk MaxUseCnt Check!\n", pipe_num);
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->blks_max_usecnt_->value_
    // = 0;
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_psc_top.psc_common.epipe[pipe_num]
                        .blks_max_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->blks_max_usecnt_->Write();
    lld_subdev_write_register(dev_id, subdev_id, addr, 0);
  }
}

static void tm_psc_common_eos_check(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id) {
  uint32_t addr, data, fld;
  // uint32_t psc_full_threshold;
  // uint32_t expd_blk_max_usecnt;
  uint32_t cnt_size;
  uint8_t owner;
  uint64_t blk_vld;
  uint32_t psc_blk_used[NumPipe];
  uint64_t blk_vld_exp;
  uint16_t usecnt;
  // uint16_t maxusecnt;

  ucli_log("[TM_EOS]:: PSC PSM Block Check!\n");

  // psc_full_threshold = 1016;

  // Calculate max usecnt per block - 4 banks
  // 10 is for buffering from psc receive full to stop
  // expd_blk_max_usecnt = 4*(psc_full_threshold+10);

  // Initialize psc_blk_used to 0
  memset(psc_blk_used, 0, sizeof(psc_blk_used));
#if 0
  for (int i = 0; i < NumPipe; i++) {
    if (!((m_pipe_mask >> i) & 0x1)) {
      continue;
    }
    psc_blk_used[i] = 0;
  }
#endif
  // Read Block Vld
  // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_valid_->Read();
  // thau blk_vld =
  // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_valid_->value_[0]; //TBD
  // - fix to 96-bits
  uint32_t bv[3];
  addr = offsetof(tof3_reg,
                  device_select.tm_top.tm_psc_top.psc_common.block_valid);
  lld_subdev_read_register(dev_id, subdev_id, addr, &bv[0]);
  lld_subdev_read_register(dev_id, subdev_id, addr + 4, &bv[1]);
  lld_subdev_read_register(dev_id, subdev_id, addr + 8, &bv[2]);

  // Check PSM Blocks
  cnt_size = 96;  // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_.size();
                  // //PSM Blocks
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->state_->Read();
    addr =
        offsetof(tof3_reg, device_select.tm_top.tm_psc_top.psc_common.block[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_psc_block_state_r_state(&data);

    // Check if state empty
    // if
    // (TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->state_->state_
    // != 0) {
    if (fld != 0) {
      ucli_log("[ERROR]::PSC Block%0d State non-empty. State:%0d\n", i, fld);
    }
    // Check if not free
    // owner =
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->state_->owner_;
    owner = getp_tof3_psc_block_state_r_owner(&data);
    fld = getp_tof3_psc_block_state_r_free(&data);

    if (owner >= NumPipe) continue;

    // if (TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->state_->free_
    // == 0) {
    if (fld == 0) {
      if (((m_pipe_mask >> owner) & 0x1)) {
        // Count number of non-free blocks per pipe
        psc_blk_used[owner]++;
      }
    }
    // blk_vld =
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_valid_->value_[i];
    blk_vld = (bv[i / 32] >> (i % 32)) & 1;

    blk_vld_exp = !((tof3_psc_blk_dis[i / 32] >> (i % 32)) & 1);
    // Check if fuse_block_dis is set that ready is not set
    if (blk_vld_exp != blk_vld) {
      ucli_log("[ERROR]::PSC Block%0d Valid Bit %016" PRIx64
               " != blk_vld_exp:%016" PRIx64 " \n",
               i,
               blk_vld,
               blk_vld_exp);
    }

    // Check read usecnt
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->addr_usecnt_->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_psc_top.psc_common.block[i].addr_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    // usecnt =
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->block_[i]->addr_usecnt_->value_;
    usecnt = getp_tof3_psc_block_addr_usecnt_r_value(&data);
    if (usecnt != 0) {
      ucli_log("[ERROR]::PSC Block%0d UseCnt %0d should be 0\n", i, usecnt);
    }
  }

  for (int pipe_num = 0; pipe_num < NumPipe; pipe_num++) {
    if (!((m_pipe_mask >> pipe_num) & 0x1)) {
      continue;
    }
    // Check pipe_enable
    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->enable_->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_psc_top.psc_common.epipe[pipe_num].enable);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = data & 1;  // hack?
    // if
    // (TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->enable_->value_
    // == 1) {
    if (fld == 1) {
      // Check if more than 2 blocks not free per pipe
      if ((psc_blk_used[pipe_num] != 0) & (psc_blk_used[pipe_num] != 3)) {
        ucli_log("[ERROR]::PSC Block%0d is holding %0d blocks (!=3)!\n",
                 pipe_num,
                 psc_blk_used[pipe_num]);
      } else {
        ucli_log("PSC Block%0d is holding %0d blocks!\n",
                 pipe_num,
                 psc_blk_used[pipe_num]);
      }
    } else {
      if (psc_blk_used[pipe_num] != 0) {
        ucli_log(
            "[ERROR]::PSC Block%0d is holding %0d blocks but NOT enabled!\n",
            pipe_num,
            psc_blk_used[pipe_num]);
      }
    }

    // TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->blks_usecnt_->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_psc_top.psc_common.epipe[pipe_num].blks_usecnt);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = data & 0x7f;  // hack?

    // if(TM_REGS->get_tm()->tm_psc_top_->psc_common_->epipe_[pipe_num]->blks_usecnt_->value_
    // != 0) {
    if ((fld != 0) && (fld != 3)) {
      ucli_log(
          "[ERROR]::PSC Pipe%0d BLK UseCnt:%0d not quiesced!\n", pipe_num, fld);
    }
  }
  ucli_log("[TM_EOS]:: PSC PSM Block Done!\n");
}

static void tm_qlc_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;

  ucli_log("[TM_EOS]:: QLC Pipe%0d Counter Checks Starting...\n", pipe_num);
  // Check QLC Count
  // TM_REGS->get_qlc_pipe_rspec(pipe_num)->dis_cred_->Read();
  addr = offsetof(tof3_reg,
                  device_select.tm_top.tm_qlc_top.qlc[pipe_num].dis_cred);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_qlc_dis_cred_qlc_dis_cred(&data);

  uint32_t m_sknobs_value = 110;

  // if (TM_REGS->get_qlc_pipe_rspec(pipe_num)->dis_cred_->qlc_dis_cred_ !=
  // m_sknobs_value) { //Default value of 32
  if (fld != m_sknobs_value) {  // Default value of 32
    ucli_log(
        "[ERROR]::QLC Pipe%0d Discard Credit MISMATCH!  Actual:%0d "
        "Expected:%0d!\n",
        pipe_num,
        fld,
        32);
  }

  cnt_size =
      tof3_reg_device_select_tm_top_tm_qlc_top_qlc_dis_qlen_array_count;  // TM_REGS->get_qlc_pipe_rspec(pipe_num)->dis_qlen_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_qlc_pipe_rspec(pipe_num)->dis_qlen_[i]->Read();
    addr = offsetof(tof3_reg,
                    device_select.tm_top.tm_qlc_top.qlc[pipe_num].dis_qlen[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_qlc_dis_qlen_cnt(&data);
    // if (TM_REGS->get_qlc_pipe_rspec(pipe_num)->dis_qlen_[i]->cnt_ != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]::QLC Pipe%0d Discard Qlen%0d not quiesced!  Actual:%0d!\n",
          pipe_num,
          i,
          fld);
    }
  }
  ucli_log("[TM_EOS]:: QLC Pipe%0d Counter Done!\n", pipe_num);
}

static void tm_sch_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  // Check SCH Count
  ucli_log("[TM_EOS]:: SCH Pipe%0d Counter Check Starting...\n", pipe_num);

#if 0
    // p_occ
    cnt_size = TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_.size();
    for (int i=0; i<cnt_size; i++) {
      TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_[i]->Read();
      /* TODO
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_[i]->node_occ_ != 0) {
        ucli_log( "[ERROR]:: SCH_P_OCC[%0d] node_occ not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_[i]->node_occ_);
      }
      */
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_[i]->node_occ_cntr_!= 0) {
        ucli_log( "[ERROR]:: SCH_P_OCC[%0d] node_occ_cntr not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->p_occ_mem_->entry_[i]->node_occ_cntr_);
      }
    }
    // l1_occ
    cnt_size = TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_.size();
    for (int i=0; i<cnt_size; i++) {
      TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_[i]->Read();
      /* TODO
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_[i]->node_occ_ != 0) {
        ucli_log( "[ERROR]:: SCH_L1_OCC[%0d] node_occ not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_[i]->node_occ_);
      }
      */
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_[i]->node_occ_cntr_!= 0) {
        ucli_log( "[ERROR]:: SCH_L1_OCC[%0d] node_occ_cntr not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->l1_occ_mem_->entry_[i]->node_occ_cntr_);
      }
    }
    // q_occ
    cnt_size = TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_.size();
    for (int i=0; i<cnt_size; i++) {
      TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_[i]->Read();
      /* TODO
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_[i]->node_occ_ != 0) {
        ucli_log( "[ERROR]:: SCH_L1_OCC[%0d] node_occ not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_[i]->node_occ_);
      }
      */
      if(TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_[i]->node_occ_cntr_!= 0) {
        ucli_log( "[ERROR]:: SCH_L1_OCC[%0d] node_occ_cntr not quiesced! Actual:0x%0x \n", i,
            TM_REGS->get_sch_pipe_mem_rspec(pipe_num)->q_occ_mem_->entry_[i]->node_occ_cntr_);
      }
    }
#endif  // 0
  // port pex credit
  cnt_size =
      tof3_reg_device_select_tm_top_tm_sch_top_sch_port_pex_status_mem_array_count;
  for (int i = 0; i < (int)cnt_size; i++) {
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_sch_top.sch[pipe_num].port_pex_status_mem[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_sch_port_pex_status_r_pri_curr_credits(&data);
    // if(TM_REGS->get_sch_pipe_rspec(pipe_num)->port_pex_status_mem_[i]->pri_curr_credits_
    // != 0) {
    if (fld != 0) {
      ucli_log(
          "[ERROR]:: SCH_PORT_PEX %0d credit not quiesced! Actual:0x%0x \n",
          i,
          fld);
    }
  }
#if 0
  // mac pex credit
  cnt_size =
      tof3_reg_device_select_tm_top_tm_sch_top_sch_port_pex_status_mem_array_count;  // TM_REGS->get_sch_pipe_rspec(pipe_num)->mac_pex_status_mem_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    uint32_t sch;
    // TM_REGS->get_sch_pipe_rspec(pipe_num)->mac_pex_status_mem_[i]->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_sch_top.sch[sch].mac_pex_status_mem[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    fld = getp_tof3_sch_mac_pex_status_r_pri_curr_credits(&data);
    // if(TM_REGS->get_sch_pipe_rspec(pipe_num)->mac_pex_status_mem_[i]->pri_curr_credits_
    // != 0) {
    if (fld != 0) {
      ucli_log("[ERROR]:: SCH_MAC_PEX %0d credit not quiesced! Actual:0x%0x \n",
               i,
               fld);
    }
  }
#endif

  ucli_log("[TM_EOS]:: SCH Pipe%0d Counter Check Done!\n", pipe_num);
}

static void tm_pre_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num) {
  uint32_t addr, data, fld;
  uint32_t cnt_size;
  // Check PRE Count
  ucli_log("[TM_EOS]:: PRE Pipe%0d Counter Check Starting...\n", pipe_num);

  cnt_size =
      tof3_reg_device_select_tm_top_tm_pre_top_pre_table_ph_count_array_count;  // TM_REGS->get_pre_pipe_rspec(pipe_num)->table_ph_count_.size();
  for (int i = 0; i < (int)cnt_size; i++) {
    // TM_REGS->get_pre_pipe_rspec(pipe_num)->table_ph_count_[i]->Read();
    addr = offsetof(
        tof3_reg,
        device_select.tm_top.tm_pre_top.pre[pipe_num].table_ph_count[i]);
    lld_subdev_read_register(dev_id, subdev_id, addr, &data);
    // if
    // (TM_REGS->get_pre_pipe_rspec(pipe_num)->table_ph_count_[i]->table_ph_count_!=
    // 0) {
    if (data != 0) {
      ucli_log(
          "[ERROR]::PRE Pipe%0d TABLE ID:%0d PH COUNT Expect:0 Actual:%0d!\n",
          pipe_num,
          i,
          data);
    }
  }

  // TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->Read();
  addr = offsetof(tof3_reg,
                  device_select.tm_top.tm_pre_top.pre[pipe_num].credit_log);
  lld_subdev_read_register(dev_id, subdev_id, addr, &data);
  fld = getp_tof3_pre_fifo_credit_log_l0_fifo_credit(&data);
  // if (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l0_fifo_credit_ !=
  // 0x12) {
  if (fld != 0x12) {
    ucli_log("[ERROR]::PRE Pipe%0d L0 FIFO CREDIT Expect:0x12 Actual:%0d!\n",
             pipe_num,
             fld);
  }
  fld = getp_tof3_pre_fifo_credit_log_l1_fifo_credit(&data);
  // if (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l1_fifo_credit_ !=
  // 0x30) {
  if (fld != 0x30) {
    ucli_log("[ERROR]::PRE Pipe%0d L1 FIFO CREDIT Expect:0x30 Actual:%0d!\n",
             pipe_num,
             fld);
  }
  fld = getp_tof3_pre_fifo_credit_log_l1l2_fifo_credit(&data);
  // if (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l1l2_fifo_credit_
  // != 0x4) {
  if (fld != 0x4) {
    ucli_log("[ERROR]::PRE Pipe%0d L1L2 FIFO CREDIT Expect:0x4 Actual:%0d!\n",
             pipe_num,
             fld);
  }
  fld = getp_tof3_pre_fifo_credit_log_l2_fifo_credit(&data);
  // if (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l2_fifo_credit_ !=
  // 0x14) {
  if (fld != 0x1f) {
    ucli_log("[ERROR]::PRE Pipe%0d L2 FIFO CREDIT Expect:0x14 Actual:%0d!\n",
             pipe_num,
             fld);
  }
  fld = getp_tof3_pre_fifo_credit_log_l2_sel_fifo_credit(&data);
  // if (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l2_sel_fifo_credit_
  // != 0x8) {
  if (fld != 0x8) {
    ucli_log("[ERROR]::PRE Pipe%0d L2 SEL FIFO CREDIT Expect:0x8 Actual:%0d!\n",
             pipe_num,
             fld);
  }
  fld = getp_tof3_pre_fifo_credit_log_l2_port_fifo_credit(&data);
  // if
  // (TM_REGS->get_pre_pipe_rspec(pipe_num)->credit_log_->l2_port_fifo_credit_
  // != 0x10) {
  if (fld != 0x10) {
    ucli_log(
        "[ERROR]::PRE Pipe%0d L2 PORT FIFO CREDIT Expect:0x10 Actual:%0d!\n",
        pipe_num,
        fld);
  }

  ucli_log("[TM_EOS]:: PRE Pipe%0d Counter Check Done!\n", pipe_num);
}

static void tm_prc_pipe_eos_check(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint8_t pipe_num__) {
  uint32_t cnt_size;
  (void)dev_id;
  (void)subdev_id;
  // Check PRE Count
  for (int pipe_num = 0; pipe_num < NumPipe; pipe_num++) {
    if (!((m_pipe_mask >> pipe_num) & 0x1)) {
      continue;
    }
    ucli_log("[TM_EOS]:: PRC Pipe%0d EOS Check Starting...\n", pipe_num);

    if (!m_sknobs_value_tm_prc_pipe_eos_check) {  // pre fifo low test
#if 0
      cnt_size = TM_REGS->get_prc_pipe_mem_rspec(pipe_num)->csr_memory_prc_map_->entry_.size();
      for(int i=0; i<cnt_size; i++) {
        TM_REGS->get_prc_pipe_mem_rspec(pipe_num)->csr_memory_prc_map_->entry_[i]->Read();
        if(TM_REGS->get_prc_pipe_mem_rspec(pipe_num)->csr_memory_prc_map_->entry_[i]->map_ != 0) {
          ucli_log( "[ERROR]::PRC Pipe%0d prc_map Entry %d Expect:0x0 Actual:0x%0llx!\n", pipe_num, i,
            TM_REGS->get_prc_pipe_mem_rspec(pipe_num)->csr_memory_prc_map_->entry_[i]->map_);
        }
      }
#else
      (void)cnt_size;
#endif  // 0
    }
  }
  ucli_log("[TM_EOS]:: PRC Pipe%0d EOS Check Done!\n", pipe_num__);
}

static void tm_ig_pipe_eos_check(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 uint8_t pipe_num) {
  //   tm_wac_pipe_eos_check(pipe_num); // mem
  tm_clc_pipe_eos_check(dev_id, subdev_id, pipe_num);
  tm_pex_pipe_eos_check(dev_id, subdev_id, pipe_num);
}

// IG EOS
static void tm_ig_eos_check_all(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                uint32_t phys_pipe_msk) {
  tm_wac_common_eos_check(dev_id, subdev_id);
  tm_caa_eos_check(dev_id, subdev_id);
  tm_clc_common_eos_check(dev_id, subdev_id);
  //  tm_pex_common_eos_check();

  for (int i = 0; i < 4; i++) {
    if (!((phys_pipe_msk >> i) & 0x1)) {
      continue;
    }
    tm_ig_pipe_eos_check(dev_id, subdev_id, i);
  }
  tm_wac_buf_full_eos_check(dev_id, subdev_id);
  tm_ig_int_chk(dev_id, subdev_id);
  ucli_log("[TM_EOS]:: IG Check Done!\n");
}

// EOS Check for Egress Modules
static void tm_eg_eos_check(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint8_t pipe_num) {
  tm_qac_pipe_eos_check(dev_id, subdev_id, pipe_num);
  tm_psc_pipe_eos_check(dev_id, subdev_id, pipe_num);
  tm_qlc_pipe_eos_check(dev_id, subdev_id, pipe_num);
  tm_pre_pipe_eos_check(dev_id, subdev_id, pipe_num);
  tm_prc_pipe_eos_check(dev_id, subdev_id, pipe_num);
  // FIX-ME, SCH PORT-PEX Status Mem
  // needs to move to Indirect
  tm_sch_pipe_eos_check(dev_id, subdev_id, pipe_num);
}

// EG EOS
static void tm_eg_eos_check_all(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                uint32_t phys_pipe_msk) {
  tm_qac_common_eos_check(dev_id, subdev_id);
  tm_psc_common_eos_check(dev_id, subdev_id);
  for (int i = 0; i < 4; i++) {
    if (!((phys_pipe_msk >> i) & 0x1)) {
      continue;
    }
    tm_eg_eos_check(dev_id, subdev_id, i);
  }
  tm_eg_int_chk(dev_id, subdev_id, 1);
  ucli_log("[TM_EOS]:: EG Check Done!\n");
}

/********************************************************************
 * lld_tof3_eos_tm_audit
 *
 * Debug cmd handler for TM end-of-sim checks
 ********************************************************************/
void lld_tof3_eos_tm_audit(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           uint32_t phys_pipe_msk) {
  m_pipe_mask = phys_pipe_msk;

  // Add all the EOS checks here
  tm_ig_eos_check_all(dev_id, subdev_id, phys_pipe_msk);
  tm_eg_eos_check_all(dev_id, subdev_id, phys_pipe_msk);
  tm_wac_common_eos_check(dev_id, subdev_id);
  //  tm_wac_drop_cnt_eos_check(); //mem
  for (int i = 0; i < 4; i++) {
    if (!((phys_pipe_msk >> i) & 0x1)) {
      continue;
    }
    //    tm_wac_pipe_eos_check(i); //mem
  }
  tm_wac_buf_full_eos_check(dev_id, subdev_id);
}
