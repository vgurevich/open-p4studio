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
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/lldlib_config.h>

#if LLDLIB_CONFIG_INCLUDE_UCLI == 1

#include <dvm/bf_drv_intf.h>
#include "lld.h"
#include "lld_dev.h"
#include <lld/lld_gpio_if.h>
#include "lldlib_log.h"
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

static ucli_status_t lldgpio_ucli_ucli__set_mode__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint32_t mode;

  UCLI_COMMAND_INFO(uc,
                    "gpio_set_mode",
                    4,
                    "set Tofino GPIO pin mode <asic> <subdev> <pin_pair> "
                    "<mode: 0:gpio, 1:i2c, 2:mdio>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  mode = strtoul(uc->pargs->args[3], NULL, 16);
  if (mode == 0) {
    aim_printf(&uc->pvs, "*** changing existing GPIO pin usage to GPIO ***\n");
    if (bf_io_set_mode_gpio(
            asic, subdev, pin_pair, BF_GPIO_OUT_Z, BF_GPIO_OUT_Z) !=
        BF_SUCCESS) {
      return UCLI_STATUS_E_IO;
    }
  } else if (mode == 1) {
    aim_printf(&uc->pvs, "*** changing existing GPIO pin usage to i2c ***\n");
    if (bf_io_set_mode_i2c(asic, subdev, pin_pair) != BF_SUCCESS) {
      return UCLI_STATUS_E_IO;
    }
  } else if (mode == 2) {
    aim_printf(&uc->pvs, "*** changing existing GPIO pin usage to MDIO ***\n");
    if (bf_io_set_mode_mdio(asic, subdev, pin_pair) != BF_SUCCESS) {
      return UCLI_STATUS_E_IO;
    }
  } else {
    return UCLI_STATUS_E_ARG;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__set_gpio_cfg__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_gpio_pin_t pin;
  bf_gpio_dir_t dir;
  bf_gpio_output_level_t level;

  UCLI_COMMAND_INFO(uc,
                    "set_gpio_cfg",
                    5,
                    "program Tofino GPIO pin <asic> <subdev> <pin> "
                    "<dir: 0:in, 1:out> <level(if dir is out) 1:highZ 2:low "
                    "3:high>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin = atoi(uc->pargs->args[2]);
  dir = atoi(uc->pargs->args[3]);
  level = atoi(uc->pargs->args[4]);

  if (bf_gpio_set_dir(asic, subdev, pin, dir, level) != BF_SUCCESS) {
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__get_gpio_cfg__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_gpio_pin_t pin;
  bf_gpio_dir_t dir;
  bool level;

  UCLI_COMMAND_INFO(
      uc, "get_gpio_cfg", 3, "Get Tofino GPIO pin state <asic> <subdev> <pin>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin = atoi(uc->pargs->args[2]);
  if (bf_gpio_get_dir(asic, subdev, pin, &dir) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error getting gpio dir for pin %d\n", pin);
    return UCLI_STATUS_E_IO;
  } else {
    aim_printf(&uc->pvs,
               "gpio pin %d dir: %s\n",
               pin,
               ((dir == BF_GPIO_OUT) ? "out" : "in"));
  }
  if (bf_gpio_get_level(asic, subdev, pin, &level) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error getting gpio level for pin %d\n", pin);
    return UCLI_STATUS_E_IO;
  } else {
    aim_printf(
        &uc->pvs, "gpio pin %d level: %s\n", pin, (level ? "high" : "low"));
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__set_i2c_cfg__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint16_t clk_period;

  UCLI_COMMAND_INFO(uc,
                    "set_i2c_cfg",
                    4,
                    "Configure Tofino GPIO-i2c pin <asic> <subdev> <pin-pair> "
                    "<clk period in micro-sec 1-100>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  clk_period = strtoul(uc->pargs->args[3], NULL, 10);
  if (clk_period == 0 || clk_period > 100) {
    aim_printf(&uc->pvs, "invalid clk-period\n");
    return UCLI_STATUS_E_ARG;
  }
  /* convert period to multiple of 10ns */
  clk_period *= 100;

  aim_printf(&uc->pvs,
             "** changing pin-pair %d in register access i2c mode **\n",
             pin_pair);
  if (bf_i2c_set_submode(asic, subdev, pin_pair, BF_I2C_MODE_REG) !=
      BF_SUCCESS) {
    aim_printf(
        &uc->pvs, "error setting i2c submode for pin-pair %d\n", pin_pair);
    return UCLI_STATUS_E_IO;
  }
  if (bf_i2c_set_clk(asic, subdev, pin_pair, clk_period) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error setting i2c clk for pin-pair %d\n", pin_pair);
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__set_mdio_cfg__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint16_t clk_period;

  UCLI_COMMAND_INFO(uc,
                    "set_mdio_cfg",
                    4,
                    "Configure Tofino GPIO-mdio pin <asic> <subdev> <pin-pair> "
                    "<clk period in micro-sec 1-100>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  clk_period = strtoul(uc->pargs->args[3], NULL, 10);
  if (clk_period == 0 || clk_period > 100) {
    aim_printf(&uc->pvs, "invalid clk-period\n");
    return UCLI_STATUS_E_ARG;
  }
  /* convert period to multiple of 10ns */
  clk_period *= 100;

  if (bf_mdio_set_clk(asic, subdev, pin_pair, clk_period) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error setting mdio clk for pin-pair %d\n", pin_pair);
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__i2c_reg_wr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t i2c_addr;
  uint32_t reg_addr;
  int num_reg_addr, num_data_bytes, bytes_read = 0;
  uint8_t *pkt_data = NULL;
  char *hex_string;

  UCLI_COMMAND_INFO(
      uc,
      "i2c_reg_wr",
      8,
      "Perform Tofino GPIO-i2c register mode write <asic> "
      "<subdev> <pin-pair> <i2c_addr> <reg_addr> "
      "<num_addr_bytes> <number of data_bytes 1-4> <data byte hex "
      "string>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  i2c_addr = strtoul(uc->pargs->args[3], NULL, 16);
  reg_addr = strtoul(uc->pargs->args[4], NULL, 16);
  num_reg_addr = atoi(uc->pargs->args[5]);
  num_data_bytes = atoi(uc->pargs->args[6]);
  if (i2c_addr > 0x7f || num_data_bytes > 4 || num_data_bytes < 0) {
    aim_printf(&uc->pvs, "bad i2c_address 0x%x\n", i2c_addr);
    return UCLI_STATUS_E_ARG;
  }
  pkt_data = (uint8_t *)bf_sys_calloc(num_data_bytes, sizeof(uint8_t));
  if (!pkt_data) {
    aim_printf(&uc->pvs, "malloc error\n");
    return UCLI_STATUS_E_INTERNAL;
  }
  hex_string = (char *)uc->pargs->args[7];
  // %2hhx reads two unsigned chars and treats them as a hex number
  while (bytes_read < num_data_bytes &&
         sscanf(hex_string, "%2hhx", &pkt_data[bytes_read]) == 1) {
    ++bytes_read;
    hex_string += 2;
  }
  if (bytes_read <= 0) {
    aim_printf(&uc->pvs,
               "failed to parse input string: %s\n"
               "Sending empty packet\n",
               hex_string);
  }
  if (bf_i2c_wr_reg_blocking(asic,
                             subdev,
                             pin_pair,
                             i2c_addr,
                             reg_addr,
                             num_reg_addr,
                             pkt_data,
                             num_data_bytes) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performing i2c write\n");
    bf_sys_free(pkt_data);
    return UCLI_STATUS_E_IO;
  }
  bf_sys_free(pkt_data);
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__i2c_reg_rd__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t i2c_addr;
  uint32_t reg_addr;
  int num_reg_addr, num_data_bytes;
  uint8_t *pkt_data = NULL;

  UCLI_COMMAND_INFO(uc,
                    "i2c_reg_rd",
                    7,
                    "Perform Tofino GPIO-i2c register mode read <asic> "
                    "<subdev> <pin-pair> <i2c_addr> <reg_addr> "
                    "<num_addr_bytes> <number of data_bytes to read 1-4>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  i2c_addr = strtoul(uc->pargs->args[3], NULL, 16);
  reg_addr = strtoul(uc->pargs->args[4], NULL, 16);
  num_reg_addr = atoi(uc->pargs->args[5]);
  num_data_bytes = atoi(uc->pargs->args[6]);
  if (i2c_addr > 0x7f || num_data_bytes > 4 || num_data_bytes < 0) {
    aim_printf(&uc->pvs, "bad i2c_address 0x%x or read count\n", i2c_addr);
    return UCLI_STATUS_E_ARG;
  }
  pkt_data = (uint8_t *)bf_sys_calloc(num_data_bytes, sizeof(uint8_t));
  if (!pkt_data) {
    aim_printf(&uc->pvs, "malloc error\n");
    return UCLI_STATUS_E_INTERNAL;
  }
  if (bf_i2c_rd_reg_blocking(asic,
                             subdev,
                             pin_pair,
                             i2c_addr,
                             reg_addr,
                             num_reg_addr,
                             pkt_data,
                             num_data_bytes) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performing i2c rd\n");
    bf_sys_free(pkt_data);
    return UCLI_STATUS_E_IO;
  }
  aim_printf(&uc->pvs, "bytes read:\n");
  for (int i = 0; i < num_data_bytes; i++) {
    aim_printf(&uc->pvs, "0x%02x ", pkt_data[i]);
  }
  aim_printf(&uc->pvs, "\n");
  bf_sys_free(pkt_data);
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__mdio_addr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t phy_addr, dev_type;
  uint16_t addr;

  UCLI_COMMAND_INFO(uc,
                    "mdio_addr_set",
                    6,
                    "Perform Tofino GPIO-mdio-45 addr set <asic> <subdev> "
                    "<pin-pair> <phy_addr> <dev_type> <addr>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  phy_addr = strtoul(uc->pargs->args[3], NULL, 16);
  dev_type = strtoul(uc->pargs->args[4], NULL, 16);
  addr = strtoul(uc->pargs->args[5], NULL, 16);
  if (bf_mdio_addr_blocking(asic, subdev, pin_pair, phy_addr, dev_type, addr) !=
      BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performing mdio-45 addr set\n");
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__mdio_wr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t phy_addr, dev_type;
  uint16_t data;

  UCLI_COMMAND_INFO(uc,
                    "mdio_data_wr",
                    6,
                    "Perform Tofino GPIO-mdio-45 data wr <asic> <subdev> "
                    "<pin-pair> <phy_addr> <dev_type> <data>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  phy_addr = strtoul(uc->pargs->args[3], NULL, 16);
  dev_type = strtoul(uc->pargs->args[4], NULL, 16);
  data = strtoul(uc->pargs->args[5], NULL, 16);
  if (bf_mdio_wr_blocking(asic, subdev, pin_pair, phy_addr, dev_type, data) !=
      BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performing mdio-45 data write\n");
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__mdio_rd__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t phy_addr, dev_type;
  uint16_t data;

  UCLI_COMMAND_INFO(uc,
                    "mdio_data_rd",
                    5,
                    "Perform Tofino GPIO-mdio-45 data rd <asic> <subdev> "
                    "<pin-pair> <phy_addr> <dev_type>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  phy_addr = strtoul(uc->pargs->args[3], NULL, 16);
  dev_type = strtoul(uc->pargs->args[4], NULL, 16);
  if (bf_mdio_rd_blocking(asic, subdev, pin_pair, phy_addr, dev_type, &data) !=
      BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performign mdio-45 data read\n");
    return UCLI_STATUS_E_IO;
  }
  aim_printf(&uc->pvs, "data read 0x%4x\n", data);
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__mdio22_wr__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t phy_addr, reg_addr;
  uint16_t data;

  UCLI_COMMAND_INFO(uc,
                    "mdio22_data_wr",
                    6,
                    "Perform Tofino GPIO-mdio-22 data wr <asic> <subdev> "
                    "<pin-pair> <phy_addr> <reg_addr> <data>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  phy_addr = strtoul(uc->pargs->args[3], NULL, 16);
  reg_addr = strtoul(uc->pargs->args[4], NULL, 16);
  data = strtoul(uc->pargs->args[5], NULL, 16);
  if (bf_mdio22_wr_blocking(asic, subdev, pin_pair, phy_addr, reg_addr, data) !=
      BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performing mdio-22 data write\n");
    return UCLI_STATUS_E_IO;
  }
  return 0;
}

static ucli_status_t lldgpio_ucli_ucli__mdio22_rd__(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_subdev_id_t subdev;
  bf_io_pin_pair_t pin_pair;
  uint8_t phy_addr, reg_addr;
  uint16_t data;

  UCLI_COMMAND_INFO(uc,
                    "mdio22_data_rd",
                    5,
                    "Perform Tofino GPIO-mdio-22 data rd <asic> <subdev> "
                    "<pin-pair> <phy_addr> <reg_addr>");

  asic = atoi(uc->pargs->args[0]);
  subdev = atoi(uc->pargs->args[1]);
  pin_pair = atoi(uc->pargs->args[2]);
  phy_addr = strtoul(uc->pargs->args[3], NULL, 16);
  reg_addr = strtoul(uc->pargs->args[4], NULL, 16);
  if (bf_mdio22_rd_blocking(
          asic, subdev, pin_pair, phy_addr, reg_addr, &data) != BF_SUCCESS) {
    aim_printf(&uc->pvs, "error performign mdio-22 data read\n");
    return UCLI_STATUS_E_IO;
  }
  aim_printf(&uc->pvs, "data read 0x%4x\n", data);
  return 0;
}

/* <auto.ucli.handlers.start> */
static ucli_command_handler_f lldgpio_ucli_ucli_handlers__[] = {
    // basic register and memory access
    lldgpio_ucli_ucli__set_mode__,
    lldgpio_ucli_ucli__set_gpio_cfg__,
    lldgpio_ucli_ucli__get_gpio_cfg__,
    lldgpio_ucli_ucli__set_i2c_cfg__,
    lldgpio_ucli_ucli__set_mdio_cfg__,
    lldgpio_ucli_ucli__i2c_reg_wr__,
    lldgpio_ucli_ucli__i2c_reg_rd__,
    lldgpio_ucli_ucli__mdio_addr__,
    lldgpio_ucli_ucli__mdio_wr__,
    lldgpio_ucli_ucli__mdio_rd__,
    lldgpio_ucli_ucli__mdio22_wr__,
    lldgpio_ucli_ucli__mdio22_rd__,
    NULL};

/* <auto.ucli.handlers.end> */

static ucli_module_t lldlib_gpio_ucli_module__ = {
    "gpio_ucli",
    NULL,
    lldgpio_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *lldlib_gpio_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&lldlib_gpio_ucli_module__);
  m = ucli_node_create("gpio", n, &lldlib_gpio_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("gpio"));
  return m;
}

#else
void *lldlib_gpio_ucli_node_create(void) { return NULL; }
#endif
