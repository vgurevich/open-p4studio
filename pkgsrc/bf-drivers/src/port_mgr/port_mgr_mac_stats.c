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

// for aim_printf
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#include <dvm/bf_drv_intf.h>
#include <port_mgr/port_mgr_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/bf_dma_dr_id.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <bf_types/bf_types.h>
#include <port_mgr/port_mgr_ha.h>
#include <port_mgr/port_mgr.h>
#include <port_mgr/port_mgr_map.h>
#include <port_mgr/port_mgr_log.h>
#include "port_mgr_dev.h"
#include "port_mgr/port_mgr_tof2/port_mgr_tof2_umac4.h"
#include "port_mgr/port_mgr_tof3/port_mgr_tof3_tmac.h"

/*
   MAC (RMON) stats are maintained in a combination of HW and SW counters.
   The HW contains 64b counters representing the most recent activity.

   There are two types of SW counters:
    1) The "MAC stat cache"
    2) The "historical" counters

   The MAC stat cache contains the counter values most recently read from the
   HW,
   either via DMA (asynchronous) or direct access (synchronous).

   The historical counters maintain the counts accumulated since he port was
   added.

   Certain operations required to bring a port to the operational state have the
   side
   effect of clearing the HW counters. To preserve them, they are retreived and
   added
   to the current historical counters prior to the operation that would clear
   them.

   The stats returned to the user are always the MAC stat cache values plus the
   corresponding historical value, giving the total accumulated since the port
   was
   added.

   For tofino1, the stats are cleared by the MAC swrst bit toggling
   For tofino2, the stats are cleared by the MAC Rx reset bit toggling

   +-----+ Sync    +-----+        +----------+
   | HW  | ------> |MAC  |------> |Historical| ------> User
   | ctrs|         |stat |   +    |ctrs      |    =    Values user sees
   +-----+         |cache|        +----------+
                   +-----+
                     ^
          ASync      |
          -----------+
          (DMA)

*/

/***************************************************************
 * port_mgr_mac_stats_copy
 *
 * Clear the referenced SW counter array.
 * This can be used to clear the MAC stats cache, the historical
 * counters, or a user defined area
 ***************************************************************/
void port_mgr_mac_stats_clear_sw_ctrs(bf_rmon_counter_array_t *sw_ctrs) {
  uint32_t ctr_id;

  for (ctr_id = 0; ctr_id < BF_NUM_RMON_COUNTERS; ctr_id++) {
    sw_ctrs->format.ctr_array[ctr_id] = 0;
  }
}

/***************************************************************
 * port_mgr_mac_stats_copy
 *
 * Add "new_ctrs" to "historical_ctrs" and place the result in
 * "dest_ctrs".
 ***************************************************************/
void port_mgr_mac_stats_copy(bf_rmon_counter_array_t *new_ctrs,
                             bf_rmon_counter_array_t *historical_ctrs,
                             bf_rmon_counter_array_t *dest_ctrs) {
  uint32_t ctr_id;

  for (ctr_id = 0; ctr_id < BF_NUM_RMON_COUNTERS; ctr_id++) {
    dest_ctrs->format.ctr_array[ctr_id] =
        new_ctrs->format.ctr_array[ctr_id] +
        historical_ctrs->format.ctr_array[ctr_id];
  }
}

/***************************************************************
 * __bf_port_mac_stats_cb_proxy
 *
 * Stub DMA cb to proxy user callback
 ***************************************************************/
static void __bf_port_mac_stats_cb_proxy(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         bf_dma_dr_id_t dr,
                                         uint64_t dma_timestamp_nsec,
                                         uint32_t attr,
                                         uint32_t status,
                                         uint32_t type,
                                         uint64_t msg_id,
                                         int s,
                                         int e) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p) {
    if (dev_p->mac_stats_callback_fn) {
      dev_p->mac_stats_callback_fn(dev_id, status, msg_id, dma_timestamp_nsec);
    }
  }
  (void)dr;
  (void)attr;
  (void)type;
  (void)s;
  (void)e;
  (void)subdev_id;
}

/** \brief Register a callback to be issued upon completion
 *         of MAC stats DMA operations for a given dev_id.
 *
 * \param dev_id: bf_dev_id_t            : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param fn  : bf_mac_stats_callback_t: user-defined callback function
 *
 * \return: BF_SUCCESS                   : callback registered successfully
 * \return: BF_INVALID_ARG               : dev_id never added or dev_id >
 *BF_MAX_DEV_COUNT-1
 *
 */
bf_status_t bf_bind_mac_stats_cb(bf_dev_id_t dev_id,
                                 bf_mac_stats_callback_t fn) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p) {
    dev_p->mac_stats_callback_fn = fn;
    // register our own to proxy the completion to the users
    // (which has a simpler signature).
    lld_register_completion_callback(
        dev_id, 0, lld_dr_cmp_mac_stat, __bf_port_mac_stats_cb_proxy);
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

/*
 */
uint64_t port_mgr_mac_stat_dma_msg_id_encode(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port) {
  return (((uint64_t)dev_id) << 32ull) | (uint64_t)dev_port;
}

/*
 */
void port_mgr_mac_stat_dma_msg_id_decode(uint64_t msg_id,
                                         bf_dev_id_t *dev_id,
                                         bf_dev_port_t *dev_port) {
  *dev_id = (bf_dev_id_t)(msg_id >> 32ull);
  *dev_port = (bf_dev_port_t)(msg_id & 0xffffffffull);
}

static void port_mgr_mac_stat_dma_callback(bf_dev_id_t dev_id,
                                           int status,
                                           uint64_t msg_id,
                                           uint64_t dma_timestamp_nsec) {
  bf_dev_id_t dev_id_from_msg_id;
  bf_dev_port_t dev_port_from_msg_id;
  port_mgr_dev_t *dev_p;
  port_mgr_port_t *port_p;
  bf_rmon_counter_array_t combined_ctrs;
  (void)dev_id;

  if (status != 0) {
    port_mgr_log("WARNING: MAC stat DMA returned non-0 status: %xh\n", status);
  }
  port_mgr_mac_stat_dma_msg_id_decode(
      msg_id, &dev_id_from_msg_id, &dev_port_from_msg_id);

  dev_p = port_mgr_map_dev_id_to_dev_p_allow_unassigned(dev_id_from_msg_id);
  if (dev_p == NULL) return;  // dev_id deleted?

  port_p =
      port_mgr_map_dev_port_to_port(dev_id_from_msg_id, dev_port_from_msg_id);
  if (port_p == NULL) return;  // port deleted?

  // disallow simultaneous access as cache is being updated
  bf_sys_mutex_lock(&port_p->port_mtx);
  /* Unmap the DMA address of the buffer before it is processed by the client */
  if (bf_sys_dma_unmap(port_p->mac_stat_hndl,
                       port_p->mac_stat_dma_vaddr,
                       port_p->mac_stat_buf_size,
                       BF_DMA_TO_CPU) != 0) {
    port_mgr_log("Unable to unmap DMA buffer %p\n",
                 (void *)port_p->mac_stat_dma_vaddr);
  }

  if (port_mgr_dev_is_tof3(dev_id)) {
    port_mgr_tof3_tmac_to_rmon_ctr_copy(
        (uint64_t *)port_p->mac_stat_dma_vaddr,  // DMA bufr
        (uint64_t *)&port_p->mac_stat_cache.format.ctr_array);

    if (port_p->mac_stat_user_cb) {
      // make a temporary copy of the combined counters for the callback
      port_mgr_mac_stats_copy(&port_p->mac_stat_cache,
                              &port_p->mac_stat_historical,
                              &combined_ctrs);
    }
    // cache copied now, safe for other accesses
    bf_sys_mutex_unlock(&port_p->port_mtx);

    // if user callback requested, issue it now
    if (port_p->mac_stat_user_cb) {
      port_p->mac_stat_user_cb(dev_id_from_msg_id,
                               dev_port_from_msg_id,
                               &combined_ctrs,
                               dma_timestamp_nsec,
                               port_p->mac_stat_user_data);
    }
    return;
  } else if (port_mgr_dev_is_tof2(dev_id)) {  // is tof2
    int is_cpu_port = false;

    port_mgr_map_dev_port_to_all(
        dev_id, dev_port_from_msg_id, NULL, NULL, NULL, NULL, &is_cpu_port);
    if (!is_cpu_port) {
      // Need to convert UMAC4 to UMAC3 counter array
      port_mgr_tof2_umac4_to_umac3_ctr_copy(
          (uint64_t *)port_p->mac_stat_dma_vaddr,  // DMA bufr
          (uint64_t *)&port_p->mac_stat_cache.format.ctr_array);
    } else {
      // copy DMA area to cached counters
      memcpy((char *)&port_p->mac_stat_cache.format.ctr_array,
             (char *)port_p->mac_stat_dma_vaddr,
             sizeof(port_p->mac_stat_cache.format.ctr_array));
    }
  } else {
    // copy DMA area to cached counters
    memcpy((char *)&port_p->mac_stat_cache.format.ctr_array,
           (char *)port_p->mac_stat_dma_vaddr,
           sizeof(port_p->mac_stat_cache.format.ctr_array));
  }

  if (port_p->mac_stat_user_cb) {
    // make a temporary copy of the combined counters for the callback
    port_mgr_mac_stats_copy(
        &port_p->mac_stat_cache, &port_p->mac_stat_historical, &combined_ctrs);
  }
  // cache copied now, safe for other accesses
  bf_sys_mutex_unlock(&port_p->port_mtx);

  // if user callback requested, issue it now
  if (port_p->mac_stat_user_cb) {
    port_p->mac_stat_user_cb(dev_id_from_msg_id,
                             dev_port_from_msg_id,
                             &combined_ctrs,
                             dma_timestamp_nsec,
                             port_p->mac_stat_user_data);
  }
}

/********************************************************************
 * port_mgr_dma_alloc_mac_stats
 * Allocate one large DMA buffer to be used for MAC stats DMAs for
 * all ports on a given device. Uses the BF_DMA_MAC_STAT_RECEIVE
 * pool.
 * Called on device_add by port_mgr to allocate DMA space for
 * MAC stats DMAs.
 *******************************************************************/
static bf_status_t port_mgr_dma_alloc_mac_stats(bf_dev_id_t dev_id,
                                                uint32_t sz,
                                                void **vaddr,
                                                uint64_t *paddr,
                                                bf_sys_dma_pool_handle_t *hndl,
                                                uint32_t *buf_size) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p == NULL) {
    port_mgr_log("Warning: dev_id %d not found!\n", dev_id);
    return BF_INVALID_ARG;
  }

  struct bf_dma_info_s *bf_dma_info_p = &dev_p->dma_info;
  // allocate dma buffer for LLD internal use
  if (sz > bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_size) {
    port_mgr_log(
        "Warning: requested %d bytes, buffers are only %d bytes. Increase pool "
        "sz\n",
        sz,
        bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_size);
    return BF_INVALID_ARG;
  }
  if (bf_sys_dma_alloc(
          bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE]
              .dma_buf_pool_handle,
          bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_size,
          vaddr,
          paddr)) {
    port_mgr_log("Error: dma buff alloc failed for dev_id%d\n", dev_id);
    return BF_INVALID_ARG;
  }
  *hndl =
      bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_pool_handle;
  *buf_size =
      bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_size;
  return BF_SUCCESS;
}

/********************************************************************
 * port_mgr_init_mac_stats
 * Allocate a DMA buffer to be used for MAC stats DMAs for
 * each port on a given device. Uses the BF_DMA_MAC_STAT_RECEIVE
 * pool.
 * Called on device_add by port_mgr to allocate DMA space for
 * MAC stats DMAs.
 *******************************************************************/
void port_mgr_init_mac_stats(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  bf_status_t bf_status;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;
  port_mgr_port_t *port_p;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p != NULL) {
        bf_status =
            port_mgr_dma_alloc_mac_stats(dev_id,
                                         sizeof(bf_rmon_counter_array_t),
                                         (void **)&port_p->mac_stat_dma_vaddr,
                                         &port_p->mac_stat_dma_paddr,
                                         &port_p->mac_stat_hndl,
                                         &port_p->mac_stat_buf_size);
        if (bf_status != BF_SUCCESS) {
          port_mgr_log(
              "Error: %d : allocating mac stat DMA area for %d:%d:%d\n",
              bf_status,
              dev_id,
              pipe,
              port_id);
        }
      }
    }
  }
  // one last one for the CPU port (mac-blk 64)
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p != NULL) {
      bf_status =
          port_mgr_dma_alloc_mac_stats(dev_id,
                                       sizeof(bf_rmon_counter_array_t),
                                       (void **)&port_p->mac_stat_dma_vaddr,
                                       &port_p->mac_stat_dma_paddr,
                                       &port_p->mac_stat_hndl,
                                       &port_p->mac_stat_buf_size);
      if (bf_status != BF_SUCCESS) {
        port_mgr_log("Error: %d : allocating mac stat DMA area for %d:%d:%d\n",
                     bf_status,
                     dev_id,
                     DEV_PORT_TO_PIPE(dev_port),
                     DEV_PORT_TO_LOCAL_PORT(dev_port));
      }
    }
  }
  // set the port_mgr mac stat DMA callback
  bf_bind_mac_stats_cb(dev_id, port_mgr_mac_stat_dma_callback);
}

/********************************************************************
 * port_mgr_dma_free_mac_stats
 * De-allocate DMA buffer used for MAC stats DMAs for
 * all ports on a given device. Frees buffer in the BF_DMA_MAC_STAT_RECEIVE
 * pool.
 * Called on device_rmv by port_mgr to de-allocate DMA space for
 * MAC stats DMAs.
 *******************************************************************/
static bf_status_t port_mgr_dma_free_mac_stats(bf_dev_id_t dev_id,
                                               void *vaddr) {
  // port_mgr_dev_t *dev_p = port_mgr_map_dev_id_to_dev_p(dev_id);
  port_mgr_ldev_t *dev_p = port_mgr_dev_logical_dev_get(dev_id);

  if (dev_p == NULL) {
    port_mgr_log("Warning: dev_id %d not found!", dev_id);
    return BF_INVALID_ARG;
  }

  struct bf_dma_info_s *bf_dma_info_p = &dev_p->dma_info;
  bf_sys_dma_free(
      bf_dma_info_p->dma_buff_info[BF_DMA_MAC_STAT_RECEIVE].dma_buf_pool_handle,
      vaddr);
  return BF_SUCCESS;
}

/********************************************************************
 * port_mgr_free_mac_stats
 * De-allocate DMA buffer used for MAC stats DMAs for
 * each port on a given device. Frees buffer in the BF_DMA_MAC_STAT_RECEIVE
 * pool.
 * Called on device_rmv by port_mgr to de-allocate DMA space for
 * MAC stats DMAs.
 *******************************************************************/
void port_mgr_free_mac_stats(bf_dev_id_t dev_id) {
  bf_dev_pipe_t pipe;
  int port_id;
  bf_status_t bf_status;
  bf_dev_port_t dev_port;
  uint32_t num_pipes = 0;
  port_mgr_port_t *port_p;

  lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  for (pipe = 0; pipe < num_pipes; pipe++) {
    for (port_id = lld_get_min_fp_port(dev_id);
         port_id <= lld_get_max_fp_port(dev_id);
         port_id++) {
      dev_port = MAKE_DEV_PORT(pipe, port_id);
      port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
      if (port_p != NULL) {
        bf_status = port_mgr_dma_free_mac_stats(
            dev_id, (void *)port_p->mac_stat_dma_vaddr);
        if (bf_status != BF_SUCCESS) {
          port_mgr_log("Error: %d : freeing mac stat DMA area for %d:%d:%d\n",
                       bf_status,
                       dev_id,
                       pipe,
                       port_id);
        }
      }
    }
  }
  // one last one for the CPU port (mac-blk 64)
  for (port_id = lld_get_min_cpu_port(dev_id);
       port_id <= lld_get_max_cpu_port(dev_id);
       port_id++) {
    dev_port = MAKE_DEV_PORT(0, port_id);  // CPU port
    port_p = port_mgr_map_dev_port_to_port_allow_unassigned(dev_id, dev_port);
    if (port_p != NULL) {
      bf_status = port_mgr_dma_free_mac_stats(
          dev_id, (void *)port_p->mac_stat_dma_vaddr);
      if (bf_status != BF_SUCCESS) {
        port_mgr_log("Error: %d : freeing mac stat DMA area for %d:%d:%d\n",
                     bf_status,
                     dev_id,
                     DEV_PORT_TO_PIPE(dev_port),
                     DEV_PORT_TO_LOCAL_PORT(dev_port));
      }
    }
  }
}
