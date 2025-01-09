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


#ifndef __MC_MGR_HANDLE_H__
#define __MC_MGR_HANDLE_H__

#include "mc_mgr.h"
#include "mc_mgr_int.h"

/* Session Handles. */
bf_mc_session_hdl_t mc_mgr_encode_sess_hdl(int index);

bool mc_mgr_decode_sess_hdl(bf_mc_session_hdl_t hdl, int *index);

/* MGID Handles. */
bf_mc_mgrp_hdl_t mc_mgr_encode_mgrp_hdl(bf_mc_grp_id_t grp);

bool mc_mgr_decode_mgrp_hdl(bf_mc_mgrp_hdl_t h,
                            bf_mc_grp_id_t *grp,
                            const char *where,
                            const int line);

/* ECMP Handles. */
bf_status_t mc_mgr_encode_ecmp_hdl(bf_dev_id_t dev, bf_mc_ecmp_hdl_t *e);
bf_status_t mc_mgr_decode_ecmp_hdl(bf_mc_ecmp_hdl_t e,
                                   const char *where,
                                   const int line);
bf_status_t mc_mgr_delete_ecmp_hdl(bf_dev_id_t dev, bf_mc_ecmp_hdl_t hdl);

/* Node Handles. */
bf_status_t mc_mgr_encode_l1_node_hdl(bf_dev_id_t dev, bf_mc_node_hdl_t *hdl);

bool mc_mgr_decode_l1_node_hdl(bf_mc_node_hdl_t h,
                               int *id,
                               const char *where,
                               const int line);

bf_status_t mc_mgr_delete_l1_node_hdl(bf_dev_id_t dev, bf_mc_node_hdl_t hdl);
#endif
