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
 * @file lld_spi_if.c
 * @date
 *
 */

/**
 * @addtogroup lld-spi-api
 * @{
 * This is a description of SPI APIs.
 */

#include <stdio.h>
#include <stdint.h>

#include <dvm/bf_drv_intf.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_spi_if.h>
#include <lld/lld_err.h>
#include <tofino_regs/tofino.h>
#include "lld_dev.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>

static bf_status_t lld_spi_reg_wr(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint32_t reg,
                                  uint32_t data) {
  if (lld_subdev_write_register(dev_id, subdev_id, reg, data) == LLD_OK) {
    return BF_SUCCESS;
  } else {
    return BF_HW_COMM_FAIL;
  }
}

static bf_status_t lld_spi_reg_rd(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  uint32_t reg,
                                  uint32_t *data) {
  if (lld_subdev_read_register(dev_id, subdev_id, reg, data) == LLD_OK) {
    return BF_SUCCESS;
  } else {
    return BF_HW_COMM_FAIL;
  }
}

static uint32_t swap32(uint32_t a) {
  uint32_t b;
  b = ((a & 0xff) << 24) | ((a & 0xff00) << 8) | ((a & 0xff0000) >> 8) |
      ((a & 0xff000000) >> 24);
  return b;
}

/**
 * @brief  lld_spi_init
 *  initialize the SPI interface
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_init(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  /* perform any chip non-default initialization here */
  /* e.g., read idcode and determine the size of spi eeprom */
  (void)dev_id;
  (void)subdev_id;
  return BF_SUCCESS;
}

/**
 * @brief  lld_spi_wr_
 *  triggers SPI write followed by an SPI read transaction
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param spi_code: uint8_t
 *  spi_code . This is always transmitted on SPI bus even if w_size or
 *             r_size is zero.
 *
 * @param w_buf: uint8_t *
 *  buf to transmit on SPI bus
 *
 * @param wr_size: int
 *  size of w_buf (zero is ok)
 *
 * @param rd_size: int
 *  number of bytes to expect to read on SPI bus (after writing)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_wr(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       uint8_t spi_code,
                       uint8_t *w_buf,
                       int w_size,
                       int r_size) {
  uint32_t out_data0;
  uint32_t out_data1 = 0;
  uint32_t offset, bytes_to_wr;
  uint32_t spi_cmd;
  uint32_t i;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT ||
      w_size >= BF_SPI_MAX_OUTDATA || r_size > BF_SPI_MAX_INDATA) {
    return BF_INVALID_ARG;
  }

  bytes_to_wr = w_size + 1;
  out_data0 = ((uint32_t)(spi_code)&0xFF);
  i = 1;
  /* prepeare 4 bytes in out_data0 word */
  while (w_size && i < 4) {
    out_data0 |= (((uint32_t)*w_buf & 0xFF) << (i * 8));
    w_size--;
    i++;
    w_buf++;
  }
  /* prepeare 4 bytes in out_data1 word */
  i = 0;
  while (w_size && i < 4) {
    out_data1 |= (((uint32_t)*w_buf & 0xFF) << (i * 8));
    w_size--;
    i++;
    w_buf++;
  }

  if (lld_dev_is_tofino(dev_id)) {
    offset = offsetof(Tofino, device_select.misc_regs.spi_outdata0);
    lld_spi_reg_wr(dev_id, 0, offset, out_data0);
    offset = offsetof(Tofino, device_select.misc_regs.spi_outdata1);
    lld_spi_reg_wr(dev_id, 0, offset, out_data1);
    spi_cmd = bytes_to_wr | 0x80;
    spi_cmd |= (r_size << 4);
    /* printf("spi_wr out0 %08x out1 %08x cmd %08x\n", out_data0, out_data1,
     * spi_cmd); */
    offset = offsetof(Tofino, device_select.misc_regs.spi_command);
    lld_spi_reg_wr(dev_id, 0, offset, spi_cmd);
  } else if (lld_dev_is_tof2(dev_id)) {
    offset = offsetof(tof2_reg, device_select.misc_regs.spi_outdata0);
    lld_spi_reg_wr(dev_id, 0, offset, out_data0);
    offset = offsetof(tof2_reg, device_select.misc_regs.spi_outdata1);
    lld_spi_reg_wr(dev_id, 0, offset, out_data1);
    spi_cmd = bytes_to_wr | 0x80;
    spi_cmd |= (r_size << 4);
    /* printf("spi_wr out0 %08x out1 %08x cmd %08x\n", out_data0, out_data1,
     * spi_cmd); */
    offset = offsetof(tof2_reg, device_select.misc_regs.spi_command);
    lld_spi_reg_wr(dev_id, 0, offset, spi_cmd);
  } else if (lld_dev_is_tof3(dev_id)) {
    offset =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.spi_outdata0);
    lld_spi_reg_wr(dev_id, subdev_id, offset, out_data0);
    offset =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.spi_outdata1);
    lld_spi_reg_wr(dev_id, subdev_id, offset, out_data1);
    spi_cmd = bytes_to_wr | 0x80;
    spi_cmd |= (r_size << 4);
    /* printf("spi_wr out0 %08x out1 %08x cmd %08x\n", out_data0, out_data1,
     * spi_cmd); */
    offset =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.spi_command);
    lld_spi_reg_wr(dev_id, subdev_id, offset, spi_cmd);
  } else {
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

/**
 * @brief  lld_spi_get_idcode
 *  returns the spi idcode read during bootup
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param spi_idcode: uint32_t *
 *  spi ID code
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_get_idcode(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               uint32_t *spi_idcode) {
  uint32_t offset;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (lld_dev_is_tofino(dev_id)) {
    offset = offsetof(Tofino, device_select.misc_regs.spi_idcode);
  } else if (lld_dev_is_tof2(dev_id)) {
    offset = offsetof(tof2_reg, device_select.misc_regs.spi_idcode);
  } else if (lld_dev_is_tof3(dev_id)) {
    offset =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.spi_idcode);
  } else {
    return BF_INVALID_ARG;
  }
  return (lld_spi_reg_rd(dev_id, subdev_id, offset, spi_idcode));
}

/**
 * @brief  lld_spi_get_rd_data
 *  returns the spi read data during  the last SPI read operation
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param spi_rd_data: uint32_t *
 *  spi read data
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_get_rd_data(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                uint32_t *spi_rd_data) {
  uint32_t offset;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (lld_dev_is_tofino(dev_id)) {
    offset = offsetof(Tofino, device_select.misc_regs.spi_indata);
  } else if (lld_dev_is_tof2(dev_id)) {
    offset = offsetof(tof2_reg, device_select.misc_regs.spi_indata);
  } else if (lld_dev_is_tof3(dev_id)) {
    offset =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.spi_indata);
  } else {
    return BF_INVALID_ARG;
  }
  return (lld_spi_reg_rd(dev_id, subdev_id, offset, spi_rd_data));
}

/**
 * @brief lld_spi_eeprom_wr_enable
 *  enable the write enable latch on the eeprom on SPI bus
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param en: int
 *  enable = 1, disable = 0
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
static bf_status_t lld_spi_eeprom_wr_enable(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id,
                                            int en) {
  uint8_t spi_code = en ? BF_SPI_OPCODE_WR_EN : BF_SPI_OPCODE_WR_DI;
  /* w_size = r_size = 0 */
  return (lld_spi_wr(dev_id, subdev_id, spi_code, NULL, 0, 0));
}

#if 0
/* Following APIs are specific to SPI eeprom and not to Tofino. Thay are tested.  We leave it outside the compilation, but available for future use. Care must be taken in using the API to write the status as it might render the eeprom device write  protected forever */

/**
 * @brief lld_spi_eeprom_wr_status
 *  send WRSR instruction to the SPI eeprom
 *
 * @param dev_id: int
 *  chip id
 *
 * @param status: uint8_t
 *  status to be written into the eeprom status register
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_eeprom_wr_status(bf_dev_id_t dev_id, uint8_t status) {
  uint8_t spi_code = BF_SPI_OPCODE_WRSR,rd_status;
  bf_status_t bf_sts = BF_SUCCESS;

  bf_sts |= lld_spi_eeprom_wr_enable(dev_id , 1);
  bf_sts |= lld_spi_wr(dev_id, spi_code, &status, 1, 0);
  bf_sts |= lld_spi_eeprom_rd_status(dev_id, &rd_status); // remove
  bf_sts |= lld_spi_eeprom_wr_enable(dev_id , 0);
  return bf_sts;
}

#if 0
/* Following APIs are specific to SPI eeprom and not to Tofino. Thay are tested.  We leave it outside the compilation, but available for future use. Care must be taken in using the API to write the status as it might render the eeprom device write  protected forever */

/**
 * @brief lld_spi_eeprom_wr_status
 *  send WRSR instruction to the SPI eeprom
 *
 * @param dev_id: int
 *  chip id
 *
 * @param status: uint8_t
 *  status to be written into the eeprom status register
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_eeprom_wr_status(bf_dev_id_t dev_id, uint8_t status) {
  uint8_t spi_code = BF_SPI_OPCODE_WRSR,rd_status;
  bf_status_t bf_sts = BF_SUCCESS;

  bf_sts |= lld_spi_eeprom_wr_enable(dev_id , 1);
  bf_sts |= lld_spi_wr(dev_id, spi_code, &status, 1, 0);
  bf_sts |= lld_spi_eeprom_rd_status(dev_id, &rd_status); // remove
  bf_sts |= lld_spi_eeprom_wr_enable(dev_id , 0);
  return bf_sts;
}

/**
 * @brief lld_spi_eeprom_rd_status
 *  send RDSR instruction to the SPI eeprom
 *
 * @param dev_id: int
 *  chip id
 *
 * @param status: uint8_t
 *  status to be read from the eeprom
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_eeprom_rd_status(bf_dev_id_t dev_id, uint8_t *status) {
  bf_status_t bf_sts;
  uint32_t data;
  uint8_t spi_code = BF_SPI_OPCODE_RDSR;

  bf_sts = lld_spi_wr(dev_id, spi_code, NULL, 0, 1);
  if (bf_sts != BF_SUCCESS) {
    return bf_sts;
  }
  bf_sts = lld_spi_get_rd_data(dev_id, &data);
  if (bf_sts != BF_SUCCESS) {
    return bf_sts;
  }
  *status = (uint8_t)data;
  return BF_SUCCESS;
}
#endif /* for future use */

/**
 * @brief lld_spi_eeprom_rd_status
 *  send RDSR instruction to the SPI eeprom
 *
 * @param dev_id: int
 *  chip id
 *
 * @param status: uint8_t
 *  status to be read from the eeprom
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t lld_spi_eeprom_rd_status(bf_dev_id_t dev_id, uint8_t *status) {
  bf_status_t bf_sts;
  uint32_t data;
  uint8_t spi_code = BF_SPI_OPCODE_RDSR;

  bf_sts = lld_spi_wr(dev_id, spi_code, NULL, 0, 1);
  if (bf_sts != BF_SUCCESS) {
    return bf_sts;
  }
  bf_sts = lld_spi_get_rd_data(dev_id, &data);
  if (bf_sts != BF_SUCCESS) {
    return bf_sts;
  }
  *status = (uint8_t)data;
  return BF_SUCCESS;
}
#endif /* for future use */

/**
 * @brief bf_spi_eeprom_wr
 *  write to eeprom address
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param addr: uint32_t
 *  address offset in EEPROM
 *
 * @param buf: uint8_t *
 *   buffer to write  to EEPROM
 *
 * @param buf_size:  int
 *   buffer  size
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_spi_eeprom_wr(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t addr,
                             uint8_t *buf,
                             int buf_size) {
  uint8_t wr_buf[8];

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT ||
      (addr + buf_size) >= BF_SPI_EEPROM_SIZE) {
    return BF_INVALID_ARG;
  }
  /* write 4 bytes (maximum) into EEPROM at at time */
  while (buf_size > BF_SPI_EEPROM_WR_SIZE) {
    /* do wr_enable, wr_disable is automatic in h/w at the end of the
     * write operation
     */
    if (lld_spi_eeprom_wr_enable(dev_id, subdev_id, 1) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    bf_sys_usleep(1000);
    wr_buf[0] = addr >> 8;
    wr_buf[1] = addr;
    memcpy(&wr_buf[2], buf, BF_SPI_EEPROM_WR_SIZE);
    if (lld_spi_wr(dev_id,
                   subdev_id,
                   BF_SPI_OPCODE_WR_DATA,
                   &wr_buf[0],
                   BF_SPI_EEPROM_WR_SIZE + 2,
                   0) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    buf += BF_SPI_EEPROM_WR_SIZE;
    buf_size -= BF_SPI_EEPROM_WR_SIZE;
    addr += BF_SPI_EEPROM_WR_SIZE;
    bf_sys_usleep(5000);
  }
  if (buf_size) {
    if (lld_spi_eeprom_wr_enable(dev_id, subdev_id, 1) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    bf_sys_usleep(1000);
    wr_buf[0] = addr >> 8;
    wr_buf[1] = addr;
    memcpy(&wr_buf[2], buf, buf_size);
    if (lld_spi_wr(dev_id,
                   subdev_id,
                   BF_SPI_OPCODE_WR_DATA,
                   &wr_buf[0],
                   buf_size + 2,
                   0) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    bf_sys_usleep(5000);
  }
  lld_spi_eeprom_wr_enable(dev_id, subdev_id, 0);
  return BF_SUCCESS;
}

/**
 * @brief lld_spi_eeprom_rd
 *  write to eeprom address
 *
 * @param dev_id: int
 *  chip id
 *
 * @param subdev_id: int
 *  subdevice id
 *
 * @param addr: uint32_t
 *  address offset in EEPROM
 *
 * @param buf: uint8_t *
 *   buffer to read into  from EEPROM
 *
 * @param buf_size:  int
 *   buffer  size
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */
bf_status_t bf_spi_eeprom_rd(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t addr,
                             uint8_t *buf,
                             int buf_size) {
  uint8_t wr_buf[BF_SPI_EEPROM_WR_SIZE];
  int spi_delay_us = (700 * BF_SPI_PERIOD_NS) / 1000;
  uint32_t data, swap_data;

  if (dev_id >= BF_MAX_DEV_COUNT || subdev_id >= BF_MAX_SUBDEV_COUNT ||
      (addr + buf_size) >= BF_SPI_EEPROM_SIZE) {
    return BF_INVALID_ARG;
  }
  while (buf_size > BF_SPI_EEPROM_RD_SIZE) {
    /* create the SPI write command */
    wr_buf[0] = (uint8_t)(addr >> 8);
    wr_buf[1] = (uint8_t)addr;
    wr_buf[2] = 0;
    if (lld_spi_wr(dev_id,
                   subdev_id,
                   BF_SPI_OPCODE_RD_DATA,
                   wr_buf,
                   BF_SPI_EEPROM_RD_CMD_SIZE,
                   BF_SPI_EEPROM_RD_SIZE) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    /* read the EEPROM content */
    /* wait for data to be read back on SPI bus */
    bf_sys_usleep(spi_delay_us);
    if (lld_spi_get_rd_data(dev_id, subdev_id, &data) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    swap_data = swap32(data);
    *(uint32_t *)buf = swap_data;
    buf += BF_SPI_EEPROM_RD_SIZE;
    buf_size -= BF_SPI_EEPROM_RD_SIZE;
    addr += BF_SPI_EEPROM_RD_SIZE;
  }
  if (buf_size) {
    /* create the SPI write command */
    wr_buf[0] = (uint8_t)(addr >> 8);
    wr_buf[1] = (uint8_t)addr;
    wr_buf[2] = 0;
    if (lld_spi_wr(dev_id,
                   subdev_id,
                   BF_SPI_OPCODE_RD_DATA,
                   wr_buf,
                   BF_SPI_EEPROM_RD_CMD_SIZE,
                   BF_SPI_EEPROM_RD_SIZE) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    /* read the EEPROM content */
    bf_sys_usleep(spi_delay_us);
    if (lld_spi_get_rd_data(dev_id, subdev_id, &data) != BF_SUCCESS) {
      return BF_HW_COMM_FAIL;
    }
    swap_data = swap32(data);
    while (buf_size) {
      *buf = swap_data;
      buf++;
      buf_size--;
      swap_data >>= 8;
    }
  }
  return BF_SUCCESS;
}

/**
 * @}
 */
