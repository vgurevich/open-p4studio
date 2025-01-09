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
 *  * @file lld_spi_if.h
 *   * @date
 *    *
 *     */

#ifndef __LLD_SPI_IF_H_
#define __LLD_SPI_IF_H_

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define BF_SPI_MAX_OUTDATA 8
#define BF_SPI_MAX_INDATA 4

#define BF_SPI_PERIOD_NS 200 /* SPI clock period in nano seconds */
/* SPI device OPcodes */
#define BF_SPI_OPCODE_WR_EN 6
#define BF_SPI_OPCODE_WR_DI 4
#define BF_SPI_OPCODE_WR_DATA 2
#define BF_SPI_OPCODE_RD_DATA 3
#define BF_SPI_OPCODE_WRSR 1
#define BF_SPI_OPCODE_RDSR 5

/* SPI EEPROM constants */
#define BF_SPI_EEPROM_SIZE (512 * 1024 / 8) /* 512 Kbits part */
#define BF_SPI_EEPROM_WR_SIZE 4             /* write 4 bytes at a time */
#define BF_SPI_EEPROM_RD_CMD_SIZE                                        \
  2                             /* bytes to write to issue a eeprom read \
                                 */
#define BF_SPI_EEPROM_RD_SIZE 4 /* write 4 bytes at a time */

/* SPI FSM Instructions */
#define BF_SPI_FSM_START 0x01
#define BF_SPI_FSM_SBUS_SINGLE 0x17
#define BF_SPI_FSM_SBUS_STREAM 0x25
#define BF_SPI_FSM_WAIT 0x32
#define BF_SPI_FSM_REG_WR 0x45
#define BF_SPI_FSM_I2CBYTE_WR 0x71
#define BF_SPI_FSM_END 0x61

bf_status_t lld_spi_init(int chip_id, bf_subdev_id_t subdev_id);
/* spi_code is always sent out on the wire. w_size and r_size could be zero
 * if only spi_code needs to go out on wire. e.g., to enable write on an eeprom,
 * only the spi_code needs to go out
 */
bf_status_t lld_spi_wr(int chip_id,
                       bf_subdev_id_t subdev_id,
                       uint8_t spi_code,
                       uint8_t *w_buf,
                       int w_size,
                       int r_size);
bf_status_t lld_spi_get_idcode(int chip_id,
                               bf_subdev_id_t subdev_id,
                               uint32_t *spi_idcode);
bf_status_t lld_spi_get_rd_data(int chip_id,
                                bf_subdev_id_t subdev_id,
                                uint32_t *spi_rd_data);
bf_status_t bf_spi_eeprom_wr(int chip_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t addr,
                             uint8_t *buf,
                             int buf_size);
bf_status_t bf_spi_eeprom_rd(int chip_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t addr,
                             uint8_t *buf,
                             int buf_size);

/* @} */

#ifdef __cplusplus
}
#endif /* C++ */

#endif
