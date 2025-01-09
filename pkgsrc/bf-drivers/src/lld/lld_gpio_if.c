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
 * @file lld_gpio_if.c
 * @date
 *
 */

/**
 * @addtogroup lld-gpio-api
 * @{
 * This is a description of GPIO MDIO and I2C APIs.
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
#include "lldlib_log.h"

typedef struct {
  bf_gpio_dir_t dir[BF_GPIO_NUM_PIN];                 // direction
  bf_gpio_output_level_t out_level[BF_GPIO_NUM_PIN];  // output level
  bf_gpio_pull_dir_t pull_dir[BF_GPIO_NUM_PIN];       // pullup or pulldown
  bool pull_en[BF_GPIO_NUM_PIN];                      // pull up/down enable
  bf_gpio_trigger_mode_t trig_mode[BF_GPIO_NUM_PIN];  // trigger mode`
  uint8_t debounce_time[BF_GPIO_NUM_PIN];             // debounce time
  bf_gpio_drive_t cur_drive[BF_GPIO_NUM_PIN];         // drive strength
} bf_gpio_setting_t;

/* this structure caches current GPIO settings */
static bf_gpio_setting_t bf_gpio_setting[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT];
static bf_io_mode_t bf_io_mode_setting[BF_MAX_DEV_COUNT][BF_MAX_SUBDEV_COUNT]
                                      [BF_IO_NUM_PIN_PAIR];

static bf_status_t lld_gpio_reg_wr(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   uint32_t reg,
                                   uint32_t data) {
  if (lld_subdev_write_register(dev_id, subdev_id, reg, data) == LLD_OK) {
    return BF_SUCCESS;
  } else {
    return BF_HW_COMM_FAIL;
  }
}

static bf_status_t lld_gpio_reg_rd(bf_dev_id_t dev_id,
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
 * @brief bf_io_get_mode:
 *  get IO mode associated with a GPIO pin
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pin_pair: enum: which of the 12/8 pin-pairs to configure
 * @param mode: bf_io_mode_t
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_io_get_mode(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_io_pin_pair_t pin_pair,
                           bf_io_mode_t *mode) {
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
  *mode = bf_io_mode_setting[dev_id][subdev_id][pin_pair];
  return BF_SUCCESS;
}

/**
 * @brief bf_io_set_mode_gpio:
 *  set IO mode to GPIO (with default GPIO settings)
 *  all pins input type and pull up/down disabled
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pin_pair: enum: which of the 12/8 pin-pairs to configure
 * @param level_pin0: enum : output level of pin-0 of the pair if gpio pin
 * configured as o/p type
 * @param level_pin1: enum : output level of pin-1 of the pair if gpio pin
 * configured as o/p type
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_io_set_mode_gpio(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_io_pin_pair_t pin_pair,
                                bf_gpio_output_level_t level_pin0,
                                bf_gpio_output_level_t level_pin1) {
  uint32_t data, config, setting, mode0, mode1;
  uint32_t offset, idx;

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
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, gpio_config);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &config);
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, gpio_settings);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &setting);

  /* capture mode, debounce time, pullup direction, pullup enable and drive
   * settings are left to default reset or the current values */

  /* cache the gpio settings after reading from the hardware */
  idx = pin_pair * 2;
  data = getp_gpio_pair_regs_gpio_config_gpio0_mode(&config);
  if (data == 0) {
    bf_gpio_setting[dev_id][subdev_id].dir[idx] = BF_GPIO_IN;
    bf_gpio_setting[dev_id][subdev_id].out_level[idx] =
        BF_GPIO_OUT_Z; /* don't care */
    mode0 = 0;
  } else {
    bf_gpio_setting[dev_id][subdev_id].dir[idx] = BF_GPIO_OUT;
    bf_gpio_setting[dev_id][subdev_id].out_level[idx] = level_pin0;
    mode0 = (uint32_t)level_pin0;
  }
  if (setting & 0x2) {
    bf_gpio_setting[dev_id][subdev_id].pull_en[idx] = true;
  } else {
    bf_gpio_setting[dev_id][subdev_id].pull_en[idx] = false;
  }
  if (setting & 0x1) {
    bf_gpio_setting[dev_id][subdev_id].pull_dir[idx] = BF_GPIO_PULL_UP;
  } else {
    bf_gpio_setting[dev_id][subdev_id].pull_dir[idx] = BF_GPIO_PULL_DOWN;
  }
  if ((setting & 0x0c) == 0) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_4MA;
  } else if ((setting & 0x0c) == 0x4) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_6MA;
  } else if ((setting & 0x0c) == 0x8) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_8MA;
  } else {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_12MA;
  }
  bf_gpio_setting[dev_id][subdev_id].trig_mode[idx] =
      getp_gpio_pair_regs_gpio_config_gpio0_capt(&config);
  bf_gpio_setting[dev_id][subdev_id].debounce_time[idx] =
      getp_gpio_pair_regs_gpio_config_gpio0_debounce(&config);
  LOG_WARN("gpio pin %d configured dir:level %d:%d\n",
           idx,
           bf_gpio_setting[dev_id][subdev_id].dir[idx],
           bf_gpio_setting[dev_id][subdev_id].out_level[idx]);

  idx++;
  data = getp_gpio_pair_regs_gpio_config_gpio1_mode(&config);
  if (data == 0) {
    bf_gpio_setting[dev_id][subdev_id].dir[idx] = BF_GPIO_IN;
    bf_gpio_setting[dev_id][subdev_id].out_level[idx] =
        BF_GPIO_OUT_Z; /* don't care */
    mode1 = 0;
  } else {
    bf_gpio_setting[dev_id][subdev_id].dir[idx] = BF_GPIO_OUT;
    bf_gpio_setting[dev_id][subdev_id].out_level[idx] = level_pin1;
    mode1 = (uint32_t)level_pin1;
  }
  if (setting & 0x20) {
    bf_gpio_setting[dev_id][subdev_id].pull_en[idx] = true;
  } else {
    bf_gpio_setting[dev_id][subdev_id].pull_en[idx] = false;
  }
  if (setting & 0x10) {
    bf_gpio_setting[dev_id][subdev_id].pull_dir[idx] = BF_GPIO_PULL_UP;
  } else {
    bf_gpio_setting[dev_id][subdev_id].pull_dir[idx] = BF_GPIO_PULL_DOWN;
  }
  if ((setting & 0xc0) == 0) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_4MA;
  } else if ((setting & 0xc0) == 0x40) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_6MA;
  } else if ((setting & 0xc0) == 0x80) {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_8MA;
  } else {
    bf_gpio_setting[dev_id][subdev_id].cur_drive[idx] = BF_GPIO_DRIVE_12MA;
  }
  bf_gpio_setting[dev_id][subdev_id].trig_mode[idx] =
      getp_gpio_pair_regs_gpio_config_gpio1_capt(&config);
  bf_gpio_setting[dev_id][subdev_id].debounce_time[idx] =
      getp_gpio_pair_regs_gpio_config_gpio1_debounce(&config);

  /* now write mode and level */
  bf_io_mode_setting[dev_id][subdev_id][pin_pair] = BF_IO_MODE_GPIO;
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, gpio_config);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &config);
  setp_gpio_pair_regs_gpio_config_gpio_type(&config, BF_IO_MODE_GPIO);
  setp_gpio_pair_regs_gpio_config_gpio0_mode(&config, mode0);
  setp_gpio_pair_regs_gpio_config_gpio1_mode(&config, mode1);
  LOG_WARN("gpio pin %d configured as gpio dir:level %d:%d\n",
           idx,
           bf_gpio_setting[dev_id][subdev_id].dir[idx],
           bf_gpio_setting[dev_id][subdev_id].out_level[idx]);
  return (lld_gpio_reg_wr(dev_id, subdev_id, offset, config));
}

/**
 * @brief bf_io_set_mode_i2c:
 *  set IO mode to i2c
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pin_pair: enum: which of the 12/8 pin-pairs to configure
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_io_set_mode_i2c(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_io_pin_pair_t pin_pair) {
  uint32_t data;
  uint32_t offset;

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
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, gpio_config);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_gpio_pair_regs_gpio_config_gpio_type(&data, BF_IO_MODE_I2C);
  bf_io_mode_setting[dev_id][subdev_id][pin_pair] = BF_IO_MODE_I2C;
  LOG_WARN("gpio pin pair %d configured as i2c type\n", (int)pin_pair);
  return (lld_gpio_reg_wr(dev_id, subdev_id, offset, data));
}

/**
 * @brief bf_io_set_mode_mdio:
 *  set IO mode to mdio
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pin_pair: enum: which of the 12/8 pin-pairs to configure
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_io_set_mode_mdio(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_io_pin_pair_t pin_pair) {
  uint32_t data;
  uint32_t offset;

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
  offset = GPIO_PAIR_OFFSET(dev_id, pin_pair, gpio_config);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_gpio_pair_regs_gpio_config_gpio_type(&data, BF_IO_MODE_MDIO);
  bf_io_mode_setting[dev_id][subdev_id][pin_pair] = BF_IO_MODE_MDIO;
  LOG_WARN("gpio pin pair %d configured as gpio type\n", (int)pin_pair);
  return (lld_gpio_reg_wr(dev_id, subdev_id, offset, data));
}

/**
 * @brief bf_io_is_gpio_mode:
 *  check if  th econfigured mode is gpio
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 *
 * @return bool
 *  true if gpio mode
 *  false if not gpio mode
 *
 */
static bool bf_io_is_gpio_mode(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_gpio_pin_t gpio_pin) {
  bf_io_mode_t mode;
  bf_status_t bf_status;

  bf_status = bf_io_get_mode(dev_id, subdev_id, gpio_pin / 2, &mode);
  if (bf_status != BF_SUCCESS) {
    return false;
  }
  if (mode != BF_IO_MODE_GPIO) {
    return false;
  } else {
    return true;
  }
}

/**
 * @brief  bf_gpio_set_level:
 *  set the output level
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param level: bf_gpio_output_level_t : output level
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_set_level(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              bf_gpio_pin_t gpio_pin,
                              bf_gpio_output_level_t level) {
  uint32_t offset, tmp;
  uint32_t data;
  bool gpio_1;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  if (bf_gpio_setting[dev_id][subdev_id].dir[gpio_pin] == BF_GPIO_IN) {
    return BF_IN_USE; /* the pin is configured as input type */
  }
  bf_gpio_setting[dev_id][subdev_id].out_level[gpio_pin] = level;
  gpio_1 = gpio_pin % 2; /* is is gpio0 or gpio1 with thin a pair */
  offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_config);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  tmp = level;
  if (gpio_1) {
    setp_gpio_pair_regs_gpio_config_gpio1_mode(&data, tmp);
  } else {
    setp_gpio_pair_regs_gpio_config_gpio0_mode(&data, tmp);
  }
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  LOG_DBG("gpio pin %d level set to %d\n", (int)gpio_pin, (int)level);
  return BF_SUCCESS; /* nothing to do */
}

/**
 * @brief  bf_gpio_get_level:
 *  get the input level
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param level: bool: input level: 1 or 0
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_get_level(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              bf_gpio_pin_t gpio_pin,
                              bool *level) {
  uint32_t offset;
  uint32_t data;

  if (!level) {
    return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  offset = GPIO_COMM_OFFSET(dev_id, gpio_pin / 2, gpio_status);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  if (gpio_pin >= BF_GPIO_PIN_12) {
    gpio_pin = gpio_pin - BF_GPIO_PIN_12;
  }
  *level = (getp_gpio_common_regs_gpio_status_gpio_in(&data) & (1 << gpio_pin))
               ? true
               : false;
  return BF_SUCCESS; /* nothing to do */
}

/**
 * @brief  bf_gpio_set_pullup:
 *  set the output pull up or pull down
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param pull_dir: bf_gpio_pull_dir_t: pull up/down direction
 * @param en: bool : enable of pull up/down
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_set_pullup(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_gpio_pin_t gpio_pin,
                               bf_gpio_pull_dir_t pull_dir,
                               bool en) {
  uint32_t offset, tmp;
  uint32_t data;
  bool gpio_1;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return BF_INVALID_ARG;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  gpio_1 = gpio_pin % 2; /* is is gpio0 or gpio1 within a pair */
  offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_settings);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  tmp = 0;
  if (pull_dir == BF_GPIO_PULL_UP) {
    tmp |= 1;
  }
  if (en) {
    tmp |= (1 << 1);
  }
  tmp |= (bf_gpio_setting[dev_id][subdev_id].cur_drive[gpio_pin] << 2);
  if (gpio_1) {
    setp_gpio_pair_regs_gpio_settings_gpio1_settings(&data, tmp);
  } else {
    setp_gpio_pair_regs_gpio_settings_gpio0_settings(&data, tmp);
  }
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  bf_gpio_setting[dev_id][subdev_id].pull_dir[gpio_pin] = pull_dir;
  bf_gpio_setting[dev_id][subdev_id].pull_en[gpio_pin] = en;
  LOG_WARN("gpio pin %d pull %s %s\n",
           (int)gpio_pin,
           (pull_dir == BF_GPIO_PULL_UP ? "up" : "down"),
           (en ? "enabled" : "disabled"));
  return BF_SUCCESS;
}

/**
 * @brief  bf_gpio_get_pullup:
 *  get the pullup/down asociated with the gpio
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param pull_dir: bf_gpio_pull_dir_t: pullup or pulldown
 * @param en: bool: if the pullup/pulldown is enabled
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_get_pullup(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_gpio_pin_t gpio_pin,
                               bf_gpio_pull_dir_t *pull_dir,
                               bool *en) {
  uint32_t offset, tmp;
  uint32_t data;
  bool gpio_1;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!pull_dir || !en) {
    return BF_INVALID_ARG;
  }

  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  gpio_1 = gpio_pin % 2; /* is it gpio0 or gpio1 with thin a pair */
  offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_settings);
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  if (gpio_1) {
    tmp = getp_gpio_pair_regs_gpio_settings_gpio1_settings(&data);
  } else {
    tmp = getp_gpio_pair_regs_gpio_settings_gpio0_settings(&data);
  }
  *pull_dir = ((tmp & 0x1) ? BF_GPIO_PULL_UP : BF_GPIO_PULL_DOWN);
  *en = (tmp >> 1);

  return BF_SUCCESS;
}

/**
 * @brief  bf_gpio_set_dir:
 *  set the direction of gpio pin
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param dir: bf_gpio_dir_t: gpio pin direction
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_set_dir(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            bf_gpio_pin_t gpio_pin,
                            bf_gpio_dir_t dir,
                            bf_gpio_output_level_t level) {
  uint32_t offset;
  uint32_t data;
  bool gpio_1;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return BF_INVALID_ARG;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  gpio_1 = gpio_pin % 2; /* is is gpio0 or gpio1 with thin a pair */

  bf_gpio_setting[dev_id][subdev_id].dir[gpio_pin] = dir;
  if (dir == BF_GPIO_OUT) {
    /* set level, output direction is implied with the level */
    bf_gpio_set_level(dev_id, subdev_id, gpio_pin, level);
  } else {
    /* just change the mode (direction) to input type */
    offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_config);
    lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
    if (gpio_1) {
      setp_gpio_pair_regs_gpio_config_gpio1_mode(&data, 0);
    } else {
      setp_gpio_pair_regs_gpio_config_gpio0_mode(&data, 0);
    }
    lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  }
  LOG_WARN("gpio pin %d dir- %s level %d\n",
           (int)gpio_pin,
           (dir == BF_GPIO_IN ? "in" : "out"),
           (int)level);
  return BF_SUCCESS;
}

/**
 * @brief  bf_gpio_get_dir:
 *  get the gpio direction
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param dir: bf_gpio_dir_t: GPIO_IN or GPIO_OUT
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_get_dir(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            bf_gpio_pin_t gpio_pin,
                            bf_gpio_dir_t *dir) {
  if ((dev_id >= BF_MAX_DEV_COUNT) || (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return BF_INVALID_ARG;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  if (!dir) {
    return BF_INVALID_ARG;
  }

  if (!bf_io_is_gpio_mode(dev_id, subdev_id, gpio_pin)) {
    return BF_IN_USE;
  }
  *dir = bf_gpio_setting[dev_id][subdev_id].dir[gpio_pin];
  return BF_SUCCESS;
}

/**
 * @brief  bf_gpio_set_debounce_time
 *  set the the debounce time (used when GPIO is used as an input or trigger)
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param period: uint8_t: multiple of 10ns
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_set_debounce_time(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      bf_gpio_pin_t gpio_pin,
                                      uint8_t period) {
  uint32_t offset;
  uint32_t data;
  bool gpio_1;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  /* set debounce time even if the pin is not configured in GPIO mode */
  offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_config);
  gpio_1 = gpio_pin % 2; /* is is gpio0 or gpio1 with thin a pair */
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  if (gpio_1) {
    setp_gpio_pair_regs_gpio_config_gpio1_debounce(&data, (uint32_t)period);
  } else {
    setp_gpio_pair_regs_gpio_config_gpio0_debounce(&data, (uint32_t)period);
  }
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  bf_gpio_setting[dev_id][subdev_id].debounce_time[gpio_pin] = period;
  LOG_DBG("gpio pin %d debounce time set to %d\n", (int)gpio_pin, (int)period);
  return BF_SUCCESS; /* nothing to do */
}

/**
 * @brief  bf_gpio_set_trigger_mode:
 *  set the trigger capture mode if gpio pin is used elsewhere for that
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param gpio_pin: enum: which of the 24/16 gpio pins
 * @param mode: bf_gpio_trigger_mode_t: capture mode
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_gpio_set_trigger_mode(bf_dev_id_t dev_id,
                                     bf_subdev_id_t subdev_id,
                                     bf_gpio_pin_t gpio_pin,
                                     bf_gpio_trigger_mode_t mode) {
  uint32_t offset;
  uint32_t data;
  bool gpio_1;

  if (lld_dev_is_tof3(dev_id)) {
    if (gpio_pin > BF_TOF3_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  } else {
    if (gpio_pin > BF_GPIO_PIN_MAX) {
      return BF_INVALID_ARG;
    }
  }
  /* set trigger mode even if the pin is not configured in GPIO mode */
  offset = GPIO_PAIR_OFFSET(dev_id, gpio_pin / 2, gpio_config);
  gpio_1 = gpio_pin % 2; /* is is gpio0 or gpio1 with thin a pair */
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  if (gpio_1) {
    setp_gpio_pair_regs_gpio_config_gpio1_capt(&data, (uint32_t)mode);
  } else {
    setp_gpio_pair_regs_gpio_config_gpio0_capt(&data, (uint32_t)mode);
  }
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  bf_gpio_setting[dev_id][subdev_id].trig_mode[gpio_pin] = mode;
  LOG_DBG("gpio pin %d trigger mode set to %d\n", (int)gpio_pin, (int)mode);
  return BF_SUCCESS; /* nothing to do */
}

/**
 * @brief  bf_host_gpio_set_mode_pad_ctrl:
 *  set the pad control for host mode_gpio_control (i2c address, VID,
 *  SPI_PRESENT_L pads)
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pad_ctrl: pad control attributes
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_host_gpio_set_mode_pad_ctrl(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           bf_host_gpio_pad_ctrl_t *pad_ctrl) {
  uint32_t offset;
  uint32_t data, val;

  val = pad_ctrl->pull_dir;
  val |= (pad_ctrl->pull_en << 1);
  val |= (pad_ctrl->drv << 2);
  offset = tofino_device_select_misc_regs_gpio_ctrl_address;
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_misc_regs_gpio_ctrl_mode_gpio_ctrl(&data, val);
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief  bf_host_gpio_set_i2c_pad_ctrl:
 *  set the pad control for host i2c_gpio_controli (i2c pads)
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pad_ctrl: pad control attributes
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_host_gpio_set_i2c_pad_ctrl(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          bf_host_gpio_pad_ctrl_t *pad_ctrl) {
  uint32_t offset;
  uint32_t data, val;

  val = pad_ctrl->pull_dir;
  val |= (pad_ctrl->pull_en << 1);
  val |= (pad_ctrl->drv << 2);
  offset = tofino_device_select_misc_regs_gpio_ctrl_address;
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_misc_regs_gpio_ctrl_i2c_gpio_ctrl(&data, val);
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief  bf_host_gpio_set_bare_pad_ctrl:
 *  set the pad control for host BareSync_gpio_control (Baresync Pads)
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pad_ctrl: pad control attributes
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_host_gpio_set_bare_pad_ctrl(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           bf_host_gpio_pad_ctrl_t *pad_ctrl) {
  uint32_t offset;
  uint32_t data, val;

  val = pad_ctrl->pull_dir;
  val |= (pad_ctrl->pull_en << 1);
  val |= (pad_ctrl->drv << 2);
  offset = tofino_device_select_misc_regs_gpio_ctrl_address;
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_misc_regs_gpio_ctrl_bare_gpio_ctrl(&data, val);
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @brief  bf_host_gpio_set_misc_pad_ctrl:
 *  set the pad control for host misc_gpio_control (SPI pads)
 *
 * @param dev_id: int     : chip #
 * @param subdev_id: int  : subdevice within dev_id #
 * @param pad_ctrl: pad control attributes
 *
 * @return status
 *  BF_SUCCESS on success
 *  BF ERROR code on failure
 *
 */
bf_status_t bf_host_gpio_set_misc_pad_ctrl(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           bf_host_gpio_pad_ctrl_t *pad_ctrl) {
  uint32_t offset;
  uint32_t data, val;

  val = pad_ctrl->pull_dir;
  val |= (pad_ctrl->pull_en << 1);
  val |= (pad_ctrl->drv << 2);
  offset = tofino_device_select_misc_regs_gpio_ctrl_address;
  lld_gpio_reg_rd(dev_id, subdev_id, offset, &data);
  setp_misc_regs_gpio_ctrl_misc_gpio_ctrl(&data, val);
  lld_gpio_reg_wr(dev_id, subdev_id, offset, data);
  return BF_SUCCESS;
}

/**
 * @}
 */
