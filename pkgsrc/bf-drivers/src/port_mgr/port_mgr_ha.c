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
#include <stdint.h>
#include <string.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
//#include <lld/lld_err.h>
//#include <lld/lld_sku.h>
#include "port_mgr_ha.h"
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/port_mgr.h>
#include "port_mgr_dev.h"
#include "port_mgr_map.h"
#include "port_mgr_log.h"
#include "port_mgr_tof1/port_mgr_tof1_ha.h"
#include "port_mgr_tof2/port_mgr_tof2_ha.h"

/********************************************************************
 ** \brief port_mgr_ha_hardware_read
 *         Read the hardare during warm reboot
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : hardware read successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 *******************************************************************/
bf_status_t port_mgr_ha_hardware_read(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_ha_hardware_read(dev_id);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    return port_mgr_tof2_ha_hardware_read(dev_id);
  } else if (port_mgr_dev_is_tof3(dev_id)) {
    // return port_mgr_tof3_ha_hardware_read(dev_id);
  }
  return BF_SUCCESS;
}

/**********************************************************************
 * \brief Compute deltas between sw and hw config
 *
 * \param dev_id   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param disable_input_pkts : indicates if tx fifo needs to be disabled
 *
 * \return: nothing
 **********************************************************************/
bf_status_t port_mgr_ha_compute_delta_changes(bf_dev_id_t dev_id,
                                              bool disable_input_pkts) {
  bf_status_t st = BF_SUCCESS;

  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);

  st = port_mgr_ha_hardware_read(dev_id);
  if (st != BF_SUCCESS) return st;

  if (port_mgr_dev_is_tof1(dev_id)) {
    st = port_mgr_tof1_ha_compute_delta_changes(dev_id);
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    st = port_mgr_tof2_ha_compute_delta_changes(dev_id);
  } else {
    st = BF_INVALID_ARG;
  }
  if (st != BF_SUCCESS) return st;

  if (disable_input_pkts) {
    st = port_mgr_ha_disable_traffic(dev_id);
    if (st != BF_SUCCESS) return st;
  }

  return BF_SUCCESS;
}

/********************************************************************
 ** \brief Placeholder for port mgr
 *
 * \param dev_id   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 *******************************************************************/
bf_status_t port_mgr_ha_push_delta_changes(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Do nothing in this callback since the delta push for the port manager
     would be co-ordinated by the DVM by making the relevant port add/del/
     enb/dis calls */
  (void)dev_id;
  return BF_SUCCESS;
}

/********************************************************************
 *
 *******************************************************************/
bf_status_t port_mgr_ha_register_port_corr_action(
    bf_dev_id_t dev_id, bf_ha_port_reconcile_info_per_device_t *recon_info) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;

  if (!recon_info) return BF_INVALID_ARG;

  /* Copy the reconciliation info determined by the port mgr for all the ports*/
  memcpy(recon_info,
         &dev_p->port_mgr_port_recon_info,
         sizeof(dev_p->port_mgr_port_recon_info));

  port_mgr_log("%s:%d Exiting after registering port corr action for dev %d",
               __func__,
               __LINE__,
               dev_id);
  return BF_SUCCESS;
}

/********************************************************************
 ** \brief Set the ha stage maintained by the port mgr on a per device basis
 *         to none
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return Status of the API call
 *******************************************************************/
bf_status_t port_mgr_ha_port_delta_push_done(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  bf_dev_pipe_t pipe_id;
  int port_id;
  uint32_t num_pipes = 0;
  bf_dev_port_t dev_port;
  port_mgr_port_t *port_p = NULL;
  int oper_state = 0;
  bf_status_t sts = BF_SUCCESS;
  /* Simply return for virtual devices */
  if (bf_drv_is_device_virtual(dev_id)) return BF_SUCCESS;
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);
  if (dev_p == NULL) return BF_INVALID_ARG;
  dev_p->ha_stage = PORT_MGR_HA_NONE;

  /* After we have pushed the delta for ports, iterate over all the ports and
     explicitly send notifications for the following reasons */
  /* We need to execute port link up/down actions so that the higher
   * level applications are notified of the link status of the port during HA.
   * If this is not done, the higher level applications will never get
   * notified about the link status of the port and thus is the port was UP
   * before HA and stayed UP even after the HA sequence, the higher application
   * will think that the port is DOWN after HA. This can lead to undesired
   * behavior. Also if a port was up before HA but was not enabled during
   * cfg replay, then we will disable the port. But that will remove the port
   * from the fsm and thus the user will never be notified about the link
   * going down
   */
  lld_sku_get_num_active_pipes(dev_id, &num_pipes);

  // for each (possible) port_p
  for (pipe_id = 0; pipe_id < num_pipes; pipe_id++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe_id, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p == NULL) return BF_INVALID_ARG;

      if (!port_p->sw.assigned) {
        /* This port has not been created so do not issue any notifications for
         * it. */
        continue;
      }

      if (port_p->hw.lpbk_mode != BF_LPBK_PIPE) {
        sts = bf_port_oper_state_get(dev_id, dev_port, &oper_state);
        if (sts != BF_SUCCESS) {
          port_mgr_log(
              "%s:%d %d:%3d: Unable to get oper state of port with err %d",
              __func__,
              __LINE__,
              dev_id,
              dev_port,
              sts);
          continue;
        }
      }
      // Always issue callbacks when the warm init ends for all the
      // valid ports
      port_p->issue_callbacks = true;
      sts = bf_port_oper_state_callbacks_issue(dev_id, dev_port);
      if (sts != BF_SUCCESS) {
        port_mgr_log(
            "%s:%d %d:%3d: Unable to issue cb for oper state %d of port with "
            "err %d",
            __func__,
            __LINE__,
            dev_id,
            dev_port,
            oper_state,
            sts);
        continue;
      }
    }
  }

  // one last time for the CPU port
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p == NULL) return BF_INVALID_ARG;
    if (!port_p->sw.assigned) {
      /* This port has not been created so do not issue any notifications for
       * it. */
      continue;
    }
    sts = bf_port_oper_state_get(dev_id, dev_port, &oper_state);
    if (sts != BF_SUCCESS) {
      port_mgr_log("%s:%d %d:%3d: Unable to get oper state with err : %d ",
                   __func__,
                   __LINE__,
                   dev_id,
                   dev_port,
                   sts);
      continue;
    }
    // Always issue callbacks when the warm init ends for all the
    // valid ports
    port_p->issue_callbacks = true;
    sts = bf_port_oper_state_callbacks_issue(dev_id, dev_port);
    if (sts != BF_SUCCESS) {
      port_mgr_log(
          "%s:%d %d:%3d: Unable to issue cb for oper state %d of port with err "
          "%d",
          __func__,
          __LINE__,
          dev_id,
          dev_port,
          oper_state,
          sts);
      continue;
    }
  }

  port_mgr_log("%s:%d Exiting port delta push done for dev %d",
               __func__,
               __LINE__,
               dev_id);
  return BF_SUCCESS;
}

bf_status_t port_mgr_ha_disable_traffic(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (port_mgr_dev_is_tof1(dev_id)) {
    /* Nothing to do for Tofino, traffic is disabled at IPB. */
    return BF_SUCCESS;
  } else if (port_mgr_dev_is_tof2(dev_id)) {
    return port_mgr_tof2_ha_disable_traffic(dev_id);
  }
  return BF_INVALID_ARG;
}

/** \brief Enable port configs to accept and transmit packets
 *         after core reset during fast reconfig
 *
 * \param dev_id    : bf_dev_id_t           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: bf_status_t    : 0 on success, error codes on failure
 */
bf_status_t port_mgr_ha_enable_input_packets(bf_dev_id_t dev_id) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_ha_enable_input_packets(dev_id);
  } else {
    return port_mgr_tof2_ha_enable_traffic(dev_id);
  }
  return BF_SUCCESS;
}

bf_status_t port_mgr_ha_port_serdes_upgrade(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t fw_ver,
                                            char *fw_path) {
  port_mgr_log("%s:%d:%d", __func__, __LINE__, dev_id);
  if (port_mgr_dev_is_tof1(dev_id)) {
    return port_mgr_tof1_ha_port_serdes_upgrade(
        dev_id, dev_port, fw_ver, fw_path);
  } else {
    // return port_mgr_tof2_ha_port_serdes_upgrade(dev_id, dev_port, fw_ver,
    // fw_path);
  }
  return BF_SUCCESS;
}

/******************************************************************
 * bf_ha_stage_is_valid
 ******************************************************************/
bool bf_ha_stage_is_valid(bf_dev_id_t dev_id) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p == NULL) return false;

  if ((dev_p->ha_stage != PORT_MGR_HA_NONE) &&
      (dev_p->ha_stage != PORT_MGR_HA_CFG_REPLAY) &&
      (dev_p->ha_stage != PORT_MGR_HA_DELTA_PUSH)) {
    port_mgr_log("Error: Unexpected HA stage: %d for dev %d at %s:%d",
                 dev_p->ha_stage,
                 dev_id,
                 __func__,
                 __LINE__);
    return false;
  }

  return true;
}
