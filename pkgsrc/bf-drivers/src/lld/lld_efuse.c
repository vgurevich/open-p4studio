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


#ifndef __KERNEL__
#include <inttypes.h>  // for PRIx64
#else
#include <bf_types/bf_kernel_types.h>
#endif

#include <bf_types/bf_types.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_bits.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include "lld.h"
#include "lld_map.h"
#include "lld_log.h"
#include "lld_dev.h"
#include "lld_efuse_tof.h"
#include "lld_efuse_tof2.h"
#include "lld_efuse_tof3.h"


// fwd ref
static void lld_efuse_log_settings(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id);

/****************************************************************************
 * lld_efuse_load
 *
 * Load EFUSE contents into dev_p and log contents
 * Returns -1 on error, 0=OK
 ****************************************************************************/
int lld_efuse_load(bf_dev_id_t dev_id,
                   bf_subdev_id_t subdev_id,
                   bf_dev_init_mode_t warm_init_mode) {
  int rc = 0;
  lld_dev_t *dev_p;
  dev_p = lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);
  if (!dev_p) return -1;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      rc = lld_efuse_tof_load(dev_id, warm_init_mode);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      rc = lld_efuse_tof2_load(dev_id, warm_init_mode);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      rc = lld_efuse_tof3_load(dev_id, subdev_id, warm_init_mode);
      break;



    default:
      return -1;
  }

  lld_efuse_log_settings(dev_id, subdev_id);
  return rc;
}

/****************************************************************************
 * lld_subdev_efuse_get_chip_part_number
 *
 * 20b field
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_chip_part_number(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.chip_part_number;
}

/****************************************************************************
 * lld_efuse_get_chip_part_number
 *
 * 20b field
 ****************************************************************************/
uint32_t lld_efuse_get_chip_part_number(bf_dev_id_t dev_id) {
  return (lld_subdev_efuse_get_chip_part_number(dev_id, 0));
}

/****************************************************************************
 * lld_subdev_efuse_get_part_revision_number
 *
 * 8b field
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_part_revision_number(bf_dev_id_t dev_id,
                                                   bf_subdev_id_t subdev_id) {
  uint32_t rev_id;
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  rev_id = dev_p->efuse_data.part_revision_number;
  if (dev_p->dev_family != BF_DEV_FAMILY_TOFINO3) {
    return rev_id;
  } else {
    /* translate TF3 specific rev_id to generic part_revision enum */
    if (rev_id == 0x80UL) {
      return BF_SKU_CHIP_PART_REV_B0;
    } else if (rev_id == 0x01UL) {
      return BF_SKU_CHIP_PART_REV_A1;
    } else {
      return BF_SKU_CHIP_PART_REV_A0;
    }
  }
}

/****************************************************************************
 * lld_efuse_get_part_revision_number
 *
 * 8b field
 ****************************************************************************/
uint32_t lld_efuse_get_part_revision_number(bf_dev_id_t dev_id) {
  return (lld_subdev_efuse_get_part_revision_number(dev_id, 0));
}

/****************************************************************************
 * lld_efuse_get_package_id
 *
 * 8b field
 ****************************************************************************/
uint32_t lld_efuse_get_package_id(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.package_id;
}

/****************************************************************************
 * lld_subdev_efuse_get_chip_id
 *
 * 64b field
 ****************************************************************************/
uint64_t lld_subdev_efuse_get_chip_id(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFFFFFFFFFFULL;
  return dev_p->efuse_data.chip_id;
}

/****************************************************************************
 * lld_efuse_get_chip_id
 *
 * 64b field
 ****************************************************************************/
uint64_t lld_efuse_get_chip_id(bf_dev_id_t dev_id) {
  return (lld_subdev_efuse_get_chip_id(dev_id, 0));
}

/****************************************************************************
 * lld_subdev_efuse_get_silent_spin
 *
 * 2b field
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_silent_spin(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.silent_spin;
}

/****************************************************************************
 * lld_subdev_efuse_get_tm_memory_disable
 *
 * 36b field : When set, the memory block is disabled
 * fuse_tm_mem_dis[35:0]
 * HW auto-config
 ****************************************************************************/
uint64_t lld_subdev_efuse_get_tm_memory_disable(bf_dev_id_t dev_id,
                                                bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFFFFFFFFFFULL;
  return dev_p->efuse_data.tm_memory_disable;
}

/****************************************************************************
 * lld_efuse_get_tm_memory_disable
 *
 * 36b field : When set, the memory block is disabled
 * fuse_tm_mem_dis[35:0]
 * HW auto-config
 ****************************************************************************/
uint64_t lld_efuse_get_tm_memory_disable(bf_dev_id_t dev_id) {
  return (lld_subdev_efuse_get_tm_memory_disable(dev_id, 0));
}

/****************************************************************************
 * lld_subdev_efuse_get_mau_stage_disable
 *
 * 12b field
 * When set, the stage is disabled
 * fuse_mau_stage_dis[11:0]
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_mau_stage_disable(bf_dev_id_t dev_id,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t phy_pipe) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (phy_pipe > 3) return 0xFFFFFFFF;
      return dev_p->efuse_data.mau_stage_disable;
    case BF_DEV_FAMILY_TOFINO2:
      if (phy_pipe > 3) return 0xFFFFFFFF;
      return dev_p->efuse_data.tof2_pipe_mau_stage_disable[phy_pipe];
    case BF_DEV_FAMILY_TOFINO3:
      if (phy_pipe >= BF_SUBDEV_PIPE_COUNT) return 0xFFFFFFFF;
      return dev_p->efuse_data.tof3_pipe_mau_stage_disable[phy_pipe];





    default:
      return 0xFFFFFFFF;
  }
}

/****************************************************************************
 * lld_efuse_get_mau_stage_disable
 *
 * 12b field
 * When set, the stage is disabled
 * fuse_mau_stage_dis[11:0]
 ****************************************************************************/
uint32_t lld_efuse_get_mau_stage_disable(bf_dev_id_t dev_id,
                                         uint32_t phy_pipe) {
  return (lld_subdev_efuse_get_mau_stage_disable(dev_id, 0, phy_pipe));
}

/****************************************************************************
 * lld_subdev_efuse_get_mau_sram_reduction
 *
 * 1b field
 * When set, only 75% of SRAM is used
 * fuse_mau_sram_dis
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_mau_sram_reduction(bf_dev_id_t dev_id,
                                                 bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.mau_sram_reduction;
}

/****************************************************************************
 * lld_subdev_efuse_get_mau_tcam_reduction
 *
 * 1b field
 * When set, only 50% of TCAM is used
 * fuse_mau_tcam_dis
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_mau_tcam_reduction(bf_dev_id_t dev_id,
                                                 bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.mau_tcam_reduction;
}

/****************************************************************************
 * lld_efuse_get_pipe_disable
 *
 * 4b field
 * When set, the pipe is disabled
 * fuse_pipe_dis[3:0]
 ****************************************************************************/
uint32_t lld_efuse_get_pipe_disable(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.pipe_disable;
}

/****************************************************************************
 * lld_efuse_get_pipe_disable
 *
 * 4b field
 * When set, the pipe is disabled
 * fuse_pipe_dis[3:0]
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_pipe_disable(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.pipe_disable;
}

/****************************************************************************
 * lld_efuse_get_port_disable
 *
 * 65b field
 * When set, the MAC port is disabled
 * fuse_port_dis[64:0]
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_port_disable(bf_dev_id_t dev_id,
                                    uint64_t *map_hi,
                                    uint64_t *map_lo) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if ((map_hi == NULL) || (map_lo == NULL)) {
        return 0xFFFFFFFF;
      }
      *map_hi = dev_p->efuse_data.port_disable_map_hi;
      *map_lo = dev_p->efuse_data.port_disable_map_lo;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (map_lo == NULL) {
        return 0xFFFFFFFF;
      }
      *map_lo = dev_p->efuse_data.port_disable_map_lo;
      if (map_hi != NULL) *map_hi = 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (map_lo == NULL) {
        return 0xFFFFFFFF;
      }
      *map_lo = dev_p->efuse_data.port_disable_map_lo;
      if (map_hi != NULL) *map_hi = 0;
      break;
    default:
      return 0xFFFFFFFF;
  }
  return 0;
}

/****************************************************************************
 * lld_subdev_efuse_get_port_disable
 *
 * 65b field
 * When set, the MAC port is disabled
 * fuse_port_dis[64:0]
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_port_disable(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           uint64_t *map_hi,
                                           uint64_t *map_lo) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, 0);

  if (dev_p == NULL) return 0xFFFFFFFF;
  if (dev_p->dev_family != BF_DEV_FAMILY_TOFINO3) {
    return (lld_efuse_get_port_disable(dev_id, map_hi, map_lo));
  }
  if (map_lo == NULL) {
    return 0xFFFFFFFF;
  }
  if (subdev_id != 0) {
    dev_p = lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);
    if (dev_p == NULL) return 0xFFFFFFFF;
  }
  *map_lo = dev_p->efuse_data.port_disable_map_lo;
  if (map_hi != NULL) *map_hi = 0;
  return 0;
}

/****************************************************************************
 * lld_efuse_get_serdes_odd
 ****************************************************************************/
uint32_t lld_efuse_get_serdes_dis_odd(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0;

  return dev_p->efuse_data.tof3_fuse_serdes_odd;
}

/****************************************************************************
 * lld_efuse_get_serdes_even
 ****************************************************************************/
uint32_t lld_efuse_get_serdes_dis_even(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0;

  return dev_p->efuse_data.tof3_fuse_serdes_even;
}

/****************************************************************************
 * lld_efuse_get_die_rotate
 * 1 bit field
 * When set, die is rotated
 * tof2_die_rotate[0]
 ****************************************************************************/
bool lld_efuse_get_die_rotated(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return false;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      return ((dev_p->efuse_data.tof2_die_rotate == 0) ? false : true);
    default:
      return false;
  }
}

/****************************************************************************
 * lld_subdev_efuse_get_port_speed_reduction
 *
 * 2b field
 * fuse_port_speed[1:0]
 * 2'b00 - 100G
 * 2'b01 -  50G
 * 2'b11 -  25G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_port_speed_reduction(bf_dev_id_t dev_id,
                                                   bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.port_speed_reduction;
}

/****************************************************************************
 * lld_efuse_get_port_speed_reduction
 *
 * 2b field
 * fuse_port_speed[1:0]
 * 2'b00 - 100G
 * 2'b01 -  50G
 * 2'b11 -  25G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_port_speed_reduction(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.port_speed_reduction;
}

/****************************************************************************
 * lld_subdev_efuse_get_cpu_port_speed_reduction
 *
 * 2b field
 * fuse_cpu_port_speed[1:0]
 * 2'b00 - 100G
 * 2'b01 -  50G
 * 2'b11 -  25G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_cpu_port_speed_reduction(
    bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.cpu_port_speed_reduction;
}

/****************************************************************************
 * lld_subdev_efuse_get_eth_port_speed_reduction
 *
 * 64b field, 2 bits per port, 32 ports
 * fuse_cpu_port_speed[1:0]
 * 2'b00 - 400G
 * 2'b01 - 100G
 * 2'b10 -  50G
 * 2'b11 -  25G
 * HW auto-config
 ****************************************************************************/
uint64_t lld_subdev_efuse_get_eth_port_speed_reduction(
    bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      return dev_p->efuse_data.tof2_eth_port_speed_reduction;
    case BF_DEV_FAMILY_TOFINO3:
      return dev_p->efuse_data.tof3_eth_port_speed_reduction;
    default:
      return 0xFFFFFFFFFFFFFFFF;
  }
}

/****************************************************************************
 * lld_subdev_efuse_get_pcie_lane_reduction
 *
 * 2b field
 * fuse_pcie_lane_dis[1:0]
 * 2'b00 - 4 lanes
 * 2'b01 - 2 lanes
 * 2'b11 - 1 lane
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_pcie_lane_reduction(bf_dev_id_t dev_id,
                                                  bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.pcie_lane_reduction;
}

/****************************************************************************
 * lld_subdev_efuse_get_packet_generator_disable
 *
 * 1b field
 * When set, packet generator is disabled
 * fuse_pg_dis
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_packet_generator_disable(
    bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.packet_generator_disable;
}

/****************************************************************************
 * lld_subdev_efuse_get_versioning
 *
 * 2b field
 * fuse_version[1:0]
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_versioning(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.versioning;
}

/****************************************************************************
 * lld_efuse_get_frequency_reduction
 *
 * 2b field
 * fuse_freq[1:0]
 * 2'b00 - no constraint
 * 2'b01 - 1.1G
 * 2'b11 - 1G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_frequency_reduction(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.frequency_reduction;
}

/****************************************************************************
 * lld_efuse_get_frequency_reduction_bps
 *
 * 2b field
 * fuse_freq[1:0]
 * Tofino 2:
 *   2'b00 - no constraint
 *   2'b01 - 1.35G
 *   2'b10 - 1.30G
 *   2'b11 - 1.25G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_frequency_reduction_bps(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.frequency_reduction & 3;
}

/****************************************************************************
 * lld_efuse_get_frequency_reduction_pps
 *
 * 2b field
 * fuse_freq[1:0]
 * Tofino 2:
 *   2'b00 - no constraint
 *   2'b01 - 1.5G
 *   2'b10 - 1.3G
 *   2'b11 - 1.1G
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_frequency_reduction_pps(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.frequency_reduction >> 2;
}

/****************************************************************************
 * lld_subdev_efuse_get_resubmit_disable
 *
 * 1b field
 * When set, resubmit function is disabled
 * fuse_resubmit_dis
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_resubmit_disable(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.resubmit_disable;
}

/****************************************************************************
 * lld_subdev_efuse_get_baresync_disable
 *
 * 1b field
 * When set, baresync function is disabled
 * fuse_bsync_dis
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_baresync_disable(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.baresync_disable;
}

/****************************************************************************
 * lld_efuse_get_voltage_scaling
 *
 * 3b field
 * fuse_gpio
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_voltage_scaling(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.voltage_scaling;
}

/****************************************************************************
 * lld_subdev_efuse_get_die_config
 *
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_die_config(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.tof3_die_config;
}

/****************************************************************************
 * lld_efuse_get_die_config
 *
 * 1b field
 ****************************************************************************/
uint32_t lld_efuse_get_die_config(bf_dev_id_t dev_id) {
  return (lld_subdev_efuse_get_die_config(dev_id, 0));
}

/****************************************************************************
 * lld_efuse_get_constant0
 *
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_constant0(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.tof3_constant0;
}

/****************************************************************************
 * lld_efuse_get_constant1
 *
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_constant1(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.tof3_constant1;
}

/****************************************************************************
 * lld_subdev_efuse_get_soft_pipe_dis
 *
 * 4b field
 * fuse_gpio
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_soft_pipe_dis(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.soft_pipe_dis;
}

/****************************************************************************
 * lld_efuse_get_frequency_check_disable
 *
 * 1b field
 * When set, the frequency check logic is disabled
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_frequency_check_disable(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.frequency_check_disable;
}

/****************************************************************************
 * lld_subdev_efuse_get_pmro_and_skew
 *
 * 12b field
 * When set, the frequency check logic is disabled
 * HW auto-config
 ****************************************************************************/
uint32_t lld_subdev_efuse_get_pmro_and_skew(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p =
      lld_map_subdev_id_to_dev_p_allow_unassigned(dev_id, subdev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.pmro_and_skew;
}

/****************************************************************************
 * lld_efuse_get_device_id
 *
 * 16b field
 * PCIe Device ID
 * HW auto-config
 ****************************************************************************/
uint32_t lld_efuse_get_device_id(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (dev_p == NULL) return 0xFFFFFFFF;
  return dev_p->efuse_data.device_id;
}

/****************************************************************************
 * lld_efuse_get_vmin
 *
 ****************************************************************************/
int lld_efuse_get_vmin(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return 0;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
      return 0;
    case BF_DEV_FAMILY_TOFINO3: {
      /* The VMIN fuse field is four eight bit sections with the MSBs having
       * priority over the LSBs.  Find the highest priority field (zero means
       * invalid). */
      uint32_t vmin = dev_p->efuse_data.voltage_scaling;
      if (0xFF000000 & vmin) {
        vmin = vmin >> 24;
      } else if (0x00FF0000 & vmin) {
        vmin = (vmin >> 16) & 0xFF;
      } else if (0x0000FF00 & vmin) {
        vmin = (vmin >> 8) & 0xFF;
      } else {
        vmin &= 0xFF;
      }
      /* Now that the highest priority field has been found check the top bit
       * for the sign and use the lower seven bits for the value. */
      return (0x80 & vmin) ? -1 * (int)(vmin & 0x7F) : (int)vmin;
    }

    case BF_DEV_FAMILY_UNKNOWN:
      return 0;
  }
  return 0;
}

#ifndef __KERNEL__
/****************************************************************************
 * lld_efuse_log_wafer
 *
 ****************************************************************************/
void lld_efuse_wafer_str_get(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             int s_len,
                             char *s) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return;
  if (!s) return;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      lld_efuse_tof_wafer_str_get(dev_id, s_len, s);
      return;
    case BF_DEV_FAMILY_TOFINO2:
      lld_efuse_tof2_wafer_str_get(dev_id, s_len, s);
      return;
    case BF_DEV_FAMILY_TOFINO3:
      lld_efuse_tof3_wafer_str_get(dev_id, subdev_id, s_len, s);
      return;



    default:
      break;
  }
  s[0] = '\0';
  return;
}
#endif

/****************************************************************************
 * lld_efuse_log_settings
 *
 ****************************************************************************/
static void lld_efuse_log_settings(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id) {
  uint64_t hi = 0UL, lo = 0UL;
  lld_log(" Efuse settings dev %d subdev %d", dev_id, subdev_id);
  // TBD for TF3, subdev_id need to passed on to many parsin APIs below

  lld_log("        %8x : resubmit_disable",
          lld_subdev_efuse_get_resubmit_disable(dev_id, subdev_id));
  lld_log("        %8x : mau_tcam_reduction",
          lld_subdev_efuse_get_mau_tcam_reduction(dev_id, subdev_id));
  lld_log("        %8x : mau_sram_reduction",
          lld_subdev_efuse_get_mau_sram_reduction(dev_id, subdev_id));
  lld_log("        %8x : packet_generator_disable",
          lld_subdev_efuse_get_packet_generator_disable(dev_id, subdev_id));
  lld_log("        %8x : pipe_disable", lld_efuse_get_pipe_disable(dev_id));

  lld_efuse_get_port_disable(dev_id, &hi, &lo);
  if (lld_dev_is_tofino(dev_id)) {
    lld_log("        %8x : mau_stage_disable",
            lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 0));
#ifdef __KERNEL__
    lld_log("%llx : port_disable (hi)", hi);
    lld_log("%llx : port_disable (lo)", lo);
    lld_log("%llx : tm_memory_disable",
            lld_subdev_efuse_get_tm_memory_disable(dev_id, subdev_id));
#else
    lld_log("%016" PRIx64 " : port_disable (hi)", hi);
    lld_log("%016" PRIx64 " : port_disable (lo)", lo);
    lld_log("%016" PRIx64 " : tm_memory_disable",
            lld_subdev_efuse_get_tm_memory_disable(dev_id, subdev_id));
#endif
    lld_log("        %8x : port_speed_reduction",
            lld_subdev_efuse_get_port_speed_reduction(dev_id, subdev_id));
  } else if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    lld_log("        %8x : soft_pipe_disable",
            lld_subdev_efuse_get_soft_pipe_dis(dev_id, subdev_id));
    lld_log("        %8x : rotated", lld_efuse_get_die_rotated(dev_id));
    lld_log("        %8x : mau_stage_disable phy-pipe 0",
            lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 0));
    lld_log("        %8x : mau_stage_disable phy-pipe 1",
            lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 1));
    lld_log("        %8x : mau_stage_disable phy-pipe 2",
            lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 2));
    lld_log("        %8x : mau_stage_disable phy-pipe 3",
            lld_subdev_efuse_get_mau_stage_disable(dev_id, subdev_id, 3));
#ifdef __KERNEL__
    lld_log("%llx : port_disable", lo);
    lld_log("%llx : tm_memory_disable",
            lld_subdev_efuse_get_tm_memory_disable(dev_id, subdev_id));
    lld_log("%llx : port_speed_reduction",
            lld_subdev_efuse_get_eth_port_speed_reduction(dev_id, subdev_id));
#else
    lld_log("%016" PRIx64 " : port_disable", lo);
    lld_log("%016" PRIx64 " : tm_memory_disable",
            lld_subdev_efuse_get_tm_memory_disable(dev_id, subdev_id));
    lld_log("%016" PRIx64 " : port_speed_reduction",
            lld_subdev_efuse_get_eth_port_speed_reduction(dev_id, subdev_id));
#endif
  } else {
    return;
  }
  lld_log("        %8x : cpu_port_speed_reduction",
          lld_subdev_efuse_get_cpu_port_speed_reduction(dev_id, subdev_id));
  lld_log("        %8x : pcie_lane_reduction",
          lld_subdev_efuse_get_pcie_lane_reduction(dev_id, subdev_id));
  lld_log("        %8x : baresync_disable",
          lld_subdev_efuse_get_baresync_disable(dev_id, subdev_id));
  if (lld_dev_is_tofino(dev_id)) {
    lld_log("        %8x : frequency_reduction",
            lld_efuse_get_frequency_reduction(dev_id));
  } else if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    lld_log("        %8x : BPS frequency_reduction BPS",
            lld_efuse_get_frequency_reduction_bps(dev_id));
    lld_log("        %8x : PPS frequency_reduction PPS",
            lld_efuse_get_frequency_reduction_pps(dev_id));
  }
  lld_log("        %8x : frequency_check_disable",
          lld_efuse_get_frequency_check_disable(dev_id));
  lld_log("        %8x : versioning",
          lld_subdev_efuse_get_versioning(dev_id, subdev_id));
  if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
    lld_log("        %8x : device_id", lld_efuse_get_device_id(dev_id));
  }

  lld_log("        %8x : chip_part_number",
          lld_subdev_efuse_get_chip_part_number(dev_id, subdev_id));
  lld_log("        %8x : part_revision_number",
          lld_subdev_efuse_get_part_revision_number(dev_id, subdev_id));
  lld_log("        %8x : package_id", lld_efuse_get_package_id(dev_id));
#ifdef __KERNEL__
  lld_log("%llx : chip_id", lld_subdev_efuse_get_chip_id(dev_id, subdev_id));
#else
  lld_log("%016" PRIx64 " : chip_id",
          lld_subdev_efuse_get_chip_id(dev_id, subdev_id));
#endif
  lld_log("        %8x : pmro_and_skew",
          lld_subdev_efuse_get_pmro_and_skew(dev_id, subdev_id));
  lld_log("        %8x : voltage_scaling",
          lld_efuse_get_voltage_scaling(dev_id));
  lld_log("        %8x : die-config",
          lld_subdev_efuse_get_die_config(dev_id, subdev_id));
  lld_log("        %8x : constant0",
          lld_efuse_get_constant0(dev_id, subdev_id));
  lld_log("        %8x : constant1",
          lld_efuse_get_constant1(dev_id, subdev_id));
  lld_log("        %8x : serdes disable even",
          lld_efuse_get_serdes_dis_even(dev_id, subdev_id));
  lld_log("        %8x : serdes disable odd",
          lld_efuse_get_serdes_dis_odd(dev_id, subdev_id));

#ifndef __KERNEL__
  {
    char wafer_str[256] = {0};
    lld_efuse_wafer_str_get(dev_id, subdev_id, 256, wafer_str);
    lld_log("Wafer-id: %s", wafer_str);
  }
#endif
}
