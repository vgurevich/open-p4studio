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


/*
 * This file contains APIs for client application to
 * read Traffic Manager drop, usage counters.
 */

#ifndef __TRAFFIC_MGR_COUNTERS_H__
#define __TRAFFIC_MGR_COUNTERS_H__

#include <traffic_mgr/traffic_mgr_types.h>
#include <bf_types/bf_types.h>

/*
 * These enum values can be used to construct a mask when the
 * users want to clear a subset of the block level counters.
 * The mask is used by bf_tm_tofino_blklvl_clear_drop_cntrs().
 */
typedef enum bf_tm_block_level_counter_type {
  WAC_NO_DEST_DROP = (1 << 0),
  QAC_NO_DEST_DROP = (1 << 1),
  WAC_BUF_FULL_DROP = (1 << 2),
  PSC_PKT_DROP = (1 << 3),
  EGRESS_PIPE_TOTAL_DROP = (1 << 4),
  PEX_TOTAL_DISC = (1 << 5),
  QAC_TOTAL_DISC = (1 << 6),
  TOTAL_DISC_DQ = (1 << 7),
  PRE_TOTAL_DROP = (1 << 8),
  QAC_PRE_MC_DROP = (1 << 9),

  COUNTERS_ALL = (1 << 30) | 0x3FFu
} bf_tm_block_level_counter_type;

typedef struct _bf_tm_blklvl_cntrs {
  uint64_t wac_no_dest_drop;   // wac_reg.ctr_drop_no_dst
  uint64_t qac_no_dest_drop;   // qac_reg.qac_ctr32_drop_no_dst
  uint64_t wac_buf_full_drop;  // wac_reg.wac_drop_buf_full.wac_drop_buf_full
  uint64_t psc_pkt_drop;       // psc pkt_dropcnt.pkt_dropcnt
  uint64_t egress_pipe_total_drop;  // pkt_dropcnt.pkt_dropcnt
  uint64_t pex_total_disc;          // pex_dis_cnt.pex_dis_cnt
  uint64_t qac_total_disc;          // qac_dis_cnt.qac_dis_cnt
  uint64_t total_disc_dq;           // tot_dis_dq_cnt.tot_dis_dq_cnt
  uint64_t pre_total_drop;          // PRE packet_drop.packet_drop
  uint64_t qac_pre_mc_drop;         // qac_reg.qac_ctr32_pre_mc_drop

  uint64_t valid_sop_cntr;           // wac_reg.ctr_vld_sop
  uint64_t ph_lost_cntr;             // pre.ph_lost
  uint64_t cpu_copy_cntr;            // pre.cpu_copies
  uint64_t total_ph_processed;       // pre.ph_processed
  uint64_t total_copied_cntr;        // pre.total_copies
  uint64_t total_xid_prunes_cntr;    // pre.xid_prunes
  uint64_t total_yid_prunes_cntr;    // pre.yid_prunes
  uint64_t ph_in_use_cntr;           // psc.psc_ph_used
  uint64_t clc_total_pkt_cntr;       // clc.tot_pkt_cnt
  uint64_t clc_total_cell_cntr;      // clc.tot_cell_cnt
  uint64_t eport_total_pkt_cntr;     // pex.port_pkt_cnt
  uint64_t eport_total_cell_cntr;    // pex.port_cell_cnt
  uint64_t pex_total_pkt_cntr;       // pex.tot_pkt_cnt
  uint64_t total_enq_cntr;           // qlc.tot_eq_cnt
  uint64_t total_deq_cntr;           // qlc.tot_dq_cnt
  uint64_t caa_used_blocks;          // tm_caa.epipe.blks_usecnt
  uint64_t psc_used_blocks;          // psc_common.epipe.blks_usecnt
  uint64_t qid_enq_cntr;             // qlc.qid_eq_cnt
  uint64_t qid_deq_cntr;             // qlc.qid_deq_cnt
  uint64_t qac_query_cntr;           // prc.qac_cnt
  uint64_t qac_query_zero_cntr;      // prc.qac_zero_cnt
  uint64_t prc_total_pex_cntr;       // prc.pex_cnt
  uint64_t prc_total_pex_zero_cntr;  // prc.pex_zero_cnt
} bf_tm_blklvl_cntrs_t;

/*
 *                  FIFO related counters
 */

typedef struct _bf_tm_pre_fifo_cntrs {
  uint64_t wac_drop_cnt_pre0_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre1_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre2_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre3_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre4_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre5_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre6_fifo[BF_PRE_FIFO_COUNT];
  uint64_t wac_drop_cnt_pre7_fifo[BF_PRE_FIFO_COUNT];
} bf_tm_pre_fifo_cntrs_t;

/**
 * @file traffic_mgr_counters.h
 * @brief This file contains APIs for Traffic Manager application to
 *        get drop and usage counters from hardware.
 */

/**
 * @addtogroup tm-stats
 * @{
 * Description of APIs for Traffic Manager application to
 * get counters from hardware. All counters in hardware are maintained
 * using 48bits. All usage limits are in units of cells.
 */

/**
 * Get per port drop count.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_get(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_dev_port_t port,
                                uint64_t *ig_count,
                                uint64_t *eg_count);

/**
 * Get per port ingress drop count.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_get(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        bf_dev_port_t port,
                                        uint64_t *ig_count);

/**
 * Get per port egress drop count.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_get(bf_dev_id_t dev,
                                       bf_dev_pipe_t pipe,
                                       bf_dev_port_t port,
                                       uint64_t *eg_count);
/**
 * Set per port drop count cache
 * This data will not be pushed to hardware.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_cache_set(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_dev_port_t port,
                                      uint64_t ig_count,
                                      uint64_t eg_count);

/**
 * Set ingress per port drop count cache.
 * This data will not be pushed to hardware.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_ingress_cache_set(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint64_t drop_count);

/**
 * Set egress per port drop count cache.
 * This data will not be pushed to hardware.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_egress_cache_set(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint64_t drop_count);

/**
 * Get per port egress drop count.
 * On Egress, queue tail drop are accounted against port.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port identifier.
 * @param[in] color         Color
 * @param[out] count        Per port Packet drops from Egress TM
 *                          perspective
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_egress_color_drop_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_color_t color,
                                             uint64_t *count);

/**
 * Get per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param ig_count      Per port Packet drops from Ingress TM perspective.
 * @param eg_count      Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_cache_get(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bf_dev_port_t port,
                                      uint64_t *ig_count,
                                      uint64_t *eg_count);

/**
 * Get egress per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Egress, queue tail drop are also accounted against port.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Egress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_egress_port_drop_cache_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bf_dev_port_t port,
                                             uint64_t *drop_count);

/**
 * Get ingress per port drop count cached 64 bit counter to account for the
 * counter wrap.
 * On Ingress, if packet is dropped when usage crosses PPG or
 * or Port drop limit, this counter gets incremented.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param drop_count    Per port Packet drops from Ingress TM perspective.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ingress_port_drop_cache_get(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe,
                                              bf_dev_port_t port,
                                              uint64_t *drop_count);

/**
 * Clear per port drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_drop_count_clear(bf_dev_id_t dev, bf_dev_port_t port);

/**
 * Clear per port ingress_drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_ingress_drop_count_clear(bf_dev_id_t dev,
                                                bf_dev_port_t port);

/**
 * Clear per port egress_drop count.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_port_egress_drop_count_clear(bf_dev_id_t dev,
                                               bf_dev_port_t port);

/**
 * Get per ppg drop count.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_get(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               bf_tm_ppg_hdl ppg,
                               uint64_t *count);

/**
 * Set per ppg drop count cache.
 * This data will not be pushed to hardware.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_cache_set(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_tm_ppg_hdl ppg,
                                     uint64_t count);

/**
 * Get per ppg drop count cached 64 bit counter to account for the
 * counter wrap.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param count         Counts number of dropped packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_cache_get(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     bf_tm_ppg_hdl ppg,
                                     uint64_t *count);

/**
 * Clear per ppg drop count.
 *
 * @param dev           ASIC device identifier.
 * @param ppg           PPG identifier.
 * @return              Status of API call.
 */
bf_status_t bf_tm_ppg_drop_count_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg);

/**
 * Get per queue drop count.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_get(bf_dev_id_t dev,
                             bf_dev_pipe_t pipe,
                             bf_dev_port_t port,
                             bf_tm_queue_t queue,
                             uint64_t *count);

/**
 * Get per queue drop count per TM Subdevice TF3 aand later.
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_get_ext(bf_dev_id_t dev,
                                 bf_subdev_id_t die_id,
                                 bf_dev_pipe_t pipe,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 uint64_t *count);

/**
 * Set per queue drop count.
 * This data will not be pushed to hardware.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_cache_set(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint64_t count);

/**
 * Get per queue drop count cached 64 bit counter to account for the
 * counter wrap.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_cache_get(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint64_t *count);

/**
 * Clear per queue drop count
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_drop_count_clear(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     bf_tm_queue_t queue);

/**
 * Get Buffer full drop  count
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param count         Counts number of packet.
 * @return              Status of API call.
 */
bf_status_t bf_tm_pipe_buffer_full_drop_get(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            uint64_t *count);

/**
 * Clear Buffer full drop count
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_pipe_buffer_full_drop_clear(bf_dev_id_t dev,
                                              bf_dev_pipe_t pipe);

/**
 * Get port current usage count in units of cells.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param port       port identifier.
 * @param ig_count   Port usage count in cells from Ingress TM perspective.
 * @param eg_count   Port usage count in cells from Egress TM perspective.
 * @param ig_wm      Watermark of port in units of cell from
 *                   Ingress TM view point.
 * @param eg_wm      Watermark of port in units of cell from
 *                   Egress TM view point.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_usage_get(bf_dev_id_t dev,
                                 bf_dev_pipe_t pipe,
                                 bf_dev_port_t port,
                                 uint32_t *ig_count,
                                 uint32_t *eg_count,
                                 uint32_t *ig_wm,
                                 uint32_t *eg_wm);

/**
 * Clear port watermark counter.
 *
 * @param dev        ASIC device identifier.
 * @param port       port identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_watermark_clear(bf_dev_id_t dev, bf_dev_port_t port);

/**
 * Clear port ingress watermark counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_ingress_watermark_clear(bf_dev_id_t dev,
                                               bf_dev_port_t port);

/**
 * Clear port ingress usage counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_ingress_usage_count_clear(bf_dev_id_t dev,
                                                 bf_dev_port_t port);

/**
 * Clear port egress usage counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_egress_usage_count_clear(bf_dev_id_t dev,
                                                bf_dev_port_t port);

/**
 * Clear port egress watermark counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       port identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_port_egress_watermark_clear(bf_dev_id_t dev,
                                              bf_dev_port_t port);

/**
 * Get Pool usage counters. Valid pools are application/shared pool,
 * negative mirror pool and skid pool. There is no water mark support
 * for negative mirror pool. In negative mirror pool case water mark
 * value will be zero all the time.
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev        ASIC device identifier.
 * @param pool       pool identifier.
 * @param count      Pool usage count in cells.
 * @param wm         Water mark value in units of cell.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_usage_get(bf_dev_id_t dev,
                                 bf_tm_app_pool_t pool,
                                 uint32_t *count,
                                 uint32_t *wm);

/**
 * Clear shared AP pool watermark counter
 *
 * @param dev        ASIC device identifier.
 * @param pool       pool identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pool_watermark_clear(bf_dev_id_t dev, bf_tm_app_pool_t pool);

/**
 * Get PPG usage count.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param ppg        PPG identifier.
 * @param gmin_count   Cell inuse from gmin pool.
 * @param shared_count Cell inuse from shared pool.
 * @param skid_count   Cell inuse from skid pool.
 * @param wm         Water mark value in units of cell.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_usage_get(bf_dev_id_t dev,
                                bf_dev_pipe_t pipe,
                                bf_tm_ppg_hdl ppg,
                                uint32_t *gmin_count,
                                uint32_t *shared_count,
                                uint32_t *skid_count,
                                uint32_t *wm);

/**
 * @brief Clear PPG usage count. [gmin, shared, skid, watermark]
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        PPG identifier.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_usage_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg);

/**
 * Clear PPG watermark counter.
 *
 * @param dev        ASIC device identifier.
 * @param ppg        PPG identifier.
 * @return           Status of API call.
 */
bf_status_t bf_tm_ppg_watermark_clear(bf_dev_id_t dev, bf_tm_ppg_hdl ppg);

/**
 * Get queue usage count
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Cell inuse .
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_usage_get(bf_dev_id_t dev,
                              bf_dev_pipe_t pipe,
                              bf_dev_port_t port,
                              bf_tm_queue_t queue,
                              uint32_t *count,
                              uint32_t *wm);

/**
 * Get queue usage count per TM Subdevice TF3 aand later.
 * Water mark value indicates maximum usage ever reached
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @param count         Cell inuse .
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_usage_get_ext(bf_dev_id_t dev,
                                  bf_subdev_id_t die_id,
                                  bf_dev_pipe_t pipe,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  uint32_t *count,
                                  uint32_t *wm);

/**
 * Clear queue usage counter.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port identifier.
 * @param[in] queue         Queue identifier behind port.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_q_usage_clear(bf_dev_id_t dev,
                                bf_dev_port_t port,
                                bf_tm_queue_t queue);

/**
 * Clear queue watermark counter.
 *
 * @param dev           ASIC device identifier.
 * @param port          Port identifier.
 * @param queue         Queue identifier behind port.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_watermark_clear(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue);

/**
 * Get discard queue current usage counter.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param count         Cell inuse.
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_discard_usage_get(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t *count,
                                      uint32_t *wm);

/**
 * Get discard queue current usage counter per TM Subdevice TF3 aand later.
 * Water mark value indicates maximum usage ever reached.
 *
 * @param dev           ASIC device identifier.
 * @param die_id        TM Subdevice ID (Valid values for TF3.2 [0 - 1]).
 * @param pipe          Pipe Identifier.
 * @param count         Cell inuse.
 * @param wm            Water mark value in units of cell.
 * @return              Status of API call.
 */
bf_status_t bf_tm_q_discard_usage_get_ext(bf_dev_id_t dev,
                                          bf_subdev_id_t die_id,
                                          bf_dev_pipe_t pipe,
                                          uint32_t *count,
                                          uint32_t *wm);

/**
 * Clear discard queue watermark counter.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_q_discard_watermark_clear(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe);

/**
 * Get Pipe counters. Total number of cells, packets received by a pipe.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param cell_count Total Cell count through the pipe.
 * @param pkt_count  Total Cell count through the pipe.
 * @return           Status of API call.
 */
bf_status_t bf_tm_pipe_counters_get(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint64_t *cell_count,
                                    uint64_t *pkt_count);

/**
 * Clear Pipe cell counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_cell_counter(bf_dev_id_t dev, bf_dev_pipe_t pipe);

/**
 * Clear Pipe packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_packet_counter(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe);

/**
 * Get Unicast and Multicast cut through packet count through a pipe.
 * Counters are incremented after packet is admitted into TM in egress
 * direction.
 *
 * @param dev        ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param uc_count   Total Unicast cut through packets through the pipe.
 * @param mc_count   Total Multicast cut through packets through the pipe.
 * @return           Status of API call.
 */
bf_status_t bf_tm_cut_through_counters_get(bf_dev_id_t dev,
                                           bf_dev_pipe_t pipe,
                                           uint64_t *uc_count,
                                           uint64_t *mc_count);

/**
 * Clear unicast cut-through packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_uc_ct_packet_counter(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe);

/**
 * Clear multicast cut-through packet counter.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier.
 * @return               Status of API call.
 */
bf_status_t bf_tm_pipe_clear_mc_ct_packet_counter(bf_dev_id_t dev,
                                                  bf_dev_pipe_t pipe);

/**
 * Get blocklevel drop counters for TM.
 * Blocklevel Drops, Error, or Discard Counters
 *
 * @param dev           ASIC device identifier.
 * @param pipe          Pipe Identifier.
 * @param blk_cntrs     Block Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_blklvl_drop_get(bf_dev_id_t dev,
                                  bf_dev_pipe_t pipe,
                                  bf_tm_blklvl_cntrs_t *blk_cntrs);

/**
 * Clear blocklevel drop counters for TM.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] pipe          Pipe Identifier.
 * @param[in] clear_mask    A mask of counters to be cleared
 * @return                  Status of API call.
 */
bf_status_t bf_tm_blklvl_drop_clear(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint32_t clear_mask);

/**
 * Get PRE FIFO drop counters for TM.
 * PRE-FIFO Drops, Error, or Discard Counters
 *
 * @param dev           ASIC device identifier.
 * @param fifo_cntrs    PRE FIFO Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_pre_fifo_drop_get(bf_dev_id_t dev,
                                    bf_tm_pre_fifo_cntrs_t *fifo_cntrs);

/**
 * Clear PRE FIFO drop counters for TM.
 *
 * @param dev           ASIC device identifier.
 * @param fifo_cntrs    PRE FIFO Level Counters (Out)
 * @return              Status of API call.
 */
bf_status_t bf_tm_pre_fifo_drop_clear(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      uint32_t fifo);

/**
 * Stop timer to poll counters for TM.
 *
 * @param dev           dev
 * @return              Status of API call.
 */
bf_status_t bf_tm_stop_cache_counters_timer(bf_dev_id_t dev);

/**
 * Start timer to poll counters for TM.
 *
 * @param dev           dev
 * @return              Status of API call.
 */
bf_status_t bf_tm_start_cache_counters_timer(bf_dev_id_t dev);

#if 0
/**
 * Get per port number of packets
 *
 * @param dev        ASIC device identifier.
 * @param port       Port Identifier.
 * @param cell_count Total Cell count through the pipe.
 * @param pkt_count  Total Cell count through the pipe.
 * @return           Status of API call.
 */
bf_status_t bf_tm_port_stats_get(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 uint32_t *ig_pkt_count,
                                 uint32_t *eg_pkt_count);
#endif

/* @} */

#endif
