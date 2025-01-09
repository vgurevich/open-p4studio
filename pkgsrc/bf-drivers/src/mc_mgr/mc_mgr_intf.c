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


#include <stdint.h>
#include <string.h>
#include "dvm/bf_drv_intf.h"
#include "lld/bf_dma_if.h"
#include "lld/lld_dr_if.h"
#include "lld/lld_err.h"
#include "lld/lld_sku.h"
#include "mc_mgr/mc_mgr_intf.h"
#include "pipe_mgr/pipe_mgr_mc_intf.h"
#include "lld/lld_efuse.h"

#include "mc_mgr_int.h"
#include "mc_mgr_handle.h"
#include "mc_mgr_bh.h"
#include "mc_mgr.h"
#include "mc_mgr_reg.h"

static void logical_port_to_physical(bf_dev_id_t dev,
                                     bf_dev_port_t l,
                                     bf_dev_port_t *p) {
  bf_dev_pipe_t x = mc_dev_port_to_pipe(dev, l);
  lld_sku_map_pipe_id_to_phy_pipe_id(dev, x, &x);
  *p = mc_make_dev_port(dev, x, mc_dev_port_to_local_port(dev, l));
  *p = lld_sku_map_devport_from_user_to_device(dev, *p);
}
static void physical_port_to_logical(bf_dev_id_t dev,
                                     bf_dev_port_t p,
                                     bf_dev_port_t *l) {
  bf_dev_pipe_t x = mc_dev_port_to_pipe(dev, p);
  lld_sku_map_phy_pipe_id_to_pipe_id(dev, x, &x);
  *l = mc_make_dev_port(dev, x, mc_dev_port_to_local_port(dev, p));
  *l = lld_sku_map_devport_from_device_to_user(dev, *l);
}

static bf_status_t logical_port_map_to_physical(bf_dev_id_t dev,
                                                bf_mc_port_map_t src,
                                                bf_mc_port_map_t *dst) {
  size_t x;
  uint32_t y;

  /* Get the mapping from logical pipe (array index) to physical pipe (array
   * element value). */
  bf_dev_pipe_t mapping[BF_PIPE_COUNT] = {0};
  uint32_t num_subdev = 0;
  lld_sku_get_num_subdev(dev, &num_subdev, NULL);
  for (y = 0; y < mc_mgr_ctx_num_max_pipes(dev); ++y) {
    mapping[y] = y;
    lld_sku_map_pipe_id_to_phy_pipe_id(dev, y, &mapping[y]);
  }
  uint32_t active_pipe_cnt = 0;
  lld_sku_get_num_active_pipes(dev, &active_pipe_cnt);
#if defined(EMU_2DIE_USING_SW_2DEV)
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    active_pipe_cnt = mc_mgr_ctx_num_max_pipes(dev);
  }
#endif

  /* Initialize the output port map to zeros. */
  BF_MC_PORT_MAP_INIT(*dst);

  /* For each byte in the port map... */
  for (x = 0; x < sizeof(bf_mc_port_map_t); ++x) {
    if ((mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO ||
         mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO2) &&
        (8 * x >= BF_SUBDEV_PORT_COUNT)) {
      /* Tofino and Tofino2 support only one subdevice. */
      break;
    }
    /* For each bit in the byte of the port map. */
    for (y = 0; y < 8; ++y) {
      if ((src[x] >> y) & 1) {
        /* Bit is set so:
         * 1) map the bit index to a dev port
         * 2) get the pipe from the dev port
         * 3) translate the pipe from logical to physical
         * 4) rebuild the dev port using the physical pipe
         * 5) convert the dev port back to a bit index
         * 6) set the bit at that index in the output port map. */
        uint32_t id = 8 * x + y;
        bf_dev_port_t dev_port = BIT_IDX_TO_DEV_PORT((id));
        dev_port = lld_sku_map_devport_from_user_to_device(dev, dev_port);
        bf_dev_port_t local_port = mc_dev_port_to_local_port(dev, dev_port);
        bf_dev_pipe_t log_pipe = mc_dev_port_to_pipe(dev, dev_port);
        if (log_pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
          continue;
        }
        if (log_pipe >= active_pipe_cnt) {
          LOG_ERROR("Requested port %d but dev %d supports %d pipes",
                    dev_port,
                    dev,
                    active_pipe_cnt);
          return BF_INVALID_ARG;
        }
/* GCC 4.9.2 gives a false positive warning for "array subscript is above array
 * bounds".  Likely related to this bug:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56273
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
        BF_MC_PORT_MAP_SET(
            *dst, mc_make_dev_port(dev, mapping[log_pipe], local_port));
#pragma GCC diagnostic pop
      }
    }
  }
  return BF_SUCCESS;
}

static void physical_port_map_to_logical(bf_dev_id_t dev,
                                         uint64_t *ports,
                                         bf_mc_port_map_t *dst) {
  if (!dst || !ports) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return;
  }

  MC_MGR_MEMSET(dst, 0, sizeof(bf_mc_port_map_t));
  // iterate over the bits not the bytes
  for (size_t i = 0; i < mc_mgr_ctx_num_max_pipes(dev); ++i) {
    bf_bitset_t bs_p;
    bf_bs_init(&bs_p,
               BF_PIPE_PORT_COUNT,
               ports + (i * BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)));
    for (size_t j = 0; j < BF_PIPE_PORT_COUNT; ++j) {
      if (!bf_bs_get(&bs_p, j)) continue;
      size_t bit_idx = j + i * BF_PIPE_PORT_COUNT;
      bf_dev_pipe_t log_pipe = 0;
      bf_dev_pipe_t phy_pipe = BIT_IDX_TO_PIPE(bit_idx);
      uint32_t loc_port = BIT_IDX_TO_LOCAL_PORT(bit_idx);
      lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
      bf_dev_port_t log_port = MAKE_DEV_PORT(log_pipe, loc_port);
      log_port = lld_sku_map_devport_from_device_to_user(dev, log_port);
      BF_MC_PORT_MAP_SET((*dst), log_port);
    }
  }
}

bf_status_t bf_mc_init() { return mc_mgr_init(); }

void mc_lag_get_size(bf_dev_id_t dev, uint32_t *count) {
  if (mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* For Tofino, one of the LAG identifiers is reserved
       reducing the size of LAG table available to the user */
    *count = BF_LAG_COUNT - 1;
  } else {
    *count = BF_LAG_COUNT;
  }
  return;
}

bf_status_t bf_mc_create_session(bf_mc_session_hdl_t *shdl) {
  bf_status_t ret = BF_SUCCESS;
  if (!shdl) return BF_INVALID_ARG;

  if (!mc_mgr_ready()) {
    /* If MC mgr is not yet initialized, initialize it */
    ret = mc_mgr_init();
    if (BF_SUCCESS != ret) {
      LOG_ERROR("Failed to initialize MC mgr at %s:%d", __func__, __LINE__);
      *shdl = 0;
      return ret;
    }
  }

  mc_mgr_one_at_a_time_begin();

  ret = mc_mgr_create_session(shdl);

  if (BF_SUCCESS != ret) {
    LOG_ERROR("Failed to create session at %s:%d", __func__, __LINE__);
    *shdl = 0;
  }

  mc_mgr_one_at_a_time_end();

  return ret;
}

bf_status_t bf_mc_destroy_session(bf_mc_session_hdl_t hdl) {
  bf_status_t ret = BF_INVALID_ARG;

  if (!mc_mgr_ready()) return BF_NOT_READY;

  mc_mgr_one_at_a_time_begin();

  int sid = -1;
  if (-1 != (sid = mc_mgr_validate_session(hdl, __func__, __LINE__))) {
    if (mc_mgr_in_batch(sid)) {
      ret = mc_mgr_end_batch(sid);
      if (BF_SUCCESS != ret) {
        LOG_ERROR("end-batch by session %d fails, status %d", hdl, ret);
      }
    }
    struct mc_session_ctx *ctx = mc_mgr_ctx_session_state(sid);
    if (ctx) {
      MC_MGR_MEMSET(ctx, 0, sizeof(struct mc_session_ctx));
    }
    ret = BF_SUCCESS;
  } else {
    LOG_ERROR(
        "Session handle (%#x) not valid at %s:%d", hdl, __func__, __LINE__);
  }

  mc_mgr_one_at_a_time_end();

  return ret;
}

bf_status_t bf_mc_begin_batch(bf_mc_session_hdl_t shdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (mc_mgr_in_batch(sid)) return BF_ALREADY_EXISTS;

  mc_mgr_begin_batch(sid);
  return BF_SUCCESS;
}

bf_status_t bf_mc_flush_batch(bf_mc_session_hdl_t shdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_in_batch(sid)) return BF_INVALID_ARG;

  return mc_mgr_flush_batch(sid);
}

bf_status_t bf_mc_end_batch(bf_mc_session_hdl_t shdl, bool hwSynchronous) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_in_batch(sid)) return BF_INVALID_ARG;

  bf_status_t sts = mc_mgr_end_batch(sid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("end-batch by session %d fails, status %d", shdl, sts);
  }

  if (hwSynchronous) {
    bf_mc_complete_operations(shdl);
  }
  return sts;
}

bf_status_t bf_mc_complete_operations(bf_mc_session_hdl_t shdl) {
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* If the session is in a batch the complete is a nop. */
  if (mc_mgr_in_batch(sid)) return BF_SUCCESS;

  int d;
  for (d = 0; d < MC_MGR_NUM_DEVICES; ++d) {
    if (!mc_mgr_dev_present(d)) continue;
    mc_mgr_drv_cmplt_operations(sid, d);
  }
  return BF_SUCCESS;
}

bf_status_t bf_mc_mgrp_create(bf_mc_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_mc_grp_id_t grp,
                              bf_mc_mgrp_hdl_t *hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  int sid = -1;


  if (grp == 0 && (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3))
    return BF_INVALID_ARG;

  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!hdl) return BF_INVALID_ARG;
  *hdl = 0;

  mc_mgr_one_at_a_time_begin();

  /* If the bit was already set in the bit map then this group has already
   * been taken. */
  if (mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "Session %#x's attempt to reserve group %#x on device %u "
        "fails (group already taken) at %s:%d",
        shdl,
        grp,
        dev,
        __func__,
        __LINE__);
    sts = BF_ALREADY_EXISTS;
  } else {
    struct mc_mgr_grp_info *grp_info = MC_MGR_CALLOC(1, sizeof(*grp_info));
    if (!grp_info) {
      LOG_ERROR(
          "Session %#x's failed to alloc grp-info for grp %#x on device %u "
          "%s:%d",
          shdl,
          grp,
          dev,
          __func__,
          __LINE__);
      mc_mgr_one_at_a_time_end();
      return BF_NO_SYS_RESOURCES;
    }
    sts = mc_mgr_mgrp_create(dev, grp);
    if (BF_SUCCESS != sts) {
      MC_MGR_FREE(grp_info);
    } else {
      *hdl = mc_mgr_encode_mgrp_hdl(grp);
      grp_info->h = *hdl;
      grp_info->dev = dev;
      grp_info->grp_id = grp;
      bf_map_init(&grp_info->node_mbrs);

      /* Add the group-info to the group map. */
      bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
      bf_status_t s = bf_map_add(grpinfo_map, grp, grp_info);
      MC_MGR_DBGCHK(BF_MAP_OK == s);

      /* Mark the MGID as taken. */
      mc_mgr_set_mgid_map_bit(dev, grp, 1);
    }
  }

  mc_mgr_one_at_a_time_end();

  LOG_TRACE(
      "Reserved grp %#x for session %#x on device %u with handle %#x (status "
      "%s)",
      grp,
      shdl,
      dev,
      *hdl,
      bf_err_str(sts));
  return sts;
}
bf_status_t bf_mc_mgrp_destroy(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_mgrp_hdl_t grp_hdl) {
  struct mc_mgr_grp_info *mgrp_info = NULL;
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;

  if (!mc_mgr_ready()) return BF_NOT_READY;

  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(grp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  bf_map_sts_t s = bf_map_get(grpinfo_map, grp, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_ERROR("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
              grp,
              dev,
              s);
    return BF_OBJECT_NOT_FOUND;
  }

  mc_mgr_one_at_a_time_begin();

  /* If the bit was already clear in the bit map then this group has already
   * been freed. if not then mark the MGID as free first such that no other
   * thread can use the group while cleaning up the state. */
  if (mc_mgr_get_mgid_map_bit(dev, grp)) {
    mc_mgr_set_mgid_map_bit(dev, grp, 0);

    MC_MGR_DBGCHK(mgrp_info->grp_id == grp);
    MC_MGR_DBGCHK(mgrp_info->h == grp_hdl);

    sts = mc_mgr_mgrp_destroy(sid, dev, grp);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Session %#x's attempt to free group %#x on device %u fails; cannot "
          "remove node/ecmp group associations",
          shdl,
          grp,
          dev);
      mc_mgr_one_at_a_time_end();
      return sts;
    }

    bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
    MC_MGR_DBGCHK(!bf_map_count(mbrs_map));
    bf_map_destroy(mbrs_map);

    bf_map_rmv(grpinfo_map, grp);
    MC_MGR_FREE(mgrp_info);

    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      mc_mgr_one_at_a_time_end();
      return sts;
    }

    sts = mc_mgr_update_pvt(sid, dev, grp, false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, grp, false, __func__, __LINE__);
  } else {
    LOG_ERROR(
        "Session %#x's attempt to free group %#x on device %u "
        "fails (group already freed) at %s:%d",
        shdl,
        grp,
        dev,
        __func__,
        __LINE__);
    sts = BF_ALREADY_EXISTS;
  }

  mc_mgr_one_at_a_time_end();

  LOG_TRACE(
      "Released grp %#x by session %#x on device %u with handle %#x (status "
      "%s)",
      grp,
      shdl,
      dev,
      grp_hdl,
      bf_err_str(sts));
  return sts;
}

bf_status_t bf_mc_mgrp_get_attr(bf_mc_session_hdl_t shdl,
                                bf_dev_id_t dev,
                                bf_mc_mgrp_hdl_t mgrp_hdl,
                                bf_mc_grp_id_t *grp) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t bf_mc_mgrp_get_first(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_mc_mgrp_hdl_t *mgrp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  for (uint32_t i = 0; i < BF_MGID_COUNT; i++) {
    bf_mc_grp_id_t j = i & 0xFFFF;
    if (mc_mgr_get_mgid_map_bit(dev, j)) {
      *mgrp_hdl = mc_mgr_encode_mgrp_hdl(j);
      mc_mgr_one_at_a_time_end();
      return sts;
    }
  }

  mc_mgr_one_at_a_time_end();

  return BF_OBJECT_NOT_FOUND;
}

bf_status_t bf_mc_mgrp_get_count(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  *count = 0;
  for (int i = 0; i < MC_MGR_MGID_MAP_SIZE; i++) {
    *count += __builtin_popcountll(mc_mgr_ctx_mgid_blk(dev, i));
  }

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_lag_get_size(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_lag_get_size(dev, count);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_next_i(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_mgrp_hdl_t mgrp_hdl,
                                  uint32_t i,
                                  bf_mc_mgrp_hdl_t *next_mgrp_hdls) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  uint32_t cur = 0;
  for (uint32_t j = grp + 1; j < BF_MGID_COUNT; j++) {
    bf_mc_grp_id_t k = j & 0xFFFF;
    if (cur == i) {
      mc_mgr_one_at_a_time_end();
      return BF_SUCCESS;
    }
    if (mc_mgr_get_mgid_map_bit(dev, k)) {
      next_mgrp_hdls[cur] = mc_mgr_encode_mgrp_hdl(k);
      cur++;
    }
  }

  mc_mgr_one_at_a_time_end();

  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  for (; cur < i; cur++) {
    next_mgrp_hdls[cur] = -1;
  }

  return BF_SUCCESS;
}

bf_status_t bf_mc_mgrp_get_first_node_mbr(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_mc_mgrp_hdl_t mgrp_hdl,
                                          bf_mc_node_hdl_t *node_hdl,
                                          bool *node_l1_xid_valid,
                                          bf_mc_l1_xid_t *node_l1_xid) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get first node member of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_first_node_mbr(
      sid, dev, grp, node_hdl, node_l1_xid_valid, node_l1_xid);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_node_mbr_count(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_mc_mgrp_hdl_t mgrp_hdl,
                                          uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get node members count of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_node_mbr_count(sid, dev, grp, count);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_next_i_node_mbr(bf_mc_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_node_hdl_t node_hdl,
                                           uint32_t i,
                                           bf_mc_node_hdl_t *next_node_hdls,
                                           bool *next_node_l1_xids_valid,
                                           bf_mc_l1_xid_t *next_node_l1_xids) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get node members count of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_next_i_node_mbr(sid,
                                        dev,
                                        grp,
                                        node_hdl,
                                        i,
                                        next_node_hdls,
                                        next_node_l1_xids_valid,
                                        next_node_l1_xids);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_first_ecmp_mbr(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_mc_mgrp_hdl_t mgrp_hdl,
                                          bf_mc_ecmp_hdl_t *ecmp_hdl,
                                          bool *ecmp_l1_xid_valid,
                                          bf_mc_l1_xid_t *ecmp_l1_xid) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get first ecmp member of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_first_ecmp_mbr(
      sid, dev, grp, ecmp_hdl, ecmp_l1_xid_valid, ecmp_l1_xid);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_ecmp_mbr_count(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_mc_mgrp_hdl_t mgrp_hdl,
                                          uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get ecmp members count of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_ecmp_mbr_count(sid, dev, grp, count);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_mgrp_get_next_i_ecmp_mbr(bf_mc_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_mc_mgrp_hdl_t mgrp_hdl,
                                           bf_mc_ecmp_hdl_t ecmp_hdl,
                                           uint32_t i,
                                           bf_mc_ecmp_hdl_t *next_ecmp_hdls,
                                           bool *next_ecmp_l1_xids_valid,
                                           bf_mc_l1_xid_t *next_ecmp_l1_xids) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session handle. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode and validate the mgrp handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_get_mgid_map_bit(dev, grp)) {
    LOG_ERROR(
        "%s : %d Session %#x's attempt to get ecmp members count of mgrp %#x "
        "on device %u fails as mc group doesn't exist",
        __func__,
        __LINE__,
        shdl,
        grp,
        dev);

    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  sts = mc_mgr_mgrp_get_next_i_ecmp_mbr(sid,
                                        dev,
                                        grp,
                                        ecmp_hdl,
                                        i,
                                        next_ecmp_hdls,
                                        next_ecmp_l1_xids_valid,
                                        next_ecmp_l1_xids);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_ecmp_create(bf_mc_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_mc_ecmp_hdl_t *ecmp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!ecmp_hdl) {
    return BF_INVALID_ARG;
  }
  *ecmp_hdl = 0;

  mc_mgr_one_at_a_time_begin();

  /* Allocate the handle. */
  if (BF_SUCCESS != mc_mgr_encode_ecmp_hdl(dev, ecmp_hdl)) {
    sts = BF_NO_SYS_RESOURCES;
    goto done;
  }
  /* Allocate an object for the group. */
  sts = mc_mgr_ecmp_alloc(sid, dev, *ecmp_hdl);
  if (sts == BF_SUCCESS) {
    sts = mc_mgr_drv_wrl_send(sid, true);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  }
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to create ECMP group, %s (%d)", bf_err_str(sts), sts);
  } else {
    LOG_TRACE(
        "Session %#x created ECMP group %#x on dev %d", shdl, *ecmp_hdl, dev);
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

static bf_status_t bf_mc_ecmp_destroy_internal(bf_mc_session_hdl_t shdl,
                                               bf_dev_id_t dev,
                                               bf_mc_ecmp_hdl_t ehdl,
                                               bool free_mgid_if_associated) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ehdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp_grp = mc_mgr_lookup_ecmp(dev, ehdl, __func__, __LINE__);
  if (!ecmp_grp) {
    LOG_ERROR("Session %#x cannot destroy unknown ECMP Group %#x", shdl, ehdl);
    sts = BF_INVALID_ARG;
    goto done;
  }
  /* make sure no mgid are associated with this ecmp_grp */
  if (!free_mgid_if_associated && ecmp_grp->refs &&
      mgid_associated(ecmp_grp->refs)) {
    LOG_ERROR(
        "Session %#x cannot destroy ECMP Group %#x with mgid (%#x) associated",
        shdl,
        ehdl,
        ecmp_grp->refs->mgid);
    sts = BF_IN_USE;
    goto done;
  }
  sts = mc_mgr_ecmp_free(sid, dev, ecmp_grp);
  if (sts == BF_SUCCESS) {
    sts = mc_mgr_drv_wrl_send(sid, true);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  }
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to destroy ECMP group 0x%x on dev %d, %s (%d)",
              ehdl,
              dev,
              bf_err_str(sts),
              sts);
  } else {
    LOG_TRACE(
        "Session %#x destroyed ECMP group %#x on dev %d", shdl, ehdl, dev);
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}
bf_status_t bf_mc_ecmp_destroy(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ehdl) {
  return bf_mc_ecmp_destroy_internal(shdl, dev, ehdl, true);
}
bf_status_t bf_mc_ecmp_destroy_checked(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ehdl) {
  return bf_mc_ecmp_destroy_internal(shdl, dev, ehdl, false);
}

bf_status_t bf_mc_ecmp_get_first(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_mc_ecmp_hdl_t *ecmp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  bf_map_t *db = mc_mgr_ctx_db_ecmp(dev);

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = NULL;
  long unsigned int key = 0;
  sts = bf_map_get_first(db, &key, (void **)&g);
  *ecmp_hdl = key;

  mc_mgr_one_at_a_time_end();

  if (sts == BF_MAP_OK) {
    sts = BF_SUCCESS;
  } else {
    sts = BF_OBJECT_NOT_FOUND;
  }

  return sts;
}

bf_status_t bf_mc_ecmp_get_count(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_ecmp(dev);
  *count = bf_map_count(db);

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_ecmp_get_next_i(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_ecmp_hdl_t ecmp_hdl,
                                  uint32_t i,
                                  bf_mc_ecmp_hdl_t *next_ecmp_hdls) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_ecmp(dev);
  mc_ecmp_grp_t *g = NULL;
  long unsigned int key = ecmp_hdl;
  for (uint32_t cur = 0; cur < i; cur++) {
    bf_map_sts_t x = bf_map_get_next(db, &key, (void **)&g);
    if (x == BF_MAP_OK) {
      next_ecmp_hdls[cur] = key;
    } else if (cur == 0) {
      sts = BF_OBJECT_NOT_FOUND;
      break;
    } else {
      next_ecmp_hdls[cur] = -1;
    }
  }

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_ecmp_mbr_add(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the L1 node. */
  mc_l1_node_t *l1_node =
      mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);

  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp_grp =
      mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);

  if (!l1_node || !ecmp_grp) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  sts = mc_mgr_ecmp_mbr_add(sid, dev, ecmp_grp, l1_node);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Session %#x failed to add node %#x to ECMP group %#x, %s (%d)",
              shdl,
              node_hdl,
              ecmp_hdl,
              bf_err_str(sts),
              sts);
  } else {
    LOG_TRACE("ECMP group %#x now has member %#x", ecmp_hdl, node_hdl);
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}
bf_status_t bf_mc_ecmp_mbr_rem(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the L1 node. */
  mc_l1_node_t *l1_node =
      mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);

  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp_grp =
      mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);

  if (!l1_node || !ecmp_grp) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  sts = mc_mgr_ecmp_mbr_rem(sid, dev, ecmp_grp, l1_node);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}
bf_status_t bf_mc_ecmp_mbr_mod(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_ecmp_hdl_t ecmp_hdl,
                               bf_mc_node_hdl_t *node_hdls,
                               uint32_t size) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!node_hdls || size == 0 || size > MC_ECMP_GROUP_MAX_MBR) {
    return BF_INVALID_ARG;
  }
  for (uint32_t i = 0; i < size; i++) {
    bf_mc_node_hdl_t node_hdl = node_hdls[i];
    if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
      return BF_INVALID_ARG;
    }
  }
  mc_mgr_one_at_a_time_begin();
  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp_grp =
      mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);

  if (!ecmp_grp) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  mc_l1_node_t *l1_nodes[MC_ECMP_GROUP_MAX_MBR] = {0};
  for (uint32_t i = 0; i < size; i++) {
    bf_mc_node_hdl_t node_hdl = node_hdls[i];
    /* Look up the L1 node. */
    mc_l1_node_t *l1_node =
        mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);
    if (!l1_node) {
      sts = BF_INVALID_ARG;
      goto done;
    }
    l1_nodes[i] = l1_node;
  }
  /* Now we have all L1 valid nodes, valid ecmp_grp */
  sts = mc_mgr_ecmp_mbr_mod(sid, dev, ecmp_grp, l1_nodes, size);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_ecmp_get_first_mbr(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_mc_ecmp_hdl_t ecmp_hdl,
                                     bf_mc_node_hdl_t *node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  for (uint32_t i = 0; i < sizeof(g->mbrs) / sizeof(g->mbrs[0]); ++i) {
    if (g->mbrs[i]) {
      *node_hdl = g->mbrs[i]->handle;
      mc_mgr_one_at_a_time_end();
      return sts;
    }
  }
  *node_hdl = 0;

  mc_mgr_one_at_a_time_end();

  return BF_OBJECT_NOT_FOUND;
}

bf_status_t bf_mc_ecmp_get_mbr_count(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_mc_ecmp_hdl_t ecmp_hdl,
                                     uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  *count = __builtin_popcount(g->valid_map);
  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_ecmp_get_next_i_mbr(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bf_mc_node_hdl_t node_hdl,
                                      uint32_t i,
                                      bf_mc_node_hdl_t *next_node_hdls) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  bool search_mode = true;
  uint32_t cur = 0;
  for (uint32_t j = 0; j < sizeof(g->mbrs) / sizeof(g->mbrs[0]); j++) {
    if (search_mode) {
      if (g->mbrs[j] && g->mbrs[j]->handle == node_hdl) {
        search_mode = false;
      }
      continue;
    }
    if (cur == i) {
      break;
    }
    if (g->mbrs[j]) {
      next_node_hdls[cur] = g->mbrs[j]->handle;
      cur++;
    }
  }

  mc_mgr_one_at_a_time_end();

  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  for (; cur < i; cur++) {
    next_node_hdls[cur] = -1;
  }

  return BF_SUCCESS;
}

bf_status_t bf_mc_ecmp_get_mbr_from_hash(bf_mc_session_hdl_t shdl,
                                         bf_dev_id_t dev,
                                         bf_mc_mgrp_hdl_t mgrp_hdl,
                                         bf_mc_ecmp_hdl_t ecmp_hdl,
                                         uint16_t level1_mcast_hash,
                                         bf_mc_l1_xid_t level1_exclusion_id,
                                         bf_mc_node_hdl_t *node_hdl,
                                         bool *is_pruned) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  *node_hdl = 0;
  *is_pruned = false;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  bf_mc_grp_id_t mgid;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &mgid, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Check for pruning */
  mc_l1_node_t *n = g->refs;
  for (; n; n = n->ecmp_next) {
    if (n->mgid == mgid) {
      break;
    }
  }
  if (!n) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (n->xid_valid && n->xid == level1_exclusion_id) {
    *is_pruned = true;
  }

  uint32_t live_cnt = __builtin_popcount(g->valid_map);
  if (!live_cnt) goto done;

  uint32_t idx1 = level1_mcast_hash % 32;
  uint32_t idx2 = level1_mcast_hash % live_cnt;

  if (g->mbrs[idx1]) {
    *node_hdl = g->mbrs[idx1]->handle;
  } else {
    uint32_t live_idx = 0;
    for (uint32_t i = 0; i < sizeof(g->mbrs) / sizeof(g->mbrs[0]); ++i) {
      if (g->mbrs[i]) {
        if (live_idx == idx2) {
          *node_hdl = g->mbrs[i]->handle;
          break;
        }
        live_idx++;
      }
    }
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_ecmp_get_first_assoc(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ecmp_hdl,
                                       bf_mc_mgrp_hdl_t *mgrp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  if (g->refs) {
    MC_MGR_DBGCHK(g->refs->mgid != -1);
    *mgrp_hdl = mc_mgr_encode_mgrp_hdl(g->refs->mgid);
  } else {
    mc_mgr_one_at_a_time_end();
    return BF_OBJECT_NOT_FOUND;
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_ecmp_get_assoc_count(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_ecmp_hdl_t ecmp_hdl,
                                       uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  *count = 0;
  mc_l1_node_t *n;
  for (n = g->refs; n; n = n->ecmp_next) {
    (*count)++;
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_ecmp_get_next_i_assoc(bf_mc_session_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        bf_mc_ecmp_hdl_t ecmp_hdl,
                                        bf_mc_mgrp_hdl_t mgrp_hdl,
                                        uint32_t i,
                                        bf_mc_mgrp_hdl_t *next_mgrp_hdls) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  mc_l1_node_t *n;
  bool search_mode = true;
  uint32_t cur = 0;
  for (n = g->refs; n; n = n->ecmp_next) {
    if (search_mode) {
      if (n->mgid == grp) {
        search_mode = false;
      }
      continue;
    }
    if (cur == i) {
      break;
    }
    next_mgrp_hdls[cur] = mc_mgr_encode_mgrp_hdl(n->mgid);
    cur++;
  }

  mc_mgr_one_at_a_time_end();

  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  for (; cur < i; cur++) {
    next_mgrp_hdls[cur] = -1;
  }

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_create(bf_mc_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_mc_rid_t rid,
                              bf_mc_port_map_t port_map,
                              bf_mc_lag_map_t lag_map,
                              bf_mc_node_hdl_t *node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!node_hdl) return BF_INVALID_ARG;
  *node_hdl = 0;

  mc_mgr_one_at_a_time_begin();

  /* Update the node with the membership information after converting it from
   * logical to physical space. */
  bf_mc_port_map_t physical_port_map;
  sts = logical_port_map_to_physical(dev, port_map, &physical_port_map);
  if (sts != BF_SUCCESS) {
    mc_mgr_one_at_a_time_end();
    return sts;
  }

  /* Allocate the object for the node. */
  mc_l1_node_t *node = mc_mgr_node_alloc(dev, rid);
  if (!node) {
    *node_hdl = 0;
    sts = BF_NO_SYS_RESOURCES;
    goto done;
  }
  *node_hdl = node->handle;

  sts = mc_mgr_set_node_membership(sid, node, physical_port_map, lag_map);
  if (sts == BF_SUCCESS && mgid_associated(node)) {
    /* Push the write list to hardware. */
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (sts == BF_SUCCESS) {
      /* Program the PVT if it has changed. */
      sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
      sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
    }
  }

  /* Set the lag to node map for all the lags used by the node. */
  if (sts == BF_SUCCESS) {
    sts |= mc_mgr_node_set_lags_map(node);
  }
done:
  mc_mgr_one_at_a_time_end();
  LOG_TRACE(
      "L1 Create by session %#x with RID %#x allocated handle %#x (status %s)",
      shdl,
      rid,
      *node_hdl,
      bf_err_str(sts));
  size_t i;
  size_t port_array_size = BF_MC_PORT_ARRAY_SIZE;
  if ((mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO ||
       mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO2)) {
    /* Allow only one subdevice for Tofino and Tofino2. */
    port_array_size = (BF_SUBDEV_PORT_COUNT + 7) / 8;
  }
  /* Two characters per a byte in the port map plus a terminating character. */
  char pm[BF_MC_PORT_ARRAY_SIZE * 2 + 1] = {0};
  char lm[BF_MC_LAG_ARRAY_SIZE * 2 + 1] = {0};
  for (i = 0; i < port_array_size; ++i)
    sprintf(&pm[2 * i], "%02x", port_map[(port_array_size - 1) - i]);
  for (i = 0; i < BF_MC_LAG_ARRAY_SIZE; ++i)
    sprintf(&lm[2 * i], "%02x", lag_map[(BF_MC_LAG_ARRAY_SIZE - 1) - i]);
  LOG_TRACE(
      "Session %#x created L2 on L1 %#x PortMap %s LagMap %s, status %s (%d)",
      shdl,
      *node_hdl,
      pm,
      lm,
      bf_err_str(sts),
      sts);
  return sts;
}

bf_status_t bf_mc_node_get_attr(bf_mc_session_hdl_t shdl,
                                bf_dev_id_t dev,
                                bf_mc_node_hdl_t node_hdl,
                                bf_mc_rid_t *rid,
                                bf_mc_port_map_t port_map,
                                bf_mc_lag_map_t lag_map) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);
  mc_l1_node_t *n = NULL;
  bf_mc_node_hdl_t key = node_hdl;
  if (BF_MAP_OK != bf_map_get(db, key, (void **)&n)) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  // get lags from l2 chain
  bf_bitset_t bs_l;
  bf_bs_init(&bs_l, BF_LAG_COUNT, n->l2_chain.lags);
  for (size_t i = 0; i < BF_MC_LAG_ARRAY_SIZE; ++i) {
    uint8_t byte = 0;
    for (size_t j = 0; j < 8; j++) {
      byte += bf_bs_get(&bs_l, 8 * i + j) << j;
    }
    lag_map[i] = byte;
  }
  // get ports from l2 chain
  bf_mc_port_map_t pm;
  physical_port_map_to_logical(dev, (uint64_t *)n->l2_chain.ports, &pm);
  memcpy(port_map, &pm, sizeof(bf_mc_port_map_t));

  *rid = n->rid;

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_destroy(bf_mc_session_hdl_t shdl,
                               bf_dev_id_t dev,
                               bf_mc_node_hdl_t node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode the node handle. */
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the L1 node. */
  mc_l1_node_t *l1_node =
      mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);
  if (!l1_node) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Reset the lag to node map for all the lags used by the node. */
  mc_mgr_node_reset_lags_map(l1_node);

  /* If it is associated, return an error. */
  if (-1 != l1_node->mgid) {
    sts = mc_mgr_l1_dissociate(sid, l1_node, l1_node->mgid);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Session %#x failed to remove node %#x from MGID %#x on dev %d",
                shdl,
                node_hdl,
                l1_node->mgid,
                dev);
      mc_mgr_node_set_lags_map(l1_node);
      goto done;
    }
  } else if (l1_node->ecmp_grp) {
    sts = mc_mgr_ecmp_mbr_rem(sid, dev, l1_node->ecmp_grp, l1_node);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Session %#x failed to remove node %#x from ECMP %#x on dev %d",
                shdl,
                node_hdl,
                l1_node->ecmp_grp->handle,
                dev);
      mc_mgr_node_set_lags_map(l1_node);
      goto done;
    }
  }

  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (sts != BF_SUCCESS) {
      MC_MGR_DBGCHK(0);
      goto done;
    }
  }

  /* Free the node. */
  sts = mc_mgr_node_free(dev, node_hdl);
  if (BF_SUCCESS == sts) {
    /* Free the handle. */
    sts = mc_mgr_delete_l1_node_hdl(dev, node_hdl);
  }

done:
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS != sts) {
    LOG_ERROR("Node destroy by session %#x for node %#x failed (status %s)",
              shdl,
              node_hdl,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  } else {
    LOG_TRACE("Node destroy by session %#x for node %#x (status %s)",
              shdl,
              node_hdl,
              bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_node_get_first(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_mc_node_hdl_t *node_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);

  mc_l1_node_t *n = NULL;
  long unsigned int key = 0;
  sts = bf_map_get_first(db, &key, (void **)&n);
  mc_mgr_one_at_a_time_end();

  if (sts == BF_MAP_OK) {
    *node_hdl = key;
    sts = BF_SUCCESS;
  } else {
    sts = BF_OBJECT_NOT_FOUND;
  }

  return sts;
}

bf_status_t bf_mc_node_get_count(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);
  long unsigned int key;
  mc_l1_node_t *n = NULL;
  bf_status_t sts;
  *count = 0;
  for (sts = bf_map_get_first(db, &key, (void **)&n); sts == BF_MAP_OK;
       sts = bf_map_get_next(db, &key, (void **)&n)) {
    (*count)++;
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_get_next_i(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_node_hdl_t node_hdl,
                                  uint32_t i,
                                  bf_mc_node_hdl_t *next_node_hdls) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (i == 0) {
    return BF_SUCCESS;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);
  mc_l1_node_t *n;
  long unsigned int key = node_hdl;
  uint32_t cur = 0;
  bf_map_sts_t x;
  for (x = bf_map_get_next(db, &key, (void **)&n); x == BF_MAP_OK;
       x = bf_map_get_next(db, &key, (void **)&n)) {
    if (cur == i) {
      break;
    }
    next_node_hdls[cur] = key;
    cur++;
  }

  mc_mgr_one_at_a_time_end();

  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  for (; cur < i; cur++) {
    next_node_hdls[cur] = -1;
  }

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_get_association(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_node_hdl_t node_hdl,
                                       bool *is_associated,
                                       bf_mc_mgrp_hdl_t *mgrp_hdl,
                                       bool *level1_exclusion_id_valid,
                                       bf_mc_l1_xid_t *level1_exclusion_id) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);
  mc_l1_node_t *n = NULL;
  bf_mc_node_hdl_t key = node_hdl;
  if (BF_MAP_OK != bf_map_get(db, key, (void **)&n)) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  if (n->mgid == -1) {
    if (is_associated) {
      *is_associated = false;
    }
    mc_mgr_one_at_a_time_end();
    return BF_SUCCESS;
  }

  if (is_associated) {
    *is_associated = true;
  }
  if (level1_exclusion_id_valid) {
    *level1_exclusion_id_valid = n->xid_valid;
  }
  if (level1_exclusion_id) {
    *level1_exclusion_id = n->xid;
  }
  if (mgrp_hdl) {
    *mgrp_hdl = mc_mgr_encode_mgrp_hdl(n->mgid);
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_is_mbr(bf_mc_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_mc_node_hdl_t node_hdl,
                              bool *is_ecmp_mbr,
                              bf_mc_ecmp_hdl_t *ecmp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_map_t *db = mc_mgr_ctx_db_l1(dev);
  mc_l1_node_t *n = NULL;
  bf_mc_node_hdl_t key = node_hdl;
  if (BF_MAP_OK != bf_map_get(db, key, (void **)&n)) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  if (n->mgid != -1 || !n->ecmp_grp) {
    if (is_ecmp_mbr) {
      *is_ecmp_mbr = false;
    }
    mc_mgr_one_at_a_time_end();
    return BF_SUCCESS;
  }

  if (ecmp_hdl) {
    *ecmp_hdl = n->ecmp_grp->handle;
  }
  if (is_ecmp_mbr) {
    *is_ecmp_mbr = true;
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_update(bf_mc_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_mc_node_hdl_t nhdl,
                              bf_mc_port_map_t port_map,
                              bf_mc_lag_map_t lag_map) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Decode the node handle. */
  if (!mc_mgr_decode_l1_node_hdl(nhdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Lookup the node based on handle. */
  mc_l1_node_t *l1_node = mc_mgr_lookup_l1_node(dev, nhdl, __func__, __LINE__);
  if (!l1_node) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Update the node with the membership information after converting it from
   * logical to physical space. */
  bf_mc_port_map_t physical_port_map;
  sts =
      logical_port_map_to_physical(l1_node->dev, port_map, &physical_port_map);
  if (sts != BF_SUCCESS) goto done;

  /* Remove the node from its lags map.*/
  mc_mgr_node_reset_lags_map(l1_node);

  /* Update the node with the SW first, if it has been associated with
   * mgid/ecmp_grp, then we update the hardware as well */
  sts = mc_mgr_set_node_membership(sid, l1_node, physical_port_map, lag_map);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "Failed to update the node with provided ports/lags %s, %d (status %s)",
        __func__,
        __LINE__,
        bf_err_str(sts));
    /* As node mbrship failed, set the node exising lags map again. */
    mc_mgr_node_set_lags_map(l1_node);
    goto done;
  }

  if (mgid_associated(l1_node) || ecmp_associated(l1_node)) {
    /* Push the write list to hardware. */
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to update the node when updating hardware %s, %d (status %s)",
          __func__,
          __LINE__,
          bf_err_str(sts));
      goto done;
    }
    if (mgid_associated(l1_node)) {
      /* Program the PVT if it has changed. */
      sts =
          mc_mgr_update_pvt(sid, dev, l1_node->mgid, false, __func__, __LINE__);
      sts |=
          mc_mgr_update_tvt(sid, dev, l1_node->mgid, false, __func__, __LINE__);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Failed to update the node mgid/pvt %s, %d", __func__, __LINE__);
        goto done;
      }
    }
  } else {
    /* "IGNORE HW UPDATE for the node without mgid/ecmp associated */
  }

  if (sts == BF_SUCCESS) {
    mc_mgr_node_set_lags_map(l1_node);
  }
done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_associate_node(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_mc_mgrp_hdl_t ghdl,
                                 bf_mc_node_hdl_t nhdl,
                                 bool level1_exclusion_id_valid,
                                 bf_mc_l1_xid_t level1_exclusion_id) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode the group handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(ghdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Decode the node handle. */
  mc_l1_node_t *node = NULL;
  if (!mc_mgr_decode_l1_node_hdl(nhdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Lookup the node based on handle. */
  node = mc_mgr_lookup_l1_node(dev, nhdl, __func__, __LINE__);
  if (!node) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Do the association. */
  sts = mc_mgr_l1_associate(
      sid, node, grp, dev, level1_exclusion_id, level1_exclusion_id_valid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to associated L1 node (%#x) to group (%#x) with L1 "
        "exclusion id %#x (%s)",
        nhdl,
        ghdl,
        level1_exclusion_id,
        level1_exclusion_id_valid ? "valid" : "invalid");
  } else {
    /* Push the write list to hardware. */
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (sts == BF_SUCCESS) {
      /* Program the PVT if it has changed. */
      sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
      sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
    }
  }

done:
  mc_mgr_one_at_a_time_end();

  if (level1_exclusion_id_valid) {
    LOG_TRACE(
        "L1 Associate by session %#x for node %#x to group %#x with XID %#x "
        "(status %s)",
        shdl,
        nhdl,
        ghdl,
        level1_exclusion_id,
        bf_err_str(sts));
  } else {
    LOG_TRACE(
        "L1 Associate by session %#x for node %#x to group %#x (status %s)",
        shdl,
        nhdl,
        ghdl,
        bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_dissociate_node(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_mgrp_hdl_t ghdl,
                                  bf_mc_node_hdl_t nhdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode the group handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(ghdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Decode the node handle. */
  if (!mc_mgr_decode_l1_node_hdl(nhdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Lookup the node based on handle. */
  mc_l1_node_t *node = NULL;
  node = mc_mgr_lookup_l1_node(dev, nhdl, __func__, __LINE__);
  if (!node) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  sts = mc_mgr_l1_dissociate(sid, node, grp);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to remove node %#x from group %#x, (%d)", nhdl, ghdl, sts);
  } else {
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (sts == BF_SUCCESS) {
      /* Program the PVT if it has changed. */
      sts = mc_mgr_update_pvt(sid, dev, grp, false, __func__, __LINE__);
      sts |= mc_mgr_update_tvt(sid, dev, grp, false, __func__, __LINE__);
    }
  }

done:
  mc_mgr_one_at_a_time_end();

  LOG_TRACE(
      "L1 Dissociate by session %#x for node %#x from group %#x (status %s)",
      shdl,
      nhdl,
      ghdl,
      bf_err_str(sts));
  return sts;
}

bf_status_t bf_mc_associate_ecmp(bf_mc_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_mc_mgrp_hdl_t ghdl,
                                 bf_mc_ecmp_hdl_t ecmp_hdl,
                                 bool level1_exclusion_id_valid,
                                 bf_mc_l1_xid_t level1_exclusion_id) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode the group handle. */
  bf_mc_grp_id_t mgid;
  if (!mc_mgr_decode_mgrp_hdl(ghdl, &mgid, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Decode the ecmp handle. */
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (ecmp) {
    sts = mc_mgr_ecmp_associate(
        sid, dev, mgid, ecmp, level1_exclusion_id, level1_exclusion_id_valid);
    if (sts == BF_SUCCESS) {
      sts = mc_mgr_drv_wrl_send(sid, true);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(0);
        mc_mgr_one_at_a_time_end();
        return sts;
      }
      if (BF_SUCCESS == sts) {
        /* Program the PVT if it has changed. */
        sts = mc_mgr_update_pvt(sid, dev, mgid, false, __func__, __LINE__);
        if (BF_SUCCESS != sts) {
          MC_MGR_DBGCHK(0);
          mc_mgr_one_at_a_time_end();
          return sts;
        }
        sts = mc_mgr_update_tvt(sid, dev, mgid, false, __func__, __LINE__);
        if (BF_SUCCESS != sts) {
          MC_MGR_DBGCHK(0);
          mc_mgr_one_at_a_time_end();
          return sts;
        }
      }
    }
  } else {
    LOG_ERROR(
        "Session %#x attempted to add ECMP %#x to group %#x but "
        "ECMP can't be found",
        shdl,
        ecmp_hdl,
        ghdl);
    sts = BF_INVALID_ARG;
  }

  if (level1_exclusion_id_valid) {
    LOG_TRACE(
        "Session %#x associated ECMP %#x to Multicast Group %#x on dev %d with "
        "Xid 0x%04x, sts %s (%d)",
        shdl,
        ecmp_hdl,
        ghdl,
        dev,
        level1_exclusion_id,
        bf_err_str(sts),
        sts);
  } else {
    LOG_TRACE(
        "Session %#x associated ECMP %#x to Multicast Group %#x on dev %d, sts "
        "%s (%d)",
        shdl,
        ecmp_hdl,
        ghdl,
        dev,
        bf_err_str(sts),
        sts);
  }

  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_ecmp_get_assoc_attr(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_mc_mgrp_hdl_t mgrp_hdl,
                                      bf_mc_ecmp_hdl_t ecmp_hdl,
                                      bool *level1_exclusion_id_valid,
                                      bf_mc_l1_xid_t *level1_exclusion_id) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate inputs. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(mgrp_hdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_ecmp_grp_t *g = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (!g) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  mc_l1_node_t *n;
  bool found = false;
  for (n = g->refs; n; n = n->ecmp_next) {
    if (n->mgid != grp) {
      continue;
    }
    *level1_exclusion_id_valid = n->xid_valid;
    *level1_exclusion_id = n->xid;
    found = true;
  }

  mc_mgr_one_at_a_time_end();

  if (!found) {
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t bf_mc_dissociate_ecmp(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_mgrp_hdl_t ghdl,
                                  bf_mc_ecmp_hdl_t ecmp_hdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Decode the group handle. */
  bf_mc_grp_id_t grp;
  if (!mc_mgr_decode_mgrp_hdl(ghdl, &grp, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Decode the ecmp handle. */
  if (BF_SUCCESS != mc_mgr_decode_ecmp_hdl(ecmp_hdl, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the ECMP group. */
  mc_ecmp_grp_t *ecmp = mc_mgr_lookup_ecmp(dev, ecmp_hdl, __func__, __LINE__);
  if (ecmp) {
    sts = mc_mgr_ecmp_dissociate(sid, dev, grp, ecmp);
  } else {
    LOG_ERROR(
        "Session %#x attempted to rmv ECMP %#x from group %#x but "
        "ECMP can't be found",
        shdl,
        ecmp_hdl,
        ghdl);
    sts = BF_INVALID_ARG;
  }
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      mc_mgr_one_at_a_time_end();
      return sts;
    }
    if (BF_SUCCESS == sts) {
      /* Program the PVT if it has changed. */
      sts = mc_mgr_update_pvt(sid, dev, grp, false, __func__, __LINE__);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(0);
        mc_mgr_one_at_a_time_end();
        return sts;
      }
      sts = mc_mgr_update_tvt(sid, dev, grp, false, __func__, __LINE__);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(0);
        mc_mgr_one_at_a_time_end();
        return sts;
      }
    }
  }

  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_set_lag_membership(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_mc_lag_id_t lag_id,
                                     bf_mc_port_map_t port_map) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate LAG Id. */
  uint32_t avail_lag_table_size = 0;
  mc_lag_get_size(dev, &avail_lag_table_size);
  if (lag_id >= avail_lag_table_size) {
    LOG_ERROR("Dev: %d Invalid LAG ID %d (max LAG ID %d) at %s:%d",
              dev,
              lag_id,
              avail_lag_table_size - 1,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  size_t i;
  size_t port_array_size = BF_MC_PORT_ARRAY_SIZE;
  /* Two characters per a byte in the port map plus a terminating character. */
  char lm[2 * BF_MC_PORT_ARRAY_SIZE + 1] = {0};

  if ((mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO ||
       mc_mgr_ctx_dev(dev)->dev_family == BF_DEV_FAMILY_TOFINO2)) {
    // Allow only one subdevice for Tofino and Tofino2.
    port_array_size = (BF_SUBDEV_PORT_COUNT + 7) / 8;
  }

  for (i = 0; i < port_array_size; ++i)
    sprintf(&lm[2 * i], "%02x", port_map[(port_array_size - 1) - i]);
  LOG_TRACE(
      "Session %#x update LAG %#x on dev %#x to %s", shdl, lag_id, dev, lm);

  mc_mgr_one_at_a_time_begin();

  bf_mc_port_map_t pm;
  sts = logical_port_map_to_physical(dev, port_map, &pm);
  if (sts != BF_SUCCESS) goto done;
  sts = mc_mgr_lag_update(sid, dev, lag_id, pm);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to update LAG %d, sts %d at %s:%d",
              lag_id,
              sts,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  } else {
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
    }
  }

done:
  mc_mgr_one_at_a_time_end();

  LOG_TRACE("Session %#x update LAG complete, status %s (%d)",
            shdl,
            bf_err_str(sts),
            sts);

  return sts;
}

bf_status_t bf_mc_get_lag_membership(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_mc_lag_id_t lag_id,
                                     bf_mc_port_map_t port_map,
                                     bool from_hw) {
  (void)from_hw;
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_mc_port_map_t pm;
  MC_MGR_MEMSET(&pm, 0, sizeof(bf_mc_port_map_t));
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
    bf_bitset_t *x, hw_bset0, hw_bset1;
    uint64_t lag_hw0_[BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    uint64_t lag_hw1_[BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    bf_bs_init(&hw_bset0, BF_PIPE_PORT_COUNT, lag_hw0_);
    bf_bs_init(&hw_bset1, BF_PIPE_PORT_COUNT, lag_hw1_);
    if (from_hw) {
      /* Read from HW.  Read both versions and make sure they match up. */
      x = &hw_bset0;
      sts = mc_mgr_get_lit_seg_reg(dev, 0, lag_id, i, &hw_bset0);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Failed to read MCast LAG table, dev %d, phy-pipe %d ver 0 row %d "
            "sts %s",
            dev,
            i,
            lag_id,
            bf_err_str(sts));
        MC_MGR_DBGCHK(sts == BF_SUCCESS);
        return sts;
      }
      sts = mc_mgr_get_lit_seg_reg(dev, 1, lag_id, i, &hw_bset1);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Failed to read MCast LAG table, dev %d, phy-pipe %d ver 1 row %d "
            "sts %s",
            dev,
            i,
            lag_id,
            bf_err_str(sts));
        MC_MGR_DBGCHK(sts == BF_SUCCESS);
        return sts;
      }
      if (!bf_bs_equal(&hw_bset0, &hw_bset1)) {
        LOG_ERROR(
            "Error reading MCast LAG table, versions out of sync, dev %d, "
            "phy-pipe %d row %d",
            dev,
            i,
            lag_id);
        LOG_ERROR("V0 0x%" PRIx64 " 0x%" PRIx64, lag_hw0_[0], lag_hw0_[1]);
        LOG_ERROR("V1 0x%" PRIx64 " 0x%" PRIx64, lag_hw1_[0], lag_hw1_[1]);
        return BF_UNEXPECTED;
      }
    } else {
      /* Read from SW, just grab it from our context. */
      x = mc_mgr_ctx_lit(dev, lag_id, i);
    }
    /* Convert the read back value from physical to logical. */
    for (size_t j = 0; j < BF_PIPE_PORT_COUNT; ++j) {
      if (!bf_bs_get(x, j)) continue;
      size_t bit_idx = j + i * BF_PIPE_PORT_COUNT;
      bf_dev_pipe_t log_pipe = 0;
      bf_dev_pipe_t phy_pipe = BIT_IDX_TO_PIPE(bit_idx);
      uint32_t loc_port = BIT_IDX_TO_LOCAL_PORT(bit_idx);
      lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
      bf_dev_port_t log_port = MAKE_DEV_PORT(log_pipe, loc_port);
      log_port = lld_sku_map_devport_from_device_to_user(dev, log_port);
      BF_MC_PORT_MAP_SET(pm, log_port);
    }
  }
  memcpy(port_map, &pm, sizeof(bf_mc_port_map_t));

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_get_lag_member_from_hash(bf_mc_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_mc_node_hdl_t node_hdl,
                                           bf_mc_lag_id_t lag_id,
                                           uint16_t level2_mcast_hash,
                                           bf_mc_l1_xid_t l1_exclusion_id,
                                           bf_mc_l2_xid_t l2_exclusion_id,
                                           bf_mc_rid_t rid,
                                           bf_dev_port_t *port,
                                           bool *is_pruned) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  *port = 0xffff;
  *is_pruned = false;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }

  /* Validate LAG Id. */
  uint32_t avail_lag_table_size = 0;
  mc_lag_get_size(dev, &avail_lag_table_size);
  if (lag_id >= avail_lag_table_size) {
    LOG_ERROR("Dev: %d Invalid LAG ID %d (max LAG ID %d) at %s:%d",
              dev,
              lag_id,
              avail_lag_table_size - 1,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  /* Validate YID. */
  if (l2_exclusion_id >=
      (BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    LOG_ERROR("Invalid L2 Exclusion ID (%#x) at %s:%d",
              l2_exclusion_id,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  /* Decode the node handle. */
  if (!mc_mgr_decode_l1_node_hdl(node_hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  /* Look up the L1 node. */
  mc_l1_node_t *l1_node =
      mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);
  if (!l1_node) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Check xid for L1 pruning */
  if (l1_node->xid_valid && l1_node->xid == l1_exclusion_id) {
    *is_pruned = true;
  }

  int l = mc_mgr_ctx_lit_np_l(dev, lag_id);
  int r = mc_mgr_ctx_lit_np_r(dev, lag_id);
  uint32_t total_cnt = l + r;
  uint32_t lag_cnts[MC_MGR_NUM_PIPES];
  uint32_t p = 0;
  for (; p < mc_mgr_ctx_num_max_pipes(dev); p++) {
    lag_cnts[p] = bf_bs_pop_count(mc_mgr_ctx_lit(dev, lag_id, p));
    total_cnt += lag_cnts[p];
  }
  if (total_cnt == 0) {
    /* Empty lag */
    goto done;
  }

  /* Tofino-2 and later will include the L1 node's RID in the LAG hash. */
  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO) {
    level2_mcast_hash = (level2_mcast_hash ^ l1_node->rid) & 0x1FFF;
  }

  /* Build an array of member ports. */
  int mbrs[MC_MGR_PORT_COUNT], mbr_cnt = 0;
  for (uint32_t phy_pipe = 0; phy_pipe < mc_mgr_ctx_num_max_pipes(dev);
       ++phy_pipe) {
    bf_bitset_t *port_map = mc_mgr_ctx_lit(dev, lag_id, phy_pipe);
    for (int local_port = bf_bs_first_set(port_map, -1); local_port != -1;
         local_port = bf_bs_first_set(port_map, local_port)) {
      int phy_dev_port = MAKE_DEV_PORT(phy_pipe, local_port);
      mbrs[mbr_cnt++] = phy_dev_port;
    }
  }

  /* Build a second array of live member ports. */
  int live[MC_MGR_PORT_COUNT], live_cnt = 0;
  for (int i = 0; i < mbr_cnt; ++i) {
    int port_bit_idx = mc_dev_port_to_bit_idx(dev, mbrs[i]);
    /* First check SW forwarding state. */
    bool is_live = true;
    mc_mgr_ctx_port_fwd_state_get_one(dev, port_bit_idx, &is_live);
    if (!is_live) continue;
    /* If Fast-Failover is enabled, check for HW liveness. */
    if (mc_mgr_ctx_ff_en(dev)) {
      mc_mgr_get_port_ff_state(dev, port_bit_idx, &is_live);
      if (!is_live) continue;
    }
    live[live_cnt++] = mbrs[i];
  }

  total_cnt &= 0x1FFF;
  int index = 0;
  if (total_cnt) {
    index = level2_mcast_hash % total_cnt;
  }

  if (index < r) {
    /* Index belongs to right remote */
    goto done;
  }
  index -= r;
  if (index >= mbr_cnt) {
    /* Index belongs to left remote */
    goto done;
  }

  /* First select the member without considering liveness. */
  int mbr = mbrs[index];
  bool mbr_is_live = true;
  /* If we have live members, ensure we've selected one of them. */
  if (live_cnt) {
    int i;
    for (i = 0; i < live_cnt; ++i)
      if (mbr == live[i]) break;
    if (i == live_cnt) {
      /* The selected member is down AND we have at least one live member, pick
       * again from the set of live members. */
      int live_index = level2_mcast_hash % live_cnt;
      mbr = live[live_index];
    } else {
      /* The selected member is live, use it! */
    }
  } else {
    /* The selected member is NOT live. */
    mbr_is_live = false;
  }

  /* Check rid and yid for L2 pruning */
  uint16_t global_rid = 0;
  mc_mgr_get_global_rid_reg(dev, &global_rid);
  if (rid == l1_node->rid || rid == global_rid) {
    int mbr_bit_idx = DEV_PORT_TO_BIT_IDX(mbr);
    bf_bitset_t *pmt = mc_mgr_ctx_pmt(dev, l2_exclusion_id);
    if (bf_bs_get(pmt, mbr_bit_idx)) {
      *is_pruned = true;
    }
  }

  /* If the member is not live and not pruned check for backup ports. */
  if (!mbr_is_live && !*is_pruned && mc_mgr_ctx_bkup_port_en(dev)) {
    int protect_port_idx = mc_dev_port_to_bit_idx(dev, mbr);
    int bkup_port_idx = mc_mgr_ctx_bkup_port(dev, protect_port_idx);
    if (bkup_port_idx != protect_port_idx)
      mbr = mc_bit_idx_to_dev_port(dev, bkup_port_idx);
  }

  physical_port_to_logical(dev, mbr, port);

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_get_remote_lag_member_count(bf_mc_session_hdl_t shdl,
                                              bf_dev_id_t dev,
                                              bf_mc_lag_id_t lag_id,
                                              int *left_count,
                                              int *right_count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;
  bf_status_t sts = BF_SUCCESS;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  /* Validate LAG Id. */
  uint32_t avail_lag_table_size = 0;
  mc_lag_get_size(dev, &avail_lag_table_size);
  if (lag_id >= avail_lag_table_size) {
    LOG_ERROR("Dev: %d Invalid LAG ID %d (max LAG ID %d) at %s:%d",
              dev,
              lag_id,
              avail_lag_table_size - 1,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  /* Validate the output params. */
  if (!left_count || !right_count) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  mc_mgr_one_at_a_time_begin();

  *left_count = mc_mgr_ctx_lit_np_l(dev, lag_id);
  *right_count = mc_mgr_ctx_lit_np_r(dev, lag_id);

  mc_mgr_one_at_a_time_end();

done:
  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d with lag_id %d, sts %s",
              __func__,
              dev,
              lag_id,
              bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_set_remote_lag_member_count(bf_mc_session_hdl_t shdl,
                                              bf_dev_id_t dev,
                                              bf_mc_lag_id_t lag_id,
                                              int left_count,
                                              int right_count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  /* Validate LAG Id. */
  uint32_t avail_lag_table_size = 0;
  mc_lag_get_size(dev, &avail_lag_table_size);
  if (lag_id >= avail_lag_table_size) {
    LOG_ERROR("Dev: %d Invalid LAG ID %d (max LAG ID %d) at %s:%d",
              dev,
              lag_id,
              avail_lag_table_size - 1,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  /* Validate counts. */
  if (0 > right_count || 0x1FFF < right_count || 0 > left_count ||
      0x1FFF < left_count) {
    LOG_ERROR(
        "Invalid counts (left %d, right %d) for LAG Id %d, dev %d at %s:%d",
        left_count,
        right_count,
        lag_id,
        dev,
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  sts = mc_mgr_lag_update_rmt_cnt(sid, dev, lag_id, left_count, right_count);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to update LAG remote counts, LAG %d (%d,%d), sts %d at %s:%d",
        lag_id,
        left_count,
        right_count,
        sts,
        __func__,
        __LINE__);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  }

  sts = mc_mgr_drv_wrl_send(sid, true);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
  }
  mc_mgr_one_at_a_time_end();

  LOG_TRACE("Session %#x update LAG complete, status %s (%d)",
            shdl,
            bf_err_str(sts),
            sts);
  return sts;
}

bf_status_t bf_mc_get_port_mc_fwd_state(bf_mc_session_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        bf_dev_port_t port_id,
                                        bool *is_active) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, port_id)) {
    LOG_ERROR("Invalid port %#x from %s:%d", port_id, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  /* Validate bool* is not null */
  if (is_active == NULL) {
    LOG_ERROR(
        "Invalid output parameter, cannot be null %s:%d", __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  bf_dev_port_t phy_port;
  logical_port_to_physical(dev, port_id, &phy_port);
  int port_index = mc_dev_port_to_bit_idx(dev, phy_port);

  mc_mgr_one_at_a_time_begin();
  sts = mc_mgr_get_port_fwd_state(sid, dev, port_index, is_active);
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to get MC port forward state sts %s (%d) session %#x dev %d "
        "port %#x",
        bf_err_str(sts),
        sts,
        shdl,
        dev,
        port_id);
  }
  LOG_TRACE(
      "Session %#x get port fwd state (port %#x, state %d), status %s (%d)",
      shdl,
      port_id,
      *is_active,
      bf_err_str(sts),
      sts);
  return sts;
}

bf_status_t bf_mc_set_port_mc_fwd_state(bf_mc_session_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        bf_dev_port_t port_id,
                                        bool is_active) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, port_id)) {
    LOG_ERROR("Invalid port %#x from %s:%d", port_id, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  bf_dev_port_t phy_port;
  logical_port_to_physical(dev, port_id, &phy_port);
  int port_index = mc_dev_port_to_bit_idx(dev, phy_port);

  mc_mgr_one_at_a_time_begin();

  sts = mc_mgr_set_port_fwd_state(sid, dev, port_index, !is_active);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to set MC port forward state sts %s (%d) session %#x dev %d "
        "port %#x to %s",
        bf_err_str(sts),
        sts,
        shdl,
        dev,
        port_id,
        is_active ? "active" : "inactive");
  } else {
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to send write list to set port mask dev %d sts %s",
                dev,
                bf_err_str(sts));
      return sts;
    }
  }
  mc_mgr_one_at_a_time_end();

  LOG_TRACE(
      "Session %#x update port fwd state (port %#x, state %d), status %s (%d)",
      shdl,
      port_id,
      is_active,
      bf_err_str(sts),
      sts);
  return sts;
}
bf_status_t bf_mc_enable_port_fast_failover(bf_mc_session_hdl_t shdl,
                                            bf_dev_id_t dev) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_set_port_ff_mode(sid, dev, true);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d, sts %s", __func__, dev, bf_err_str(sts));
  }
  return sts;
}
bf_status_t bf_mc_disable_port_fast_failover(bf_mc_session_hdl_t shdl,
                                             bf_dev_id_t dev) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_set_port_ff_mode(sid, dev, false);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d, sts %s", __func__, dev, bf_err_str(sts));
  }
  return sts;
}
bf_status_t bf_mc_get_fast_failover_state(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev,
                                          bf_dev_port_t port_id,
                                          bool *is_active) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, port_id)) {
    LOG_ERROR("Invalid port %#x from %s:%d", port_id, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  /* Validate bool* is not null */
  if (is_active == NULL) {
    LOG_ERROR("Invalid output parameter *is_active, cannot be null %s:%d",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  bf_dev_port_t phy_port;
  logical_port_to_physical(dev, port_id, &phy_port);
  int port_index = mc_dev_port_to_bit_idx(dev, phy_port);

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_get_port_ff_state(dev, port_index, is_active);
  mc_mgr_one_at_a_time_end();

  return sts;
}
bf_status_t bf_mc_clear_fast_failover_state(bf_mc_session_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            bf_dev_port_t port_id) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, port_id)) {
    LOG_ERROR("Invalid port %#x from %s:%d", port_id, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  bf_dev_port_t phy_port;
  logical_port_to_physical(dev, port_id, &phy_port);
  int port_index = mc_dev_port_to_bit_idx(dev, phy_port);

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_clr_port_ff_state(dev, port_index);
  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_set_port_prune_table(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_l2_xid_t l2_exclusion_id,
                                       bf_mc_port_map_t pruned_ports) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate YID. */
  if (l2_exclusion_id >=
      (BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    LOG_ERROR("Invalid L2 Exclusion ID (%#x) at %s:%d",
              l2_exclusion_id,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  bf_mc_port_map_t pm;
  sts = logical_port_map_to_physical(dev, pruned_ports, &pm);
  if (sts != BF_SUCCESS) goto done;
  sts = mc_mgr_update_yid_tbl(sid, dev, l2_exclusion_id, pm);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  if (BF_SUCCESS != sts) {
    mc_mgr_one_at_a_time_end();
    LOG_ERROR("Failed to update prune table idx %#x, %s (%d)",
              l2_exclusion_id,
              bf_err_str(sts),
              sts);
    return sts;
  }

done:
  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_get_port_prune_table(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_mc_l2_xid_t l2_exclusion_id,
                                       bf_mc_port_map_t *pruned_ports,
                                       bool from_hw) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate YID. */
  if (l2_exclusion_id >=
      (BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    LOG_ERROR("Invalid L2 Exclusion ID (%#x) at %s:%d",
              l2_exclusion_id,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  /* Validate output pointer. */
  if (!pruned_ports) return BF_INVALID_ARG;

  BF_MC_PORT_MAP_INIT(*pruned_ports);
  mc_mgr_one_at_a_time_begin();

  if (!from_hw) {
    bf_bitset_t *pmt = mc_mgr_ctx_pmt(dev, l2_exclusion_id);
    for (int i = 0; i < (int)mc_mgr_ctx_num_max_ports(dev); ++i) {
      if (!bf_bs_get(pmt, i)) continue;
      bf_dev_pipe_t log_pipe = 0;
      bf_dev_pipe_t phy_pipe = BIT_IDX_TO_PIPE(i);
      uint32_t loc_port = BIT_IDX_TO_LOCAL_PORT(i);
      lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
      bf_dev_port_t log_port = MAKE_DEV_PORT(log_pipe, loc_port);
      log_port = lld_sku_map_devport_from_device_to_user(dev, log_port);
      BF_MC_PORT_MAP_SET((*pruned_ports), log_port);
    }
  } else {
    for (int phy_pipe = 0; phy_pipe < (int)mc_mgr_ctx_num_max_pipes(dev);
         ++phy_pipe) {
      uint32_t log_pipe = 0;
      lld_sku_map_phy_pipe_id_to_pipe_id(dev, phy_pipe, &log_pipe);
      bf_bitset_t pmt_hw;
      uint64_t pmt_hw_[BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
      bf_bs_init(&pmt_hw, BF_PIPE_PORT_COUNT, pmt_hw_);
      sts = mc_mgr_get_pmt_seg_reg(dev, 0, l2_exclusion_id, phy_pipe, &pmt_hw);
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "Failed to get PMT seg %d on dev %d for L2-Exclusion-Id %d, %s",
            phy_pipe,
            dev,
            l2_exclusion_id,
            bf_err_str(sts));
        mc_mgr_one_at_a_time_end();
        return sts;
      }
      for (int i = 0; i < BF_PIPE_PORT_COUNT; ++i) {
        if (!bf_bs_get(&pmt_hw, i)) continue;
        bf_dev_port_t log_port = MAKE_DEV_PORT(log_pipe, i);
        log_port = lld_sku_map_devport_from_device_to_user(dev, log_port);
        BF_MC_PORT_MAP_SET(*pruned_ports, log_port);
      }
    }
  }

  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t bf_mc_get_port_prune_table_size(bf_mc_session_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            uint32_t *count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  sts = mc_get_port_prune_table_size(dev, count);
  return sts;
}

bf_status_t bf_mc_enable_port_protection(bf_mc_session_hdl_t shdl,
                                         bf_dev_id_t dev) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_set_backup_port_mode(sid, dev, true);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d, sts %s", __func__, dev, bf_err_str(sts));
  }
  return sts;
}
bf_status_t bf_mc_disable_port_protection(bf_mc_session_hdl_t shdl,
                                          bf_dev_id_t dev) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts = mc_mgr_set_backup_port_mode(sid, dev, false);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d, sts %s", __func__, dev, bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_get_port_protection(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_port_t protected_port,
                                      bf_dev_port_t *backup_port) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, protected_port)) {
    LOG_ERROR(
        "Invalid port %#x from %s:%d", protected_port, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  /* Validate [out]* is not null */
  if (backup_port == NULL) {
    LOG_ERROR("Invalid output parameter backup_port, cannot be null %s:%d",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_port_t phy_pport, phy_bport;
  logical_port_to_physical(dev, protected_port, &phy_pport);
  int phy_pport_bit_index = mc_dev_port_to_bit_idx(dev, phy_pport);
  int phy_bport_bit_index = 0;
  mc_mgr_one_at_a_time_begin();
  sts = mc_mgr_get_backup_port(
      sid, dev, phy_pport_bit_index, &phy_bport_bit_index);
  mc_mgr_one_at_a_time_end();

  if (BF_SUCCESS == sts) {
    phy_bport = mc_bit_idx_to_dev_port(dev, phy_bport_bit_index);
    physical_port_to_logical(dev, phy_bport, backup_port);
    if (protected_port != *backup_port) {
      LOG_TRACE("Dev %d port %d has multicast backup with port %d",
                dev,
                protected_port,
                *backup_port);
    } else {
      LOG_TRACE(
          "Dev %d port %d does not have backup port", dev, protected_port);
    }
  } else {
    LOG_ERROR("%s sts %s failed to get Dev %d protect port %d 's backup port",
              __func__,
              bf_err_str(sts),
              dev,
              protected_port);
  }
  return sts;
}
bf_status_t bf_mc_set_port_protection(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_port_t protected_port,
                                      bf_dev_port_t backup_port) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, protected_port)) {
    LOG_ERROR(
        "Invalid port %#x from %s:%d", protected_port, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (!mc_dev_port_validate(dev, backup_port)) {
    LOG_ERROR("Invalid port %#x from %s:%d", backup_port, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_port_t phy_pport, phy_bport;
  logical_port_to_physical(dev, protected_port, &phy_pport);
  logical_port_to_physical(dev, backup_port, &phy_bport);

  mc_mgr_one_at_a_time_begin();
  sts = mc_mgr_set_backup_port(sid,
                               dev,
                               mc_dev_port_to_bit_idx(dev, phy_pport),
                               mc_dev_port_to_bit_idx(dev, phy_bport));
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);

    /* Push the batched PVT update. */
    pipe_mgr_mc_pipe_msk_update_push(mc_mgr_ctx_pipe_sess(), true);
  }
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS == sts) {
    if (protected_port != backup_port) {
      LOG_TRACE("Dev %d port %d has multicast backup with port %d",
                dev,
                protected_port,
                backup_port);
    } else {
      LOG_TRACE("Dev %d port %d removed multicast backup", dev, protected_port);
    }
  } else {
    LOG_ERROR("%s failed with sts %s.  Dev %d protect %d backup %d",
              __func__,
              bf_err_str(sts),
              dev,
              protected_port,
              backup_port);
  }
  return sts;
}
bf_status_t bf_mc_clear_port_protection(bf_mc_session_hdl_t shdl,
                                        bf_dev_id_t dev,
                                        bf_dev_port_t port) {
  return bf_mc_set_port_protection(shdl, dev, port, port);
}

bf_status_t bf_mc_set_global_exclusion_rid(bf_mc_session_hdl_t shdl,
                                           bf_dev_id_t dev,
                                           bf_mc_rid_t rid) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  bf_status_t sts = BF_SUCCESS;
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  sts = mc_mgr_set_global_rid(dev, rid);
  if (BF_SUCCESS != sts) {
    mc_mgr_one_at_a_time_end();
    LOG_ERROR("Failed to update global rid, %s (%d)", bf_err_str(sts), sts);
    return sts;
  }

  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_mc_get_copy_to_cpu(bf_dev_id_t dev,
                                  bf_dev_port_t *port,
                                  bool *enable) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  bf_mc_session_hdl_t shdl = mc_mgr_ctx_int_sess();

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate bool* is not null */
  if (enable == NULL) {
    LOG_ERROR("Invalid output parameter *enable, cannot be null %s:%d",
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  sts = mc_mgr_get_c2c(sid, dev, port, enable);
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d get cpu port and state, sts %s",
              __func__,
              dev,
              bf_err_str(sts));
  }
  bf_dev_family_t dev_family = mc_mgr_ctx_dev_family(dev);
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
  lld_sku_get_chip_part_revision_number(dev, &rev);
  bool use_logical =
      dev_family == BF_DEV_FAMILY_TOFINO2 && rev != BF_SKU_CHIP_PART_REV_A0;

  if (!use_logical) {
    // convert hardware physical port number to logical port
    bf_dev_port_t logical_port;
    physical_port_to_logical(dev, *port, &logical_port);
    *port = logical_port;
  }

  return sts;
}

bf_status_t bf_mc_set_copy_to_cpu(bf_dev_id_t dev,
                                  bool enable,
                                  bf_dev_port_t port) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  bf_mc_session_hdl_t shdl = mc_mgr_ctx_int_sess();

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate port id. */
  if (!mc_dev_port_validate(dev, port)) {
    LOG_ERROR("Invalid port %#x from %s:%d", port, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_family_t dev_family = mc_mgr_ctx_dev_family(dev);
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
  lld_sku_get_chip_part_revision_number(dev, &rev);
  bool use_logical =
      dev_family == BF_DEV_FAMILY_TOFINO2 && rev != BF_SKU_CHIP_PART_REV_A0;

  bf_dev_pipe_t pipe;
  bf_dev_port_t prog_port;
  if (use_logical) {
    pipe = mc_dev_port_to_pipe(dev, port);
    prog_port = port;
  } else {
    bf_dev_port_t pport;
    logical_port_to_physical(dev, port, &pport);
    pipe = mc_dev_port_to_pipe(dev, pport);
    prog_port = pport;
  }
  uint32_t c2c_pipe_mask = 1 << pipe;
  dev_target_t dev_tgt = {dev, DEV_PIPE_ALL};

  mc_mgr_one_at_a_time_begin();
  /* For TF3, the c2c_pipe_mask moved to TM from Deparser. */
  bool batch = mc_mgr_in_batch(sid);
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    sts = mc_mgr_c2c_pipe_msk_set_reg_wrl(sid, dev, c2c_pipe_mask);
    if (BF_SUCCESS != sts) {
      mc_mgr_one_at_a_time_end();
      LOG_ERROR("C2C Pipe mask update failed (%d) ", sts);
      return sts;
    }
  }

  pipe_status_t x = pipe_mgr_mc_c2c_pipe_msk_set(
      mc_mgr_ctx_pipe_sess(), dev_tgt, c2c_pipe_mask, !batch, true);
  if (PIPE_SUCCESS != x) {
    LOG_ERROR("Failed to update copy-to-cpu pipe mask, dev %d mask %#x, sts %s",
              dev_tgt.device_id,
              c2c_pipe_mask,
              pipe_str_err(x));
    sts = BF_HW_COMM_FAIL;
  }
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_set_c2c(sid, dev, enable, prog_port);
    if (BF_SUCCESS == sts) {
      sts = mc_mgr_drv_wrl_send(sid, true);
    }
  }
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d en %d port %d, sts %s",
              __func__,
              dev,
              enable,
              port,
              bf_err_str(sts));
  }

  return sts;
}

bf_status_t bf_mc_set_max_nodes_before_yield(bf_mc_session_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             int count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;
  mc_mgr_one_at_a_time_begin();

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  /* Validate the count. */
  if (0 > count || 0xFF < count) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  sts = mc_mgr_set_l1_time_slice(sid, dev, count);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
done:
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d count %d, sts %s",
              __func__,
              dev,
              count,
              bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_set_max_node_threshold(bf_mc_session_hdl_t shdl,
                                         bf_dev_id_t dev,
                                         int node_count,
                                         int node_port_lag_count) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  /* Validate the session. */
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate the count. */
  if (0 > node_count || 0xFFFFF < node_count) {
    return BF_INVALID_ARG;
  }
  if (0 > node_port_lag_count || 0xFFFFF < node_port_lag_count) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  bf_status_t sts =
      mc_mgr_set_max_nodes(sid, dev, node_count, node_port_lag_count);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_drv_wrl_send(sid, true);
  }
  mc_mgr_one_at_a_time_end();
  if (BF_SUCCESS != sts) {
    LOG_ERROR("%s failed; dev %d count1 %d count2 %d, sts %s",
              __func__,
              dev,
              node_count,
              node_port_lag_count,
              bf_err_str(sts));
  }
  return sts;
}

bf_status_t bf_mc_get_pipe_vector(bf_mc_session_hdl_t shdl,
                                  bf_dev_id_t dev,
                                  bf_mc_grp_id_t grp,
                                  int *logical_pipe_vector) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  mc_mgr_one_at_a_time_begin();

  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }
  /* Validate the device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }
  /* Validate [out]* is not null */
  if (logical_pipe_vector == NULL) {
    LOG_ERROR(
        "Invalid output parameter logical_pipe_vector, cannot be null %s:%d",
        __func__,
        __LINE__);
    return BF_INVALID_ARG;
  }

  int log_pipe_vec = 0;
  uint8_t phy_pipe_vec = mc_mgr_get_pvt(dev, grp);
  uint32_t num_subdev = 0;
  lld_sku_get_num_subdev(dev, &num_subdev, NULL);
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
    if (~phy_pipe_vec & (1u << i)) continue;
    bf_dev_pipe_t log_pipe = 0;
    if (LLD_OK != lld_sku_map_phy_pipe_id_to_pipe_id(dev, i, &log_pipe)) {
      *logical_pipe_vector = 0;
      mc_mgr_one_at_a_time_end();
      return BF_INVALID_ARG;
    }
    log_pipe_vec |= (1u << log_pipe);
  }
  *logical_pipe_vector = log_pipe_vec;

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_get_max_node_threshold_exceeded_data(bf_mc_session_hdl_t shdl,
                                                       bf_dev_id_t dev,
                                                       bf_mc_grp_id_t *grp,
                                                       uint32_t *addr) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)grp;
  (void)addr;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_get_max_node_threshold_exdeeded_data_port_lag(
    bf_mc_session_hdl_t shdl,
    bf_dev_id_t dev,
    bf_mc_grp_id_t *grp,
    uint32_t *addr) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)grp;
  (void)addr;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_set_debug_filter(bf_mc_session_hdl_t shdl,
                                   bf_dev_id_t dev,
                                   uint16_t ingress_port,
                                   uint16_t mgid,
                                   uint16_t ingress_rid,
                                   uint16_t l1_xid,
                                   uint16_t l2_xid,
                                   uint16_t ingress_port_m,
                                   uint16_t mgid_m,
                                   uint16_t ingress_rid_m,
                                   uint16_t l1_xid_m,
                                   uint16_t l2_xid_m) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)ingress_port;
  (void)mgid;
  (void)ingress_rid;
  (void)l1_xid;
  (void)l2_xid;
  (void)ingress_port_m;
  (void)mgid_m;
  (void)ingress_rid_m;
  (void)l1_xid_m;
  (void)l2_xid_m;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_set_node_watch_point(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       bool enable,
                                       uint32_t address) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)pipe;
  (void)enable;
  (void)address;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_get_node_watch_point_data(bf_mc_session_hdl_t shdl,
                                            bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool *hit,
                                            bf_mc_debug_packet_state_t *state) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)pipe;
  (void)hit;
  (void)state;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_get_debug_counters(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_mc_debug_counters_t *cntrs) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)pipe;
  (void)cntrs;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_get_copy_dest_vector(bf_mc_session_hdl_t shdl,
                                       bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       uint64_t *ports_hi,
                                       uint64_t *ports_lo) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  // implement a read and clear.
  (void)shdl;
  (void)dev;
  (void)pipe;
  (void)ports_hi;
  (void)ports_lo;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_get_int_fifo_credit(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_mc_debug_int_fifo_credit_t *credit) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)dev;
  (void)pipe;
  (void)credit;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t mc_rdm_change_intr_cb_int(bf_mc_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe) {
  int sid = -1;
  if (-1 == (sid = mc_mgr_validate_session(shdl, __func__, __LINE__))) {
    return BF_INVALID_ARG;
  }
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %#x at %s:%d", dev, __func__, __LINE__);
    return BF_INVALID_ARG;
  }
  if (0 > (int)pipe || mc_mgr_ctx_num_max_pipes(dev) <= pipe) {
    LOG_ERROR("Invalid pipe %d (session %#x, dev %d) at %s:%d",
              pipe,
              shdl,
              dev,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  mc_mgr_rdm_change_done(sid, dev, pipe, true);

  return BF_SUCCESS;
}

bf_status_t bf_mc_node_garbage_collection_scheduler_mode(
    bf_mc_session_hdl_t shdl, bool interrupt_or_periodic) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  (void)shdl;
  (void)interrupt_or_periodic;
  mc_mgr_one_at_a_time_begin();
  mc_mgr_one_at_a_time_end();
  return BF_SUCCESS;
}

bf_status_t bf_mc_do_node_garbage_collection(bf_mc_session_hdl_t shdl) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = mc_mgr_one_at_a_time_begin_try();
  if (sts != BF_SUCCESS) {
    return sts;
  }

  bf_dev_id_t dev;
  int pipe;
  for (dev = 0; dev < MC_MGR_NUM_DEVICES; ++dev) {
    if (!mc_mgr_dev_present(dev)) continue;
    if (mc_mgr_is_device_locked(dev)) continue;
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      if (mc_mgr_ctx_rdm_pending_get(dev, pipe)) {
        /* Change was initiated, check the HW */
        int rc = mc_mgr_drv_read_rdm_change(dev, pipe);
        if (!rc) {
          /* Change has completed. */
          mc_rdm_change_intr_cb_int(shdl, dev, pipe);
        } else if (-1 == rc) {
          LOG_ERROR(
              "Dev %d, Pipe %d, RDM Change Read Error (%d)", dev, pipe, rc);
        } else {
          LOG_TRACE(
              "Dev %d, Pipe %d, RDM Change not complete (%#x)", dev, pipe, rc);
        }
      } else {
        /* No change has been initiated by sw yet. */
      }
    }
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

bf_status_t bf_mc_rdm_change_intr_cb(bf_mc_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe) {
  if (!mc_mgr_ready()) return BF_NOT_READY;

  bf_status_t sts = BF_SUCCESS;

  /* Validate device. */
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  /* Validate pipe. */
  if ((int)pipe < 0 || pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    LOG_ERROR(
        "Invaid dev/pipe (%d/%d) at %s:%d", dev, pipe, __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();
  sts = mc_rdm_change_intr_cb_int(shdl, dev, pipe);
  mc_mgr_one_at_a_time_end();

  return sts;
}

bf_status_t bf_dma_service_write_list_completion(bf_dev_id_t dev) {
  int x = 0;
  int num_subdevices = 0;
  /* Validate device. */
  if (MC_MGR_INVALID_DEV(dev) || !mc_mgr_dev_present(dev))
    return BF_INVALID_ARG;
  num_subdevices = mc_mgr_ctx_num_subdevices(dev);

  for (bf_subdev_id_t subdev = 0; subdev < num_subdevices; ++subdev) {
    x = lld_dr_service(dev, subdev, lld_dr_cmp_que_write_list, 10);
    if (x < 0) {
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_dma_service_write_list1_completion(bf_dev_id_t dev) {
  int x = 0;
  /* Validate device. */
  if (MC_MGR_INVALID_DEV(dev) || !mc_mgr_dev_present(dev)) {
    return BF_INVALID_ARG;
  }

  for (bf_subdev_id_t subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    x = lld_dr_service(dev, subdev, lld_dr_cmp_que_write_list_1, 10);
    if (x < 0) {
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_dma_service_read_block0_completion(bf_dev_id_t dev) {
  int x = 0;
  /* Validate device. */
  if (MC_MGR_INVALID_DEV(dev) || !mc_mgr_dev_present(dev)) {
    return BF_INVALID_ARG;
  }

  for (bf_subdev_id_t subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    x = lld_dr_service(dev, subdev, lld_dr_cmp_que_read_block_0, 10);
    if (x < 0) {
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_dma_service_read_block1_completion(bf_dev_id_t dev) {
  int x = 0;
  /* Validate device. */
  if (MC_MGR_INVALID_DEV(dev) || !mc_mgr_dev_present(dev)) {
    return BF_INVALID_ARG;
  }

  for (bf_subdev_id_t subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    x = lld_dr_service(dev, subdev, lld_dr_cmp_que_read_block_1, 10);
    if (x < 0) {
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}
