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
#include <lld/bf_dma_if.h>
#include <lld/bf_lld_if.h>
#include "lld.h"
#include "lld_log.h"
#include "lld_map.h"

/**
 * @file bf_lld_if.c
 * \brief Details LLD module-level APIs.
 *
 */

/**
 * @addtogroup lld-api
 * @{
 * This is a description of some APIs.
 */

// fwd refs
char *bf_dbg_get_full_reg_path_name(uint32_t offset);
bf_status_t bf_bind_wr_fn(bf_reg_wr_fn fn);
bf_status_t bf_bind_rd_fn(bf_reg_rd_fn fn);

/** \brief Initializa the LLD submodule of a process
 *
 * \param is_master    : whether this LLD instance is the "master" LLD instance
 * \param wr_fn        : Function used to write 32b chip registers
 * \param rd_fn        : Function used to read  32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
bf_status_t bf_lld_init(bool is_master,
                        bf_reg_wr_fn wr_fn,
                        bf_reg_rd_fn rd_fn) {
  lld_init(is_master, wr_fn, rd_fn);
  return BF_SUCCESS;
}

/** \brief Set the 32b register write function for LLD to use
 *
 * \param fn           : Function used to write 32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
bf_status_t bf_lld_bind_wr_fn(bf_reg_wr_fn fn) {
  lld_set_wr_fn(fn);
  return BF_SUCCESS;
}

/** \brief Set the 32b register write function for LLD to use
 *
 * \param fn           : Function used to read 32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
bf_status_t bf_lld_bind_rd_fn(bf_reg_rd_fn fn) {
  lld_set_rd_fn(fn);
  return BF_SUCCESS;
}

/** \brief Debug function to return the full hierarchical path name of a chip
 *register
 *
 * \param fn           : Function used to read 32b chip registers
 *
 * \return: BF_SUCCESS : LLD initialized successfully
 */
char *bf_lld_dbg_get_full_reg_path_name(bf_dev_family_t dev_family,
                                        uint32_t offset) {
  return lld_reg_parse_get_full_reg_path_name(dev_family, offset);
}

// legacy API, kept for backwards-compat.
char *bf_dbg_get_full_reg_path_name(uint32_t offset) {
  return bf_lld_dbg_get_full_reg_path_name(BF_DEV_FAMILY_TOFINO, offset);
}

// legacy API, kept for backwards-compat.
bf_status_t bf_bind_wr_fn(bf_reg_wr_fn fn) { return bf_lld_bind_wr_fn(fn); }

// legacy API, kept for backwards-compat.
bf_status_t bf_bind_rd_fn(bf_reg_rd_fn fn) { return bf_lld_bind_rd_fn(fn); }

bool bf_lld_dev_is_tof1(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p(dev_id);

  if (dev_p) {
    if (dev_p->dev_family == BF_DEV_FAMILY_TOFINO) {
      return true;
    }
  }
  return false;
}

bool bf_lld_dev_is_tof2(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p(dev_id);

  if (dev_p) {
    if (dev_p->dev_family == BF_DEV_FAMILY_TOFINO2) {
      return true;
    }
  }
  return false;
}

bool bf_lld_dev_is_tof3(bf_dev_id_t dev_id) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p(dev_id);

  if (dev_p) {
    if (dev_p->dev_family == BF_DEV_FAMILY_TOFINO3) {
      return true;
    }
  }
  return false;
}
