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


#include "dvm/bf_drv_intf.h"
#include "lld/bf_int_if.h"
#include "lld/lld_interrupt_if.h"
#include "lld.h"
#include "lld_map.h"
#include "lld_dev.h"

/**
 * @file bf_int_if.c
 * \brief Details Interrupt APIs.
 *
 */

/**
 * @addtogroup lld-int-api
 * @{
 * This is a description of some APIs.
 */

/** \brief Bind a callback function to be invoked on interrupt detection
 *
 * \param dev_id   : system-assigned identifier
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param offset   : interrupt register offset (from base)
 * \param fn       : user callback function
 * \param userdata : void *             :
 *
 * \return: BF_SUCCESS     : Callback set successfully
 * \return: BF_INVALID_ARG : offset not found in interrupt tree
 */
bf_status_t bf_bind_interrupt_cb(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 uint32_t offset,
                                 bf_int_cb fn,
                                 void *userdata) {
  int rc;

  rc = lld_int_register_cb(dev_id, subdev_id, offset, fn, userdata);
  // rc == 1 indicates a previous callback fn was overwritten by this one
  if ((rc != 0) && (rc != 1)) {
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

/** \brief Service all pending interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupts successfully serviced
 * \return: BF_INVALID_AR G: invalid dev_id
 */
bf_status_t bf_interrupt_service(bf_dev_id_t dev_id) {
  bool is_tofino3 = lld_dev_is_tof3(dev_id);
  lld_dev_t *dev_p;
  int i;

  for (i = 0; i <= BF_MAX_SUBDEV_COUNT; i++) {
    if (!is_tofino3 && i != 0) continue;
    dev_p = lld_map_subdev_id_to_dev_p(dev_id, i);
    if (dev_p) {
      lld_int_poll(dev_id, i, false);
    }
  }
  return BF_SUCCESS;
}

/** \brief Claim ownership of a device interrupt
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr : logical interrupt number (0-LLD_MAX_INT_NBR)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int_nbr
 */
bf_status_t bf_int_claim(bf_dev_id_t dev_id,
                         bf_subdev_id_t subdev_id,
                         bf_int_nbr_t int_nbr) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  lld_err_t lld_err;

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr > LLD_MAX_INT_NBR) return BF_INVALID_ARG;

  lld_err = lld_int_claim(dev_id, subdev_id, int_nbr);
  return (lld_err == LLD_OK) ? BF_SUCCESS : BF_INVALID_ARG;
}

/** \brief Claim ownership of a range of device interrupts
 *
 * Note: returns at first error or all done
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr_hi : logical interrupt number (0-LLD_MAX_INT_NBR)
 * \param int_nbr_lo : logical interrupt number (0-LLD_MAX_INT_NBR)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int_nbr
 */
bf_status_t bf_int_claim_range(bf_dev_id_t dev_id,
                               bf_subdev_id_t subdev_id,
                               bf_int_nbr_t int_nbr_hi,
                               bf_int_nbr_t int_nbr_lo) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  bf_int_nbr_t int_nbr;

  if (dev_p == NULL) return BF_INVALID_ARG;

  for (int_nbr = int_nbr_lo; int_nbr <= int_nbr_hi; int_nbr++) {
    bf_status_t bf_status;

    bf_status = bf_int_claim(dev_id, subdev_id, int_nbr);
    if (bf_status != BF_SUCCESS) return bf_status;
  }
  return BF_SUCCESS;
}

/** \brief Claim ownership of all device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_all(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_claim_range(dev_id, 0, bf_tof_int_hi, bf_tof_int_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_claim_range(dev_id, 0, bf_tof2_int_hi, bf_tof2_int_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_claim_range(
        dev_id, subdev_id, bf_tof3_int_hi, bf_tof3_int_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Claim ownership of all device host interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_host(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (!dev_p) return BF_INVALID_ARG;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      int_nbr_hi = bf_tof_int_host_hi;
      int_nbr_lo = bf_tof_int_host_lo;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      int_nbr_hi = bf_tof2_int_host_hi;
      int_nbr_lo = bf_tof2_int_host_lo;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      int_nbr_hi = bf_tof3_int_host_hi;
      int_nbr_lo = bf_tof3_int_host_lo;
      break;


    default:
      return BF_INVALID_ARG;
  }
  return bf_int_claim_range(dev_id, subdev_id, int_nbr_hi, int_nbr_lo);
}

/** \brief Claim ownership of all device tbus interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_tbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof_int_tbus_hi, bf_tof_int_tbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof2_int_tbus_hi, bf_tof2_int_tbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_claim_range(
        dev_id, subdev_id, bf_tof3_int_tbus_hi, bf_tof3_int_tbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Claim ownership of all device cbus interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_cbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof_int_cbus_hi, bf_tof_int_cbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof2_int_cbus_hi, bf_tof2_int_cbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_claim_range(
        dev_id, subdev_id, bf_tof3_int_cbus_hi, bf_tof3_int_cbus_lo);
  }






  return BF_INVALID_ARG;
}

static bf_status_t get_pbus_range(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  bf_int_nbr_t *int_nbr_hi,
                                  bf_int_nbr_t *int_nbr_lo) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (!dev_p) return BF_INVALID_ARG;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *int_nbr_hi = bf_tof_int_pbus_hi;
      *int_nbr_lo = bf_tof_int_pbus_lo;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *int_nbr_hi = bf_tof2_int_pbus_hi;
      *int_nbr_lo = bf_tof2_int_pbus_lo;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *int_nbr_hi = bf_tof3_int_pbus_hi;
      *int_nbr_lo = bf_tof3_int_pbus_lo;
      break;







    default:
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

/** \brief Claim ownership of all device pbus interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_pbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (get_pbus_range(dev_id, subdev_id, &int_nbr_hi, &int_nbr_lo))
    return BF_INVALID_ARG;
  return bf_int_claim_range(dev_id, subdev_id, int_nbr_hi, int_nbr_lo);
}

/** \brief Claim ownership of all device mbus interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_claim_mbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof_int_mbus_hi, bf_tof_int_mbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_claim_range(
        dev_id, 0, bf_tof2_int_mbus_hi, bf_tof2_int_mbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_claim_range(
        dev_id, subdev_id, bf_tof3_int_mbus_hi, bf_tof3_int_mbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service a range of device interrupts
 *
 * \param dev_id      : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param int_nbr_hi  : logical interrupt number (0-511)
 * \param int_nbr_lo  : logical interrupt number (0-511)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int nbr
 *
 */
bf_status_t bf_int_svc_range(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_int_nbr_t int_nbr_hi,
                             bf_int_nbr_t int_nbr_lo) {
  uint32_t sh_int_reg, sh_int_bit, bit_fld;
  bf_int_nbr_t int_nbr;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr_lo > int_nbr_hi) return BF_INVALID_ARG;
  if (lld_dev_is_locked(dev_id, subdev_id)) return BF_SUCCESS;

  if (lld_dev_is_tofino(dev_id)) {
    if (int_nbr_hi > bf_tof_int_hi) return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof2(dev_id)) {
    if (int_nbr_hi > bf_tof2_int_hi) return BF_INVALID_ARG;
  }
  if (lld_dev_is_tof3(dev_id)) {
    if (int_nbr_hi > bf_tof3_int_hi) return BF_INVALID_ARG;
  }
  for (int_nbr = int_nbr_lo; int_nbr <= int_nbr_hi; int_nbr++) {
    sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
    sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);
    bit_fld = (1u << sh_int_bit);
    lld_int_svc(dev_id, subdev_id, bit_fld, sh_int_reg);
  }
  return BF_SUCCESS;
}

/** \brief Service all device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param msk   : true=leave masked on exit
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_all(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof_int_hi, bf_tof_int_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof2_int_hi, bf_tof2_int_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_svc_range(dev_id, subdev_id, bf_tof3_int_hi, bf_tof3_int_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service host device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_host(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof_int_host_hi, bf_tof_int_host_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_svc_range(
        dev_id, 0, bf_tof2_int_host_hi, bf_tof2_int_host_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_svc_range(
        dev_id, subdev_id, bf_tof3_int_host_hi, bf_tof3_int_host_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service tbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_tbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof_int_tbus_hi, bf_tof_int_tbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_svc_range(
        dev_id, 0, bf_tof2_int_tbus_hi, bf_tof2_int_tbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_svc_range(
        dev_id, subdev_id, bf_tof3_int_tbus_hi, bf_tof3_int_tbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service cbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_cbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof_int_cbus_hi, bf_tof_int_cbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_svc_range(
        dev_id, 0, bf_tof2_int_cbus_hi, bf_tof2_int_cbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_svc_range(
        dev_id, subdev_id, bf_tof3_int_cbus_hi, bf_tof3_int_cbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service pbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_pbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (get_pbus_range(dev_id, subdev_id, &int_nbr_hi, &int_nbr_lo))
    return BF_INVALID_ARG;
  return bf_int_svc_range(dev_id, subdev_id, int_nbr_hi, int_nbr_lo);
}

/** \brief Service mbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_svc_mbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_svc_range(dev_id, 0, bf_tof_int_mbus_hi, bf_tof_int_mbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_svc_range(
        dev_id, 0, bf_tof2_int_mbus_hi, bf_tof2_int_mbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_svc_range(
        dev_id, subdev_id, bf_tof3_int_mbus_hi, bf_tof3_int_mbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Enable (un-mask) a device interrupt
 *
 * \param dev_id : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr: logical interrupt number (0-511)
 *
 * \return: BF_SUCCESS  : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int_nbr
 *
 */
bf_status_t bf_int_ena(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       bf_int_nbr_t int_nbr) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  lld_err_t lld_err;

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr > LLD_MAX_INT_NBR) return BF_INVALID_ARG;
  if (lld_dev_is_locked(dev_id, subdev_id)) return BF_SUCCESS;

  lld_err = lld_int_ena(dev_id, subdev_id, int_nbr);
  return (lld_err == LLD_OK) ? BF_SUCCESS : BF_INVALID_ARG;
}

/** \brief Enable a range of device interrupts
 *
 * Note: returns on first error or all done
 *
 * \param dev_id     : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr_hi : logical interrupt number (0-511)
 * \param int_nbr_lo : logical interrupt number (0-511)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int_nbr
 */
bf_status_t bf_int_ena_range(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_int_nbr_t int_nbr_hi,
                             bf_int_nbr_t int_nbr_lo) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  bf_int_nbr_t int_nbr;

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr_lo > int_nbr_hi) return BF_INVALID_ARG;
  if (lld_dev_is_locked(dev_id, subdev_id)) return BF_SUCCESS;

  if (lld_dev_is_tofino(dev_id)) {
    if (int_nbr_hi > bf_tof_int_hi) return BF_INVALID_ARG;
  } else if (lld_dev_is_tof2(dev_id)) {
    if (int_nbr_hi > bf_tof2_int_hi) return BF_INVALID_ARG;
  } else if (lld_dev_is_tof3(dev_id)) {
    if (int_nbr_hi > bf_tof3_int_hi) return BF_INVALID_ARG;
  }

  for (int_nbr = int_nbr_lo; int_nbr <= int_nbr_hi; int_nbr++) {
    bf_status_t bf_status;

    bf_status = bf_int_ena(dev_id, subdev_id, int_nbr);
    if (bf_status != BF_SUCCESS) return bf_status;
  }
  return BF_SUCCESS;
}

/** \brief Enable all device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_all(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_ena_range(dev_id, 0, bf_tof_int_hi, bf_tof_int_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_ena_range(dev_id, 0, bf_tof2_int_hi, bf_tof2_int_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_ena_range(dev_id, subdev_id, bf_tof3_int_hi, bf_tof3_int_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Enable host device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_host(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (!dev_p) return BF_INVALID_ARG;

  switch (dev_p->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      int_nbr_hi = bf_tof_int_host_hi;
      int_nbr_lo = bf_tof_int_host_lo;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      int_nbr_hi = bf_tof2_int_host_hi;
      int_nbr_lo = bf_tof2_int_host_lo;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      int_nbr_hi = bf_tof3_int_host_hi;
      int_nbr_lo = bf_tof3_int_host_lo;
      break;


    default:
      return BF_INVALID_ARG;
  }
  return bf_int_ena_range(dev_id, subdev_id, int_nbr_hi, int_nbr_lo);
}

/** \brief Enable tbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_tbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_ena_range(dev_id, 0, bf_tof_int_tbus_hi, bf_tof_int_tbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_ena_range(
        dev_id, 0, bf_tof2_int_tbus_hi, bf_tof2_int_tbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_ena_range(
        dev_id, subdev_id, bf_tof3_int_tbus_hi, bf_tof3_int_tbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Enable cbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_cbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_ena_range(dev_id, 0, bf_tof_int_cbus_hi, bf_tof_int_cbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_ena_range(
        dev_id, 0, bf_tof2_int_cbus_hi, bf_tof2_int_cbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_ena_range(
        dev_id, subdev_id, bf_tof3_int_cbus_hi, bf_tof3_int_cbus_lo);
  }






  return BF_INVALID_ARG;
}

/** \brief Enable pbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS  : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_pbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (get_pbus_range(dev_id, subdev_id, &int_nbr_hi, &int_nbr_lo))
    return BF_INVALID_ARG;
  return bf_int_ena_range(dev_id, subdev_id, int_nbr_hi, int_nbr_lo);
}

/** \brief Enable mbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_ena_mbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_ena_range(dev_id, 0, bf_tof_int_mbus_hi, bf_tof_int_mbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_ena_range(
        dev_id, 0, bf_tof2_int_mbus_hi, bf_tof2_int_mbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_ena_range(
        dev_id, subdev_id, bf_tof3_int_mbus_hi, bf_tof3_int_mbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Mask (disable) a specific device interrupt
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr : logical interrupt number (0-511)
 *
 * \return: BF_SUCCESS  : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int nbr
 *
 */
bf_status_t bf_int_msk(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       bf_int_nbr_t int_nbr) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  lld_err_t lld_err;

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr > LLD_MAX_INT_NBR) return BF_INVALID_ARG;
  if (lld_dev_is_locked(dev_id, subdev_id)) return BF_SUCCESS;

  lld_err = lld_int_msk(dev_id, subdev_id, int_nbr);
  return (lld_err == LLD_OK) ? BF_SUCCESS : BF_INVALID_ARG;
}

/** \brief Mask (disable) a range of device interrupts
 *
 * Note: Returns on first error or all done
 *
 * \param dev_id     : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 * \param int_nbr_hi : logical interrupt number (0-511)
 * \param int_nbr_lo : logical interrupt number (0-511)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid int nbr
 *
 */
bf_status_t bf_int_msk_range(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_int_nbr_t int_nbr_hi,
                             bf_int_nbr_t int_nbr_lo) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  bf_int_nbr_t int_nbr;

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (int_nbr_hi > bf_tof_int_hi) return BF_INVALID_ARG;
  if (int_nbr_lo > int_nbr_hi) return BF_INVALID_ARG;
  if (lld_dev_is_locked(dev_id, subdev_id)) return BF_SUCCESS;

  for (int_nbr = int_nbr_lo; int_nbr <= int_nbr_hi; int_nbr++) {
    bf_status_t bf_status;

    bf_status = bf_int_msk(dev_id, subdev_id, int_nbr);
    if (bf_status != BF_SUCCESS) return bf_status;
  }
  return BF_SUCCESS;
}

/** \brief Mask (disable) all device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) enabled successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_all(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof_int_hi, bf_tof_int_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof2_int_hi, bf_tof2_int_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_msk_range(dev_id, subdev_id, bf_tof3_int_hi, bf_tof3_int_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Mask (disable) host device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_host(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof_int_host_hi, bf_tof_int_host_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_msk_range(
        dev_id, 0, bf_tof2_int_host_hi, bf_tof2_int_host_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_msk_range(
        dev_id, subdev_id, bf_tof3_int_host_hi, bf_tof3_int_host_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Mask (disable) tbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_tbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof_int_tbus_hi, bf_tof_int_tbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_msk_range(
        dev_id, 0, bf_tof2_int_tbus_hi, bf_tof2_int_tbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_msk_range(
        dev_id, subdev_id, bf_tof3_int_tbus_hi, bf_tof3_int_tbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Mask (disable) cbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_cbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof_int_cbus_hi, bf_tof_int_cbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_msk_range(
        dev_id, 0, bf_tof2_int_cbus_hi, bf_tof2_int_cbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_msk_range(
        dev_id, subdev_id, bf_tof3_int_cbus_hi, bf_tof3_int_cbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Mask (disable) pbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_pbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  bf_int_nbr_t int_nbr_hi, int_nbr_lo;
  if (get_pbus_range(dev_id, subdev_id, &int_nbr_hi, &int_nbr_lo))
    return BF_INVALID_ARG;
  return bf_int_msk_range(dev_id, 0, int_nbr_hi, int_nbr_lo);
}

/** \brief Mask (disable) mbus device interrupts
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id  : subdevice within dev_id (0..BF_MAX_SUBDEV_COUNT-1)
 *
 * \return: BF_SUCCESS     : interrupt(s) masked successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_msk_mbus(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (lld_dev_is_tofino(dev_id)) {
    return bf_int_msk_range(dev_id, 0, bf_tof_int_mbus_hi, bf_tof_int_mbus_lo);
  }
  if (lld_dev_is_tof2(dev_id)) {
    return bf_int_msk_range(
        dev_id, 0, bf_tof2_int_mbus_hi, bf_tof2_int_mbus_lo);
  }
  if (lld_dev_is_tof3(dev_id)) {
    return bf_int_msk_range(
        dev_id, subdev_id, bf_tof3_int_mbus_hi, bf_tof3_int_mbus_lo);
  }
  return BF_INVALID_ARG;
}

/** \brief Service an MSI-x interrupt.
 *
 * \param dev_id   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param msi_x_int: MSI-x vector number (0-511)
 * \param msk      : true=leave masked on exit
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid msi_x int nbr
 *
 */
bf_status_t bf_int_msi_x_svc(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_msi_x_int_nbr_t msi_x_int,
                             bool msk) {
  bf_int_nbr_t int_nbr = msi_x_int & ((1 << 9) - 1);
  uint32_t sh_int_reg = lld_map_int_nbr_to_sh_int_reg(int_nbr);
  uint32_t sh_int_bit = lld_map_int_nbr_to_sh_int_bit(int_nbr);
  uint32_t bit_fld = (1u << sh_int_bit);

  if (msk) return 1;

  lld_int_svc(dev_id, subdev_id, bit_fld, sh_int_reg);
  return BF_SUCCESS;
}

/** \brief Set MSI-x map.
 * maps top level interrupt <int_nbr> to a MSIX index
 * \param dev_id   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param int_nbr : logical interrupt number (0-511)
 * \param msix_num: MSI-x vector number (0-32) Tofino-2 only
 *
 * \return: BF_SUCCESS     : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid msi_x int nbr
 *
 */
bf_status_t bf_int_msix_map_set(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_int_nbr_t int_nbr,
                                int msix_num) {
  return (lld_int_msix_map_set(dev_id, subdev_id, int_nbr, msix_num));
}

/** \brief Set MSI-x map view.
 * initialize a copy of msix map -- top level interrupt <int_nbr> to MSIX index
 * this map  would eventually get applied to device msix_map during device_add
 * \param dev_id   : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param irq : logical interrupt number (0-511)
 * \param msix: MSI-x vector number (0-31) Tofino-2 and later family only
 *
 * \return: BF_SUCCESS     : interrupt(s) mapped successfully
 * \return: BF_INIT_ERROR : invalid dev_id or other initialization error
 * \return: BF_INVALID_ARG : invalid msix or irq
 *
 */
bf_status_t bf_msix_map_view_init(bf_dev_id_t dev_id, int irq, int msix) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p_allow_unassigned(dev_id);

  if (irq >= LLD_TOFINO_NUM_IRQ || msix >= LLD_TOF2_MSIX_MAX) {
    return BF_INVALID_ARG;
  }
  if (dev_p) {
    dev_p->irq_to_msix[irq] = msix;
    return BF_SUCCESS;
  }
  return BF_INIT_ERROR;
}

/** \brief Service any interrupt.
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param msk     : true=leave masked on exit
 *
 * \return: BF_SUCCESS  : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid msi int nbr
 *
 * The function is not thread-safe and should be invoked in one assigned thread
 * only for the current process.
 *
 *        bus : h t c c p p p p m m m m m m m m
 * glb_sh_int : | | | | | | | | | | | | | | | |
 *
 * Reading glb_sh_int gives which of the above 16 bits are set. If more than
 * 1 bit is set, we loop through all the bits and process each of them.
 * Next read the array of 16 shadow_int 32-bit registers to find which of
 * leaf bit is set. If more, than 1 bit is set, they are all individually
 * processed inside the call to lld_int_svc()

 * In MSI mode the ASIC will not generate a second interrupt over PCIe until
 * all status bits within the MSI vector have been cleared.  Since
 * interrupts may trigger at any time it is possible a few bits early in the
 * 512 bits suddenly set as we finish processing the bits at the end of the
 * 512 bit array. Left in this state the ASIC would not generate a second
 * MSI interrupt over PCIe because the status of the first interrupt delivery
 * is never fully cleared. To ensure this does not happen we mask all
 * the interrupts, process the delivered interrupts and then unmask
 * all the interrupts to ensure there is a transition to a "interrupts fully
 * cleared" state.
 *
 */
bf_status_t bf_int_svc_common(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              bool msk) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  uint32_t glb_ints;
  uint16_t i = 0;
  uint32_t sh_msk_vals[LLD_TOF_TOF2_TOF3_SHADOW_REG_NUMB];

  if (dev_p == NULL) return BF_INVALID_ARG;

  // get overview of shadow regs with pending ints
  if (!lld_dev_is_tof3(dev_id)) {
    subdev_id = 0;
  }
  glb_ints = lld_int_get_glb_status(dev_id, subdev_id);
  if (glb_ints == 0) return BF_SUCCESS;

  if (lld_dev_is_tofino(dev_id) || lld_dev_is_tof2(dev_id) ||
      lld_dev_is_tof3(dev_id)) {
    for (i = 0; i < LLD_TOF_TOF2_TOF3_SHADOW_REG_NUMB; i++) {
      sh_msk_vals[i] = lld_int_get_shadow_msk_status(dev_id, subdev_id, i);
      lld_int_set_shadow_msk_status(dev_id, subdev_id, i, 0xffffffff);
    }
    for (i = 0; i < LLD_TOF_TOF2_TOF3_SHADOW_REG_NUMB; i++) {
      if (glb_ints & (1u << i)) {
        uint32_t sh_int_val =
            lld_int_get_shadow_int_status(dev_id, subdev_id, i);
        if (msk) {
          sh_msk_vals[i] |= sh_int_val;
        }
        lld_int_svc(dev_id, subdev_id, sh_int_val, i);
      }
    }
    for (i = 0; i < LLD_TOF_TOF2_TOF3_SHADOW_REG_NUMB; i++) {
      lld_int_set_shadow_msk_status(dev_id, subdev_id, i, sh_msk_vals[i]);
    }
  }
  return BF_SUCCESS;
}

/** \brief Service an MSI interrupt. Allowable vectors
 *         are 1 (tbus) and 0 (all others)
 *
 * \param dev_id  : system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param msi_int : MSI vector number (0-1)
 * \param msk     : true=leave masked on exit
 *
 * \return: BF_SUCCESS  : interrupt(s) claimed successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 * \return: BF_INVALID_ARG : invalid msi int nbr
 *
 */
bf_status_t bf_int_msi_svc(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_msi_int_nbr_t msi_int,
                           bool msk) {
  lld_dev_t *dev_p = lld_map_dev_id_to_dev_p(dev_id);

  if (dev_p == NULL) return BF_INVALID_ARG;
  if (msi_int > 1) return BF_INVALID_ARG;

  return bf_int_svc_common(dev_id, subdev_id, msk);
}

/** \brief Service a legacy (INTx) interrupt.
 *
 * \param dev_id: system-assigned identifier (0..BF_MAX_DEV_COUNT-1)
 * \param subdev_id: subdevice within dev_id
 * \param msk   : true=leave masked on exit
 *
 * \return: BF_SUCCESS     : interrupt(s) serviced successfully
 * \return: BF_INVALID_ARG : invalid dev_id
 *
 */
bf_status_t bf_int_int_x_svc(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bool msk) {
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);

  if (dev_p == NULL) return BF_INVALID_ARG;

  return bf_int_svc_common(dev_id, subdev_id, msk);
}
