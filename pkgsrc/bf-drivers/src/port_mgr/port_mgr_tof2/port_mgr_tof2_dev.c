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


#include <bf_types/bf_types.h>
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
#include <lld/lld_reg_if.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_dev.h>
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_tof2_microp.h"
#include "port_mgr_tof2_umac.h"
#include <port_mgr/port_mgr_tof2/port_mgr_tof2_serdes_defs.h>
#include <port_mgr/bf_tof2_serdes_if.h>
#include <port_mgr/port_mgr_tof2/port_mgr_tof2_gpio.h>
#include <port_mgr/port_mgr_tof2/port_mgr_tof2_port.h>
#include <tof2_regs/tof2_reg_drv.h>

static void port_mgr_tof2_init_port_mutex(bf_dev_id_t dev_id);
extern void port_mgr_tof2_serdes_fw_cmd_lock_init(void);

/** \brief port_mgr_tof_2_dev_add
 *         Add a Tofino2 chip to the system
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : dev_id added successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid firmware filename
 *
 */
bf_status_t port_mgr_tof2_dev_add(bf_dev_id_t dev_id,
                                  bf_dev_family_t dev_family,
                                  bf_device_profile_t *profile,
                                  struct bf_dma_info_s *dma_info,
                                  bf_dev_init_mode_t warm_init_mode) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  bf_status_t rc = BF_SUCCESS;
  lld_err_t lld_err;
  bf_sku_chip_part_rev_t part_rev;
  bf_subdev_id_t subdev_id = 0;

  if (dev_p == NULL) return BF_INVALID_ARG;

  if (!port_mgr_dev_is_locked(dev_id)) {
    // Enable all the DR
    port_mgr_dev_enable_all_dr(dev_id);
  }
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
    port_mgr_dev_dma_info_set(dev_id, dma_info);
    port_mgr_init_mac_stats(dev_id);
  }
  // set default MAC interrupt handler
  // note: must be done after efuse info is loaded so the
  //       mapping between mac-blk and bf_port_id is known
  // port_mgr_mac_register_default_handler_for_mac_ints(dev_id);
  rc = port_mgr_tof2_port_register_default_handler_for_mac_ints(dev_id);
  if (rc) {
    port_mgr_log_error(
        "%d: Error %d: setting default port interrupt handler", dev_id, rc);
  }

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Initialize the port mutexes
    port_mgr_tof2_init_port_mutex(dev_id);
    // init the fw_cmd mutex
    port_mgr_tof2_serdes_fw_cmd_lock_init();
  }

  /* If this is a warm init read configuration from hardware
     and save in port_p->hw. This will be reconciled with
     port_p->sw at the end of the warm init replay */
  if ((warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
      (warm_init_mode == BF_DEV_WARM_INIT_HITLESS)) {
    port_mgr_dev_ha_stage_set(dev_id, PORT_MGR_HA_CFG_REPLAY);
  } else {
    // disable all mac interrupts
    bf_port_mac_int_all_disable(dev_id);
  }

  // save FW path names
  char *fw_file_name = profile->sds_prof.tof2_serdes_grp_0_7_fw;
  char *fw_grp8_file_name = profile->sds_prof.tof2_serdes_grp_8_fw;
  port_mgr_tof2_pdev_t *pdev = port_mgr_dev_physical_dev_tof2_get(dev_id);
  pdev->fw_grp_0_7_path = fw_file_name;
  pdev->fw_grp_8_path = fw_grp8_file_name;

  if (!port_mgr_dev_is_locked(dev_id)) {
    bool skip_group_reset = false;
    bool skip_post = true;
    bool force_fw_dnld = false;
    bool skip_power_dn = false;
    bool is_sw_model;
    bf_serdes_process_corner_t part_type;

    bf_drv_device_type_get(dev_id, &is_sw_model);
    if (!is_sw_model) {
      port_mgr_tof2_microp_init(dev_id, profile);
      port_mgr_tof2_gpio_tile_init(dev_id, BF_TOF2_MDIOCI_CLK_DIV);
      port_mgr_tof2_umac_init(dev_id, BF_TOF2_MDIOCI_CLK_DIV);

      {
        uint32_t dro[4] = {0};  // one per tile, most-likely DRO value
        int32_t max_dro = 0;
        uint32_t num_dro_values = 0;

        rc =
            bf_tof2_serdes_tile_dro_get(dev_id, dro, &max_dro, &num_dro_values);
        if (rc == BF_SUCCESS) {
          port_mgr_log("DRO[%d] : 0: %d, 1: %d, 2: %d, 3: %d",
                       num_dro_values,
                       dro[0],
                       dro[1],
                       dro[2],
                       dro[3]);
        }
      }
      rc = bf_tof2_serdes_part_type_get(dev_id, &part_type);
      if (rc == BF_SUCCESS) {
        port_mgr_log("Tile Corner: %s",
                     (part_type == PROCESS_CORNER_SS)
                         ? "SS (slow)"
                         : (part_type == PROCESS_CORNER_TT)
                               ? "TT (typical)"
                               : (part_type == PROCESS_CORNER_FF)
                                     ? "FF (fast)"
                                     : (part_type == PROCESS_CORNER_UNDEF)
                                           ? "undefined"
                                           : "error");
      } else {
        port_mgr_log("Tile Corner: <error: determination not possible>");
      }

      port_mgr_log("FW path (grp 0-7) : %s", fw_file_name);
      port_mgr_log("FW path (grp 8)   : %s", fw_grp8_file_name);
      bf_tof2_serdes_tile_init(dev_id,
                               fw_file_name,
                               fw_grp8_file_name,
                               skip_group_reset,
                               skip_post,
                               force_fw_dnld,
                               skip_power_dn);
    }
    // Clear the state for all the port
    port_mgr_tof2_set_default_all_ports_state(dev_id);
  } else {
    // port_mgr_serdes_init_warm_boot(dev_id);
  }

  // finally, mark it ready
  port_mgr_dev_ready_set(dev_id, true);

  // Claim the port manager interrupts
  // Enable the port manager interrupts
  rc = bf_int_claim_mbus(dev_id, subdev_id);
  if (rc != BF_SUCCESS) {
    port_mgr_log_error(
        "interrupt registration with LLD failed for device %d, sts %s (%d)",
        dev_id,
        bf_err_str(rc),
        rc);
    return rc;
  }

  // Enable the port manager interrupts
  rc = bf_int_ena_mbus(dev_id, subdev_id);
  if (rc != BF_SUCCESS) {
    port_mgr_log_error(
        "interrupt enable with LLD failed for device %d, sts %s (%d)",
        dev_id,
        bf_err_str(rc),
        rc);
    return rc;
  }

  return BF_SUCCESS;
}

/** \brief port_mgr_tof2_dev_remove
 *         Remove a Tofino device from the system
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS : dev_id removed successfully
 *
 */
bf_status_t port_mgr_tof2_dev_remove(bf_dev_id_t dev_id) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  // Disable all the DR
  // port_mgr_dev_disable_all_dr(dev_id);

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Free the DMA memory pool used for MAC STATS
    port_mgr_free_mac_stats(dev_id);
  }

  // Clear the state for all the port
  port_mgr_tof2_set_default_all_ports_state(dev_id);

  // tbd, HW cleanup..
  if (dev_p != NULL) {
    port_mgr_dev_assigned_set(dev_id, false);
    port_mgr_dev_ready_set(dev_id, false);
  }

  if (!bf_drv_is_device_virtual(dev_id)) {
    // Deinitialize the port mutexes
    // port_mgr_deinit_port_mtx(dev_id);
  }

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
bf_status_t port_mgr_tof2_dev_register_port_cb(bf_dev_id_t dev_id,
                                               port_mgr_port_callback_t fn,
                                               void *userdata) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);

  if (dev_p != NULL) {
    port_mgr_dev_port_cb_set(dev_id, fn, userdata);
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

static void port_mgr_tof2_init_port_mutex(bf_dev_id_t dev_id) {
  uint32_t port;
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (!dev_p) return;
  for (port = 0;
       port < sizeof(dev_p->ldev.lport) / sizeof(dev_p->ldev.lport[0]);
       port++) {
    bf_sys_mutex_init(&dev_p->ldev.lport[port].port_mtx);
  }
}

/** \brief port_mgr_tof2_dev_remove
 *         Remove a Tofino device from the system
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS : dev_id removed successfully
 *
 */
bf_status_t port_mgr_tof2_dev_fw_get(bf_dev_id_t dev_id,
                                     char **fw_grp_0_7,
                                     char **fw_grp_8) {
  port_mgr_tof2_pdev_t *pdev;

  // save FW path names
  pdev = port_mgr_dev_physical_dev_tof2_get(dev_id);
  if (pdev == NULL) return BF_INVALID_ARG;

  *fw_grp_0_7 = pdev->fw_grp_0_7_path;
  *fw_grp_8 = pdev->fw_grp_8_path;
  return BF_SUCCESS;
}
