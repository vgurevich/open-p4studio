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


#include <errno.h>
#include <time.h>

#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <pipe_mgr/pipe_mgr_drv.h>
#include <target-utils/uCli/ucli.h>
#include <tofino_regs/tofino.h>

#include "perf_util.h"
#include <perf/perf_common_intf.h>
#include <perf/perf_mem_intf.h>
#include "perf_mem.h"

/**
 * @brief Run performance test that will write/read to/from
 * SRAM/TCAM memory, calculate the rate
 *
 * @param dev_id Device id
 * @param mem_type Memory type
 * @param pipes Number of PIPEs
 * @param maus Number of MAUs
 * @param rows Number of ROWs
 * @param cols Number of COLUMNs
 * @param result pointer to struct with results
 * @return ucli_status_t
 */
bf_status_t run_mem_test(bf_dev_id_t dev_id,
                         pipe_mem_type_t mem_type,
                         int pipes,
                         int maus,
                         int rows,
                         int cols,
                         struct test_results *result) {
  bf_status_t status = BF_UNEXPECTED;

  if (!result) {
    LOG_ERROR("%s:%d: No allocated memory for results\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }
  result->status = false;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }

  if (pipes <= 0 || pipes > (int)dev_info->num_active_pipes) {
    LOG_ERROR("%s:%d: Number of PIPEs must be within the range <1..%d>\n",
              __func__,
              __LINE__,
              (int)dev_info->num_active_pipes);
    return BF_INVALID_ARG;
  }
  if (maus <= 0 || maus > (int)dev_info->num_active_mau) {
    LOG_ERROR("%s:%d: Number of MAUs must be within the range <1..%d>\n",
              __func__,
              __LINE__,
              (int)dev_info->num_active_mau);
    return BF_INVALID_ARG;
  }

  int first_row_id = 0, first_col_id = 0;
  int num_rows, num_cols;
  uint32_t mem_width, mem_depth;

  switch (mem_type) {
    case pipe_mem_type_unit_ram:
      first_col_id = 2;
      num_rows = dev_info->dev_cfg.stage_cfg.num_sram_rows;
      num_cols = dev_info->dev_cfg.stage_cfg.num_sram_cols;
      mem_width = dev_info->dev_cfg.stage_cfg.sram_unit_width / 8;
      mem_depth = pipe_mgr_get_sram_unit_depth(dev_id);
      break;
    case pipe_mem_type_tcam:
      num_rows = dev_info->dev_cfg.stage_cfg.num_tcam_rows;
      num_cols = dev_info->dev_cfg.stage_cfg.num_tcam_cols;
      /* The TCAM width is equal 44 bits which needs to be rounded up
       * to 64 bits and packed into word0 (64 bits). Together with key/mask
       * in word1 (64 bits) can be written into memory. */
      mem_width = 16;
      mem_depth = pipe_mgr_get_tcam_unit_depth(dev_id);
      break;
    default:
      LOG_ERROR("%s:%d: Memory type not supported %d\n",
                __func__,
                __LINE__,
                mem_type);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
  }

  if (rows <= 0 || rows > num_rows) {
    LOG_ERROR("%s:%d: Number of ROWs must be within the range <1..%d>\n",
              __func__,
              __LINE__,
              num_rows);
    return BF_INVALID_ARG;
  }
  if (cols <= 0 || cols > num_cols) {
    LOG_ERROR("%s:%d: Number of COLs must be within the range <1..%d>\n",
              __func__,
              __LINE__,
              num_cols);
    return BF_INVALID_ARG;
  }

  rows = first_row_id + rows < num_rows ? first_row_id + rows : num_rows;
  cols = first_col_id + cols < num_cols ? first_col_id + cols : num_cols;

  result->res_int[RES_PIPES] = pipes;
  result->res_int[RES_MAUS] = maus;
  result->res_int[RES_ROWS] = rows - first_row_id;
  result->res_int[RES_COLS] = cols - first_col_id;
  /* Get the CTRL0 register address. */
  uint32_t arb_ctrl0_addr = 0, arb_ctrl0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      arb_ctrl0_addr = offsetof(Tofino, device_select.pbc.pbc_pbus.arb_ctrl0);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      arb_ctrl0_addr = offsetof(tof2_reg, device_select.pbc.pbc_pbus.arb_ctrl0);
      break;
    case BF_DEV_FAMILY_UNKNOWN:
    default:
      LOG_ERROR("%s:%d: Device type not supported %d\n",
                __func__,
                __LINE__,
                dev_info->dev_family);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
  }

  pipe_sess_hdl_t sess_hdl = get_pipe_mgr_ctx()->int_ses_hndl;

  /* Put the MAUs into fast mode. */
  pipe_mgr_set_mem_slow_mode(dev_info, false);
  pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);

  /* Read ctrl0 register */
  lld_read_register(dev_id, arb_ctrl0_addr, &arb_ctrl0);
  uint32_t arb_ctrl0_cache = arb_ctrl0;

  /* Turn off the Read/Write DRs by setting its weight to zero. */
  setp_pbus_arb_ctrl0_wb_req_weight(&arb_ctrl0, 0);
  setp_pbus_arb_ctrl0_rb_req_weight(&arb_ctrl0, 0);
  lld_write_register(dev_id, arb_ctrl0_addr, arb_ctrl0);

  /* Prepare fixed data to write into the memory blocks. */
  uint32_t buf_size = pipe_mgr_drv_buf_size(dev_id, PIPE_MGR_DRV_BUF_BWR);

  size_t wr_mem_size = 0, rd_mem_size = 0;
  size_t mem_size =
      buf_size < mem_width * mem_depth ? buf_size : mem_width * mem_depth;

  uint32_t **mem_data =
      PIPE_MGR_MALLOC(sizeof(uint32_t *) * num_rows * num_cols);
  if (mem_data == NULL) {
    LOG_ERROR("%s:%d: Malloc error\n", __func__, __LINE__);
    status = BF_NO_SYS_RESOURCES;
    bf_sys_dbgchk(0);
    goto cleanup;
  }

  for (int mau_id = 0; mau_id < maus; mau_id++) {
    for (int row_id = first_row_id; row_id < rows; ++row_id) {
      for (int col_id = first_col_id; col_id < cols; ++col_id) {
        uint32_t *mem_data_addr = PIPE_MGR_MALLOC(mem_size);
        if (!mem_data_addr) {
          LOG_ERROR("%s:%d: Malloc error\n", __func__, __LINE__);
          status = BF_NO_SYS_RESOURCES;
          bf_sys_dbgchk(0);
          goto cleanup;
        }

        int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
            mau_id, col_id, row_id, mem_type);
        PIPE_MGR_MEMSET(mem_data_addr, mem_id, mem_size);

        mem_data[row_id * cols + col_id] = mem_data_addr;
      }
    }
  }

  /* Fill the DR with our write data. */
  for (int pipe_id = 0; pipe_id < pipes; pipe_id++) {
    int log_pipe_mask = 1 << pipe_id;
    bf_dev_pipe_t phy_pipe_id;
    if (pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe_id) !=
        PIPE_SUCCESS) {
      LOG_ERROR("%s:%d: Failed to map logical pipe %d to phy pipe\n",
                __func__,
                __LINE__,
                pipe_id);
      status = BF_UNEXPECTED;
      goto cleanup;
    }

    for (int mau_id = 0; mau_id < maus; mau_id++) {
      for (int row_id = first_row_id; row_id < rows; ++row_id) {
        for (int col_id = first_col_id; col_id < cols; ++col_id) {
          int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
              mau_id, col_id, row_id, mem_type);
          uint64_t full_addr = dev_info->dev_cfg.get_full_phy_addr(
              0, phy_pipe_id, mau_id, mem_id, 0, mem_type);

          pipe_mgr_drv_buf_t *buf = pipe_mgr_drv_buf_alloc(
              0, dev_id, mem_size, PIPE_MGR_DRV_BUF_BWR, false);
          if (!buf) {
            LOG_ERROR(
                "%s:%d: Error in allocating drv buffer\n", __func__, __LINE__);
            status = BF_UNEXPECTED;
            bf_sys_dbgchk(0);
            goto cleanup;
          }
          PIPE_MGR_MEMSET(buf->addr, mem_id, mem_size);

          if (pipe_mgr_drv_blk_wr(&sess_hdl,
                                  mem_width,
                                  mem_depth,
                                  1,
                                  full_addr,
                                  log_pipe_mask,
                                  buf) != PIPE_SUCCESS) {
            pipe_mgr_drv_buf_free(buf);
            LOG_ERROR("%s:%d: Write block push for mau-id=%d, row=%d, col=%d\n",
                      __func__,
                      __LINE__,
                      mau_id,
                      row_id,
                      col_id);
            status = BF_UNEXPECTED;
            bf_sys_dbgchk(0);
            goto cleanup;
          }
          wr_mem_size = wr_mem_size + mem_size;
        }
      }
    }
  }

  for (int pipe_id = 0; pipe_id < pipes; pipe_id++) {
    bf_dev_pipe_t phy_pipe_id;
    if (pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe_id, &phy_pipe_id) !=
        PIPE_SUCCESS) {
      LOG_ERROR("%s:%d: Failed to map logical pipe %d to phy pipe\n",
                __func__,
                __LINE__,
                pipe_id);
      status = BF_UNEXPECTED;
      goto cleanup;
    }

    for (int mau_id = 0; mau_id < maus; mau_id++) {
      for (int row_id = first_row_id; row_id < rows; ++row_id) {
        for (int col_id = first_col_id; col_id < cols; ++col_id) {
          int mem_id = dev_info->dev_cfg.mem_id_from_col_row(
              mau_id, col_id, row_id, mem_type);
          uint64_t full_addr = dev_info->dev_cfg.get_full_phy_addr(
              0, phy_pipe_id, mau_id, mem_id, 0, mem_type);

          if (pipe_mgr_drv_blk_rd(&sess_hdl,
                                  dev_id,
                                  mem_width,
                                  mem_depth,
                                  1,
                                  full_addr,
                                  NULL,
                                  NULL) != PIPE_SUCCESS) {
            LOG_ERROR("%s:%d: Read block push for mau-id=%d, row=%d, col=%d\n",
                      __func__,
                      __LINE__,
                      mau_id,
                      row_id,
                      col_id);
            status = BF_UNEXPECTED;
            bf_sys_dbgchk(0);
            goto cleanup;
          }
          rd_mem_size = rd_mem_size + mem_size;
        }
      }
    }
  }

  /* Get start time. */
  struct timespec start, stop;

  /* Turn on the Write DR by setting its weight to one. */
  setp_pbus_arb_ctrl0_wb_req_weight(&arb_ctrl0, 1);

  clock_gettime(CLOCK_MONOTONIC, &start);
  /* Reset the weights so the DR is processed again. */
  lld_write_register(dev_id, arb_ctrl0_addr, arb_ctrl0);
  /* Wait for all operations to complete. */
  pipe_mgr_drv_wr_blk_cmplt_all(sess_hdl, dev_id);
  /* Get end time. */
  clock_gettime(CLOCK_MONOTONIC, &stop);

  /* Turn off the Write DRs by setting its weight to zero. */
  setp_pbus_arb_ctrl0_wb_req_weight(&arb_ctrl0, 0);

  if (!ts_to_mb(start,
                stop,
                wr_mem_size,
                &result->res_double[RES_WRITE_MB],
                &result->res_double[RES_WRITE_US])) {
    LOG_ERROR("%s:%d: Invalid test results\n", __func__, __LINE__);
    status = BF_UNEXPECTED;
    bf_sys_dbgchk(0);
    goto cleanup;
  }

  uint32_t size_rx = lld_dr_used_get(dev_id, lld_dr_cmp_pipe_write_blk);
  if (size_rx != 0) {
    status = BF_UNEXPECTED;
    bf_sys_dbgchk(size_rx == 0);
    goto cleanup;
  }

  /* Turn on the Read DR by setting its weight to one. */
  setp_pbus_arb_ctrl0_rb_req_weight(&arb_ctrl0, 1);

  clock_gettime(CLOCK_MONOTONIC, &start);
  /* Reset the weights so the DR is processed again. */
  lld_write_register(dev_id, arb_ctrl0_addr, arb_ctrl0);
  /* Wait for all operations to complete. */
  pipe_mgr_drv_rd_blk_cmplt_all(sess_hdl, dev_id);
  /* Get end time. */
  clock_gettime(CLOCK_MONOTONIC, &stop);

  if (!ts_to_mb(start,
                stop,
                rd_mem_size,
                &result->res_double[RES_READ_MB],
                &result->res_double[RES_READ_US])) {
    LOG_ERROR("%s:%d: Invalid test results\n", __func__, __LINE__);
    status = BF_UNEXPECTED;
    bf_sys_dbgchk(0);
    goto cleanup;
  }

  result->status = true;
  status = BF_SUCCESS;

cleanup:
  /* Restore original content of the register */
  lld_write_register(dev_id, arb_ctrl0_addr, arb_ctrl0_cache);

  /* Put the MAUs back to slow mode. */
  pipe_mgr_set_mem_slow_mode(dev_info, true);
  pipe_mgr_drv_i_list_cmplt_all(&sess_hdl);

  if (mem_data != NULL) {
    for (int row_id = first_row_id; row_id < rows; ++row_id) {
      for (int col_id = first_col_id; col_id < cols; ++col_id) {
        if (mem_data[row_id * cols + col_id] != NULL) {
          PIPE_MGR_FREE(mem_data[row_id * cols + col_id]);
        }
      }
    }
    PIPE_MGR_FREE(mem_data);
  }

  return status;
}

struct test_description sram_dma_test = {
    .test_name = "sram_dma",
    .description =
        "WARNING: The SRAM DMA (Static RAM Direct Memory Access) test is\n"
        "DISRUPTIVE, and it will overwrite the contents of memory blocks.\n"
        "This test should not be used on an active production system.\n\n"
        "The test measures the speed of an SRAM block read and write\n"
        "operations.\n"
        "It the CLOCK_MONOTONIC POSIX clock to measure the speed of read\n"
        "and write events to a specified SRAM memory block. The memory block\n"
        "is specified within the test parameters (number of pipes, MAUs, rows\n"
        "and columns), which are programmatically confirme to be within\n"
        "acceptable minimum and maximum values for each platform.\n"
        "The SRAM DMA test does not verify accuracy of data.\n"
        "The reported test result value is the ratio of the memory block size\n"
        "to the operation processing time (MB/s).\n",
    .params = {{.name = "pipes", .type = "int", .defaults = "2"},
               {.name = "maus", .type = "int", .defaults = "1"},
               {.name = "rows", .type = "int", .defaults = "1"},
               {.name = "cols", .type = "int", .defaults = "1"},
               // last element
               {.name = ""}},
    .results = {{.header = "pipes", .unit = "[-]", .type = "int"},
                {.header = "maus", .unit = "[-]", .type = "int"},
                {.header = "rows", .unit = "[-]", .type = "int"},
                {.header = "cols", .unit = "[-]", .type = "int"},
                {.header = "write", .unit = "[MB/s]", .type = "double"},
                {.header = "write", .unit = "[us/MB]", .type = "double"},
                {.header = "read", .unit = "[MB/s]", .type = "double"},
                {.header = "read", .unit = "[us/MB]", .type = "double"},
                // last element
                {.header = ""}}};

/**
 * @brief Run performance test that will write/read to/from SRAM memory,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param num_pipes Number of PIPEs
 * @param num_maus Number of MAUs
 * @param num_rows Number of ROWs
 * @param num_cols Number of COLUMNs
 * @return test_results
 */
struct test_results sram_dma(bf_dev_id_t dev_id,
                             int num_pipes,
                             int num_maus,
                             int num_rows,
                             int num_cols) {
  struct test_results raw_results;
  memset(&raw_results, 0, sizeof(raw_results));

  run_mem_test(dev_id,
               pipe_mem_type_unit_ram,
               num_pipes,
               num_maus,
               num_rows,
               num_cols,
               &raw_results);
  return raw_results;
}

struct test_description tcam_dma_test = {
    .test_name = "tcam_dma",
    .description =
        "WARNING: The TCAM DMA (Ternary Content-Addressable Memory Direct\n"
        "Memory Access) test is DISRUPTIVE, and it will overwrite the"
        "contents\n"
        "of memory blocks. This test should not be used on an active\n"
        "production system.\n\n"
        "The test measures the speed of a TCAM block read and write\n"
        "operation.\n"
        "It uses the CLOCK_MONOTONIC POSIX clock to measure the speed of read\n"
        "and write events to a specified TCAM memory block. The memory block\n"
        "is specified within the test parameters (number of pipes,\n"
        "MAUs, rows and columns), which are programmatically confirmed\n"
        "to be within acceptable minimum and maximum values for each "
        "platform.\n"
        "The TCAM DMA test does not verify accuracy of data.\n"
        "The reported test result value is the ratio of the memory block size\n"
        "to the operation processing time (MB/s).\n",
    .params = {{.name = "pipes", .type = "int", .defaults = "2"},
               {.name = "maus", .type = "int", .defaults = "1"},
               {.name = "rows", .type = "int", .defaults = "1"},
               {.name = "cols", .type = "int", .defaults = "1"},
               // last element
               {.name = ""}},
    .results = {{.header = "pipes", .unit = "[-]", .type = "int"},
                {.header = "maus", .unit = "[-]", .type = "int"},
                {.header = "rows", .unit = "[-]", .type = "int"},
                {.header = "cols", .unit = "[-]", .type = "int"},
                {.header = "write", .unit = "[MB/s]", .type = "double"},
                {.header = "write", .unit = "[us/MB]", .type = "double"},
                {.header = "read", .unit = "[MB/s]", .type = "double"},
                {.header = "read", .unit = "[us/MB]", .type = "double"},
                // last element
                {.header = ""}}};

/**
 * @brief Run performance test that will write/read to/from TCAM memory,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param num_pipes Number of PIPEs
 * @param num_maus Number of MAUs
 * @param num_rows Number of ROWs
 * @param num_cols Number of COLUMNs
 * @return test_results
 */
struct test_results tcam_dma(bf_dev_id_t dev_id,
                             int num_pipes,
                             int num_maus,
                             int num_rows,
                             int num_cols) {
  struct test_results raw_results;
  memset(&raw_results, 0, sizeof(raw_results));

  run_mem_test(dev_id,
               pipe_mem_type_tcam,
               num_pipes,
               num_maus,
               num_rows,
               num_cols,
               &raw_results);
  return raw_results;
}
