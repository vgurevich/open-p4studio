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


#ifndef LLD_TOF2_INTERRUPT_INCLUDED
#define LLD_TOF2_INTERRUPT_INCLUDED

#include <lld/lld_interrupt.h>

#define LLD_TOF2_COUNT_NUMB (32)

void lld_tof2_int_svc(bf_dev_id_t dev_id,
                      uint32_t sh_int_val,
                      uint16_t sh_int_reg);
void lld_tof2_int_gbl_en_set(bf_dev_id_t dev_id, bool en);
void lld_tof2_int_msk(bf_dev_id_t dev_id,
                      uint32_t sh_int_reg,
                      uint32_t sh_int_bit);
void lld_tof2_int_ena(bf_dev_id_t dev_id,
                      uint32_t sh_int_reg,
                      uint32_t sh_int_bit);

int lld_tof2_int_register_cb(bf_dev_id_t dev_id,
                             uint32_t offset,
                             lld_int_cb cb_fn,
                             void *userdata);
// int lld_tof2_get_cbus_cbc_status_offset(uint32_t *offset_p, void *userdata,
// int len);
lld_int_cb lld_tof2_get_int_cb(bf_dev_id_t dev_id,
                               uint32_t offset,
                               void **userdata);
bf_status_t lld_tof2_int_poll(bf_dev_id_t dev_id, bool all_ints);
uint32_t lld_tof2_int_get_glb_status(bf_dev_id_t dev_id);
uint32_t lld_tof2_int_get_shadow_int_status(bf_dev_id_t dev_id,
                                            uint16_t sh_int_reg);
uint32_t lld_tof2_int_get_shadow_msk_status(bf_dev_id_t dev_id,
                                            uint16_t sh_msk_reg);
void lld_tof2_int_set_shadow_msk_status(bf_dev_id_t dev_id,
                                        uint16_t sh_msk_reg,
                                        uint32_t value);
void lld_tof2_int_host_leaf_enable_set(bf_dev_id_t dev_id, bool en);
bf_status_t lld_tof2_int_disable_all_cb(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        void *blk_lvl_int);
bf_status_t lld_tof2_int_traverse_blk_lvl_int(bf_dev_id_t dev_id,
                                              lld_blk_int_traverse_cb cb_fn);
bf_status_t lld_tof2_clear_all_ints_cb(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       void *blk_lvl_int_vd);
bf_status_t lld_tof2_clear_ints_cb(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   void *blk_lvl_int_vd);
bf_status_t lld_tof2_enable_all_pipe_ints_cb(bf_dev_id_t dev_id,
                                             bf_subdev_id_t subdev_id,
                                             void *blk_lvl_int_vd);
bf_status_t lld_tof2_inject_all_ints_cb(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        void *blk_lvl_int_vd);
bf_status_t lld_tof2_inject_ints_with_offset(bf_dev_id_t dev_id,
                                             uint32_t offset);
void lld_tof2_int_mbus_leaf_enable_set(bf_dev_id_t dev_id, bool en);
bf_status_t lld_tof2_dump_int_list_cb(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      void *blk_lvl_int_vd);
bf_status_t lld_tof2_dump_new_ints_cb(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      void *blk_lvl_int_vd);
bf_status_t lld_tof2_clear_new_ints_cb(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       void *blk_lvl_int_vd);
bf_status_t lld_tof2_dump_all_ints_cb(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      void *blk_lvl_int_vd);
bf_status_t lld_tof2_dump_unfired_ints_cb(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          void *blk_lvl_int_vd);
uint32_t lld_tof2_get_status_reg(void *blk_lvl_int);
bf_status_t lld_tof2_msix_map_set(bf_dev_id_t dev_id, int irq, int msix_num);

#endif  // LLD_TOF2_INTERRUPT_INCLUDED
