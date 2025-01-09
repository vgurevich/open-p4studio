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
#include <string.h>  //for memset
#else
#include <linux/string.h>
#define bf_sys_assert(x) \
  do {                   \
  } while (0)
#endif

#include <dvm/bf_drv_intf.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_err.h>
#include "lld.h"
#include "lld_map.h"
#include "lld_memory_mapping.h"
#include "lld_dev.h"

// global LLD context
lld_context_t g_ctx;
lld_context_t *lld_ctx = &g_ctx;

extern void lld_debug_init(void);

/** \brief Set the register write function
 *
 * \param fn : bf_reg_wr_fn : user-configurable access function
 */
void lld_set_wr_fn(bf_reg_wr_fn fn) { lld_ctx->wr_fn = fn; }

/** \brief Set the register read function
 *
 * \param fn : bf_reg_rd_fn : user-configurable access function
 */
void lld_set_rd_fn(bf_reg_rd_fn fn) { lld_ctx->rd_fn = fn; }

/** \brief Set the is_master flag
 *
 * \param is_master : bool : whether this LLD instance is the master instance
 */
void lld_set_is_master(bool is_master) { lld_ctx->is_master = is_master; }

/** \brief Check if this LLD instance is the "master LLD" instance
 *
 * \param is_master : bool : whether this LLD instance is the master instance
 */
bool lld_is_master(void) { return (lld_ctx->is_master ? true : false); }

/** \brief Initialize LLD module
 *
 * \param is_master, LLD instance is "master" and will perform
 * core-reset and interrupt initialization
 * \param wr_fn, user-defined register write callback fn
 * \param rd_fn, user-defined register read callback fn
 * called during bf_device_add
 */
void lld_init(bool is_master, bf_reg_wr_fn wr_fn, bf_reg_rd_fn rd_fn) {
  /* Register for notificatons */
  bf_drv_client_handle_t bf_drv_hdl;
  bf_status_t r;
  bf_drv_client_callbacks_t callbacks;

  // init and clear LLD context structure
  memset(lld_ctx, 0, sizeof(*lld_ctx));

  // set register access functions
  lld_set_wr_fn(wr_fn);
  lld_set_rd_fn(rd_fn);

  // flag as "master" LLD instance
  lld_set_is_master(is_master);

  // register with dvm for important callbacks
  r = bf_drv_register("lld", &bf_drv_hdl);
  bf_sys_assert(r == BF_SUCCESS);

  memset((void *)&callbacks, 0, sizeof(bf_drv_client_callbacks_t));

  callbacks.pkt_mgr_dev_add = lld_master_dev_add;
  callbacks.device_add = lld_dev_add;
  callbacks.device_del = lld_dev_remove;
  callbacks.lock = lld_dev_lock;
  callbacks.core_reset = lld_reset_core;
  callbacks.unlock_reprogram_core = lld_dev_unlock;
  callbacks.warm_init_quick = lld_warm_init_quick;

  bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_5);

  /* init the mutexes for indirect access registers */
  lld_dev_lock_ind_reg_mutex_init();

  // initialize debug environment
  lld_debug_init();
}

/** \brief Returns 1 if specified 32b address references pipe register
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param addr  : target address
 *
 * \returns true : if addr is a pipe address
 * \returns false: if addr is not a pipe address
 */
bool lld_is_pipe_addr(bf_dev_id_t dev_id, uint32_t addr) {
  if (lld_dev_is_tofino(dev_id)) {
    return (((addr >> 25) & 1) ? true : false);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return (((addr >> 26) & 1) ? true : false);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return (((addr >> 26) & 1) ? true : false);
  }
  return false;
}

/** \brief Returns pipe associated with specified 32b address or -1
 *         if not a pipe address.
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param addr  : target address
 *
 * \returns 2 bit pipe number from target address (0-3)
 * \returns -1 if not a pipe address
 */
int lld_get_pipe_from_addr(bf_dev_id_t dev_id, uint32_t addr) {
  if (!lld_is_pipe_addr(dev_id, addr)) return -1;

  if (lld_dev_is_tofino(dev_id)) {
    return ((addr >> 23) & 3);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return ((addr >> 24) & 3);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return ((addr >> 24) & 3);
  }
  return -1;
}

/** \brief Returns stage associated with specified 32b address or -1
 *         if not a pipe address.
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param addr  : target address
 *
 * \returns stage number from target address
 * \returns -1 if not a pipe address
 */
int lld_get_stage_from_addr(bf_dev_id_t dev_id, uint32_t addr) {
  if (!lld_is_pipe_addr(dev_id, addr)) return -1;

  if (lld_dev_is_tofino(dev_id)) {
    return ((addr >> 19) & 0xF);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return ((addr >> 19) & 0x1F);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return ((addr >> 19) & 0x1F);
  }
  return -1;
}
