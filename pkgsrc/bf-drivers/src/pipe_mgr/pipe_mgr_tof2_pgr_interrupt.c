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


#include "pipe_mgr_int.h"
#include "pipe_mgr_interrupt_comm.h"
#include "pipe_mgr_tof2_interrupt.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_db.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>

extern pipe_mgr_ctx_t *pipe_mgr_ctx;

static uint32_t pipe_mgr_tof2_pgr_err_handle0(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("pgr intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[pipe].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4
  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // pfc0_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_mbe_log_1_2_pfc0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc0 multi bit err at addr %d ", mem_err_addr);
          /* No further action */
          break;

        case 1:  // pfc0_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_sbe_log_1_2_pfc0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc0 single bit err at addr %d ", mem_err_addr);
          /* No further action */
          break;

        case 2:  // pfc1_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_mbe_log_1_2_pfc1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc1 multi bit err at addr %d ", mem_err_addr);
          /* No further action */
          break;

        case 3:  // pfc1_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_sbe_log_1_2_pfc1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR pfc1 single bit err at addr %d ", mem_err_addr);
          /* No further action */
          break;

        case 4:  // tbc_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_tbc_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 5:  // tbc_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_tbc_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 6:  // tbc_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_tbc_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 7:  // tbc_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_tbc_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 8:  // eth_cpu_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_eth_cpu_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 9:  // eth_cpu_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_eth_cpu_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 10:  // eth_cpu_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_eth_cpu_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 11:  // eth_cpu_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_eth_cpu_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 12:  // ebuf_p0_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_ebuf_p0_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 13:  // ebuf_p0_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_ebuf_p0_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 14:  // ebuf_p0_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_ebuf_p0_fifo0_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 15:  // ebuf_p0_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_ebuf_p0_fifo0_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 16:  // ebuf_p1_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = data & 0x7;
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr << 2) | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 17:  // ebuf_p1_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p1_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 18:  // ebuf_p1_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = data & 0x7;
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr << 2) | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 19:  // ebuf_p1_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p1_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 20:  // ebuf_p2_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p2_fifo0_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 21:  // ebuf_p2_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p2_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 22:  // ebuf_p2_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p2_fifo0_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 23:  // ebuf_p2_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p2_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 24:  // ebuf_p3_fifo0_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p3_fifo0_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 25:  // ebuf_p3_fifo0_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_mbe_log.pgr_data_fifo0_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_mbe_log_pgr_data_fifo0_mbe_log_1_2_ebuf_p3_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 26:  // ebuf_p3_fifo0_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p3_fifo0_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 27:  // ebuf_p3_fifo0_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo0_sbe_log.pgr_data_fifo0_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo0_sbe_log_pgr_data_fifo0_sbe_log_1_2_ebuf_p3_fifo0_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo0 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo0 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 28:  // tbc_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_tbc_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 29:  // tbc_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_tbc_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 30:  // tbc_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_tbc_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 31:  // tbc_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_tbc_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_pgr_err_handle1(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("pgr intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[n].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4
  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // eth_cpu_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_eth_cpu_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 1:  // eth_cpu_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_eth_cpu_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 2:  // eth_cpu_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_eth_cpu_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 3:  // eth_cpu_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_eth_cpu_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 4:  // ebuf_p0_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_ebuf_p0_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p0 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 5:  // ebuf_p0_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_ebuf_p0_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p0 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 6:  // ebuf_p0_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_ebuf_p0_fifo1_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 7:  // ebuf_p0_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_ebuf_p0_fifo1_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 8:  // ebuf_p1_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr) | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p1 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 9:  // ebuf_p1_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p1_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p1 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 10:  // ebuf_p1_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (mem_err_addr) | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 11:  // ebuf_p1_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p1_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 12:  // ebuf_p2_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p2_fifo1_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p2 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 13:  // ebuf_p2_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p2_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebug p2 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 14:  // ebuf_p2_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p2_fifo1_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 15:  // ebuf_p2_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p2_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 16:  // ebuf_p3_fifo1_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p3_fifo1_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 17:  // ebuf_p3_fifo1_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_mbe_log.pgr_data_fifo1_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_mbe_log_pgr_data_fifo1_mbe_log_1_2_ebuf_p3_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 18:  // ebuf_p3_fifo1_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p3_fifo1_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 19:  // ebuf_p3_fifo1_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo1_sbe_log.pgr_data_fifo1_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo1_sbe_log_pgr_data_fifo1_sbe_log_1_2_ebuf_p3_fifo1_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo1 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo1 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 20:  // tbc_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_tbc_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 21:  // tbc_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_tbc_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 22:  // tbc_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_tbc_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 23:  // tbc_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_tbc_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR tbc fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 24:  // eth_cpu_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_eth_cpu_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 25:  // eth_cpu_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_eth_cpu_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 26:  // eth_cpu_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_eth_cpu_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 27:  // eth_cpu_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_eth_cpu_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR eth cpu fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 28:  // ebuf_p0_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_ebuf_p0_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 29:  // ebuf_p0_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_ebuf_p0_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 30:  // ebuf_p0_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_ebuf_p0_fifo2_mem0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 31:  // ebuf_p0_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_ebuf_p0_fifo2_mem1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO,
                     "PGR (pkt-gen) ebuf p0 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p0 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_pgr_err_handle2(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)subdev_id;

  LOG_TRACE("%s dev %d int_addr 0x%x int_status_val 0x%x en_hi 0x%x en_lo 0x%x",
            __func__,
            dev,
            intr_address,
            intr_status_val,
            enable_hi_addr,
            enable_lo_addr);

  uint32_t address = 0, data = 0;
  bf_dev_pipe_t phy_pipe = 0, pipe = 0;
  const pipe_mgr_intr_userdata_t *userdata_p = NULL;
  int bitpos = 0;
  uint32_t mem_err_addr;

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("pgr intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pipes[].pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4
  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      bf_dev_target_t dev_tgt = {dev, pipe};
      switch (bitpos) {
        case 0:  // ebuf_p1_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (uint32_t)((data & 0x7) << 2);
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = mem_err_addr | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 1:  // ebuf_p1_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p1_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 2:  // ebuf_p1_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = (data & 0x7) << 2;
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = mem_err_addr | ((data >> 30) & 0x3);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 3:  // ebuf_p1_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p1_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO,
                     "PGR (pkt-gen) ebuf p1 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p1 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 4:  // ebuf_p2_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p2_fifo2_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 5:  // ebuf_p2_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p2_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 6:  // ebuf_p2_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p2_fifo2_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 7:  // ebuf_p2_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p2_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO,
                     "PGR (pkt-gen) ebuf p2 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p2 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 8:  // ebuf_p3_fifo2_mem0_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p3_fifo2_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem0 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 9:  // ebuf_p3_fifo2_mem1_mbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_mbe_log.pgr_data_fifo2_mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_mbe_log_pgr_data_fifo2_mbe_log_1_2_ebuf_p3_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem1 multi bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 10:  // ebuf_p3_fifo2_mem0_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p3_fifo2_mem0_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem0 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 11:  // ebuf_p3_fifo2_mem1_sbe
          /* Decode location */
          address =
              offsetof(tof2_reg,
                       pipes[phy_pipe]
                           .pardereg.pgstnreg.pgrreg.pgr_common
                           .pgr_data_fifo2_sbe_log.pgr_data_fifo2_sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr =
              getp_tof2_pgr_data_fifo2_sbe_log_pgr_data_fifo2_sbe_log_1_2_ebuf_p3_fifo2_mem1_addr(
                  &data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO,
                     "PGR (pkt-gen) ebuf p3 fifo2 mem1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR ebuf p3 fifo2 mem1 single bit err at addr %d ",
                    mem_err_addr);
          /* No further action */
          break;

        case 12:  // buffer0_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_buffer0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer0 multi bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 13:  // buffer0_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_buffer0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer0 single bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 14:  // buffer1_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_buffer1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer1 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer1 multi bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 15:  // buffer1_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_buffer1_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer1 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer1 single bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 16:  // buffer2_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_buffer2_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer2 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer2 multi bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 17:  // buffer2_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_buffer2_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer2 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer2 single bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 18:  // buffer3_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_buffer3_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer3 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer3 multi bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 19:  // buffer3_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_0_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_buffer3_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_BUFFER,
                     "PGR (pkt-gen) buffer3 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR buffer3 single bit err at addr %d ", mem_err_addr);
          /* Repair memory */
          pipe_mgr_pktgen_buffer_write_from_shadow(pipe_mgr_ctx->int_ses_hndl,
                                                   dev_tgt);
          break;

        case 20:  // phase0_mbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.mbe_log.mbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_mbe_log_mbe_log_1_2_phase0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_MULTI_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PHASE0,
                     "PGR (pkt-gen) phase0 multi bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR phase0 multi bit err at addr %d ", mem_err_addr);
          //! @note FIXME pipe_mgr_pkt_buffer_write_from_shadow
          break;

        case 21:  // phase0_sbe
          /* Decode location */
          address = offsetof(
              tof2_reg,
              pipes[phy_pipe]
                  .pardereg.pgstnreg.pgrreg.pgr_common.sbe_log.sbe_log_1_2);
          pipe_mgr_interrupt_read_register(dev, address, &data);
          mem_err_addr = getp_tof2_pgr_sbe_log_sbe_log_1_2_phase0_addr(&data);
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     mem_err_addr,
                     BF_ERR_TYPE_SINGLE_BIT_ECC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PHASE0,
                     "PGR (pkt-gen) phase0 single bit error at"
                     " addr %d",
                     mem_err_addr);
          LOG_TRACE("PGR phase0 single bit err at addr %d ", mem_err_addr);
          //! @note FIXME pipe_mgr_pkt_buffer_write_from_shadow
          break;

        case 22:  // app_evt_ovf0
        case 23:  // app_evt_ovf1
        case 24:  // app_evt_ovf2
        case 25:  // app_evt_ovf3
        case 26:  // app_evt_ovf4
        case 27:  // app_evt_ovf5
        case 28:  // app_evt_ovf6
        case 29:  // app_evt_ovf7
        case 30:  // app_evt_ovf8
        case 31:  // app_evt_ovf9
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_APP_EVT,
                     "PGR (pkt-gen) app event fifo overflow error%d",
                     (bitpos - 22));
          LOG_TRACE("PGR (pkt-gen) app event fifo overflow error%d",
                    (bitpos - 22));
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

static uint32_t pipe_mgr_tof2_pgr_err_handle3(bf_dev_id_t dev,
                                              bf_subdev_id_t subdev_id,
                                              uint32_t intr_address,
                                              uint32_t intr_status_val,
                                              uint32_t enable_hi_addr,
                                              uint32_t enable_lo_addr,
                                              void *userdata) {
  (void)enable_hi_addr;
  (void)subdev_id;

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

  /* Get device info */
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s : Failed to get device info, dev %d ", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  /* Get pipe from userdata */
  userdata_p = (pipe_mgr_intr_userdata_t *)userdata;
  pipe = userdata_p->pipe;

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

  LOG_TRACE("pgr intr (dev %d, pipe %d): Addr 0x%x, Status-Val 0x%x",
            dev,
            pipe,
            intr_address,
            intr_status_val);

  pipe_sess_hdl_t shdl = pipe_mgr_ctx->int_ses_hndl;
  pipe_status_t status = pipe_mgr_api_enter(shdl);
  if (status != PIPE_SUCCESS) return status;

  // pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4
  for (bitpos = 0; bitpos < 32; bitpos++) {
    if (intr_status_val & (0x1u << bitpos)) {
      switch (bitpos) {
        case 0:  // app_evt_ovf10
        case 1:  // app_evt_ovf11
        case 2:  // app_evt_ovf12
        case 3:  // app_evt_ovf13
        case 4:  // app_evt_ovf14
        case 5:  // app_evt_ovf15
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_APP_EVT,
                     "PGR (pkt-gen) app event fifo overflow error%d",
                     (bitpos + 10));
          LOG_TRACE("PGR (pkt-gen) app event fifo overflow error%d",
                    (bitpos + 10));
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 6:  // pfc_evt_ovf
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_NON_CORRECTABLE,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_PFC,
                     "PGR (pkt-gen) pfc event fifo overflow error");
          LOG_TRACE("PGR (pkt-gen) pfc event fifo overflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 7:  // ipb_chnl_seq_wrong
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_IPB_CHNL_SEQ,
                     "PGR (pkt-gen) ipb chnl sequence wrong");
          LOG_TRACE("PGR (pkt-gen) ipb chnl sequence wrong");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 8:  // eth_cpu_samechnl
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL,
                     "PGR (pkt-gen) eth cpu same chnl, configuration error");
          LOG_TRACE("PGR (pkt-gen) eth cpu same chnl, configuration error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 9:  // evt_tbc_samechnl
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_GENERIC,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL,
                     "PGR (pkt-gen) event tbc same chnl, configuration error");
          LOG_TRACE("PGR (pkt-gen) event tbc same chnl, configuration error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 10:  // tbc_fifo_ovf
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_TBC_FIFO,
                     "PGR (pkt-gen) tbc fifo overflow error");
          LOG_TRACE("PGR (pkt-gen) tbc fifo overflow error");
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 11:  // eth_cpu_ch0_fifo_ovf
        case 12:  // eth_cpu_ch1_fifo_ovf
        case 13:  // eth_cpu_ch2_fifo_ovf
        case 14:  // eth_cpu_ch3_fifo_ovf
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO,
                     "PGR (pkt-gen) eth cpu chnl%d fifo overflow error",
                     (bitpos - 11));
          LOG_TRACE("PGR (pkt-gen) eth cpu chnl%d fifo overflow error",
                    (bitpos - 11));
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        case 15:  // ebuf_port0_ch0_fifo_ovf
        case 16:  // ebuf_port0_ch1_fifo_ovf
        case 17:  // ebuf_port1_ch0_fifo_ovf
        case 18:  // ebuf_port1_ch1_fifo_ovf
        case 19:  // ebuf_port2_ch0_fifo_ovf
        case 20:  // ebuf_port2_ch1_fifo_ovf
        case 21:  // ebuf_port3_ch0_fifo_ovf
        case 22:  // ebuf_port3_ch1_fifo_ovf
          /* Report event */
          BF_ERR_EVT(BF_ERR_SEV_FATAL,
                     dev,
                     pipe,
                     0,
                     0,
                     BF_ERR_TYPE_OVERFLOW,
                     BF_ERR_BLK_PKTGEN,
                     BF_ERR_LOC_PKTGEN_ETH_PORT_FIFO,
                     "PGR (pkt-gen) ebuf port%d chnl%d fifo overflow error",
                     (bitpos - 15) / 2,
                     (bitpos - 15) % 2);
          LOG_TRACE("PGR (pkt-gen) ebuf port%d chnl%d fifo overflow error",
                    (bitpos - 15) / 2,
                    (bitpos - 15) % 2);
          /* Disable interrupt */
          pipe_mgr_interrupt_set_enable_val(
              dev, 0, enable_hi_addr, 0x1u << bitpos);
          break;

        default:
          break;
      }
    }
  }

  /* Clear the interrupt status */
  pipe_mgr_interrupt_write_register(dev, 0, intr_address, intr_status_val);

  pipe_mgr_api_exit(shdl);
  return PIPE_SUCCESS;
}

/* Register pktgen interrupt handlers */
pipe_status_t pipe_mgr_tof2_register_pgr_interrupt_notifs(
    rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int ret = 0;

  LOG_TRACE("Pipe-mgr Registering for pgr interrupts");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_0_4),
        &pipe_mgr_tof2_pgr_err_handle0,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_1_4),
        &pipe_mgr_tof2_pgr_err_handle1,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_2_4),
        &pipe_mgr_tof2_pgr_err_handle2,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);

    ret = lld_int_register_cb(
        dev,
        0,
        offsetof(
            tof2_reg,
            pipes[phy_pipe]
                .pardereg.pgstnreg.pgrreg.pgr_common.intr_stat.intr_stat_3_4),
        &pipe_mgr_tof2_pgr_err_handle3,
        (void *)&(PIPE_INTR_CALLBACK_DATA(dev, pipe, 0, 0)));
    PIPE_MGR_ASSERT(ret != -1);
  }

  return PIPE_SUCCESS;
}

/* Enable/disable pktgen interrupts */
pipe_status_t pipe_mgr_tof2_pgr_interrupt_en_set(rmt_dev_info_t *dev_info,
                                                 bool enable) {
  uint32_t en_val = enable ? 0xFFFFFFFF : 0;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  pipe_bitmap_t pbm;
  pipe_status_t ret;

  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  for (bf_dev_pipe_t pipe = 0; pipe < dev_info->num_active_pipes; ++pipe) {
    PIPE_BITMAP_SET(&pbm, pipe);
  }

  LOG_TRACE(" Setting PGR Interrupt mode to %s ",
            enable ? "Enable" : "Disable");

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_0_4),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_1_4),
      en_val);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_2_4),
      en_val & 0x7FFFF);
  if (ret != PIPE_SUCCESS) return ret;

  ret = pipe_mgr_tof2_intr_reg_wr(
      dev_info,
      &pbm,
      shdl,
      offsetof(
          tof2_reg,
          pipes[0].pardereg.pgstnreg.pgrreg.pgr_common.intr_en0.intr_en0_3_4),
      en_val & 0xFFFFFFF8);
  if (ret != PIPE_SUCCESS) return ret;

  return PIPE_SUCCESS;
}
