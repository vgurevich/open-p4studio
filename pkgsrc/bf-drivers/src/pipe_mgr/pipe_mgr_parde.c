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


/*!
 * @file pipe_mgr_parde.c
 * @date
 *
 * Parser and Deparser programming
 */

#include "pipe_mgr_int.h"
#include "pipe_mgr_parde.h"
#include "pipe_mgr_tof_ebuf.h"
#include "pipe_mgr_tof_ibuf.h"
#include "pipe_mgr_tof_parde.h"
#include "pipe_mgr_tof2_parde.h"
#include "pipe_mgr_tof3_parde.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_drv_intf.h"

pipe_status_t pipe_mgr_parde_device_add(pipe_sess_hdl_t shdl,
                                        rmt_dev_info_t *dev_info) {
  pipe_status_t sts;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_parde_tof_device_add(shdl, dev_info);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_parde_tof2_device_add(shdl, dev_info);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_parde_tof3_device_add(shdl, dev_info);
      break;






    default:
      sts = PIPE_INVALID_ARG;
      break;
  }
  return sts;
}

void pipe_mgr_parde_device_rmv(rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_parde_tof_device_rmv(dev_info);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_parde_tof2_device_rmv(dev_info);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_parde_tof3_device_rmv(dev_info);
      break;





    default:
      break;
  }
}

pipe_status_t pipe_mgr_parde_port_add(pipe_sess_hdl_t shdl,
                                      rmt_dev_info_t *dev_info,
                                      bf_port_cb_direction_t direction,
                                      bf_dev_port_t port_id) {
  if (BF_PORT_CB_DIRECTION_EGRESS == direction) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        return pipe_mgr_parde_tof_port_add_egr(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO2:
        return pipe_mgr_parde_tof2_port_add_egr(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO3:
        return pipe_mgr_parde_tof3_port_add_egr(shdl, dev_info, port_id);




      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  } else if (BF_PORT_CB_DIRECTION_INGRESS == direction) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        return pipe_mgr_parde_tof_port_add_ing(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO2:
        return pipe_mgr_parde_tof2_port_add_ing(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO3:
        return pipe_mgr_parde_tof3_port_add_ing(shdl, dev_info, port_id);




      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  } else {
    PIPE_MGR_DBGCHK(BF_PORT_CB_DIRECTION_EGRESS == direction ||
                    BF_PORT_CB_DIRECTION_INGRESS == direction);
    return PIPE_INVALID_ARG;
  }
}

pipe_status_t pipe_mgr_parde_complete_port_mode_transition_wa(
    pipe_sess_hdl_t shdl, rmt_dev_info_t *dev_info, bf_dev_port_t port_id) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_parde_tof_complete_port_mode_transition_wa(
          shdl, dev_info, port_id);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_port_rmv(pipe_sess_hdl_t shdl,
                                      rmt_dev_info_t *dev_info,
                                      bf_port_cb_direction_t direction,
                                      bf_dev_port_t port_id) {
  if (BF_PORT_CB_DIRECTION_EGRESS == direction) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        return pipe_mgr_parde_tof_port_rmv_egr(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO2:
        return pipe_mgr_parde_tof2_port_rmv_egr(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO3:
        return pipe_mgr_parde_tof3_port_rmv_egr(shdl, dev_info, port_id);
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  } else if (BF_PORT_CB_DIRECTION_INGRESS == direction) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        return pipe_mgr_parde_tof_port_rmv_ing(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO2:
        return pipe_mgr_parde_tof2_port_rmv_ing(shdl, dev_info, port_id);
      case BF_DEV_FAMILY_TOFINO3:
        return pipe_mgr_parde_tof3_port_rmv_ing(shdl, dev_info, port_id);
      default:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  } else {
    PIPE_MGR_DBGCHK(BF_PORT_CB_DIRECTION_EGRESS == direction ||
                    BF_PORT_CB_DIRECTION_INGRESS == direction);
    return PIPE_INVALID_ARG;
  }
}

pipe_status_t pipe_mgr_parde_traffic_disable(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ibuf_tof_disable_all_chan(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_parde_tof2_port_dis_ing_all(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_parde_tof3_port_dis_ing_all(shdl, dev_info);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_traffic_enable(pipe_sess_hdl_t shdl,
                                            rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ibuf_tof_enable_channel_all(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_parde_tof2_port_ena_ing_all(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_parde_tof3_port_ena_ing_all(shdl, dev_info);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_wait_for_traffic_flush(pipe_sess_hdl_t shdl,
                                                    rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ebuf_tof_wait_for_flush_all_chan(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO2:
      // TODO: Tofino2 changes will be done later
      return BF_SUCCESS;
    case BF_DEV_FAMILY_TOFINO3:
      // TODO: Tofino3 changes will be done later
      return BF_SUCCESS;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_traffic_enable_one_prsr(pipe_sess_hdl_t shdl,
                                                     rmt_dev_info_t *dev_info,
                                                     uint8_t logical_pipe,
                                                     int ipb_num,
                                                     bool ing_0_egr_1) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_parde_tof_port_ena_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_parde_tof2_port_ena_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_parde_tof3_port_ena_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_traffic_disable_one_prsr(pipe_sess_hdl_t shdl,
                                                      rmt_dev_info_t *dev_info,
                                                      uint8_t logical_pipe,
                                                      int ipb_num,
                                                      bool ing_0_egr_1) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_parde_tof_port_dis_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_parde_tof2_port_dis_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_parde_tof3_port_dis_one(
          shdl, dev_info, logical_pipe, ipb_num, ing_0_egr_1);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_set_port_cut_through(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  bf_dev_port_t port_id,
                                                  bool cut_through_enabled) {
  (void)shdl;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ebuf_tof_set_port_cut_through(
          dev_info, port_id, cut_through_enabled);
    case BF_DEV_FAMILY_TOFINO2:
      return PIPE_SUCCESS;
    case BF_DEV_FAMILY_TOFINO3:
      return PIPE_SUCCESS;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_port_set_drop_threshold(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     bf_dev_port_t port_id,
                                                     uint32_t drop_hi_thrd,
                                                     uint32_t drop_low_thrd) {
  (void)sess_hdl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_status_t sts;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_port_set_drop_threshold(
          dev_info, port_id, drop_hi_thrd, drop_low_thrd);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }
  return sts;
}

pipe_status_t pipe_mgr_parde_port_set_afull_threshold(pipe_sess_hdl_t sess_hdl,
                                                      bf_dev_id_t dev_id,
                                                      bf_dev_port_t port_id,
                                                      uint32_t afull_hi_thrd,
                                                      uint32_t afull_low_thrd) {
  (void)sess_hdl;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_status_t sts;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_ibuf_tof_port_set_afull_threshold(
          dev_info, port_id, afull_hi_thrd, afull_low_thrd);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }
  return sts;
}

static pipe_status_t pipe_mgr_parser_config_regs(pipe_sess_hdl_t sess_hdl,
                                                 rmt_dev_info_t *dev_info) {
  (void)sess_hdl;
  pipe_status_t err = PIPE_SUCCESS;
  for (rmt_port_info_t *port_info = dev_info->port_list;
       port_info && err == PIPE_SUCCESS;
       port_info = port_info->next) {
    err = pipe_mgr_parde_iprsr_pri_threshold_set(
        dev_info, port_info->port_id, port_info->iprsr_pri_thresh);
  }

  return err;
}

pipe_status_t pipe_mgr_parser_config_create_dma(pipe_sess_hdl_t sess_hdl,
                                                rmt_dev_info_t *dev_info) {
  // walk through all prsr_instance_hdl to configure prsr
  bf_map_sts_t map_sts;
  unsigned long prsr_instance_hdl;
  struct pipe_mgr_prsr_instance_t *instance;
  uint8_t gress;
  uint32_t profile;
  pipe_bitmap_t pipe_bmp;
  pipe_status_t sts;
  for (gress = 0; gress < PIPE_DIR_MAX; gress++) {
    for (profile = 0; profile < dev_info->num_pipeline_profiles; profile++) {
      pipe_bmp = dev_info->profile_info[profile]->pipe_bmp;
      if (PIPE_BITMAP_BITCOUNT(&pipe_bmp) == 0) continue;
      map_sts = bf_map_get_first(
          &PIPE_PRSR_DATA(
              dev_info->dev_id, PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp), gress),
          (&prsr_instance_hdl),
          (void **)&instance);
      while (map_sts == BF_MAP_OK) {
        if (instance->prsr_map == 0) continue;
        switch (dev_info->dev_family) {
          case BF_DEV_FAMILY_TOFINO:
            sts = pipe_mgr_parser_config_tof(sess_hdl,
                                             dev_info,
                                             gress,
                                             pipe_bmp,
                                             instance->prsr_map,
                                             &instance->bin_cfg.tof);
            break;
          case BF_DEV_FAMILY_TOFINO2:
            sts = pipe_mgr_parser_config_tof2(sess_hdl,
                                              dev_info,
                                              gress,
                                              pipe_bmp,
                                              instance->prsr_map,
                                              &instance->bin_cfg.tof2);
            break;
          case BF_DEV_FAMILY_TOFINO3:
            sts = pipe_mgr_parser_config_tof3(sess_hdl,
                                              dev_info,
                                              gress,
                                              pipe_bmp,
                                              instance->prsr_map,
                                              &instance->bin_cfg.tof3);
            break;
          default:
            PIPE_MGR_DBGCHK(0);
            sts = PIPE_UNEXPECTED;
        }
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d parser configure fail, dev %d, %s, pipeline_profile %d, "
              "prsr_map 0x%" PRIx64,
              __func__,
              __LINE__,
              dev_info->dev_id,
              gress == 0 ? "ingress" : "egress",
              profile,
              instance->prsr_map);
          return sts;
        }
        map_sts = bf_map_get_next(
            &PIPE_PRSR_DATA(
                dev_info->dev_id, PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp), gress),
            (&prsr_instance_hdl),
            (void **)&instance);
      }
    }
  }
  sts = pipe_mgr_parser_config_regs(sess_hdl, dev_info);
  return sts;
}

pipe_status_t pipe_mgr_parde_port_ebuf_counter_get(pipe_sess_hdl_t shdl,
                                                   rmt_dev_info_t *dev_info,
                                                   bf_dev_port_t port_id,
                                                   uint64_t *value) {
  (void)shdl;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ebuf_tof_get_port_counter(dev_info, port_id, value);
    case BF_DEV_FAMILY_TOFINO2:
      return BF_NOT_IMPLEMENTED;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_port_ebuf_bypass_counter_get(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint64_t *value) {
  (void)shdl;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ebuf_tof_get_port_bypass_counter(
          dev_info, port_id, value);
    case BF_DEV_FAMILY_TOFINO2:
      return BF_NOT_IMPLEMENTED;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_port_ebuf_100g_credits_get(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    bf_dev_port_t port_id,
    uint64_t *value) {
  (void)shdl;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_ebuf_tof_get_port_100g_credits(dev_info, port_id, value);
    case BF_DEV_FAMILY_TOFINO2:
      return BF_NOT_IMPLEMENTED;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_parde_iprsr_pri_threshold_set(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id,
                                                     uint32_t threshold) {
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }
  if (port_info->iprsr_pri_thresh == threshold) {
    return PIPE_SUCCESS;
  }

  pipe_status_t sts = PIPE_SUCCESS;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* 3 bits wide */
      if (threshold & 0xFFFFFFF8) {
        return PIPE_INVALID_ARG;
      }
      sts = pipe_mgr_tof_iprsr_set_pri_thresh(dev_info, port_info, threshold);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      /* 2 bits wide */
      if (threshold & 0xFFFFFFFC) {
        return PIPE_INVALID_ARG;
      }
      sts = pipe_mgr_tof2_iprsr_set_pri_thresh(dev_info, port_info, threshold);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* 2 bits wide */
      if (threshold & 0xFFFFFFFC) {
        return PIPE_INVALID_ARG;
      }
      sts = pipe_mgr_tof3_iprsr_set_pri_thresh(dev_info, port_info, threshold);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
  }

  if (sts == PIPE_SUCCESS) {
    port_info->iprsr_pri_thresh = threshold;
  }

  return sts;
}

pipe_status_t pipe_mgr_parde_iprsr_pri_threshold_get(rmt_dev_info_t *dev_info,
                                                     bf_dev_port_t port_id,
                                                     uint32_t *threshold) {
  rmt_port_info_t *port_info =
      pipe_mgr_get_port_info(dev_info->dev_id, port_id);
  if (!port_info) {
    LOG_ERROR("%s: port with port_id(%d) doesn't exists", __func__, port_id);
    return PIPE_INVALID_ARG;
  }

  if (pipe_mgr_warm_init_in_progress(dev_info->dev_id)) {
    /* Read from shadow only. */
    *threshold = port_info->iprsr_pri_thresh;
    return PIPE_SUCCESS;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_iprsr_get_pri_thresh(dev_info, port_info, threshold);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_iprsr_get_pri_thresh(dev_info, port_info, threshold);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_iprsr_get_pri_thresh(dev_info, port_info, threshold);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}
