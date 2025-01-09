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


/*!
 * @file lld_tof2_ts.c
 * @date
 *
 */

/**
 * @addtogroup lld-ts-api
 * @{
 * This is a description of Timestamp APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>
#include <string.h>
#include <time.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_err.h>
#include <lld/bf_ts_if.h>
#include "lld_tof2_ts.h"
#include <tof2_regs/tof2_reg_drv.h>

#define TS_TOF2_MBC_MBUS_REG_OFFSET(reg) \
  (offsetof(tof2_reg, device_select.mbc.mbc_mbus.reg))

static bf_status_t lld_ts_reg_wr(bf_dev_id_t dev_id,
                                 uint32_t reg,
                                 uint32_t data) {
  if (lld_write_register(dev_id, reg, data) != LLD_OK) {
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

static bf_status_t lld_ts_reg_rd(bf_dev_id_t dev_id,
                                 uint32_t reg,
                                 uint32_t *data) {
  if (lld_read_register(dev_id, reg, data) != LLD_OK) {
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_state_set
 *  Enable global timestamp function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable or disable timestamp function
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_state_set(bf_dev_id_t dev_id, bool enable) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ctrl);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  setp_tof2_mbus_ctrl_ts_en(&data, enable);
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_state_get
 *  Get the enable state of global timestamp function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable state of timestamp function
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_state_get(bf_dev_id_t dev_id, bool *enable) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ctrl);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data = getp_tof2_mbus_ctrl_ts_en(&data);
  *enable = (data ? true : false);
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_value_set
 *  Configure the global timestamp counter
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_value_set(bf_dev_id_t dev_id,
                                            uint64_t global_ts_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_set.global_ts_set_0_2);
  setp_tof2_mbus_global_ts_set_global_ts_set_0_2_count_31_0(
      &data, (uint32_t)(global_ts_ns));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_set.global_ts_set_1_2);
  setp_tof2_mbus_global_ts_set_global_ts_set_1_2_count_47_32(
      &data, (uint32_t)(global_ts_ns >> 32));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_value_get
 *  Get the global timestamp counter
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_value_get(bf_dev_id_t dev_id,
                                            uint64_t *global_ts_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  /* Trigger a TS capture */
  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ts_capture);

  setp_tof2_mbus_ts_capture_capture(&data, 1);
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_set.global_ts_set_0_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data = getp_tof2_mbus_global_ts_set_global_ts_set_0_2_count_31_0(&data);
  *global_ts_ns = (uint64_t)data & 0xFFFFFFFFULL;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_set.global_ts_set_1_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data = getp_tof2_mbus_global_ts_set_global_ts_set_1_2_count_47_32(&data);
  *global_ts_ns |= ((uint64_t)data << 32);

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_inc_value_set
 *  Configure the global timestamp inc value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int32
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_inc_value_set(bf_dev_id_t dev_id,
                                                uint32_t global_inc_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_inc_value);
  setp_tof2_mbus_global_ts_inc_value_count(&data, global_inc_ns);
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_inc_value_get
 *  Get the global timestamp  increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int32
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_inc_value_get(bf_dev_id_t dev_id,
                                                uint32_t *global_inc_ns) {
  uint32_t address = 0;
  uint32_t data = 0;
  *global_inc_ns = 0UL;

  /* read lower address first */
  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_inc_value);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  *global_inc_ns = getp_tof2_mbus_global_ts_inc_value_count(&data);
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_increment_one_time_set
 *  Increment Global timestamp register one-time
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_inc_time_ns:  int64
 *   Value to increment Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_increment_one_time_set(
    bf_dev_id_t dev_id, uint64_t global_ts_inc_time_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_inc.global_ts_inc_0_2);
  setp_tof2_mbus_global_ts_inc_global_ts_inc_0_2_count_31_0(
      &data, (uint32_t)(global_ts_inc_time_ns));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_inc.global_ts_inc_1_2);
  setp_tof2_mbus_global_ts_inc_global_ts_inc_1_2_count_47_32(
      &data, (uint32_t)(global_ts_inc_time_ns >> 32));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_ts_offset_set
 *  Set global timestamp offset value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_offset_ns:  int64
 *   Value to set for Global timestamp offset value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_offset_set(bf_dev_id_t dev_id,
                                             uint64_t global_ts_offset_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      global_ts_offset_value.global_ts_offset_value_0_2);
  setp_tof2_mbus_global_ts_offset_value_global_ts_offset_value_0_2_count_31_0(
      &data, (uint32_t)(global_ts_offset_ns));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      global_ts_offset_value.global_ts_offset_value_1_2);
  setp_tof2_mbus_global_ts_offset_value_global_ts_offset_value_1_2_count_47_32(
      &data, (uint32_t)(global_ts_offset_ns >> 32));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof_ts_global_ts_offset_get
 *  Get global timestamp offset value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_offset_ns:  int64
 *   Value to set for Global timestamp offset value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_ts_offset_get(bf_dev_id_t dev_id,
                                             uint64_t *global_ts_offset_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      global_ts_offset_value.global_ts_offset_value_0_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data =
      getp_tof2_mbus_global_ts_offset_value_global_ts_offset_value_0_2_count_31_0(
          &data);
  *global_ts_offset_ns = (uint64_t)data & 0xFFFFFFFFULL;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      global_ts_offset_value.global_ts_offset_value_1_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data =
      getp_tof2_mbus_global_ts_offset_value_global_ts_offset_value_1_2_count_47_32(
          &data);
  *global_ts_offset_ns |= ((uint64_t)data << 32);

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_global_baresync_ts_get
 *  Trigger and retrieve both global timestamp value and baresync timestamp
 *value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64 ptr
 *   Pointer to buffer to store global timestamp value in ns
 *
 * @param baresync_ts_ns:  int64 ptr
 *   Pointer to buffer to store baresync timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_global_baresync_ts_get(bf_dev_id_t dev_id,
                                               uint64_t *global_ts_ns,
                                               uint64_t *baresync_ts_ns) {
  uint32_t address = 0;
  uint32_t data = 0;
  uint64_t tmp = 0;

  /* Trigger a TS capture */
  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ts_capture);

  setp_tof2_mbus_ts_capture_capture(&data, 1);
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  /* Read Global time stamp */
  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_value.global_ts_value_0_2);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  *global_ts_ns =
      getp_tof2_mbus_global_ts_value_global_ts_value_0_2_count_31_0(&data);

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(global_ts_value.global_ts_value_1_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  tmp = getp_tof2_mbus_global_ts_value_global_ts_value_1_2_count_47_32(&data);

  *global_ts_ns |= (uint64_t)(tmp << 32);

  /* Read Baresync time stamp */
  address =
      TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_value.baresync_ts_value_0_2);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  *baresync_ts_ns =
      getp_tof2_mbus_baresync_ts_value_baresync_ts_value_0_2_count_31_0(&data);

  address =
      TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_value.baresync_ts_value_1_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  tmp =
      getp_tof2_mbus_baresync_ts_value_baresync_ts_value_1_2_count_47_32(&data);
  *baresync_ts_ns |= (uint64_t)(tmp << 32);

  return BF_SUCCESS;
}

/* Baresync related API */

/**
 * @brief lld_tof2_ts_baresync_state_set
 *  Enable baresync function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable or disable baresync function
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_state_set(bf_dev_id_t dev_id,
                                           uint32_t reset_count_threshold,
                                           uint32_t debounce_count,
                                           bool enable) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ctrl);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  setp_tof2_mbus_ctrl_baresync_load_en(&data, enable);
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = offsetof(tof2_reg, device_select.misc_regs.baresync_ctrl);
  reset_count_threshold &= 0x00FFFFFFUL;
  debounce_count &= 0x0000007FUL;
  data = (reset_count_threshold | (debounce_count << 24));
  if (enable) {
    data |= (1 << 31);
  }
  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof_ts_baresync_state_get
 *  Get config state of baresync function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable state of baresync function
 *
 * @param reset_count_threshold: uint32_t
 *   reset count threshold value
 *
 * @param debounce_count: uint32_t
 *   debounce count value
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_state_get(bf_dev_id_t dev_id,
                                           uint32_t *reset_count_threshold,
                                           uint32_t *debounce_count,
                                           bool *enable) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(ctrl);

  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data = getp_tof2_mbus_ctrl_baresync_load_en(&data);
  *enable = (data ? true : false);

  address = offsetof(tof2_reg, device_select.misc_regs.baresync_ctrl);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  *reset_count_threshold = (data & 0x00FFFFFFUL);
  *debounce_count = ((data >> 24) & 0x0000007FUL);
  if (*enable) {
    data = data & 0x80000000UL;
    if (data == 0) {
      *enable = false;
    }
  }
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_baresync_reset_value_set
 *  Set the value to be written into baresync timestamp register on reset
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_time_ns:  int64
 *   Value to be written into baresync register on reset in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_reset_value_set(bf_dev_id_t dev_id,
                                                 uint64_t baresync_time_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      baresync_ts_set_value.baresync_ts_set_value_0_2);
  setp_tof2_mbus_baresync_ts_set_value_baresync_ts_set_value_0_2_count_31_0(
      &data, (uint32_t)(baresync_time_ns));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      baresync_ts_set_value.baresync_ts_set_value_1_2);
  setp_tof2_mbus_baresync_ts_set_value_baresync_ts_set_value_1_2_count_47_32(
      &data, (uint32_t)(baresync_time_ns >> 32));

  if (lld_ts_reg_wr(dev_id, address, data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_baresync_reset_value_get
 *  Get the value of the baresync timestamp register on reset
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_time_ns:  int64
 *   Value of the baresync register on reset in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_reset_value_get(bf_dev_id_t dev_id,
                                                 uint64_t *baresync_time_ns) {
  uint32_t address = 0;
  uint32_t data = 0;

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      baresync_ts_set_value.baresync_ts_set_value_0_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data =
      getp_tof2_mbus_baresync_ts_set_value_baresync_ts_set_value_0_2_count_31_0(
          &data);

  *baresync_time_ns = ((uint64_t)data & 0xFFFFFFFFUL);
  address = TS_TOF2_MBC_MBUS_REG_OFFSET(
      baresync_ts_set_value.baresync_ts_set_value_1_2);
  if (lld_ts_reg_rd(dev_id, address, &data) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  data =
      getp_tof2_mbus_baresync_ts_set_value_baresync_ts_set_value_1_2_count_47_32(
          &data);
  *baresync_time_ns |= ((uint64_t)data << 32);

  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_baresync_increment_set
 *  Set the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @param baresync_inc_time_fract_den:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_increment_set(
    bf_dev_id_t dev_id,
    uint32_t baresync_inc_time_ns,
    uint32_t baresync_inc_time_fract_ns,
    uint32_t baresync_inc_time_fract_den) {
  uint32_t address = 0;
  uint32_t inc_0_3[3] = {0, 0, 0};

  setp_tof2_mbus_baresync_ts_inc_inc_ns(inc_0_3, baresync_inc_time_ns);
  setp_tof2_mbus_baresync_ts_inc_fr_den(inc_0_3, baresync_inc_time_fract_den);
  setp_tof2_mbus_baresync_ts_inc_fr_num(inc_0_3, baresync_inc_time_fract_ns);

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_0_3);
  if (lld_ts_reg_wr(dev_id, address, inc_0_3[0]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_1_3);
  if (lld_ts_reg_wr(dev_id, address, inc_0_3[1]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_2_3);
  if (lld_ts_reg_wr(dev_id, address, inc_0_3[2]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

/**
 * @brief lld_tof2_ts_baresync_increment_get
 *  Get the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @param baresync_inc_time_fract_den:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t lld_tof2_ts_baresync_increment_get(
    bf_dev_id_t dev_id,
    uint32_t *baresync_inc_time_ns,
    uint32_t *baresync_inc_time_fract_ns,
    uint32_t *baresync_inc_time_fract_den) {
  uint32_t address = 0;
  uint32_t inc_0_3[3] = {0, 0, 0};

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_0_3);
  if (lld_ts_reg_rd(dev_id, address, &inc_0_3[0]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_1_3);
  if (lld_ts_reg_rd(dev_id, address, &inc_0_3[1]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  address = TS_TOF2_MBC_MBUS_REG_OFFSET(baresync_ts_inc.baresync_ts_inc_2_3);
  if (lld_ts_reg_rd(dev_id, address, &inc_0_3[2]) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  *baresync_inc_time_ns = getp_tof2_mbus_baresync_ts_inc_inc_ns(inc_0_3);
  *baresync_inc_time_fract_den = getp_tof2_mbus_baresync_ts_inc_fr_den(inc_0_3);
  *baresync_inc_time_fract_ns = getp_tof2_mbus_baresync_ts_inc_fr_num(inc_0_3);
  return BF_SUCCESS;
}

/**
 * @}
 */
