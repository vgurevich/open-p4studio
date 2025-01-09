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
#include <lld/lld_interrupt_if.h>
#include <bf_types/bf_types.h>
#include "port_mgr.h"
#include "port_mgr_dev.h"
#include "port_mgr_map.h"
#include "port_mgr_tof1/port_mgr_tof1_dev.h"
#include "port_mgr_tof1/port_mgr_tof1_port.h"
#include "port_mgr_tof1/port_mgr_tof1_ha.h"
#include "port_mgr_tof2/port_mgr_tof2_dev.h"
#include "port_mgr_tof3/port_mgr_tof3_dev.h"
#include "port_mgr_log.h"

bool port_mgr_dev_is_family_tof1(bf_dev_family_t dev_family);
bool port_mgr_dev_is_family_tof2(bf_dev_family_t dev_family);
bool port_mgr_dev_is_family_tof3(bf_dev_family_t dev_family);

/********************************************************************
 * \brief port_mgr_dev_add
 *         Add a device to the port mgr database
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : dev_id added successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid firmware filename
 ********************************************************************/
bf_status_t port_mgr_dev_add(bf_dev_id_t dev_id,
                             bf_dev_family_t dev_family,
                             bf_device_profile_t *profile,
                             struct bf_dma_info_s *dma_info,
                             bf_dev_init_mode_t warm_init_mode) {
  port_mgr_log("%s:%d:%d:%d(warm_init_mode)",
               __func__,
               __LINE__,
               dev_id,
               warm_init_mode);
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *ldev_p;

  if (dev_p != NULL) return BF_INVALID_ARG;  // already assigned

  dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;  // invalid dev_id?

  ldev_p = &dev_p->ldev;
  ldev_p->dev_family = dev_family;
  // Initialize the bottom half processing flag to false
  ldev_p->port_intr_bhalf_valid = false;

  if (port_mgr_dev_is_family_tof1(dev_family)) {
    return port_mgr_tof1_dev_add(
        dev_id, dev_family, profile, dma_info, warm_init_mode);
  } else if (port_mgr_dev_is_family_tof2(dev_family)) {
    return port_mgr_tof2_dev_add(
        dev_id, dev_family, profile, dma_info, warm_init_mode);
  } else if (port_mgr_dev_is_family_tof3(dev_family)) {
    return port_mgr_tof3_dev_add(  // TBD TF3 fix for dma_info
        dev_id,
        dev_family,
        profile,
        dma_info,
        warm_init_mode);
  }
  return BF_SUCCESS;
}

/********************************************************************
 * \brief port_mgr_dev_remove
 *         Remove a device from the port mgr database
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS : dev_id removed successfully
 ********************************************************************/
bf_status_t port_mgr_dev_remove(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_dev_remove(dev_id);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    return port_mgr_tof2_dev_remove(dev_id);
  } else if (port_mgr_dev_is_tof3(dev_id)) {
    return port_mgr_tof3_dev_remove(dev_id);
  }
  return BF_SUCCESS;
}

/********************************************************************
 * port_mgr_dev_is_family_tof1
 ********************************************************************/
bool port_mgr_dev_is_family_tof1(bf_dev_family_t dev_family) {
  return (dev_family == BF_DEV_FAMILY_TOFINO);
}

/********************************************************************
 * port_mgr_dev_is_family_tof2
 ********************************************************************/
bool port_mgr_dev_is_family_tof2(bf_dev_family_t dev_family) {
  return (dev_family == BF_DEV_FAMILY_TOFINO2);
}

/********************************************************************
 * port_mgr_dev_is_family_tof3
 ********************************************************************/
bool port_mgr_dev_is_family_tof3(bf_dev_family_t dev_family) {
  return (dev_family == BF_DEV_FAMILY_TOFINO3);
}

/********************************************************************
 * port_mgr_dev_is_tof1
 *
 * Determine whether the passed device ID is of type Tofino 1
 * or not.
 ********************************************************************/
bool port_mgr_dev_is_tof1(bf_dev_id_t dev_id) {
  port_mgr_ldev_t *ldev_p = port_mgr_map_dev_id_to_ldev_p(dev_id);

  if (ldev_p == NULL) return false;

  return (port_mgr_dev_is_family_tof1(ldev_p->dev_family));
}

/********************************************************************
 * port_mgr_dev_is_tof2
 *
 * Determine whether the passed device ID is of type Tofino 2
 * or not.
 ********************************************************************/
bool port_mgr_dev_is_tof2(bf_dev_id_t dev_id) {
  port_mgr_ldev_t *ldev_p = port_mgr_map_dev_id_to_ldev_p(dev_id);

  if (ldev_p == NULL) return false;

  return (port_mgr_dev_is_family_tof2(ldev_p->dev_family));
}
/********************************************************************
 * port_mgr_dev_is_tof3
 *
 * Determine whether the passed device ID is of type Tofino 3
 * or not.
 ********************************************************************/
bool port_mgr_dev_is_tof3(bf_dev_id_t dev_id) {
  port_mgr_ldev_t *ldev_p = port_mgr_map_dev_id_to_ldev_p(dev_id);

  if (ldev_p == NULL) return false;

  return (port_mgr_dev_is_family_tof3(ldev_p->dev_family));
}

/********************************************************************
 * port_mgr_dev_is_ready
 *
 * Return "locked" status of the device
 ********************************************************************/
int port_mgr_dev_is_ready(bf_dev_id_t dev_id) {
  port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);

  if (!dev_p) return 0;
  if (!port_mgr_dev_assigned_get(dev_id)) return 0;

  return port_mgr_dev_ready_get(dev_id);
}

/********************************************************************
 * \brief Register a callback function to be issued on port "events".
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param fn      : function to be called
 * \param userdata: opaque user data (cookie)
 *
 * \return: BF_SUCCESS     : callback added successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 ********************************************************************/
bf_status_t port_mgr_register_port_cb(bf_dev_id_t dev_id,
                                      port_mgr_port_callback_t fn,
                                      void *userdata) {
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_dev_register_port_cb(dev_id, fn, userdata);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    return port_mgr_tof2_dev_register_port_cb(dev_id, fn, userdata);
  } else if (port_mgr_dev_is_tof3(dev_id)) {
    return port_mgr_tof3_dev_register_port_cb(dev_id, fn, userdata);
  }
  return BF_INVALID_ARG;
}

/********************************************************************
 * port_mgr_dev_enable_all_dr
 *
 * Enable all the DRs used by the port manager
 ********************************************************************/
void port_mgr_dev_enable_all_dr(bf_dev_id_t dev_id) {
  lld_dr_enable_set(dev_id, lld_dr_tx_mac_stat, true);
  lld_dr_enable_set(dev_id, lld_dr_cmp_mac_stat, true);
}

/********************************************************************
 * port_mgr_dev_disable_all_dr
 *
 * Disable all the DRs used by the port manager
 ********************************************************************/
void port_mgr_dev_disable_all_dr(bf_dev_id_t dev_id) {
  lld_dr_enable_set(dev_id, lld_dr_tx_mac_stat, false);
  lld_dr_enable_set(dev_id, lld_dr_cmp_mac_stat, false);
}

/********************************************************************
 * port_mgr_dev_lock
 *
 * "Lock" the device in preparation for an HA event
 ********************************************************************/
bf_status_t port_mgr_dev_lock(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }
  port_mgr_ctx->dev[dev_id].ldev.dev_locked = true;

  return BF_SUCCESS;
}

/********************************************************************
 * port_mgr_dev_unlock
 *
 * "Un-Lock" the device at the conclusion of an HA event
 ********************************************************************/
bf_status_t port_mgr_dev_unlock(bf_dev_id_t dev_id) {
  bf_subdev_id_t subdev_id = 0;  // TBD tf3-fix
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }
  port_mgr_ctx->dev[dev_id].ldev.dev_locked = false;

  // FIXME: This should be in the calling function, not here
  // Enable the DR
  port_mgr_dev_enable_all_dr(dev_id);

  // Enable interrupts
  port_mgr_ldev_t *ldev_p = port_mgr_map_dev_id_to_ldev_p(dev_id);
  if (ldev_p && port_mgr_dev_is_family_tof1(ldev_p->dev_family)) {
    bf_status_t status = bf_int_ena_mbus(dev_id, subdev_id);
    if (status != BF_SUCCESS) {
      port_mgr_log_error(
          "interrupt enable with LLD failed for device %d, sts %s (%d)",
          dev_id,
          bf_err_str(status),
          status);
      return status;
    }
  }

  return BF_SUCCESS;
}

/********************************************************************
 * port_mgr_dev_is_locked
 *
 * Return "locked" status of the device
 ********************************************************************/
bool port_mgr_dev_is_locked(bf_dev_id_t dev_id) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    return false;
  }
  return (port_mgr_ctx->dev[dev_id].ldev.dev_locked);
}

/********************************************************************
 * port_mgr_dev_ready_get
 *
 * Return "ready" status of the device
 ********************************************************************/
bool port_mgr_dev_ready_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.ready;
}

/********************************************************************
 * port_mgr_dev_ready_set
 *
 * Set "ready" status of the device
 ********************************************************************/
void port_mgr_dev_ready_set(bf_dev_id_t dev_id, bool st) {
  port_mgr_ctx->dev[dev_id].ldev.ready = st;
}

/********************************************************************
 * port_mgr_dev_assigned_get
 *
 * Return "assigned" status of the device
 ********************************************************************/
bool port_mgr_dev_assigned_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.assigned;
}

/********************************************************************
 * port_mgr_dev_assigned_set
 *
 * Set "assigned" status of the device
 ********************************************************************/
void port_mgr_dev_assigned_set(bf_dev_id_t dev_id, bool st) {
  port_mgr_ctx->dev[dev_id].ldev.assigned = st;
}

/********************************************************************
 * port_mgr_dev_family_get
 *
 * Return "dev family" status of the device
 ********************************************************************/
bf_dev_family_t port_mgr_dev_family_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.dev_family;
}

/********************************************************************
 * port_mgr_dev_family_set
 *
 * Set "dev_family" of the device
 ********************************************************************/
void port_mgr_dev_family_set(bf_dev_id_t dev_id, bf_dev_family_t dev_family) {
  port_mgr_ctx->dev[dev_id].ldev.dev_family = dev_family;
}

/********************************************************************
 * port_mgr_dev_part_rev_get
 *
 * Return "part_rev" of the device
 ********************************************************************/
bf_sku_chip_part_rev_t port_mgr_dev_part_rev_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.part_rev;
}

/********************************************************************
 * port_mgr_dev_part_rev_set
 *
 * Set "part_rev" of the device
 ********************************************************************/
void port_mgr_dev_part_rev_set(bf_dev_id_t dev_id,
                               bf_sku_chip_part_rev_t part_rev) {
  port_mgr_ctx->dev[dev_id].ldev.part_rev = part_rev;
}

/********************************************************************
 * port_mgr_dev_dma_info_get
 *
 * Return DMA info associated with the device
 ********************************************************************/
struct bf_dma_info_s *port_mgr_dev_dma_info_get(bf_dev_id_t dev_id) {
  return &port_mgr_ctx->dev[dev_id].ldev.dma_info;
}

/********************************************************************
 * port_mgr_dev_dma_info_set
 *
 * Set "dma_info" to be associated with the device
 ********************************************************************/
void port_mgr_dev_dma_info_set(bf_dev_id_t dev_id,
                               struct bf_dma_info_s *dma_info) {
  port_mgr_ctx->dev[dev_id].ldev.dma_info = *dma_info;
}

/********************************************************************
 * port_mgr_dev_ha_stage_get
 *
 * Return ha_stage associated with the device
 ********************************************************************/
port_mgr_ha_stages_t port_mgr_dev_ha_stage_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.ha_stage;
}

/********************************************************************
 * port_mgr_dev_ha_stage_set
 *
 * Set "ha_stage" to be associated with the device
 ********************************************************************/
void port_mgr_dev_ha_stage_set(bf_dev_id_t dev_id,
                               port_mgr_ha_stages_t ha_stage) {
  port_mgr_ctx->dev[dev_id].ldev.ha_stage = ha_stage;
}

/********************************************************************
 * port_mgr_dev_init_mode_get
 *
 * Return "init_mode" associated with the device
 ********************************************************************/
bf_dev_init_mode_t port_mgr_dev_init_mode_get(bf_dev_id_t dev_id) {
  return port_mgr_ctx->dev[dev_id].ldev.init_mode;
}

/********************************************************************
 * port_mgr_dev_init_mode_set
 *
 * Set "init_mode" to be associated with the device
 ********************************************************************/
void port_mgr_dev_init_mode_set(bf_dev_id_t dev_id,
                                bf_dev_init_mode_t init_mode) {
  port_mgr_ctx->dev[dev_id].ldev.init_mode = init_mode;
}

/********************************************************************
 * port_mgr_dev_port_cb_get
 *
 * Return port callback info associated with the device
 ********************************************************************/
void port_mgr_dev_port_cb_get(bf_dev_id_t dev_id,
                              port_mgr_port_callback_t *fn,
                              void **userdata) {
  *fn = port_mgr_ctx->dev[dev_id].ldev.port_cb;
  *userdata = port_mgr_ctx->dev[dev_id].ldev.port_cb_userdata;
  return;
}

/********************************************************************
 * port_mgr_dev_port_cb_set
 *
 * Set "port callback data"" to be associated with the device
 ********************************************************************/
void port_mgr_dev_port_cb_set(bf_dev_id_t dev_id,
                              port_mgr_port_callback_t fn,
                              void *userdata) {
  port_mgr_ctx->dev[dev_id].ldev.port_cb = fn;
  port_mgr_ctx->dev[dev_id].ldev.port_cb_userdata = userdata;
}

/********************************************************************
 * port_mgr_dev_logical_dev_get
 *
 * Return logical device associated with the device
 ********************************************************************/
port_mgr_ldev_t *port_mgr_dev_logical_dev_get(bf_dev_id_t dev_id) {
  return &port_mgr_ctx->dev[dev_id].ldev;
}

/********************************************************************
 * port_mgr_dev_pdev_get
 *
 * Return physical device associated with the device
 ********************************************************************/
port_mgr_pdev_t *port_mgr_dev_pdev_get(bf_dev_id_t dev_id) {
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) {
    port_mgr_log_error(
        "%s:%d:Incorrect dev_id: %d", __func__, __LINE__, dev_id);
    return NULL;
  }

  return &port_mgr_ctx->dev[dev_id].pdev;
}

/********************************************************************
 * port_mgr_dev_physical_dev_get
 *
 * Return physical device associated with the device
 ********************************************************************/
port_mgr_tof1_pdev_t *port_mgr_dev_physical_dev_get(bf_dev_id_t dev_id) {
  port_mgr_pdev_t *pdev;

  pdev = port_mgr_dev_pdev_get(dev_id);
  return &pdev->u.pdev_tof1;
}

/********************************************************************
 * port_mgr_dev_physical_dev_tof2_get
 *
 * Return physical device associated with the device
 ********************************************************************/
port_mgr_tof2_pdev_t *port_mgr_dev_physical_dev_tof2_get(bf_dev_id_t dev_id) {
  port_mgr_pdev_t *pdev;

  pdev = port_mgr_dev_pdev_get(dev_id);
  return &pdev->u.pdev_tof2;
}

/********************************************************************
 * port_mgr_dev_physical_dev_tof3_get
 *
 * Return physical device associated with the device
 ********************************************************************/
void *port_mgr_dev_physical_dev_tof3_get(bf_dev_id_t dev_id) {
	return NULL;
}
