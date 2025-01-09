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


#include <tofino/bf_pal/bf_pal_types.h>
#include <tofino/bf_pal/bf_pal_pltfm_porting.h>
#include <dvm/bf_drv_intf.h>

/* This function enables the application (bf_switchd) to set multi-device
   mode for the model */
bf_status_t bf_model_num_devices_set(int devices);

/* The following functions are required by bf_pm */
bf_status_t bf_model_front_panel_port_get_first(
    bf_pal_front_port_info_t *first_port_info);

bf_status_t bf_model_front_panel_port_get_next(
    bf_pal_front_port_info_t *curr_port_info,
    bf_pal_front_port_info_t *next_port_info);

bf_status_t bf_model_mac_to_serdes_map_get(
    bf_pal_front_port_handle_t *port_hdl,
    bf_pal_mac_to_serdes_lane_map_t *mac_blk_map);

bf_status_t bf_model_serdes_info_get(bf_pal_front_port_handle_t *port_hdl,
                                     bf_pal_serdes_info_t *serdes_info);

bf_status_t bf_model_media_type_get(bf_pal_front_port_handle_t *port_hdl,
                                    bf_media_type_t *media_type);

bf_status_t bf_model_port_str_to_hdl_get(const char *port_str,
                                         bf_pal_front_port_handle_t *port_hdl);

bf_status_t bf_model_init(bf_dev_init_mode_t warm_init_mode);

bf_status_t bf_model_safe_to_call_in_notify();
bf_status_t bf_model_pre_port_enable_cfg_set(
    bf_pal_front_port_handle_t *port_hdl, bf_pal_front_port_cb_cfg_t *port_cfg);

bf_status_t bf_model_mac_to_multi_serdes_map_get(
    bf_pal_front_port_handle_t *port_hdl,
    bf_pal_mac_to_multi_serdes_lane_map_t *mac_blk_map);
