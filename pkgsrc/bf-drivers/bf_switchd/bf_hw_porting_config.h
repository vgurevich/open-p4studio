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


#ifndef _BF_HW_PORTING_CONFIG_H_
#define _BF_HW_PORTING_CONFIG_H_
#include "bf_switchd.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <tofino/bf_pal/bf_pal_pltfm_porting.h>
#include <tofino/bf_pal/bf_pal_types.h>

// Default path
#define BOARD_LANE_MAP_CONF_FILE_DEFAULT_PATH \
  "/usr/share/sonic/hwsku/board_lane_map.json"

#define MAX_CHAN_PER_CONNECTORS 8

#define BF_GET_CONN_MAP_KEY(conn, channel) ((conn) | ((channel) << 16))
#define BF_GET_CONN_FROM_KEY(key) (key & 0xFFFF)
#define BF_GET_CHNL_FROM_KEY(key) ((key >> 16) & 0xFFFF)

#define BF_FIRST_CONN_ID 1
#define BF_FIRST_CHNL_ID 0

enum {
  BF_CHIP_TYPE_TF1 = 0,
  BF_CHIP_TYPE_TF2,
  BF_CHIP_TYPE_TF3,
};

/*
 * Encapsulates the information of a port on the board
 */
typedef struct bf_pltfm_port_info_t {
  uint32_t conn_id;
  uint32_t chnl_id;
} bf_pltfm_port_info_t;

typedef struct serdes_lane_tx_eq_ {
  int32_t tx_main[1];
  int32_t tx_pre1[1];
  int32_t tx_pre2[1];
  int32_t tx_post1[1];
  int32_t tx_post2[1];
} serdes_lane_tx_eq_t;

// Lane map
typedef struct lane_map_t {
  uint32_t channel;  // lane
  uint32_t mac_ch;

  // common settings for channels
  uint32_t tx_lane;
  uint32_t tx_pn_swap;
  uint32_t rx_lane;
  uint32_t rx_pn_swap;

  // TF1 serdes settings
  uint32_t tx_eq_pre;
  uint32_t tx_eq_post;
  uint32_t tx_eq_attn;

  // TF2 serdes settings
  serdes_lane_tx_eq_t tf2_tx_eq[2];  // 0: NRZ 1:PAM4

} lane_map_t;

// Connector map
typedef struct bf_conn_map_t {
  uint32_t connector;
  uint32_t device_id;
  uint32_t media_type;
  uint32_t mac_block;
  lane_map_t lane[MAX_CHAN_PER_CONNECTORS];
  uint32_t is_internal_port;
  int chip_type;

  // internally derived
  bool lane_present[MAX_CHAN_PER_CONNECTORS];
  uint32_t mark_port_as_ready;
  bool enable_auto_neg;
  char pack_pin_name[50];
  uint32_t nlanes_per_ch;
  uint32_t lane_arr[2];
} bf_conn_map_t;

typedef struct bf_board_map_t {
  bool cfg_found;                          // cfg file found
  bool cfg_parsed_ok;                      // incase parsing files mark it.
  bool device_id_cfged[BF_MAX_DEV_COUNT];  // derived from conn_map
  uint32_t num_of_connectors;

  // From conf file
  bool enable_debug_log;  // display the parsed configurations
  bool is_hardware;       // default hardware. user can set it 0 for simulation
  bf_conn_map_t *cmap;
} bf_board_map_t;

// reutrns 0 if success or no-board file config found.
// returns -1 if any, while parsing the file
int bf_hw_porting_handle_board_config_map(
    int *board_conf_present, bf_switchd_internal_context_t *switchd_ctx);

int bf_hw_cfg_bd_num_port_get(void);

bf_conn_map_t *bf_pltfm_conn_map_find(uint32_t connector, uint32_t channel);

int bf_hw_cfg_bd_cfg_found_get(void);
#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_HW_PORTING_CONFIG_H_ */
