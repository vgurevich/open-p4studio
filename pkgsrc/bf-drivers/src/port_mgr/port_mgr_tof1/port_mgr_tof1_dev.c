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


#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/bf_dma_dr_id.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_interrupt_if.h>
#include <bf_types/bf_types.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_dev.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_tof1_port.h"
#include "port_mgr_tof1_map.h"
#include "port_mgr_tof1.h"
#include "port_mgr_mac.h"
#include "port_mgr_serdes.h"

// fwd refs
bf_status_t port_mgr_enable_eth_cpu_port(bf_dev_id_t dev_id);
bool port_mgr_dev_is_locked(bf_dev_id_t dev_id);
bf_status_t port_mgr_dev_add(bf_dev_id_t dev_id,
                             bf_dev_family_t dev_family,
                             bf_device_profile_t *profile,
                             struct bf_dma_info_s *dma_info,
                             bf_dev_init_mode_t warm_init_mode);
bf_status_t port_mgr_dev_remove(bf_dev_id_t dev_id);
void port_mgr_mac_stat_dma_callback(bf_dev_id_t dev_id,
                                    int status,
                                    uint64_t msg_id);
#ifndef UTEST
static bf_status_t port_mgr_cache_sds_fw_info(bf_dev_id_t dev_id,
                                              bf_device_profile_t *profile);
#endif

/** \brief port_mgr_tof_1_dev_add
 *         Add a Tofino chip to the system
 *
 * \param dev_id: bf_dev_id_t: system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : dev_id added successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid firmware filename
 *
 */
bf_status_t port_mgr_tof1_dev_add(bf_dev_id_t dev_id,
                                  bf_dev_family_t dev_family,
                                  bf_device_profile_t *profile,
                                  struct bf_dma_info_s *dma_info,
                                  bf_dev_init_mode_t warm_init_mode) {
  (void)warm_init_mode;
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  bf_status_t status = BF_SUCCESS;
  lld_err_t lld_err;
  bf_sku_chip_part_rev_t part_rev;
  bf_subdev_id_t subdev_id = 0;
  if (dev_p == NULL) return BF_INVALID_ARG;

  if (!port_mgr_dev_is_locked(dev_id)) {
    // Enable all the DR
    port_mgr_dev_enable_all_dr(dev_id);
  }

  port_mgr_tof1_init();

  port_mgr_dev_family_set(dev_id, dev_family);
  port_mgr_dev_ready_set(dev_id, false);
  port_mgr_dev_assigned_set(dev_id, true);

  /* Get the chip part revision number. */
  lld_err = lld_sku_get_chip_part_revision_number(dev_id, &part_rev);
  if (lld_err != LLD_OK) {
    port_mgr_log_error(
        "Not able to get chip part revision number, dev %d sts %d",
        dev_id,
        lld_err);
    return BF_UNEXPECTED;
  }

  port_mgr_dev_part_rev_set(dev_id, part_rev);
  port_mgr_log_trace(
      "set chip part revision number, dev %d, part rev %d", dev_id, part_rev);

  if (dma_info) {
    port_mgr_dev_dma_info_set(dev_id, dma_info);  // note: structure assignment
    port_mgr_init_mac_stats(dev_id);
  }
#ifndef UTEST
  // cache serdes firmware info from the profile
  status = port_mgr_cache_sds_fw_info(dev_id, profile);
  if (status != BF_SUCCESS) return BF_INVALID_ARG;
#else
  (void)profile;
#endif

  // One-time mac-block inits (currently only min_thr for 10g)
  status = port_mgr_mac_block_init(dev_id);
  if (status != BF_SUCCESS) {
    /* Error is already logged, just return */
    return status;
  }

  // set default MAC interrupt handler
  // note: must be done after efuse info is loaded so the
  //       mapping between mac-blk and bf_port_id is known
  port_mgr_mac_register_default_handler_for_mac_ints(dev_id);

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Initialize the port mutexes
    port_mgr_init_port_mtx(dev_id);
  }

  /* If this is a warm init read configuration from hardware
     and save in port_p->hw. This will be reconciled with
     port_p->sw at the end of the warm init replay */
  if ((warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
      (warm_init_mode == BF_DEV_WARM_INIT_HITLESS)) {
    port_mgr_dev_ha_stage_set(dev_id, PORT_MGR_HA_CFG_REPLAY);
  } else {
    // disable all mac interrupts
    port_mgr_port_disable_all(dev_id);
  }

  // Cache the dev init mode
  port_mgr_dev_init_mode_set(dev_id, warm_init_mode);

  if (!port_mgr_dev_is_locked(dev_id)) {
    status = port_mgr_serdes_init(dev_id);
    if (status != BF_SUCCESS) {
      return status;
    }
  } else {
    port_mgr_serdes_init_warm_boot(dev_id);
  }
  // finally, mark it ready
  port_mgr_dev_ready_set(dev_id, true);

  // Claim the port manager interrupts
  status = bf_int_claim_mbus(dev_id, subdev_id);
  if (status != BF_SUCCESS) {
    port_mgr_log_error(
        "interrupt registration with LLD failed for device %d, sts %s (%d)",
        dev_id,
        bf_err_str(status),
        status);
    return status;
  }

  // Enable the port manager interrupts
  status = bf_int_ena_mbus(dev_id, subdev_id);
  if (status != BF_SUCCESS) {
    port_mgr_log_error(
        "interrupt enable with LLD failed for device %d, sts %s (%d)",
        dev_id,
        bf_err_str(status),
        status);
    return status;
  }

  return BF_SUCCESS;
}

/** \brief port_mgr_dev_remove
 *         Remove a Tofino device from the system
 *
 * \param dev_id: bf_dev_id_t : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS : dev_id removed successfully
 *
 */
bf_status_t port_mgr_tof1_dev_remove(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  // Disable all the DR
  port_mgr_dev_disable_all_dr(dev_id);

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Free the DMA memory pool used for MAC STATS
    port_mgr_free_mac_stats(dev_id);
  }

  // Clear the state for all the port
  port_mgr_tof1_set_default_all_ports_state(dev_id);

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Deinitialize the port mutexes
    port_mgr_deinit_port_mtx(dev_id);
  }

  // tbd, HW cleanup..
  if (dev_p != NULL) {
    port_mgr_dev_assigned_set(dev_id, false);
    port_mgr_dev_ready_set(dev_id, false);
  }

  // Reset the dev init mode to 0 (same as cold boot)
  port_mgr_dev_init_mode_set(dev_id, 0);

  return BF_SUCCESS;
}

/** \brief Register a callback function to be issued on port "events".
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param fn      : port_mgr_port_callback_t: function to be called
 * \param userdata: void *                  : opaque user data (cookie)
 *
 * \return: BF_SUCCESS     : callback added successfully
 * \return: BF_INVALID_ARG : Too many dev_ids added or dev_id >
 *BF_MAX_DEV_COUNT-1
 *
 */
bf_status_t port_mgr_tof1_dev_register_port_cb(bf_dev_id_t dev_id,
                                               port_mgr_port_callback_t fn,
                                               void *userdata) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);

  if (dev_p != NULL) {
    port_mgr_dev_port_cb_set(dev_id, fn, userdata);
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

#ifndef UTEST
/**********************************************************************
 * port_mgr_parse_sds_fname
 *
 * Filenames are assumed to contain the fw version and build-id as the
 * last characters with the following form:
 *
 *  "<any-prefix><version>_<build-id>.rom"
 *
 * Where "version" and "build-id" are 4 hexadecimal digits each.
 *
 * For example: "serdes.0x1066_204D.rom"
 *
 * So, fname[ strlen(fname) ] is the \NULL after ".rom"
 * The build-id starts at fname[ strlen(fname) - 8 ]
 * The version starts at fname[ strlen(fname) - 13 ]
 */
static void port_mgr_parse_sds_fname(char *fname,
                                     uint32_t *version,
                                     uint32_t *build_id) {
  uint32_t hex_val;
  uint32_t digit;
  uint32_t build_ofs, version_ofs;
  uint32_t len = strlen(fname);

  /* drv-1876 */
  /* If fname is too short, report the build and version as 0 */
  if (len < 13) {
    *build_id = 0;
    *version = 0;
    return;
  }

#define __hex(c)                       \
  (((c) >= '0') && ((c) <= '9'))       \
      ? ((c) - '0')                    \
      : (((c) >= 'a') && ((c) <= 'f')) \
            ? ((c) - 'a' + 10)         \
            : (((c) >= 'A') && ((c) <= 'F')) ? ((c) - 'A' + 10) : 0

  build_ofs = len - 8;
  version_ofs = len - 13;

  hex_val = 0;
  for (digit = 0; digit < 4; digit++) {
    char c = fname[build_ofs + digit];
    uint32_t hex_c = __hex(c);
    hex_val = (hex_val << 4) | hex_c;
  }
  *build_id = hex_val;

  hex_val = 0;
  for (digit = 0; digit < 4; digit++) {
    char c = fname[version_ofs + digit];
    uint32_t hex_c = __hex(c);
    hex_val = (hex_val << 4) | hex_c;
  }
  *version = hex_val;
}

static bf_status_t port_mgr_cache_sds_fw_info(bf_dev_id_t dev_id,
                                              bf_device_profile_t *profile) {
  port_mgr_tof1_pdev_t *dev_p = port_mgr_dev_physical_dev_get(dev_id);

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (profile == NULL) return BF_INVALID_ARG;
  if (profile->sds_prof.sbus_master_fw &&
      (strlen(profile->sds_prof.sbus_master_fw) >=
       sizeof dev_p->sbus_master_fw))
    return BF_INVALID_ARG;
  if (profile->sds_prof.pcie_fw &&
      (strlen(profile->sds_prof.pcie_fw) >= sizeof dev_p->pcie_fw))
    return BF_INVALID_ARG;
  if (profile->sds_prof.serdes_fw &&
      (strlen(profile->sds_prof.serdes_fw) >= sizeof dev_p->serdes_fw))
    return BF_INVALID_ARG;

  if (profile->sds_prof.sbus_master_fw == NULL) {
    dev_p->sbus_master_fw[0] = 0;
    dev_p->sbus_master_fw_ver = 0;
    dev_p->sbus_master_fw_bld = 0;
  } else {
    snprintf(dev_p->sbus_master_fw,
             sizeof(dev_p->sbus_master_fw),
             "%s",
             profile->sds_prof.sbus_master_fw);
    port_mgr_parse_sds_fname(dev_p->sbus_master_fw,
                             &dev_p->sbus_master_fw_ver,
                             &dev_p->sbus_master_fw_bld);
  }

  if (profile->sds_prof.pcie_fw == NULL) {
    dev_p->pcie_fw[0] = 0;
    dev_p->pcie_fw_ver = 0;
    dev_p->pcie_fw_bld = 0;
  } else {
    snprintf(dev_p->pcie_fw,
             sizeof(dev_p->pcie_fw),
             "%s",
             profile->sds_prof.pcie_fw);
    port_mgr_parse_sds_fname(
        dev_p->pcie_fw, &dev_p->pcie_fw_ver, &dev_p->pcie_fw_bld);
  }

  if (profile->sds_prof.serdes_fw == NULL) {
    dev_p->serdes_fw[0] = 0;
    dev_p->serdes_fw_ver = 0;
    dev_p->serdes_fw_bld = 0;
  } else {
    snprintf(dev_p->serdes_fw,
             sizeof(dev_p->serdes_fw),
             "%s",
             profile->sds_prof.serdes_fw);
    port_mgr_parse_sds_fname(
        dev_p->serdes_fw, &dev_p->serdes_fw_ver, &dev_p->serdes_fw_bld);
  }
  port_mgr_log("%d:-:-: sbus master fw: %04x_%04x",
               dev_id,
               dev_p->sbus_master_fw_ver,
               dev_p->sbus_master_fw_bld);
  port_mgr_log("%d:-:-: PCIe serdes fw: %04x_%04x",
               dev_id,
               dev_p->pcie_fw_ver,
               dev_p->pcie_fw_bld);
  port_mgr_log("%d:-:-:      serdes fw: %04x_%04x",
               dev_id,
               dev_p->serdes_fw_ver,
               dev_p->serdes_fw_bld);
  return BF_SUCCESS;
}

#endif
