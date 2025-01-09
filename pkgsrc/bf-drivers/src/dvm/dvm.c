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
#include <sched.h>
#include <string.h>

#include "dvm_log.h"
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <mc_mgr/mc_mgr_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <port_mgr/port_mgr_port_evt.h>
#include <port_mgr/bf_port_if.h>
#include "dvm.h"
#include "bf_drv_ver.h"
#include <lld/lld.h>
#include <lld/lld_map.h>
static bool bf_drv_init_done = false;

dvm_asic_t *dev_id_set[BF_MAX_DEV_COUNT];

/**
 * This function iterates through all corrective actions reported by all the
 * different clients and then decides which action to apply
 */
static void determine_final_corrective_action_this_port(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  bf_ha_port_reconcile_info_t *recon_info, *final_recon_info;
  bf_status_t status;
  int client_id = -1;
  int dev_port_idx = DEV_PORT_TO_BIT_IDX(dev_port);

  if (dev_port_idx >= BF_PORT_COUNT) {
    LOG_ERROR(
        "%s:%d Error, dev_port_idx(%d) not in range [0; "
        "BF_PORT_COUNT(%d)[",
        __func__,
        __LINE__,
        dev_port_idx,
        (int)BF_PORT_COUNT);
    bf_sys_dbgchk(0);
    return;
  }

  /* Determine client(s) for corrective actions.
   *
   * FIXME : Change logic to iterate through all clients to determine the
   *  final corrective actions to be applied finally on the port.
   *  For the time being, only the first applicable client is considered,
   *  which is supposed to be port manage, to determine the final action.
   */

  status = bf_drv_get_get_next_client_for_port_corrective_actions(
      dev_id, -1, &client_id);
  if (status != BF_SUCCESS || client_id == -1) {
    LOG_ERROR(
        "%s:%d Error, dev_port_idx(%d) NO client to determine corrective "
        "actions",
        __func__,
        __LINE__,
        dev_port_idx);
    return;
  }

  LOG_DBG("Determining Final Corr : Dev : %d : Port : %d : Client : %d",
          dev_id,
          dev_port,
          client_id);
  recon_info = &dev_id_set[dev_id]
                    ->port_corr_action.port_reconcile_info[client_id]
                    .recon_info_per_port[dev_port_idx];
  final_recon_info = &dev_id_set[dev_id]
                          ->port_corr_action.final_port_reconcile_info
                          .recon_info_per_port[dev_port_idx];

  final_recon_info->dev_id = recon_info->dev_id;
  final_recon_info->dev_port = recon_info->dev_port;
  final_recon_info->ca = recon_info->ca;
  memcpy(&final_recon_info->port_attrib,
         &recon_info->port_attrib,
         sizeof(final_recon_info->port_attrib));
}

/**
 * This function iterates through all the ports and determines the final
 * corrective action for each of them
 */
static void determine_final_corrective_action(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = 0; port_id < BF_PIPE_PORT_COUNT; port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      determine_final_corrective_action_this_port(dev_id, dev_port);
    }
  }
}

/**
 * Once the corrective actions are determined for the ports, this function
 * loads the ports in different buckets according to their corrective action.
 * So for example, if the corrective action of a port is ADD_THEN_ENABLE, then
 * the port would be loaded in the the add port bucket and the enable port
 * bucket.
 */
static void load_port_in_bucket(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  /* Depending on the corrective action which has been selected for that
     port, put the port in the corresponding action buckets (add,del,enb.dis).
     We do this because we want all port APIs to be called in the following
     order.
     1. All Port Deletes
     2. All Port Disables
     3. All Port Adds
     4. All Port Enables */
  bf_ha_port_reconcile_info_t *recon;
  int dev_port_idx = DEV_PORT_TO_BIT_IDX(dev_port);
  bf_dev_port_t *port_add_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_add_bucket[0];
  bf_dev_port_t *port_enable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_enable_bucket[0];
  bf_dev_port_t *port_disable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_disable_bucket[0];
  bf_dev_port_t *port_delete_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_delete_bucket[0];
  bf_dev_port_t *port_fsm_link_monitoring_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_fsm_link_monitoring_bucket[0];

  if (dev_port_idx >= BF_PORT_COUNT) {
    LOG_ERROR(
        "%s:%d Error, dev_port_idx(%d) not in range [0; "
        "BF_PORT_COUNT(%d)[",
        __func__,
        __LINE__,
        dev_port_idx,
        (int)BF_PORT_COUNT);
    bf_sys_dbgchk(0);
    return;
  }

  recon = &dev_id_set[dev_id]
               ->port_corr_action.final_port_reconcile_info
               .recon_info_per_port[dev_port_idx];

  switch (recon->ca) {
    case BF_HA_CA_PORT_NONE:
      LOG_DBG("Final Corr : Dev : %d : Port : %d : NONE", dev_id, dev_port);
      break;
    case BF_HA_CA_PORT_FSM_LINK_MONITORING:
      LOG_DBG("Final Corr : Dev : %d : Port : %d : FSM_LINK_MONITORING",
              dev_id,
              dev_port);
      port_fsm_link_monitoring_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_ADD:
      LOG_DBG("Final Corr : Dev : %d : Port : %d : PORT ADD", dev_id, dev_port);
      port_add_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_ENABLE:
      LOG_DBG(
          "Final Corr : Dev : %d : Port : %d : PORT ENABLE", dev_id, dev_port);
      port_enable_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_ADD_THEN_ENABLE:
      LOG_DBG("Final Corr : Dev : %d : Port : %d : PORT ADD THEN ENABLE",
              dev_id,
              dev_port);
      port_add_bucket[dev_port_idx] = dev_port;
      port_enable_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_DISABLE:
      LOG_DBG(
          "Final Corr : Dev : %d : Port : %d : PORT DISABLE", dev_id, dev_port);
      port_disable_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_DELETE_THEN_ADD:
      LOG_DBG("Final Corr : Dev : %d : Port : %d : PORT DELETE THEN ADD",
              dev_id,
              dev_port);
      port_delete_bucket[dev_port_idx] = dev_port;
      port_add_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE:
      LOG_DBG(
          "Final Corr : Dev : %d : Port : %d : PORT DELETE THEN ADD THEN "
          "ENABLE",
          dev_id,
          dev_port);
      port_delete_bucket[dev_port_idx] = dev_port;
      port_add_bucket[dev_port_idx] = dev_port;
      port_enable_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_DELETE:
      LOG_DBG(
          "Final Corr : Dev : %d : Port : %d : PORT DELETE", dev_id, dev_port);
      port_delete_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_FLAP:
      LOG_DBG(
          "Final Corr : Dev : %d : Port : %d : PORT FLAP", dev_id, dev_port);
      port_disable_bucket[dev_port_idx] = dev_port;
      port_enable_bucket[dev_port_idx] = dev_port;
      break;
    case BF_HA_CA_PORT_MAX:
    default:
      LOG_ERROR(
          "Invalid Corrective Action: %d being applied for device %d, dev port "
          "%d",
          recon->ca,
          dev_id,
          dev_port);
  }
}

/**
 * Initialize the port buckets
 */
static void port_bucket_init(bf_dev_id_t dev_id) {
  int i;

  bf_dev_port_t *port_add_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_add_bucket[0];
  bf_dev_port_t *port_enable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_enable_bucket[0];
  bf_dev_port_t *port_disable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_disable_bucket[0];
  bf_dev_port_t *port_delete_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_delete_bucket[0];
  bf_dev_port_t *port_bringup_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_bringup_bucket[0];
  bf_dev_port_t *port_fsm_link_monitoring_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_fsm_link_monitoring_bucket[0];
  bf_dev_port_t *port_serdes_upgrade_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_serdes_upgrade_bucket[0];

  for (i = 0; i < BF_PORT_COUNT; i++) {
    port_delete_bucket[i] = -1;
    port_disable_bucket[i] = -1;
    port_add_bucket[i] = -1;
    port_enable_bucket[i] = -1;
    port_bringup_bucket[i] = -1;
    port_fsm_link_monitoring_bucket[i] = -1;
    port_serdes_upgrade_bucket[i] = -1;
  }
}

/**
 * Add all the ports from the add port bucket
 */
static bf_status_t port_addition_issue(bf_dev_id_t dev_id) {
  int i = 0;
  bf_status_t sts = BF_SUCCESS;
  bf_dev_port_t dev_port;
  bf_dev_port_t *port_add_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_add_bucket[0];

  /* Traverse through the entire port add bucket and issue port additions */
  for (i = 0; i < BF_PORT_COUNT; i++) {
    if (port_add_bucket[i] != -1) {
      dev_port = port_add_bucket[i];
      sts = bf_port_add_with_lane_numb(
          dev_id,
          dev_port,
          dev_id_set[dev_id]
              ->port_corr_action.final_port_reconcile_info
              .recon_info_per_port[i]
              .port_attrib.port_speeds,
          dev_id_set[dev_id]
              ->port_corr_action.final_port_reconcile_info
              .recon_info_per_port[i]
              .port_attrib.n_lanes,
          dev_id_set[dev_id]
              ->port_corr_action.final_port_reconcile_info
              .recon_info_per_port[i]
              .port_attrib.port_fec_types);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Port Add failed during warm init end for device %d port %d with "
            "sts %s(%d)",
            dev_id,
            dev_port,
            bf_err_str(sts),
            sts);
        return sts;
      }
      port_add_bucket[i] = -1;
    }
  }

  return BF_SUCCESS;
}

/**
 * Delete all the ports from the delete port bucket
 */
static bf_status_t port_deletion_issue(bf_dev_id_t dev_id) {
  int i = 0;
  bf_status_t sts = BF_SUCCESS;
  bf_dev_port_t *port_delete_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_delete_bucket[0];

  /* Traverse through the entire port delete bucket and issue port deletions */
  for (i = 0; i < BF_PORT_COUNT; i++) {
    if (port_delete_bucket[i] != -1) {
      sts = bf_port_remove(dev_id, port_delete_bucket[i]);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Port Delete failed during warm init end for device %d port %d "
            "with sts %s(%d)",
            dev_id,
            port_delete_bucket[i],
            bf_err_str(sts),
            sts);
        return sts;
      }
      port_delete_bucket[i] = -1;
    }
  }

  return BF_SUCCESS;
}

/**
 * Mark all the ports from the serdes upgrade bucket for loading the new serdes
 * firmware
 */
static bf_status_t port_serdes_upgrade_notify(bf_dev_id_t dev_id) {
  int i = 0;
  bf_status_t sts = BF_SUCCESS;

  bf_dev_port_t *port_serdes_upgrade_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_serdes_upgrade_bucket[0];

  /* Traverse through the entire serdes upgrade bucket and mark ports for serdes
   * upgrade */
  for (i = 0; i < BF_PORT_COUNT; i++) {
    if (port_serdes_upgrade_bucket[i] != -1) {
      sts = bf_port_serdes_upgrade_notify(0, port_serdes_upgrade_bucket[i]);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Port serdes upgrade failed during warm init end for device %d "
            "port %d with sts %s(%d)",
            dev_id,
            port_serdes_upgrade_bucket[i],
            bf_err_str(sts),
            sts);
        return sts;
      }
    }
  }

  return BF_SUCCESS;
}

/**
 * Enable all the ports from the enable port bucket
 */
static bf_status_t port_enable_issue(bf_dev_id_t dev_id) {
  int i = 0;
  bf_status_t sts = BF_SUCCESS;
  int oper_state = 0;
  int dev_port_idx;

  bf_dev_port_t *port_enable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_enable_bucket[0];
  bf_dev_port_t *port_bringup_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_bringup_bucket[0];

  /* Traverse through the entire port enable bucket and issue port enables */
  for (i = 0; i < BF_PORT_COUNT; i++) {
    if (port_enable_bucket[i] != -1) {
      sts = bf_port_enable(dev_id, port_enable_bucket[i], true);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Port Enable failed during warm init end for device %d port %d "
            "with sts %s(%d)",
            dev_id,
            port_enable_bucket[i],
            bf_err_str(sts),
            sts);
        return sts;
      }
      bool is_internal = false;
      lld_sku_is_dev_port_internal(dev_id, port_enable_bucket[i], &is_internal);
      if (is_internal) {
        oper_state = 1;
      } else {
        // Get the link state of the port read from the hardware
        sts = bf_port_oper_state_get_no_side_effect(
            dev_id, port_enable_bucket[i], &oper_state);
        if (sts != BF_SUCCESS) {
          LOG_ERROR(
              "Unable to get the oper state of device %d port %d with sts "
              "%s(%d)",
              dev_id,
              port_enable_bucket[i],
              bf_err_str(sts),
              sts);
          return sts;
        }
      }
      // Note the link-up or down state so that fsm state can be set
      // accordingly.
      dev_port_idx = DEV_PORT_TO_BIT_IDX(port_enable_bucket[i]);
      if (!BIT_IDX_VALIDATE(dev_port_idx)) {
        LOG_ERROR("Error, dev_port_idx equals %d", dev_port_idx);
        return BF_UNEXPECTED;
      }
      port_bringup_bucket[dev_port_idx] = oper_state;
      port_enable_bucket[i] = -1;
    }
  }

  return BF_SUCCESS;
}

/**
 * Disable all the ports from the disable port bucket
 */
static bf_status_t port_disable_issue(bf_dev_id_t dev_id) {
  int i = 0;
  bf_status_t sts = BF_SUCCESS;

  bf_dev_port_t *port_disable_bucket =
      &dev_id_set[dev_id]->port_corr_action.port_disable_bucket[0];

  /*raverse through the entire port disable bucket and issue port disables */
  for (i = 0; i < BF_PORT_COUNT; i++) {
    if (port_disable_bucket[i] != -1) {
      sts = bf_port_enable(dev_id, port_disable_bucket[i], false);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Port Disable failed during warm init end for device %d port %d "
            "with sts %s(%d)",
            dev_id,
            port_disable_bucket[i],
            bf_err_str(sts),
            sts);
        return sts;
      }
      port_disable_bucket[i] = -1;
    }
  }

  return BF_SUCCESS;
}

/**
 * This function determines the final corrective actions for all the ports and
 * then carries them out
 */
static bf_status_t final_corrective_action_apply(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  bf_status_t sts;

  // Initialize the port action buckets
  port_bucket_init(dev_id);

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = 0; port_id < BF_PIPE_PORT_COUNT; port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      load_port_in_bucket(dev_id, dev_port);
    }
  }

  /* Once all the port action buckets are filled in, traverse them in the
     following order-> Delete, Disable, Add, Enable */
  sts = port_deletion_issue(dev_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  sts = port_disable_issue(dev_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  sts = port_addition_issue(dev_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }
  /* Notify the clients about serdes upgrade on port enable */
  if (0) {
    sts = port_serdes_upgrade_notify(dev_id);
    if (sts != BF_SUCCESS) {
      return sts;
    }
  }
  sts = port_enable_issue(dev_id);
  if (sts != BF_SUCCESS) {
    return sts;
  }

  return BF_SUCCESS;
}

/**
 * Examine all the corrective actions registered by all the clients wrt to
 * the ports and issue final actions (in the right order) so that the port
 * related delta can be correctly applied by all the clients
 */
static bf_status_t bf_ha_port_corrective_action_apply(bf_dev_id_t dev_id,
                                                      bool is_fast_recfg) {
  if (!dev_id_set[dev_id]) return BF_OBJECT_NOT_FOUND;

  determine_final_corrective_action(dev_id);
  /* Set the fast recfg flag in the clients so that the port
   -     add/delete/enable/disable
   -     callbacks are only issued for the port mgr in case of fast recfg. For
   other
   -     cases, the callbacks are issued to all the clients. This approach has
   been
   -     taken for this reason:
   -     During fast recfg, all the clients except port manager compute no delta
   -     since they are going to incur a core reset. As a result, whatever
   config is
   -     replayed to them during fast recfg, they apply that and thats their
   final
   -     state. Now if the port add/del/enb/dis callbacks were to be issued to
   them
   -     during warm init end in fast recfg, they wouldn't know what to do with
   it.
   -     For eg. if the ports 0 and 2 existed in 50G mode, and the cfg was
   replayed
   -     with port 0 as 100G, then the port mgr would determine the delta to be
   port
   -     del for 0 and 2 and then port add for 0 with 100G. However, all the
   clients
   -     except for port mgr would not know what to do with the port del for 0
   and 2
   -     since they would have lost all their existing config during core reset.
   -     Hence, we don't want to issue port callbacks to the clients other than
   -     the port mgr.*/
  bf_status_t sts = bf_drv_skip_port_delta_push_set(dev_id, is_fast_recfg);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Unable to set the cfg replay flag for dev %d : %s (%d)",
              dev_id,
              bf_err_str(sts),
              sts);
    return sts;
  }

  /* Once we have determined the final corrective actions for all the ports,
     apply them */
  sts = final_corrective_action_apply(dev_id);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Unable to apply port corrective actions for dev %d : %s (%d)",
              dev_id,
              bf_err_str(sts),
              sts);
    return sts;
  }

  sts = bf_drv_skip_port_delta_push_set(dev_id, false);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Unable to reset the cfg replay flag for dev %d : %s (%d)",
              dev_id,
              bf_err_str(sts),
              sts);
    return sts;
  }

  return BF_SUCCESS;
}

/****************************************************************
 * bf_drv_init
 ****************************************************************/
bf_status_t bf_drv_init() {
  if (bf_drv_init_done) {
    return BF_SUCCESS;
  }
  memset((char *)dev_id_set, 0, sizeof(dev_id_set));

  // log SDE version
  LOG_DBG("BF-SDE vesion: %s", BF_DRV_REL_VER);

  bf_drv_init_done = true;
  return BF_SUCCESS;
}

/**
 * Add a new device to the pktmgr on the device. After this call, the device is
 * ready for
 * packet rx/tx api processing.
 * @param dev_family The device family.
 * @param dev_id The ASIC id.
 * @param dma_info Information regarding DMA DRs and associated buffer pools for
 * the CPU Pkt receive/transmit DRs
 * @return Status of the API call.
 */
bf_status_t bf_pktmgr_device_add(bf_dev_family_t dev_family,
                                 bf_dev_id_t dev_id,
                                 bf_dma_info_t *dma_info,
                                 bf_dev_flags_t flags) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_init_mode_t warm_init_mode;
  bool is_sw_model = BF_DEV_IS_SW_MODEL_GET(flags);

  LOG_TRACE(
      "bf_pktmgr_device_add dev id %d, is_sw_model %d", dev_id, is_sw_model);

  if (dev_id_set[dev_id] && (dev_id_set[dev_id]->dev_add_done ||
                             dev_id_set[dev_id]->pktmgr_dev_add_done)) {
    LOG_ERROR("Device id %u already exists in device add", dev_id);
    return BF_ALREADY_EXISTS;
  }

  if (dev_id_set[dev_id]) {
    warm_init_mode = dev_id_set[dev_id]->warm_init_mode;
  } else {
    warm_init_mode = BF_DEV_INIT_COLD;
  }

  if (!dev_id_set[dev_id]) {
    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      return BF_NO_SYS_RESOURCES;
    }
  }

  /* Update dev_id and device type before notifying clients */
  dev_id_set[dev_id]->dev_id = dev_id;
  dev_id_set[dev_id]->dev_family = dev_family;
  dev_id_set[dev_id]->is_sw_model = is_sw_model;

  // notify clients
  status = bf_drv_notify_clients_pktmgr_dev_add(
      dev_id, dev_family, dma_info, warm_init_mode);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device add failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    bf_drv_notify_clients_dev_del(dev_id, false);

    bf_sys_free(dev_id_set[dev_id]);
    dev_id_set[dev_id] = NULL;
    return status;
  }

  // update our db
  dev_id_set[dev_id]->pktmgr_dev_add_done = true;

  return status;
}

/****************************************************************
 * bf_device_add
 ****************************************************************/
bf_status_t bf_device_add(bf_dev_family_t dev_family,
                          bf_dev_id_t dev_id,
                          bf_device_profile_t *profile,
                          bf_dma_info_t *dma_info,
                          bf_dev_flags_t flags) {
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pipes = 0;
  bf_dev_init_mode_t warm_init_mode;
  bool is_sw_model = BF_DEV_IS_SW_MODEL_GET(flags);
  bool is_virtual_dev_slave = BF_DEV_IS_VIRTUAL_DEV_SLAVE_GET(flags);

  LOG_TRACE("bf_device_add dev id %d, is_sw_model %d", dev_id, is_sw_model);

  /* Sanitize dev_id and dev_family. */
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT ||
      dev_family < BF_DEV_FAMILY_TOFINO || dev_family >= BF_DEV_FAMILY_UNKNOWN)
    return BF_INVALID_ARG;

  if (dev_id_set[dev_id] && dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u already exists in device add", dev_id);
    return BF_ALREADY_EXISTS;
  }

  if (dev_id_set[dev_id]) {
    warm_init_mode = dev_id_set[dev_id]->warm_init_mode;
  } else {
    warm_init_mode = BF_DEV_INIT_COLD;
  }

  if (!dev_id_set[dev_id]) {
    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      return BF_NO_SYS_RESOURCES;
    }
  }

  /* Update dev_id and device type before notifying clients */
  dev_id_set[dev_id]->dev_id = dev_id;
  dev_id_set[dev_id]->dev_family = dev_family;
  dev_id_set[dev_id]->is_sw_model = is_sw_model;
  dev_id_set[dev_id]->is_virtual_dev_slave = is_virtual_dev_slave;

  // update our db
  // Cache the number of logical pipes
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  dev_id_set[dev_id]->num_pipes = num_pipes;

  /* Sanitize the profile.  Check that invalid pipes are not requested. */
  bf_device_profile_t local_profile;
  if (profile) {
    local_profile = *profile;
    bool bad_pipe = false;
    for (unsigned i = 0; i < local_profile.num_p4_programs; ++i) {
      bf_p4_program_t *prog = &local_profile.p4_programs[i];
      for (unsigned j = 0; j < prog->num_p4_pipelines; ++j) {
        bf_p4_pipeline_t *pipeline = &prog->p4_pipelines[j];

        /* Sort the pipe_scope array. */
        for (int J = 0; J < pipeline->num_pipes_in_scope; ++J)
          for (int O = J + 1; O < pipeline->num_pipes_in_scope; ++O)
            if (pipeline->pipe_scope[O] < pipeline->pipe_scope[J]) {
              int x = pipeline->pipe_scope[J];
              pipeline->pipe_scope[J] = pipeline->pipe_scope[O];
              pipeline->pipe_scope[O] = x;
            }

        /* Limit the number of pipes a P4 Pipeline is assigned to based on the
         * number of pipes on the chip. */
        if ((uint32_t)pipeline->num_pipes_in_scope > num_pipes) {
          pipeline->num_pipes_in_scope = num_pipes;
        }
        /* Check that the requested pipes exist on this device. */
        for (int k = 0; k < pipeline->num_pipes_in_scope; ++k) {
          if ((uint32_t)pipeline->pipe_scope[k] >= num_pipes) {
            bad_pipe = true;
            LOG_ERROR(
                "Program \"%s\" Pipeline \"%s\" cannot be assigned to device "
                "%d pipe %d, only %d pipe(s) available",
                prog->prog_name,
                pipeline->p4_pipeline_name,
                dev_id,
                pipeline->pipe_scope[k],
                num_pipes);
          }
        }
      }
    }
    if (bad_pipe) return BF_INVALID_ARG;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_add(dev_id,
                                         dev_family,
                                         profile ? &local_profile : NULL,
                                         dma_info,
                                         warm_init_mode);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device add failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    bf_drv_notify_clients_dev_del(dev_id, false);

    bf_sys_free(dev_id_set[dev_id]);
    dev_id_set[dev_id] = NULL;
    return status;
  }

  if (warm_init_mode == BF_DEV_WARM_INIT_HITLESS) {
    status = bf_drv_notify_clients_complete_hitless_hw_read(dev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR("Hitless HW read failed for dev %d, sts %s (%d)",
                dev_id,
                bf_err_str(status),
                status);
      bf_drv_notify_clients_dev_del(dev_id, false);

      bf_sys_free(dev_id_set[dev_id]);
      dev_id_set[dev_id] = NULL;
      return status;
    }
  }

  dev_id_set[dev_id]->dev_add_done = true;

  /* Register with lld for link change and speed notifs */
  port_mgr_register_port_cb(
      dev_id, bf_drv_notify_clients_port_status_chg, NULL);

  return status;
}

/****************************************************************
 * bf_virtual_device_add
 ****************************************************************/
bf_status_t bf_virtual_device_add(bf_dev_id_t dev_id,
                                  bf_device_profile_t *profile,
                                  bf_dev_type_t dev_type) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_init_mode_t warm_init_mode;

  if (BF_DEV_UNKNOWN == dev_type || BF_DEV_TYPE_MAX <= dev_type) {
    LOG_ERROR("Unsupported device type (%u) for virtual device add", dev_type);
    return BF_INVALID_ARG;
  }

  if (dev_id_set[dev_id] && dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u already exists in virtual device add", dev_id);
    return BF_ALREADY_EXISTS;
  }

  if (dev_id_set[dev_id]) {
    warm_init_mode = dev_id_set[dev_id]->warm_init_mode;
  } else {
    warm_init_mode = BF_DEV_INIT_COLD;
  }

  // notify clients
  status = bf_drv_notify_clients_virtual_dev_add(
      dev_id, dev_type, profile, warm_init_mode);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Virtual device add failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    bf_drv_notify_clients_dev_del(dev_id, false);
    return status;
  }

  if (!dev_id_set[dev_id]) {
    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      bf_drv_notify_clients_dev_del(dev_id, false);
      return BF_NO_SYS_RESOURCES;
    }
  }

  // update our db
  dev_id_set[dev_id]->dev_id = dev_id;
  dev_id_set[dev_id]->dev_family = bf_dev_type_to_family(dev_type);
  // Cache the number of logical pipes
  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  dev_id_set[dev_id]->num_pipes = num_pipes;
  dev_id_set[dev_id]->dev_add_done = true;
  dev_id_set[dev_id]->is_virtual = true;

  return status;
}
/****************************************************************
 * bf_device_set_virtual_dev_slave_mode
 ****************************************************************/
bf_status_t bf_device_set_virtual_dev_slave_mode(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_type_virtual_dev_slave(dev_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Virtual dev slave mode set  failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
  }
  return status;
}

/****************************************************************
 * bf_device_vdd_get
 ****************************************************************/
bf_status_t bf_device_vdd_get(bf_dev_id_t dev_id, bf_vdd_info_t *val) {
  int vmin = 0;
  if (!val) return BF_INVALID_ARG;
  if (dev_id >= BF_MAX_DEV_COUNT) return BF_INVALID_ARG;
  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_A0;
  if (LLD_OK != lld_sku_get_chip_part_revision_number(dev_id, &rev)) {
    return BF_INVALID_ARG;
  }
  if (LLD_OK != lld_sku_get_vmin(dev_id, &vmin)) {
    return BF_INVALID_ARG;
  }
  switch (dev_id_set[dev_id]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
      /* Tofino-1 and 2 do not implement fine grained voltage scaling and
       * instead use an SVS table. */
      return BF_NOT_IMPLEMENTED;
    case BF_DEV_FAMILY_TOFINO3:
      /* Tofino-3 has two voltage rails for the core logic: VDD driving TM and
       * ParDe blocks and VDDL driving MAUs and SNIF.  TF3 A0 uses a baseline of
       * 775 mV and 750 mV for VDD and VDDL respectively.  The actual voltage
       * will offset from that baseline using a signed value from EFuse (-127 ..
       * 127). */
      if (vmin < -127 || vmin > 127) return BF_UNEXPECTED;
      val->family = BF_DEV_FAMILY_TOFINO3;
      val->u.tof3.vdd = 775 + vmin;
      val->u.tof3.vddl = 750 + vmin;
      return BF_SUCCESS;

    case BF_DEV_FAMILY_UNKNOWN:
      return BF_UNEXPECTED;
  }

  return BF_UNEXPECTED;
}

/****************************************************************
 * bf_device_remove
 ****************************************************************/
bf_status_t bf_device_remove(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_del(dev_id, true);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device remove failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    /* Do not return here as we cannot recover from this, update DB */
  }

  // update our db
  bf_sys_free(dev_id_set[dev_id]);
  dev_id_set[dev_id] = NULL;
  return status;
}

/****************************************************************
 * bf_device_log
 ****************************************************************/
bf_status_t bf_device_log(bf_dev_id_t dev_id, const char *filepath) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_log(dev_id, filepath);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device log failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
  }
  return status;
}

/****************************************************************
 * bf_device_restore
 ****************************************************************/
bf_status_t bf_device_restore(bf_dev_id_t dev_id, const char *filepath) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  // notify clients
  status = bf_drv_notify_clients_dev_restore(dev_id, filepath);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Device restore failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
  }
  return status;
}

/****************************************************************
 * bf_port_get_default_lane_numb
 ****************************************************************/
bf_status_t bf_port_get_default_lane_numb(bf_dev_id_t dev_id,
                                          bf_port_speeds_t port_speed,
                                          uint32_t *lane_numb) {
  (void)dev_id;
  if (lane_numb == NULL) return BF_INVALID_ARG;
  *lane_numb = 0;
  switch (port_speed) {
    case BF_SPEED_1G:
    case BF_SPEED_10G:
    case BF_SPEED_25G:
      *lane_numb = 1;
      break;
    case BF_SPEED_40G_R2:
    case BF_SPEED_50G:
    case BF_SPEED_50G_CONS:
      *lane_numb = 2;
      break;
    case BF_SPEED_200G:
    case BF_SPEED_100G:
    case BF_SPEED_40G:
      *lane_numb = 4;
      break;
    case BF_SPEED_400G:
      *lane_numb = 8;
      break;
    default:
      break;
  }
  return BF_SUCCESS;
}

/****************************************************************
 * bf_port_add
 ****************************************************************/
bf_status_t bf_port_add(bf_dev_id_t dev_id,
                        bf_dev_port_t port,
                        bf_port_speeds_t port_speed,
                        bf_fec_types_t port_fec_type) {
  /* get default lane_numb for the speed, then call bf_port_add_internal()*/
  uint32_t lane_numb = 0;
  if (bf_port_get_default_lane_numb(dev_id, port_speed, &lane_numb) !=
      BF_SUCCESS)
    return BF_INVALID_ARG;
  return bf_port_add_with_lane_numb(
      dev_id, port, port_speed, lane_numb, port_fec_type);
}

/****************************************************************
 * bf_port_add_with_lane_numb
 ****************************************************************/
bf_status_t bf_port_add_with_lane_numb(bf_dev_id_t dev_id,
                                       bf_dev_port_t port,
                                       bf_port_speeds_t port_speed,
                                       uint32_t lane_numb,
                                       bf_fec_types_t port_fec_type) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_pipe_t pipe_id = DEV_PORT_TO_PIPE(port);
  int port_id = DEV_PORT_TO_LOCAL_PORT(port);
  bf_dev_init_mode_t warm_init_mode;

  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(port) ||
      !LOCAL_PORT_VALIDATE(port_id)) {
    status = BF_INVALID_ARG;
    goto done;
  }

  if (!dev_id_set[dev_id]) {
    status = BF_OBJECT_NOT_FOUND;
    goto done;
  }
  if (pipe_id >= dev_id_set[dev_id]->num_pipes) {
    status = BF_INVALID_ARG;
    goto done;
  }

  warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  if (warm_init_mode == BF_DEV_INIT_COLD) {
    if (dev_id_set[dev_id]->port[pipe_id][port_id].added) {
      status = BF_ALREADY_EXISTS;
      goto done;
    }
  }
  // notify clients
  status = bf_drv_notify_clients_port_add(
      dev_id, port, port_speed, lane_numb, port_fec_type);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Port add failed for dev %d, port %d, sts %s (%d)",
              dev_id,
              port,
              bf_err_str(status),
              status);
    bf_drv_notify_clients_port_del(dev_id, port, false);
    return status;
  }

  // update our db
  dev_id_set[dev_id]->port[pipe_id][port_id].port = port;
  dev_id_set[dev_id]->port[pipe_id][port_id].added = 1;
  dev_id_set[dev_id]->port[pipe_id][port_id].speed = port_speed;
  dev_id_set[dev_id]->port[pipe_id][port_id].lane_numb = lane_numb;
done:
  LOG_TRACE("Dev %d port %d added sts %s (%d)",
            dev_id,
            port,
            bf_err_str(status),
            status);
  return status;
}

bf_status_t bf_port_info_get(bf_dev_id_t dev_id,
                             bf_dev_port_t port,
                             bf_port_speed_t *speed,
                             uint32_t *lane_numb) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_pipe_t pipe_id = DEV_PORT_TO_PIPE(port);
  int port_id = DEV_PORT_TO_LOCAL_PORT(port);

  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(port) ||
      !LOCAL_PORT_VALIDATE(port_id)) {
    status = BF_INVALID_ARG;
    goto done;
  }

  if (!dev_id_set[dev_id]) {
    status = BF_OBJECT_NOT_FOUND;
    goto done;
  }
  if (pipe_id >= dev_id_set[dev_id]->num_pipes) {
    status = BF_INVALID_ARG;
    goto done;
  }
  if (dev_id_set[dev_id]->port[pipe_id][port_id].added) {
    if (speed) *speed = dev_id_set[dev_id]->port[pipe_id][port_id].speed;
    if (lane_numb)
      *lane_numb = dev_id_set[dev_id]->port[pipe_id][port_id].lane_numb;
    return BF_SUCCESS;
  } else {
    return BF_OBJECT_NOT_FOUND;
  }
done:
  return status;
}

/****************************************************************
 * bf_port_remove
 ****************************************************************/
bf_status_t bf_port_remove(bf_dev_id_t dev_id, bf_dev_port_t port) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_pipe_t pipe_id = DEV_PORT_TO_PIPE(port);
  int port_id = DEV_PORT_TO_LOCAL_PORT(port);
  bf_dev_init_mode_t warm_init_mode;

  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(port) ||
      !LOCAL_PORT_VALIDATE(port_id)) {
    status = BF_INVALID_ARG;
    goto done;
  }
  if (!dev_id_set[dev_id]) {
    status = BF_OBJECT_NOT_FOUND;
    goto done;
  }
  if (pipe_id >= dev_id_set[dev_id]->num_pipes) {
    status = BF_INVALID_ARG;
    goto done;
  }
  /* During HA, not necessary the exact config will be replayed. The task
     of determining if any ports need to be deleted and any ports are to be
     added
     in place of them is left with the SDE port mgr. As a result, we don't need
     this check. For eg. Consider the following scenario. If initially there was
     a port 185 with speed 10G, and then during config replayed, it was never
     replayed; however another port 184 was replayed with speed 10G. Thus we
     need
     to delete port 185. However, since 185 was never replayed, this check would
     fail and we would return immediately. */
  warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  if (warm_init_mode == BF_DEV_INIT_COLD) {
    if (!dev_id_set[dev_id]->port[pipe_id][port_id].added) {
      status = BF_OBJECT_NOT_FOUND;
      goto done;
    }
  }

  /* Disable the port before deleting. */
  if (dev_id_set[dev_id]->port[pipe_id][port_id].enabled) {
    bf_port_enable(dev_id, port, false);
  }

  // notify clients if the device is not locked
  status = bf_drv_notify_clients_port_del(dev_id, port, true);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Port remove failed for dev %d, port %d, sts %s (%d)",
              dev_id,
              port,
              bf_err_str(status),
              status);
    /* Do not return here as we cannot recover from this, update DB */
  }

  // update our db
  memset((char *)&dev_id_set[dev_id]->port[pipe_id][port_id],
         0,
         sizeof(dev_id_set[dev_id]->port[pipe_id][port_id]));

done:
  LOG_TRACE("Dev %d port %d removed sts %s (%d)",
            dev_id,
            port,
            bf_err_str(status),
            status);
  return status;
}

/****************************************************************
 * bf_port_enable
 ****************************************************************/
bf_status_t bf_port_enable(bf_dev_id_t dev_id,
                           bf_dev_port_t port,
                           bool enable) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_pipe_t pipe_id = DEV_PORT_TO_PIPE(port);
  int port_id = DEV_PORT_TO_LOCAL_PORT(port);

  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(port) ||
      !LOCAL_PORT_VALIDATE(port_id)) {
    return BF_INVALID_ARG;
  }
  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }
  if (pipe_id >= dev_id_set[dev_id]->num_pipes) {
    return BF_INVALID_ARG;
  }
  if (!dev_id_set[dev_id]->port[pipe_id][port_id].added) {
    LOG_ERROR("Request to %sable dev %d port %d that was not yet added",
              enable ? "en" : "dis",
              dev_id,
              port);
    return BF_INVALID_ARG;
  }

  status = bf_drv_notify_clients_port_admin_state(dev_id, port, enable);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "Port enable failed for dev %d, port %d, state %d,"
        " sts %s (%d)",
        dev_id,
        port,
        enable,
        bf_err_str(status),
        status);
    return status;
  }
  dev_id_set[dev_id]->port[pipe_id][port_id].enabled = enable ? 1 : 0;
  return status;
}

/****************************************************************
 * bf_port_serdes_upgrade_notify
 ****************************************************************/
bf_status_t bf_port_serdes_upgrade_notify(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port) {
  bf_status_t status = BF_SUCCESS;

  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(dev_port)) {
    return BF_INVALID_ARG;
  }
  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }
  if (DEV_PORT_TO_PIPE(dev_port) >= dev_id_set[dev_id]->num_pipes) {
    return BF_INVALID_ARG;
  }

  /* FIXME Temporarily serdes upgrade in warm init is not supported */
  status = bf_drv_notify_clients_port_serdes_upgrade(dev_id, dev_port, 0, NULL);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "Port serdes upgrade notify failed for dev %d, port %d,"
        " sts %s (%d)",
        dev_id,
        dev_port,
        bf_err_str(status),
        status);
    return status;
  }
  return status;
}

/****************************************************************
 * cpu_pcie_port and cpu_eth port
 ****************************************************************/
bf_dev_port_t bf_pcie_cpu_port_get(bf_dev_id_t dev_id) {
  if (!dev_id_set[dev_id]) return -1;
  return lld_get_pcie_cpu_port(dev_id);
}

bf_dev_port_t bf_pcie_cpu_port2_get(bf_dev_id_t dev_id) {
  if (!dev_id_set[dev_id]) return -1;
  return lld_get_pcie_cpu_port2(dev_id);
}

bf_dev_port_t bf_eth_cpu_port_get(bf_dev_id_t dev_id) {
  return (dev_id_set[dev_id]) ? lld_get_min_cpu_port(dev_id) : -1;
}

bf_dev_port_t bf_eth_cpu_port2_get(bf_dev_id_t dev_id) {
  return (dev_id_set[dev_id]) ? lld_get_min_cpu_port2(dev_id) : -1;
}

bf_dev_port_t bf_eth_max_cpu_port_get(bf_dev_id_t dev_id) {
  return (dev_id_set[dev_id]) ? lld_get_max_cpu_port(dev_id) : -1;
}

bf_status_t bf_eth_get_next_cpu_port(bf_dev_id_t dev_id, bf_dev_port_t *port) {
  if (!dev_id_set[dev_id]) return BF_INVALID_ARG;
  return lld_get_next_cpu_port(dev_id, port);
}

/****************************************************************
 * bf_set_copy_to_cpu
 ****************************************************************/
bf_status_t bf_set_copy_to_cpu(bf_dev_id_t dev_id,
                               bool enable,
                               bf_dev_port_t port) {
  if ((dev_id >= BF_MAX_DEV_COUNT) || !DEV_PORT_VALIDATE(port)) {
    return BF_INVALID_ARG;
  }
  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }
  if (DEV_PORT_TO_PIPE(port) >= dev_id_set[dev_id]->num_pipes) {
    return BF_INVALID_ARG;
  }
  return bf_mc_set_copy_to_cpu(dev_id, enable, port);
}

void bf_drv_dump_cfg(ucli_context_t *uc) {
  bf_dev_id_t asic;
  bf_dev_port_t port;
  bf_dev_pipe_t pipe_id;

  aim_printf(&uc->pvs,
             "+----+------+-----+---+--------------------------------\n");
  aim_printf(&uc->pvs,
             "|dev | port                                            \n");
  aim_printf(&uc->pvs,
             "+----+------+-----+---+--------------------------------\n");

  for (asic = 0; asic < BF_MAX_DEV_COUNT; asic++) {
    uint32_t num_subdev = 0;
    if (!dev_id_set[asic]) continue;

    lld_sku_get_num_subdev(asic, &num_subdev, NULL);
    for (pipe_id = 0; pipe_id < (BF_SUBDEV_PIPE_COUNT * num_subdev);
         pipe_id++) {
      for (port = 0; port < BF_PIPE_PORT_COUNT; port++) {
        if (!dev_id_set[asic]->port[pipe_id][port].added) continue;

        aim_printf(&uc->pvs,
                   "| %2d | %3d   \n",
                   asic,
                   dev_id_set[asic]->port[pipe_id][port].port);
      }
    }
  }
  aim_printf(&uc->pvs,
             "+----+------+-----+---+--------------------------------\n");
}

bf_status_t bf_drv_get_dma_dr_size_in_bytes(
    int num_dr_entries[BF_DMA_TYPE_MAX][BF_DMA_DR_DIRS],
    unsigned int *total_bytes) {
  (void)num_dr_entries;
  *total_bytes = 0;
  return BF_SUCCESS;
}

const char *bf_drv_get_version(void) { return BF_DRV_VER; }

const char *bf_drv_get_internal_version(void) { return BF_DRV_INTERNAL_VER; }

/****************************************************************
 * bf_err_interrupt_handling_enable
 ****************************************************************/
bf_status_t bf_err_interrupt_handling_mode_set(bf_dev_id_t dev_id,
                                               bool enable) {
  bf_status_t status = BF_SUCCESS;

  if (!dev_id_set[dev_id]) {
    return BF_INVALID_ARG;
  }

  // notify clients
  status = bf_drv_notify_clients_err_interrupt_handling_mode(dev_id, enable);
  if (status != BF_SUCCESS) {
    LOG_ERROR("Interrupt mode set failed for dev %d, sts %s (%d)",
              dev_id,
              bf_err_str(status),
              status);
    return status;
  }

  return status;
}

/**
 * Perform a compatibility check prior to planned warm-restarts
 * @param dev_id The device id
 * @param new_profile The new profile to perform compatibility check against
 * @param new_drv_path The path of the new bf-drivers
 * @param serdes_upgrade_p Function returns the best warm initialization mod e
 * @param serdes_upgrade_p Function returns whether SerDes upgrade is needed
 * @return Status of the API call
 */

bf_status_t bf_device_compat_check(bf_dev_id_t dev_id,
                                   bf_device_profile_t *new_profile,
                                   char *new_drv_path,
                                   bf_dev_init_mode_t *warm_init_mode_p,
                                   bool *serdes_upgrade_p) {
  (void)dev_id;
  (void)new_profile;
  (void)new_drv_path;
  (void)warm_init_mode_p;
  (void)serdes_upgrade_p;
  return BF_NOT_IMPLEMENTED;
}

/**
 * Initiate a warm init process for a device
 * @param dev_id The device id
 * @param warm_init_mode The warm init mode to use for this device
 * @param serdes_upgrade_mode The mode to use for updating SerDes
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_begin(
    bf_dev_id_t dev_id,
    bf_dev_init_mode_t warm_init_mode,
    bf_dev_serdes_upgrade_mode_t serdes_upgrade_mode) {
  LOG_TRACE(
      "%s:%d Entering WARM_INIT_BEGIN for device %d with warm_init_mode %d "
      "serdes_upgrade_mode %d",
      __func__,
      __LINE__,
      dev_id,
      warm_init_mode,
      serdes_upgrade_mode);
  bf_status_t status = BF_SUCCESS;

  if (warm_init_mode != BF_DEV_INIT_COLD) {
    status = bf_drv_apply_reconfig_step(dev_id, BF_RECONFIG_LOCK);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to lock device %d, status %s", dev_id, bf_err_str(status));
      return status;
    }
  }
  if (warm_init_mode != BF_DEV_WARM_INIT_FAST_RECFG_QUICK) {
    if (dev_id_set[dev_id] && dev_id_set[dev_id]->dev_add_done) {
      /* If the device already exists, lock the device and remove it */
      status = bf_device_remove(dev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d Device id %u cannot be removed error %d",
                  __func__,
                  __LINE__,
                  dev_id,
                  status);
        return status;
      }
    }

    dev_id_set[dev_id] = bf_sys_calloc(1, sizeof(dvm_asic_t));
    if (!dev_id_set[dev_id]) {
      return BF_NO_SYS_RESOURCES;
    }

    dev_id_set[dev_id]->dev_id = dev_id;
    dev_id_set[dev_id]->dev_family =
        bf_dev_type_to_family(lld_sku_get_dev_type(dev_id));
    dev_id_set[dev_id]->dev_add_done = false;
  } else {
    if (!dev_id_set[dev_id]) {
      return BF_INVALID_ARG;
    }
  }
  dev_id_set[dev_id]->warm_init_mode = warm_init_mode;
  // Set warm init in progress flag
  dev_id_set[dev_id]->warm_init_in_progress = true;
  dev_id_set[dev_id]->serdes_upgrade_mode = serdes_upgrade_mode;

  LOG_TRACE("%s:%d Exiting WARM_INIT_BEGIN for device %d",
            __func__,
            __LINE__,
            dev_id);
  return status;
}

/**
 * End the warm init sequence for the device and resume normal operation
 * @param dev_id The device id
 * @return Status of the API call.
 */
bf_status_t bf_device_warm_init_end(bf_dev_id_t dev_id) {
  LOG_TRACE(
      "%s:%d Entering WARM_INIT_END for device %d", __func__, __LINE__, dev_id);
  bf_status_t status = BF_SUCCESS;
  bf_dev_init_mode_t warm_init_mode;

  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("%s:%d Device id %u not found in warm init end",
              __func__,
              __LINE__,
              dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  switch (warm_init_mode) {
    case BF_DEV_INIT_COLD:
      /* Do nothing */
      return status;
    case BF_DEV_WARM_INIT_FAST_RECFG:
      LOG_TRACE("%s:%d Apply BF_RECONFIG_LOCK step to device %d",
                __func__,
                __LINE__,
                dev_id);
      status = bf_drv_apply_reconfig_step(dev_id, BF_RECONFIG_UNLOCK);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "Dev %d reconfig unlock failed %s", dev_id, bf_err_str(status));
        return status;
      }
      break;
    case BF_DEV_WARM_INIT_FAST_RECFG_QUICK:
      status = bf_drv_apply_reconfig_step(dev_id, BF_RECONFIG_UNLOCK_QUICK);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "Dev %d reconfig unlock failed %s", dev_id, bf_err_str(status));
        return status;
      }
      break;
    case BF_DEV_WARM_INIT_HITLESS:
      LOG_TRACE("%s:%d Apply BF_DEV_WARM_INIT_HITLESS step to device %d",
                __func__,
                __LINE__,
                dev_id);
      status = bf_drv_notify_clients_hitless_warm_init_end(dev_id);
      if (status != BF_SUCCESS) {
        return status;
      }
      break;
  }

  if (dev_id_set[dev_id]->is_sw_model) {
    LOG_TRACE("%s:%d Notify clients of port push delta done for dev %d",
              __func__,
              __LINE__,
              dev_id);
    // Mark HA to done state
    status = bf_drv_notify_clients_port_delta_push_done(dev_id);
    if (status != BF_SUCCESS) {
      return status;
    }
    // Don't run the HA sequence on ports in case of the model
    return BF_SUCCESS;
  }
  LOG_TRACE("%s:%d  Get port delta from clients for dev %d",
            __func__,
            __LINE__,
            dev_id);
  /* Get the port delta from all the clients */
  status = bf_drv_get_port_delta_from_clients(
      dev_id, &dev_id_set[dev_id]->port_corr_action);
  if (status != BF_SUCCESS) {
    return status;
  }
  LOG_TRACE("%s:%d Push port delta to clients for dev %d",
            __func__,
            __LINE__,
            dev_id);
  /* Push the delta related to the ports */
  status = bf_ha_port_corrective_action_apply(
      dev_id,
      ((warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG) ||
       (warm_init_mode == BF_DEV_WARM_INIT_FAST_RECFG_QUICK))
          ? true
          : false);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "Unable to apply corrective action for ports for dev %d : %s (%d)",
        dev_id,
        bf_err_str(status),
        status);
    return status;
  }
  LOG_TRACE("%s:%d Notify clients of port push delta done for dev %d",
            __func__,
            __LINE__,
            dev_id);
  /* Communicate the completion of the port delta push to the clients*/
  status = bf_drv_notify_clients_port_delta_push_done(dev_id);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Reset warm init in progress flag
  dev_id_set[dev_id]->warm_init_in_progress = false;

  LOG_TRACE(
      "%s:%d Exiting WARM_INIT_END for device %d", __func__, __LINE__, dev_id);
  return status;
}

/**
 * Return if a port needs to be brought up using the FSM at the end of warm init
 * end
 * @param dev_id The device id
 * @param dev_port Device port number
 * @param bring_up Flag to indicate if this port needs to be brought up
 * @return Status of the API call
 */
bf_status_t bf_device_warm_init_port_bring_up(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool *link_down) {
  int dev_port_idx;

  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done ||
      (!link_down)) {
    LOG_ERROR("Device id %u not found in warm init port bring up", dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  dev_port_idx = DEV_PORT_TO_BIT_IDX(dev_port);
  if (dev_port_idx >= BF_PORT_COUNT) {
    LOG_ERROR(
        "%s:%d Error, dev_port_idx(%d) not in range [0; "
        "BF_PORT_COUNT(%d)[",
        __func__,
        __LINE__,
        dev_port_idx,
        (int)BF_PORT_COUNT);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  if (dev_id_set[dev_id]->port_corr_action.port_bringup_bucket[dev_port_idx] !=
      -1) {
    *link_down = true;
    dev_id_set[dev_id]->port_corr_action.port_bringup_bucket[dev_port_idx] = -1;
  } else if (dev_id_set[dev_id]
                 ->port_corr_action
                 .port_fsm_link_monitoring_bucket[dev_port_idx] != -1) {
    *link_down = false;
    dev_id_set[dev_id]
        ->port_corr_action.port_fsm_link_monitoring_bucket[dev_port_idx] = -1;
  } else {
    LOG_TRACE("%s:%d FSM not going to be started for dev %u port %d",
              __func__,
              __LINE__,
              dev_id,
              dev_port);
    return BF_OBJECT_NOT_FOUND;
  }

  return BF_SUCCESS;
}

/**
 * Get the init mode of a device
 * @param dev_id The device id
 * @param warm_init_mode The warm init mode to use for this device
 * @return Status of the API call
 */
bf_status_t bf_device_init_mode_get(bf_dev_id_t dev_id,
                                    bf_dev_init_mode_t *warm_init_mode) {
  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u not found in warm init end", dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *warm_init_mode = dev_id_set[dev_id]->warm_init_mode;

  return BF_SUCCESS;
}

/**
 * Check if warm init is currently in progress on the device
 * @param dev_id The ID of the device
 * @param warm_init_in_progress True is warm init is in progress
 * @return Status of the API call
 */
bf_status_t bf_device_warm_init_in_progress(bf_dev_id_t dev_id,
                                            bool *warm_init_in_progress) {
  if (!warm_init_in_progress) {
    LOG_ERROR("Argument warm_init_in_progress is NULL");
    return BF_INVALID_ARG;
  }

  if (!bf_dev_id_validate(dev_id)) {
    LOG_ERROR("Invalid device id %d", dev_id);
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev_id]) {
    LOG_ERROR("Device id %u not found", dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *warm_init_in_progress = dev_id_set[dev_id]->warm_init_in_progress;

  return BF_SUCCESS;
}

/**
 * Get the family of a device
 * @param dev_id The device id
 * @param dev_family The family for this device
 * @return Status of the API call
 */
bf_status_t bf_device_family_get(bf_dev_id_t dev_id,
                                 bf_dev_family_t *dev_family) {
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev_id] || !dev_id_set[dev_id]->dev_add_done) {
    LOG_ERROR("Device id %u not found in family get", dev_id);
    return BF_OBJECT_NOT_FOUND;
  }

  *dev_family = dev_id_set[dev_id]->dev_family;

  return BF_SUCCESS;
}

/**
 * Get the device type (model or asic)
 * @param dev_id The device id
 * @param is_sw_model Pointer to bool flag to return true for model and
 *                    false for asic devices
 * @return Status of the API call.
 */
bf_status_t bf_drv_device_type_get(bf_dev_id_t dev_id, bool *is_sw_model) {
  *is_sw_model = false;

  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  *is_sw_model = dev_id_set[dev_id]->is_sw_model;

  return BF_SUCCESS;
}

/**
 * Check if the device type is a virtual device slave
 * @param dev_id The device id
 * @param is_sw_model Pointer to bool flag to return true for virtual dev slave
 * and otherwise
 * @return Status of the API call.
 */
bf_status_t bf_drv_device_type_virtual_dev_slave(bf_dev_id_t dev_id,
                                                 bool *is_virtual_dev_slave) {
  *is_virtual_dev_slave = false;

  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev_id]) {
    return BF_OBJECT_NOT_FOUND;
  }

  *is_virtual_dev_slave = dev_id_set[dev_id]->is_virtual_dev_slave;

  return BF_SUCCESS;
}

/**
 * Get the clock speed of the given device
 * @param dev_id          The device id
 * @param clock_speed     The pointer in which to save the device clock speed
 * @return                Status of the API call
 */
bf_status_t bf_drv_get_clock_speed(bf_dev_id_t dev_id,
                                   uint64_t *bps_clock_speed,
                                   uint64_t *pps_clock_speed) {
  bf_sku_core_clk_freq_t bps_freq, pps_freq;
  lld_err_t lld_err = lld_sku_get_core_clk_freq(dev_id, &bps_freq, &pps_freq);
  if (LLD_OK != lld_err) {
    LOG_ERROR(
        "Cannot determine device frequency, dev %d, sts %d", dev_id, lld_err);
    return lld_err;
  }
  if (dev_id < 0 || dev_id >= BF_MAX_DEV_COUNT) return BF_INVALID_ARG;
  if (!dev_id_set[dev_id]) return BF_INVALID_ARG;
  bool is_tof = dev_id_set[dev_id]->dev_family == BF_DEV_FAMILY_TOFINO;
  bool is_tof3 = dev_id_set[dev_id]->dev_family == BF_DEV_FAMILY_TOFINO3;

  // bps
  switch (bps_freq) {
    case BF_SKU_CORE_CLK_1_3_GHZ:
      if (is_tof3) {
        *bps_clock_speed = 1300000000ull;
      } else {
        *bps_clock_speed = 1312500000ull;
      }
      break;
    case BF_SKU_CORE_CLK_1_25_GHZ:
      if (is_tof)
        *bps_clock_speed = 1220000000ull;
      else
        *bps_clock_speed = 1250000000ull;
      break;
    case BF_SKU_CORE_CLK_1_1_GHZ:
      *bps_clock_speed = 1100000000ull;
      break;
    case BF_SKU_CORE_CLK_1_0_GHZ:
      *bps_clock_speed = 1000000000ull;
      break;
    case BF_SKU_CORE_CLK_1_05_GHZ:
      *bps_clock_speed = 1050000000ull;
      break;
    default:
      return BF_UNEXPECTED;
  }

  switch (pps_freq) {
    case BF_SKU_CORE_CLK_1_5_GHZ:
      *pps_clock_speed = 1500000000ull;
      break;
    case BF_SKU_CORE_CLK_1_3_GHZ:
      if (is_tof3) {
        *pps_clock_speed = 1300000000ull;
      } else {
        *pps_clock_speed = 1312500000ull;
      }
      break;
    case BF_SKU_CORE_CLK_1_2625_GHZ:
      *pps_clock_speed = 1262500000ull;
      break;
    case BF_SKU_CORE_CLK_1_25_GHZ:
      if (is_tof)
        *pps_clock_speed = 1220000000ull;
      else
        *pps_clock_speed = 1250000000ull;
      break;
    case BF_SKU_CORE_CLK_1_1_GHZ:
      *pps_clock_speed = 1100000000ull;
      break;
    case BF_SKU_CORE_CLK_1_0_GHZ:
      *pps_clock_speed = 1000000000ull;
      break;
    case BF_SKU_CORE_CLK_1_05_GHZ:
      *pps_clock_speed = 1050000000ull;
      break;
    default:
      return BF_UNEXPECTED;
  }
  return BF_SUCCESS;
}

bool bf_drv_is_device_virtual(bf_dev_id_t dev_id) {
  if (!dev_id_set[dev_id]) {
    return false;
  }
  return dev_id_set[dev_id]->is_virtual;
}

/**
 * Check whether port mode transition workaround needs to be applied or not
 * @param dev_id The device id
 * @param port_id The port identifier
 * @param port_speed The port speed
 * @return TRUE - if the workaround needs to be applied
 *         FALSE - if the workaround need not be applied
 */
bool bf_drv_check_port_mode_transition_wa(bf_dev_id_t dev_id,
                                          bf_dev_port_t port_id,
                                          bf_port_speeds_t port_speed) {
  /*
   * When a port gets transitioned from multi-channel mode (10G/25G/50G) to
   * single channel mode (40G/100G) in Tofino1, the egress path would be
   * stuck due to an EPB HW issue. To overcome this, a workaround is
   * implemented in SW. This function checks whether the workaround needs
   * be applied or not.
   *
   * Workaround needs to be applied only if all of the following condtions
   * are satisfied -
   *    - ASIC is Tofino1
   *    - Warm init (fast reconfig, fast reconfig quick or hitless HA)
   *      is not in progress
   *    - Application has registered the port mode transition callback as
   *      the workaround involves sending special packets
   *    - Port is in single channel mode (40G/100G)
   *    - Port belongs to first 16 MACs in the pipe (this may be changed
   *      later if needed)
   */
  if (dev_id_set[dev_id]->dev_family == BF_DEV_FAMILY_TOFINO &&
      bf_drv_client_port_mode_change_callback_registered(dev_id) &&
      (port_speed == BF_SPEED_100G || port_speed == BF_SPEED_40G) &&
      (DEV_PORT_TO_LOCAL_PORT(port_id) < 64) &&
      dev_id_set[dev_id]->warm_init_in_progress == false) {
    // warm_init_in_progress flag takes care of all warm init types -
    //       fast reconfig, fast reconfig quick and hitless HA
    LOG_DBG(
        "%s, applying port mode transition workaround for dev %d, "
        "port %d, speed %d",
        __func__,
        dev_id,
        port_id,
        port_speed);
    return true;
  } else {
    return false;
  }
}

bf_status_t bf_drv_lrt_dr_timeout_set(bf_dev_id_t dev_id, uint32_t timeout_ms) {
  bf_status_t sts = BF_SUCCESS;
  uint64_t bps_clock_speed, pps_clock_speed;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, 0);
  if (!dev_p) return BF_INVALID_ARG;
  uint16_t num_subdevices = dev_p->num_subdev;
  sts = bf_drv_get_clock_speed(dev_id, &bps_clock_speed, &pps_clock_speed);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to get the clock speed for dev %d : err %d(%s)",
              __func__,
              __LINE__,
              dev_id,
              sts,
              bf_err_str(sts));
    return sts;
  }
  uint32_t num_clocks = timeout_ms * (bps_clock_speed / 1000);
  for (uint16_t sub_dev = 0; sub_dev < num_subdevices; sub_dev++) {
    lld_subdev_dr_data_timeout_set(dev_id, sub_dev, lld_dr_fm_lrt, num_clocks);
  }
  return sts;
}

bf_status_t bf_drv_lrt_dr_timeout_get(bf_dev_id_t dev_id,
                                      uint32_t *timeout_ms) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t num_clocks = 0;
  uint16_t sub_dev = 0;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, 0);
  if (!dev_p) return BF_INVALID_ARG;
  uint16_t num_subdevices = dev_p->num_subdev;
  sts = lld_subdev_dr_data_timeout_get(
      dev_id, sub_dev, lld_dr_fm_lrt, &num_clocks);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d : Unable to get the LRT DR timeout from the hardware for dev %d "
        ": err %d(%s)",
        __func__,
        __LINE__,
        dev_id,
        sts,
        bf_err_str(sts));
    return sts;
  }
  /*Do subdevice lrt clock timeout checks if we have subdevices*/
  for (sub_dev = 1; sub_dev < num_subdevices; sub_dev++) {
    uint32_t sub_dev_clocks = 0;
    sts = lld_subdev_dr_data_timeout_get(
        dev_id, sub_dev, lld_dr_fm_lrt, &sub_dev_clocks);
    if (sub_dev_clocks != num_clocks || sts != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d : Mismatched LRT DR timeout in the hardware for dev %d "
          "subdev %d with subdev 0"
          ": err %d(%s)",
          __func__,
          __LINE__,
          dev_id,
          sub_dev,
          sts,
          bf_err_str(sts));
      return sts;
    }
  }

  uint64_t bps_clock_speed, pps_clock_speed;
  sts = bf_drv_get_clock_speed(dev_id, &bps_clock_speed, &pps_clock_speed);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to get the clock speed for dev %d : err %d(%s)",
              __func__,
              __LINE__,
              dev_id,
              sts,
              bf_err_str(sts));
    return sts;
  }

  *timeout_ms = ((uint64_t)num_clocks * 1000) / bps_clock_speed;
  return sts;
}

bf_status_t bf_drv_lrt_dr_timeout_set_us(bf_dev_id_t dev_id,
                                         uint32_t timeout_us) {
  bf_status_t sts = BF_SUCCESS;
  uint64_t bps_clock_speed, pps_clock_speed;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, 0);
  if (!dev_p) return BF_INVALID_ARG;
  uint16_t num_subdevices = dev_p->num_subdev;
  uint32_t num_clocks;
  sts = bf_drv_get_clock_speed(dev_id, &bps_clock_speed, &pps_clock_speed);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to get the clock speed for dev %d : err %d(%s)",
              __func__,
              __LINE__,
              dev_id,
              sts,
              bf_err_str(sts));
    return sts;
  }
  num_clocks = timeout_us * (bps_clock_speed / 1000000);
  for (uint16_t sub_dev = 0; sub_dev < num_subdevices; sub_dev++) {
    lld_subdev_dr_data_timeout_set(dev_id, sub_dev, lld_dr_fm_lrt, num_clocks);
  }
  return sts;
}

bf_status_t bf_drv_lrt_dr_timeout_get_us(bf_dev_id_t dev_id,
                                         uint32_t *timeout_us) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t num_clocks = 0;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, 0);
  if (!dev_p) return BF_INVALID_ARG;
  uint16_t num_subdevices = dev_p->num_subdev;
  sts = lld_subdev_dr_data_timeout_get(dev_id, 0, lld_dr_fm_lrt, &num_clocks);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d : Unable to get the LRT DR timeout from the hardware for dev %d "
        ": err %d(%s)",
        __func__,
        __LINE__,
        dev_id,
        sts,
        bf_err_str(sts));
    return sts;
  }
  /*Do subdevice lrt clock timeout checks if we have subdevices*/
  if (num_subdevices > 1) {
    for (uint16_t sub_dev = 0; sub_dev < num_subdevices; sub_dev++) {
      uint32_t sub_dev_clocks = 0;
      sts = lld_subdev_dr_data_timeout_get(
          dev_id, sub_dev, lld_dr_fm_lrt, &sub_dev_clocks);
      if (sub_dev_clocks != num_clocks || sts != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d : Mismatched LRT DR timeout in the hardware for dev %d "
            "subdev %d with subdev 0"
            ": err %d(%s)",
            __func__,
            __LINE__,
            dev_id,
            sub_dev,
            sts,
            bf_err_str(sts));
        return sts;
      }
    }
  }
  uint64_t bps_clock_speed, pps_clock_speed;
  sts = bf_drv_get_clock_speed(dev_id, &bps_clock_speed, &pps_clock_speed);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("%s:%d : Unable to get the clock speed for dev %d : err %d(%s)",
              __func__,
              __LINE__,
              dev_id,
              sts,
              bf_err_str(sts));
    return sts;
  }
  *timeout_us = ((uint64_t)num_clocks * 1000000) / bps_clock_speed;
  return sts;
}
bf_dev_type_t bf_drv_get_dev_type(bf_dev_id_t dev_id) {
  return lld_sku_get_dev_type(dev_id);
}

#ifdef PORT_MGR_HA_UNIT_TESTING
extern bf_status_t bf_port_mac_corr_action_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bf_ha_corrective_action_t *corr);
extern bf_status_t bf_port_serdes_corr_action_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_corrective_action_t *corr);
/**
 * Get the MAC reconciliation actions as determined by the port mgr.
 * (This function is mainly for debug purposes)
 * @param dev_id The ID of the device
 * @param dev_port Device port number
 * @param ca Corrective action for the port as determined by the port mgr
 * 	  based on the state of the MAC
 * @return Status of the API call
 */
bf_status_t bf_ha_port_mgr_port_corrective_action_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, bf_ha_corrective_action_t *ca) {
  if (!DEV_PORT_VALIDATE(dev_port)) return BF_INVALID_ARG;
  if (ca == NULL) return BF_INVALID_ARG;
  bf_status_t sts = bf_port_mac_corr_action_get(dev_id, dev_port, ca);
  return sts;
}

/**
 * Get the Serdes reconciliation actions as determined by the port mgr.
 * (This function is mainly for debug purposes)
 * @param dev_id The ID of the device
 * @param dev_port Device port number
 * @param ca Corrective action for the port as determined by the port mgr
 * 	  based on the state of the Serdes
 * @return Status of the API call
 */
bf_status_t bf_ha_port_mgr_serdes_corrective_action_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, bf_ha_corrective_action_t *ca) {
  if (!DEV_PORT_VALIDATE(dev_port)) return BF_INVALID_ARG;
  if (ca == NULL) return BF_INVALID_ARG;
  bf_status_t sts = bf_port_serdes_corr_action_get(dev_id, dev_port, ca);
  return sts;
}
#endif

void bf_warm_init_error_set(bf_dev_id_t dev_id, bool state) {
  if (!dev_id_set[dev_id]) return;
  dev_id_set[dev_id]->warm_init_error = state;
}

bool bf_warm_init_error_get(bf_dev_id_t dev_id) {
  if (!dev_id_set[dev_id]) return false;
  return dev_id_set[dev_id]->warm_init_error;
}
