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


/**
 * @file lld_fault_hdlr.c
 * \brief Details Fault-handling APIs.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_err.h>
#include "lld.h"
#include <lld/lld_fault.h>
#include <lld/lld_map.h>
#include <lld/lld_bits.h>
#include <lld/lld_dr_if.h>
#include "lld_log.h"
#include "lld_diag_ext.h"
#include "lld_memory_mapping.h"

int default_lld_fault_handler(bf_fault_e fault,
                              uint64_t p1,
                              uint64_t p2,
                              uint64_t p3) {
  printf("FAULT: %d : %016" PRIx64 " : %016" PRIx64 " : %016" PRIx64 "\n",
         fault,
         p1,
         p2,
         p3);
  lld_log("FAULT: %d : %016" PRIx64 " : %016" PRIx64 " : %016" PRIx64 "\n",
          fault,
          p1,
          p2,
          p3);
  return 0;
}

bf_fault_handler_t g_lld_fault_handler = default_lld_fault_handler;

/**
 * @addtogroup lld-fault-api
 * @{
 * This is a description of some APIs.
 */

/** \brief lld_register_fault_handler
 *         Register a user function as the fault handling function.
 *         This same API may be used to revert back to the default
 *         function by specifying "fn" as NULL.
 *
 * \param fn: lld_fault_handler_t : user-defined function
 *
 * \return: LLD_OK
 *
 */
lld_err_t lld_register_fault_handler(bf_fault_handler_t fn) {
  if (fn != NULL) {
    g_lld_fault_handler = fn;
  } else {
    g_lld_fault_handler = default_lld_fault_handler;
  }
  return LLD_OK;
}

/**
 * @}
 */

/** \brief lld_fault_possible_unimplemented_reg
 *         Notify of a possible attempt to read from an unimplemented
 *         PCIe register offset.
 *         If an attempt is made to read an offset that is not mapped
 *         to any functional register Tofino will return 0x0BAD0BAD
 *         for the "register" contents. It is possible, however, that
 *         0x0BAD0BAD is in fact the actual contents of a valid Tofino
 *         register. This function simply logs potential bad accesses.
 *
 * \param dev_id: int      : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param reg:  uint32_t : register offset resulting in 0x0BAD0BAD value
 *
 * \return: nothing
 *
 */
void lld_fault_possible_unimplemented_reg(bf_dev_id_t dev_id, uint32_t reg) {
  static uint32_t last_possible_fault = 0xffffffff;

  if (reg != last_possible_fault) {
    lld_log(
        "Warning: Possible unimplemented register access: dev_id=%d, "
        "offset=%08x",
        dev_id,
        reg);
    last_possible_fault = reg;
  }
}

/** \brief lld_fault_possible_uncorrectable_ecc_u64
 *         Notify of a possible uncorrectable ECC error resulting from
 *         a PCIe register or memory read.
 *         If an uncorrectable ECC error is encountered while reading
 *         a Tofino register Tofino will return 0x0ECC0ECC for the
 *         register contents. It is possible, however, that
 *         0x0ECC0ECC is in fact the actual contents of a Tofino
 *         register.
 *         If this is an ECC error Tofino will set a status bit indicating
 *         as much as well as trigger an interrupt. However, since the
 *         interrupt will not be received immediately, this function can
 *         be used to poll the correspondng status register to determine
 *         if the returned value is due to a real ECC error or just happens
 *         to be the contents of the register (read correctly).
 *
 * \param dev_id  : int      : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param mem64b: uint64_t : offset resulting in 0x0ECC0ECC value
 *
 * \return: nothing
 *
 */
void lld_fault_possible_uncorrectable_ecc_u64(bf_dev_id_t dev_id,
                                              uint64_t mem64b) {
  static uint64_t last_possible_fault = 0xffffffffffffffffull;

  if (mem64b != last_possible_fault) {
    printf(
        "Warning: Possible uncorrectable ECC error: dev_id=%d, addr=%016" PRIx64
        "\n",
        dev_id,
        mem64b);
    lld_log(
        "Warning: Possible uncorrectable ECC error: dev_id=%d, addr=%016" PRIx64
        "",
        dev_id,
        mem64b);
    last_possible_fault = mem64b;
  }
  // check corresponding ecc status
  // if no ecc2 status set, just return
  // else notify user
  // lld_fault_uncorrectable_ecc( dev_id, mem64b );
}

/** \brief lld_fault_uncorrectable_ecc
 *         Notify the user of an uncorrectable ECC error resulting from
 *         a PCIe register or memory read.
 *         If an uncorrectable ECC error is encountered while reading
 *         a Tofino register Tofino will return 0x0ECC0ECC for the
 *         register contents. It will also set a status bit indicating
 *         as much as well as trigger an interrupt.
 *         This function is called any time an uncorrectable ECC error
 *         has been confirmed (either via interrupt or checks in the
 *         register/memory read functions).
 *
 * \param dev_id  : int      : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param mem64b: uint64_t : offset resulting in 0x0ECC0ECC value
 *
 * \return: nothing
 *
 */
void lld_fault_uncorrectable_ecc(bf_dev_id_t dev_id, uint64_t mem64b) {
  printf("LLD: FAULT: Uncorrectable ECC: dev_id=%d, addr=%016" PRIx64 "\n",
         dev_id,
         mem64b);
  lld_log("LLD: FAULT: Uncorrectable ECC: dev_id=%d, addr=%016" PRIx64 "",
          dev_id,
          mem64b);

  g_lld_fault_handler(BF_FAULT_UNCORRECTABLE_ECC, dev_id, mem64b, 0ull);
  // by definition, if above returns fault is non-fatal
}

/** \brief lld_fault_correctable_ecc
 *         Notify the user of a correctable ECC error resulting from
 *         a PCIe register or memory read.
 *         If a correctable ECC error is encountered while reading
 *         a Tofino register Tofino will set a status bit indicating
 *         as much as well as trigger an interrupt.
 *         This function is called any time a correctable ECC error
 *         has been detected (via interrupt).
 *         Certain registers/memories require manual repair by
 *         re-writing the value. That re-write is handled here.
 *         g_lld_fault_handler is also called, in case
 *         the user wishes to track such errors externally.
 *
 * \param dev_id  : int      : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param mem64b: uint64_t : offset resulting in the correctable ECC error
 *
 * \return: nothing
 *
 */
void lld_fault_correctable_ecc(bf_dev_id_t dev_id, uint64_t mem64b) {
  printf("LLD: FAULT: Correctable ECC: dev_id=%d, addr=%016" PRIx64 "\n",
         dev_id,
         mem64b);
  lld_log("LLD: FAULT: Correctable ECC: dev_id=%d, addr=%016" PRIx64 "",
          dev_id,
          mem64b);

  g_lld_fault_handler(BF_FAULT_CORRECTABLE_ECC, dev_id, mem64b, 0ull);
  // by definition, if above returns fault is non-fatal
}

/** \brief lld_fault_dma_error
 *         Notify the user of a data integrity error resulting from
 *         a DMA operation.
 *         If a data integrity error is encountered during a DMA operation
 *         Tofino will set a status bit in the completion descriptor indicating
 *         as much.
 *         This function is called any time a data integrity error
 *         has been detected (via the DMA completion handler).
 *
 * \param dev_id           : int      : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param completion_desc: uint64_t*: pointer to the 2x 64b words containing the
 *                                    completion descriptor reporting the error.
 *The
 *                                    2nd 64b word contains the message-id which
 *(may)
 *                                    be useful in identifying the failed
 *operation.
 *
 * \return: nothing
 *
 */
void lld_fault_dma_error(bf_dev_id_t dev_id, uint64_t *completion_desc) {
  printf("LLD: FAULT: DMA error: dev_id=%d, d0=%016" PRIx64 ", d1=%016" PRIx64
         "\n",
         dev_id,
         *(completion_desc + 0),
         *(completion_desc + 1));
  lld_log("LLD: FAULT: DMA error: dev_id=%d, d0=%016" PRIx64 ", d1=%016" PRIx64
          "",
          dev_id,
          *(completion_desc + 0),
          *(completion_desc + 1));

  g_lld_fault_handler(BF_FAULT_DMA_ERROR,
                      dev_id,
                      *(completion_desc + 0),
                      *(completion_desc + 1));
  // by definition, if above returns fault is non-fatal
}

/** \brief lld_fault_sw_error
 *         Notify the user of an unrecoverable software error
 *
 * \param dev_id: int           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param hint: lld_sw_fault_e: hint as to the nature of the failure
 *
 * \return: nothing
 *
 */
void lld_fault_sw_error(bf_dev_id_t dev_id, bf_sw_fault_e hint) {
  printf("LLD: FAULT: Software error: dev_id=%d, hint=%d\n", dev_id, hint);
  lld_log("LLD: FAULT: Software error: dev_id=%d, hint=%d", dev_id, hint);
  g_lld_fault_handler(BF_FAULT_SW_ERROR, dev_id, hint, 0ull);
  // by definition, if above returns fault is non-fatal
}

// mapping for diag DMA buffers (cli command only)
static int diag_dma_buf_allocd[BF_MAX_DEV_COUNT] = {0};
static uint64_t diag_dma_buf_xlat[BF_MAX_DEV_COUNT] = {0};
static int diag_sample = 0;

static FILE *debug_bus_fd;

void lld_diag_event_cb(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int data_sz,
                       bf_dma_addr_t address) {
  uint64_t hi, lo;
  uint64_t diag_bus_n_data = 0ull;
  int diag_bus_n_data_valid = 0ull;
  bf_status_t rc;
  uint64_t *vaddr_p =
      (uint64_t *)(uintptr_t)((uint64_t)address - diag_dma_buf_xlat[dev_id]);
  (void)subdev_id;
  // printf("DIAG EVENT: dev_id=%d : data_sz=%d : dma address=%016" PRIx64
  // "vaddr=%p\n",
  //       dev_id,
  //       data_sz,
  //       address,
  //       (void*)vaddr_p);

  lo = *(vaddr_p);      //*((uint64_t *)(uintptr_t)address + 0);
  hi = *(vaddr_p + 1);  //*((uint64_t *)(uintptr_t)address + 1);

  fprintf(debug_bus_fd, "-\n");
  fprintf(debug_bus_fd, " sample : %d\n", diag_sample);

  diag_bus_n_data = extract_bit_fld_128(hi, lo, 115, 68);
  fprintf(debug_bus_fd,
          " timestamp : 0x%02" PRIx64 "%02" PRIx64 "%02" PRIx64 "%02" PRIx64
          "%02" PRIx64 "%02" PRIx64 "\n",
          (diag_bus_n_data >> 40ull) & 0xff,
          (diag_bus_n_data >> 32ull) & 0xff,
          (diag_bus_n_data >> 24ull) & 0xff,
          (diag_bus_n_data >> 16ull) & 0xff,
          (diag_bus_n_data >> 8ull) & 0xff,
          (diag_bus_n_data >> 0ull) & 0xff);

  fprintf(debug_bus_fd, " bus_info :\n");

  diag_bus_n_data = extract_bit_fld_128(hi, lo, 15, 0);
  diag_bus_n_data_valid = extract_bit_fld_128(hi, lo, 16, 16);

  fprintf(debug_bus_fd, " -\n");
  fprintf(debug_bus_fd, "  bus0_data : 0x%04" PRIx64 "\n", diag_bus_n_data);
  fprintf(debug_bus_fd, "  bus0_vld : 0x%d\n", diag_bus_n_data_valid);

  diag_bus_n_data = extract_bit_fld_128(hi, lo, 32, 17);
  diag_bus_n_data_valid = extract_bit_fld_128(hi, lo, 33, 33);

  fprintf(debug_bus_fd, " -\n");
  fprintf(debug_bus_fd, "  bus1_data : 0x%04" PRIx64 "\n", diag_bus_n_data);
  fprintf(debug_bus_fd, "  bus1_vld : 0x%d\n", diag_bus_n_data_valid);

  diag_bus_n_data = extract_bit_fld_128(hi, lo, 49, 34);
  diag_bus_n_data_valid = extract_bit_fld_128(hi, lo, 50, 50);

  fprintf(debug_bus_fd, " -\n");
  fprintf(debug_bus_fd, "  bus2_data : 0x%04" PRIx64 "\n", diag_bus_n_data);
  fprintf(debug_bus_fd, "  bus2_vld : 0x%d\n", diag_bus_n_data_valid);

  diag_bus_n_data = extract_bit_fld_128(hi, lo, 66, 51);
  diag_bus_n_data_valid = extract_bit_fld_128(hi, lo, 67, 67);

  fprintf(debug_bus_fd, " -\n");
  fprintf(debug_bus_fd, "  bus3_data : 0x%04" PRIx64 "\n", diag_bus_n_data);
  fprintf(debug_bus_fd, "  bus3_vld : 0x%d\n", diag_bus_n_data_valid);

  fflush(debug_bus_fd);

  // bump sample sequence #
  diag_sample++;

  rc = lld_push_fm(dev_id, lld_dr_fm_diag, address, 256);
  if (rc == 0) {
    lld_dr_start(dev_id, 0, lld_dr_fm_diag);
  } else {
    printf("Error: %d : from lld_push_fm(diag_fm)\n", rc);
  }
  (void)data_sz;
}

/** \brief lld_debug_bus_init
 *         Initialize the debug bus handler
 *
 * \param dev_id: int           : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 *
 * \return: nothing
 *
 */
void lld_debug_bus_init(bf_dev_id_t dev_id) {
  int rc;
  int i;
  static void *vaddr = NULL;
  static uint64_t paddr = 0ull;
  bf_dma_addr_t dma_addr;
  int num_dma_bufs_allocd = 1024;

  lld_log("LLD: Fault handler initializing..: dev_id=%d\n", dev_id);

  debug_bus_fd = fopen("debug_data_dump.yml", "w");
  if (debug_bus_fd == NULL) {
    printf("Error opening debug bus data file\n");
    return;
  }

  if (!diag_dma_buf_allocd[dev_id]) {
    rc = lld_diag_dma_alloc(dev_id, 0, 256, &vaddr, &paddr);
    if (rc != 0) {
      printf("Error: Cant allocate DMA buffer for DIAG\n");
      return;
    }
    // set mapping
    diag_dma_buf_xlat[dev_id] = paddr - (uint64_t)(uintptr_t)vaddr;
    diag_dma_buf_allocd[dev_id] = 1;
  }
  dma_addr = (bf_dma_addr_t)paddr;

  lld_register_rx_diag_callback(dev_id, 0, lld_diag_event_cb);

  for (i = 0; i < num_dma_bufs_allocd; i++) {
    rc = lld_push_fm(dev_id, lld_dr_fm_diag, dma_addr + (256 * i), 256);
    if (rc != 0) {
      lld_log("Error: %d : from lld_push_fm(diag_fm)\n", rc);
    }
  }
  lld_dr_start(dev_id, 0, lld_dr_fm_diag);
}
