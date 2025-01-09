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


#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include "lld.h"
#include <lld/lld_dr_if.h>
#include <lld/lld_sku.h>

/**
 * @file bf_dma_if.c
 * \brief Details DMA APIs.
 *
 */

/**
 * @addtogroup lld-dma-api
 * @{
 * This is a description of some APIs.
 */

/** \brief Process one or more descriptors from a DR.
 *
 * \param chip    : bf_dev_id_t  : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param dr_id   : bf_dma_dr_id_t     : DR to service
 * \param max_work: int          : Max # of entries to service this call
 *
 * \return: BF_SUCCESS           : DR serviced successfully
 * \return: BF_HW_COMM_FAIL      : No DR serviced
 */
bf_status_t bf_dma_service(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_dma_dr_id_t dr_id,
                           int max_work,
                           int *work_done) {
  int desc_srvcd;

  desc_srvcd = lld_dr_service(dev_id, subdev_id, dr_id, max_work);
  if (desc_srvcd >= 0) {
    if (work_done) {
      *work_done = desc_srvcd;
    }
    return BF_SUCCESS;
  } else if (work_done) {
    *work_done = 0;
  }
  return BF_HW_COMM_FAIL;
}

/** \brief Service the MAC stats DMA Descriptor Ring (DR)
 *
 * \param chip    : bf_dev_id_t  : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param max_work: int          : Max # of entries to service this call
 *
 * \return: BF_SUCCESS           : serviced successfully
 *
 */
bf_status_t bf_dma_service_mac_stats(bf_dev_id_t dev_id, int max_work) {
  uint32_t num_subdev;
  bf_subdev_id_t subdev_id;
  lld_err_t err;

  err = lld_sku_get_num_subdev(dev_id, &num_subdev, NULL);
  if (err != LLD_OK) {
    return BF_HW_COMM_FAIL;
  }
  for (subdev_id = 0; subdev_id < (int)num_subdev; subdev_id++) {
    lld_dr_service(dev_id, subdev_id, lld_dr_cmp_mac_stat, max_work);
  }
  return BF_SUCCESS;
}

bf_status_t bf_dma_service_mac_write_block_completion(bf_dev_id_t dev_id,
                                                      bf_subdev_id_t subdev_id,
                                                      int max_work) {
  lld_dr_service(dev_id, subdev_id, lld_dr_cmp_mac_write_block, max_work);
  return BF_SUCCESS;
}

/** \brief Service the Diagnostic DMA Descriptor Ring (DR)
 *
 * \param chip    : bf_dev_id_t  : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param max_work: int          : Max # of entries to service this call
 *
 * \return: BF_SUCCESS           : serviced successfully
 *
 */
bf_status_t bf_dma_service_diag(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                int max_work) {
  lld_dr_service(dev_id, subdev_id, lld_dr_rx_diag, max_work);

  return BF_SUCCESS;
}

/** \brief bf_dma_start:
 *         Update the DR pointer letting the DRU
 *         know there are descriptors ready.
 *
 * \param chip: int: dev_id #
 * \param bf_dma_dr_id_t: DR to update
 *
 * \return BF_HW_COMM_FAIL= invalid DR specifier
 * \return BF_SUCCESS
 *
 */
bf_status_t bf_dma_start(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bf_dma_dr_id_t dr_id) {
  int err;

  err = lld_dr_start(dev_id, subdev_id, dr_id);
  if (err != 0) {
    return BF_HW_COMM_FAIL;
  }
  return BF_SUCCESS;
}

/** \brief Set or clear pushed pointer mode
 *
 * \param chip: bf_dev_id_t   : dev_id #
 * \param dr  : bf_dma_dr_id_t: DR to update
 * \param mode: bf_dma_ptr_mode_t : pushed-ptr or not
 *
 * \return LLD_OK (0)
 *
 */
bf_status_t bf_dma_dr_ptr_mode_set(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   bf_dma_dr_id_t dr_id,
                                   bf_dma_ptr_mode_t mode) {
  bf_status_t bf_status;
  bool en = (mode == BF_DMA_DR_MODE_PUSHED_PTR) ? true : false;

  bf_status = lld_dr_pushed_ptr_mode_set(dev_id, subdev_id, dr_id, en);
  return bf_status;
}

/** \brief Get the maximum size of a DR pair.
 *
 * \param dev_id    : bf_dev_id_t   : system-assigned identifier
 *(0..BF_MAX_DEV_COUNT-1)
 * \param type     : bf_dma_type_t : Which DR
 * \param tx_depth : int*          : Returned size in the CPU to device
 *direction
 * \param rx_depth : int*          : Returned size in the device to CPU
 *direction
 *
 * \return: BF_SUCCESS           :
 *
 */
bf_status_t bf_dma_dr_get_max_depth(bf_dev_id_t dev_id,
                                    bf_dma_type_t type,
                                    unsigned int *tx_depth,
                                    unsigned int *rx_depth) {
  lld_dr_max_dr_depth_get(dev_id, type, (int *)tx_depth, (int *)rx_depth);

  return BF_SUCCESS;
}

/** \brief Get the amount of DMA'able memory required for a DR.
 *
 * \param dev_fam    : bf_dev_family_t : ASIC Family
 *(0..BF_MAX_DEV_COUNT-1)
 * \param type     : bf_dma_type_t : Which DR
 * \param tx_depth : int           : DR depth in the CPU to device direction
 * \param rx_depth : int           : DR depth in the device to CPU direction
 *
 * \return: The amount of memory required in bytes.
 *
 */
int bf_dma_dr_get_mem_requirement(bf_dev_family_t dev_fam,
                                  bf_dma_type_t type,
                                  int tx_depth,
                                  int rx_depth) {
  return lld_dr_mem_requirement_get(dev_fam, type, tx_depth, rx_depth);
}

/**
 * @}
 */
