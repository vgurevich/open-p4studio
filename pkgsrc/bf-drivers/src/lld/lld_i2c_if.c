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
 * @file lld_i2c_if.c
 * @date
 *
 */

/**
 * @addtogroup lld-i2c-api
 * @{
 * This is a description of I2C APIs.
 */

#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <tofino_regs/tofino.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_gpio_if.h>
#include <lld/lld_reg_if.h>
#include "lld_dev.h"
#include "lld_log.h"

typedef struct {
  uint16_t period_reg[BF_IO_NUM_PIN_PAIR]; /* programmed value in register */
  /* i2c period on wire microsec, precomputed for performace reasons */
  uint16_t period_2B[BF_IO_NUM_PIN_PAIR];
  bf_i2c_mode_t mode[BF_IO_NUM_PIN_PAIR]; /* reg/statein/stateout modes */
} bf_i2c_setting_t;

/* this structure caches current I2C settings */
static bf_i2c_setting_t bf_i2c_setting[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];

static bf_status_t lld_i2c_reg_wr(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint32_t reg,
                                  uint32_t data) {
  if (lld_subdev_write_register(dev_id, subdev_id, reg, data) == LLD_OK) {
    return BF_SUCCESS;
  } else {
    return BF_HW_COMM_FAIL;
  }
}

static bf_status_t lld_i2c_reg_rd(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint32_t reg,
                                  uint32_t *data) {
  if (lld_subdev_read_register(dev_id, subdev_id, reg, data) == LLD_OK) {
    return BF_SUCCESS;
  } else {
    return BF_HW_COMM_FAIL;
  }
}

/**
 * @brief  bf_io_is_i2c_mode
 *  check if the configured mode is i2c
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 *
 * @return bool
 *   true if i2c mode
 *   false if not i2c mode
 *
 */
static bool bf_io_is_i2c_mode(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              bf_io_pin_pair_t pin_pair) {
  bf_io_mode_t mode;

  bf_io_get_mode(dev_id, subdev_id, pin_pair, &mode);
  if (mode != BF_IO_MODE_I2C) {
    return false;
  } else {
    return true;
  }
}

/**
 * @brief  bf_i2c_set_clk:
 *   set the i2c clock
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param clock_period: uint16_t
 *  clock period in multiples of 10ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_set_clk(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_io_pin_pair_t pin_pair,
                           uint16_t clock_period) {
  uint32_t offset;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!bf_io_is_i2c_mode(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE;
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_scl_freq);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  setp_gpio_pair_regs_i2c_scl_freq_i2c_prer(&data, clock_period);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  bf_i2c_setting[dev_id][subdev_id].period_reg[pin_pair] = clock_period;
  /* this is a convenience precomputed number used in i2c delays */
  bf_i2c_setting[dev_id][subdev_id].period_2B[pin_pair] =
      (clock_period * 16) / 100;
  return BF_SUCCESS;
}

/**
 * @brief  : bf_i2c_set_submode
 *   set the i2c submode
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param mode : enum
 *  i2c submode (reg, statein or stateout)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_set_submode(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_io_pin_pair_t pin_pair,
                               bf_i2c_mode_t mode) {
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  bf_i2c_setting[dev_id][subdev_id].mode[pin_pair] = mode;
  return BF_SUCCESS;
}

/**
 * @brief  : bf_i2c_get_submode
 *   set the i2c submode
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param mode: enum
 *  i2c submode (reg, statein or stateout)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_get_submode(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_io_pin_pair_t pin_pair,
                               bf_i2c_mode_t *mode) {
  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  *mode = bf_i2c_setting[dev_id][subdev_id].mode[pin_pair];
  return BF_SUCCESS;
}

/**
 * @brief  bf_is_i2c_submode_reg:
 *   check if i2c submode is set to register access
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 *
 * @return bool
 *   true if reg-i2c submode
 *   false otherwise
 *
 */
static bool bf_is_i2c_submode_reg(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_io_pin_pair_t pin_pair) {
  if (!bf_io_is_i2c_mode(dev_id, subdev_id, pin_pair)) {
    return false;
  }
  if (bf_i2c_setting[dev_id][subdev_id].mode[pin_pair] == BF_I2C_MODE_REG) {
    return true;
  } else {
    return false;
  }
}

/**
 * @brief  bf_is_i2c_submode_statein:
 *   check if i2c submode is set to statein access
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 *
 * @return bool
 *   true if statein-i2c submode
 *   false otherwise
 *
 */
static bool bf_is_i2c_submode_statein(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      bf_io_pin_pair_t pin_pair) {
  if (!bf_io_is_i2c_mode(dev_id, subdev_id, pin_pair)) {
    return false;
  }
  if (bf_i2c_setting[dev_id][subdev_id].mode[pin_pair] == BF_I2C_MODE_STATEIN) {
    return true;
  } else {
    return false;
  }
}

/**
 * @brief  bf_is_i2c_submode_stateout:
 *   check if i2c submode is set to stateout access
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 *
 * @return bool
 *   true if stateout-i2c submode
 *   false otherwise
 *
 */
static bool bf_is_i2c_submode_stateout(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       bf_io_pin_pair_t pin_pair) {
  if (!bf_io_is_i2c_mode(dev_id, subdev_id, pin_pair)) {
    return false;
  }
  if (bf_i2c_setting[dev_id][subdev_id].mode[pin_pair] ==
      BF_I2C_MODE_STATEOUT) {
    return true;
  } else {
    return false;
  }
}

/* make addr and (addr+ size) word aligned */
static void make_seg_word_aligned(uint32_t addr,
                                  uint32_t size,
                                  uint32_t *addr_aligned,
                                  uint32_t *size_aligned) {
  uint32_t temp;

  *addr_aligned = addr & ~3UL;
  temp = addr + size;
  if (temp & 0x3UL) {
    temp = temp & ~3UL;
    temp += 0x4;
  }
  *size_aligned = temp - *addr_aligned;
}

/**
 * @brief  bf_read_stateout_buf
 *   read i2c-stateout buffer
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param off: uint32_t
 *  offset into stateout buffer **MUST be multiple of 4 bytes
 * @param buf: uint8_t *
 *  buffer to read into
 * @param num_data_bytes: int
 *  number of bytes to read  ** MUST be multiple of 4 bytes
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_read_stateout_buf(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_io_pin_pair_t pin_pair,
                                 uint32_t off,
                                 uint8_t *buf,
                                 int num_data_bytes) {
#if 0
  uint32_t offset;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || !buf || !num_data_bytes ||
      ((off + num_data_bytes) > BF_IO_STATEOUT_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }

  /* check 4 byte alignment */
  if ((off & 0x3) || (num_data_bytes & 0x3)) {
    return BF_INVALID_ARG;
  }

  offset = GPIO_COMM_OFFSET(dev_id, pin_pair, stateout);
  offset += off;
  while (num_data_bytes) {
    lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
    /* copy 4 bytes */
    buf[0] = data;
    buf[1] = data >> 8;
    buf[2] = data >> 16;
    buf[3] = data >> 24;
    buf += 4;
    offset += 4;
    num_data_bytes -= 4;
  }
  return BF_SUCCESS;
#endif
  uint32_t reg_addr;
  uint32_t data;
  uint32_t off_aligned, st_buff_idx;
  uint32_t size_aligned;
  uint8_t st_buf[BF_IO_STATEOUT_BUF_SIZE]; /* allocate stateout_buf size */

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || !num_data_bytes ||
      ((off + num_data_bytes) > BF_IO_STATEOUT_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  /* make word aligned read into temporary buffer and then copy
   * appropriate bytes into buf
   */
  /* make word aligned */
  make_seg_word_aligned(
      off, (uint32_t)num_data_bytes, &off_aligned, &size_aligned);

  reg_addr = GPIO_COMM_OFFSET(dev_id, pin_pair, stateout);

  reg_addr += off_aligned;
  st_buff_idx = off_aligned;

  while (size_aligned) {
    lld_i2c_reg_rd(dev_id, subdev_id, reg_addr, &data);
    /* copy 4 bytes */
    st_buf[st_buff_idx++] = data;
    st_buf[st_buff_idx++] = data >> 8;
    st_buf[st_buff_idx++] = data >> 16;
    st_buf[st_buff_idx++] = data >> 24;
    reg_addr += 4;
    size_aligned -= 4;
  }

  memcpy(buf, &st_buf[off], num_data_bytes);
  return BF_SUCCESS;
}

/**
 * @brief  bf_write_stateout_buf
 *   write i2c-stateout buffer
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param off: uint32_t
 *  offset into stateout buffer **MUST be multiple of 4 bytes
 * @param buf: uint8_t *
 *  buffer containing data
 * @param num_data_bytes: int
 *  number of bytes to write ** MUST be multiple of 4 bytes
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_write_stateout_buf(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_io_pin_pair_t pin_pair,
                                  uint32_t off,
                                  uint8_t *buf,
                                  int num_data_bytes) {
#if 0
  uint32_t offset;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || !buf || !num_data_bytes ||
      ((off + num_data_bytes) > BF_IO_STATEOUT_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }

  /* check 4 byte alignment */
  if ((off & 0x3) || (num_data_bytes & 0x3)) {
    return BF_INVALID_ARG;
  }

  offset = GPIO_COMM_OFFSET(dev_id, pin_pair, stateout);
  offset += off;
  while (num_data_bytes) {
    /* copy 4 bytes */
    data = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
    buf += 4;
    offset += 4;
    num_data_bytes -= 4;
  }
#endif
  uint32_t reg_addr;
  uint32_t data;
  uint32_t off_aligned, st_buff_idx;
  uint32_t size_aligned;
  uint8_t st_buf[BF_IO_STATEOUT_BUF_SIZE]; /* allocate stateout_buf size */

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  /* make word aligned */
  make_seg_word_aligned(
      off, (uint32_t)num_data_bytes, &off_aligned, &size_aligned);

  /* firstly read the unaligned part of stateout buffer from the device */
  /* towards the starting part */
  if (off & 3UL) {
    bf_read_stateout_buf(
        dev_id, subdev_id, pin_pair, off_aligned, &st_buf[off_aligned], 4);
  }
  /* towards the end part */
  if ((off + num_data_bytes) & 3UL) {
    uint32_t temp_addr = off_aligned + size_aligned - 4;
    bf_read_stateout_buf(
        dev_id, subdev_id, pin_pair, temp_addr, &st_buf[temp_addr], 4);
  }

  /* now overwrite with the user suppplied buffer */
  memcpy(&st_buf[off], buf, num_data_bytes);

  reg_addr = GPIO_COMM_OFFSET(dev_id, pin_pair, stateout);
  reg_addr += off_aligned;
  st_buff_idx = off_aligned;

  while (size_aligned) {
    /* copy 4 bytes */
    data = st_buf[st_buff_idx++] & 0xff;
    data |= (st_buf[st_buff_idx++] & 0xff) << 8;
    data |= (st_buf[st_buff_idx++] & 0xff) << 16;
    data |= (st_buf[st_buff_idx++] & 0xff) << 24;

    lld_i2c_reg_wr(dev_id, subdev_id, reg_addr, data);
    reg_addr += 4;
    size_aligned -= 4;
  }

  /* firstly, read the non-word aligned part into the temp buffer */
  return BF_SUCCESS;
}

/**
 * @brief  bf_read_statein_buf
 *   read i2c-statein buffer
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param off: uint32_t
 *  offset into statein buffer **MUST be multiple of 4 bytes
 * @param buf: uint8_t *
 *  buffer to read into
 * @param num_data_bytes: int
 *  number of bytes to read  ** MUST be multiple of 4 bytes
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_read_statein_buf(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_io_pin_pair_t pin_pair,
                                uint32_t off,
                                uint8_t *buf,
                                int num_data_bytes) {
  uint32_t offset;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || !num_data_bytes ||
      ((off + num_data_bytes) > BF_IO_STATEIN_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  /* check 4 byte alignment */
  if ((off & 0x3) || (num_data_bytes & 0x3)) {
    return BF_INVALID_ARG;
  }

  offset = GPIO_COMM_OFFSET(dev_id, pin_pair, statein);
  offset += off;
  while (num_data_bytes) {
    lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
    /* copy 4 bytes */
    buf[0] = data;
    buf[1] = data >> 8;
    buf[2] = data >> 16;
    buf[3] = data >> 24;
    buf += 4;
    offset += 4;
    num_data_bytes -= 4;
  }
  return BF_SUCCESS;
}

/**
 * @brief  bf_write_statein_buf
 *   write i2c-statein buffer
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param off: uint32_t
 *  offset into statein buffer **MUST be multiple of 4 bytes
 * @param buf: uint8_t *
 *  buffer containing data
 * @param num_data_bytes: int
 *  number of bytes to write ** MUST be multiple of 4 bytes
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_write_statein_buf(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_io_pin_pair_t pin_pair,
                                 uint32_t off,
                                 uint8_t *buf,
                                 int num_data_bytes) {
  uint32_t offset;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || !num_data_bytes ||
      ((off + num_data_bytes) > BF_IO_STATEIN_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  /* check 4 byte alignment */
  if ((off & 0x3) || (num_data_bytes & 0x3)) {
    return BF_INVALID_ARG;
  }

  offset = GPIO_COMM_OFFSET(dev_id, pin_pair, statein);
  offset += off;
  while (num_data_bytes) {
    /* copy 4 bytes */
    data = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
    buf += 4;
    offset += 4;
    num_data_bytes -= 4;
  }
  return BF_SUCCESS;
}

/**
 * @brief  bf_i2c_issue_wr_reg
 *  issue an i2c write transaction
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param buf: uint8_t *
 *  buffer containing data bytes
 * @param num_data_bytes: int
 *  how many bytes (maximum 4) of buf should go on wire in i2c transaction
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_issue_wr_reg(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_io_pin_pair_t pin_pair,
                                uint8_t i2c_addr,
                                uint32_t reg_addr,
                                int num_addr_bytes,
                                uint8_t *buf,
                                int num_data_bytes) {
  uint32_t offset, i2c_cmd;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || (num_data_bytes > 4) || (num_addr_bytes > 4)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!num_data_bytes && !num_addr_bytes) {
    return BF_INVALID_ARG;
  }

  if (!bf_is_i2c_submode_reg(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  if (getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
    return BF_IN_USE; /* i2c operation is currently going on */
  }
  /* program address and data registers */
  if (num_addr_bytes) {
    offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_addr);
    data = reg_addr;
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  }

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_wdata);
  /* copy 4 bytes even if num_data_bytes < 4 */
  data = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);

  if (!num_data_bytes) {
    i2c_cmd = 0x2;
  } else if (!num_addr_bytes) {
    i2c_cmd = 0x3;
  } else {
    i2c_cmd = 0x0;
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  data = 0;
  setp_gpio_pair_regs_i2c_ctrl_i2c_datanum(&data, num_data_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data, 1);
  setp_gpio_pair_regs_i2c_ctrl_i2c_devaddr(&data, i2c_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_addrnum(&data, num_addr_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_cmd(&data, i2c_cmd);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief  bf_i2c_issue_rd_reg
 *  issue an i2c read transaction
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param num_data_bytes: int
 *  how many bytes (maximum 4) of buf should go on wire in i2c transaction
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_issue_rd_reg(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_io_pin_pair_t pin_pair,
                                uint8_t i2c_addr,
                                uint32_t reg_addr,
                                int num_addr_bytes,
                                int num_data_bytes) {
  uint32_t offset, i2c_cmd;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !num_data_bytes || (num_data_bytes > 4) || (num_addr_bytes > 4)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_is_i2c_submode_reg(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  if (getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
    return BF_IN_USE; /* i2c operation is currently going on */
  }
  /* program address and data registers */
  if (num_addr_bytes) {
    offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_addr);
    data = reg_addr;
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  }

  if (!num_addr_bytes) {
    i2c_cmd = 0x3;
  } else {
    i2c_cmd = 0x1;
  }
  data = 0;
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  setp_gpio_pair_regs_i2c_ctrl_i2c_datanum(&data, num_data_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data, 1);
  setp_gpio_pair_regs_i2c_ctrl_i2c_devaddr(&data, i2c_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_addrnum(&data, num_addr_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_cmd(&data, i2c_cmd);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_issue_stateout
 *  issue an i2c stateout transaction
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param sateout_addr: uint8_t
 *   offset, bytes from this offset (into) stateout buffer go out on wire
 * @param num_bytes: int
 *  how many bytes (max 256) should go on wire in i2c transaction
 *  statout_addr + num_bytes must not exceed stateout buffer size.
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_issue_stateout(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_io_pin_pair_t pin_pair,
                                  uint8_t i2c_addr,
                                  uint32_t reg_addr,
                                  int num_addr_bytes,
                                  uint8_t statout_addr,
                                  int num_bytes) {
  uint32_t offset, i2c_type;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !num_bytes || (num_addr_bytes > 4) ||
      ((statout_addr + num_bytes) > BF_IO_STATEOUT_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_is_i2c_submode_stateout(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  if (getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
    return BF_IN_USE; /* i2c operation is currently going on */
  }
  /* program address and data registers */
  if (num_addr_bytes) {
    offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_addr);
    data = reg_addr;
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  }

  i2c_type = 0x1; /* stateout type */
  data = 0;
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  setp_gpio_pair_regs_i2c_ctrl_i2c_stateaddr(&data, statout_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_datanum(&data, num_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data, 1);
  setp_gpio_pair_regs_i2c_ctrl_i2c_devaddr(&data, i2c_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_addrnum(&data, num_addr_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_type(&data, i2c_type);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_issue_stateout_blocking
 *  issue an i2c stateout blocking transaction
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param sateout_addr: uint8_t
 *   offset, bytes from this offset (into) stateout buffer go out on wire
 * @param num_bytes: int
 *  how many bytes (max 256) should go on wire in i2c transaction
 *  statout_addr + num_bytes must not exceed stateout buffer size.
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_issue_stateout_blocking(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           bf_io_pin_pair_t pin_pair,
                                           uint8_t i2c_addr,
                                           uint32_t reg_addr,
                                           int num_addr_bytes,
                                           uint8_t statout_addr,
                                           int num_bytes) {
  uint32_t offset, i2c_type;
  uint32_t data;
  int wait_cnt;
  bool complete;

  if (dev_id >= BF_MAX_DEV_COUNT || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !num_bytes || (num_addr_bytes > 4) ||
      ((statout_addr + num_bytes) > BF_IO_STATEOUT_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_is_i2c_submode_stateout(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  if (getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
    return BF_IN_USE; /* i2c operation is currently going on */
  }
  /* program address and data registers */
  if (num_addr_bytes) {
    offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_addr);
    data = reg_addr;
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  }

  i2c_type = 0x1; /* stateout type */
  data = 0;
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  setp_gpio_pair_regs_i2c_ctrl_i2c_stateaddr(&data, statout_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_datanum(&data, num_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data, 1);
  setp_gpio_pair_regs_i2c_ctrl_i2c_devaddr(&data, i2c_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_addrnum(&data, num_addr_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_type(&data, i2c_type);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);

  /* perform a busy wait with sleep */
  wait_cnt = 0;
  while (wait_cnt++ < 10) {
    bf_sys_usleep(100);
    if (bf_i2c_get_completion_status(dev_id, subdev_id, pin_pair, &complete) !=
        BF_SUCCESS) {
      return BF_INVALID_ARG;
    }
    if (!complete) {
      return BF_SUCCESS;
    }
  }

  return BF_IN_USE;
}

/**
 * @brief  bf_i2c_issue_statein
 *  issue an i2c statein read transaction(s)
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param satein_addr: uint8_t
 *   offset, data in read beginning at this offset into statein buffer
 * @param num_bits: int
 *  how many bits (max 256) should be read in i2c transaction
 *  statein_addr + num_bits must not exceed statein buffer size.
 * @param trigger_mode: uint32_t
 *  trigger mode : determine the trigger point of periodic statin i2c ops.
 *   -- 001b : rising edge
 *
 *   -- 010b : falling edge
 *
 *   -- 011b : any edge
 *
 *   -- 100b : high
 *
 *   -- 101b : low
 *
 *   -- 110b : timeout mode
 *
 *   - bit [11:8]: Trigger Pin Select (pins that are configured as GPIO type)
 * @param period: uint32_t
 *  this is a divider into i2c_basetime value and determines the timeout period
 *  if trigger_mode selects "timeout mode"
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_issue_statein(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_io_pin_pair_t pin_pair,
                                 uint8_t i2c_addr,
                                 uint32_t reg_addr,
                                 int num_addr_bytes,
                                 uint8_t statein_addr,
                                 int num_bits,
                                 uint32_t trigger_mode,
                                 uint32_t period) {
  uint32_t offset, i2c_type;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !num_bits || (num_addr_bytes > 4) ||
      ((statein_addr + (num_bits / 8)) > BF_IO_STATEIN_BUF_SIZE)) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_is_i2c_submode_statein(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }
  /* first disable statein */
  if (bf_i2c_statein_enable(dev_id, subdev_id, pin_pair, false) != BF_SUCCESS) {
    return BF_HW_COMM_FAIL;
  }

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_statein);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  setp_gpio_pair_regs_i2c_statein_statein_timeout(&data, period);
  setp_gpio_pair_regs_i2c_statein_statein_ctrl(&data, trigger_mode);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);

  /* program address and data registers */
  if (num_addr_bytes) {
    offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_addr);
    data = reg_addr;
    lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  }

  i2c_type = 0x2; /* statein type */
  data = 0;
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  setp_gpio_pair_regs_i2c_ctrl_i2c_stateaddr(&data, statein_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_datanum(&data, num_bits);
  setp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data, 1);
  setp_gpio_pair_regs_i2c_ctrl_i2c_devaddr(&data, i2c_addr);
  setp_gpio_pair_regs_i2c_ctrl_i2c_addrnum(&data, num_addr_bytes);
  setp_gpio_pair_regs_i2c_ctrl_i2c_type(&data, i2c_type);
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_statein_enable
 *  enable or disable statein i2c operations
 *  the ongoing statin would be completed enable of disable
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param en: bool
 *  true  to enable, false to disable
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_statein_enable(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_io_pin_pair_t pin_pair,
                                  bool en) {
  uint32_t offset, tmp;
  uint32_t data;
  bf_status_t status = BF_SUCCESS;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_is_i2c_submode_statein(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_statein);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  if (en) {
    setp_gpio_pair_regs_i2c_statein_statein_ctrl(&data, 6); /* to enable*/
  } else {
    setp_gpio_pair_regs_i2c_statein_statein_ctrl(&data, 7); /* to disable*/
  }
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  if (!en) {
    tmp = 100;
    status = BF_IN_USE; /* initialize */
    /* wait for ongoing i2c to be completed */
    while (tmp--) {
      lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
      if (!getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
        status = BF_SUCCESS;
        break;
      }
      bf_sys_usleep(1000); /* 1 ms */
    }
  }
  return status;
}

/**
 * @brief bf_i2c_get_completion_status
 *  returns the ccurrent i2c completion status
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param complete: bool
 *  true:complete, false: incomplete (on-going)
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_get_completion_status(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         bf_io_pin_pair_t pin_pair,
                                         bool *complete) {
  uint32_t offset;
  uint32_t data;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  *complete = getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data);
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_get_rd_data
 *  get the i2c_read data from a previously completed i2c read operation
 *
 * @param dev_id: int
 *  chip id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param buf: uint8_t *
 *  buffer to read into
 * @param num_bytes: int
 *  how many bytes (maximum 4) to read
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_get_rd_data(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_io_pin_pair_t pin_pair,
                               uint8_t *buf,
                               int num_bytes) {
  uint32_t offset;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || num_bytes > 4) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_rdata);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  while (num_bytes--) {
    *buf = data;
    buf++;
    data >>= 8;
  }
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_get_statein_data
 *  get the i2c_statein data from the last i2c statein read operation
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param buf: uint8_t *
 *  buffer to read into
 * @param num_bytes: int
 *  how many bytes (maximum 32) to read
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_get_statein_data(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id,
                                    bf_io_pin_pair_t pin_pair,
                                    uint8_t *buf,
                                    int num_bytes) {
  uint32_t offset, i;
  uint32_t data;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT) ||
      !buf || num_bytes > BF_IO_STATEIN_BUF_SIZE) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  offset = GPIO_COMM_OFFSET(dev_id, pin_pair, statein);
  while (num_bytes) {
    i = 0;
    lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
    while (num_bytes && (i < 4)) {
      *buf = data;
      data >>= 8;
      num_bytes--;
      buf++;
      i++;
    }
  }
  return BF_SUCCESS;
}

/**
 * @brief bf_i2c_probe_blocking
 *  probes an i2c device to determine its presence on the bus
 *  could block for upto approxomately 24 bytes of i2c wire delay
 *
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: nt8_t
 *  7bit i2c device address
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_probe_blocking(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_io_pin_pair_t pin_pair,
                                  uint8_t i2c_addr) {
  uint8_t chr;

  return (bf_i2c_rd_reg_blocking(
      dev_id, subdev_id, pin_pair, i2c_addr, 0, 0, &chr, 1));
}

/**
 * @brief bf_i2c_rd_blocking
 *  i2c read operation without specifying any endpoint device register address
 *  could block for upto approxomately 24 bytes of i2c wire delay
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param buf: uint8_t*
 *  buffer to read into
 * @param num_data_bytes: int
 *  how many bytes (maximum 4) of buf should go on wire in i2c transaction
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_rd_blocking(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_io_pin_pair_t pin_pair,
                               uint8_t i2c_addr,
                               uint8_t *buf,
                               int num_data_bytes) {
  return (bf_i2c_rd_reg_blocking(
      dev_id, subdev_id, pin_pair, i2c_addr, 0, 0, buf, num_data_bytes));
}

/**
 * @brief bf_i2c_wr_reg_blocking
 *  issue an i2c write transaction
 *  could block for upto approxomately 20 bytes of i2c wire delay
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param buf: uint8_t*
 *  buffer containing data bytes
 * @param num_data_bytes: int
 *  how many bytes (maximum 4) of buf should go on wire in i2c transaction
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_wr_reg_blocking(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   bf_io_pin_pair_t pin_pair,
                                   uint8_t i2c_addr,
                                   uint32_t reg_addr,
                                   int num_addr_bytes,
                                   uint8_t *buf,
                                   int num_data_bytes) {
  bf_status_t status;
  int wait_cnt;
  bool complete;

  status = bf_i2c_issue_wr_reg(dev_id,
                               subdev_id,
                               pin_pair,
                               i2c_addr,
                               reg_addr,
                               num_addr_bytes,
                               buf,
                               num_data_bytes);
  if (status != BF_SUCCESS) {
    return status;
  }
  /* perform a busy wait with sleep */
  wait_cnt = 0;
  while (wait_cnt++ < 10) {
    /* sleep for 2 byte i2c wire delay */
    bf_sys_usleep(bf_i2c_setting[dev_id][subdev_id].period_2B[pin_pair]);
    status =
        bf_i2c_get_completion_status(dev_id, subdev_id, pin_pair, &complete);
    if (status != BF_SUCCESS) {
      return status;
    }
    if (!complete) {
      return BF_SUCCESS;
    }
  }
  return BF_HW_COMM_FAIL;
}

/**
 * @brief bf_i2c_rd_reg_blocking
 *  i2c read transaction
 *  could block for upto approxomately 24 bytes of i2c wire delay
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 *  which of the 12 pin-pairs to configure
 * @param i2c_addr: uint8_t
 *   7 bit i2c address (most significant bit == 0)
 * @param reg_addr: uint32_t
 *  upto 4 bytes of register address within the endpoint device
 *  bits 0-7, 8-15, 16-23, 24-31 go on wire in that order.
 *  if (num_addr_bytes < 4), lower order bytes go on the wire.
 * @param num_addr_bytes: int
 *  how many bytes (maximum 4) of reg_addr should go on wire in i2c transaction
 * @param buf: uint8_t*
 *  buffer to read into
 * @param num_data_bytes: int
 *  how many bytes (maximum 4) of buf should go on wire in i2c transaction
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_rd_reg_blocking(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   bf_io_pin_pair_t pin_pair,
                                   uint8_t i2c_addr,
                                   uint32_t reg_addr,
                                   int num_addr_bytes,
                                   uint8_t *buf,
                                   int num_data_bytes) {
  bf_status_t status;
  int wait_cnt;
  bool complete;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }
  status = bf_i2c_issue_rd_reg(dev_id,
                               subdev_id,
                               pin_pair,
                               i2c_addr,
                               reg_addr,
                               num_addr_bytes,
                               num_data_bytes);
  if (status != BF_SUCCESS) {
    return status;
  }
  /* perform a busy wait with sleep */
  wait_cnt = 0;
  while (wait_cnt++ < 10) {
    /* sleep for 2 byte i2c wire delay */
    bf_sys_usleep(bf_i2c_setting[dev_id][subdev_id].period_2B[pin_pair]);
    status =
        bf_i2c_get_completion_status(dev_id, subdev_id, pin_pair, &complete);
    if (status != BF_SUCCESS) {
      return status;
    }
    if (!complete) {
      status =
          bf_i2c_get_rd_data(dev_id, subdev_id, pin_pair, buf, num_data_bytes);
      return status;
    }
  }
  return BF_HW_COMM_FAIL;
}

/**
 * @brief bf_i2c_reset
 *  perform i2c bus reset
 *  could block for upto approxomately 100ms
 *
 * @param dev_id: int
 *  chip id
 * @param subdev_id: int
 *  subdevice id within dev_id
 * @param pin_pair: enum
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_i2c_reset(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bf_io_pin_pair_t pin_pair) {
  uint32_t offset, tmp;
  uint32_t data;
  bf_status_t status = BF_SUCCESS;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (pin_pair > BF_TOF3_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (pin_pair > BF_IO_PIN_PAIR_MAX) {
      return BF_INVALID_ARG;
    }
  }

  if (!bf_io_is_i2c_mode(dev_id, subdev_id, pin_pair)) {
    return BF_IN_USE; /* i2c interface is being used for some other mode */
  }
  /* disable statein, just in case it is used */
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_statein);
  lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
  setp_gpio_pair_regs_i2c_statein_statein_ctrl(&data, 7); /* to disable*/
  lld_i2c_reg_wr(dev_id, subdev_id, offset, data);

  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, i2c_ctrl);
  tmp = 100;
  /* wait for on going i2c to be completed */
  while (tmp--) {
    lld_i2c_reg_rd(dev_id, subdev_id, offset, &data);
    if (!getp_gpio_pair_regs_i2c_ctrl_i2c_exec(&data)) {
      break;
    }
    bf_sys_usleep(1000); /* 1 ms */
  }
  if (!tmp) {
    /* i2c could not be completed */
    printf("error in i2c completion chip %d id %d\n", dev_id, pin_pair);
    /* go ahead with reset, anyway */
  }
  /* reset i2c by toggling the gpio_mode */
  bf_io_set_mode_gpio(
      dev_id, subdev_id, pin_pair, BF_GPIO_OUT_L, BF_GPIO_OUT_L);
  bf_sys_usleep(1000);
  status = bf_io_set_mode_i2c(dev_id, subdev_id, pin_pair);
  return status;
}
/**
 * @}
 */
