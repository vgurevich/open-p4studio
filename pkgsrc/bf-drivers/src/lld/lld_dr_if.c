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
 * @file lld_dr_if.c
 * \brief Details DMA (Desriptor Ring) APIs.
 *
 */

/**
 * @addtogroup lld-dma-api
 * @{
 * This is a description of some APIs.
 */

#ifdef __KERNEL__
#define bf_sys_assert()
#endif

#include <dvm/bf_drv_intf.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/bf_dev_if.h>
#include "lld_memory_mapping.h"
#include <lld/lld_dr_descriptors.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include "lld_memory_mapping.h"
#include "lld.h"
#include "lld_map.h"
#include "lld_dev.h"

// fwd ref
static lld_err_t lld_dr_buf_misaligned(bf_dma_addr_t addr, int requirement);

/** \brief lld_subdev_push_ilist:
 *         Push an instruction list descriptor
 *         into the specified ILIST DR (0-3)
 *
 * \param dev_id  : dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_0_3  : which of the 4 inst-list DRs to use
 * \param msg_id  : opaque message-id, returned in completion callback
 * \param list    : bus address of buffer containing instruction-list
 * \param list_len: length, in bytes, of instruction-list
 * \param rsp_sz  : 4/8/16B, size of each instruction response (if
 *ack_ptr != NULL)
 * \param ack_ptr : bus address of buffer to receive acks, NULL
 *if no response
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invaid chip or subdev_id
 * \return LLD_ERR_BAD_PARM : invalid dr specifier (0-3)
 * \return LLD_ERR_BAD_PARM : NULL list
 * \return LLD_ERR_BAD_PARM : list_len < 0 or > max
 * \return LLD_ERR_BAD_PARM : ack != 0 but ack_ptr NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 *
 */
int lld_subdev_push_ilist(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          int dr_0_3,
                          bf_dma_addr_t list,
                          int list_len,
                          int rsp_sz,
                          bool s_f,
                          bf_dma_addr_t ack_ptr,
                          uint64_t msg_id) {
  lld_dr_view_t *view;
  bf_dma_dr_id_t dr_id;
  int push_failed;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  uint64_t entry_sz = 0ull, list_typ = 0ull;
  (void)subdev_id;

  if (list == 0) return LLD_ERR_BAD_PARM;  // invalid list ptr
  if (lld_dr_buf_misaligned(list, 64))
    return LLD_ERR_BAD_PARM;                               // invalid list ptr
  if (list_len <= 0) return LLD_ERR_BAD_PARM;              // invalid list len
  if (list_len > LLD_MAX_DMA_SZ) return LLD_ERR_BAD_PARM;  // invalid list len
  if (dr_0_3 > 3) return LLD_ERR_BAD_PARM;  // invalid inst-list DR index
  if ((ack_ptr != 0) && ((rsp_sz != 4) && (rsp_sz != 8) && (rsp_sz != 16)))
    return LLD_ERR_BAD_PARM;  // invalid combo
  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  dr_id = lld_dr_tx_pipe_inst_list_0 + dr_0_3;
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  if (ack_ptr != 0) {
    entry_sz = (rsp_sz == 4) ? 0 : (rsp_sz == 8) ? 1 : (rsp_sz == 16) ? 2 : 0;
    list_typ = 1ull;
  }
  if (s_f == 1) {
    if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
      if ((list_len > LLD_MAX_DMA_SZ_SF) || (list_typ != 1))
        return LLD_ERR_BAD_PARM;
      entry_sz |= (s_f << 8);
    }








  }
  format_dr_msg_tx_ilist_wd0(
      desc[0], list_len, list_typ, entry_sz, tx_m_type_il, 1, 1);
  msg->source_address = list;
  msg->destination_address = ack_ptr;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_ilist:
 *         Push an instruction list descriptor
 *         into the specified ILIST DR (0-3)
 *
 * \param dev_id  : dev_id #
 * \param dr_0_3  : which of the 4 inst-list DRs to use
 * \param msg_id  : opaque message-id, returned in completion callback
 * \param list    : bus address of buffer containing instruction-list
 * \param list_len: length, in bytes, of instruction-list
 * \param rsp_sz  : 4/8/16B, size of each instruction response (if
 *ack_ptr != NULL)
 * \param ack_ptr : bus address of buffer to receive acks, NULL
 *if no response
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invaid chip
 * \return LLD_ERR_BAD_PARM : invalid dr specifier (0-3)
 * \return LLD_ERR_BAD_PARM : NULL list
 * \return LLD_ERR_BAD_PARM : list_len < 0 or > max
 * \return LLD_ERR_BAD_PARM : ack != 0 but ack_ptr NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 *
 */
int lld_push_ilist(bf_dev_id_t dev_id,
                   int dr_0_3,
                   bf_dma_addr_t list,
                   int list_len,
                   int rsp_sz,
                   bool s_f,
                   bf_dma_addr_t ack_ptr,
                   uint64_t msg_id) {
  return (lld_subdev_push_ilist(
      dev_id, 0, dr_0_3, list, list_len, rsp_sz, s_f, ack_ptr, msg_id));
}

/** \brief lld_subdev_push_ilist_mcast:
 *         Push a multicast Instruction List descriptor
 *         into the IL DR, only for Tof2
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_0_3  : which of the 4 inst-list DRs to use
 * \param entry_sz : size of each entry in the block
 * \param list_typ: 0=no instruction response, 1=responses for
 *every instruction
 * \param data_sz  : number of writes
 * \param s_f : store and forward enable
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_ilist_mcast(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                int dr_0_3,
                                bf_dma_addr_t list,
                                int list_len,
                                int rsp_sz,
                                bool s_f,
                                uint32_t mcast_vector,
                                bf_dma_addr_t ack_ptr,
                                uint64_t msg_id) {
  lld_dr_view_t *view;
  bf_dma_dr_id_t dr_id;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  uint64_t entry_sz = 0ull, list_typ = 0ull;

  if ((!lld_dev_is_tof2(dev_id)) && (!lld_dev_is_tof3(dev_id)))

      return LLD_ERR_BAD_PARM;    // invalid chip
  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  if (dr_0_3 > 3) return LLD_ERR_BAD_PARM;  // invalid inst-list DR index
  dr_id = lld_dr_tx_pipe_inst_list_0 + dr_0_3;
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  if (ack_ptr != 0) {
    entry_sz = (rsp_sz == 4) ? 0 : (rsp_sz == 8) ? 1 : (rsp_sz == 16) ? 2 : 0;
    list_typ = 1ull;
  }
  attr = list_typ;
  attr |= (entry_sz << 1);
  if (attr > 5) return LLD_ERR_BAD_PARM;                     // bad entry sz
  if (list_len <= 0) return LLD_ERR_BAD_PARM;                // bad DMA len
  if ((list_len) > LLD_MAX_DMA_SZ) return LLD_ERR_BAD_PARM;  // invalid DMA len
  if (list == 0) return LLD_ERR_BAD_PARM;                    // bad buffer ptr
  if (lld_dr_buf_misaligned(list, 64))
    return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(ack_ptr, 64))
    return LLD_ERR_BAD_PARM;                        // bad  address
  if (mcast_vector <= 0) return LLD_ERR_BAD_PARM;   // bad multicast vector
  if (mcast_vector > 0xF) return LLD_ERR_BAD_PARM;  // bad multicast vector

  attr |= (1 << 4);  // mcast enable
  attr |= (mcast_vector << 5);
  if (s_f == 1) {
    if (lld_dev_is_tof2(dev_id) || lld_dev_is_tof3(dev_id)) {
      if ((list_len > LLD_MAX_DMA_SZ_SF) || (list_typ != 0))
        return LLD_ERR_BAD_PARM;
      attr |= s_f << 9;
    }








  }
  format_dr_msg_tx_wd0(desc[0], list_len, attr, tx_m_type_il, 1, 1);

  msg->source_address = list;
  msg->destination_address = ack_ptr;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_ilist_mcast:
 *         Push a multicast Instruction List descriptor
 *         into the IL DR, only for Tof2
 *
 * \param dev_id   : dev_id #
 * \param dr_0_3  : which of the 4 inst-list DRs to use
 * \param entry_sz : size of each entry in the block
 * \param list_typ: 0=no instruction response, 1=responses for
 *every instruction
 * \param data_sz  : number of writes
 * \param s_f : store and forward enable
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_ilist_mcast(bf_dev_id_t dev_id,
                         int dr_0_3,
                         bf_dma_addr_t list,
                         int list_len,
                         int rsp_sz,
                         bool s_f,
                         uint32_t mcast_vector,
                         bf_dma_addr_t ack_ptr,
                         uint64_t msg_id) {
  return (lld_subdev_push_ilist_mcast(dev_id,
                                      0,
                                      dr_0_3,
                                      list,
                                      list_len,
                                      rsp_sz,
                                      s_f,
                                      mcast_vector,
                                      ack_ptr,
                                      msg_id));
}

/** \brief lld_subdev_push_wb:
 *         Push a write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz : size of each entry in the block
 * \param write_typ: 0=register, 1=memory
 * \param data_sz  : number of ENTRIES to write (not bytes)
 * \param source   : bus address of buffer containing data to write
 * \param dest     : starting Tofino address to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_wb(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int entry_sz,
                       uint32_t addr_inc,
                       int data_sz,
                       bool single_entry,
                       bf_dma_addr_t source,
                       uint64_t dest,
                       uint64_t msg_id) {
  bf_dev_family_t dev_fam;
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_pipe_write_block);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if ((data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // invalid DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(source, 64))
    return LLD_ERR_BAD_PARM;               // bad buffer ptr
  if (dest == 0) return LLD_ERR_BAD_PARM;  // bad tofino address

  dev_fam = lld_dev_family_get(dev_id);
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      if (addr_inc != 1 && addr_inc != 4) return LLD_ERR_BAD_PARM;
      /* Increment of 1 is encoded as a 1, increment of 4 is encoded as a 0. */
      attr |= (addr_inc == 1 ? 1 : 0) << 3;
      (void)single_entry;
      break;
    case BF_DEV_FAMILY_TOFINO2:

    case BF_DEV_FAMILY_TOFINO3:
      attr |= (single_entry << 3);
      if (addr_inc == 1) {
        attr |= (0 << 9);
      } else if (addr_inc == 2) {
        attr |= (1 << 9);
      } else if (addr_inc == 4) {
        attr |= (2 << 9);
      } else if (addr_inc == 8) {
        attr |= (3 << 9);
      } else if (addr_inc == 16) {
        attr |= (4 << 9);
      } else if (addr_inc == 32) {
        attr |= (5 << 9);
      } else {
        return LLD_ERR_BAD_PARM;
      }
      break;
    default:
      return LLD_ERR_BAD_PARM;  // bad chip
  }

  format_dr_msg_tx_wd0(
      desc[0], (data_sz * entry_sz), attr, tx_m_type_wr_blk, 1, 1);

  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_wb:
 *         Push a write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param entry_sz : size of each entry in the block
 * \param write_typ: 0=register, 1=memory
 * \param data_sz  : number of ENTRIES to write (not bytes)
 * \param source   : bus address of buffer containing data to write
 * \param dest     : starting Tofino address to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_wb(bf_dev_id_t dev_id,
                int entry_sz,
                uint32_t addr_inc,
                int data_sz,
                bool single_entry,
                bf_dma_addr_t source,
                uint64_t dest,
                uint64_t msg_id) {
  return (lld_subdev_push_wb(dev_id,
                             0,
                             entry_sz,
                             addr_inc,
                             data_sz,
                             single_entry,
                             source,
                             dest,
                             msg_id));
}

/** \brief lld_subdev_push_wb_mcast:
 *         Push a multicast write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz : size of each entry in the block
 * \param write_typ: 0=register, 1=memory
 * \param data_sz  : number of writes
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_wb_mcast(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             int entry_sz,
                             uint32_t addr_inc,
                             int data_sz,
                             bool single_entry,
                             bf_dma_addr_t source,
                             uint64_t dest,
                             uint32_t mcast_vector,
                             uint64_t msg_id) {
  bf_dev_family_t dev_fam;
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_pipe_write_block);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if (!single_entry && (data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // invalid DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(source, 64))
    return LLD_ERR_BAD_PARM;                        // bad buffer ptr
  if (dest == 0) return LLD_ERR_BAD_PARM;           // bad tofino address
  if (mcast_vector <= 0) return LLD_ERR_BAD_PARM;   // bad multicast vector
  if (mcast_vector > 0xF) return LLD_ERR_BAD_PARM;  // bad multicast vector

  dev_fam = lld_dev_family_get(dev_id);
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      if (addr_inc != 1 && addr_inc != 4) return LLD_ERR_BAD_PARM;
      /* Increment of 1 is encoded as a 1, increment of 4 is encoded as a 0. */
      attr |= (addr_inc == 1 ? 1 : 0) << 3;
      attr |= (1 << 4);  // mcast enable
      attr |= (mcast_vector << 5);
      (void)single_entry;
      break;
    case BF_DEV_FAMILY_TOFINO2:

    case BF_DEV_FAMILY_TOFINO3: {
      int stride = -1;
      if (addr_inc == 1)
        stride = 0;
      else if (addr_inc == 2)
        stride = 1;
      else if (addr_inc == 4)
        stride = 2;
      else if (addr_inc == 8)
        stride = 3;
      else if (addr_inc == 16)
        stride = 4;
      else if (addr_inc == 32)
        stride = 5;
      if (stride == -1) return LLD_ERR_BAD_PARM;
      attr |= (single_entry << 3);  // Single-Mode
      attr |= (1 << 4);             // mcast enable
      attr |= (mcast_vector << 5);
      attr |= stride << 9;
      break;
    }
    default:
      return LLD_ERR_BAD_PARM;  // bad chip
  }
  format_dr_msg_tx_wd0(
      desc[0], (data_sz * entry_sz), attr, tx_m_type_wr_blk, 1, 1);

  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_wb_mcast:
 *         Push a multicast write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param entry_sz : size of each entry in the block
 * \param write_typ: 0=register, 1=memory
 * \param data_sz  : number of writes
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_wb_mcast(bf_dev_id_t dev_id,
                      int entry_sz,
                      uint32_t addr_inc,
                      int data_sz,
                      bool single_entry,
                      bf_dma_addr_t source,
                      uint64_t dest,
                      uint32_t mcast_vector,
                      uint64_t msg_id) {
  return (lld_subdev_push_wb_mcast(dev_id,
                                   0,
                                   entry_sz,
                                   addr_inc,
                                   data_sz,
                                   single_entry,
                                   source,
                                   dest,
                                   mcast_vector,
                                   msg_id));
}

/** \brief lld_subdev_push_mac_wb:
 *         Push a mac write-block descriptor
 *         into the WB DR, Tof2 only
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz : size of each entry in the block
 * \param addr_inc : address stride
 * \param data_sz  : number of ENTRIES to write (not bytes)
 * \param single_entry
 * \param source   : bus address of buffer containing data to write
 * \param dest     : starting Tofino address to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_mac_wb(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           int entry_sz,
                           uint32_t addr_inc,
                           int data_sz,
                           bool single_entry,
                           bf_dma_addr_t source,
                           uint64_t dest,
                           uint64_t msg_id) {
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if ((!lld_dev_is_tof2(dev_id)) && (!lld_dev_is_tof3(dev_id)))

      return LLD_ERR_BAD_PARM;    // invalid chip
  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_mac_write_block);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if ((data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // invalid DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(source, 64))
    return LLD_ERR_BAD_PARM;               // bad buffer ptr
  if (dest == 0) return LLD_ERR_BAD_PARM;  // bad tofino address

  attr |= single_entry << 3;  // single entry mode
  if (addr_inc == 1) {
    attr |= 0 << 9;
  } else if (addr_inc == 2) {
    attr |= 1 << 9;
  } else if (addr_inc == 4) {
    attr |= 2 << 9;
  } else if (addr_inc == 8) {
    attr |= 3 << 9;
  } else if (addr_inc == 16) {
    attr |= 4 << 9;
  } else if (addr_inc == 32) {
    attr |= 5 << 9;
  } else {
    return LLD_ERR_BAD_PARM;
  }
  format_dr_msg_tx_wd0(
      desc[0], (data_sz * entry_sz), attr, tx_m_type_mac_wr_blk, 1, 1);

  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_mac_wb:
 *         Push a mac write-block descriptor
 *         into the WB DR, Tof2 only
 *
 * \param dev_id   : dev_id #
 * \param entry_sz : size of each entry in the block
 * \param addr_inc : address stride
 * \param data_sz  : number of ENTRIES to write (not bytes)
 * \param single_entry
 * \param source   : bus address of buffer containing data to write
 * \param dest     : starting Tofino address to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_mac_wb(bf_dev_id_t dev_id,
                    int entry_sz,
                    uint32_t addr_inc,
                    int data_sz,
                    bool single_entry,
                    bf_dma_addr_t source,
                    uint64_t dest,
                    uint64_t msg_id) {
  return (lld_subdev_push_mac_wb(dev_id,
                                 0,
                                 entry_sz,
                                 addr_inc,
                                 data_sz,
                                 single_entry,
                                 source,
                                 dest,
                                 msg_id));
}

/** \brief lld_subdev_push_mac_wb_mcast:
 *         Push a multicast mac write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz : size of each entry in the block
 * \param addr_inc : address stride
 * \param data_sz  : number of writes
 * \param single_entry
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_mac_wb_mcast(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 int entry_sz,
                                 uint32_t addr_inc,
                                 int data_sz,
                                 bool single_entry,
                                 bf_dma_addr_t source,
                                 uint64_t dest,
                                 uint32_t mcast_vector,
                                 uint64_t msg_id) {
  lld_dr_view_t *view;
  dr_msg_tx_t *msg;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  int stride;

  msg = (dr_msg_tx_t *)&desc[0];
  if ((!lld_dev_is_tof2(dev_id)) && (!lld_dev_is_tof3(dev_id)))

      return LLD_ERR_BAD_PARM;    // invalid chip
  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_mac_write_block);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if ((data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // invalid DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(source, 64))
    return LLD_ERR_BAD_PARM;                        // bad buffer ptr
  if (dest == 0) return LLD_ERR_BAD_PARM;           // bad tofino address
  if (mcast_vector <= 0) return LLD_ERR_BAD_PARM;   // bad multicast vector
  if (mcast_vector > 0xF) return LLD_ERR_BAD_PARM;  // bad multicast vector

  if (addr_inc == 1) {
    stride = 0;
  } else if (addr_inc == 2) {
    stride = 1;
  } else if (addr_inc == 4) {
    stride = 2;
  } else if (addr_inc == 8) {
    stride = 3;
  } else if (addr_inc == 16) {
    stride = 4;
  } else if (addr_inc == 32) {
    stride = 5;
  } else {
    return LLD_ERR_BAD_PARM;
  }
  attr |= (single_entry << 3);  // Single-Mode
  attr |= (1 << 4);             // mcast enable
  attr |= (mcast_vector << 5);
  attr |= stride << 9;

  format_dr_msg_tx_wd0(
      desc[0], (data_sz * entry_sz), attr, tx_m_type_mac_wr_blk, 1, 1);

  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_mac_wb_mcast:
 *         Push a multicast mac write-block descriptor
 *         into the WB DR
 *
 * \param dev_id   : dev_id #
 * \param entry_sz : size of each entry in the block
 * \param addr_inc : address stride
 * \param data_sz  : number of writes
 * \param single_entry
 * \param source   : bus address of buffer containing data to
 *write
 * \param dest     : starting Tofino address to write
 * \param mcast_vector: 4b mask of pipes to write, bit0=pipe0,
 *bit1=pipe1..
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : invalid write-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_BAD_PARM : dest buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_mac_wb_mcast(bf_dev_id_t dev_id,
                          int entry_sz,
                          uint32_t addr_inc,
                          int data_sz,
                          bool single_entry,
                          bf_dma_addr_t source,
                          uint64_t dest,
                          uint32_t mcast_vector,
                          uint64_t msg_id) {
  return (lld_subdev_push_mac_wb_mcast(dev_id,
                                       0,
                                       entry_sz,
                                       addr_inc,
                                       data_sz,
                                       single_entry,
                                       source,
                                       dest,
                                       mcast_vector,
                                       msg_id));
}

/** \brief lld_subdev_push_rb:
 *         Push a read-block to the RB DR
 *
 * \param dev_id  : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz: size of each entry in the block
 * \param addr_inc: Address increment on each read
 * \param data_sz : number of ENTRIES to read
 * \param source  : starting Tofino address to read from
 * \param dest    : bus address of buffer to contain read data
 * \param msg_id  : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad read-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source NULL
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_rb(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int entry_sz,
                       uint32_t addr_inc,
                       int data_sz,
                       uint64_t source,
                       bf_dma_addr_t dest,
                       uint64_t msg_id) {
  bf_dev_family_t dev_fam;
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_pipe_read_block);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if ((data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // bad DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad tofino address
  if (dest == 0) return LLD_ERR_BAD_PARM;    // bad buffer pointer
  if (lld_dr_buf_misaligned(dest, 64))
    return LLD_ERR_BAD_PARM;  // bad buffer ptr

  dev_fam = lld_dev_family_get(dev_id);
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      if (addr_inc != 1 && addr_inc != 4) return LLD_ERR_BAD_PARM;
      /* Increment of 1 is encoded as a 1, increment of 4 is encoded as a 0. */
      attr |= (addr_inc == 1 ? 1 : 0) << 3;
      break;
    case BF_DEV_FAMILY_TOFINO2:

    case BF_DEV_FAMILY_TOFINO3:
      if (addr_inc == 1) {
        attr |= (0 << 3);
      } else if (addr_inc == 2) {
        attr |= (1 << 3);
      } else if (addr_inc == 4) {
        attr |= (2 << 3);
      } else if (addr_inc == 8) {
        attr |= (3 << 3);
      } else if (addr_inc == 16) {
        attr |= (4 << 3);
      } else if (addr_inc == 32) {
        attr |= (5 << 3);
      } else {
        return LLD_ERR_BAD_PARM;
      }
      break;
    default:
      return LLD_ERR_BAD_PARM;  // bad chip
  }

  format_dr_msg_tx_wd0(desc[0], data_sz, attr, tx_m_type_rd_blk, 1, 1);
  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_rb:
 *         Push a read-block to the RB DR
 *
 * \param dev_id  : dev_id #
 * \param entry_sz: size of each entry in the block
 * \param addr_inc: Address increment on each read
 * \param data_sz : number of ENTRIES to read
 * \param source  : starting Tofino address to read from
 * \param dest    : bus address of buffer to contain read data
 * \param msg_id  : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad read-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source NULL
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_rb(bf_dev_id_t dev_id,
                int entry_sz,
                uint32_t addr_inc,
                int data_sz,
                uint64_t source,
                bf_dma_addr_t dest,
                uint64_t msg_id) {
  return (lld_subdev_push_rb(
      dev_id, 0, entry_sz, addr_inc, data_sz, source, dest, msg_id));
}

/** \brief lld_subdev_push_que_rb:
 *         Push a que read-block to the RB DR, only for Tof2
 *
 * \param dev_id  : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz: size of each entry in the block
 * \param addr_inc: Address increment on each read
 * \param data_sz : number of ENTRIES to read
 * \param source  : starting Tofino address to read from
 * \param dest    : bus address of buffer to contain read data
 * \param msg_id  : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad read-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source NULL
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_que_rb(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           int dr_0_1,
                           int entry_sz,
                           uint32_t addr_inc,
                           int data_sz,
                           uint64_t source,
                           bf_dma_addr_t dest,
                           uint64_t msg_id) {
  bf_dev_family_t dev_fam;
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  bf_dma_dr_id_t dr_id;
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if ((!lld_dev_is_tof2(dev_id)) && (!lld_dev_is_tof3(dev_id)))

      return LLD_ERR_BAD_PARM;    // invalid chip
  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use
  if (dr_0_1 == 0) {
    dr_id = lld_dr_tx_que_read_block_0;
  } else if (dr_0_1 == 1) {
    dr_id = lld_dr_tx_que_read_block_1;
  } else {
    return LLD_ERR_BAD_PARM;
  }
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : -1;
  if (attr > 2) return LLD_ERR_BAD_PARM;      // bad entry sz
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;  // bad DMA len
  if ((data_sz * entry_sz) > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;                 // bad DMA len
  if (source == 0) return LLD_ERR_BAD_PARM;  // bad tofino address
  if (dest == 0) return LLD_ERR_BAD_PARM;    // bad buffer pointer
  if (lld_dr_buf_misaligned(dest, 64))
    return LLD_ERR_BAD_PARM;  // bad buffer ptr

  dev_fam = lld_dev_family_get(dev_id);
  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      if (addr_inc != 1 && addr_inc != 4) return LLD_ERR_BAD_PARM;
      /* Increment of 1 is encoded as a 1, increment of 4 is encoded as a 0. */
      attr |= (addr_inc == 1 ? 1 : 0) << 3;
      break;
    case BF_DEV_FAMILY_TOFINO2:

    case BF_DEV_FAMILY_TOFINO3:
      if (addr_inc == 1) {
        attr |= (0 << 3);
      } else if (addr_inc == 2) {
        attr |= (1 << 3);
      } else if (addr_inc == 4) {
        attr |= (2 << 3);
      } else if (addr_inc == 8) {
        attr |= (3 << 3);
      } else if (addr_inc == 16) {
        attr |= (4 << 3);
      } else if (addr_inc == 32) {
        attr |= (5 << 3);
      } else {
        return LLD_ERR_BAD_PARM;
      }
      break;
    default:
      return LLD_ERR_BAD_PARM;  // bad chip
  }

  format_dr_msg_tx_wd0(desc[0], data_sz, attr, tx_m_type_que_rd_blk, 1, 1);
  msg->source_address = source;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_que_rb:
 *         Push a que read-block to the RB DR, only for Tof2
 *
 * \param dev_id  : dev_id #
 * \param entry_sz: size of each entry in the block
 * \param addr_inc: Address increment on each read
 * \param data_sz : number of ENTRIES to read
 * \param source  : starting Tofino address to read from
 * \param dest    : bus address of buffer to contain read data
 * \param msg_id  : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad read-type (0-1)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*data_sz > max DMA
 * \return LLD_ERR_BAD_PARM : source NULL
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_que_rb(bf_dev_id_t dev_id,
                    int dr_0_1,
                    int entry_sz,
                    uint32_t addr_inc,
                    int data_sz,
                    uint64_t source,
                    bf_dma_addr_t dest,
                    uint64_t msg_id) {
  return (lld_subdev_push_que_rb(
      dev_id, 0, dr_0_1, entry_sz, addr_inc, data_sz, source, dest, msg_id));
}

/** \brief lld_subdev_push_mac_stats_read:
 *         Push a mac statistics read request to the LLD
 *
 * \param dev_id        : dev_id #
 * \param subdev_id: subdev_id #
 * \param mac_block     : mac_block 0-64
 * \param channel       : 0-3
 * \param clear_on_read : 1=clear after reading, 0=dont clear
 * \param priority      : (currently unused)
 * \param dest          : bus address of buffer to contain read data
 * \param msg_id        : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad pipe
 * \return LLD_ERR_BAD_PARM : bad mac_block (> (LLD_MAX_MAC_BLOCKS-1))
 * \return LLD_ERR_BAD_PARM : bad channel (> 3)
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_mac_stats_read(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   int mac_block,
                                   int channel,
                                   bf_dma_addr_t dest,
                                   uint64_t msg_id) {
  lld_dr_view_t *view;
  uint64_t encoded_src_fld;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  int push_failed;
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  view =
      lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, lld_dr_tx_mac_stat);
  if (view == NULL) return LLD_ERR_BAD_PARM;   // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;   // no space in DR
  if (mac_block < 0) return LLD_ERR_BAD_PARM;  // bad mac_block
  if (mac_block >= lld_get_max_mac_blocks(dev_id)) {
    if (lld_dev_is_tof2(dev_id)) {
      if (mac_block == 39) {  // rotated CPU port MAC
      } else {
        return LLD_ERR_BAD_PARM;  // bad mac_block
      }
    } else {
      return LLD_ERR_BAD_PARM;  // bad mac_block
    }
  }
  if (channel < 0) return LLD_ERR_BAD_PARM;  // bad channel
  if (channel >= lld_get_chnls_per_mac(dev_id)) return LLD_ERR_BAD_PARM;
  if (dest == 0) return LLD_ERR_BAD_PARM;  // bad buffer ptr
  if (lld_dr_buf_misaligned(dest, 64))
    return LLD_ERR_BAD_PARM;  // bad buffer ptr

  encoded_src_fld = ((mac_block << 3) | (channel));

  format_dr_msg_tx_wd0(desc[0], 0, 0, tx_m_type_mac_stat, 1, 1);
  msg->source_address = encoded_src_fld;
  msg->destination_address = dest;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_mac_stats_read:
 *         Push a mac statistics read request to the LLD
 *
 * \param dev_id        : dev_id #
 * \param mac_block     : mac_block 0-64
 * \param channel       : 0-3
 * \param clear_on_read : 1=clear after reading, 0=dont clear
 * \param priority      : (currently unused)
 * \param dest          : bus address of buffer to contain read data
 * \param msg_id        : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad pipe
 * \return LLD_ERR_BAD_PARM : bad mac_block (> (LLD_MAX_MAC_BLOCKS-1))
 * \return LLD_ERR_BAD_PARM : bad channel (> 3)
 * \return LLD_ERR_BAD_PARM : dest NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_mac_stats_read(bf_dev_id_t dev_id,
                            int mac_block,
                            int channel,
                            bf_dma_addr_t dest,
                            uint64_t msg_id) {
  return (lld_subdev_push_mac_stats_read(
      dev_id, 0, mac_block, channel, dest, msg_id));
}

/** \brief lld_subdev_push_fm:
 *         Push a free buffer to the specified FM DR
 *
 * \param dev_id : dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id  : enum for associated dr
 * \param buf    : bus address of free buffer (256B aligned)
 * \param buf_len: length of buffer
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad dr < lld_dr_fm_pkt_0 or > lld_dr_fm_diag
 * \return LLD_ERR_BAD_PARM : buf NULL
 * \return LLD_ERR_BAD_PARM : buf improper alignment (must be 64-byte aligned)
 * \return LLD_ERR_BAD_PARM : buf buf_len (< 0 or > max DMA)
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 *
 */
int lld_subdev_push_fm(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       bf_dma_dr_id_t dr_id,
                       bf_dma_addr_t buf,
                       int buf_len) {
  lld_dr_view_t *view;
  uint64_t desc[1];
  int buf_sz_fld;
  int push_failed;
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  if (dr_id < lld_dr_fm_pkt_0) return LLD_ERR_BAD_PARM;  // bad dr
  if (dr_id > lld_dr_fm_diag) return LLD_ERR_BAD_PARM;   // bad dr

  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR
  if (buf == 0) return LLD_ERR_BAD_PARM;      // bad buffer ptr
  if (lld_dr_buf_misaligned(buf, 256))
    return LLD_ERR_BAD_PARM;                              // bad buffer ptr
  if (buf_len < 256) return LLD_ERR_BAD_PARM;             // bad buffer len
  if (buf_len > LLD_MAX_DMA_SZ) return LLD_ERR_BAD_PARM;  // bad buffer len

  buf_sz_fld = (buf_len < 512)
                   ? 0
                   : (buf_len < 1024)
                         ? 1
                         : (buf_len < 2048)
                               ? 2
                               : (buf_len < 4096)
                                     ? 3
                                     : (buf_len < 8192)
                                           ? 4
                                           : (buf_len < 16384)
                                                 ? 5
                                                 : (buf_len < 32768) ? 6 : 7;

  format_dr_msg_fm_wd0(desc[0], (uint64_t)buf, buf_sz_fld);

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_fm:
 *         Push a free buffer to the specified FM DR
 *
 * \param dev_id : dev_id #
 * \param dr_id  : enum for associated dr
 * \param buf    : bus address of free buffer (256B aligned)
 * \param buf_len: length of buffer
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr < lld_dr_fm_pkt_0 or > lld_dr_fm_diag
 * \return LLD_ERR_BAD_PARM : buf NULL
 * \return LLD_ERR_BAD_PARM : buf improper alignment (must be 64-byte aligned)
 * \return LLD_ERR_BAD_PARM : buf buf_len (< 0 or > max DMA)
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 *
 */
int lld_push_fm(bf_dev_id_t dev_id,
                bf_dma_dr_id_t dr_id,
                bf_dma_addr_t buf,
                int buf_len) {
  return (lld_subdev_push_fm(dev_id, 0, dr_id, buf, buf_len));
}

/** \brief lld_subdev_push_wl:
 *         Push a write list descriptor to the WL DR
 *
 * \param dev_id   : dev_id #
 * \param subdev_id: subdev_id #
 * \param entry_sz : size of each entry in the block
 * \param list_len : number of writes
 * \param list     : bus address of buffer containing data to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*list_len > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_subdev_push_wl(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int dr_0_1,
                       int entry_sz,
                       int list_len,
                       bf_dma_addr_t list,
                       uint64_t msg_id) {
  lld_dr_view_t *view;
  int push_failed;
  uint32_t attr;
  uint64_t desc[4];
  bf_dma_dr_id_t dr_id;
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use
  if (dr_0_1 == 0) {
    dr_id = lld_dr_tx_que_write_list;
  } else if (dr_0_1 == 1) {
    if ((!lld_dev_is_tof2(dev_id)) && (!lld_dev_is_tof3(dev_id)))

        return LLD_ERR_BAD_PARM;
    dr_id = lld_dr_tx_que_write_list_1;
  } else {
    return LLD_ERR_BAD_PARM;
  }
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // invalid chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;  // no space in DR
  if (list == 0) return LLD_ERR_BAD_PARM;     // bad buffer ptr
  if (lld_dr_buf_misaligned(list, 64))
    return LLD_ERR_BAD_PARM;                               // bad buffer ptr
  if (list_len <= 0) return LLD_ERR_BAD_PARM;              // bad buffer len
  if (list_len > LLD_MAX_DMA_SZ) return LLD_ERR_BAD_PARM;  // bad buffer len
  if (list_len * entry_sz > LLD_MAX_DMA_SZ)
    return LLD_ERR_BAD_PARM;  // bad buffer len

  attr = (entry_sz == 4) ? 0 : (entry_sz == 8) ? 1 : (entry_sz == 16) ? 2 : 3;
  if (attr == 3) return LLD_ERR_BAD_PARM;  // bad entry sz

  format_dr_msg_tx_wd0(
      desc[0], list_len * (8 + entry_sz), attr, tx_m_type_que_wr_list, 1, 1);
  msg->source_address = list;
  msg->message_id = msg_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_wl:
 *         Push a write list descriptor to the WL DR
 *
 * \param dev_id   : dev_id #
 * \param entry_sz : size of each entry in the block
 * \param list_len : number of writes
 * \param list     : bus address of buffer containing data to write
 * \param msg_id   : opaque value, returned on completion
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM : bad entry size (4/8/16)
 * \return LLD_ERR_BAD_PARM : entry_sz*list_len > max DMA
 * \return LLD_ERR_BAD_PARM : source buffer NULL
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 */
int lld_push_wl(bf_dev_id_t dev_id,
                int dr_0_1,
                int entry_sz,
                int list_len,
                bf_dma_addr_t list,
                uint64_t msg_id) {
  return (
      lld_subdev_push_wl(dev_id, 0, dr_0_1, entry_sz, list_len, list, msg_id));
}

/** \brief lld_subdev_push_tx_packet:
 *         Push a Tx packet descriptor to the one of the
 *         TX PKT DRs (0-3)
 *
 * \param dev_id : dev_id #
 * \param subdev_id: subdev_id #
 * \param cos    : which dr to use (0-3)
 * \param s      : start bit. 1=start of packet
 * \param e      : end bit. 1= end of packet
 * \param data_sz: length, in bytes, of packet data
 * \param source : bus address of buffer containing packet data
 * \param msg_id : opaque message-id
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 * \return LLD_ERR_BAD_PARM : bad COS specifier (0-3)
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad data_sz (< 0 or > LLD_MAX_DMA_SZ)
 * \return LLD_ERR_BAD_PARM : bad buffer ptr (NULL)
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 */
int lld_subdev_push_tx_packet(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t cos,
                              int s,
                              int e,
                              int data_sz,
                              bf_dma_addr_t source,
                              uint64_t message_id) {
  lld_dr_view_t *view;
  int push_failed;
  uint64_t desc[4];
  dr_msg_tx_t *msg = (dr_msg_tx_t *)&desc[0];
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id))
    return LLD_ERR_NOT_READY;  // chip not ready to use

  if (cos > 3) return LLD_ERR_BAD_PARM;  // bad COS

  view = lld_map_subdev_id_and_dr_to_view(
      dev_id, subdev_id, lld_dr_tx_pkt_0 + cos);
  if (view == NULL) return LLD_ERR_BAD_PARM;              // bad chip
  if (dr_full(view)) return LLD_ERR_DR_FULL;              // no space in DR
  if (data_sz <= 0) return LLD_ERR_BAD_PARM;              // bad data_sz
  if (data_sz > LLD_MAX_DMA_SZ) return LLD_ERR_BAD_PARM;  // bad data_sz
  if (source == 0) return LLD_ERR_BAD_PARM;               // bad buffer ptr

  // constrain to single bit
  s = (s) ? 1 : 0;
  e = (e) ? 1 : 0;

  format_dr_msg_tx_wd0(desc[0], data_sz, 0, tx_m_type_pkt, e, s);
  msg->source_address = source;
  msg->destination_address = source;  // needed?
  msg->message_id = message_id;

  push_failed = view->push(view, desc);
  if (push_failed) {
    return push_failed;
  }
  return LLD_OK;
}

/** \brief lld_push_tx_packet:
 *         Push a Tx packet descriptor to the one of the
 *         TX PKT DRs (0-3)
 *
 * \param dev_id : dev_id #
 * \param cos    : which dr to use (0-3)
 * \param s      : start bit. 1=start of packet
 * \param e      : end bit. 1= end of packet
 * \param data_sz: length, in bytes, of packet data
 * \param source : bus address of buffer containing packet data
 * \param msg_id : opaque message-id
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_DR_FULL  : no space in DR for descriptor
 * \return LLD_ERR_BAD_PARM : bad COS specifier (0-3)
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad data_sz (< 0 or > LLD_MAX_DMA_SZ)
 * \return LLD_ERR_BAD_PARM : bad buffer ptr (NULL)
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 */
int lld_push_tx_packet(bf_dev_id_t dev_id,
                       uint32_t cos,
                       int s,
                       int e,
                       int data_sz,
                       bf_dma_addr_t source,
                       uint64_t message_id) {
  return (lld_subdev_push_tx_packet(
      dev_id, 0, cos, s, e, data_sz, source, message_id));
}

/*****************************************************************************
 * lld_dr_map_dma_type_to_dr_id
 *****************************************************************************/
void lld_dr_map_dma_type_to_dr_id(bf_dev_family_t dev_fam,
                                  bf_dma_type_t t,
                                  bf_dma_dr_id_t *tx_start,
                                  bf_dma_dr_id_t *tx_end,
                                  bf_dma_dr_id_t *rx_start,
                                  bf_dma_dr_id_t *rx_end) {
  switch (t) {
    case BF_DMA_PIPE_INSTRUCTION_LIST:
      *tx_start = lld_dr_tx_pipe_inst_list_0;
      *rx_start = lld_dr_cmp_pipe_inst_list_0;
      if (dev_fam == BF_DEV_FAMILY_TOFINO) {
        *tx_end = lld_dr_tx_pipe_inst_list_3;
        *rx_end = lld_dr_cmp_pipe_inst_list_3;
      } else {
        *tx_end = lld_dr_tx_pipe_inst_list_1;
        *rx_end = lld_dr_cmp_pipe_inst_list_1;
      }
      return;
    case BF_DMA_PIPE_LEARN_NOTIFY:
      *tx_start = *tx_end = lld_dr_fm_learn;
      *rx_start = *rx_end = lld_dr_rx_learn;
      break;
    case BF_DMA_PIPE_STAT_NOTIFY:
      *tx_start = *tx_end = lld_dr_fm_lrt;
      *rx_start = *rx_end = lld_dr_rx_lrt;
      break;
    case BF_DMA_PIPE_IDLE_STATE_NOTIFY:
      *tx_start = *tx_end = lld_dr_fm_idle;
      *rx_start = *rx_end = lld_dr_rx_idle;
      break;
    case BF_DMA_PIPE_BLOCK_WRITE:
      *tx_start = *tx_end = lld_dr_tx_pipe_write_block;
      *rx_start = *rx_end = lld_dr_cmp_pipe_write_blk;
      break;
    case BF_DMA_PIPE_BLOCK_READ:
      *tx_start = *tx_end = lld_dr_tx_pipe_read_block;
      *rx_start = *rx_end = lld_dr_cmp_pipe_read_blk;
      break;
    case BF_DMA_TM_WRITE_LIST:
      *tx_start = *tx_end = lld_dr_tx_que_write_list;
      *rx_start = *rx_end = lld_dr_cmp_que_write_list;
      break;
    case BF_DMA_DIAG_ERR_NOTIFY:
      *tx_start = *tx_end = lld_dr_fm_diag;
      *rx_start = *rx_end = lld_dr_rx_diag;
      break;
    case BF_DMA_MAC_STAT_RECEIVE:
      *tx_start = *tx_end = lld_dr_tx_mac_stat;
      *rx_start = *rx_end = lld_dr_cmp_mac_stat;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_0:
      *tx_start = *tx_end = lld_dr_fm_pkt_0;
      *rx_start = *rx_end = lld_dr_rx_pkt_0;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_1:
      *tx_start = *tx_end = lld_dr_fm_pkt_1;
      *rx_start = *rx_end = lld_dr_rx_pkt_1;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_2:
      *tx_start = *tx_end = lld_dr_fm_pkt_2;
      *rx_start = *rx_end = lld_dr_rx_pkt_2;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_3:
      *tx_start = *tx_end = lld_dr_fm_pkt_3;
      *rx_start = *rx_end = lld_dr_rx_pkt_3;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_4:
      *tx_start = *tx_end = lld_dr_fm_pkt_4;
      *rx_start = *rx_end = lld_dr_rx_pkt_4;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_5:
      *tx_start = *tx_end = lld_dr_fm_pkt_5;
      *rx_start = *rx_end = lld_dr_rx_pkt_5;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_6:
      *tx_start = *tx_end = lld_dr_fm_pkt_6;
      *rx_start = *rx_end = lld_dr_rx_pkt_6;
      break;
    case BF_DMA_CPU_PKT_RECEIVE_7:
      *tx_start = *tx_end = lld_dr_fm_pkt_7;
      *rx_start = *rx_end = lld_dr_rx_pkt_7;
      break;
    case BF_DMA_CPU_PKT_TRANSMIT_0:
      *tx_start = *tx_end = lld_dr_tx_pkt_0;
      *rx_start = *rx_end = lld_dr_cmp_tx_pkt_0;
      break;
    case BF_DMA_CPU_PKT_TRANSMIT_1:
      *tx_start = *tx_end = lld_dr_tx_pkt_1;
      *rx_start = *rx_end = lld_dr_cmp_tx_pkt_1;
      break;
    case BF_DMA_CPU_PKT_TRANSMIT_2:
      *tx_start = *tx_end = lld_dr_tx_pkt_2;
      *rx_start = *rx_end = lld_dr_cmp_tx_pkt_2;
      break;
    case BF_DMA_CPU_PKT_TRANSMIT_3:
      *tx_start = *tx_end = lld_dr_tx_pkt_3;
      *rx_start = *rx_end = lld_dr_cmp_tx_pkt_3;
      break;
    case BF_DMA_MAC_BLOCK_WRITE:
      *tx_start = *tx_end = lld_dr_tx_mac_write_block;
      *rx_start = *rx_end = lld_dr_cmp_mac_write_block;
      break;
    case BF_DMA_TM_WRITE_LIST_1:
      *tx_start = *tx_end = lld_dr_tx_que_write_list_1;
      *rx_start = *rx_end = lld_dr_cmp_que_write_list_1;
      break;
    case BF_DMA_TM_BLOCK_READ_0:
      *tx_start = *tx_end = lld_dr_tx_que_read_block_0;
      *rx_start = *rx_end = lld_dr_cmp_que_read_block_0;
      break;
    case BF_DMA_TM_BLOCK_READ_1:
      *tx_start = *tx_end = lld_dr_tx_que_read_block_1;
      *rx_start = *rx_end = lld_dr_cmp_que_read_block_1;
      break;
    default:
      *tx_start = *tx_end = BF_DMA_NO_DR;
      *rx_start = *rx_end = BF_DMA_NO_DR;
      return;
  }
}

/*****************************************************************************
 * lld_dr_max_dr_depth_get
 *****************************************************************************/
void lld_dr_max_dr_depth_get(bf_dev_id_t dev_id,
                             bf_dma_type_t type,
                             int *tx_depth,
                             int *rx_depth) {
  bf_dev_family_t dev_fam;
  int tx_desc_sz;
  int rx_desc_sz;
  int max_entries_per_dr;
  bf_dma_dr_id_t dr_s[2], dr_e[2];

  if (lld_dev_is_tofino(dev_id)) {
    dev_fam = BF_DEV_FAMILY_TOFINO;
  } else if (lld_dev_is_tof2(dev_id)) {
    dev_fam = BF_DEV_FAMILY_TOFINO2;
  } else if (lld_dev_is_tof3(dev_id)) {
    dev_fam = BF_DEV_FAMILY_TOFINO3;




  } else {
    dev_fam = BF_DEV_FAMILY_UNKNOWN;
  }

  lld_dr_map_dma_type_to_dr_id(
      dev_fam, type, &dr_s[0], &dr_e[0], &dr_s[1], &dr_e[1]);

  /* Get number of 64 bit words the Tx and Rx DRs use in an entry. */
  tx_desc_sz = lld_dr_words_per_desc(dev_id, dr_s[0]);
  rx_desc_sz = lld_dr_words_per_desc(dev_id, dr_s[1]);

  /* Max DR size is 1MB, each descriptor in the DR is 8 bytes. */
  max_entries_per_dr = 1024 * 1024 / 8;

  /* Different DRs use different numbers of descriptors per entry. */
  *tx_depth = max_entries_per_dr / tx_desc_sz;
  *rx_depth = max_entries_per_dr / rx_desc_sz;
}

/*****************************************************************************
 * lld_dr_mem_requirement_get
 *****************************************************************************/
int lld_dr_mem_requirement_get(bf_dev_family_t dev_fam,
                               bf_dma_type_t type,
                               int tx_depth,
                               int rx_depth) {
  bf_dma_dr_id_t dr_s[2], dr_e[2];
  int size = 0, dir;

  switch (dev_fam) {
    case BF_DEV_FAMILY_TOFINO:
      if (type >= BF_DMA_TYPE_MAX_TOF) return LLD_ERR_BAD_PARM;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (type >= BF_DMA_TYPE_MAX_TOF2) return LLD_ERR_BAD_PARM;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (type >= BF_DMA_TYPE_MAX_TOF3) return LLD_ERR_BAD_PARM;
      break;





    default:
      return LLD_ERR_BAD_PARM;
  }
  if (tx_depth < 0 || rx_depth < 0) return LLD_ERR_BAD_PARM;
  if (!tx_depth || !rx_depth) return 0;

  lld_dr_map_dma_type_to_dr_id(
      dev_fam, type, &dr_s[0], &dr_e[0], &dr_s[1], &dr_e[1]);

  if (BF_DMA_NO_DR == dr_s[0]) return 0;

  for (dir = 0; dir < 2; ++dir) {
    bf_dma_dr_id_t x;
    for (x = dr_s[dir]; x <= dr_e[dir]; ++x) {
      int n_bytes_per_desc = 8 * lld_dr_words_per_desc(dev_fam, x);
      int this_dr_sz = n_bytes_per_desc * (!dir ? tx_depth : rx_depth);

      // round up to cache-line boundary
      this_dr_sz = (this_dr_sz + 63) & ~(63);

      size += this_dr_sz;
      /* Extra 64 bytes for the additional cache line used by the chip to
       * push its pointer. */
      size += 64;
    }
  }
  return size;
}

/** \brief lld_subdev_dr_used_get
 * Return the number of occupied entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of free entries in the fifo
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_subdev_dr_used_get(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_dma_dr_id_t dr_id) {
  lld_dr_view_t *view;
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id)) return LLD_ERR_NOT_READY;
  if (lld_validate_dr_id(dev_id, dr_id)) return LLD_ERR_BAD_PARM;
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip

  dr_update_view(view);
  return dr_used(view);
}

/** \brief lld_subdev_dr_unused_get
 * Return the number of available entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of free entries in the fifo
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_subdev_dr_unused_get(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_dma_dr_id_t dr_id) {
  lld_dr_view_t *view;
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id)) return LLD_ERR_NOT_READY;
  if (lld_validate_dr_id(dev_id, dr_id)) return LLD_ERR_BAD_PARM;
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip

  dr_update_view(view);
  return dr_space(view);
}

/** \brief lld_subdev_dr_depth_get
 * Return the number of entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of entries in the DR
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_subdev_dr_depth_get(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            bf_dma_dr_id_t dr_id) {
  lld_dr_view_t *view;
  (void)subdev_id;

  if (!lld_dev_ready(dev_id, subdev_id)) return LLD_ERR_NOT_READY;
  if (lld_validate_dr_id(dev_id, dr_id)) return LLD_ERR_BAD_PARM;
  view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip

  return dr_depth(view);
}

/** \brief lld_dr_used_get
 * Return the number of occupied entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of free entries in the fifo
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_dr_used_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id) {
  return (lld_subdev_dr_used_get(dev_id, 0, dr_id));
}

/** \brief lld_dr_unused_get
 * Return the number of available entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of free entries in the fifo
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_dr_unused_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id) {
  return (lld_subdev_dr_unused_get(dev_id, 0, dr_id));
}

/** \brief lld_dr_depth_get
 * Return the number of entries in the given DR
 *
 * \param dev_id: dev_id #
 * \param dr_id : enum identifying DR to check
 *
 * \return             >= 0 : number of entries in the DR
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 * \return LLD_ERR_NOT_READY: chip_add in still progress
 *
 */
int lld_dr_depth_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id) {
  return (lld_subdev_dr_depth_get(dev_id, 0, dr_id));
}

/** \brief lld_subdev_dr_lock_required
 * Indicates the specified DR must be locked to prevent
 * simultaneous access (to protect head/tail pointers)
 *
 * \param dev_id: dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id : enum identifying DR to be lockable
 *
 * \return           LLD_OK
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad dr
 *
 */
int lld_subdev_dr_lock_required(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_dma_dr_id_t dr_id) {
  int x;
  lld_dr_view_t *view;
  (void)subdev_id;

  if (lld_validate_dr_id(dev_id, dr_id)) return LLD_ERR_BAD_PARM;
  view = lld_map_subdev_id_and_dr_to_view_allow_unassigned(
      dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip

  if (!view->lock_reqd) {
    x = bf_sys_mutex_init(&view->mtx[0]);
    if (x) return LLD_ERR_LOCK_FAILED;
    x = bf_sys_mutex_init(&view->mtx[1]);
    if (x) return LLD_ERR_LOCK_FAILED;

    view->lock_reqd = 1;
  }

  return LLD_OK;
}

/** \brief lld_dr_lock_required
 * Indicates the specified DR must be locked to prevent
 * simultaneous access (to protect head/tail pointers)
 *
 * \param dev_id: dev_id #
 * \param dr_id : enum identifying DR to be lockable
 *
 * \return           LLD_OK
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 *
 */
int lld_dr_lock_required(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id) {
  return (lld_subdev_dr_lock_required(dev_id, 0, dr_id));
}

/** \brief lld_subdev_dr_lock_not_required
 * Indicates the specified DR no longer requires a lock
 *
 * \param dev_id: dev_id #
 * \param subdev_id: subdev_id #
 * \param dr_id : enum identifying DR to be changed
 *
 * \return           LLD_OK
 * \return LLD_ERR_BAD_PARM : bad chip or subdev_id
 * \return LLD_ERR_BAD_PARM : bad dr
 *
 */
int lld_subdev_dr_lock_not_required(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id,
                                    bf_dma_dr_id_t dr_id) {
  lld_dr_view_t *view;
  (void)subdev_id;

  if (lld_validate_dr_id(dev_id, dr_id)) return LLD_ERR_BAD_PARM;
  view = lld_map_subdev_id_and_dr_to_view_allow_unassigned(
      dev_id, subdev_id, dr_id);
  if (view == NULL) return LLD_ERR_BAD_PARM;  // bad chip

  if (view->lock_reqd) {
    bf_sys_mutex_del(&view->mtx[0]);
    bf_sys_mutex_del(&view->mtx[1]);
    view->lock_reqd = 0;
  }

  return LLD_OK;
}

/** \brief lld_dr_lock_not_required
 * Indicates the specified DR no longer requires a lock
 *
 * \param dev_id: dev_id #
 * \param dr_id : enum identifying DR to be changed
 *
 * \return           LLD_OK
 * \return LLD_ERR_BAD_PARM : bad chip
 * \return LLD_ERR_BAD_PARM : bad dr
 *
 */
int lld_dr_lock_not_required(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id) {
  return (lld_subdev_dr_lock_not_required(dev_id, 0, dr_id));
}

/*****************************************************************
 * lld_dr_buf_misaligned
 *
 * \param addr       : logical address of buffer to check
 * \param requirement: required alignment, in bytes
 *
 * \return LLD_OK (0)
 * \return LLD_ERR_BAD_PARM = invaid chip
 *
 ****************************************************************/
static lld_err_t lld_dr_buf_misaligned(bf_dma_addr_t addr, int requirement) {
  uint64_t mask;

  mask = (uint64_t)requirement - 1ull;
  if (((uint64_t)lld_ptr_to_u64(addr)) & mask) {
    return LLD_ERR_BAD_PARM;
  }
  return LLD_OK;
}

/**
 * @}
 */
