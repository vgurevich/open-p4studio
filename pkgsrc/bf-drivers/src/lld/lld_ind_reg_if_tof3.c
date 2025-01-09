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
#else
#define assert()
#endif

#include <dvm/bf_drv_intf.h>
//#include <dvm/bf_dma_types.h>
//#include <lld/bf_dma_if.h>
#include <lld/bf_dev_if.h>
//#include "lld_memory_mapping.h"
//#include <lld/lld_dr_descriptors.h>
#include <lld/lld_reg_if.h>
//#include "lld_memory_mapping.h"
//#include <lld/lld_fault.h>
#include "lld.h"
#include "lld_map.h"
//#include "lld_log.h"
#include "lld_dev.h"
//#include <lld/lld_sku.h>
//#include <dru_sim/dru_sim.h>
#include <tof3_regs/tof3_reg_drv.h>

/** \brief lld_ind_write_tof3
 *         Write a 42b address using the indirect address registers
 *
 * \param dev_id  : dev_id #
 * \param ind_addr: 42b register/memory offset
 * \param data_hi : upper 64-bits to write
 * \param data_lo : lower 64-bits to write
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad dev_id
 *
 * note: dev_id has already been qualified in lld_ind_reg_if.c
 */
lld_err_t lld_ind_write_tof3(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint64_t ind_addr,
                             uint64_t data_hi,
                             uint64_t data_lo) {
  uint32_t data;
  uint32_t addr_reg_lo =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_addr_low_address;
  uint32_t addr_reg_hi =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_addr_high_address;
  uint32_t data_reg_lo =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_data00_address;
  uint32_t data_reg_hi =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_data11_address;

  data = (uint32_t)(ind_addr >> 32ull);
  lld_subdev_write_register(dev_id, subdev_id, addr_reg_hi, data);
  data = (uint32_t)(ind_addr & 0xffffffffull);
  lld_subdev_write_register(dev_id, subdev_id, addr_reg_lo, data);

  data = (uint32_t)(data_hi >> 32ull);
  lld_subdev_write_register(dev_id, subdev_id, data_reg_hi, data);
  data = (uint32_t)(data_hi & 0xffffffffull);
  lld_subdev_write_register(dev_id, subdev_id, data_reg_hi - 4, data);

  data = (uint32_t)(data_lo >> 32ull);
  lld_subdev_write_register(dev_id, subdev_id, data_reg_hi - 8, data);
  data = (uint32_t)(data_lo & 0xffffffffull);
  lld_subdev_write_register(
      dev_id, subdev_id, data_reg_lo, data);  // note: triggers write

  return LLD_OK;
}

/** \brief lld_ind_read_tof3
 *         Read a 42b address using the indirect address registers
 *
 * \param dev_id  : dev_id #
 * \param ind_addr: 42b register/memory offset
 * \param data_hi : (returned) upper 64-bits
 * \param data_lo : (returned) lower 64-bits
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad dev_id
 *
 * note: dev_id has already been qualified in lld_ind_reg_if.c
 */
lld_err_t lld_ind_read_tof3(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            uint64_t ind_addr,
                            uint64_t *data_hi,
                            uint64_t *data_lo) {
  uint32_t data;
  uint32_t data00;
  uint32_t data01;
  uint32_t data10;
  uint32_t data11;
  uint32_t addr_reg_lo =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_addr_low_address;
  uint32_t addr_reg_hi =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_addr_high_address;
  uint32_t data_reg_lo =
      tof3_reg_device_select_pcie_bar01_regs_cpu_ind_data00_address;

  data = (uint32_t)(ind_addr & 0xffffffffull);
  lld_subdev_write_register(dev_id, subdev_id, addr_reg_lo, data);
  data = (uint32_t)(ind_addr >> 32ull);
  lld_subdev_write_register(dev_id, subdev_id, addr_reg_hi, data);

  lld_subdev_read_register(
      dev_id, subdev_id, data_reg_lo + 0, &data00);  // note: triggers read
  lld_subdev_read_register(dev_id, subdev_id, data_reg_lo + 4, &data01);
  lld_subdev_read_register(dev_id, subdev_id, data_reg_lo + 8, &data10);
  lld_subdev_read_register(dev_id, subdev_id, data_reg_lo + 12, &data11);

  *data_hi = ((uint64_t)data11 << 32ull) | ((uint64_t)data10);
  *data_lo = ((uint64_t)data01 << 32ull) | ((uint64_t)data00);

  return 0;
}
