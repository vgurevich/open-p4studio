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


#ifndef lld_dr_if_h
#define lld_dr_if_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dr_completion_callback_fn)(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          bf_dma_dr_id_t dr,
                                          uint64_t datasz_timestamp,
                                          uint32_t attr,
                                          uint32_t status,
                                          uint32_t type,
                                          uint64_t msg_id,
                                          int s,
                                          int e);

typedef void (*dr_rx_diag_callback_fn)(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       int data_sz,
                                       bf_dma_addr_t address);
typedef void (*dr_rx_lrt_callback_fn)(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      int data_sz,
                                      bf_dma_addr_t address);
typedef void (*dr_rx_idle_callback_fn)(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       int data_sz,
                                       bf_dma_addr_t address);
typedef void (*dr_rx_learn_callback_fn)(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        int data_sz,
                                        bf_dma_addr_t address,
                                        int s,
                                        int e,
                                        bf_dev_pipe_t pipe);
typedef void (*dr_rx_packet_callback_fn)(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         int data_sz,
                                         bf_dma_addr_t address,
                                         int s,
                                         int e,
                                         int cos);

typedef union {
  dr_completion_callback_fn cmp_fn;
  dr_rx_diag_callback_fn rx_diag_fn;
  dr_rx_lrt_callback_fn rx_lrt_fn;
  dr_rx_idle_callback_fn rx_idle_fn;
  dr_rx_learn_callback_fn rx_learn_fn;
  dr_rx_packet_callback_fn rx_packet_fn;
  void *any_fn;
} lld_cb_fn_u;

typedef struct pbus_arb_ctrl_s {
  uint32_t ctrl_0;
  uint32_t ctrl_1[BF_PIPE_COUNT];
} pbus_arb_ctrl_t;

int lld_dr_lock_required(bf_dev_id_t dev_id, bf_dma_dr_id_t dr);
int lld_subdev_dr_lock_required(bf_dev_id_t dev_id,
                                bf_subdev_id_t subdev_id,
                                bf_dma_dr_id_t dr);
int lld_dr_lock_not_required(bf_dev_id_t dev_id, bf_dma_dr_id_t dr);
void lld_dr_max_dr_depth_get(bf_dev_id_t dev_id,
                             bf_dma_type_t type,
                             int *tx_depth,
                             int *rx_depth);
int lld_dr_mem_requirement_get(bf_dev_family_t dev_fam,
                               bf_dma_type_t type,
                               int tx_depth,
                               int rx_depth);
int lld_register_completion_callback(bf_dev_id_t dev_id,
                                     bf_subdev_id_t subdev_id,
                                     bf_dma_dr_id_t dr,
                                     dr_completion_callback_fn fn);
int lld_register_rx_diag_callback(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  dr_rx_diag_callback_fn fn);
int lld_register_rx_lrt_callback(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 dr_rx_lrt_callback_fn fn);
int lld_register_rx_idle_callback(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  dr_rx_idle_callback_fn fn);
int lld_register_rx_learn_callback(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   dr_rx_learn_callback_fn fn);
int lld_register_rx_packet_callback(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id,
                                    int cos,
                                    dr_rx_packet_callback_fn fn);

int lld_dr_start(bf_dev_id_t dev_id,
                 bf_subdev_id_t subdev_id,
                 bf_dma_dr_id_t dr);
int lld_dr_get_used_count(bf_dev_id_t dev_id,
                          bf_subdev_id_t subdev_id,
                          bf_dma_dr_id_t dr_id,
                          bool from_hw);
int lld_dr_service(bf_dev_id_t dev_id,
                   bf_subdev_id_t subdev_id,
                   bf_dma_dr_id_t dr_e,
                   int n);

int lld_push_ilist(bf_dev_id_t dev_id,
                   int dr_0_3,
                   bf_dma_addr_t list,
                   int list_len,
                   int rsp_sz,
                   bool s_f,
                   bf_dma_addr_t ack_ptr,
                   uint64_t msg_id);
int lld_push_ilist_mcast(bf_dev_id_t dev_id,
                         int dr_0_3,
                         bf_dma_addr_t list,
                         int list_len,
                         int rsp_sz,
                         bool s_f,
                         uint32_t mcast_vector,
                         bf_dma_addr_t ack_ptr,
                         uint64_t msg_id);
int lld_push_wb(bf_dev_id_t dev_id,
                int entry_sz,
                uint32_t addr_inc,
                int data_sz,
                bool single_entry,
                bf_dma_addr_t source,
                uint64_t dest,
                uint64_t msg_id);

int lld_push_wb_mcast(bf_dev_id_t dev_id,
                      int entry_sz,
                      uint32_t addr_inc,
                      int data_sz,
                      bool single_entry,
                      bf_dma_addr_t source,
                      uint64_t dest,
                      uint32_t mcast_vector,
                      uint64_t msg_id);
int lld_push_mac_wb(bf_dev_id_t dev_id,
                    int entry_sz,
                    uint32_t addr_inc,
                    int data_sz,
                    bool single_entry,
                    bf_dma_addr_t source,
                    uint64_t dest,
                    uint64_t msg_id);
int lld_push_mac_wb_mcast(bf_dev_id_t dev_id,
                          int entry_sz,
                          uint32_t addr_inc,
                          int data_sz,
                          bool single_entry,
                          bf_dma_addr_t source,
                          uint64_t dest,
                          uint32_t mcast_vector,
                          uint64_t msg_id);
int lld_push_rb(bf_dev_id_t dev_id,
                int entry_sz,
                uint32_t addr_inc,
                int data_sz,
                uint64_t source,
                bf_dma_addr_t dest,
                uint64_t msg_id);
int lld_push_que_rb(bf_dev_id_t dev_id,
                    int dr_0_1,
                    int entry_sz,
                    uint32_t addr_inc,
                    int data_sz,
                    uint64_t source,
                    bf_dma_addr_t dest,
                    uint64_t msg_id);
int lld_push_wl(bf_dev_id_t dev_id,
                int dr_0_1,
                int entry_sz,
                int list_len,
                bf_dma_addr_t list,
                uint64_t msg_id);

int lld_push_fm(bf_dev_id_t dev_id,
                bf_dma_dr_id_t dr_e,
                bf_dma_addr_t buf,
                int buf_len);
int lld_subdev_push_fm(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       bf_dma_dr_id_t dr_id,
                       bf_dma_addr_t buf,
                       int buf_len);

int lld_push_mac_stats_read(bf_dev_id_t dev_id,
                            int mac_block,
                            int channel,
                            bf_dma_addr_t dest,
                            uint64_t msg_id);

int lld_push_tx_packet(bf_dev_id_t dev_id,
                       uint32_t cos_map,
                       int s,
                       int e,
                       int data_sz,
                       bf_dma_addr_t source,
                       uint64_t message_id);

int lld_subdev_push_tx_packet(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t cos_map,
                              int s,
                              int e,
                              int data_sz,
                              bf_dma_addr_t source,
                              uint64_t message_id);

int lld_dr_used_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr);
int lld_dr_unused_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr);
int lld_dr_depth_get(bf_dev_id_t dev_id, bf_dma_dr_id_t dr);

void lld_dr_enable_set(bf_dev_id_t dev_id, bf_dma_dr_id_t dr, bool en);
void lld_subdev_dr_enable_set(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              bf_dma_dr_id_t dr,
                              bool en);
bf_status_t lld_dr_set_write_time_mode(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       bf_dma_dr_id_t dr_id,
                                       bool en);
bf_status_t lld_dr_pushed_ptr_mode_set(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       bf_dma_dr_id_t dr_id,
                                       bool en);
void lld_dr_data_timeout_set(bf_dev_id_t dev_id,
                             bf_dma_dr_id_t dr_id,
                             uint32_t timeout);
void lld_subdev_dr_data_timeout_set(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id,
                                    bf_dma_dr_id_t dr_id,
                                    uint32_t timeout);
bf_status_t lld_subdev_dr_data_timeout_get(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           bf_dma_dr_id_t dr_id,
                                           uint32_t *timeout);
void lld_dr_cbus_arb_ctrl_set(bf_dev_id_t dev_id,
                              bf_subdev_id_t subdev_id,
                              uint32_t cbus_arb_ctrl_val);
void lld_dr_map_dma_type_to_dr_id(bf_dev_family_t dev_fam,
                                  bf_dma_type_t t,
                                  bf_dma_dr_id_t *tx_start,
                                  bf_dma_dr_id_t *tx_end,
                                  bf_dma_dr_id_t *rx_start,
                                  bf_dma_dr_id_t *rx_end);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
