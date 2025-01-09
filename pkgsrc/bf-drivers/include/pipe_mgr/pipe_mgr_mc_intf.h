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


#ifndef __PIPE_MGR_MC_INTF_H__
#define __PIPE_MGR_MC_INTF_H__

#include <pipe_mgr/pipe_mgr_intf.h>

typedef struct mc_mgr_pvt_entry {
  uint64_t d;
  uint8_t has_400g_map;
} mc_mgr_pvt_entry_t;

typedef struct mc_mgr_tvt_entry {
  uint16_t diemap;
} mc_mgr_tvt_entry_t;

typedef struct mc_mgr_table_entry {
  mc_mgr_pvt_entry_t pv;
  mc_mgr_tvt_entry_t tv;
  uint16_t pv_row;
  uint16_t tv_row;
} mc_mgr_table_entry_t;

pipe_status_t pipe_mgr_mc_pipe_msk_set(pipe_sess_hdl_t shdl,
                                       dev_target_t dev,
                                       int mgid_grp,
                                       mc_mgr_pvt_entry_t masks,
                                       uint32_t table_msk,
                                       bool push,
                                       bool complete,
                                       bool rebuilding);

pipe_status_t pipe_mgr_mc_pipe_pvt_msk_set(pipe_sess_hdl_t shdl,
                                           dev_target_t dev,
                                           uint16_t row,
                                           mc_mgr_pvt_entry_t entry,
                                           bool push,
                                           bool complete,
                                           bool rebuilding);

pipe_status_t pipe_mgr_mc_pipe_tvt_msk_set(pipe_sess_hdl_t shdl,
                                           dev_target_t dev,
                                           uint16_t row,
                                           mc_mgr_tvt_entry_t entry,
                                           bool push,
                                           bool complete,
                                           bool rebuilding);

pipe_status_t pipe_mgr_mc_pipe_msk_get(bf_dev_id_t dev,
                                       int mgid_grp,
                                       int phy_pipe,
                                       uint32_t *masks);

pipe_status_t pipe_mgr_mc_tvt_msk_get(bf_dev_id_t dev,
                                      int mgid_grp,
                                      int phy_pipe,
                                      uint32_t *masks);

pipe_status_t pipe_mgr_mc_c2c_pipe_msk_set(pipe_sess_hdl_t shdl,
                                           dev_target_t dev,
                                           uint32_t mask,
                                           bool push,
                                           bool complete);

pipe_status_t pipe_mgr_mc_pipe_msk_update_push(pipe_sess_hdl_t shdl,
                                               bool complete);

pipe_status_t pipe_mgr_mc_c2c_pipe_msk_get(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint32_t *mask);

pipe_status_t pipe_mgr_mc_tbl_ver_set(pipe_sess_hdl_t shdl,
                                      dev_target_t dev,
                                      int ver,
                                      bool rebuilding);

pipe_status_t pipe_mgr_mc_reg_write(pipe_sess_hdl_t shdl,
                                    dev_target_t dev,
                                    uint32_t addr,
                                    uint32_t val);

pipe_status_t pipe_mgr_mc_reg_read(pipe_sess_hdl_t shdl,
                                   dev_target_t dev,
                                   uint32_t addr,
                                   uint32_t *val);
#endif
