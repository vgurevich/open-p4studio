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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <regex.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include "bf_switchd_log.h"
#include "bf_switchd.h"

extern bf_switchd_context_t *switchd_ctx;
#define ACCESS_LOG_SZ 4096

// hack
extern char *bf_lld_dbg_get_full_reg_path_name(bf_dev_family_t dev_family,
                                               uint32_t offset);
char *switchd_full_reg_path_name(bf_dev_id_t dev_id, uint32_t offset) {
  if (bf_lld_dev_is_tof3(dev_id)) {
    return bf_lld_dbg_get_full_reg_path_name(BF_DEV_FAMILY_TOFINO3, offset);
  } else if (bf_lld_dev_is_tof2(dev_id)) {
    return bf_lld_dbg_get_full_reg_path_name(BF_DEV_FAMILY_TOFINO2, offset);
  } else {
    return bf_lld_dbg_get_full_reg_path_name(BF_DEV_FAMILY_TOFINO, offset);
  }
}

typedef struct bf_switchd_log_access_log_entry_t {
  bf_switchd_log_acc_type acc_typ;
  bf_dev_id_t dev_id;
  uint32_t reg;
  uint32_t data;
  struct timeval tm;
} bf_switchd_log_access_log_entry_t;

typedef struct bf_switchd_log_access_log_t {
  int next;
  bf_switchd_log_access_log_entry_t entry[ACCESS_LOG_SZ];
} bf_switchd_log_access_log_t;

bf_switchd_log_access_log_t bf_switchd_log_access_log;

bf_sys_mutex_t bf_switchd_log_mtx;  // lock for access log
int bf_switchd_log_mtx_initd = 0;

// regular expresssion (for "pcie_logx")
regex_t l_regex;

/******************************************************************
 * bf_switchd_log_mtx_init
 *
 * Initialize the mutex protecting the access log from simultaneous
 * update from multiple threads.
 ******************************************************************/
static void bf_switchd_log_mtx_init(void) {
  if (!bf_switchd_log_mtx_initd) {
    int x;

    bf_switchd_log_mtx_initd = 1;

    x = bf_sys_mutex_init(&bf_switchd_log_mtx);
    if (x) {
      printf("Error: bf_switchd log lock init failed: <%d>\n", x);
      assert(0);
    } else {
      bf_switchd_log_mtx_initd = 1;
    }
  }
}

/******************************************************************
 * bf_switchd_log_init
 *
 * Initialize the access log. This may be called at initialization
 * time or at run-time if the user wants to "clear" the log. If
 * "clearing, skip the mutex init.
 ******************************************************************/
void bf_switchd_log_init(bool init_mtx) {
  if (init_mtx) {
    bf_switchd_log_mtx_init();
  }
  memset(
      (char *)&bf_switchd_log_access_log, 0, sizeof(bf_switchd_log_access_log));
}

/******************************************************************
 * List of addresses to filter form the log as they are accessed
 * too frequently and don't provide much information that can't
 * be gleaned from other sources.
 ******************************************************************/
uint32_t bf_switchd_tofino1_log_filter[] = {
    offsetof(Tofino, device_select.misc_regs.sbm_ind_rslt),
    offsetof(Tofino, device_select.mbc.mbc_mac_tx_dr.head_ptr),
    offsetof(Tofino, device_select.mbc.mbc_mac_tx_dr.head_ptr),
    offsetof(Tofino, device_select.mbc.mbc_mac_cpl_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_tx_dr[0].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_cpl_dr[0].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_tx_dr[1].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_cpl_dr[1].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_tx_dr[2].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_cpl_dr[2].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_tx_dr[3].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_il_cpl_dr[3].head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_wb_tx_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_wb_cpl_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_rb_tx_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_rb_cpl_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_stat_fm_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_stat_rx_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_idle_fm_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_idle_rx_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_diag_fm_dr.head_ptr),
    offsetof(Tofino, device_select.pbc.pbc_diag_rx_dr.head_ptr),
    offsetof(Tofino, device_select.cbc.cbc_wl_tx_dr.head_ptr),
    offsetof(Tofino, device_select.cbc.cbc_wl_cpl_dr.head_ptr),
    offsetof(Tofino, device_select.cbc.cbc_lq_fm_dr.head_ptr),
    offsetof(Tofino, device_select.cbc.cbc_lq_rx_dr.head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_tx_dr[0].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_cpl_dr[0].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_tx_dr[1].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_cpl_dr[1].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_tx_dr[2].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_cpl_dr[2].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_tx_dr[3].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_cpl_dr[3].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[0].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[0].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[1].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[1].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[2].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[2].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[3].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[3].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[4].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[4].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[5].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[5].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[6].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[6].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_fm_dr[7].head_ptr),
    offsetof(Tofino, device_select.tbc.tbc_rx_dr[7].head_ptr),
    0x00200858,
    0x002804d8,
};
uint32_t bf_switchd_tofino2_log_filter[] = {
    offsetof(tof2_reg, device_select.mbc.mbc_mac_0_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.mbc.mbc_mac_0_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.mbc.mbc_wb_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.mbc.mbc_wb_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_tx_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_cpl_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_tx_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_cpl_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_tx_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_cpl_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_tx_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_il_cpl_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_wb_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_wb_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_rb_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_rb_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_stat_fm_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_stat_rx_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_idle_fm_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_idle_rx_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_diag_fm_dr.head_ptr),
    offsetof(tof2_reg, device_select.pbc.pbc_diag_rx_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_wl0_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_wl0_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_wl1_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_wl1_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_rb0_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_rb0_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_rb1_tx_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_rb1_cpl_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_lq_fm_dr.head_ptr),
    offsetof(tof2_reg, device_select.cbc.cbc_lq_rx_dr.head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_tx_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_cpl_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_tx_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_cpl_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_tx_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_cpl_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_tx_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_cpl_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[0].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[1].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[2].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[3].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[4].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[4].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[5].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[5].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[6].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[6].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_fm_dr[7].head_ptr),
    offsetof(tof2_reg, device_select.tbc.tbc_rx_dr[7].head_ptr),
    offsetof(tof2_reg, device_select.lfltr[0].ctrl.bft_ctrl),
    offsetof(tof2_reg, device_select.lfltr[1].ctrl.bft_ctrl),
    offsetof(tof2_reg, device_select.lfltr[2].ctrl.bft_ctrl),
    offsetof(tof2_reg, device_select.lfltr[3].ctrl.bft_ctrl),
};

uint32_t bf_switchd_tofino3_log_filter[] = {
    offsetof(tof3_reg, device_select.mbc.mbc_mac_0_tx_dr.head_ptr),
    offsetof(tof3_reg, device_select.mbc.mbc_mac_0_cpl_dr.head_ptr),
    offsetof(tof3_reg, device_select.mbc.mbc_wb_tx_dr.head_ptr),
    offsetof(tof3_reg, device_select.mbc.mbc_wb_cpl_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_tx_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_cpl_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_tx_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_cpl_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_tx_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_cpl_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_tx_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_il_cpl_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_wb_tx_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_wb_cpl_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_rb_tx_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_rb_cpl_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_stat_fm_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_stat_rx_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_idle_fm_dr.head_ptr),
    offsetof(tof3_reg, device_select.pbc.pbc_idle_rx_dr.head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_tx_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_cpl_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_tx_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_cpl_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_tx_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_cpl_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_tx_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_cpl_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[0].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[1].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[2].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[3].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[4].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[4].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[5].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[5].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[6].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[6].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_fm_dr[7].head_ptr),
    offsetof(tof3_reg, device_select.tbc.tbc_rx_dr[7].head_ptr),
    offsetof(tof3_reg, pipes[0].pardereg.lfltr_reg.ctrl.bft_ctrl),
    offsetof(tof3_reg, pipes[1].pardereg.lfltr_reg.ctrl.bft_ctrl),
    offsetof(tof3_reg, pipes[2].pardereg.lfltr_reg.ctrl.bft_ctrl),
    offsetof(tof3_reg, pipes[3].pardereg.lfltr_reg.ctrl.bft_ctrl),

    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_addr_low),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_addr_high),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data00),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data00),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data01),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data01),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data10),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data10),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data11),
    offsetof(tof3_reg, device_select.pcie_bar01_regs.cpu_ind_data11),
};

/******************************************************************
 * bf_switchd_log_is_filtered_reg
 *
 * Filter some common accesses from the log.
 ******************************************************************/
static bool bf_switchd_log_is_filtered_reg(bf_dev_family_t family,
                                           uint32_t offset) {
  if (family == BF_DEV_FAMILY_TOFINO) {
    uint32_t num_elem = sizeof bf_switchd_tofino1_log_filter /
                        sizeof bf_switchd_tofino1_log_filter[0];
    for (uint32_t i = 0; i < num_elem; i++) {
      if (offset == bf_switchd_tofino1_log_filter[i]) {  // head ptr
        return true;
      }
      if (offset == (bf_switchd_tofino1_log_filter[i] + 4)) {  // tail ptr
        return true;
      }
    }
  } else if (family == BF_DEV_FAMILY_TOFINO2) {
    uint32_t num_elem = sizeof bf_switchd_tofino2_log_filter /
                        sizeof bf_switchd_tofino2_log_filter[0];
    for (uint32_t i = 0; i < num_elem; i++) {
      if (offset == bf_switchd_tofino2_log_filter[i]) {  // head ptr
        return true;
      }
      if (offset == (bf_switchd_tofino2_log_filter[i] + 4)) {  // tail ptr
        return true;
      }
    }
  } else if (family == BF_DEV_FAMILY_TOFINO3) {
    uint32_t num_elem = sizeof bf_switchd_tofino3_log_filter /
                        sizeof bf_switchd_tofino3_log_filter[0];
    for (uint32_t i = 0; i < num_elem; i++) {
      if (offset == bf_switchd_tofino3_log_filter[i]) {  // head ptr
        return true;
      }
      if (offset == (bf_switchd_tofino3_log_filter[i] + 4)) {  // tail ptr
        return true;
      }
    }
  }
  return false;
}

/******************************************************************
 * bf_switchd_log_access
 *
 * Log a PCIe register access. Called from bf_switchd.c in the
 * callback functions providing the actual register access.
 ******************************************************************/
int reg_print_to_screen = 0;

void bf_switchd_log_access(bf_switchd_log_acc_type acc_typ,
                           bf_dev_id_t dev_id,
                           bf_dev_family_t family,
                           uint32_t reg,
                           uint32_t data) {
  int e;
  if (reg_print_to_screen) {
    fprintf(stderr,
            "%s 0x%08x data 0x%08x\n",
            BF_SWITCHD_LOG_READ_BEFORE == acc_typ
                ? "Rd"
                : BF_SWITCHD_LOG_WRITE == acc_typ ? "Wr" : "Rd+",
            reg,
            data);
    fflush(stderr);
    bf_sys_usleep(1000);
  }

  if (!bf_switchd_log_mtx_initd) {
    bf_switchd_log_init(true);
  }
  if (bf_switchd_log_is_filtered_reg(family, reg)) return;

  // lock log while updating "next"
  bf_sys_mutex_lock(&bf_switchd_log_mtx);
  e = bf_switchd_log_access_log.next;
  bf_switchd_log_access_log.next =
      (bf_switchd_log_access_log.next == (ACCESS_LOG_SZ - 1))
          ? 0
          : (bf_switchd_log_access_log.next + 1);
  bf_sys_mutex_unlock(&bf_switchd_log_mtx);

  gettimeofday(&bf_switchd_log_access_log.entry[e].tm, NULL);

  bf_switchd_log_access_log.entry[e].acc_typ = acc_typ;
  bf_switchd_log_access_log.entry[e].dev_id = dev_id;
  bf_switchd_log_access_log.entry[e].reg = reg;
  bf_switchd_log_access_log.entry[e].data = data;
}

static void bf_switchd_log_dump_timeval(struct timeval *tm) {
  char tbuf[256] = {0};
  char ubuf[256] = {0};
  struct tm *loctime;

  loctime = localtime(&tm->tv_sec);

  strftime(tbuf, sizeof(tbuf), "%a %b %d", loctime);
  printf("%s ", tbuf);

  strftime(ubuf, sizeof(ubuf), "%T\n", loctime);
  ubuf[strlen(ubuf) - 1] = 0;  // remove CR
  printf("%s.%06d", ubuf, (int)tm->tv_usec);
}

void bf_switchd_log_dump_access_log(void) {
  int e, n;

  printf("Pcie Register Access log:\n\n");

  printf(
      "                           |Type|Chp| Register | Contents | Register "
      "name\n");
  printf(
      "+--------------------------+----+---+----------+----------+-------------"
      "-\n");

  e = bf_switchd_log_access_log.next;
  for (n = 0; n < ACCESS_LOG_SZ; n++) {
    bf_switchd_log_access_log_entry_t *p_ent =
        &bf_switchd_log_access_log.entry[e];
    // skip emtpy entries
    if (p_ent->tm.tv_sec == 0) {
      e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
      continue;
    }
    bf_switchd_log_dump_timeval(&p_ent->tm);
    printf(" | ");
    if (p_ent->acc_typ == BF_SWITCHD_LOG_READ_BEFORE) {  // no data yet
      printf("%s | %d | %08x | -------- | %s\n",
             "Rd",
             p_ent->dev_id,
             p_ent->reg,
             switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    } else {
      printf("%s | %d | %08x | %08x | %s\n",
             (p_ent->acc_typ == BF_SWITCHD_LOG_WRITE) ? "Wr" : "Rd",
             p_ent->dev_id,
             p_ent->reg,
             p_ent->data,
             switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    }
    e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
  }
}

void bf_switchd_log_dump_access_log_last_n(int num_logs) {
  int e, n;

  printf("Pcie Register Access log:\n\n");

  printf(
      "                           |Type|Chp| Register | Contents | Register "
      "name\n");
  printf(
      "+--------------------------+----+---+----------+----------+-------------"
      "-\n");

  // count backwards min(num_logs, num_filled_entries)
  e = bf_switchd_log_access_log.next;
  for (n = 0; n < num_logs; n++) {
    e = (e == 0) ? (ACCESS_LOG_SZ - 1) : (e - 1);
    if (bf_switchd_log_access_log.entry[e].tm.tv_sec == 0) break;
  }
  // if backed up to an empty entry, move forward one entry
  if (bf_switchd_log_access_log.entry[e].tm.tv_sec == 0) {
    e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
  }

  for (n = 0; n < num_logs; n++) {
    bf_switchd_log_access_log_entry_t *p_ent =
        &bf_switchd_log_access_log.entry[e];
    // skip emtpy entries
    if (p_ent->tm.tv_sec == 0) {
      e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
      continue;
    }
    bf_switchd_log_dump_timeval(&p_ent->tm);
    printf(" | ");
    if (p_ent->acc_typ == BF_SWITCHD_LOG_READ_BEFORE) {  // no data yet
      printf("%s | %d | %08x | -------- | %s\n",
             "Rd",
             p_ent->dev_id,
             p_ent->reg,
             switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    } else {
      printf("%s | %d | %08x | %08x | %s\n",
             (p_ent->acc_typ == BF_SWITCHD_LOG_WRITE) ? "Wr" : "Rd",
             p_ent->dev_id,
             p_ent->reg,
             p_ent->data,
             switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    }
    e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
  }
}

void bf_switchd_log_dump_access_log_w_filter(char *pattern) {
  int e, n, rc;
  char str_to_match[1024];

  rc = regcomp(&l_regex, pattern, 0);
  if (rc != 0) {
    printf("Invalid regular expression: '%s'\n", pattern);
    return;
  }

  printf("Pcie Register Access logi (filtered): Pattern=\"%s\"\n\n", pattern);

  printf(
      "                           |Type|Chp| Register | Contents | Register "
      "name\n");
  printf(
      "+--------------------------+----+---+----------+----------+-------------"
      "-\n");

  e = bf_switchd_log_access_log.next;
  for (n = 0; n < ACCESS_LOG_SZ; n++) {
    bf_switchd_log_access_log_entry_t *p_ent =
        &bf_switchd_log_access_log.entry[e];
    // skip emtpy entries
    if (p_ent->tm.tv_sec == 0) {
      e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
      continue;
    }
    str_to_match[0] = 0;

    // bf_switchd_log_dump_timeval( &p_ent->tm );
    {
      char tbuf[256] = {0};
      char ubuf[256] = {0};
      struct tm *loctime;

      loctime = localtime(&p_ent->tm.tv_sec);

      strftime(tbuf, sizeof(tbuf), "%a %b %d", loctime);
      snprintf(str_to_match + strlen(str_to_match),
               sizeof(str_to_match) - strlen(str_to_match) - 1,
               "%s ",
               tbuf);

      strftime(ubuf, sizeof(ubuf), "%T\n", loctime);
      ubuf[strlen(ubuf) - 1] = 0;  // remove CR
      snprintf(str_to_match + strlen(str_to_match),
               sizeof(str_to_match) - strlen(str_to_match) - 1,
               "%s.%06d",
               ubuf,
               (int)p_ent->tm.tv_usec);
    }
    snprintf(str_to_match + strlen(str_to_match),
             sizeof(str_to_match) - strlen(str_to_match) - 1,
             " | ");
    if (p_ent->acc_typ == BF_SWITCHD_LOG_READ_BEFORE) {
      snprintf(str_to_match + strlen(str_to_match),
               sizeof(str_to_match) - strlen(str_to_match) - 1,
               "%s | %d | %08x | -------- | %s\n",
               "Rd",
               p_ent->dev_id,
               p_ent->reg,
               switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    } else {
      snprintf(str_to_match + strlen(str_to_match),
               sizeof(str_to_match) - strlen(str_to_match) - 1,
               "%s | %d | %08x | %08x | %s\n",
               (p_ent->acc_typ == BF_SWITCHD_LOG_WRITE) ? "Wr" : "Rd",
               p_ent->dev_id,
               p_ent->reg,
               p_ent->data,
               switchd_full_reg_path_name(p_ent->dev_id, p_ent->reg));
    }
    rc = regexec(&l_regex, str_to_match, 0, NULL, 0);
    if (rc == 0) {
      printf("%s", str_to_match);
    }

    e = (e == (ACCESS_LOG_SZ - 1)) ? 0 : (e + 1);
  }
}
