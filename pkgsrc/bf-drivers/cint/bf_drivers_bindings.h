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

#include <bf_types/bf_types.h>
#include <mc_mgr/mc_mgr_intf.h>
#include <mc_mgr/mc_mgr_types.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_devport_mgr.h>
#include <tofino/pdfixed/pd_mc.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_ms.h>
#include <tofino/pdfixed/pd_pkt.h>
#include <tofino/pdfixed/pd_plcmt.h>
#include <tofino/pdfixed/pd_tm.h>
#include <tofino/bf_pal/dev_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/bf_pal/pltfm_func_mgr.h>
#include <tofino/bf_pal/pltfm_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/bf_packetpath_counter.h>
#include <dru_sim/dru_sim.h>
#include <dru_sim/dru_mti.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/bf_drv_profile.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <lld/bf_dma_dr_id.h>
#include <lld/lld_spi_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_reg_if.h>
#include <lld/bf_dev_if.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr_port_evt.h>
#include <port_mgr/port_mgr_intf.h>
#include <port_mgr/bf_serdes_if.h>
#include <port_mgr/bf_fsm_hdlrs.h>
#include <traffic_mgr/traffic_mgr.h>
#include <traffic_mgr/tm_intf.h>
#include <traffic_mgr/traffic_mgr_types.h>
#include <traffic_mgr/traffic_mgr_porting.h>
#include <traffic_mgr/traffic_mgr_config.h>
#include <traffic_mgr/traffic_mgr_ppg_intf.h>
#include <traffic_mgr/traffic_mgr_q_intf.h>
#include <traffic_mgr/traffic_mgr_port_intf.h>
#include <traffic_mgr/traffic_mgr_pool_intf.h>
#include <traffic_mgr/traffic_mgr_pipe_intf.h>
#include <traffic_mgr/traffic_mgr_sch_intf.h>
#include <traffic_mgr/traffic_mgr_apimode.h>
#include <traffic_mgr/traffic_mgr_miscapi.h>
#include <traffic_mgr/traffic_mgr_mcast.h>
#include <traffic_mgr/traffic_mgr_read_apis.h>
#include <traffic_mgr/traffic_mgr_counters.h>
#include <pkt_mgr/bf_pkt.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include <tofino_regs/tofino.h>
#include <diag/bf_dev_diag.h>
