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
/*
 * This code is generated using  packet_path_counters.py ; Any edits to this
 * file will be lost.
 */

#define __STDC_FORMAT_MACROS
#include <stddef.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <tofino_regs/tofino.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include "pipe_mgr_tof_pkt_path_counter_display.h"

#define HLINE                                                                  \
  "__________________________________________________________________________" \
  "_________________________"

#if DVM_CONFIG_INCLUDE_UCLI == 1
void pipe_mgr_tof_pkt_path_display_ibuf_counter(
    ucli_context_t *uc, int hex, bf_dev_id_t devid, int p, int pg_port) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  aim_printf(
      &uc->pvs, "%-49s", "ibuf-counter                                 ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts From MAC on Channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_macs_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts From MAC on Channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_macs_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts From MAC on Channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_macs_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts From MAC on Channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_macs_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel0 sent to Parser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_parser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel1 sent to Parser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_parser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel2 sent to Parser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_parser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel3 sent to Parser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_parser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel0 sent to DeParser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_deparser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel1 sent to DeParser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_deparser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel2 sent to DeParser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_deparser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Pkts from channel3 sent to DeParser");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_deparser_send_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Recirculation Pkts on channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_recirc_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Recirculation Pkts on channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_recirc_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Recirculation Pkts on channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_recirc_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Recirculation Pkts on channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_recirc_received_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_drop_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Total Pkts Dropped on channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_deparser_drop_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_drop_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Total Pkts Dropped on channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_deparser_drop_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_drop_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Total Pkts Dropped on channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_deparser_drop_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_drop_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Total Pkts Dropped on channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_deparser_drop_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_wsch_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Pkts in ibuf on channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_wsch_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_wsch_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Pkts in ibuf on channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_wsch_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_wsch_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Pkts in ibuf on channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_wsch_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_wsch_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Pkts in ibuf on channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_wsch_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_wsch_trunc_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Truncated Pkts on channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_wsch_trunc_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_wsch_trunc_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Truncated Pkts on channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_wsch_trunc_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_wsch_trunc_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Truncated Pkts on channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_wsch_trunc_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_wsch_trunc_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Truncated Pkts on channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_wsch_trunc_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Recirculated Pkts on ch0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_recirc_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Recirculated Pkts on ch1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_recirc_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Recirculated Pkts on ch2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_recirc_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "IBUF: Discarded Recirculated Pkts on ch3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_recirc_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs,
               "%-45s",
               "IBUF: Discarded Pkts in PRSR due to ibuf ch0 full or prsr req "
               "to drop ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan0_group.chnl_parser_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs,
               "%-45s",
               "IBUF: Discarded Pkts in PRSR due to ibuf ch1 full or prsr req "
               "to drop ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan1_group.chnl_parser_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs,
               "%-45s",
               "IBUF: Discarded Pkts in PRSR due to ibuf ch2 full or prsr req "
               "to drop ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan2_group.chnl_parser_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_discard_pkt),
        &val);
    (void)cnt;
    aim_printf(&uc->pvs,
               "%-45s",
               "IBUF: Discarded Pkts in PRSR due to ibuf ch3 full or prsr req "
               "to drop ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(
        &uc->pvs, "%-30s", "ing_buf_regs.chan3_group.chnl_parser_discard_pkt");
    aim_printf(&uc->pvs, "\n");
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

static void _pipe_mgr_tof_pkt_path_display_iprsr_counter(ucli_context_t *uc,
                                                         int hex,
                                                         bf_dev_id_t devid,
                                                         uint32_t offset1,
                                                         uint32_t offset2,
                                                         const char *cnt_name,
                                                         const char *reg_name) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  lld_read_register(devid, offset1, &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(devid, offset2, &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", cnt_name);
  aim_printf(&uc->pvs, (!hex) ? "%-30" PRIu64 : "%-30" PRIx64, cnt);
  aim_printf(&uc->pvs, "%-30s\n", reg_name);
}

#define prsr_reg_alias pipes[p].pmarb.ibp18_reg.ibp_reg[pg_port / 4].prsr_reg

void pipe_mgr_tof_pkt_path_display_iprsr_counter(
    ucli_context_t *uc, int hex, bf_dev_id_t devid, int p, int pg_port) {
  uint32_t val;
  uint32_t offset1 = 0;
  uint32_t offset2 = 0;
  char log_msg[64] = {0};
  const int channel_num = pg_port % 4;

  aim_printf(
      &uc->pvs, "%-49s", "iprsr-counter                                ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid, offsetof(Tofino, prsr_reg_alias.pkt_drop_cnt[channel_num]), &val);
  snprintf(log_msg,
           sizeof(log_msg),
           "IPRSR: Total Pkts Dropped on Channel%d",
           channel_num);
  aim_printf(&uc->pvs, "%-45s", log_msg);
  aim_printf(&uc->pvs, (!hex) ? "%-30" PRIu32 : "%-30" PRIx32, val);
  memset(log_msg, 0, sizeof(log_msg));
  snprintf(log_msg, sizeof(log_msg), "prsr_reg.pkt_drop_cnt[%d]", channel_num);
  aim_printf(&uc->pvs, "%-30s\n", log_msg);

  offset1 =
      offsetof(Tofino, prsr_reg_alias.op_fifo_full_cnt.op_fifo_full_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.op_fifo_full_cnt.op_fifo_full_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: output fifo full counter",
      "prsr_reg.op_fifo_full_cnt");

  offset1 = offsetof(
      Tofino, prsr_reg_alias.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2);
  offset2 = offsetof(
      Tofino, prsr_reg_alias.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: output fifo stall counter",
      "prsr_reg.op_fifo_full_stall_cnt");

  offset1 = offsetof(
      Tofino, prsr_reg_alias.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2);
  offset2 = offsetof(
      Tofino, prsr_reg_alias.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: TCAM match Error",
      "prsr_reg.no_tcam_match_err_cnt");

  offset1 = offsetof(
      Tofino, prsr_reg_alias.partial_hdr_err_cnt.partial_hdr_err_cnt_0_2);
  offset2 = offsetof(
      Tofino, prsr_reg_alias.partial_hdr_err_cnt.partial_hdr_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: Partial Header Error",
                                               "prsr_reg.partial_hdr_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.ctr_range_err_cnt.ctr_range_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.ctr_range_err_cnt.ctr_range_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: Counter Range Error",
                                               "prsr_reg.ctr_range_err_cnt");

  offset1 = offsetof(
      Tofino, prsr_reg_alias.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2);
  offset2 = offsetof(
      Tofino, prsr_reg_alias.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: Timeout or Excess state iteration Error",
      "prsr_reg.timeout_iter_err_cnt");

  offset1 = offsetof(
      Tofino, prsr_reg_alias.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2);
  offset2 = offsetof(
      Tofino, prsr_reg_alias.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: Timeout or Excess clock cycle",
      "prsr_reg.timeout_cycle_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.src_ext_err_cnt.src_ext_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.src_ext_err_cnt.src_ext_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: Extraction source error counter",
      "prsr_reg.src_ext_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.dst_cont_err_cnt.dst_cont_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.dst_cont_err_cnt.dst_cont_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: Container size error counter",
      "prsr_reg.dst_cont_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.phv_owner_err_cnt.phv_owner_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.phv_owner_err_cnt.phv_owner_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: PHV owner error counter",
                                               "prsr_reg.phv_owner_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.multi_wr_err_cnt.multi_wr_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.multi_wr_err_cnt.multi_wr_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(
      uc,
      hex,
      devid,
      offset1,
      offset2,
      "IPRSR: PHV multiple write error",
      "prsr_reg.multi_wr_err_cnt");

  offset1 = offsetof(Tofino, prsr_reg_alias.fcs_err_cnt.fcs_err_cnt_0_2);
  offset2 = offsetof(Tofino, prsr_reg_alias.fcs_err_cnt.fcs_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: FCS error",
                                               "prsr_reg.fcs_err_cnt");

  offset1 = offsetof(Tofino, prsr_reg_alias.csum_err_cnt.csum_err_cnt_0_2);
  offset2 = offsetof(Tofino, prsr_reg_alias.csum_err_cnt.csum_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: Checksum error",
                                               "prsr_reg.csum_err_cnt");

  offset1 =
      offsetof(Tofino, prsr_reg_alias.tcam_par_err_cnt.tcam_par_err_cnt_0_2);
  offset2 =
      offsetof(Tofino, prsr_reg_alias.tcam_par_err_cnt.tcam_par_err_cnt_1_2);
  _pipe_mgr_tof_pkt_path_display_iprsr_counter(uc,
                                               hex,
                                               devid,
                                               offset1,
                                               offset2,
                                               "IPRSR: TCAM parity error",
                                               "prsr_reg.tcam_par_err_cnt");

  aim_printf(&uc->pvs, "\n\n\n");
}

void pipe_mgr_tof_pkt_path_display_idprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bf_dev_id_t devid,
                                                  int p) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;
  aim_printf(
      &uc->pvs, "%-49s", "idprsr-counter                               ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.iir.hdr_too_long_i), &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: header too long");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "hdr_too_long_i");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Valid PHV into Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_phv");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Valid TPHV into Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_tphv");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets into Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_read");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets discarded ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_discard");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets resubmitted ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_resubmit");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Learn Counter due to packets ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_learn");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets processed in output phase ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_pkts");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[0]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[0]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[1]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[1]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[2]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[2]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[3]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[3]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[4]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[4]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[5]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[5]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[6]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[6]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[7]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[7]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[8]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[8]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[9]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[9]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[10]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[10]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[11]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[11]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[12]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[12]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[13]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[13]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[14]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[14]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[15]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[15]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[16]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[16]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[17]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[17]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[18]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[18]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[19]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[19]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[20]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[20]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[21]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[21]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[22]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[22]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[23]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[23]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[24]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[24]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[25]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[25]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[26]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[26]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[27]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[27]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[28]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[28]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[29]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[29]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[30]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[30]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[31]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[31]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[32]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[32]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[33]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[33]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[34]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[34]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[35]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[35]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[36]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[36]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[37]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[37]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[38]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[38]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[39]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[39]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[40]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[40]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[41]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[41]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[42]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[42]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[43]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[43]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[44]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[44]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[45]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[45]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[46]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[46]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[47]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[47]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[48]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[48]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[49]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[49]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[50]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[50]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[51]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[51]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[52]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[52]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[53]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[53]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[54]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[54]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[55]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[55]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[56]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[56]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[57]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[57]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[58]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[58]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[59]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[59]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[60]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[60]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[61]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[61]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[62]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[62]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[63]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[63]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[64]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[64]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[65]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[65]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[66]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[66]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[67]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[67]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[68]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[68]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[69]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[69]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[70]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[70]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[71]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: CRC error on packets from Ibuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[71]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_egr_pkt_err),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Errored packets to TM ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "i_egr_pkt_err");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_ctm_pkt_err),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Errored packets from Ibuf to iCTM");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "i_ctm_pkt_err");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets sent to TM");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "i_fwd_pkts");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "IDPRSR: Packets discarded at TM interface ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "i_disc_pkts");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(
      &uc->pvs, "%-45s", "IDPRSR: Packets sent to Ingress mirror buffer");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "i_mirr_pkts");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_ebuf_counter(
    ucli_context_t *uc, int hex, bf_dev_id_t devid, int p, int pg_port) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  aim_printf(
      &uc->pvs, "%-49s", "ebuf-counter                                 ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[0]
                                   .egr_bypass_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[0]
                                   .egr_bypass_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(
        &uc->pvs, "%-45s", "EPB: Total Pkts bypassing Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_bypass_count[0]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[1]
                                   .egr_bypass_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[1]
                                   .egr_bypass_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(
        &uc->pvs, "%-45s", "EPB: Total Pkts bypassing Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_bypass_count[1]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[2]
                                   .egr_bypass_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[2]
                                   .egr_bypass_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(
        &uc->pvs, "%-45s", "EPB: Total Pkts bypassing Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_bypass_count[2]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[3]
                                   .egr_bypass_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_bypass_count[3]
                                   .egr_bypass_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(
        &uc->pvs, "%-45s", "EPB: Total Pkts bypassing Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_bypass_count[3]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 0) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[0]
                                   .egr_pipe_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[0]
                                   .egr_pipe_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "EPB: Total Pkts sent to Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_pipe_count[0]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[1]
                                   .egr_pipe_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[1]
                                   .egr_pipe_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "EPB: Total Pkts sent to Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_pipe_count[1]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[2]
                                   .egr_pipe_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[2]
                                   .egr_pipe_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "EPB: Total Pkts sent to Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_pipe_count[2]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[3]
                                   .egr_pipe_count_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                   .epb_disp_port_regs.egr_pipe_count[3]
                                   .egr_pipe_count_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    aim_printf(&uc->pvs, "%-45s", "EPB: Total Pkts sent to Epipe on Channel ");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
    } else {
      aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
    }
    aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.egr_pipe_count[3]");
    aim_printf(&uc->pvs, "\n");
  }
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_eprsr_counter(
    ucli_context_t *uc, int hex, bf_dev_id_t devid, int p, int pg_port) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  aim_printf(
      &uc->pvs, "%-49s", "eprsr-counter                                ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: output fifo full counter");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.op_fifo_full_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: output fifo stall counter");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.op_fifo_full_stall_cnt");
  aim_printf(&uc->pvs, "\n");

  if ((pg_port % 4) == 0) {
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.pkt_drop_cnt[0]),
                      &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "EPRSR: Total Pkts Dropped on channel0");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(&uc->pvs, "%-30s", "prsr_reg.pkt_drop_cnt[0]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 1) {
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.pkt_drop_cnt[1]),
                      &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "EPRSR: Total Pkts Dropped on channel1");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(&uc->pvs, "%-30s", "prsr_reg.pkt_drop_cnt[1]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 2) {
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.pkt_drop_cnt[2]),
                      &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "EPRSR: Total Pkts Dropped on channel2");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(&uc->pvs, "%-30s", "prsr_reg.pkt_drop_cnt[2]");
    aim_printf(&uc->pvs, "\n");
  }
  if ((pg_port % 4) == 3) {
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.pkt_drop_cnt[3]),
                      &val);
    (void)cnt;
    aim_printf(&uc->pvs, "%-45s", "EPRSR: Total Pkts Dropped on channel3");
    if (!hex) {
      aim_printf(&uc->pvs, "%-30u", val);
    } else {
      aim_printf(&uc->pvs, "%-30x", val);
    }
    aim_printf(&uc->pvs, "%-30s", "prsr_reg.pkt_drop_cnt[3]");
    aim_printf(&uc->pvs, "\n");
  }
  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: TCAM match Error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.no_tcam_match_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Partial Header Error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.partial_hdr_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Counter Range Error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.ctr_range_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(
      &uc->pvs, "%-45s", "EPRSR: Timeout or Excess state iteration Error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.timeout_iter_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Timeout or Excess clock cycle");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.timeout_cycle_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_0_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_1_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Extraction source error counter");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.src_ext_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Container size error counter");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.dst_cont_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: PHV owner error counter");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.phv_owner_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: PHV multiple write error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.multi_wr_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.fcs_err_cnt.fcs_err_cnt_0_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.fcs_err_cnt.fcs_err_cnt_1_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: FCS error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.fcs_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.csum_err_cnt.csum_err_cnt_0_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .prsr_reg.csum_err_cnt.csum_err_cnt_1_2),
                    &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Checksum error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.csum_err_cnt");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EPRSR: TCAM parity error");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.tcam_par_err_cnt");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_edprsr_counter(ucli_context_t *uc,
                                                  int hex,
                                                  bf_dev_id_t devid,
                                                  int p) {
  uint32_t val;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;
  aim_printf(
      &uc->pvs, "%-49s", "edprsr-counter                               ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.ier.hdr_too_long_e), &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: header too long");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "hdr_too_long_e");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Valid PHV into Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_phv");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Valid TPHV into Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_i_tphv");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Packets processed in output phase ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "cnt_pkts");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[0]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[0]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[1]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[1]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[2]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[2]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[3]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[3]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[4]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[4]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[5]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[5]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[6]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[6]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[7]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[7]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[8]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[8]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[9]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[9]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[10]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[10]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[11]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[11]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[12]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[12]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[13]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[13]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[14]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[14]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[15]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[15]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[16]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[16]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[17]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[17]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[18]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[18]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[19]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[19]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[20]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[20]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[21]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[21]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[22]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[22]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[23]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[23]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[24]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[24]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[25]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[25]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[26]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[26]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[27]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[27]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[28]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[28]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[29]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[29]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[30]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[30]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[31]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[31]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[32]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[32]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[33]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[33]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[34]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[34]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[35]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[35]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[36]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[36]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[37]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[37]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[38]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[38]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[39]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[39]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[40]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[40]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[41]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[41]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[42]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[42]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[43]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[43]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[44]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[44]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[45]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[45]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[46]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[46]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[47]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[47]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[48]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[48]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[49]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[49]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[50]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[50]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[51]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[51]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[52]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[52]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[53]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[53]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[54]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[54]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[55]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[55]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[56]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[56]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[57]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[57]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[58]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[58]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[59]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[59]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[60]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[60]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[61]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[61]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[62]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[62]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[63]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[63]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[64]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[64]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[65]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[65]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[66]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[66]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[67]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[67]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[68]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[68]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[69]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[69]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[70]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[70]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[71]),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: CRC error on packets from ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ctm_crc_err[71]");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_egr_pkt_err),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Errored packets from deprsr to Ebuf ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "e_egr_pkt_err");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_ctm_pkt_err),
      &val);
  (void)cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Errored packets from Ebuf to eCTM ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "e_ctm_pkt_err");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Packets sent out of Egress Deparser");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "e_fwd_pkts");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Packets discarded ");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "e_disc_pkts");
  aim_printf(&uc->pvs, "\n");

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  aim_printf(&uc->pvs, "%-45s", "EDPRSR: Packets sent to Egress mirror buffer");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30" PRIu64, cnt);
  } else {
    aim_printf(&uc->pvs, "%-30" PRIx64, cnt);
  }
  aim_printf(&uc->pvs, "%-30s", "e_mirr_pkts");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_pipe_counter(ucli_context_t *uc,
                                                int hex,
                                                bf_dev_id_t devid,
                                                int p) {
  pipe_mgr_tof_pkt_path_display_idprsr_counter(uc, hex, devid, p);

  pipe_mgr_tof_pkt_path_display_edprsr_counter(uc, hex, devid, p);

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_interrupts_intr(
    ucli_context_t *uc, int hex, bf_dev_id_t devid, int p, int pg_port) {
  uint32_t val;

  aim_printf(
      &uc->pvs, "%-49s", "interrupts-intr                              ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                 .ing_buf_regs.glb_group.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "IBUF:  Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ing_buf_regs.glb_group.int_stat");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].pmarb.ibp18_reg.ibp_reg[pg_port / 4].prsr_reg.intr.status),
      &val);
  aim_printf(&uc->pvs, "%-45s", "IPRSR: Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.intr.status");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                 .epb_prsr_port_regs.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "epb_prsr_port_regs.int_stat");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].pmarb.ebp18_reg.ebp_reg[pg_port / 4].prsr_reg.intr.status),
      &val);
  aim_printf(&uc->pvs, "%-45s", "EPRSR: Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "prsr_reg.intr.status");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                 .ebuf_disp_regs.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "EBUF:  Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ebuf_disp_regs.int_stat");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                 .ebuf_fifo_regs.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "EBUF:  Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "ebuf_fifo_regs.int_stat");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                 .epb_disp_port_regs.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "EPB:   Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "epb_disp_port_regs.int_stat");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(devid,
                    offsetof(Tofino,
                             pipes[p]
                                 .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                 .epb_dprs_regs.int_stat),
                    &val);
  aim_printf(&uc->pvs, "%-45s", "EPB:   Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "epb_dprs_regs.int_stat");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

void pipe_mgr_tof_pkt_path_display_dprsr_interrupt_intr(ucli_context_t *uc,
                                                        int hex,
                                                        bf_dev_id_t devid,
                                                        int p) {
  uint32_t val;
  aim_printf(
      &uc->pvs, "%-49s", "dprsr_interrupt-intr                         ");
  aim_printf(&uc->pvs, "%-30s", "Count");
  aim_printf(&uc->pvs, "%-20s", "Register Name");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "%.*s", 81, HLINE);
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.icr.intr.status), &val);
  aim_printf(&uc->pvs, "%-45s", "IDPRSR:Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "inp.icr.intr.status");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.hdr.hir.ingr.intr.status),
      &val);
  aim_printf(&uc->pvs, "%-45s", "IDPRSR:Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "hdr.hir.ingr.intr.status");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.intr.status),
      &val);
  aim_printf(&uc->pvs, "%-45s", "IDPRSR:Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "out_ingr.regs.intr.status");
  aim_printf(&uc->pvs, "\n");

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.intr.status),
      &val);
  aim_printf(&uc->pvs, "%-45s", "EDPRSR:Interrupt Status");
  if (!hex) {
    aim_printf(&uc->pvs, "%-30u", val);
  } else {
    aim_printf(&uc->pvs, "%-30x", val);
  }
  aim_printf(&uc->pvs, "%-30s", "out_egr.regs.intr.status");
  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");

  aim_printf(&uc->pvs, "\n");
}

#endif
void pipe_mgr_tof_pkt_path_clear_ibuf_counter(bf_dev_id_t devid,
                                              int p,
                                              int pg_port) {
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                     .chnl_macs_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                     .chnl_parser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                     .chnl_deparser_send_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_1_2),
        0);
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                     .chnl_recirc_received_pkt_0_2),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_deparser_drop_pkt),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_deparser_drop_pkt),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_deparser_drop_pkt),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_deparser_drop_pkt),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_wsch_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_wsch_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_wsch_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_wsch_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_wsch_trunc_pkt),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_wsch_trunc_pkt),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_wsch_trunc_pkt),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_wsch_trunc_pkt),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_recirc_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_recirc_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_recirc_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_recirc_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan0_group.chnl_parser_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan1_group.chnl_parser_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan2_group.chnl_parser_discard_pkt),
        0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .ing_buf_regs.chan3_group.chnl_parser_discard_pkt),
        0);
  }
}

void pipe_mgr_tof_pkt_path_clear_iprsr_counter(bf_dev_id_t devid,
                                               int p,
                                               int pg_port) {
  if ((pg_port % 4) == 0) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[0]),
                       0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[1]),
                       0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[2]),
                       0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[3]),
                       0);
  }
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_0_2),
      0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                  .prsr_reg.fcs_err_cnt.fcs_err_cnt_1_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                  .prsr_reg.fcs_err_cnt.fcs_err_cnt_0_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                  .prsr_reg.csum_err_cnt.csum_err_cnt_1_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                  .prsr_reg.csum_err_cnt.csum_err_cnt_0_2),
                     0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_0_2),
      0);
}

void pipe_mgr_tof_pkt_path_clear_idprsr_counter(bf_dev_id_t devid, int p) {
  lld_write_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.iir.hdr_too_long_i), 0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[0]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[1]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[2]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[3]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[4]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[5]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[6]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[7]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[8]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[9]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[10]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[11]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[12]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[13]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[14]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[15]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[16]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[17]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[18]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[19]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[20]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[21]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[22]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[23]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[24]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[25]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[26]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[27]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[28]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[29]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[30]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[31]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[32]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[33]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[34]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[35]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[36]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[37]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[38]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[39]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[40]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[41]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[42]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[43]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[44]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[45]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[46]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[47]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[48]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[49]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[50]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[51]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[52]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[53]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[54]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[55]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[56]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[57]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[58]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[59]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[60]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[61]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[62]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[63]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[64]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[65]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[66]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[67]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[68]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[69]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[70]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[71]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_egr_pkt_err),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_ctm_pkt_err),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_0_2),
      0);
}

void pipe_mgr_tof_pkt_path_clear_ebuf_counter(bf_dev_id_t devid,
                                              int p,
                                              int pg_port) {
  if ((pg_port % 4) == 0) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[0]
                                    .egr_bypass_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[0]
                                    .egr_bypass_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[1]
                                    .egr_bypass_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[1]
                                    .egr_bypass_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[2]
                                    .egr_bypass_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[2]
                                    .egr_bypass_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[3]
                                    .egr_bypass_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_bypass_count[3]
                                    .egr_bypass_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 0) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[0]
                                    .egr_pipe_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[0]
                                    .egr_pipe_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[1]
                                    .egr_pipe_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[1]
                                    .egr_pipe_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[2]
                                    .egr_pipe_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[2]
                                    .egr_pipe_count_0_2),
                       0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[3]
                                    .egr_pipe_count_1_2),
                       0);
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                    .epb_disp_port_regs.egr_pipe_count[3]
                                    .egr_pipe_count_0_2),
                       0);
  }
}

void pipe_mgr_tof_pkt_path_clear_eprsr_counter(bf_dev_id_t devid,
                                               int p,
                                               int pg_port) {
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2),
      0);
  if ((pg_port % 4) == 0) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[0]),
                       0);
  }
  if ((pg_port % 4) == 1) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[1]),
                       0);
  }
  if ((pg_port % 4) == 2) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[2]),
                       0);
  }
  if ((pg_port % 4) == 3) {
    lld_write_register(devid,
                       offsetof(Tofino,
                                pipes[p]
                                    .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                    .prsr_reg.pkt_drop_cnt[3]),
                       0);
  }
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_0_2),
      0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                  .prsr_reg.fcs_err_cnt.fcs_err_cnt_1_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                  .prsr_reg.fcs_err_cnt.fcs_err_cnt_0_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                  .prsr_reg.csum_err_cnt.csum_err_cnt_1_2),
                     0);
  lld_write_register(devid,
                     offsetof(Tofino,
                              pipes[p]
                                  .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                  .prsr_reg.csum_err_cnt.csum_err_cnt_0_2),
                     0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p]
                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                   .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_0_2),
      0);
}

void pipe_mgr_tof_pkt_path_clear_edprsr_counter(bf_dev_id_t devid, int p) {
  lld_write_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.ier.hdr_too_long_e), 0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[0]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[1]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[2]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[3]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[4]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[5]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[6]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[7]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[8]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[9]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[10]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[11]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[12]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[13]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[14]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[15]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[16]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[17]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[18]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[19]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[20]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[21]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[22]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[23]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[24]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[25]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[26]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[27]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[28]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[29]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[30]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[31]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[32]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[33]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[34]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[35]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[36]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[37]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[38]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[39]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[40]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[41]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[42]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[43]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[44]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[45]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[46]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[47]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[48]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[49]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[50]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[51]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[52]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[53]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[54]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[55]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[56]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[57]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[58]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[59]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[60]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[61]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[62]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[63]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[64]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[65]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[66]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[67]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[68]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[69]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[70]),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[71]),
      0);
  lld_write_register(
      devid, offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_egr_pkt_err), 0);
  lld_write_register(
      devid, offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_ctm_pkt_err), 0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_0_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_1_2),
      0);
  lld_write_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_0_2),
      0);
}

void pipe_mgr_tof_pkt_path_clear_pipe_counter(bf_dev_id_t devid, int p) {
  pipe_mgr_tof_pkt_path_clear_idprsr_counter(devid, p);

  pipe_mgr_tof_pkt_path_clear_edprsr_counter(devid, p);
}

void pipe_mgr_tof_pkt_path_clear_all_counter(bf_dev_id_t devid, int p) {
  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 0);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 0);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 0);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 1);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 1);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 1);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 2);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 2);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 2);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 3);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 3);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 3);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 4);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 4);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 4);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 5);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 5);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 5);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 6);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 6);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 6);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 7);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 7);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 7);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 8);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 8);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 8);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 9);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 9);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 9);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 10);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 10);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 10);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 11);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 11);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 11);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 12);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 12);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 12);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 13);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 13);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 13);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 14);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 14);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 14);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 15);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 15);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 15);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 16);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 16);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 16);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 17);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 17);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 17);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 18);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 18);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 18);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 19);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 19);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 19);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 20);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 20);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 20);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 21);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 21);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 21);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 22);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 22);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 22);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 23);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 23);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 23);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 24);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 24);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 24);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 25);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 25);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 25);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 26);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 26);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 26);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 27);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 27);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 27);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 28);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 28);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 28);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 29);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 29);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 29);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 30);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 30);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 30);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 31);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 31);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 31);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 32);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 32);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 32);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 33);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 33);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 33);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 34);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 34);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 34);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 35);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 35);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 35);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 36);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 36);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 36);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 37);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 37);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 37);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 38);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 38);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 38);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 39);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 39);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 39);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 40);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 40);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 40);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 41);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 41);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 41);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 42);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 42);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 42);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 43);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 43);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 43);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 44);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 44);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 44);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 45);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 45);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 45);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 46);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 46);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 46);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 47);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 47);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 47);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 48);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 48);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 48);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 49);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 49);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 49);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 50);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 50);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 50);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 51);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 51);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 51);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 52);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 52);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 52);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 53);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 53);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 53);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 54);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 54);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 54);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 55);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 55);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 55);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 56);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 56);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 56);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 57);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 57);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 57);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 58);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 58);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 58);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 59);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 59);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 59);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 60);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 60);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 60);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 61);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 61);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 61);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 62);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 62);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 62);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 63);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 63);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 63);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 64);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 64);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 64);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 65);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 65);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 65);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 66);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 66);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 66);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 67);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 67);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 67);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 68);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 68);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 68);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 69);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 69);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 69);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 70);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 70);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 70);

  pipe_mgr_tof_pkt_path_clear_ibuf_counter(devid, p, 71);

  pipe_mgr_tof_pkt_path_clear_iprsr_counter(devid, p, 71);

  pipe_mgr_tof_pkt_path_clear_eprsr_counter(devid, p, 71);

  pipe_mgr_tof_pkt_path_clear_idprsr_counter(devid, p);

  pipe_mgr_tof_pkt_path_clear_edprsr_counter(devid, p);
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_ibuf_read_counter(
    bf_dev_id_t devid, int p, int port, int chan_numb, int *count) {
  uint32_t val, num_counters;
  int pg_port;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (!count) return NULL;
  num_counters = 36;
  counter_head = bf_sys_calloc((num_counters * chan_numb),
                               sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  for (pg_port = port; pg_port < (port + chan_numb); pg_port++) {
    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 0;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 1;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 2;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_macs_received_pkt
                       .chnl_macs_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 3;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 4;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 5;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 6;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_parser_send_pkt
                       .chnl_parser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 7;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 8;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 9;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 10;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_deparser_send_pkt
                       .chnl_deparser_send_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 11;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 12;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 13;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 14;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_0_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_recirc_received_pkt
                       .chnl_recirc_received_pkt_1_2),
          &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 15;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_deparser_drop_pkt),
          &val);
      cnt = val;
      counters->description_index = 16;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_deparser_drop_pkt),
          &val);
      cnt = val;
      counters->description_index = 17;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_deparser_drop_pkt),
          &val);
      cnt = val;
      counters->description_index = 18;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_deparser_drop_pkt),
          &val);
      cnt = val;
      counters->description_index = 19;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_wsch_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 20;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_wsch_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 21;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_wsch_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 22;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_wsch_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 23;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_wsch_trunc_pkt),
          &val);
      cnt = val;
      counters->description_index = 24;
      counters->counter_type = 2;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_wsch_trunc_pkt),
          &val);
      cnt = val;
      counters->description_index = 25;
      counters->counter_type = 2;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_wsch_trunc_pkt),
          &val);
      cnt = val;
      counters->description_index = 26;
      counters->counter_type = 2;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_wsch_trunc_pkt),
          &val);
      cnt = val;
      counters->description_index = 27;
      counters->counter_type = 2;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_recirc_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 28;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_recirc_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 29;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_recirc_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 30;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_recirc_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 31;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan0_group.chnl_parser_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 32;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan1_group.chnl_parser_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 33;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan2_group.chnl_parser_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 34;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(
          devid,
          offsetof(Tofino,
                   pipes[p]
                       .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                       .ing_buf_regs.chan3_group.chnl_parser_discard_pkt),
          &val);
      cnt = val;
      counters->description_index = 35;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }
  }
  *count = num_counters;
  return counter_head;
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_iprsr_read_counter(
    bf_dev_id_t devid, int p, int port, int chan_numb, int *count) {
  uint32_t val, num_counters;
  int pg_port;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (!count) return NULL;
  num_counters = 17;
  counter_head = bf_sys_calloc((num_counters * chan_numb),
                               sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  for (pg_port = port; pg_port < (port + chan_numb); pg_port++) {
    if ((pg_port % 4) == 0) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[0]),
                        &val);
      cnt = val;
      counters->description_index = 36;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[1]),
                        &val);
      cnt = val;
      counters->description_index = 37;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[2]),
                        &val);
      cnt = val;
      counters->description_index = 38;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[3]),
                        &val);
      cnt = val;
      counters->description_index = 39;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 40;
    counters->counter_type = 3;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(
            Tofino,
            pipes[p]
                .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(
            Tofino,
            pipes[p]
                .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 41;
    counters->counter_type = 3;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 42;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 43;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 44;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 45;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 46;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 47;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 48;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 49;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                   .prsr_reg.fcs_err_cnt.fcs_err_cnt_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                   .prsr_reg.fcs_err_cnt.fcs_err_cnt_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 50;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                   .prsr_reg.csum_err_cnt.csum_err_cnt_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                                   .prsr_reg.csum_err_cnt.csum_err_cnt_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 51;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ibp18_reg.ibp_reg[pg_port / 4]
                     .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 52;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;
  }
  *count = num_counters;
  return counter_head;
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_idprsr_counter(bf_dev_id_t devid,
                                                              int p,
                                                              int *count) {
  uint32_t val, num_counters;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  num_counters = 85;
  counter_head = bf_sys_calloc(num_counters, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  lld_read_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.iir.hdr_too_long_i), &val);
  cnt = val;
  counters->description_index = 53;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_phv.cnt_i_phv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 54;
  counters->counter_type = 4;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.main_i.cnt_i_tphv.cnt_i_tphv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 55;
  counters->counter_type = 4;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_read.cnt_i_read_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 56;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_discard.cnt_i_discard_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 57;
  counters->counter_type = 1;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(
          Tofino,
          pipes[p].deparser.inp.iir.ingr.cnt_i_resubmit.cnt_i_resubmit_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 58;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.iir.ingr.cnt_i_learn.cnt_i_learn_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 59;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.cnt_pkts.cnt_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 60;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[0]),
      &val);
  cnt = val;
  counters->description_index = 61;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[1]),
      &val);
  cnt = val;
  counters->description_index = 62;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[2]),
      &val);
  cnt = val;
  counters->description_index = 63;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[3]),
      &val);
  cnt = val;
  counters->description_index = 64;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[4]),
      &val);
  cnt = val;
  counters->description_index = 65;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[5]),
      &val);
  cnt = val;
  counters->description_index = 66;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[6]),
      &val);
  cnt = val;
  counters->description_index = 67;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[7]),
      &val);
  cnt = val;
  counters->description_index = 68;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[8]),
      &val);
  cnt = val;
  counters->description_index = 69;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[9]),
      &val);
  cnt = val;
  counters->description_index = 70;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[10]),
      &val);
  cnt = val;
  counters->description_index = 71;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[11]),
      &val);
  cnt = val;
  counters->description_index = 72;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[12]),
      &val);
  cnt = val;
  counters->description_index = 73;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[13]),
      &val);
  cnt = val;
  counters->description_index = 74;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[14]),
      &val);
  cnt = val;
  counters->description_index = 75;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[15]),
      &val);
  cnt = val;
  counters->description_index = 76;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[16]),
      &val);
  cnt = val;
  counters->description_index = 77;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[17]),
      &val);
  cnt = val;
  counters->description_index = 78;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[18]),
      &val);
  cnt = val;
  counters->description_index = 79;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[19]),
      &val);
  cnt = val;
  counters->description_index = 80;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[20]),
      &val);
  cnt = val;
  counters->description_index = 81;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[21]),
      &val);
  cnt = val;
  counters->description_index = 82;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[22]),
      &val);
  cnt = val;
  counters->description_index = 83;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[23]),
      &val);
  cnt = val;
  counters->description_index = 84;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[24]),
      &val);
  cnt = val;
  counters->description_index = 85;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[25]),
      &val);
  cnt = val;
  counters->description_index = 86;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[26]),
      &val);
  cnt = val;
  counters->description_index = 87;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[27]),
      &val);
  cnt = val;
  counters->description_index = 88;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[28]),
      &val);
  cnt = val;
  counters->description_index = 89;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[29]),
      &val);
  cnt = val;
  counters->description_index = 90;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[30]),
      &val);
  cnt = val;
  counters->description_index = 91;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[31]),
      &val);
  cnt = val;
  counters->description_index = 92;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[32]),
      &val);
  cnt = val;
  counters->description_index = 93;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[33]),
      &val);
  cnt = val;
  counters->description_index = 94;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[34]),
      &val);
  cnt = val;
  counters->description_index = 95;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[35]),
      &val);
  cnt = val;
  counters->description_index = 96;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[36]),
      &val);
  cnt = val;
  counters->description_index = 97;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[37]),
      &val);
  cnt = val;
  counters->description_index = 98;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[38]),
      &val);
  cnt = val;
  counters->description_index = 99;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[39]),
      &val);
  cnt = val;
  counters->description_index = 100;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[40]),
      &val);
  cnt = val;
  counters->description_index = 101;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[41]),
      &val);
  cnt = val;
  counters->description_index = 102;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[42]),
      &val);
  cnt = val;
  counters->description_index = 103;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[43]),
      &val);
  cnt = val;
  counters->description_index = 104;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[44]),
      &val);
  cnt = val;
  counters->description_index = 105;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[45]),
      &val);
  cnt = val;
  counters->description_index = 106;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[46]),
      &val);
  cnt = val;
  counters->description_index = 107;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[47]),
      &val);
  cnt = val;
  counters->description_index = 108;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[48]),
      &val);
  cnt = val;
  counters->description_index = 109;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[49]),
      &val);
  cnt = val;
  counters->description_index = 110;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[50]),
      &val);
  cnt = val;
  counters->description_index = 111;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[51]),
      &val);
  cnt = val;
  counters->description_index = 112;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[52]),
      &val);
  cnt = val;
  counters->description_index = 113;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[53]),
      &val);
  cnt = val;
  counters->description_index = 114;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[54]),
      &val);
  cnt = val;
  counters->description_index = 115;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[55]),
      &val);
  cnt = val;
  counters->description_index = 116;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[56]),
      &val);
  cnt = val;
  counters->description_index = 117;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[57]),
      &val);
  cnt = val;
  counters->description_index = 118;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[58]),
      &val);
  cnt = val;
  counters->description_index = 119;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[59]),
      &val);
  cnt = val;
  counters->description_index = 120;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[60]),
      &val);
  cnt = val;
  counters->description_index = 121;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[61]),
      &val);
  cnt = val;
  counters->description_index = 122;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[62]),
      &val);
  cnt = val;
  counters->description_index = 123;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[63]),
      &val);
  cnt = val;
  counters->description_index = 124;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[64]),
      &val);
  cnt = val;
  counters->description_index = 125;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[65]),
      &val);
  cnt = val;
  counters->description_index = 126;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[66]),
      &val);
  cnt = val;
  counters->description_index = 127;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[67]),
      &val);
  cnt = val;
  counters->description_index = 128;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[68]),
      &val);
  cnt = val;
  counters->description_index = 129;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[69]),
      &val);
  cnt = val;
  counters->description_index = 130;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[70]),
      &val);
  cnt = val;
  counters->description_index = 131;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.ctm_crc_err[71]),
      &val);
  cnt = val;
  counters->description_index = 132;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_egr_pkt_err),
      &val);
  cnt = val;
  counters->description_index = 133;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_ingr.regs.i_ctm_pkt_err),
      &val);
  cnt = val;
  counters->description_index = 134;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_fwd_pkts.i_fwd_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 135;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_disc_pkts.i_disc_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 136;
  counters->counter_type = 1;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_ingr.regs.i_mirr_pkts.i_mirr_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 137;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  *count = num_counters;
  return counter_head;
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_ebuf_read_counter(
    bf_dev_id_t devid, int p, int port, int chan_numb, int *count) {
  uint32_t val, num_counters;
  int pg_port;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (!count) return NULL;
  num_counters = 8;
  counter_head = bf_sys_calloc((num_counters * chan_numb),
                               sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  for (pg_port = port; pg_port < (port + chan_numb); pg_port++) {
    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[0]
                                     .egr_bypass_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[0]
                                     .egr_bypass_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 138;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[1]
                                     .egr_bypass_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[1]
                                     .egr_bypass_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 139;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[2]
                                     .egr_bypass_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[2]
                                     .egr_bypass_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 140;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[3]
                                     .egr_bypass_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_bypass_count[3]
                                     .egr_bypass_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 141;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 0) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[0]
                                     .egr_pipe_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[0]
                                     .egr_pipe_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 142;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[1]
                                     .egr_pipe_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[1]
                                     .egr_pipe_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 143;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[2]
                                     .egr_pipe_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[2]
                                     .egr_pipe_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 144;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      cnt = 0;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[3]
                                     .egr_pipe_count_0_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 0) + cnt;
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.egrNx_reg[pg_port / 4]
                                     .epb_disp_port_regs.egr_pipe_count[3]
                                     .egr_pipe_count_1_2),
                        &val);
      reg_cnt = val;
      cnt = (reg_cnt << 32) + cnt;
      counters->description_index = 145;
      counters->counter_type = 0;
      counters->value = cnt;
      counters++;
      num_counters++;
    }
  }
  *count = num_counters;
  return counter_head;
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_eprsr_read_counter(
    bf_dev_id_t devid, int p, int port, int chan_numb, int *count) {
  uint32_t val, num_counters;
  int pg_port;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  if (!count) return NULL;
  num_counters = 18;
  counter_head = bf_sys_calloc((num_counters * chan_numb),
                               sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  for (pg_port = port; pg_port < (port + chan_numb); pg_port++) {
    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.op_fifo_full_cnt.op_fifo_full_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 146;
    counters->counter_type = 3;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(
            Tofino,
            pipes[p]
                .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(
            Tofino,
            pipes[p]
                .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                .prsr_reg.op_fifo_full_stall_cnt.op_fifo_full_stall_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 147;
    counters->counter_type = 3;
    counters->value = cnt;
    counters++;
    num_counters++;

    if ((pg_port % 4) == 0) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[0]),
                        &val);
      cnt = val;
      counters->description_index = 148;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 1) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[1]),
                        &val);
      cnt = val;
      counters->description_index = 149;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 2) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[2]),
                        &val);
      cnt = val;
      counters->description_index = 150;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    if ((pg_port % 4) == 3) {
      lld_read_register(devid,
                        offsetof(Tofino,
                                 pipes[p]
                                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                     .prsr_reg.pkt_drop_cnt[3]),
                        &val);
      cnt = val;
      counters->description_index = 151;
      counters->counter_type = 1;
      counters->value = cnt;
      counters++;
      num_counters++;
    }

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.no_tcam_match_err_cnt.no_tcam_match_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 152;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.partial_hdr_err_cnt.partial_hdr_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 153;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.ctr_range_err_cnt.ctr_range_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 154;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.timeout_iter_err_cnt.timeout_iter_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 155;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.timeout_cycle_err_cnt.timeout_cycle_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 156;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.src_ext_err_cnt.src_ext_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 157;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.dst_cont_err_cnt.dst_cont_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 158;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.phv_owner_err_cnt.phv_owner_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 159;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.multi_wr_err_cnt.multi_wr_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 160;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.fcs_err_cnt.fcs_err_cnt_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.fcs_err_cnt.fcs_err_cnt_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 161;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.csum_err_cnt.csum_err_cnt_0_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(devid,
                      offsetof(Tofino,
                               pipes[p]
                                   .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                                   .prsr_reg.csum_err_cnt.csum_err_cnt_1_2),
                      &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 162;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;

    cnt = 0;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_0_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 0) + cnt;
    lld_read_register(
        devid,
        offsetof(Tofino,
                 pipes[p]
                     .pmarb.ebp18_reg.ebp_reg[pg_port / 4]
                     .prsr_reg.tcam_par_err_cnt.tcam_par_err_cnt_1_2),
        &val);
    reg_cnt = val;
    cnt = (reg_cnt << 32) + cnt;
    counters->description_index = 163;
    counters->counter_type = 2;
    counters->value = cnt;
    counters++;
    num_counters++;
  }
  *count = num_counters;
  return counter_head;
}

bf_packetpath_counter_t *pipe_mgr_tof_pkt_path_edprsr_counter(bf_dev_id_t devid,
                                                              int p,
                                                              int *count) {
  uint32_t val, num_counters;
  bf_packetpath_counter_t *counters, *counter_head = NULL;
  uint64_t cnt = 0;
  uint64_t reg_cnt = 0;

  num_counters = 81;
  counter_head = bf_sys_calloc(num_counters, sizeof(bf_packetpath_counter_t));
  if (!counter_head) return NULL;
  counters = counter_head;

  num_counters = 0;

  lld_read_register(
      devid, offsetof(Tofino, pipes[p].deparser.inp.ier.hdr_too_long_e), &val);
  cnt = val;
  counters->description_index = 164;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_phv.cnt_i_phv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 165;
  counters->counter_type = 4;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.inp.ier.main_e.cnt_i_tphv.cnt_i_tphv_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 166;
  counters->counter_type = 4;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.cnt_pkts.cnt_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 167;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[0]),
      &val);
  cnt = val;
  counters->description_index = 168;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[1]),
      &val);
  cnt = val;
  counters->description_index = 169;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[2]),
      &val);
  cnt = val;
  counters->description_index = 170;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[3]),
      &val);
  cnt = val;
  counters->description_index = 171;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[4]),
      &val);
  cnt = val;
  counters->description_index = 172;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[5]),
      &val);
  cnt = val;
  counters->description_index = 173;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[6]),
      &val);
  cnt = val;
  counters->description_index = 174;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[7]),
      &val);
  cnt = val;
  counters->description_index = 175;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[8]),
      &val);
  cnt = val;
  counters->description_index = 176;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[9]),
      &val);
  cnt = val;
  counters->description_index = 177;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[10]),
      &val);
  cnt = val;
  counters->description_index = 178;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[11]),
      &val);
  cnt = val;
  counters->description_index = 179;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[12]),
      &val);
  cnt = val;
  counters->description_index = 180;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[13]),
      &val);
  cnt = val;
  counters->description_index = 181;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[14]),
      &val);
  cnt = val;
  counters->description_index = 182;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[15]),
      &val);
  cnt = val;
  counters->description_index = 183;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[16]),
      &val);
  cnt = val;
  counters->description_index = 184;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[17]),
      &val);
  cnt = val;
  counters->description_index = 185;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[18]),
      &val);
  cnt = val;
  counters->description_index = 186;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[19]),
      &val);
  cnt = val;
  counters->description_index = 187;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[20]),
      &val);
  cnt = val;
  counters->description_index = 188;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[21]),
      &val);
  cnt = val;
  counters->description_index = 189;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[22]),
      &val);
  cnt = val;
  counters->description_index = 190;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[23]),
      &val);
  cnt = val;
  counters->description_index = 191;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[24]),
      &val);
  cnt = val;
  counters->description_index = 192;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[25]),
      &val);
  cnt = val;
  counters->description_index = 193;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[26]),
      &val);
  cnt = val;
  counters->description_index = 194;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[27]),
      &val);
  cnt = val;
  counters->description_index = 195;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[28]),
      &val);
  cnt = val;
  counters->description_index = 196;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[29]),
      &val);
  cnt = val;
  counters->description_index = 197;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[30]),
      &val);
  cnt = val;
  counters->description_index = 198;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[31]),
      &val);
  cnt = val;
  counters->description_index = 199;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[32]),
      &val);
  cnt = val;
  counters->description_index = 200;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[33]),
      &val);
  cnt = val;
  counters->description_index = 201;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[34]),
      &val);
  cnt = val;
  counters->description_index = 202;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[35]),
      &val);
  cnt = val;
  counters->description_index = 203;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[36]),
      &val);
  cnt = val;
  counters->description_index = 204;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[37]),
      &val);
  cnt = val;
  counters->description_index = 205;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[38]),
      &val);
  cnt = val;
  counters->description_index = 206;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[39]),
      &val);
  cnt = val;
  counters->description_index = 207;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[40]),
      &val);
  cnt = val;
  counters->description_index = 208;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[41]),
      &val);
  cnt = val;
  counters->description_index = 209;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[42]),
      &val);
  cnt = val;
  counters->description_index = 210;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[43]),
      &val);
  cnt = val;
  counters->description_index = 211;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[44]),
      &val);
  cnt = val;
  counters->description_index = 212;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[45]),
      &val);
  cnt = val;
  counters->description_index = 213;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[46]),
      &val);
  cnt = val;
  counters->description_index = 214;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[47]),
      &val);
  cnt = val;
  counters->description_index = 215;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[48]),
      &val);
  cnt = val;
  counters->description_index = 216;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[49]),
      &val);
  cnt = val;
  counters->description_index = 217;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[50]),
      &val);
  cnt = val;
  counters->description_index = 218;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[51]),
      &val);
  cnt = val;
  counters->description_index = 219;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[52]),
      &val);
  cnt = val;
  counters->description_index = 220;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[53]),
      &val);
  cnt = val;
  counters->description_index = 221;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[54]),
      &val);
  cnt = val;
  counters->description_index = 222;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[55]),
      &val);
  cnt = val;
  counters->description_index = 223;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[56]),
      &val);
  cnt = val;
  counters->description_index = 224;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[57]),
      &val);
  cnt = val;
  counters->description_index = 225;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[58]),
      &val);
  cnt = val;
  counters->description_index = 226;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[59]),
      &val);
  cnt = val;
  counters->description_index = 227;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[60]),
      &val);
  cnt = val;
  counters->description_index = 228;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[61]),
      &val);
  cnt = val;
  counters->description_index = 229;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[62]),
      &val);
  cnt = val;
  counters->description_index = 230;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[63]),
      &val);
  cnt = val;
  counters->description_index = 231;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[64]),
      &val);
  cnt = val;
  counters->description_index = 232;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[65]),
      &val);
  cnt = val;
  counters->description_index = 233;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[66]),
      &val);
  cnt = val;
  counters->description_index = 234;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[67]),
      &val);
  cnt = val;
  counters->description_index = 235;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[68]),
      &val);
  cnt = val;
  counters->description_index = 236;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[69]),
      &val);
  cnt = val;
  counters->description_index = 237;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[70]),
      &val);
  cnt = val;
  counters->description_index = 238;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.ctm_crc_err[71]),
      &val);
  cnt = val;
  counters->description_index = 239;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_egr_pkt_err),
      &val);
  cnt = val;
  counters->description_index = 240;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  lld_read_register(
      devid,
      offsetof(Tofino, pipes[p].deparser.out_egr.regs.e_ctm_pkt_err),
      &val);
  cnt = val;
  counters->description_index = 241;
  counters->counter_type = 2;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_fwd_pkts.e_fwd_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 242;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_disc_pkts.e_disc_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 243;
  counters->counter_type = 1;
  counters->value = cnt;
  counters++;
  num_counters++;

  cnt = 0;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_0_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 0) + cnt;
  lld_read_register(
      devid,
      offsetof(Tofino,
               pipes[p].deparser.out_egr.regs.e_mirr_pkts.e_mirr_pkts_1_2),
      &val);
  reg_cnt = val;
  cnt = (reg_cnt << 32) + cnt;
  counters->description_index = 244;
  counters->counter_type = 0;
  counters->value = cnt;
  counters++;
  num_counters++;

  *count = num_counters;
  return counter_head;
}

static const char *pipe_mgr_tof_pkt_path_counter_strings[] = {
    "IBUF: Pkts From MAC on Channel0",
    "IBUF: Pkts From MAC on Channel1",
    "IBUF: Pkts From MAC on Channel2",
    "IBUF: Pkts From MAC on Channel3",
    "IBUF: Pkts from channel0 sent to Parser",
    "IBUF: Pkts from channel1 sent to Parser",
    "IBUF: Pkts from channel2 sent to Parser",
    "IBUF: Pkts from channel3 sent to Parser",
    "IBUF: Pkts from channel0 sent to DeParser",
    "IBUF: Pkts from channel1 sent to DeParser",
    "IBUF: Pkts from channel2 sent to DeParser",
    "IBUF: Pkts from channel3 sent to DeParser",
    "IBUF: Recirculation Pkts on channel0",
    "IBUF: Recirculation Pkts on channel1",
    "IBUF: Recirculation Pkts on channel2",
    "IBUF: Recirculation Pkts on channel3",
    "IBUF: Total Pkts Dropped on channel0",
    "IBUF: Total Pkts Dropped on channel1",
    "IBUF: Total Pkts Dropped on channel2",
    "IBUF: Total Pkts Dropped on channel3",
    "IBUF: Discarded Pkts in ibuf on channel0",
    "IBUF: Discarded Pkts in ibuf on channel1",
    "IBUF: Discarded Pkts in ibuf on channel2",
    "IBUF: Discarded Pkts in ibuf on channel3",
    "IBUF: Truncated Pkts on channel0",
    "IBUF: Truncated Pkts on channel1",
    "IBUF: Truncated Pkts on channel2",
    "IBUF: Truncated Pkts on channel3",
    "IBUF: Discarded Recirculated Pkts on ch0",
    "IBUF: Discarded Recirculated Pkts on ch1",
    "IBUF: Discarded Recirculated Pkts on ch2",
    "IBUF: Discarded Recirculated Pkts on ch3",
    "IBUF: Discarded Pkts in PRSR due to ibuf ch0 full or prsr req to drop ",
    "IBUF: Discarded Pkts in PRSR due to ibuf ch1 full or prsr req to drop ",
    "IBUF: Discarded Pkts in PRSR due to ibuf ch2 full or prsr req to drop ",
    "IBUF: Discarded Pkts in PRSR due to ibuf ch3 full or prsr req to drop ",
    "IPRSR: Total Pkts Dropped on Channel0",
    "IPRSR: Total Pkts Dropped on Channel1",
    "IPRSR: Total Pkts Dropped on Channel2",
    "IPRSR: Total Pkts Dropped on Channel3",
    "IPRSR: output fifo full counter",
    "IPRSR: output fifo stall counter",
    "IPRSR: TCAM match Error",
    "IPRSR: Counter Range Error",
    "IPRSR: Timeout or Excess state iteration Error",
    "IPRSR: Timeout or Excess clock cycle",
    "IPRSR: Extraction source error counter",
    "IPRSR: Container size error counter",
    "IPRSR: PHV owner error counter",
    "IPRSR: PHV multiple write error",
    "IPRSR: FCS error",
    "IPRSR: Checksum error",
    "IPRSR: TCAM parity error",
    "IDPRSR: header too long",
    "IDPRSR: Valid PHV into Deparser",
    "IDPRSR: Valid TPHV into Deparser",
    "IDPRSR: Packets into Deparser",
    "IDPRSR: Packets discarded ",
    "IDPRSR: Packets resubmitted ",
    "IDPRSR: Learn Counter due to packets ",
    "IDPRSR: Packets processed in output phase ",
    "IDPRSR: CRC error on packets from Ibuf 0",
    "IDPRSR: CRC error on packets from Ibuf 1",
    "IDPRSR: CRC error on packets from Ibuf 2",
    "IDPRSR: CRC error on packets from Ibuf 3",
    "IDPRSR: CRC error on packets from Ibuf 4",
    "IDPRSR: CRC error on packets from Ibuf 5",
    "IDPRSR: CRC error on packets from Ibuf 6",
    "IDPRSR: CRC error on packets from Ibuf 7",
    "IDPRSR: CRC error on packets from Ibuf 8",
    "IDPRSR: CRC error on packets from Ibuf 9",
    "IDPRSR: CRC error on packets from Ibuf 10",
    "IDPRSR: CRC error on packets from Ibuf 11",
    "IDPRSR: CRC error on packets from Ibuf 12",
    "IDPRSR: CRC error on packets from Ibuf 13",
    "IDPRSR: CRC error on packets from Ibuf 14",
    "IDPRSR: CRC error on packets from Ibuf 15",
    "IDPRSR: CRC error on packets from Ibuf 16",
    "IDPRSR: CRC error on packets from Ibuf 17",
    "IDPRSR: CRC error on packets from Ibuf 18",
    "IDPRSR: CRC error on packets from Ibuf 19",
    "IDPRSR: CRC error on packets from Ibuf 20",
    "IDPRSR: CRC error on packets from Ibuf 21",
    "IDPRSR: CRC error on packets from Ibuf 22",
    "IDPRSR: CRC error on packets from Ibuf 23",
    "IDPRSR: CRC error on packets from Ibuf 24",
    "IDPRSR: CRC error on packets from Ibuf 25",
    "IDPRSR: CRC error on packets from Ibuf 26",
    "IDPRSR: CRC error on packets from Ibuf 27",
    "IDPRSR: CRC error on packets from Ibuf 28",
    "IDPRSR: CRC error on packets from Ibuf 29",
    "IDPRSR: CRC error on packets from Ibuf 30",
    "IDPRSR: CRC error on packets from Ibuf 31",
    "IDPRSR: CRC error on packets from Ibuf 32",
    "IDPRSR: CRC error on packets from Ibuf 33",
    "IDPRSR: CRC error on packets from Ibuf 34",
    "IDPRSR: CRC error on packets from Ibuf 35",
    "IDPRSR: CRC error on packets from Ibuf 36",
    "IDPRSR: CRC error on packets from Ibuf 37",
    "IDPRSR: CRC error on packets from Ibuf 38",
    "IDPRSR: CRC error on packets from Ibuf 39",
    "IDPRSR: CRC error on packets from Ibuf 40",
    "IDPRSR: CRC error on packets from Ibuf 41",
    "IDPRSR: CRC error on packets from Ibuf 42",
    "IDPRSR: CRC error on packets from Ibuf 43",
    "IDPRSR: CRC error on packets from Ibuf 44",
    "IDPRSR: CRC error on packets from Ibuf 45",
    "IDPRSR: CRC error on packets from Ibuf 46",
    "IDPRSR: CRC error on packets from Ibuf 47",
    "IDPRSR: CRC error on packets from Ibuf 48",
    "IDPRSR: CRC error on packets from Ibuf 49",
    "IDPRSR: CRC error on packets from Ibuf 50",
    "IDPRSR: CRC error on packets from Ibuf 51",
    "IDPRSR: CRC error on packets from Ibuf 52",
    "IDPRSR: CRC error on packets from Ibuf 53",
    "IDPRSR: CRC error on packets from Ibuf 54",
    "IDPRSR: CRC error on packets from Ibuf 55",
    "IDPRSR: CRC error on packets from Ibuf 56",
    "IDPRSR: CRC error on packets from Ibuf 57",
    "IDPRSR: CRC error on packets from Ibuf 58",
    "IDPRSR: CRC error on packets from Ibuf 59",
    "IDPRSR: CRC error on packets from Ibuf 60",
    "IDPRSR: CRC error on packets from Ibuf 61",
    "IDPRSR: CRC error on packets from Ibuf 62",
    "IDPRSR: CRC error on packets from Ibuf 63",
    "IDPRSR: CRC error on packets from Ibuf 64",
    "IDPRSR: CRC error on packets from Ibuf 65",
    "IDPRSR: CRC error on packets from Ibuf 66",
    "IDPRSR: CRC error on packets from Ibuf 67",
    "IDPRSR: CRC error on packets from Ibuf 68",
    "IDPRSR: CRC error on packets from Ibuf 69",
    "IDPRSR: CRC error on packets from Ibuf 70",
    "IDPRSR: CRC error on packets from Ibuf 71",
    "IDPRSR: Errored packets to TM ",
    "IDPRSR: Errored packets from Ibuf to iCTM",
    "IDPRSR: Packets sent to TM",
    "IDPRSR: Packets discarded at TM interface ",
    "IDPRSR: Packets sent to Ingress mirror buffer",
    "EPB: Total Pkts bypassing Epipe on Channel 0",
    "EPB: Total Pkts bypassing Epipe on Channel 1",
    "EPB: Total Pkts bypassing Epipe on Channel 2",
    "EPB: Total Pkts bypassing Epipe on Channel 3",
    "EPB: Total Pkts sent to Epipe on Channel 0",
    "EPB: Total Pkts sent to Epipe on Channel 1",
    "EPB: Total Pkts sent to Epipe on Channel 2",
    "EPB: Total Pkts sent to Epipe on Channel 3",
    "EPRSR: output fifo full counter",
    "EPRSR: output fifo stall counter",
    "EPRSR: Total Pkts Dropped on channel0",
    "EPRSR: Total Pkts Dropped on channel1",
    "EPRSR: Total Pkts Dropped on channel2",
    "EPRSR: Total Pkts Dropped on channel3",
    "EPRSR: TCAM match Error",
    "EPRSR: Partial Header Error",
    "EPRSR: Counter Range Error",
    "EPRSR: Timeout or Excess state iteration Error",
    "EPRSR: Timeout or Excess clock cycle",
    "EPRSR: Extraction source error counter",
    "EPRSR: Container size error counter",
    "EPRSR: PHV owner error counter",
    "EPRSR: PHV multiple write error",
    "EPRSR: FCS error",
    "EPRSR: Checksum error",
    "EPRSR: TCAM parity error",
    "EDPRSR: header too long",
    "EDPRSR: Valid PHV into Deparser",
    "EDPRSR: Valid TPHV into Deparser",
    "EDPRSR: Packets processed in output phase ",
    "EDPRSR: CRC error on packets from ebuf 0",
    "EDPRSR: CRC error on packets from ebuf 1",
    "EDPRSR: CRC error on packets from ebuf 2",
    "EDPRSR: CRC error on packets from ebuf 3",
    "EDPRSR: CRC error on packets from ebuf 4",
    "EDPRSR: CRC error on packets from ebuf 5",
    "EDPRSR: CRC error on packets from ebuf 6",
    "EDPRSR: CRC error on packets from ebuf 7",
    "EDPRSR: CRC error on packets from ebuf 8",
    "EDPRSR: CRC error on packets from ebuf 9",
    "EDPRSR: CRC error on packets from ebuf 10",
    "EDPRSR: CRC error on packets from ebuf 11",
    "EDPRSR: CRC error on packets from ebuf 12",
    "EDPRSR: CRC error on packets from ebuf 13",
    "EDPRSR: CRC error on packets from ebuf 14",
    "EDPRSR: CRC error on packets from ebuf 15",
    "EDPRSR: CRC error on packets from ebuf 16",
    "EDPRSR: CRC error on packets from ebuf 17",
    "EDPRSR: CRC error on packets from ebuf 18",
    "EDPRSR: CRC error on packets from ebuf 19",
    "EDPRSR: CRC error on packets from ebuf 20",
    "EDPRSR: CRC error on packets from ebuf 21",
    "EDPRSR: CRC error on packets from ebuf 22",
    "EDPRSR: CRC error on packets from ebuf 23",
    "EDPRSR: CRC error on packets from ebuf 24",
    "EDPRSR: CRC error on packets from ebuf 25",
    "EDPRSR: CRC error on packets from ebuf 26",
    "EDPRSR: CRC error on packets from ebuf 27",
    "EDPRSR: CRC error on packets from ebuf 28",
    "EDPRSR: CRC error on packets from ebuf 29",
    "EDPRSR: CRC error on packets from ebuf 30",
    "EDPRSR: CRC error on packets from ebuf 31",
    "EDPRSR: CRC error on packets from ebuf 32",
    "EDPRSR: CRC error on packets from ebuf 33",
    "EDPRSR: CRC error on packets from ebuf 34",
    "EDPRSR: CRC error on packets from ebuf 35",
    "EDPRSR: CRC error on packets from ebuf 36",
    "EDPRSR: CRC error on packets from ebuf 37",
    "EDPRSR: CRC error on packets from ebuf 38",
    "EDPRSR: CRC error on packets from ebuf 39",
    "EDPRSR: CRC error on packets from ebuf 40",
    "EDPRSR: CRC error on packets from ebuf 41",
    "EDPRSR: CRC error on packets from ebuf 42",
    "EDPRSR: CRC error on packets from ebuf 43",
    "EDPRSR: CRC error on packets from ebuf 44",
    "EDPRSR: CRC error on packets from ebuf 45",
    "EDPRSR: CRC error on packets from ebuf 46",
    "EDPRSR: CRC error on packets from ebuf 47",
    "EDPRSR: CRC error on packets from ebuf 48",
    "EDPRSR: CRC error on packets from ebuf 49",
    "EDPRSR: CRC error on packets from ebuf 50",
    "EDPRSR: CRC error on packets from ebuf 51",
    "EDPRSR: CRC error on packets from ebuf 52",
    "EDPRSR: CRC error on packets from ebuf 53",
    "EDPRSR: CRC error on packets from ebuf 54",
    "EDPRSR: CRC error on packets from ebuf 55",
    "EDPRSR: CRC error on packets from ebuf 56",
    "EDPRSR: CRC error on packets from ebuf 57",
    "EDPRSR: CRC error on packets from ebuf 58",
    "EDPRSR: CRC error on packets from ebuf 59",
    "EDPRSR: CRC error on packets from ebuf 60",
    "EDPRSR: CRC error on packets from ebuf 61",
    "EDPRSR: CRC error on packets from ebuf 62",
    "EDPRSR: CRC error on packets from ebuf 63",
    "EDPRSR: CRC error on packets from ebuf 64",
    "EDPRSR: CRC error on packets from ebuf 65",
    "EDPRSR: CRC error on packets from ebuf 66",
    "EDPRSR: CRC error on packets from ebuf 67",
    "EDPRSR: CRC error on packets from ebuf 68",
    "EDPRSR: CRC error on packets from ebuf 69",
    "EDPRSR: CRC error on packets from ebuf 70",
    "EDPRSR: CRC error on packets from ebuf 71",
    "EDPRSR: Errored packets from deprsr to Ebuf ",
    "EDPRSR: Errored packets from Ebuf to eCTM ",
    "EDPRSR: Packets sent out of Egress Deparser",
    "EDPRSR: Packets discarded ",
    "EDPRSR: Packets sent to Egress mirror buffer",
};

const char *pipe_mgr_tof_pkt_path_get_counter_description(
    uint32_t description_index) {
  if (description_index >= BF_MAX_PKT_PATH_COUNTER_INDEX) return NULL;
  return (pipe_mgr_tof_pkt_path_counter_strings[description_index]);
}
