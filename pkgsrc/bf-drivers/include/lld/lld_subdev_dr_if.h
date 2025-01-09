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


#ifndef lld_subdev_dr_if_h
#define lld_subdev_dr_if_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

int lld_subdev_dr_lock_required(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_dma_dr_id_t dr);
int lld_subdev_dr_lock_not_required(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id,
                                    bf_dma_dr_id_t dr);

int lld_subdev_push_ilist(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          int dr_0_3,
                          bf_dma_addr_t list,
                          int list_len,
                          int rsp_sz,
                          bool s_f,
                          bf_dma_addr_t ack_ptr,
                          uint64_t msg_id);
int lld_subdev_push_ilist_mcast(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                int dr_0_3,
                                bf_dma_addr_t list,
                                int list_len,
                                int rsp_sz,
                                bool s_f,
                                uint32_t mcast_vector,
                                bf_dma_addr_t ack_ptr,
                                uint64_t msg_id);
int lld_subdev_push_wb(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int entry_sz,
                       uint32_t addr_inc,
                       int data_sz,
                       bool single_entry,
                       bf_dma_addr_t source,
                       uint64_t dest,
                       uint64_t msg_id);

int lld_subdev_push_wb_mcast(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             int entry_sz,
                             uint32_t addr_inc,
                             int data_sz,
                             bool single_entry,
                             bf_dma_addr_t source,
                             uint64_t dest,
                             uint32_t mcast_vector,
                             uint64_t msg_id);
int lld_subdev_push_mac_wb(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           int entry_sz,
                           uint32_t addr_inc,
                           int data_sz,
                           bool single_entry,
                           bf_dma_addr_t source,
                           uint64_t dest,
                           uint64_t msg_id);
int lld_subdev_push_mac_wb_mcast(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 int entry_sz,
                                 uint32_t addr_inc,
                                 int data_sz,
                                 bool single_entry,
                                 bf_dma_addr_t source,
                                 uint64_t dest,
                                 uint32_t mcast_vector,
                                 uint64_t msg_id);
int lld_subdev_push_rb(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int entry_sz,
                       uint32_t addr_inc,
                       int data_sz,
                       uint64_t source,
                       bf_dma_addr_t dest,
                       uint64_t msg_id);
int lld_subdev_push_que_rb(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           int dr_0_1,
                           int entry_sz,
                           uint32_t addr_inc,
                           int data_sz,
                           uint64_t source,
                           bf_dma_addr_t dest,
                           uint64_t msg_id);
int lld_subdev_push_wl(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       int dr_0_1,
                       int entry_sz,
                       int list_len,
                       bf_dma_addr_t list,
                       uint64_t msg_id);

int lld_subdev_push_fm(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       bf_dma_dr_id_t dr_e,
                       bf_dma_addr_t buf,
                       int buf_len);

int lld_subdev_push_mac_stats_read(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   int mac_block,
                                   int channel,
                                   bf_dma_addr_t dest,
                                   uint64_t msg_id);

int lld_subdev_push_tx_packet(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t cos_map,
                              int s,
                              int e,
                              int data_sz,
                              bf_dma_addr_t source,
                              uint64_t message_id);

int lld_subdev_dr_used_get(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_dma_dr_id_t dr);
int lld_subdev_dr_unused_get(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             bf_dma_dr_id_t dr);
int lld_subdev_dr_depth_get(bf_dev_id_t dev_id,
                            bf_subdev_id_t subdev_id,
                            bf_dma_dr_id_t dr);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
