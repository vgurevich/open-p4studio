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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <bf_types/bf_types.h>
#endif
#include "lld.h"
#include "lld_log.h"
#include "lld_map.h"
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include "lld_tof_interrupt.h"
#include "lld_tof2_interrupt.h"
#include "lld_tof3_interrupt.h"

#include <lld/lld_interrupt_if.h>
#include "lld_memory_mapping.h"
#include "lld_dev.h"
#ifndef __KERNEL__
#include <diag/bf_dev_diag.h>
#include <lld/lld_diag_ext.h>
#endif

// for telnet access
extern void *rl_outstream;

uint32_t lld_map_int_nbr_to_sh_int_reg(bf_int_nbr_t int_nbr) {
  return (int_nbr >> 5);
}

uint32_t lld_map_int_nbr_to_sh_int_bit(bf_int_nbr_t int_nbr) {
  return (int_nbr & 0x1F);
}

void lld_int_gbl_en_set(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id, bool en) {
  lld_dev_t *dev_p;
  dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (dev_p == NULL) return;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      lld_tof_int_gbl_en_set(dev_id, en);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      lld_tof2_int_gbl_en_set(dev_id, en);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      lld_tof3_int_gbl_en_set(dev_id, subdev_id, en);
      break;



    case BF_DEV_FAMILY_UNKNOWN:
      return;
  }
}

bf_status_t lld_int_claim(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          bf_int_nbr_t int_nbr) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  uint32_t sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
  uint32_t sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);
  uint32_t bit_fld = (1u << sh_int_bit);

  if (!dev_p) return BF_INVALID_ARG;

  // claim ownership of this int_nbr
  dev_p->module_int.shadow_mask[sh_int_reg] |= bit_fld;
  return BF_SUCCESS;
}

void lld_int_svc(bf_dev_id_t dev_id,
                 bf_subdev_id_t subdev_id,
                 uint32_t sh_int_val,
                 uint16_t sh_int_reg) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_svc(dev_id, sh_int_val, sh_int_reg);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_svc(dev_id, sh_int_val, sh_int_reg);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_svc(dev_id, subdev_id, sh_int_val, sh_int_reg);
  }

  if (lld_ctx->mac_int_bh_wakeup_cb) {
    lld_ctx->mac_int_bh_wakeup_cb(dev_id);
  }
}

bool lld_int_nbr_claimed(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bf_int_nbr_t int_nbr) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  uint32_t sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
  uint32_t sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);
  uint32_t bit_fld = (1u << sh_int_bit);

  // make sure this int_nbr is in our subset to control
  if (dev_p && (dev_p->module_int.shadow_mask[sh_int_reg] & bit_fld)) {
    return true;
  }
  return false;
}

bf_status_t lld_int_msk(bf_dev_id_t dev_id,
                        bf_subdev_id_t subdev_id,
                        bf_int_nbr_t int_nbr) {
  uint32_t sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
  uint32_t sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);

  // make sure this int_nbr is in our subset to control
  if (lld_int_nbr_claimed(dev_id, subdev_id, int_nbr)) {
    if (lld_dev_is_tofino(dev_id)) {
      lld_tof_int_msk(dev_id, sh_int_reg, sh_int_bit);
    } else if (lld_dev_is_tof2(dev_id)) {
      lld_tof2_int_msk(dev_id, sh_int_reg, sh_int_bit);
    } else if (lld_dev_is_tof3(dev_id)) {
      lld_tof3_int_msk(dev_id, subdev_id, sh_int_reg, sh_int_bit);
    }
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

bf_status_t lld_int_ena(bf_dev_id_t dev_id,
                        bf_subdev_id_t subdev_id,
                        bf_int_nbr_t int_nbr) {
  uint32_t sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
  uint32_t sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);

  // make sure this int_nbr is in our subset to control
  if (lld_int_nbr_claimed(dev_id, subdev_id, int_nbr)) {
    if (lld_dev_is_tofino(dev_id)) {
      lld_tof_int_ena(dev_id, sh_int_reg, sh_int_bit);
    } else if (lld_dev_is_tof2(dev_id)) {
      lld_tof2_int_ena(dev_id, sh_int_reg, sh_int_bit);
    } else if (lld_dev_is_tof3(dev_id)) {
      lld_tof3_int_ena(dev_id, subdev_id, sh_int_reg, sh_int_bit);
    }
    return BF_SUCCESS;
  }
  return BF_INVALID_ARG;
}

int lld_int_register_cb(bf_dev_id_t dev_id,
                        bf_subdev_id_t subdev_id,
                        uint32_t offset,
                        lld_int_cb cb_fn,
                        void *userdata) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_int_register_cb(dev_id, offset, cb_fn, userdata);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_int_register_cb(dev_id, offset, cb_fn, userdata);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_int_register_cb(dev_id, subdev_id, offset, cb_fn, userdata);
  }
  return -1;
}

uint32_t lld_default_handler(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             uint32_t status_reg,
                             uint32_t status_value,
                             uint32_t enable_hi_reg,
                             uint32_t enable_lo_reg,
                             void *userdata) {
  // default is to just clear them
  lld_subdev_write_register(dev_id, subdev_id, status_reg, status_value);

  (void)enable_hi_reg;
  (void)enable_lo_reg;
  (void)userdata;
  return 0;
}

bf_status_t lld_register_default_handler_all_ints_cb(bf_dev_id_t dev_id,
                                                     bf_subdev_id_t subdev_id,
                                                     void *blk_lvl_int) {
  uint32_t offset;
  if (lld_dev_is_tofino(dev_id)) {
    subdev_id = 0;  // force for tof
    offset = lld_tof_get_status_reg(blk_lvl_int);
  } else if (lld_dev_is_tof2(dev_id)) {
    subdev_id = 0;  // force for tof2
    offset = lld_tof2_get_status_reg(blk_lvl_int);
  } else if (lld_dev_is_tof3(dev_id)) {
    offset = lld_tof3_get_status_reg(blk_lvl_int);
  } else {
    return BF_INVALID_ARG;
  }
  lld_int_register_cb(
      dev_id, subdev_id, offset, lld_default_handler, blk_lvl_int);
  return BF_SUCCESS;
}
void lld_register_default_handler_for_all_ints(bf_dev_id_t dev_id,
                                               bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_traverse_blk_lvl_int(dev_id,
                                     lld_register_default_handler_all_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_traverse_blk_lvl_int(dev_id,
                                      lld_register_default_handler_all_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_register_default_handler_all_ints_cb);
  }
}

lld_int_cb lld_get_int_cb(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          uint32_t offset,
                          void **userdata) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_get_int_cb(dev_id, offset, userdata);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_get_int_cb(dev_id, offset, userdata);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_get_int_cb(dev_id, subdev_id, offset, userdata);
  }
  return NULL;
}

bf_status_t lld_int_disable_all(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if ((dev_id < 0) || (dev_id >= BF_MAX_DEV_COUNT) ||
      (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return status;
  if (lld_dev_is_tofino(dev_id)) {
    status =
        lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_int_disable_all_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_int_disable_all_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_int_disable_all_cb);
  }
  return status;
}

bf_status_t lld_inject_all_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if ((dev_id < 0) || (dev_id >= BF_MAX_DEV_COUNT) ||
      (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return status;
  if (lld_dev_is_tofino(dev_id)) {
    status =
        lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_inject_all_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_inject_all_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_inject_all_ints_cb);
  }
  return status;
}

bf_status_t lld_inject_reg_with_offset(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       uint32_t offset) {
  bf_status_t status = BF_INVALID_ARG;
  if ((dev_id < 0) || (dev_id >= BF_MAX_DEV_COUNT)) return status;

  if (lld_dev_is_tofino(dev_id)) {
    status = lld_tof_inject_ints_with_offset(dev_id, offset);
  } else if (lld_dev_is_tof2(dev_id)) {
    status = lld_tof2_inject_ints_with_offset(dev_id, offset);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_inject_ints_with_offset(dev_id, subdev_id, offset);
  }
  return status;
}

void lld_int_host_leaf_enable_set(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bool en) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_host_leaf_enable_set(dev_id, en);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_host_leaf_enable_set(dev_id, en);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_host_leaf_enable_set(dev_id, subdev_id, en);
  }
}

void lld_int_mbus_leaf_enable_set(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bool en) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_mbus_leaf_enable_set(dev_id, en);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_mbus_leaf_enable_set(dev_id, en);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_mbus_leaf_enable_set(dev_id, subdev_id, en);
  }
}

bf_status_t lld_int_poll(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bool all_ints) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_int_poll(dev_id, all_ints);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_int_poll(dev_id, all_ints);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_int_poll(dev_id, subdev_id, all_ints);
  }

  if (lld_ctx->mac_int_bh_wakeup_cb) {
    lld_ctx->mac_int_bh_wakeup_cb(dev_id);
  }

  return BF_INVALID_ARG;
}

uint32_t lld_int_get_glb_status(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_int_get_glb_status(dev_id);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_int_get_glb_status(dev_id);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_int_get_glb_status(dev_id, subdev_id);
  }
  return -1;
}
uint32_t lld_int_get_shadow_int_status(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       uint16_t sh_int_reg) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_int_get_shadow_int_status(dev_id, sh_int_reg);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_int_get_shadow_int_status(dev_id, sh_int_reg);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_int_get_shadow_int_status(dev_id, subdev_id, sh_int_reg);
  }
  return -1;
}
uint32_t lld_int_get_shadow_msk_status(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       uint16_t sh_msk_reg) {
  if (lld_dev_is_tofino(dev_id)) {
    return lld_tof_int_get_shadow_msk_status(dev_id, sh_msk_reg);
  } else if (lld_dev_is_tof2(dev_id)) {
    return lld_tof2_int_get_shadow_msk_status(dev_id, sh_msk_reg);
  } else if (lld_dev_is_tof3(dev_id)) {
    return lld_tof3_int_get_shadow_msk_status(dev_id, subdev_id, sh_msk_reg);
  }
  return -1;
}
void lld_int_set_shadow_msk_status(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   uint16_t sh_msk_reg,
                                   uint32_t value) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_set_shadow_msk_status(dev_id, sh_msk_reg, value);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_set_shadow_msk_status(dev_id, sh_msk_reg, value);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_set_shadow_msk_status(dev_id, subdev_id, sh_msk_reg, value);
  }
}
bf_status_t lld_clear_all_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status =
        lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_clear_all_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_clear_all_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_clear_all_ints_cb);
  }
  return status;
}
bf_status_t lld_clear_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status = lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_clear_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status = lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_clear_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_clear_ints_cb);
  }
  return status;
}
bf_status_t lld_enable_all_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;

  if ((dev_id >= BF_MAX_DEV_COUNT) || (dev_id < 0) ||
      (subdev_id >= BF_MAX_SUBDEV_COUNT))
    return status;
  if (lld_dev_is_tofino(dev_id)) {
    status = lld_tof_int_traverse_blk_lvl_int(dev_id,
                                              lld_tof_enable_all_pipe_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status = lld_tof2_int_traverse_blk_lvl_int(
        dev_id, lld_tof2_enable_all_pipe_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_enable_all_pipe_ints_cb);
  }
  return status;
}

bf_status_t lld_int_msix_map_set(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_int_nbr_t int_nbr,
                                 int msix_num) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (!dev_p) return BF_INVALID_ARG;
  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return BF_SUCCESS; /* no map, but pretend to be successful */
    case BF_DEV_FAMILY_TOFINO2:
      return lld_tof2_msix_map_set(dev_id, int_nbr, msix_num);
    case BF_DEV_FAMILY_TOFINO3:
      return lld_tof3_msix_map_set(dev_id, subdev_id, int_nbr, msix_num);


    default:
      break;
  }
  return BF_INVALID_ARG;
}

#ifndef __KERNEL__

void lld_dump_int_list(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_dump_int_list_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_dump_int_list_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_dump_int_list_cb);
  }
}

int lld_register_mac_int_poll_cb(lld_mac_int_poll_cb fn) {
  lld_ctx->mac_int_poll_cb = fn;
  return 0;
}

int lld_register_mac_int_dump_cb(lld_mac_int_dump_cb fn) {
  lld_ctx->mac_int_dump_cb = fn;
  return 0;
}

int lld_register_mac_int_bh_wakeup_cb(lld_mac_int_bh_wakeup_cb fn) {
  lld_ctx->mac_int_bh_wakeup_cb = fn;
  return 0;
}

extern int dump_field_name_by_offset(bf_dev_id_t dev_id,
                                     uint32_t target_offset,
                                     int bit);
void lld_dump_new_mac_ints_cb(bf_dev_id_t dev_id,
                              int mac_block,
                              int ch,
                              uint32_t int_reg,
                              int bit,
                              uint32_t total,
                              uint32_t shown) {
  static int last_chip = 0;
  static uint32_t last_int_reg = 0;
  static int last_bit = 0;

  if (total != shown) {
    if ((last_chip != dev_id) || (last_int_reg != int_reg) ||
        (last_bit >= bit)) {
      // dump reg name
      char *path = get_full_reg_path_name(dev_id, int_reg);

      fprintf(rl_outstream,
              "%d: %08x : %s\n",
              dev_id,
              int_reg,
              path ? path : "NONE");
    }
    last_chip = dev_id;
    last_int_reg = int_reg;
    last_bit = bit;

    fprintf(rl_outstream,
            "  [%2d] : %d : %d : %8u : ",
            bit,
            mac_block,
            ch,
            total - shown);
    dump_field_name_by_offset((int)dev_id, int_reg, bit);
    fprintf(rl_outstream, "\n");
  }
}

void lld_clear_new_mac_ints_cb(bf_dev_id_t dev_id,
                               int mac_block,
                               int ch,
                               uint32_t int_reg,
                               int bit,
                               uint32_t total,
                               uint32_t shown) {
  static int last_chip = 0;
  static uint32_t last_int_reg = 0;
  static int last_bit = 0;

  if (total != shown) {
    if ((last_chip != dev_id) || (last_int_reg != int_reg) ||
        (last_bit >= bit)) {
      // dump reg name
      // fprintf(rl_outstream,
      //        "%d: %08x : %s\n",
      //        dev_id,
      //        int_reg,
      //        get_full_reg_path_name(int_reg));
    }
    last_chip = dev_id;
    last_int_reg = int_reg;
    last_bit = bit;

    // fprintf(rl_outstream,
    //        "  [%2d] : %d : %d : %8u : ",
    //        bit,
    //        mac_block,
    //        ch,
    //        total - shown);
    // dump_field_name_by_offset(int_reg, bit);
    // fprintf(rl_outstream, "\n");
  }
  (void)mac_block;
  (void)ch;
}

void lld_dump_all_mac_ints_cb(bf_dev_id_t dev_id,
                              int mac_block,
                              int ch,
                              uint32_t int_reg,
                              int bit,
                              uint32_t total,
                              uint32_t shown) {
  static int last_chip = 0;
  static uint32_t last_int_reg = 0;
  static int last_bit = 0;

  if ((last_chip != dev_id) || (last_int_reg != int_reg) || (last_bit >= bit)) {
    // dump reg name
    char *path = get_full_reg_path_name(dev_id, int_reg);

    fprintf(
        rl_outstream, "%d: %08x : %s\n", dev_id, int_reg, path ? path : "NONE");
  }
  last_chip = dev_id;
  last_int_reg = int_reg;
  last_bit = bit;

  fprintf(
      rl_outstream, "  [%2d] : %d : %d : %8u : ", bit, mac_block, ch, total);
  dump_field_name_by_offset((int)dev_id, int_reg, bit);
  fprintf(rl_outstream, "\n");
  (void)shown;
}

bf_status_t lld_dump_new_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status = lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_dump_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_new_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_dump_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_new_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_dump_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_new_mac_ints_cb, dev_id);
    }
  }
  return status;
}

bf_status_t lld_clear_new_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status =
        lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_clear_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_clear_new_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_clear_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_clear_new_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_clear_new_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_clear_new_mac_ints_cb,
                               dev_id);  // TBD Tf3-fix
    }
  }
  return status;
}

bf_status_t lld_dump_all_ints(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status = lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_dump_all_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_all_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof2(dev_id)) {
    status =
        lld_tof2_int_traverse_blk_lvl_int(dev_id, lld_tof2_dump_all_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_all_mac_ints_cb, dev_id);
    }
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_dump_all_ints_cb);
    if (lld_ctx->mac_int_dump_cb) {
      lld_ctx->mac_int_dump_cb(lld_dump_all_mac_ints_cb, dev_id);  // TBD
                                                                   // Tf3-fix
    }
  }
  return status;
}

bf_status_t lld_dump_unfired_ints(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id) {
  bf_status_t status = BF_INVALID_ARG;
  if (lld_dev_is_tofino(dev_id)) {
    status =
        lld_tof_int_traverse_blk_lvl_int(dev_id, lld_tof_dump_unfired_ints_cb);
  } else if (lld_dev_is_tof2(dev_id)) {
    status = lld_tof2_int_traverse_blk_lvl_int(dev_id,
                                               lld_tof2_dump_unfired_ints_cb);
  } else if (lld_dev_is_tof3(dev_id)) {
    status = lld_tof3_int_traverse_blk_lvl_int(
        dev_id, subdev_id, lld_tof3_dump_unfired_ints_cb);
  }
  return status;
}

int callback_called = 0;
int exp_chip;
uint32_t exp_status_reg;
void *exp_userdata;

uint32_t lld_int_test_cb(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         uint32_t status_reg,
                         uint32_t status_value,
                         uint32_t enable_hi_reg,
                         uint32_t enable_lo_reg,
                         void *userdata) {
  callback_called++;

  if ((dev_id != exp_chip) || (status_reg != exp_status_reg) ||
      (userdata != exp_userdata)) {
    fprintf(
        rl_outstream,
        "Interrupt cb: parameter error: chip:subdev=%d:%d : status_reg=%08x : "
        "userdata=%p\n",
        dev_id,
        subdev_id,
        status_reg,
        userdata);
  } else {
    fprintf(rl_outstream, "Interrupt cb: Passed\n");
  }
  (void)enable_hi_reg;
  (void)enable_lo_reg;
  (void)status_value;
  return 0;
}

void lld_test_int_cb(bf_dev_id_t dev_id,
                     bf_subdev_id_t subdev_id,
                     uint32_t offset) {
  int rc;

  rc = lld_int_register_cb(dev_id,
                           subdev_id,
                           offset,
                           lld_int_test_cb,
                           lld_u64_to_void_ptr(((uint64_t)offset)));
  if (rc < 0) {
    fprintf(rl_outstream,
            "Error: %d : trying to register cb on %d:%d:%08x\n",
            rc,
            dev_id,
            subdev_id,
            offset);
    return;
  }
  callback_called = 0;
  exp_chip = dev_id;
  exp_status_reg = offset;
  exp_userdata = lld_u64_to_void_ptr(((uint64_t)offset));

  lld_int_poll(dev_id, 0, 0);
  if (!callback_called) {
    fprintf(rl_outstream, "No c/b received\n");
  }
  rc = lld_int_register_cb(
      dev_id, subdev_id, offset, NULL, lld_u64_to_void_ptr(((uint64_t)offset)));
  if (rc < 0) {
    fprintf(rl_outstream,
            "Error: %d : trying to un-register cb on %d:%d:%08x\n",
            rc,
            dev_id,
            subdev_id,
            offset);
    return;
  }
}

/* Test interrupts */
bf_status_t lld_interrupt_test_extended(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;

  lld_inttest_results_clear();
  lld_inttest_result_set(true);
  fprintf(rl_outstream, "Clear interrupt counts and disable all..\n");
  status = lld_int_disable_all(dev_id, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts disable failed \n");
  }
  status = lld_int_poll(dev_id, 0, 1);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts poll failed \n");
  }
  status = lld_clear_ints(dev_id, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts clear failed \n");
  }
  status = lld_int_poll(dev_id, 0, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts poll failed \n");
  }
  fprintf(rl_outstream, "No interrupts should be firing:\n");
  status = lld_dump_new_ints(dev_id, 0);  // should be none
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts fired even in disable state\n");
    /* OK to proceed */
  }

  fprintf(rl_outstream, "enable all..\n");
  status = lld_enable_all_ints(dev_id, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts enable failed \n");
  }
  /* Some interrupts get fired after enable */
  status = lld_int_poll(dev_id, 0, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts poll failed \n");
  }
  fprintf(rl_outstream, "No interrupts should be firing:\n");
  status = lld_dump_new_ints(dev_id, 0);  // should be none
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts fired without injecting \n");
    /* OK to proceed */
  }

  fprintf(rl_outstream, "inject all..\n");
  status = lld_inject_all_ints(dev_id, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts inject failed \n");
  }
  fprintf(rl_outstream, "polling now...\n");
  status = lld_int_poll(dev_id, 0, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts poll failed \n");
  }
  fprintf(rl_outstream, "All interrupts should have fired. These haven't:\n");
  status = lld_dump_unfired_ints(dev_id, 0);
  if (status != BF_SUCCESS) {
    fprintf(rl_outstream, "ERROR: Interrupts did not fire \n");
    lld_inttest_result_set(false);
    return status;
  }

  return status;
}
#endif /* __KERNEL__ */
