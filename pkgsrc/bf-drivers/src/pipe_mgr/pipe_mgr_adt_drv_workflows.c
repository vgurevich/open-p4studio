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
 * @file pipe_exm_drv_workflows.c
 * @date
 *
 * Provides driver workflow implementations to be used by the action data table
 * manager for performing various operations on the action data tables.
 *
 */

/* Standard header includes */
#include <stdint.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_adt_mgr_int.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_drv_intf.h"

pipe_status_t pipe_mgr_adt_program_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t entry_idx,
    uint8_t **shadow_ptr_arr,
    mem_id_t *mem_id_arr,
    uint32_t num_ram_units,
    bool update) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  uint32_t ram_idx = 0;
  uint32_t data_size = 0;
  uint32_t num_entries_per_wide_word = 0;
  uint32_t ram_line_num = 0;
  pipe_mem_type_t pipe_mem_type = pipe_mem_type_unit_ram;
  pipe_instr_set_memdata_t adt_write_instr;
  pipe_instr_set_memdata_t *adt_write_instr_p = NULL;
  adt_write_instr_p = &adt_write_instr;
  ram_alloc_info = adt_stage_info->ram_alloc_info;
  num_entries_per_wide_word = ram_alloc_info->num_entries_per_wide_word;
  /* Figure out the ram_line_number in which the entry is programmed into
   * based on the packing format and the logical entry index within the stage.
   */
  ram_line_num =
      ((entry_idx) / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;

  /* When we write to the action data RAM we always write one RAM word
   * worth of data at a time (which is the maximum)
   */
  data_size = TOF_SRAM_UNIT_WIDTH / 8;

  /* Write to the shadow register for updates */
  if (update && (adt->dev_info->dev_family == BF_DEV_FAMILY_TOFINO2 ||
                 adt->dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    pipe_mem_type = pipe_mem_type_shadow_reg;
  }
  for (ram_idx = 0; ram_idx < num_ram_units; ram_idx++) {
    PIPE_MGR_MEMSET(&adt_write_instr, 0, sizeof(pipe_instr_set_memdata_t));

    construct_instr_set_memdata(adt->dev_info,
                                adt_write_instr_p,
                                shadow_ptr_arr[ram_idx],
                                data_size,
                                mem_id_arr[ram_idx],
                                adt->direction,
                                adt_stage_info->stage_id,
                                ram_line_num,
                                pipe_mem_type);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    adt->dev_info,
                                    &adt_tbl_data->pipe_bmp,
                                    adt_stage_info->stage_id,
                                    (uint8_t *)&adt_write_instr,
                                    sizeof(pipe_instr_set_memdata_t));

    if (status != PIPE_SUCCESS) {
      /* This is pretty bad */
      LOG_ERROR(
          "%s : Ilist add failed for action data table entry "
          "add for device id %d",
          __func__,
          adt->dev_id);
      return status;
    } else {
      if (adt_tbl_data->pipe_id == BF_DEV_PIPE_ALL) {
        /* Log the memory write */
        LOG_TRACE(
            "Action Data Write : 128 bits to line num %d, ram id %d"
            " for all pipes, stage id %d, dev %d",
            ram_line_num,
            mem_id_arr[ram_idx],
            adt_stage_info->stage_id,
            adt->dev_id);
      } else {
        LOG_TRACE(
            "Action Data Write : 128 bits to line num %d, ram id %d"
            " for pipe id %d, stage id %d, dev %d",
            ram_line_num,
            mem_id_arr[ram_idx],
            adt_tbl_data->pipe_id,
            adt_stage_info->stage_id,
            adt->dev_id);
      }
    }
  }

  return PIPE_SUCCESS;
}
