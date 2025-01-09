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


#ifndef lld_serdes_sbus_map_h_included
#define lld_serdes_sbus_map_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  IP_UNPOPULATED = 0,
  IP_TYPE_PCIE_PLL,
  IP_TYPE_CORE_PLL,
  IP_TYPE_MAC_PLL,
  IP_TYPE_PCIE_PMA,
  IP_TYPE_PCIE_PCS,
  IP_TYPE_ETH_PMA,
  IP_TYPE_TEMP_SENSOR,
} port_mgr_sbus_ip_type_e;

typedef enum {
  bf_chip_tofino = 0,
  bf_chip_trestles,
} bf_chip_family_e;

typedef struct port_mgr_sbus_node_map_t {
  port_mgr_sbus_ip_type_e ip_type;
  int inst;
  int sub_inst;
  char *desc;
} port_mgr_sbus_node_map_t;

typedef struct port_mgr_sbus_map_t {
  int num_nodes;
  port_mgr_sbus_node_map_t *node_map;
} port_mgr_sbus_map_t;

typedef struct port_mgr_sbus_ring_map_t {
  int num_rings;
  port_mgr_sbus_map_t *sbus_map[2];
} port_mgr_sbus_ring_map_t;

void port_mgr_configure_sbus_map(bf_dev_id_t dev_id);
int port_mgr_num_sbus_rings_get(bf_dev_id_t dev_id);
int port_mgr_num_sbus_nodes_get(bf_dev_id_t dev_id, int ring);

int port_mgr_find_addr_for(bf_dev_id_t dev_id,
                           port_mgr_sbus_ip_type_e ip_type,
                           int inst,
                           int sub_inst,
                           int *ring,
                           int *sd);
void port_mgr_get_nodes_of_type(bf_dev_id_t dev_id,
                                int ring,
                                port_mgr_sbus_ip_type_e ip_type,
                                int *ip_nodes,
                                int *n_nodes);
int port_mgr_find_mac_info_for(bf_dev_id_t dev_id,
                               int ring,
                               int sd,
                               port_mgr_sbus_ip_type_e *ip_type,
                               int *inst,
                               int *sub_inst);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // lld_serdes_sbus_map_h_included
