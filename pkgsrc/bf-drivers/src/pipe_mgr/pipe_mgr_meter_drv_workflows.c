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
 * @file pipe_mgr_meter_drv_workflows.c
 * @date
 *
 * Meter table manager hardware interactions.
 */

/* Standard header includes */
#include <math.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include "lld/bf_dma_if.h"
#include <lld/lld_reg_if.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_meter_drv_workflows.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_db.h"

static void get_color_ram_init_data(pipe_meter_stage_idx_t meter_stage_idx,
                                    uint32_t *color_ram_data,
                                    uint32_t color_val) {
  /* There are 4 color entries per MAP RAM line */
  uint8_t subword = meter_stage_idx % 4;
  uint32_t write_mask = 0xffff;
  write_mask &= ~(0x3 << (subword * 2));
  uint16_t data = color_val << (subword * 2);
  /* This is a masked write, where the mask is of 11 bits starts from bit 11,
   * and the data is the first 10 bits. The mask is inverted
   */
  *color_ram_data = (write_mask << TOF_MAP_RAM_UNIT_WIDTH) | data;
  return;
}

static void is_max_rate_spec(pipe_mgr_meter_tbl_t *meter_tbl,
                             pipe_meter_spec_t *spec,
                             bool *c_all_green,
                             bool *p_all_green) {
  pipe_meter_tbl_info_t *tbl_info =
      pipe_mgr_get_meter_tbl_info(meter_tbl->dev_info->dev_id,
                                  meter_tbl->meter_tbl_hdl,
                                  __func__,
                                  __LINE__);

  if (tbl_info) {
    *c_all_green = spec->cburst >= tbl_info->max_burst_size &&
                   tbl_info->max_rate <= (spec->cir.type == METER_RATE_TYPE_KBPS
                                              ? spec->cir.value.kbps
                                              : spec->cir.value.pps);
    *p_all_green = spec->cburst >= tbl_info->max_burst_size &&
                   tbl_info->max_rate <= (spec->pir.type == METER_RATE_TYPE_KBPS
                                              ? spec->pir.value.kbps
                                              : spec->pir.value.pps);
  } else {
    *c_all_green = false;
    *p_all_green = false;
  }
}

static void is_zero_rate_spec(pipe_meter_spec_t *spec,
                              bool *c_all_red,
                              bool *p_all_red) {
  *c_all_red = !(spec->cir.type == METER_RATE_TYPE_KBPS ? spec->cir.value.kbps
                                                        : spec->cir.value.pps);
  *p_all_red = !(spec->pir.type == METER_RATE_TYPE_KBPS ? spec->pir.value.kbps
                                                        : spec->pir.value.pps);
}

static pipe_status_t pipe_mgr_meter_drv_get_burstsize_params(
    bool all_green,
    bool all_red,
    uint64_t burstsize,
    uint32_t *burst_mantissa,
    uint32_t *burst_exponent,
    uint64_t *actual_burstsize,
    bool byte_meter) {
  if (all_red) {
    /* Set a bucket size of zero. */
    *burst_mantissa = 0;
    *burst_exponent = 0;
    *actual_burstsize = 0;
    return PIPE_SUCCESS;
  }
  if (all_green) {
    /* Set a maximum bucket size. */
    *burst_mantissa = 255;
    *burst_exponent = 31;
    *actual_burstsize = *burst_mantissa * pow(2, (double)*burst_exponent);
    return PIPE_SUCCESS;
  }

  float error = 0.0;
  uint64_t burst = 0;

  if (byte_meter) {
    burst = (burstsize * 1000) / 8;
  } else {
    burst = burstsize;
  }

  uint64_t iter = burst;
  while (iter > 255) {
    (*burst_exponent)++;
    iter = iter >> 1;
  }

  *burst_mantissa = iter & 0xff;

  /* Now compare the burstsize that was passed in, to the mantissa,exponent
   * that we arrived at, the error should be within tolerable limits.
   */
  *actual_burstsize = *burst_mantissa * pow(2, (double)*burst_exponent);
  error = (float)burst / (float)*actual_burstsize;

  if (error > 1.2 || error < 0.8) {
    /* Approximation too erroneous, return an error */
    return PIPE_NOT_SUPPORTED;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_meter_drv_get_rate_params(
    bf_dev_id_t dev_id,
    bool all_green,
    bool all_red,
    pipe_meter_rate_t *meter_rate,
    uint32_t burst_exp_adj,
    uint64_t burst_sz,
    uint32_t *exponent,
    uint32_t *mantissa,
    uint32_t *burst_limited) {
  if (all_red) {
    /* Set a mantissa of 0 to indicate no rate.  Set an exponent larger than 27
     * to ensure the meter never sees time advance. */
    *mantissa = 0;
    *exponent = 31;
    *burst_limited = 0;
    return PIPE_SUCCESS;
  }
  if (all_green) {
    /* Set maximum values for mantissa and exponent. */
    *mantissa = 511;
    *exponent = 0;
    *burst_limited = 0;
    return PIPE_SUCCESS;
  }

  float units_per_cycle = 0;
  float iter = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(dev_id);

  if (!clock_speed) {
    LOG_ERROR(
        "%s:%d Clock speed for device id %d is 0", __func__, __LINE__, dev_id);
    return PIPE_UNEXPECTED;
  }

  /* Compute the exponent and mantissa (final mantissa, not relative) for a
   * given rate.  Ideally we would like to maximize the exponent since that
   * results in the meter being credited more often.
   *   rate = mantissa * 2^-exponent
   *   rate = mantissa / 2^exponent
   * Legal values for mantissa are 1-511
   * Legal values for exponent+burst_exp_adj are 0-27
   */

  /* The meter rate spec will be in kPackets/second or kBits/second.
   * Need to convert it to packets/cycle or bytes/cycle.
   */
  if (meter_rate->type == METER_RATE_TYPE_KBPS) {
    units_per_cycle =
        ((float)(meter_rate->value.kbps * 1000) / 8) / (float)clock_speed;
  } else {
    units_per_cycle = (float)meter_rate->value.pps / (float)clock_speed;
  }
  iter = units_per_cycle;

  *exponent = 0;
  *mantissa = (uint32_t)iter & 0x1FF;

  /* Keep increasing the exponent as long as the mantissa is still representible
   * in a 9 bit number. */
  *burst_limited = 0;
  while (iter * 2 <= 511 && *exponent + 1 + burst_exp_adj <= 27) {
    if (iter * 2 > burst_sz) {
      /* Ideally we would continue to increase the mantissa and exponent however
       * we must stop since the mantissa cannot be larger than the burst size.
       * Continue to look as if we were not limited so we can return an ideal
       * burst size to the caller. */
      float m = iter;
      uint32_t e = *exponent;
      while (m * 2 <= 511 && e + 1 + burst_exp_adj <= 27) {
        ++e;
        m *= 2;
      }
      *burst_limited = (uint32_t)m & 0x1FF;
      break;
    }
    (*exponent)++;
    iter *= 2;

    *mantissa = (uint32_t)iter & 0x1ff;
  }

  if ((*exponent + burst_exp_adj) < 28 && *mantissa < 512) return PIPE_SUCCESS;

  return PIPE_NOT_SUPPORTED;
}

static pipe_status_t pipe_mgr_meter_tof_encode_meter_spec(
    bf_dev_id_t dev_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_meter_spec_t *meter_spec,
    rmt_ram_line_t *ram_line) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tof_entry_t tof_meter_entry;
  uint32_t cburst_mantissa = 0;
  uint32_t cburst_exponent = 0;
  uint32_t pburst_mantissa = 0;
  uint32_t pburst_exponent = 0;
  uint32_t cir_exponent = 0;
  uint32_t cir_mantissa = 0;
  uint32_t pir_exponent = 0;
  uint32_t pir_mantissa = 0;
  uint32_t cburst_exponent_adj = 0;
  uint32_t pburst_exponent_adj = 0;
  uint64_t cburst = 0, pburst = 0;

  PIPE_MGR_MEMSET(&tof_meter_entry, 0, sizeof(pipe_mgr_meter_tof_entry_t));

  bool c_all_red, p_all_red;
  is_zero_rate_spec(meter_spec, &c_all_red, &p_all_red);
  bool c_all_green, p_all_green;
  is_max_rate_spec(meter_tbl, meter_spec, &c_all_green, &p_all_green);
  bool byte_meter =
      (meter_spec->cir.type == METER_RATE_TYPE_KBPS) ? true : false;

  /* Calculate the committed burst size parameters. */
  status = pipe_mgr_meter_drv_get_burstsize_params(c_all_green,
                                                   c_all_red,
                                                   meter_spec->cburst,
                                                   &cburst_mantissa,
                                                   &cburst_exponent,
                                                   &cburst,
                                                   byte_meter);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in representing the committed burst size %" PRIu64,
              __func__,
              __LINE__,
              meter_spec->cburst);
    return PIPE_NOT_SUPPORTED;
  }
  cburst_exponent_adj = cburst_exponent > 14 ? cburst_exponent - 14 : 0;

  /* Calculate the peak burst size parameters. */
  status = pipe_mgr_meter_drv_get_burstsize_params(p_all_green,
                                                   p_all_red,
                                                   meter_spec->pburst,
                                                   &pburst_mantissa,
                                                   &pburst_exponent,
                                                   &pburst,
                                                   byte_meter);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in representing the peak burst size %" PRIu64,
              __func__,
              __LINE__,
              meter_spec->pburst);
    return PIPE_NOT_SUPPORTED;
  }
  pburst_exponent_adj = pburst_exponent > 14 ? pburst_exponent - 14 : 0;

  /* Calculate the commited rate parameters. */
  uint32_t burst_limited = 0;
  status = pipe_mgr_meter_drv_get_rate_params(dev_id,
                                              c_all_green,
                                              c_all_red,
                                              &meter_spec->cir,
                                              cburst_exponent_adj,
                                              cburst,
                                              &cir_exponent,
                                              &cir_mantissa,
                                              &burst_limited);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in representing the CIR", __func__, __LINE__);
    return status;
  }
  if (cir_mantissa > cburst) {
    LOG_ERROR(
        "%s:%d Committed burst size %" PRIu64
        " is too small for the requested CIR, it should be larger than %d",
        __func__,
        __LINE__,
        cburst,
        cir_mantissa);
    return PIPE_NOT_SUPPORTED;
  }
  if (burst_limited) {
    LOG_WARN("%s:%d Committed burst size %" PRIu64
             " is too small, increasing it to %d may improve meter accuracy",
             __func__,
             __LINE__,
             cburst,
             burst_limited);
  }

  /* Calculate the commited rate parameters. */
  burst_limited = 0;
  status = pipe_mgr_meter_drv_get_rate_params(dev_id,
                                              p_all_green,
                                              p_all_red,
                                              &meter_spec->pir,
                                              pburst_exponent_adj,
                                              pburst,
                                              &pir_exponent,
                                              &pir_mantissa,
                                              &burst_limited);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in representing the PIR", __func__, __LINE__);
    return status;
  }
  if (pir_mantissa > pburst) {
    LOG_ERROR(
        "%s:%d Peak burst size %" PRIu64
        " is too small for the requested PIR, it should be larger than %d",
        __func__,
        __LINE__,
        pburst,
        pir_mantissa);
    return PIPE_NOT_SUPPORTED;
  }
  if (burst_limited) {
    LOG_WARN("%s:%d Peak burst size %" PRIu64
             " is too small, increasing it to %d may improve meter accuracy",
             __func__,
             __LINE__,
             pburst,
             burst_limited);
  }

  /* Convert the exponents to the "relative_exponents". */
  cir_exponent = 31 - cir_exponent;
  pir_exponent = 31 - pir_exponent;

  /* Finally encode the meter entry in the RAM line.  Note the bucket levels are
   * always initialized to the full bucket level values unless it is an "all
   * red" meter and then the levels are set to a negative value. */
  tof_meter_entry.cir_exponent = cir_exponent;
  tof_meter_entry.cir_mantissa = cir_mantissa;
  tof_meter_entry.pir_exponent = pir_exponent;
  tof_meter_entry.pir_mantissa = pir_mantissa;
  tof_meter_entry.cbs_exponent = cburst_exponent;
  tof_meter_entry.cbs_mantissa = cburst_mantissa;
  tof_meter_entry.pbs_exponent = pburst_exponent;
  tof_meter_entry.pbs_mantissa = pburst_mantissa;
  if (c_all_red) {
    tof_meter_entry.committed_level = 0x7FFFFF;
  } else {
    tof_meter_entry.committed_level =
        cburst_mantissa << (cburst_exponent - cburst_exponent_adj);
  }
  if (p_all_red) {
    tof_meter_entry.peak_level = 0x7FFFFF;
  } else {
    tof_meter_entry.peak_level = pburst_mantissa
                                 << (pburst_exponent - pburst_exponent_adj);
  }
  tof_meter_entry.u.color_aware =
      meter_spec->meter_type == METER_TYPE_COLOR_AWARE;
  pipe_mgr_meter_encode_meter_entry(ram_line, tof_meter_entry);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_meter_stage_idx_t meter_stage_idx,
    pipe_meter_spec_t *meter_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_instr_set_memdata_v_t instr;
  pipe_instr_set_memdata_i_only_t p_instr;
  rmt_virt_addr_t ent_virt_addr;
  rmt_ram_line_t ram_line;
  bf_dev_pipe_t pipe_id;
  uint32_t color_map_ram_data = 0;
  uint8_t ltbl_id = meter_tbl_stage_info->stage_table_handle;
  uint8_t stage_id = meter_tbl_stage_info->stage_id;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));

  bool c_all_red, p_all_red;
  is_zero_rate_spec(meter_spec, &c_all_red, &p_all_red);

  /* Compute the entry virtual address */
  ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);

  /* Encode the meter contents */
  status = pipe_mgr_meter_tof_encode_meter_spec(
      device_id, meter_tbl, meter_spec, &ram_line);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error %s while encoding meter spec; CIR %" PRIu64
              " %s CBurst %" PRIu64 " PIR %" PRIu64 " %s PBurst %" PRIu64,
              __func__,
              __LINE__,
              pipe_str_err(status),
              meter_spec->cir.type == METER_RATE_TYPE_KBPS
                  ? meter_spec->cir.value.kbps
                  : meter_spec->cir.value.pps,
              meter_spec->cir.type == METER_RATE_TYPE_KBPS ? "kbps" : "pps",
              meter_spec->cburst,
              meter_spec->pir.type == METER_RATE_TYPE_KBPS
                  ? meter_spec->pir.value.kbps
                  : meter_spec->pir.value.pps,
              meter_spec->pir.type == METER_RATE_TYPE_KBPS ? "kbps" : "pps",
              meter_spec->pburst);
    return status;
  }

  /* Also initialize the color MAP RAM to indicate Green */
  uint32_t color = c_all_red ? (p_all_red ? 3 : 1) : 0;
  get_color_ram_init_data(meter_stage_idx, &color_map_ram_data, color);

  uint32_t map_ram_line = (meter_stage_idx / 4) % TOF_MAP_RAM_UNIT_DEPTH;
  uint8_t blk_idx = meter_stage_idx / (4 * TOF_MAP_RAM_UNIT_DEPTH);

  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info =
      meter_tbl_stage_info->ram_alloc_info;
  mem_id_t mem_id = ram_alloc_info->color_tbl_word_blk[blk_idx].mem_id[0];

  construct_instr_set_memdata_no_data(device_id,
                                      &p_instr,
                                      4,
                                      mem_id,
                                      meter_tbl->direction,
                                      stage_id,
                                      map_ram_line,
                                      pipe_mem_type_map_ram);

  status = pipe_mgr_drv_ilist_add_2(&sess_hdl,
                                    meter_tbl->dev_info,
                                    &pipe_bmp,
                                    stage_id,
                                    (uint8_t *)&p_instr,
                                    sizeof(pipe_instr_set_memdata_i_only_t),
                                    (uint8_t *)&color_map_ram_data,
                                    4);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in ilist add, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  /* Now, add the instruction to write the meter spec */

  construct_instr_set_v_memdata(device_id,
                                &instr,
                                (uint8_t *)ram_line,
                                sizeof(rmt_ram_line_t),
                                ltbl_id,
                                pipe_virt_mem_type_meter,
                                ent_virt_addr);

  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  meter_tbl->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_set_memdata_v_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in ilist add, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  /* Update the shadow memory with the encoded meter spec */

  if (meter_tbl->symmetric) {
    pipe_id = meter_tbl->lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp);
  }

  status = pipe_mgr_phy_mem_map_write(device_id,
                                      meter_tbl->direction,
                                      pipe_id,
                                      stage_id,
                                      pipe_mem_type_unit_ram,
                                      ram_id,
                                      ram_line_num,
                                      (uint8_t *)ram_line,
                                      NULL);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating shadow memory for meter spec for ram id %d"
        " stage id %d, err %s",
        __func__,
        __LINE__,
        ram_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_spec_move_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t src_stage_id,
    uint8_t dst_stage_id,
    mem_id_t src_ram_id,
    uint32_t src_ram_line_num,
    mem_id_t dst_ram_id,
    uint32_t dst_ram_line_num,
    vpn_id_t dst_vpn,
    uint8_t ltbl_id) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t src_phy_addr = 0;
  uint32_t dst_phy_addr = 0;
  pipe_instr_set_memdata_v_t instr;
  uint8_t *data = NULL;
  bf_dev_pipe_t pipe_id;

  if (meter_tbl->symmetric) {
    pipe_id = meter_tbl->lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp);
  }

  src_phy_addr = meter_tbl->dev_info->dev_cfg.get_relative_phy_addr(
      src_ram_id, src_ram_line_num, pipe_mem_type_unit_ram);

  dst_phy_addr = meter_tbl->dev_info->dev_cfg.get_relative_phy_addr(
      dst_ram_id, dst_ram_line_num, pipe_mem_type_unit_ram);

  status = pipe_mgr_phy_mem_map_copy(device_id,
                                     meter_tbl->direction,
                                     pipe_id,
                                     src_stage_id,
                                     dst_stage_id,
                                     src_phy_addr,
                                     dst_phy_addr,
                                     true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in copying shadow memory from src addr 0x%x to"
        " dst addr 0x%x, src stage id %d, dst stage id %d err %s",
        __func__,
        __LINE__,
        src_phy_addr,
        dst_phy_addr,
        src_stage_id,
        dst_stage_id,
        pipe_str_err(status));
    return status;
  }

  if (src_stage_id != dst_stage_id) {
    /* If the move is across stages, then need to program the meter spec
     * to the new location. If it is within the same stage, then the
     * "movereg" in hardware would take care of moving the meter spec, and
     * we just update the shadow copy.
     */
    /* Compute the entry virtual address */
    rmt_virt_addr_t ent_virt_addr =
        pipe_mgr_meter_compute_ent_virt_addr(dst_vpn, dst_ram_line_num);

    status = pipe_mgr_phy_mem_map_get_ref(device_id,
                                          meter_tbl->direction,
                                          pipe_mem_type_unit_ram,
                                          pipe_id,
                                          dst_stage_id,
                                          dst_ram_id,
                                          dst_ram_line_num,
                                          &data,
                                          false);

    if (status != PIPE_SUCCESS) {
      return status;
    }
    construct_instr_set_v_memdata(device_id,
                                  &instr,
                                  data,
                                  sizeof(rmt_ram_line_t),
                                  ltbl_id,
                                  pipe_virt_mem_type_meter,
                                  ent_virt_addr);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    meter_tbl->dev_info,
                                    &pipe_bmp,
                                    dst_stage_id,
                                    (uint8_t *)&instr,
                                    sizeof(pipe_instr_set_memdata_v_t));

    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error in ilist add, err %s",
                __func__,
                __LINE__,
                pipe_str_err(status));
      return status;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_lpf_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_lpf_spec_t *lpf_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_instr_set_memdata_v_t instr;
  rmt_virt_addr_t ent_virt_addr;
  rmt_ram_line_t ram_line;
  bf_dev_pipe_t pipe_id;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));

  /* Compute the entry virtual address */
  ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);

  /* Encode the meter contents */
  status = pipe_mgr_meter_tof_encode_lpf_spec(device_id, lpf_spec, &ram_line);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in encoding meter spec. Error %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  construct_instr_set_v_memdata(device_id,
                                &instr,
                                (uint8_t *)ram_line,
                                sizeof(rmt_ram_line_t),
                                ltbl_id,
                                pipe_virt_mem_type_meter,
                                ent_virt_addr);

  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  meter_tbl->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_set_memdata_v_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in ilist add, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  if (meter_tbl->symmetric) {
    pipe_id = meter_tbl->lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp);
  }
  /* Update the shadow memory with the encoded meter spec */
  status = pipe_mgr_phy_mem_map_write(device_id,
                                      meter_tbl->direction,
                                      pipe_id,
                                      stage_id,
                                      pipe_mem_type_unit_ram,
                                      ram_id,
                                      ram_line_num,
                                      (uint8_t *)ram_line,
                                      NULL);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating shadow memory for meter spec for ram id %d"
        " stage id %d, err %s",
        __func__,
        __LINE__,
        ram_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_wred_spec_update_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_wred_spec_t *wred_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_instr_set_memdata_v_t instr;
  rmt_virt_addr_t ent_virt_addr;
  rmt_ram_line_t ram_line;
  bf_dev_pipe_t pipe_id;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));

  /* Compute the entry virtual address */
  ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);

  /* Encode the meter contents */
  status = pipe_mgr_meter_tof_encode_wred_spec(device_id, wred_spec, &ram_line);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in encoding meter spec. Error %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  construct_instr_set_v_memdata(device_id,
                                &instr,
                                (uint8_t *)ram_line,
                                sizeof(rmt_ram_line_t),
                                ltbl_id,
                                pipe_virt_mem_type_meter,
                                ent_virt_addr);

  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  meter_tbl->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_set_memdata_v_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in ilist add, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  if (meter_tbl->symmetric) {
    pipe_id = meter_tbl->lowest_pipe_id;
  } else {
    pipe_id = PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp);
  }
  /* Update the shadow memory with the encoded meter spec */
  status = pipe_mgr_phy_mem_map_write(device_id,
                                      meter_tbl->direction,
                                      pipe_id,
                                      stage_id,
                                      pipe_mem_type_unit_ram,
                                      ram_id,
                                      ram_line_num,
                                      (uint8_t *)ram_line,
                                      NULL);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating shadow memory for meter spec for ram id %d"
        " stage id %d, err %s",
        __func__,
        __LINE__,
        ram_id,
        stage_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_max_meter_spec_workflow(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_bitmap_t pipe_bmp,
    uint8_t ltbl_id,
    uint8_t stage_id,
    vpn_id_t vpn,
    uint32_t ram_line_num) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_instr_set_memdata_v_t instr;
  rmt_virt_addr_t ent_virt_addr;
  rmt_ram_line_t ram_line;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));

  /* Compute the entry virtual address */
  ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);

  /* Encode the meter contents */
  status = pipe_mgr_meter_tof_encode_max_meter_spec(&ram_line);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in encoding max meter spec. Error %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  construct_instr_set_v_memdata(meter_tbl->dev_info->dev_id,
                                &instr,
                                (uint8_t *)ram_line,
                                sizeof(rmt_ram_line_t),
                                ltbl_id,
                                pipe_virt_mem_type_meter,
                                ent_virt_addr);

  status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                  meter_tbl->dev_info,
                                  &pipe_bmp,
                                  stage_id,
                                  (uint8_t *)&instr,
                                  sizeof(pipe_instr_set_memdata_v_t));

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error during ilist add, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tof_encode_lpf_spec(bf_dev_id_t device_id,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 rmt_ram_line_t *ram_line) {
  pipe_lpf_type_e lpf_type = lpf_spec->lpf_type;
  pipe_mgr_lpf_red_tof_entry_t lpf_red_entry;
  double gain_time_constant_in_cycles = 0;
  double decay_time_constant_in_cycles = 0;
  double time_constant_in_cycles = 0;
  int exponent = 0;
  int mantissa = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(device_id);

  PIPE_MGR_MEMSET(&lpf_red_entry, 0, sizeof(pipe_mgr_lpf_red_tof_entry_t));

  /* The time constant specified in the spec is in nano seconds */

  if (lpf_spec->gain_decay_separate_time_constant == true) {
    gain_time_constant_in_cycles =
        (lpf_spec->gain_time_constant * clock_speed) / 1000000000.0;
    pipe_mgr_lpf_get_exponent_mantissa(
        gain_time_constant_in_cycles, &exponent, &mantissa);
    if (exponent == -1 || mantissa == -1) {
      LOG_ERROR("%s:%d Error in encoding gain time constant value of %f",
                __func__,
                __LINE__,
                lpf_spec->gain_time_constant);
      return PIPE_NO_SYS_RESOURCES;
    }
    lpf_red_entry.gain_time_constant_exponent = exponent;
    lpf_red_entry.time_constant_mantissa = mantissa;
    decay_time_constant_in_cycles =
        (lpf_spec->decay_time_constant * clock_speed) / 1000000000.0;
    pipe_mgr_lpf_get_exponent_mantissa(
        decay_time_constant_in_cycles, &exponent, &mantissa);
    if (exponent == -1 || mantissa == -1) {
      LOG_ERROR("%s:%d Error in encoding gain time constant value of %f",
                __func__,
                __LINE__,
                lpf_spec->gain_time_constant);
      return PIPE_NO_SYS_RESOURCES;
    }
    lpf_red_entry.decay_time_constant_exponent = exponent;
    /* There is a common mantissa, ensure it is that way */
    if (mantissa != lpf_red_entry.time_constant_mantissa) {
      LOG_ERROR(
          "%s:%d Gain time and decay time constant did not result in the same"
          " mantissa, %f & %f",
          __func__,
          __LINE__,
          lpf_spec->gain_time_constant,
          lpf_spec->decay_time_constant);
      PIPE_MGR_DBGCHK(mantissa == lpf_red_entry.time_constant_mantissa);
      return PIPE_UNEXPECTED;
    }
  } else {
    time_constant_in_cycles =
        (lpf_spec->time_constant * clock_speed) / 1000000000.0;
    pipe_mgr_lpf_get_exponent_mantissa(
        time_constant_in_cycles, &exponent, &mantissa);
    if (exponent == -1 || mantissa == -1) {
      LOG_ERROR("%s:%d Error in encoding time constant value of %f",
                __func__,
                __LINE__,
                lpf_spec->time_constant);
      return PIPE_NO_SYS_RESOURCES;
    }
    lpf_red_entry.gain_time_constant_exponent = exponent;
    lpf_red_entry.decay_time_constant_exponent = exponent;
    lpf_red_entry.time_constant_mantissa = mantissa;
  }

  lpf_red_entry.lpf_action_scale = lpf_spec->output_scale_down_factor & 0x1f;
  lpf_red_entry.rate_sample_enable = (lpf_type == LPF_TYPE_SAMPLE) ? 0 : 1;

  /* Encode the entry in the ram_line */
  pipe_mgr_lpf_red_encode_entry(ram_line, lpf_red_entry);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_compute_wred_params(
    pipe_mgr_lpf_red_tof_entry_t *wred_entry, pipe_wred_spec_t *wred_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t red_level100 = 0;
  uint32_t red_dlevel100 = 0;
  uint32_t red_level100_with_scaling = 0;
  uint32_t red_dlevel100_with_scaling = 0;
  uint32_t red_level0 = 0;
  uint32_t red_levelmax = 0;
  uint32_t max_drop_probability = 0;
  uint8_t probability_scale = 0;
  float rng_value = 0;
  float rng_value_with_scaling = 0;
  float slope = 0;
  uint8_t red_exponent = 0;
  uint8_t red_exponent_with_scaling = 0;

  if (wred_spec == NULL) {
    return PIPE_INVALID_ARG;
  }

  red_level0 = wred_spec->red_min_threshold;
  red_levelmax = wred_spec->red_max_threshold;

  max_drop_probability = ceil(wred_spec->max_probability * 100);

  if (max_drop_probability == 0) {
    LOG_ERROR("%s:%d Max drop probability of ZERO is not supported for WRED",
              __func__,
              __LINE__);
    return PIPE_NOT_SUPPORTED;
  }

  /* First, calculate level100 based on max probability */
  /* Based on the percentage drop probability, calculate the 8-bit
   * random number which represents it.
   */
  probability_scale = pow(2, (double)wred_entry->red_probability_scale);

  if (max_drop_probability < 100) {
    rng_value = (max_drop_probability / 100.0) * 255 * probability_scale;
    if (rng_value > 255) {
      rng_value = floorf(rng_value);
    }
  } else {
    /* Max drop probability is 100% */
    rng_value = 255.0;
  }

  /* Do calculations without probability scale first and then compare it
   * with probability scale
   */

  /* Now, calculate the slope of the WRED curve */
  if (wred_spec->red_max_threshold == wred_spec->red_min_threshold) {
    if (max_drop_probability != 100) {
      LOG_ERROR(
          "%s:%d Max drop probability of %d not supported when WRED min "
          "and max thresholds are the same",
          __func__,
          __LINE__,
          max_drop_probability);
      return PIPE_NOT_SUPPORTED;
    }
    red_level100 = red_levelmax;
  } else {
    if (max_drop_probability == 100) {
      red_level100 = red_levelmax;
    } else {
      slope = rng_value /
              (wred_spec->red_max_threshold - wred_spec->red_min_threshold);
      /* Now, based on the slope, calculate Level100
       * The equation is this : slope = 255/(Level100 - Level0)
       */
      if (slope) {
        red_level100 = (255 + (slope * red_level0)) / slope;
      }
    }
  }

  red_dlevel100 = red_level100 - red_level0;

  status = pipe_mgr_meter_get_red_exponent(
      red_dlevel100, red_level0, red_levelmax, &red_exponent);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in encoding RED exponent, red level0 %d"
        " red level100 %d, red_levelmax %d, err %s",
        __func__,
        __LINE__,
        red_level0,
        red_level100,
        red_levelmax,
        pipe_str_err(status));
    return status;
  }

  /* Now, calculate the red probability scale bits */
  wred_entry->red_probability_scale =
      pipe_mgr_meter_get_red_probability_scale(rng_value, max_drop_probability);
  probability_scale = pow(2, (double)wred_entry->red_probability_scale);

  if (probability_scale > 1 &&
      (wred_spec->red_max_threshold != wred_spec->red_min_threshold)) {
    /* If min and max are the same, then it implies we just want one
     * threshold, below which there are no drops, and above which 100%
     * drops..implies that max drop probability is 100%. This check was done
     * above. This renders probability scaling useles. Probability scaling has
     * an impact in getting us finer step sizes in drop probability increase
     * between the two thresholds for low max drop prbabilities especially.  So,
     * we do not use probability scaling when
     * the two thresholds are the same.
     */

    /* If we chose a probability scale, then check on the RED exponent
     * based on the new dlevel100 value. The exponent plays a major role
     * in step size in thresholds between level0 and levelmax. If the
     * exponent we get is the same or greater than without probability
     * scaling, it implies we are better off without probability scaling.
     */
    rng_value_with_scaling = rng_value * probability_scale;

    slope = rng_value_with_scaling /
            (wred_spec->red_max_threshold - wred_spec->red_min_threshold);
    PIPE_MGR_DBGCHK(slope > 0);
    red_level100_with_scaling = (255 + (slope * red_level0)) / slope;

    /* Now, based on the slope, calculate Level100
     * The equation is this : slope = 255/(Level100 - Level0)
     */
    red_dlevel100_with_scaling = red_level100_with_scaling - red_level0;
    status = pipe_mgr_meter_get_red_exponent(red_dlevel100_with_scaling,
                                             red_level0,
                                             red_levelmax,
                                             &red_exponent_with_scaling);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in encoding RED exponent, red level0 %d"
          " red level100 %d, red_levelmax %d, err %s",
          __func__,
          __LINE__,
          red_level0,
          red_level100,
          red_levelmax,
          pipe_str_err(status));
      return status;
    }
    if (red_exponent_with_scaling <= red_exponent) {
      red_dlevel100 = red_dlevel100_with_scaling;
      wred_entry->red_level_exponent = red_exponent_with_scaling;
    } else {
      wred_entry->red_probability_scale = 0;
      wred_entry->red_level_exponent = red_exponent;
    }
  } else {
    wred_entry->red_probability_scale = 0;
    wred_entry->red_level_exponent = red_exponent;
  }

  /* Now get the mantissa for level0, levelmax and leveld100 */
  wred_entry->red_dlevel100 = pipe_mgr_meter_get_red_mantissa(
      red_dlevel100, wred_entry->red_level_exponent);
  wred_entry->red_level0 = pipe_mgr_meter_get_red_mantissa(
      red_level0, wred_entry->red_level_exponent);
  wred_entry->red_level_max = pipe_mgr_meter_get_red_mantissa(
      red_levelmax, wred_entry->red_level_exponent);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_get_red_exponent(uint32_t dlevel100,
                                              uint32_t level0,
                                              uint32_t levelmax,
                                              uint8_t *exponent) {
  /* Idea is to always minimize the exponent, to minimize the exponent
   * to minimize the error in representing the values. For RED, there
   * is a common exponent which is used to represent all the three
   * parameters. Hence pick the largest value to be represented and
   * arrive at the least exponent for that.
   */
  uint32_t max_val = dlevel100;
  if (exponent) {
    *exponent = 0;
  } else {
    return PIPE_INVALID_ARG;
  }

  if (max_val < level0) {
    max_val = level0;
  }
  if (max_val < levelmax) {
    max_val = levelmax;
  }
  /* Given 8bit mantissa, we need to solve the following equation to
   * with minimum exponent.
   * max_val/2^exponent <= 255
   */
  PIPE_MGR_DBGCHK(max_val > 0);
  while (max_val > 255) {
    max_val = max_val / 2;
    (*exponent)++;
  }
  /* Check if the exponent exceeds 31, which is the maximum */
  if (*exponent > 31) {
    /* This value cannot be represented with the available bits */
    return PIPE_NOT_SUPPORTED;
  }
  return PIPE_SUCCESS;
}

uint8_t pipe_mgr_meter_get_red_mantissa(uint32_t red_level, uint8_t exponent) {
  return red_level / (1 << exponent);
}

uint8_t pipe_mgr_meter_get_red_probability_scale(
    float rng_value, uint32_t max_drop_probability) {
  double iter = rng_value;
  uint32_t drop_probability_with_scaling = 100;
  uint8_t probability_scale = 0;
  /* We always want to use probability scaling, because that gives us a
   * more precision to the WRED drop curve, by the way of making it more
   * responsive to changes in the average value. We want to use the maximum
   * possible scaling, which is 1/2^7 (3bits of probability scale), with the
   * limitation
   * that the new RNG value resulting as a result of this probability scaling
   * is less than 255 (8bit RNG is used in Tofino).
   */

  /* Also, compare the max drop probability that can be obtained
   * by choosing this probability scaling with that of
   * the required max drop probability. We can only scale up the probability
   * scaling until the max drop probability with scaling exceeds what is the
   * required maximum drop probability.
   */
  while (iter <= 255 &&
         (drop_probability_with_scaling >= max_drop_probability)) {
    iter = iter * 2;
    if (probability_scale == 7) {
      break;
    }
    probability_scale++;
    drop_probability_with_scaling =
        ((1 / pow(2, (double)probability_scale)) * 100);
  }

  /* Check if we broke out of the while loop above by hitting one of our
   * constraints. If so, probability_scale is one less than where we left off.
   * Else, it is what it is at.
   */
  if (iter > 255 || drop_probability_with_scaling <= max_drop_probability) {
    return probability_scale - 1;
  }

  return probability_scale;
}

pipe_status_t pipe_mgr_meter_tof_encode_wred_spec(bf_dev_id_t device_id,
                                                  pipe_wred_spec_t *wred_spec,
                                                  rmt_ram_line_t *ram_line) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_lpf_red_tof_entry_t lpf_red_entry;
  double time_constant_in_cycles = 0;
  int exponent = 0;
  int mantissa = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(device_id);
  PIPE_MGR_MEMSET(&lpf_red_entry, 0, sizeof(pipe_mgr_lpf_red_tof_entry_t));

  /* Do some sanity checks on the WRED spec */
  if (wred_spec->max_probability > 1) {
    LOG_ERROR(
        "%s:%d Invalid WRED spec with max drop probability greather than 1",
        __func__,
        __LINE__);
    return PIPE_INVALID_ARG;
  }
  /* WRED entry also consists of a time constant just like LPF */
  time_constant_in_cycles =
      (wred_spec->time_constant * clock_speed) / 1000000000.0;
  pipe_mgr_lpf_get_exponent_mantissa(
      time_constant_in_cycles, &exponent, &mantissa);
  if (exponent == -1 || mantissa == -1) {
    LOG_ERROR("%s:%d Error in encoding time constant value of %f",
              __func__,
              __LINE__,
              wred_spec->time_constant);
    return PIPE_NO_SYS_RESOURCES;
  }
  lpf_red_entry.gain_time_constant_exponent = exponent;
  lpf_red_entry.decay_time_constant_exponent = exponent;
  lpf_red_entry.time_constant_mantissa = mantissa;

  status = pipe_mgr_meter_compute_wred_params(&lpf_red_entry, wred_spec);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Error in computing WRED parameters, err %s",
              __func__,
              __LINE__,
              pipe_str_err(status));
    return status;
  }
  pipe_mgr_lpf_red_encode_entry(ram_line, lpf_red_entry);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tof_encode_max_meter_spec(
    rmt_ram_line_t *ram_line) {
  pipe_mgr_meter_tof_entry_t tof_meter_entry;

  PIPE_MGR_MEMSET(&tof_meter_entry, 0, sizeof(pipe_mgr_meter_tof_entry_t));

  pipe_mgr_meter_drv_get_max_rate_params(&tof_meter_entry.cir_exponent,
                                         &tof_meter_entry.cir_mantissa);

  pipe_mgr_meter_drv_get_max_rate_params(&tof_meter_entry.pir_exponent,
                                         &tof_meter_entry.pir_mantissa);

  pipe_mgr_meter_drv_get_max_burstsize_params(&tof_meter_entry.cbs_mantissa,
                                              &tof_meter_entry.cbs_exponent);

  pipe_mgr_meter_drv_get_max_burstsize_params(&tof_meter_entry.pbs_mantissa,
                                              &tof_meter_entry.pbs_exponent);

  tof_meter_entry.committed_level =
      (tof_meter_entry.cbs_mantissa << (tof_meter_entry.cbs_exponent - 14));
  tof_meter_entry.peak_level =
      (tof_meter_entry.pbs_mantissa << (tof_meter_entry.pbs_exponent - 14));

  pipe_mgr_meter_encode_meter_entry(ram_line, tof_meter_entry);

  return PIPE_SUCCESS;
}

rmt_virt_addr_t pipe_mgr_meter_compute_ent_virt_addr(vpn_id_t vpn,
                                                     uint32_t ram_line_num) {
  /* Tofino's meter Virtual address format
   * ----------------------------------------
   * |5b of zero| 6b of VPN| 10b of Ram line|
   * ----------------------------------------
   */
  rmt_virt_addr_t virt_addr = 0;

  virt_addr |= (((vpn & 0x3f) << 10) | ((ram_line_num & 0x3ff)));

  return virt_addr;
}

void pipe_mgr_meter_drv_get_max_burstsize_params(uint32_t *burst_mantissa,
                                                 uint32_t *burst_exponent) {
  *burst_mantissa = 255;
  *burst_exponent = 31;
}

void pipe_mgr_meter_drv_get_max_rate_params(uint32_t *exponent,
                                            uint32_t *mantissa) {
  *exponent = 31;
  *mantissa = 511;

  return;
}

void pipe_mgr_lpf_get_exponent_mantissa(double decay_time_constant_in_cycles,
                                        int *exponent,
                                        int *mantissa) {
  double iter = decay_time_constant_in_cycles;
  (*exponent) = 0;
  (*mantissa) = 0;

  while (iter >= 2.0) {
    (*exponent)++;
    iter = iter / 2;
  }
  iter = (iter - 1.0) * 512;
  *mantissa = (int)iter;

  return;
}

pipe_status_t pipe_mgr_meter_ent_read_drv_workflow(
    rmt_dev_info_t *dev_info,
    pipe_tbl_dir_t gress,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    uint8_t ltbl_id,
    vpn_id_t vpn,
    mem_id_t ram_id,
    uint32_t ram_line_num,
    pipe_meter_rate_type_e meter_rate_type,
    pipe_meter_spec_t *meter_spec,
    bool from_hw) {
  bf_dev_id_t device_id = dev_info->dev_id;
  pipe_status_t sts = PIPE_SUCCESS;
  rmt_ram_line_t ram_line;
  pipe_full_virt_addr_t full_virt_addr;
  pipe_mgr_meter_tof_entry_t tof_meter_entry = {0};
  uint64_t ind_addr = 0;
  rmt_virt_addr_t ent_virt_addr = 0;
  uint64_t data[2] = {0, 0};
  float bytes_per_cycle = 0;
  uint64_t bytes_per_second = 0;
  float packets_per_cycle = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(device_id);

  if (from_hw) {
    bf_dev_pipe_t phy_pipe;

    sts = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "%s:%d Error in reading memory for meter spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
    ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);
    construct_full_virt_addr(dev_info,
                             &full_virt_addr,
                             ltbl_id,
                             pipe_virt_mem_type_meter,
                             ent_virt_addr,
                             phy_pipe,
                             stage_id);
    ind_addr = full_virt_addr.addr;

    lld_subdev_ind_read(device_id, subdev, ind_addr, &data[1], &data[0]);
    pipe_mgr_meter_decode_hw_meter_entry(&data[0], &data[1], &tof_meter_entry);
  } else {
    sts = pipe_mgr_phy_mem_map_read(device_id,
                                    gress,
                                    pipe_id,
                                    stage_id,
                                    pipe_mem_type_unit_ram,
                                    ram_id,
                                    ram_line_num,
                                    (uint8_t *)ram_line,
                                    sizeof(rmt_ram_line_t));
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in reading shadow for meter spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }

    pipe_mgr_meter_decode_meter_entry(&ram_line, &tof_meter_entry);
  }

  uint64_t cburst = ((uint64_t)tof_meter_entry.cbs_mantissa *
                     pow(2, (double)tof_meter_entry.cbs_exponent));
  uint64_t pburst = ((uint64_t)tof_meter_entry.pbs_mantissa *
                     pow(2, (double)tof_meter_entry.pbs_exponent));

  if (meter_rate_type == METER_RATE_TYPE_KBPS) {
    meter_spec->cburst = (cburst * 8 + 999) / 1000;
    meter_spec->pburst = (pburst * 8 + 999) / 1000;

    /* CIR */
    bytes_per_cycle = tof_meter_entry.cir_mantissa /
                      (pow(2, (double)(31 - tof_meter_entry.cir_exponent)));
    bytes_per_second = bytes_per_cycle * clock_speed;
    meter_spec->cir.value.kbps = (bytes_per_second * 8 + 999) / 1000;
    /* PIR */
    bytes_per_cycle = tof_meter_entry.pir_mantissa /
                      (pow(2, (double)(31 - tof_meter_entry.pir_exponent)));
    bytes_per_second = bytes_per_cycle * clock_speed;
    meter_spec->pir.value.kbps = (bytes_per_second * 8 + 999) / 1000;
  } else if (meter_rate_type == METER_RATE_TYPE_PPS) {
    meter_spec->cburst = cburst;
    meter_spec->pburst = pburst;

    /* CIR */
    packets_per_cycle = tof_meter_entry.cir_mantissa /
                        (pow(2, (double)(31 - tof_meter_entry.cir_exponent)));
    meter_spec->cir.value.pps = packets_per_cycle * clock_speed;
    /* PIR */
    packets_per_cycle = tof_meter_entry.pir_mantissa /
                        (pow(2, (double)(31 - tof_meter_entry.pir_exponent)));
    meter_spec->pir.value.pps = packets_per_cycle * clock_speed;
  }

  meter_spec->meter_type = tof_meter_entry.u.color_aware
                               ? METER_TYPE_COLOR_AWARE
                               : METER_TYPE_COLOR_UNAWARE;

  return sts;
}

pipe_status_t pipe_mgr_lpf_ent_read_drv_workflow(rmt_dev_info_t *dev_info,
                                                 pipe_tbl_dir_t gress,
                                                 bf_dev_pipe_t pipe_id,
                                                 uint8_t stage_id,
                                                 uint8_t ltbl_id,
                                                 vpn_id_t vpn,
                                                 mem_id_t ram_id,
                                                 uint32_t ram_line_num,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 bool from_hw) {
  pipe_status_t sts;
  bf_dev_id_t device_id = dev_info->dev_id;
  rmt_ram_line_t ram_line;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));
  if (from_hw) {
    pipe_full_virt_addr_t full_virt_addr;
    rmt_virt_addr_t ent_virt_addr;
    uint64_t ind_addr;
    uint64_t read_data[2] = {0};
    bf_dev_pipe_t phy_pipe;

    sts = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "%s:%d Error in reading memory for lpf spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
    ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);
    construct_full_virt_addr(dev_info,
                             &full_virt_addr,
                             ltbl_id,
                             pipe_virt_mem_type_meter,
                             ent_virt_addr,
                             phy_pipe,
                             stage_id);
    ind_addr = full_virt_addr.addr;
    lld_subdev_ind_read(
        device_id, subdev, ind_addr, &read_data[1], &read_data[0]);
    PIPE_MGR_MEMCPY(ram_line, read_data, sizeof(rmt_ram_line_t));
  } else {
    sts = pipe_mgr_phy_mem_map_read(device_id,
                                    gress,
                                    pipe_id,
                                    stage_id,
                                    pipe_mem_type_unit_ram,
                                    ram_id,
                                    ram_line_num,
                                    (uint8_t *)ram_line,
                                    sizeof(rmt_ram_line_t));
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in reading shadow for lpf spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }
  }

  pipe_mgr_meter_tof_decode_lpf_spec(device_id, lpf_spec, &ram_line);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_wred_ent_read_drv_workflow(rmt_dev_info_t *dev_info,
                                                  pipe_tbl_dir_t gress,
                                                  bf_dev_pipe_t pipe_id,
                                                  uint8_t stage_id,
                                                  uint8_t ltbl_id,
                                                  vpn_id_t vpn,
                                                  mem_id_t ram_id,
                                                  uint32_t ram_line_num,
                                                  pipe_wred_spec_t *wred_spec,
                                                  bool from_hw) {
  bf_dev_id_t device_id = dev_info->dev_id;
  pipe_status_t sts;
  rmt_ram_line_t ram_line;

  PIPE_MGR_MEMSET(ram_line, 0, sizeof(rmt_ram_line_t));
  if (from_hw) {
    pipe_full_virt_addr_t full_virt_addr;
    rmt_virt_addr_t ent_virt_addr;
    uint64_t ind_addr;
    uint64_t read_data[2] = {0};
    bf_dev_pipe_t phy_pipe;

    sts = pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe);
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR(
          "%s:%d Error in reading memory for wred spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe);
    ent_virt_addr = pipe_mgr_meter_compute_ent_virt_addr(vpn, ram_line_num);
    construct_full_virt_addr(dev_info,
                             &full_virt_addr,
                             ltbl_id,
                             pipe_virt_mem_type_meter,
                             ent_virt_addr,
                             phy_pipe,
                             stage_id);
    ind_addr = full_virt_addr.addr;
    lld_subdev_ind_read(
        device_id, subdev, ind_addr, &read_data[1], &read_data[0]);
    PIPE_MGR_MEMCPY(ram_line, read_data, sizeof(rmt_ram_line_t));
  } else {
    sts = pipe_mgr_phy_mem_map_read(device_id,
                                    gress,
                                    pipe_id,
                                    stage_id,
                                    pipe_mem_type_unit_ram,
                                    ram_id,
                                    ram_line_num,
                                    (uint8_t *)ram_line,
                                    sizeof(rmt_ram_line_t));
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in reading shadow for wred spec for dev %d pipe %d "
          "stage id %d lt %d vpn %d ram id %d line %d, err %s",
          __func__,
          __LINE__,
          device_id,
          pipe_id,
          stage_id,
          ltbl_id,
          vpn,
          ram_id,
          ram_line_num,
          pipe_str_err(sts));
      return sts;
    }
  }

  pipe_mgr_meter_tof_decode_wred_spec(device_id, wred_spec, &ram_line);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tof_decode_lpf_spec(bf_dev_id_t device_id,
                                                 pipe_lpf_spec_t *lpf_spec,
                                                 rmt_ram_line_t *ram_line) {
  pipe_mgr_lpf_red_tof_entry_t lpf_entry;
  double gain_time_constant_in_cycles = 0;
  double decay_time_constant_in_cycles = 0;
  double time_constant_in_cycles = 0;
  int exponent = 0;
  int mantissa = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(device_id);

  if (!clock_speed) {
    LOG_ERROR("%s:%d Clock speed for device id %d is 0",
              __func__,
              __LINE__,
              device_id);
    return PIPE_UNEXPECTED;
  }

  PIPE_MGR_MEMSET(&lpf_entry, 0, sizeof(pipe_mgr_lpf_red_tof_entry_t));
  pipe_mgr_lpf_red_decode_entry(ram_line, &lpf_entry);

  /* The time constant specified in the spec is in nano seconds */

  if (lpf_entry.gain_time_constant_exponent ==
      lpf_entry.decay_time_constant_exponent) {
    mantissa = lpf_entry.time_constant_mantissa;
    exponent = lpf_entry.gain_time_constant_exponent;
    time_constant_in_cycles =
        ((mantissa / 512.0) + 1.0) * (pow(2, (double)exponent));

    lpf_spec->time_constant =
        (time_constant_in_cycles * 1000000000.0) / clock_speed;
    lpf_spec->gain_decay_separate_time_constant = false;
  } else {
    mantissa = lpf_entry.time_constant_mantissa;

    exponent = lpf_entry.gain_time_constant_exponent;
    gain_time_constant_in_cycles =
        ((mantissa / 512.0) + 1.0) * (pow(2, (double)exponent));

    exponent = lpf_entry.decay_time_constant_exponent;
    decay_time_constant_in_cycles =
        ((mantissa / 512.0) + 1.0) * (pow(2, (double)exponent));

    lpf_spec->gain_time_constant =
        (gain_time_constant_in_cycles * 1000000000.0) / clock_speed;
    lpf_spec->decay_time_constant =
        (decay_time_constant_in_cycles * 1000000000.0) / clock_speed;
    lpf_spec->gain_decay_separate_time_constant = true;
  }

  lpf_spec->output_scale_down_factor = lpf_entry.lpf_action_scale & 0x1f;
  lpf_spec->lpf_type =
      (lpf_entry.rate_sample_enable) ? LPF_TYPE_RATE : LPF_TYPE_SAMPLE;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tof_decode_wred_spec(bf_dev_id_t device_id,
                                                  pipe_wred_spec_t *wred_spec,
                                                  rmt_ram_line_t *ram_line) {
  pipe_mgr_lpf_red_tof_entry_t wred_entry;

  PIPE_MGR_MEMSET(&wred_entry, 0, sizeof(pipe_mgr_lpf_red_tof_entry_t));
  pipe_mgr_lpf_red_decode_entry(ram_line, &wred_entry);

  double time_constant_in_cycles = 0;
  int exponent = 0;
  int mantissa = 0;
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(device_id);

  if (!clock_speed) {
    LOG_ERROR("%s:%d Clock speed for device id %d is 0",
              __func__,
              __LINE__,
              device_id);
    return PIPE_UNEXPECTED;
  }

  mantissa = wred_entry.time_constant_mantissa;
  exponent = wred_entry.gain_time_constant_exponent;
  time_constant_in_cycles =
      ((mantissa / 512.0) + 1.0) * (pow(2, (double)exponent));

  uint32_t red_dlevel100 = 0;
  uint32_t red_level0 = 0;
  uint32_t red_levelmax = 0;
  uint32_t max_drop_probability = 0;
  float rng_value = 0;
  float slope = 0;

  red_level0 = wred_entry.red_level0 * (1 << wred_entry.red_level_exponent);
  red_dlevel100 =
      wred_entry.red_dlevel100 * (1 << wred_entry.red_level_exponent);
  red_levelmax =
      wred_entry.red_level_max * (1 << wred_entry.red_level_exponent);
  slope = 255.0 / red_dlevel100;
  rng_value = slope * (red_levelmax - red_level0);
  rng_value = rng_value / pow(2, (double)wred_entry.red_probability_scale);
  max_drop_probability = (rng_value * 100.0) / 255.0;

  wred_spec->time_constant =
      (time_constant_in_cycles * 1000000000.0) / clock_speed;
  wred_spec->red_min_threshold = red_level0;
  wred_spec->red_max_threshold = red_levelmax;
  wred_spec->max_probability = max_drop_probability / 100.0;
  if (wred_spec->max_probability > 1) {
    // If the decoded drop probability is > 1, cap it at 1
    wred_spec->max_probability = 1;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_set_bytecount_adjust_drv_workflow(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev,
    pipe_bitmap_t pbm,
    uint32_t *data_db,
    uint8_t stage_id,
    uint8_t alu_id,
    int bytecount) {
  uint32_t addr;
  uint32_t data = 0;
  uint32_t bytecount_uint = (uint32_t)(bytecount & 0x3fff);
  pipe_instr_write_reg_t instr;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  data = *data_db;
  setp_meter_ctl_meter_bytecount_adjust(&data, bytecount_uint);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      addr = offsetof(Tofino,
                      pipes[0]
                          .mau[stage_id]
                          .rams.map_alu.meter_group[alu_id]
                          .meter.meter_ctl);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      addr = offsetof(tof2_reg,
                      pipes[0]
                          .mau[stage_id]
                          .rams.map_alu.meter_group[alu_id]
                          .meter.meter_ctl);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      addr = offsetof(tof3_reg,
                      pipes[0]
                          .mau[stage_id]
                          .rams.map_alu.meter_group[alu_id]
                          .meter.meter_ctl);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  construct_instr_reg_write(dev, &instr, addr, data);
  pipe_status_t sts = pipe_mgr_drv_ilist_add(
      &sess_hdl, dev_info, &pbm, stage_id, (uint8_t *)(&instr), sizeof(instr));
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR(
        "Meter update with ilist fails (%s) dev %d", pipe_str_err(sts), dev);
    return PIPE_COMM_FAIL;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_get_bytecount_adjust_drv_workflow(uint32_t data_db,
                                                               int *bytecount) {
  uint32_t bytecount_uint;
  bytecount_uint = getp_meter_ctl_meter_bytecount_adjust(&data_db);
  if (bytecount != NULL) {
    if ((bytecount_uint & 0x2000) != 0) {
      // negative number
      *bytecount = (int)((0xffffc000) | bytecount_uint);
    } else {
      *bytecount = (int)(bytecount_uint);
    }
    return PIPE_SUCCESS;
  }
  return PIPE_INVALID_ARG;
}

void pipe_mgr_meter_encode_meter_entry(
    rmt_ram_line_t *ram_line, pipe_mgr_meter_tof_entry_t tof_meter_entry) {
  /* CIR stuff */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_CIR_BIT_POS,
                TOF_METER_CIR_MANTISSA_WIDTH,
                tof_meter_entry.cir_mantissa);
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_CIR_BIT_POS + TOF_METER_CIR_MANTISSA_WIDTH,
                TOF_METER_CIR_EXPONENT_WIDTH,
                tof_meter_entry.cir_exponent);

  /* PIR stuff */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_PIR_BIT_POS,
                TOF_METER_PIR_MANTISSA_WIDTH,
                tof_meter_entry.pir_mantissa);
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_PIR_BIT_POS + TOF_METER_PIR_MANTISSA_WIDTH,
                TOF_METER_PIR_EXPONENT_WIDTH,
                tof_meter_entry.pir_exponent);

  /* CBS stuff */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_CBS_BIT_POS,
                TOF_METER_CBS_MANTISSA_WIDTH,
                tof_meter_entry.cbs_mantissa);
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_CBS_BIT_POS + TOF_METER_CBS_MANTISSA_WIDTH,
                TOF_METER_CBS_EXPONENT_WIDTH,
                tof_meter_entry.cbs_exponent);

  /* PBS stuff */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_PBS_BIT_POS,
                TOF_METER_PBS_MANTISSA_WIDTH,
                tof_meter_entry.pbs_mantissa);
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_PBS_BIT_POS + TOF_METER_PBS_MANTISSA_WIDTH,
                TOF_METER_PBS_EXPONENT_WIDTH,
                tof_meter_entry.pbs_exponent);

  /* Committed bucket level */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_COMMITTED_LEVEL_BIT_POS,
                TOF_METER_COMMITTED_LEVEL_BIT_WIDTH,
                tof_meter_entry.committed_level);
  /* Peak bucket level */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_PEAK_LEVEL_BIT_POS,
                TOF_METER_PEAK_LEVEL_BIT_WIDTH,
                tof_meter_entry.peak_level);
  /* Timestamp value which is overloaded to contain a bit to indicate if the
   * meter spec is is color aware or not. This is encoded in the meter spec so
   * we know what kind of meter spec it is when we are constructing the indirect
   * address. All to make meter mgr completely stateless, aside from shadow
   * memory which is handled by the shadow memory management */
  meter_set_val((uint8_t *)ram_line,
                TOF_METER_TIMESTAMP_BIT_POS,
                TOF_METER_TIMESTAMP_BIT_WIDTH,
                tof_meter_entry.u.timestamp);
}

void pipe_mgr_lpf_red_encode_entry(rmt_ram_line_t *ram_line,
                                   pipe_mgr_lpf_red_tof_entry_t lpf_red_entry) {
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RED_LEVEL0_BIT_POS,
                TOF_LPF_RED_RED_LEVEL0_BIT_WIDTH,
                lpf_red_entry.red_level0);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RED_DLEVEL100_BIT_POS,
                TOF_LPF_RED_RED_DLEVEL100_BIT_WIDTH,
                lpf_red_entry.red_dlevel100);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RED_LEVEL_MAX_BIT_POS,
                TOF_LPF_RED_RED_LEVEL_MAX_BIT_WIDTH,
                lpf_red_entry.red_level_max);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_POS,
                TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_WIDTH,
                lpf_red_entry.red_level_exponent);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_POS,
                TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_WIDTH,
                lpf_red_entry.red_probability_scale);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_POS,
                TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_WIDTH,
                lpf_red_entry.decay_time_constant_exponent);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_POS,
                TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_WIDTH,
                lpf_red_entry.gain_time_constant_exponent);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_POS,
                TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_WIDTH,
                lpf_red_entry.time_constant_mantissa);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_LPF_ACTION_SCALE_BIT_POS,
                TOF_LPF_RED_LPF_ACTION_SCALE_BIT_WIDTH,
                lpf_red_entry.lpf_action_scale);
  meter_set_val((uint8_t *)ram_line,
                TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_POS,
                TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_WIDTH,
                lpf_red_entry.rate_sample_enable);

  return;
}

void meter_set_val(uint8_t *dst,
                   uint32_t dst_offset,
                   uint32_t len,
                   uint32_t val) {
  uint8_t *wp;
  // Write pointer
  uint8_t wo;  // Write offset (bit offset within byte pointed to by wp).
  uint8_t wm;  // Write mask (mask of bits in byte pointed to by wp to set).

  wp = dst + dst_offset / 8;
  wo = dst_offset % 8;
  while (len) {
    wm = len < 8 ? (1 << len) - 1 : 0xFF;
    wm = wm << wo;

    *wp = (*wp & ~wm) | ((val << wo) & wm);

    val = val >> (8 - wo);
    ++wp;
    len -= (len < (uint8_t)(8 - wo)) ? len : (uint8_t)(8 - wo);
    wo = 0;
  }
}

static void meter_hw_get_val(uint64_t *data0,
                             uint64_t *data1,
                             uint64_t dst_offset,
                             uint32_t len,
                             uint32_t *val) {
  uint8_t word_idx = 0;
  uint8_t field_offset = 0;
  uint64_t mask = 0;
  uint8_t shift = 0;
  uint32_t num_bits = 0;

  word_idx = dst_offset / 64;
  field_offset = dst_offset % 64;
  num_bits =
      (((unsigned)(64 - field_offset) > len) ? len
                                             : (unsigned)(64 - field_offset));
  shift = 0;
  while (len) {
    mask = ((uint64_t)1 << num_bits) - 1;
    if (word_idx == 0) {
      *val |= (((*data0 >> field_offset) & mask) << shift);
    } else if (word_idx == 1) {
      *val |= (((*data1 >> field_offset) & mask) << shift);
    }
    len -= num_bits;
    field_offset = 0;
    shift = num_bits;
    num_bits = len;
  }
}

void meter_ram_get_val(uint8_t *dst,
                       uint32_t dst_offset,
                       uint32_t len,
                       uint32_t *val) {
  uint8_t *wp;
  // Write pointer
  uint8_t wo;  // Write offset (bit offset within byte pointed to by wp).
  uint8_t wm;  // Write mask (mask of bits in byte pointed to by wp to set).
  uint8_t val_offset = 0;

  wp = dst + dst_offset / 8;
  wo = dst_offset % 8;
  while (len) {
    wm = len < 8 ? (1 << len) - 1 : 0xFF;
    *val |= (((*wp >> wo) & wm) << val_offset);

    ++wp;
    val_offset += (8 - wo);
    len -= (len < (uint8_t)(8 - wo)) ? len : (uint8_t)(8 - wo);
    wo = 0;
  }
}

static void lpf_red_get_val(uint8_t *src,
                            uint64_t src_offset,
                            uint32_t len,
                            uint8_t *val) {
  uint8_t *src_ptr;
  uint8_t field_offset;
  uint8_t mask;
  uint8_t shift;
  uint8_t num_bits;
  uint8_t orig_len = len;
  uint16_t time_constant_mantissa = 0;  // Only non-uint8_t field
  (void)time_constant_mantissa;

  src_ptr = src + src_offset / 8;
  field_offset = src_offset % 8;
  shift = 0;
  while (len) {
    mask = len < 8 ? (1 << len) - 1 : 0xFF;
    mask = mask << field_offset;

    if (field_offset > shift) {
      if (orig_len > 8) {
        time_constant_mantissa = (*src_ptr & mask);
        *(uint16_t *)val |= time_constant_mantissa >> field_offset;
      } else {
        *val |= (*src_ptr & mask) >> field_offset;
      }
    } else {
      if (orig_len > 8) {
        time_constant_mantissa = (*src_ptr & mask);
        *(uint16_t *)val |= time_constant_mantissa << shift;
      } else {
        *val |= (*src_ptr & mask) << shift;
      }
    }

    ++src_ptr;
    num_bits =
        (len < (uint8_t)(8 - field_offset)) ? len : (uint8_t)(8 - field_offset);
    len -= num_bits;
    field_offset = 0;
    shift += num_bits;
  }
}

void pipe_mgr_meter_decode_hw_meter_entry(
    uint64_t *data0,
    uint64_t *data1,
    pipe_mgr_meter_tof_entry_t *tof_meter_entry) {
  /* CIR stuff */
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_CIR_BIT_POS,
                   TOF_METER_CIR_MANTISSA_WIDTH,
                   &tof_meter_entry->cir_mantissa);
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_CIR_BIT_POS + TOF_METER_CIR_MANTISSA_WIDTH,
                   TOF_METER_CIR_EXPONENT_WIDTH,
                   &tof_meter_entry->cir_exponent);

  /* PIR stuff */
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_PIR_BIT_POS,
                   TOF_METER_PIR_MANTISSA_WIDTH,
                   &tof_meter_entry->pir_mantissa);
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_PIR_BIT_POS + TOF_METER_PIR_MANTISSA_WIDTH,
                   TOF_METER_PIR_EXPONENT_WIDTH,
                   &tof_meter_entry->pir_exponent);

  /* CBS stuff */
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_CBS_BIT_POS,
                   TOF_METER_CBS_MANTISSA_WIDTH,
                   &tof_meter_entry->cbs_mantissa);
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_CBS_BIT_POS + TOF_METER_CBS_MANTISSA_WIDTH,
                   TOF_METER_CBS_EXPONENT_WIDTH,
                   &tof_meter_entry->cbs_exponent);

  /* PBS stuff */
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_PBS_BIT_POS,
                   TOF_METER_PBS_MANTISSA_WIDTH,
                   &tof_meter_entry->pbs_mantissa);
  meter_hw_get_val(data0,
                   data1,
                   TOF_METER_PBS_BIT_POS + TOF_METER_PBS_MANTISSA_WIDTH,
                   TOF_METER_PBS_EXPONENT_WIDTH,
                   &tof_meter_entry->pbs_exponent);

  /* Peak bucket level. */
  int32_t pl = (*data1 >> (77 - 64)) & 0x7FFFFF;
  /* It is a signed 23 bit number, sign extend if needed. */
  if (pl & (1 << 22)) pl = (uint32_t)pl | ~0x7FFFFFu;
  tof_meter_entry->peak_level = pl;

  /* Committed bucket level. */
  int32_t cl = ((*data1 & 0x1FFF) << 10) | (*data0 >> 54);
  /* It is a signed 23 bit number, sign extend if needed. */
  if (cl & (1 << 22)) cl = (uint32_t)cl | ~0x7FFFFFu;
  tof_meter_entry->committed_level = cl;

  return;
}

void pipe_mgr_meter_decode_meter_entry(
    rmt_ram_line_t *ram_line, pipe_mgr_meter_tof_entry_t *tof_meter_entry) {
  /* CIR stuff */
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_CIR_BIT_POS,
                    TOF_METER_CIR_MANTISSA_WIDTH,
                    &tof_meter_entry->cir_mantissa);
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_CIR_BIT_POS + TOF_METER_CIR_MANTISSA_WIDTH,
                    TOF_METER_CIR_EXPONENT_WIDTH,
                    &tof_meter_entry->cir_exponent);

  /* PIR stuff */
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_PIR_BIT_POS,
                    TOF_METER_PIR_MANTISSA_WIDTH,
                    &tof_meter_entry->pir_mantissa);
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_PIR_BIT_POS + TOF_METER_PIR_MANTISSA_WIDTH,
                    TOF_METER_PIR_EXPONENT_WIDTH,
                    &tof_meter_entry->pir_exponent);

  /* CBS stuff */
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_CBS_BIT_POS,
                    TOF_METER_CBS_MANTISSA_WIDTH,
                    &tof_meter_entry->cbs_mantissa);
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_CBS_BIT_POS + TOF_METER_CBS_MANTISSA_WIDTH,
                    TOF_METER_CBS_EXPONENT_WIDTH,
                    &tof_meter_entry->cbs_exponent);

  /* PBS stuff */
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_PBS_BIT_POS,
                    TOF_METER_PBS_MANTISSA_WIDTH,
                    &tof_meter_entry->pbs_mantissa);
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_PBS_BIT_POS + TOF_METER_PBS_MANTISSA_WIDTH,
                    TOF_METER_PBS_EXPONENT_WIDTH,
                    &tof_meter_entry->pbs_exponent);

  /* Timestamp (holds color_aware flag) */
  meter_ram_get_val((uint8_t *)ram_line,
                    TOF_METER_TIMESTAMP_BIT_POS,
                    TOF_METER_TIMESTAMP_BIT_WIDTH,
                    &tof_meter_entry->u.timestamp);

  return;
}

void pipe_mgr_lpf_red_decode_entry(
    rmt_ram_line_t *ram_line, pipe_mgr_lpf_red_tof_entry_t *lpf_red_entry) {
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RED_LEVEL0_BIT_POS,
                  TOF_LPF_RED_RED_LEVEL0_BIT_WIDTH,
                  &lpf_red_entry->red_level0);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RED_DLEVEL100_BIT_POS,
                  TOF_LPF_RED_RED_DLEVEL100_BIT_WIDTH,
                  &lpf_red_entry->red_dlevel100);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RED_LEVEL_MAX_BIT_POS,
                  TOF_LPF_RED_RED_LEVEL_MAX_BIT_WIDTH,
                  &lpf_red_entry->red_level_max);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_POS,
                  TOF_LPF_RED_RED_LEVEL_EXPONENT_BIT_WIDTH,
                  &lpf_red_entry->red_level_exponent);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_POS,
                  TOF_LPF_RED_RED_PROBABILITY_SCALE_BIT_WIDTH,
                  &lpf_red_entry->red_probability_scale);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_POS,
                  TOF_LPF_RED_FALL_TIME_CONSTANT_EXPONENT_BIT_WIDTH,
                  &lpf_red_entry->decay_time_constant_exponent);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_POS,
                  TOF_LPF_RED_RISE_TIME_CONSTANT_EXPONENT_BIT_WIDTH,
                  &lpf_red_entry->gain_time_constant_exponent);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_POS,
                  TOF_LPF_RED_TIME_CONSTANT_MANTISSA_BIT_WIDTH,
                  (uint8_t *)(&lpf_red_entry->time_constant_mantissa));
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_LPF_ACTION_SCALE_BIT_POS,
                  TOF_LPF_RED_LPF_ACTION_SCALE_BIT_WIDTH,
                  &lpf_red_entry->lpf_action_scale);
  lpf_red_get_val((uint8_t *)ram_line,
                  TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_POS,
                  TOF_LPF_RED_RATE_SAMPLE_ENABLE_BIT_WIDTH,
                  &lpf_red_entry->rate_sample_enable);

  return;
}
