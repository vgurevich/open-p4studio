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


#ifndef __TRAFFIC_MGR_READ_APIS_H__
#define __TRAFFIC_MGR_READ_APIS_H__

/**
 * @file traffic_mgr_read_apis.h
 * @brief This file contains APIs for Traffic Manager application to
 *        read/get configuration from hardware.
 */

#include <bf_types/bf_types.h>
#include <traffic_mgr/traffic_mgr_types.h>

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Get Egress pipe limit.
 * Default value of the pipe limit is set to maximum buffering capability
 * of the traffic manager.
 *
 * When admitting packet into Traffic manager, apart from other
 * checks, the packet has to also pass usage check on per egress pipe
 * usage limit. A packet destined to egress pipe whose limit  has
 * crossed, will not be admitted.
 *
 * Related API: bf_tm_pipe_egress_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier
 * @param[out] cells     Limits in terms of number of cells.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_egress_limit_get(bf_dev_id_t dev,
                                        bf_dev_pipe_t pipe,
                                        uint32_t *cells);

/**
 * @brief Get Egress pipe default limit.
 * Default value of the pipe limit is set to maximum buffering capability
 * of the traffic manager.
 *
 * When admitting packet into Traffic manager, apart from other
 * checks, the packet has to also pass usage check on per egress pipe
 * usage limit. A packet destined to egress pipe whose limit  has
 * crossed, will not be admitted.
 *
 * Related API: bf_tm_pipe_egress_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier
 * @param[out] cells     Default limit in terms of number of cells.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_egress_limit_get_default(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t *cells);

/**
 * @brief Get egress pipe hysteresis limit.
 * When usage of cells goes below the hysteresis
 * limit, pipe level drop condition  will be cleared.
 *
 * Related API: bf_tm_pipe_egress_hysteresis_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier
 * @param[out] cells     Limits in terms of number of cells.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_egress_hysteresis_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint32_t *cells);

/**
 * @brief Get egress pipe default hysteresis limit.
 * When usage of cells goes below the hysteresis
 * limit, pipe level drop condition will be cleared.
 *
 * Related API: bf_tm_pipe_egress_hysteresis_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe Identifier
 * @param[out] cells     Default limit in terms of number of cells.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_egress_hysteresis_get_default(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     uint32_t *cells);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Check the pipe is valid.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  pipe                Device pipe to validate.
 *
 * @return BF_SUCCESS          :   If the pipe is valid for the device.
 *
 */
bf_status_t bf_tm_pipe_is_valid(bf_dev_id_t dev, bf_dev_pipe_t pipe);

/**
 * @brief Check the port is valid.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  dev_port            Device port to validate.
 *
 * @return BF_SUCCESS          :   If the port is valid for the device.
 *
 */
bf_status_t bf_tm_port_is_valid(bf_dev_id_t dev, bf_dev_port_t dev_port);

/**
 * @brief This API can be used to get queue count and mapping of a port.
 *
 * Related APIs: bf_tm_port_q_mapping_set ()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port handle.
 * @param[out] q_count    Number of queues used for the port.
 * @param[out] q_mapping  Array of integer values specifying queue mapping
 *                        Mapping is indexed by ig_intr_md.qid.
 *                        Value q_mapping[ig_intr_md.qid] is port's QID
 *                        Caller has to provide array of size 32 (TOF1) or
 *                        128 (TOF2)
 *                        .
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_q_mapping_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint8_t *q_count,
                                     uint8_t *q_mapping);

/**
 * @brief This API can be used to get physical queue for particular port and
 * ingress qid.
 *
 * @param[in] dev          ASIC device identifier.
 * @param[in] port         port handle.
 * @param[in] ingress_qid  ingress qid.
 * @param[out] log_pipe    logical pipe ID.
 * @param[out] phys_q      physical queue ID.
 *
 * @return                 Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_pipe_physical_queue_get(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               uint32_t ingress_qid,
                                               bf_dev_pipe_t *log_pipe,
                                               uint32_t *phys_q);

/**
 * @brief This API can be used to get port and ingress qid list for
 * particular physical queue
 *
 * @param[in] dev              ASIC device identifier.
 * @param[in] log_pipe         Logical pipe ID.
 * @param[in] pipe_queue       Physical pipe-related queue ID.
 * @param[out] port            Port handle {logical pipe id, port id}.
 * @param[out] qid_count       Number of ingress qids (first ingress_q_count
 *                             in array is significant).
 * @param[out] qid_list        Ingress qids list.
 *                             :
 *                             Caller has to provide array of size 32 for TOF or
 *                             128 for TOF2
 *                             if physical queue is not currently mapped, port
 *                             is set to the port's group first port and
 *                             qid_count is set to the 0
 *
 * @return                     Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_queue_qid_list_get(bf_dev_id_t dev,
                                          bf_dev_pipe_t log_pipe,
                                          bf_tm_queue_t pipe_queue,
                                          bf_dev_port_t *port,
                                          uint32_t *qid_count,
                                          bf_tm_queue_t *qid_list);

/**
 * @brief Get Queue App pool, limit configuration
 * A queue can be optionally assigned to any application pool.
 * When assigned to application pool, get static or dynamic shared limit
 *
 * Related APIs: bf_tm_q_app_pool_usage_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port handle.
 * @param[in] queue           Queue identifier. Valid range [ 0..31 ] TOF1
 *                            [ 0..127 ] TOF2
 * @param[out] pool           Application pool to which queue is assigned to.
 *                            Valid values are BF_TM_EG_POOL0..3.
 * @param[out] base_use_limit Limit to which PPG can grow inside application
 *                            pool. Once this limit is crossed, if queue burst
 *                            absroption factor (BAF) is non zero, depending
 *                            availability of buffer, queue is allowed to
 *                            use buffer upto BAF limit. If BAF limit is zero,
 *                            queue is treated as static and no dynamic
 *                            thresholding.
 * @param[out] dynamic_baf    One of the values listed in bf_tm_queue_baf_t.
 *                            When BF_TM_QUEUE_BAF_DISABLE is used, queue uses
 *                            static limit.
 * @param[out] hysteresis     Hysteresis value of queue.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */

bf_status_t bf_tm_q_app_pool_usage_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       bf_tm_app_pool_t *pool,
                                       uint32_t *base_use_limit,
                                       bf_tm_queue_baf_t *dynamic_baf,
                                       uint32_t *hysteresis);

/**
 * @brief bf_tm_q_app_pool_usage_get_default
 * Default shared pool settings for a queue.
 *
 * Related APIs: bf_tm_q_app_pool_usage_get()
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[out] pool            Default application pool to which a queue
 *                             is assigned.
 * @param[out] base_use_limit  Default limit to which a queue can grow inside
 *                             an application pool.
 * @param[out] dynamic_baf     Default BAF setting for a queue.
 * @param[out] hysteresis      Default Hysteresis value of a queue.
 * @return                     Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_app_pool_usage_get_default(bf_dev_id_t dev,
                                               bf_tm_app_pool_t *pool,
                                               uint32_t *base_use_limit,
                                               bf_tm_queue_baf_t *dynamic_baf,
                                               uint32_t *hysteresis);

/**
 * @brief bf_tm_q_app_poolid_get
 * A queue can be optionally assigned to any application pool.
 * When assigned to application pool, get pool ID
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            port handle.
 * @param[in] queue           queue identifier. Valid range [0..31] for
 *                            Tofino and [0..127] for Tofino2
 * @param[out] pool           Application pool to which queue is assigned to.
 *                            Valid values are BF_TM_EG_POOL0..3.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */

bf_status_t bf_tm_q_app_poolid_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bf_tm_app_pool_t *pool);

/**
 * @brief Get queue fast recovery mode.
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[in]  port            Port handle.
 * @param[in]  queue           Queue identifier. Valid range [0..31] for
 *                             Tofino and [0..127] for Tofino2
 * @param[out] fast_recovery   Queue fast recovery mode.
 * @return                     Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_fast_recovery_mode_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_queue_t queue,
                                           bool *fast_recovery);

/**
 * @brief bf_tm_q_hysteresis_get
 * Get queue hysteresis in terms of buffer cells.
 *
 * Related APIs: bf_tm_q_hysteresis_set()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue identifier. Valid range [ 0..31 ] TOF1
 *                    [ 0..127 ] TOF2
 * @param[out] cells  Queue hysteresis specified in cell count.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_hysteresis_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   uint32_t *cells);

/**
 * @brief bf_tm_q_app_pool_limit_get
 * Get queue limit set at a shared application pool.
 * The queue does not use a shared pool if the cells value is zero.
 *
 * Related APIs: bf_tm_q_app_pool_usage_get()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue identifier. Valid range [ 0..31 ] TOF1
 *                    [ 0..127 ] TOF2
 * @param[out] cells  Queue limit set at a shared pool in cell count.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_app_pool_limit_get(bf_dev_id_t dev,
                                       bf_dev_port_t port,
                                       bf_tm_queue_t queue,
                                       uint32_t *cells);

/**
 * @brief Get queue min limits.
 * Returned limits are accounted in terms of cells.
 *
 * Related APIs: bf_tm_q_guaranteed_min_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port handle.
 * @param[in] queue      Queue identifier. Valid range [ 0..31 ] TOF1
 *                       [ 0..127 ] TOF2
 * @param[out] cells          Queue limits specified in cell count
 * @return               Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_guaranteed_min_limit_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             bf_tm_queue_t queue,
                                             uint32_t *cells);

/**
 * @brief bf_tm_q_guaranteed_min_limit_get_default
 * Get default minmum buffer limit for a queue.
 *
 * Related APIs: bf_tm_q_guaranteed_min_limit_get()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[out] cells  Default minimum buffer limit for a queue in cell count.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_guaranteed_min_limit_get_default(bf_dev_id_t dev,
                                                     uint32_t *cells);

/**
 * @brief bf_tm_q_tail_drop_get
 * Get queue tail drop enable mode.
 *
 * Related APIs: bf_tm_q_tail_drop_enable ()
 *               bf_tm_q_tail_drop_disable ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose tail drop mode has to be set.
 * @param[out] mode   Tail drop mode current setting.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_tail_drop_get(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bf_tm_queue_t queue,
                                  bool *mode);

/**
 * @brief bf_tm_q_drop_state_get
 * Get queue drop state.
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose color drop state has to be get.
 * @param[in]  color  Color
 * @param[out] state  Color drop state.
 * @return            Status of the API call.
 * BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_drop_state_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bf_tm_color_t color,
                                   bool *state);

/**
 * @brief bf_tm_q_tail_drop_get_default
 * Get default tail drop mode for a queue.
 *
 * Related APIs: bf_tm_q_tail_drop_get ()
 *
 * @param[in]  dev   ASIC device identifier.
 * @param[out] mode  Tail drop default mode.
 * @return           Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_tail_drop_get_default(bf_dev_id_t dev, bool *mode);

/**
 * @brief bf_tm_q_color_drop_get
 * Get queue color drop enable mode.
 *
 * Related APIs: bf_tm_q_color_drop_enable ()
 *               bf_tm_q_color_drop_disable ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Port handle.
 * @param[in]  queue  Queue whose color drop mode has to be set.
 * @param[out] mode   Color drop mode current setting.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_drop_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   bf_tm_queue_t queue,
                                   bool *mode);

/**
 * @brief bf_tm_q_color_drop_get
 * Get queue color drop default mode.
 *
 * Related APIs: bf_tm_q_color_drop_enable ()
 *               bf_tm_q_color_drop_disable ()
 *               bf_tm_q_color_drop_get ()
 *
 * @param[in]  dev   ASIC device identifier.
 * @param[out] mode  Color drop mode default setting.
 * @return           Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_drop_get_default(bf_dev_id_t dev, bool *mode);

/**
 * @brief Get color drop limits for queue.
 *
 * Related APIs: bf_tm_q_color_limit_set ()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port handle.
 * @param[in] queue      Queue whose dynamic limits has to be adjusted.
 * @param[in] color      Color (RED, YELLOW)
 * @param[out] limit     Color Limit is specified in percentage of guaranteed
 *                       queue limit.
 *                       Green Color limit is equal to queue limit.
 *                       For yellow, red, limit obtained is percentage of
 *                       overall queue share limit. Once queue usage reaches
 *                       the limit, appropriate colored packets are tail
 *                       dropped.
 *                       To get GREEN Color limit use
 *                       bf_tm_q_guaranteed_min_limit_get()
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_limit_get(bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bf_tm_queue_t queue,
                                    bf_tm_color_t color,
                                    bf_tm_queue_color_limit_t *limit);

/**
 * @brief bf_tm_q_color_limit_get_default
 * Get color drop limit defaults for queues.
 *
 * Related APIs: bf_tm_q_color_limit_get ()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  color  Color (RED, YELLOW)
 * @param[out] limit  HW default color limit.
 * @return            Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_limit_get_default(bf_dev_id_t dev,
                                            bf_tm_color_t color,
                                            bf_tm_queue_color_limit_t *limit);

/**
 * @brief Get drop state shadow for queue.
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port handle
 * @param[in] queue      queue whose shadow register has to be read.
 * @param[out] state     32-bit state:
 *                       per queue 4 bits states (green, yellow, red, min)
 *                       [3:0] -- Qi in PIPE0,
 *                       [7:4] -- Qi in PIPE 1,
 *                       [11:8] -- Qi in PIPE2,
 *                       [15:12] -- Qi in PIPE3
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_drop_state_shadow_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_queue_t queue,
                                          uint32_t *state);

/**
 * @brief Get color hysteresis for queue.
 *
 * Related APIs: bf_tm_q_color_hysteresis_set ()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port handle.
 * @param[in] queue      Queue whose dynamic limits has to be adjusted.
 * @param[in] color      Color (RED, YELLOW, GREEN)
 * @param[out] cells     Number of cells queue usage drops to
 *                       when drop condition is cleared.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_hysteresis_get(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_color_t color,
                                         bf_tm_thres_t *cells);

/**
 * @brief bf_tm_q_color_hysteresis_get_default
 * Get color hysteresis defaults for queues.
 *
 * Related APIs: bf_tm_q_color_hysteresis_get ()
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  color       Color (RED, YELLOW)
 * @param[out] cells       HW default color hysteresis.
 * @return                 Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_color_hysteresis_get_default(bf_dev_id_t dev,
                                                 bf_tm_color_t color,
                                                 bf_tm_thres_t *cells);

/**
 * @brief Get Port Unicast Cut Through Limit
 * This API can be used to get cut through buffer size on per port basis.
 * The specified size is set aside for unicast traffic in cut through mode.
 *
 * Related APIs: bf_tm_port_uc_cut_through_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port handle.
 * @param[out] size      Size in terms of cells (upto 16). Valid value [1..15]
 *                       If size is set to zero, then cut through get disabled.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_uc_cut_through_limit_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                uint8_t *size);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Get total number of supported PPGs.
 *
 * Related APIs: bf_tm_ppg_unusedppg_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe identifier.
 * @param[out] total_ppg Pointer to unsigned integer location where total
 *                       supported PPG count will be stored.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_totalppg_get(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint32_t *total_ppg);

/**
 * @brief Get total number of unused PPGs.
 *
 * Related APIs: bf_tm_ppg_totalppg_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] pipe       Pipe identifier.
 * @param[out] total_ppg Pointer to unsigned integer location where
 *                       current unused PPG count will be stored.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_unusedppg_get(bf_dev_id_t dev,
                                    bf_dev_pipe_t pipe,
                                    uint32_t *unused_ppg);

/**
 * @brief Default TM buffering settings for a PPG.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  ppg_hdl             ppg handle.
 * @param[out] min_limit_cells     The PPG default guaranteed min limit
 *                                 in cells.
 * @param[out] hysteresis_cells    The PPG default hysteresis limit in cells.
 * @return                         Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_buffer_get_default(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg_hdl,
                                         uint32_t *min_limit_cells,
                                         uint32_t *hysteresis_cells);

/*
 * @brief Default shared pool settings for a PPG.
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_get()
 *
 * @param[in]  dev             ASIC device identifier.
 * @param[in]  ppg_hdl         ppg handle.
 * @param[out] pool            Default application pool to which the PPG
 *                             is assigned.
 * @param[out] pool_max_cells  Default limit to which the PPg can grow inside
 *                             an application pool.
 * @param[out] dynamic_baf     Default BAF setting for the PPG.
 * @return                     Status of API call.
 */
bf_status_t bf_tm_ppg_app_pool_usage_get_default(bf_dev_id_t dev,
                                                 bf_tm_ppg_hdl ppg_hdl,
                                                 bf_tm_app_pool_t *pool,
                                                 uint32_t *pool_max_cells,
                                                 bf_tm_ppg_baf_t *dynamic_baf);

/**
 * @brief Get default iCoS DPG map for the Port.
 *
 * Related APIs: bf_tm_ppg_defaultppg_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port_id     Device port identifier (pipe id and port number).
 * @param[out] icos_mask  Default iCoS bitmask with DPG assignments: 1 is set
 *                        if the iCoS is attached to the Port's DPG.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 *
 */
bf_status_t bf_tm_port_icos_map_get_default(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            uint8_t *icos_mask);

/**
 * @brief Get PPG identifiers for a Port iCoS map.
 *
 * Related APIs: bf_tm_ppg_icos_mapping_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port_id     Device port identifier (pipe id and port number).
 * @param[out] ppg_mask   iCoS bitmask of PPG assignments: 1 is set if the iCoS
 *                        is detached from Port's DPG and attached to a PPG.
 * @param[out] ppg_hdlrs  Map of the port's iCoS to PPG handlers.
 *                        Index is iCoS (0..BF_TM_MAX_PFC_LEVELS-1).
 *                        Each item is a PPG handler assigned to the iCoS
 *                        or zero if not. In the latter case the appropriate
 *                        ppg_mask bit is also zero. Items are not unique as
 *                        several iCoS might be assigned to the same PPG.
 * @return                Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_port_ppg_map_get(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   uint8_t *ppg_mask,
                                   bf_tm_ppg_hdl *ppg_hdlrs);

/**
 * @brief Get device port from PPG handle.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] ppg_hdl   PPG handle used for TM API calls.
 * @param[out] port_id  Device port identifier (pipe id and port number)
 *                      which was used to allocate the PPG handle.
 *                      It's current allocation status might be obsolete.
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_port_get(bf_dev_id_t dev,
                               bf_tm_ppg_hdl ppg_hdl,
                               bf_dev_port_t *port_id);

/**
 * @brief Get ppg number from PPG handle.
 *
 * Related APIs: bf_tm_ppg_allocate()
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] ppg_hdl   PPG handle used for TM API calls.
 * @param[out] ppg_nr   PPG number
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_nr_get(bf_dev_id_t dev,
                             bf_tm_ppg_hdl ppg_hdl,
                             bf_tm_ppg_id_t *ppg_nr);

/**
 * @brief Get TM Mirror Port default PG's handle.
 *
 * @param[in] dev       ASIC device identifier.
 * @param[in] pipe_id   Pipe identifier.
 * @param[out] ppg_hdl  Default PG handle of the Mirror port.
 * @return              Status of the API call.
 *  BF_SUCCESS on success.
 *  Non-Zero on other errors.
 */
bf_status_t bf_tm_ppg_mirror_port_handle_get(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe_id,
                                             bf_tm_ppg_hdl *ppg_hdl);

/**
 * @brief bf_tm_ppg_app_pool_id_get
 * A PPG can be optionally assigned to any application pool.
 * Get the application pool id of the ppg
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] ppg             Ppg handle.
 * @param[out] pool           Application pool to which PPG is assigned to.
 * @return                    Status of API call
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_app_pool_id_get(bf_dev_id_t dev,
                                      bf_tm_ppg_hdl ppg,
                                      uint32_t *pool);

/**
 * @brief Get PPG pool limit information
 * A non deafult PPG can be optionally assigned to any application pool.
 * When assigned to application pool, get static or dynamic shared limit
 *
 * Related APIs: bf_tm_ppg_app_pool_usage_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] ppg             Ppg handle.
 * @param[in] pool            Application pool to which PPG is assigned to.
 * @param[out] base_use_limit Limit to which PPG can grow inside application
 *                            pool. Once this limit is crossed, if PPG burst
 *                            absroption factor (BAF) is non zero, depending
 *                            availability of buffer, PPG is allowed to
 *                            use buffer upto BAF limit. If BAF limit is zero,
 *                            PPG is treated as static and no dynamic
 *                            thresholding.
 * @param[out] dynamic_baf    One of the values listed in bf_tm_ppg_baf_t.
 *                            When BF_TM_PPG_BAF_DISABLE is used, PPG uses
 *                            static limit.
 * @param[out] hysteresis     Hysteresis value.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_app_pool_usage_get(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg,
                                         bf_tm_app_pool_t pool,
                                         uint32_t *base_use_limit,
                                         bf_tm_ppg_baf_t *dynamic_baf,
                                         uint32_t *hysteresis);

/**
 * @brief Get Fast recovery mode for PPG
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] ppg             Ppg handle.
 * @param[out] fast_recover   Fast recovery mode
 * @return                    status of API call
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_fast_recovery_mode_get(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bool *fast_recover);

/**
 * @brief Get PPG miminum limits.
 * Returned limits are accounted in terms of cells.
 *
 * Related APIs: bf_tm_ppg_guaranteed_min_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        Ppg whose limits has to be adjusted.
 * @param[out] cells     Number of cells set as minimum limit
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_guaranteed_min_limit_get(bf_dev_id_t dev,
                                               bf_tm_ppg_hdl ppg,
                                               uint32_t *cells);

/**
 * @brief bf_tm_ppg_lossless_treatment_get
 * Get PFC(Lossless) enable flag value
 *
 * Related APIs: bf_tm_ppg_lossless_treatment_enable()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        Ppg whose lossless flag has to be fetched.
 * @param[out] pfc_flag  Value of the pfc flag
 * @return               Status of API call;
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_lossless_treatment_get(bf_dev_id_t dev,
                                             bf_tm_ppg_hdl ppg,
                                             bool *pfc_flag);

/**
 * @brief Get ppg skid limits.
 *
 * Related APIs: bf_tm_ppg_skid_limit_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        Ppg whose skid limits has to be fetched.
 * @param[out] cells     Limits in terms of number of cells
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_skid_limit_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     uint32_t *cells);

/**
 * Get ppg resume limit.
 *
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        ppg whose skid limits has to be fetched.
 * @param[out] limit     Limits in terms of number of cells
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_resume_limit_get(bf_dev_id_t dev,
                                       bf_tm_ppg_hdl ppg,
                                       uint32_t *limit);

/**
 * @brief Get PFC(Lossless) state for port (pfc has no final and remote/internal
 * status)
 *
 * Related APIs: bf_tm_port_pfc_state_set(), bf_tm_port_clear_pfc_state()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port whose pfc state has to be fetched.
 * @param[out] state     Bitmap for 8 iCOS (priority) PFC state
 *
 * @return status of API call;
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_pfc_state_get(bf_dev_id_t dev,
                                     bf_dev_port_t port,
                                     uint8_t *state);

/**
 * @brief Get Port PFC(Lossless) CoS enabled status
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port whose lossless enable bit has to be fetched.
 * @param[in] cos        CoS (0-7)
 * @param[out] enabled   CoS enabled
 * @return status of API call;
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_pfc_enable_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint8_t cos,
                                      bool *enabled);

/**
 * @brief Get Port PFC(Lossless) state (TOF2, TOF3) including final and
 * remote/internal pfc status)
 *
 * Related APIs: bf_tm_port_clear_pfc_state()
 *
 * @param[in] dev               ASIC device identifier.
 * @param[in] port              Port whose PFC extended state has to be get.
 * @param[out] port_ppg_state   Port PPG PFC state bitmap for each of 8 iCoS
 * @param[out] rm_pfc_state     Remote PFC state bitmap for each of 8 iCoS
 * @param[out] mac_pfc_out      MAC PFC state bitmap for each of 8 iCoS
 * @param[out] mac_pause_out    MAC pause state
 *
 * @return status of API call;
 */
bf_status_t bf_tm_port_pfc_state_get_ext(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         uint8_t *port_ppg_state,
                                         uint8_t *rm_pfc_state,
                                         uint8_t *mac_pfc_out,
                                         bool *mac_pause_out);

/**
 * @brief Get ppg allocation config.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] ppg           ppg whose icos mask has to be fetched.
 * @param[out] ppgid        ppg id
 * @param[out] poolid       Application Pool id
 * @param[out] default_ppg  Is ppg default
 * @return                  Status of API call.
 */
bf_status_t bf_tm_ppg_allocation_cfg_get(bf_dev_id_t dev,
                                         bf_tm_ppg_hdl ppg,
                                         uint32_t *ppgid,
                                         uint32_t *poolid,
                                         bool *default_ppg);

/**
 * @brief bf_tm_ppg_drop_state_get
 * Get ppg drop state
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        ppg whose drop state has to be fetched.
 * @param[out] state     State
 * @return           Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_drop_state_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     bool *state);

/**
 * @brief Get ppg hysteresis limits.
 * Same hysteresis limits are applied to PPGs limits inside MIN pool
 * and PPGs mapped to Skid Pool. Hysterisis limits are numbers of cells
 * the ppg usage should fall by from its limit value. Once usage limits
 * are below hysteresis, appropriate condition is cleared. Example when
 * PPG's skid usage limit falls below its allowed limits limit by
 * hysteresis value, drop condition is cleared.
 *
 * Related APIs: bf_tm_ppg_skid_hysteresis_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        Ppg whose hysteresis limits has to be fetched.
 * @param[out] cells     Limits in terms of number of cells
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_guaranteed_min_skid_hysteresis_get(bf_dev_id_t dev,
                                                         bf_tm_ppg_hdl ppg,
                                                         uint32_t *cells);

/**
 * @brief Get all iCoS (iCoS = ig_intr_md.ingress_cos) traffic is attached
 * to PPG
 *
 *
 * Related APIs: bf_tm_ppg_icos_mapping_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] ppg        Ppg handle
 * @param[out] icos_bmap Bit map of iCoS (iCoS = ig_intr_md.ingress_cos).
 *                       Bit 7 is interpreted as iCoS 7 that is attached.
 *                       ppg handle.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 *
 */
bf_status_t bf_tm_ppg_icos_mapping_get(bf_dev_id_t dev,
                                       bf_tm_ppg_hdl ppg,
                                       uint8_t *icos_bmap);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Get application pool size.
 * Size in units of cell set aside for application pool.
 *
 * Related APIs: bf_tm_pool_size_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            Pool identifier.
 * @param[out] cells          Size of pool in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_size_get(bf_dev_id_t dev,
                                bf_tm_app_pool_t pool,
                                uint32_t *cells);

/**
 * @brief Get color drop state for application pool
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] entry       Color.
 * @param[out] state      4-bit drop state for 4 pools
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pools_color_drop_state_get(bf_dev_id_t dev,
                                             bf_tm_color_t color,
                                             uint32_t *state);

/**
 * @brief Clear color drop state for application pool
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] entry       Color.
 * @param[out] state      4-bit drop state for 4 pools
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_color_drop_state_clear(bf_dev_id_t dev,
                                              bf_tm_color_t color);

/**
 * @brief Get Application pool color drop limits.
 *
 * Related APIs: bf_tm_pool_color_drop_limit_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            Pool handle.
 * @param[in] color           Color (Green, Yellow, Red)
 * @param[out] limit          Limits in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_color_drop_limit_get(bf_dev_id_t dev,
                                            bf_tm_app_pool_t pool,
                                            bf_tm_color_t color,
                                            uint32_t *limit);

/**
 * @brief Get Color drop hysteresis.
 * The same hysteresis value is applied on all application pools.
 * Resume condition is triggered when pool usage drops
 * by hysteresis value.
 *
 * Related APIs: bf_tm_pool_color_drop_hysteresis_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] color       Color (Green, Yellow, Red)
 * @param[out] limit      Limits in terms of cells.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_color_drop_hysteresis_get(bf_dev_id_t dev,
                                                 bf_tm_color_t color,
                                                 uint32_t *limit);

/**
 * @brief bf_tm_pool_color_drop_hysteresis_get_default
 * Get default color drop hysteresis for all the application pools.
 *
 * Related APIs: bf_tm_pool_color_drop_hysteresis_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] color       Color (Green, Yellow, Red)
 * @param[out] limit      Limits in terms of cells.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_color_drop_hysteresis_get_default(bf_dev_id_t dev,
                                                         bf_tm_color_t color,
                                                         uint32_t *limit);

/**
 * @brief Get per PFC level limit values.
 * PFC level limits are configurable on per application pool basis.
 * When usage numbers hit pfc limits, PAUSE is triggered
 * for lossless traffic or PFC enabled traffc.
 *
 * Related APIs: bf_tm_pool_pfc_limit_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            Pool handle for which limits are configured.
 * @param[in] icos            Internal CoS (iCoS = ig_intr_md.ingress_cos) level
 *                            on which limits are applied.
 * @param[out] limit          Limit value in terms of cell count.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_pfc_limit_get(bf_dev_id_t dev,
                                     bf_tm_app_pool_t pool,
                                     bf_tm_icos_t icos,
                                     uint32_t *limit);

/**
 * @brief bf_tm_pool_pfc_limit_get_default
 * Get per PFC level default limit values.
 *
 * Related APIs: bf_tm_pool_pfc_limit_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            Pool handle for which limits are configured.
 * @param[in] icos            Internal CoS (iCoS = ig_intr_md.ingress_cos) level
 *                            on which limits are applied.
 * @param[out] limit          Limit value in terms of cell count.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_pfc_limit_get_default(bf_dev_id_t dev,
                                             bf_tm_app_pool_t pool,
                                             bf_tm_icos_t icos,
                                             uint32_t *limit);

/**
 * @brief bf_tm_pool_color_drop_state_get
 * Get Application pool's color drop policy. Default policy is to
 * trigger drops based on color.
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] pool            Pool identifier.
 * @param[out] drop_state     Drop or no drop condition
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_color_drop_state_get(bf_dev_id_t dev,
                                            bf_tm_app_pool_t pool,
                                            bool *drop_state);

/**
 * @brief Get Application pool's drop state egress
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] drop_type       Register entry [0..7]:
 *                            0 - Per Egress PIPE Buffer drop state (4 PIPEs)
 *                            1 - Global Buffer AP Green drop state (4 Pools)
 *                            2 - Global Buffer AP Yellow drop state(4 Pools)
 *                            3 - Global Buffer AP Red drop state (4 Pools)
 *                            4 - PIPE0 PRE FIFO drop state (4 FIFOs)
 *                            5 - PIPE1 PRE FIFO drop state (4 FIFOs)
 *                            6 - PIPE2 PRE FIFO drop state (4 FIFOs)
 *                            7 - PIPE3 PRE FIFO drop state (4 FIFOs)
 *                            Next entries will be available for TOF3 only:
 *                            8 - PIPE4 PRE FIFO drop state (4 FIFOs)
 *                            9 - PIPE5 PRE FIFO drop state (4 FIFOs)
 *                            10 - PIPE6 PRE FIFO drop state (4 FIFOs)
 *                            11 - PIPE7 PRE FIFO drop state (4 FIFOs)
 * @param[out] state          drop state value
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_egress_buffer_drop_state_get(
    bf_dev_id_t dev, bf_tm_eg_buffer_drop_state_en drop_type, uint32_t *state);

/**
 * @brief Clear Application pool's drop state egress
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] drop_type       Register entry [0..7]:
 *                            0 - Per Egress PIPE Buffer drop state (4 PIPEs)
 *                            1 - Global Buffer AP Green drop state (4 Pools)
 *                            2 - Global Buffer AP Yellow drop state(4 Pools)
 *                            3 - Global Buffer AP Red drop state (4 Pools)
 *                            4 - PIPE0 PRE FIFO drop state (4 FIFOs)
 *                            5 - PIPE1 PRE FIFO drop state (4 FIFOs)
 *                            6 - PIPE2 PRE FIFO drop state (4 FIFOs)
 *                            7 - PIPE3 PRE FIFO drop state (4 FIFOs)
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_egress_buffer_drop_state_clear(
    bf_dev_id_t dev, bf_tm_eg_buffer_drop_state_en drop_type);

/**
 * @brief Get skid pool size.
 *
 * Related APIs: bf_tm_pool_skid_size_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] cells          Size of pool in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_skid_size_get(bf_dev_id_t dev, uint32_t *cells);

/**
 * @brief Get global skid pool hysteresis.
 *
 * Related APIs: bf_tm_pool_skid_hysteresis_set()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[out] cells     Number of cells set as skid pool hysteresis.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_skid_hysteresis_get(bf_dev_id_t dev, uint32_t *cells);

/**
 * @brief Get default global skid pool hysteresis.
 *
 * Related APIs: bf_tm_pool_skid_hysteresis_set()
 *
 * @param dev        ASIC device identifier.
 * @param cells      Number of cells set as skid pool hysteresis.
 * @return           Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_skid_hysteresis_get_default(bf_dev_id_t dev,
                                                   uint32_t *cells);

/**
 * @brief Get negative mirror pool limit.
 * Returned limit are accounted in terms of cells.
 *
 * Related APIs: bf_tm_pool_mirror_on_drop_size_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[out] cells      Size of pool.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_mirror_on_drop_size_get(bf_dev_id_t dev,
                                               uint32_t *cells);

/**
 * @brief bf_tm_pre_fifo_limit_get
 * Get per pipe per FIFO limit. Returned limit are accounted
 * in terms of cells.
 *
 * Related APIs: bf_tm_pre_fifo_limit_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] pipe        Pipe identifier.
 * @param[in] fifo        FIFO identifier.
 * @param[out] cells      Size of pool.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pre_fifo_limit_get(bf_dev_id_t dev,
                                     bf_dev_pipe_t pipe,
                                     uint8_t fifo,
                                     uint32_t *cells);

/**
 * @brief bf_tm_global_min_limit_get
 * Get global min limit. Returned limit are accounted
 * in terms of cells.
 *
 * Default : 0x6000
 *
 *
 * param[in] dev        ASIC device identifier.
 * param[out] cells     New size of pool.
 * return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_global_min_limit_get(bf_dev_id_t dev, uint32_t *cells);

/**
 * @brief Get negative mirror destination in a pipe.
 * Port queue is get according to the port's current queues map.
 *
 * Related APIs: bf_tm_port_mirror_on_drop_dest_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] pipe        Device pipe.
 * @param[out] port       Negative mirror traffic destination device port.
 * @param[out] queue      Negative mirror traffic queue within destination port.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_mirror_on_drop_dest_get(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_dev_port_t *port,
                                               bf_tm_queue_t *queue);

/**
 * @brief Get default negative mirror destination in a pipe.
 *
 * Related APIs: bf_tm_port_mirror_on_drop_dest_get()
 *
 * @param[in] dev      ASIC device identifier.
 * @param[in] pipe     Device pipe.
 * @param[out] port    Default negative mirror traffic destination device port.
 * @param[out] queue   Default negative mirror traffic port queue.
 * @return             Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_mirror_on_drop_dest_get_default(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bf_dev_port_t *port,
                                                       bf_tm_queue_t *queue);

/**
 * @brief Get cut through pool size for unicast traffic.
 *
 * Related APIs:  bf_tm_pool_uc_cut_through_size_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] cells          Size of pool in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_uc_cut_through_size_get(bf_dev_id_t dev,
                                               uint32_t *cells);

/**
 * @brief Get cut through pool size for Multicast traffic.
 * This size determines total buffer set aside for multicast
 * cut through traffic.
 *
 * Related APIs:  bf_tm_pool_mc_cut_through_size_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] cells          Size of pool in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pool_mc_cut_through_size_get(bf_dev_id_t dev,
                                               uint32_t *cells);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Get Ingress port limit.
 * When buffer usage accounted on port basis crosses the limit,
 * traffic is not admitted into traffic manager.
 *
 * Related APIs: bf_tm_port_ingress_drop_limit_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier
 * @param[out] cells      Limit in terms of number of cells.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_ingress_drop_limit_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t *cells);

/**
 * @brief Get Egress port limit.
 * When buffer usage accounted on port basis crosses the limit,
 * traffic Will be dropped on QAC stage.
 *
 * Related APIs: bf_tm_port_egress_drop_limit_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier
 * @param[out] cells      Limit in terms of number of cells.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_egress_drop_limit_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t *cells);

/**
 * @brief bf_tm_port_wac_drop_state_get
 * Get port drop state in WAC. (TOF1 only)
 *
 * Related APIs: bf_tm_port_get_wac_drop_state()
 *
 * @param[in] dev            ASIC device identifier.
 * @param port               Port Identifier
 * @param state              Drop state
 * @return                   Status of API call.
 */
bf_status_t bf_tm_port_wac_drop_state_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *state);

/**
 * @brief bf_tm_port_wac_drop_state_get_ext
 * Get port drop state in WAC. (TOF2, TOF3 ...)
 *
 * Related APIs: bf_tm_port_get_wac_drop_state_ext()
 *
 * @param[in] dev                 ASIC device identifier.
 * @param[in] port                Port Identifier
 * @param[out] shr_lmt_state      Drop state caused by shared limit.
 * @param[out] hdr_lmt_state      Drop state caused by headroom limit.
 * @return                        Status of API call.
 * BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_wac_drop_state_get_ext(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              bool *shr_lmt_state,
                                              bool *hdr_lmt_state);

/**
 * @brief bf_tm_port_qac_drop_state_get
 * Get port drop state in QAC.
 *
 * Related APIs: bf_tm_port_get_qac_drop_state()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port Identifier
 * @param[out] state     Drop state.
 * @return               Status of API call.
 * BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_qac_drop_state_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *state);

/**
 * @brief Get port hysteresis limits.
 * When usage of cells goes below hysteresis value  port pause or drop
 * condition  will be cleared.
 *
 * Related APIs: bf_tm_port_ingress_hysteresis_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier
 * @param[out] cells      Offset Limit
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_ingress_hysteresis_get(bf_dev_id_t dev,
                                              bf_dev_port_t port,
                                              uint32_t *cells);

/**
 * @brief Get port hysteresis limits.
 * When usage of cells goes below hysteresis value  port drop
 * condition  will be cleared.
 *
 * Related APIs: bf_tm_port_egress_hysteresis_set()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier
 * @param[out] cells      Offset Limit
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_egress_hysteresis_get(bf_dev_id_t dev,
                                             bf_dev_port_t port,
                                             uint32_t *cells);

/**
 * @brief Get port skid limit.
 *
 * Related APIs: bf_tm_port_skid_limit_set()
 *
 * @param[in] dev     ASIC device identifier.
 * @param[in] port    Port Identifier
 * @param[out] cells  Limit in terms of number of cells.
 * @return            Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_skid_limit_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint32_t *cells);

/**
 * @brief Get port PFC status
 * Get port PFC status.
 *
 * Related APIs: bf_tm_sch_get_port_egress_pfc_status()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[out] status         PFC status.
 * @return            Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_port_egress_pfc_status_get(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   uint8_t *status);
/**
 * @brief queue PFC status
 * Get queue PFC status
 *
 * Related APIs: bf_tm_sch_get_q_egress_pfc_status(),
 *               bf_tm_sched_q_egress_pfc_status_set(),
 *               bf_tm_sched_q_egress_pfc_status_clear()
 *
 * @param[in]   dev         ASIC device identifier.
 * @param[in]   port        port
 * @param[in]   queue       queue
 * @param[out]  status      Egress Queue PFC Status - True or False value of
 *                          pfc_pause
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_q_egress_pfc_status_get(bf_dev_id_t dev,
                                                bf_dev_port_t port,
                                                bf_tm_queue_t queue,
                                                bool *status);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * @brief Default TM buffering settings for a port.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  port                Port Identifier
 * @param[out] ct_enable           The port default cut-through mode.
 * @param[out] uc_ct_limit_cells   The port default cut-through buffer size.
 * @param[out] ig_limit_cells      The port default ingress limit in cells.
 * @param[out] ig_hysteresis_cells The port default ingress hysteresis limit in
 *                                 cells.
 * @param[out] eg_limit_cells      The port default egress limit in cells.
 * @param[out] eg_hysteresis_cells The port default egress hysteresis limit in
 *                                 cells.
 * @param[out] skid_limit_cells    The port default skid limit in cells.
 * @return                         Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_buffer_get_default(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bool *ct_enable,
                                          uint8_t *uc_ct_limit_cells,
                                          uint32_t *ig_limit_cells,
                                          uint32_t *ig_hysteresis_cells,
                                          uint32_t *eg_limit_cells,
                                          uint32_t *eg_hysteresis_cells,
                                          uint32_t *skid_limit_cells);

/**
 * @brief Default flow control settings for a port.
 *
 * Related APIs: bf_tm_port_flowcontrol_mode_get()
 *
 * @param[in]  dev      ASIC device identifier.
 * @param[in]  port     Port Identifier
 * @param[out] mode_tx  Default flow control mode on the port.
 * @param[out] mode_rx  Default react on flow control frame
 *                      received at the port.
 * @param[out] cos_map  Default CoS to iCoS map.
 * @return              Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_flowcontrol_get_default(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_flow_ctrl_type_t *mode_tx,
                                               bf_tm_flow_ctrl_type_t *mode_rx,
                                               uint8_t *cos_map);

/**
 * @brief This API can be used to get pause type set on port.
 *
 * Related APIs: bf_tm_port_flowcontrol_mode_set()
 *
 * Default : No Pause or flow control.
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port handle.
 * @param[out] type       Pause type.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_flowcontrol_mode_get(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_flow_ctrl_type_t *type);

/**
 * @brief bf_tm_port_flowcontrol_rx_get
 * This API can be used to get pause type set on port to react when
 * port sees pause or pfc from peer.
 *
 * Related APIs: bf_tm_port_flowcontrol_rx_set()
 *               bf_tm_port_flowcontrol_mode_set()
 *
 * Default : Ignore Pause, PFC.
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port handle.
 * @param[out] type       Pause type.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_flowcontrol_rx_get(bf_dev_id_t dev,
                                          bf_dev_port_t port,
                                          bf_tm_flow_ctrl_type_t *type);

/**
 * @brief Get iCoS(iCoS = ig_intr_md.ingress_cos) to packet CoS.
 *
 * Default: No PFC
 *
 * Related APIs: bf_tm_port_pfc_cos_mapping_set()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port handle
 * @param[out] cos_to_icos  Array of 8 uint8_t values.
 *                          Array index is CoS and array value is iCoS.
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_pfc_cos_mapping_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           uint8_t *cos_to_icos);

/**
 * @brief bf_tm_q_pfc_cos_mapping_get_default
 * PFC default CoS value.
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_get()
 *
 * param[in]  dev    ASIC device identifier.
 * param[out] cos    PFC CoS default value.
 * return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_pfc_cos_mapping_get_default(bf_dev_id_t dev, uint8_t *cos);

/**
 * @brief bf_tm_port_cut_through_enable_status_get
 * This API can be used to get cut-through enable status of a port.
 *
 * Related APIs: bf_tm_port_cut_through_enable()
 *               bf_tm_port_cut_through_disable()
 *
 * Default : Cut-through is disabled.
 *
 * @param[in] dev                 ASIC device identifier.
 * @param[in] port                Port handle.
 * @param[out] sw_ct_enabled      Cut-through enable status in TM SW cache
 * @param[out] hw_ct_enabled      Cut-through enable status in TM HW register
 * @return                        Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_cut_through_enable_status_get(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bool *sw_ct_enabled,
                                                     bool *hw_ct_enabled);

/**
 * @brief Get number of Global timestamp bits that are right shifted.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[out] shift        Number of Global timestamp bits that are
 *                          right shifted.
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_timestamp_shift_get(bf_dev_id_t dev, uint8_t *shift);

/**
 * Get the default number of global timestamp bits that is to be right shifted.
 *
 * @param[in] dev            ASIC device identifier.
 * @param[out] shift         Number of Global timestamp bits that are right
 *                           shifted.
 *                           Up to 16 bits can be right shifted. Any shift value
 *                           greater than 16 is capped to 16.
 * @return                   Status of API call.
 */
bf_status_t bf_tm_timestamp_shift_get_default(bf_dev_id_t dev, uint8_t *shift);

/**
 * @brief bf_tm_port_credits_get
 * Get current credits for the port.
 *
 * Related APIs: bf_tm_port_get_credits()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port Identifier.
 * @param[out] credits      Number of credits.
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_credits_get(bf_dev_id_t dev,
                                   bf_dev_port_t port,
                                   uint32_t *credits);

/**
 * @brief bf_tm_port_credits_get_safe
 * Get current credits for 100G ports (thread safe).
 *
 * Related APIs: bf_tm_port_credits_get()
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] port        Port Identifier.
 * @param[out] credits    Number of credits.
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_credits_get_safe(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        uint32_t *credits);

/**
 * @brief bf_tm_ppg_skid_usage_get
 * Per ppg skid usage counter
 *
 * Related APIs: bf_tm_ppg_get_skid_usage_counter()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] ppg             ppg handle.
 * @param[out] counter        Counter value.
 * @return status of API call
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_skid_usage_get(bf_dev_id_t dev,
                                     bf_tm_ppg_hdl ppg,
                                     uint32_t *counter);

/* @} */

/**
 * @addtogroup tm-get
 * @{
 */

/**
 * Get TM device-specific settings.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[out] cfg            Config.
 * @return                    Status of API call.
 *
 */
bf_status_t bf_tm_dev_config_get(bf_dev_id_t dev, bf_tm_dev_cfg_t *cfg);

/**
 * @brief Get number of active pipes.
 *
 * @param[in]  dev       ASIC device identifier.
 * @param[out] count     Number of active pipes.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_get_count(bf_dev_id_t dev, uint8_t *count);

/**
 * @brief Get first logical pipe.
 *
 * @param[in]  dev              ASIC device identifier.
 * @param[out] first_l_pipe     First logical pipe.
 * @return                      Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_get_first(bf_dev_id_t dev, bf_dev_pipe_t *first_l_pipe);

/**
 * @brief Get next logical pipe.
 *
 * @param[in]  dev              ASIC device identifier.
 * @param[in]  current_l_pipe   Current logical pipe.
 * @param[out] next_l_pipe      Next logical pipe.
 *
 * @return                      Status of API call.
 * @return BF_SUCCESS          : Next pipe found
 * @return BF_OBJECT_NOT_FOUND : Next pipe not found
 * @return BF_UNEXPECTED       : TM context is invalid
 *
 */
bf_status_t bf_tm_pipe_get_next(bf_dev_id_t dev,
                                bf_dev_pipe_t current_l_pipe,
                                bf_dev_pipe_t *next_l_pipe);

/**
 * @brief Get number of ports per pipe.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier
 * @param[in]  regular_ports_only   Get number of regular ports or all ports
 * @param[out] count                Number of ports per pipe.
 * @return                          Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pipe_port_get_count(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bool regular_ports_only,
                                      uint16_t *count);

/**
 * @brief Get first devport in a pipe.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier
 * @param[in]  regular_ports_only   Get only regular port or any (regular or
 *                                  mirror)
 * @param[out] port_id              Device port identifier (pipe id and port
 *                                  number).
 *
 * @return BF_SUCCESS          :    First port in a pipe found
 * @return BF_UNEXPECTED       :    TM context is invalid
 */
bf_status_t bf_tm_pipe_port_get_first(bf_dev_id_t dev,
                                      bf_dev_pipe_t pipe,
                                      bool regular_ports_only,
                                      bf_dev_port_t *port);
/**
 * @brief Get next devport in a pipe.
 *
 * @param[in]  dev                 ASIC device identifier.
 * @param[in]  current_port        Current device port identifier.
 * @param[in]  regular_ports_only  Get only regular next port (0..71), exclude
 *                                 special (mirror ports). Set to true to get
 *                                 regular ports iterator, set to false to get
 *                                 all ports iterator.
 * @param[out] next_port           Next device port identifier.
 *
 * @return BF_SUCCESS          :   Next port found
 * @return BF_OBJECT_NOT_FOUND :   Next port not found
 * @return BF_UNEXPECTED       :   TM context is invalid
 * @return BF_NOT_IMPLEMENTED  :   This functionality is not implemented for
 *                                 current Tofino version
 *
 */
bf_status_t bf_tm_pipe_port_get_next(bf_dev_id_t dev,
                                     bf_dev_port_t current_port,
                                     bool regular_ports_only,
                                     bf_dev_port_t *next_port);
/**
 * @brief bf_tm_port_icos_count_get
 * Get number of supported iCoS.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[in]  port           Port Identifier
 * @param[out] icos_count     Number of supported iCoS.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_icos_count_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      uint8_t *icos_count);

/**
 * @brief bf_tm_port_status_get
 * Get TM Port current status.
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[out] is_offline     Port is currently offline.
 * @param[out] is_enabled     Port is currently enabled.
 * @param[out] qac_rx_enabled QAC Rx current status.
 * @param[out] recirc_enabled Recirculation status.
 * @param[out] has_mac        Port has MAC.
 * @return             Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_status_get(bf_dev_id_t dev,
                                  bf_dev_port_t port,
                                  bool *is_offline,
                                  bool *is_enabled,
                                  bool *qac_rx_enable,
                                  bool *recirc_enabe,
                                  bool *has_mac);
/**
 * @brief Get first dev_port in a port group.
 *
 * @param[in]  dev                  ASIC device identifier.
 * @param[in]  pipe                 Pipe identifier.
 * @param[in]  pg_id                Device port group (a.k.a. 'quad').
 * @param[out] port_id              Device port identifier (pipe id and port
 *                                  number).
 *
 * @return BF_SUCCESS          :    First port in a pipe found
 * @return BF_UNEXPECTED       :    TM context is invalid
 */
bf_status_t bf_tm_pipe_port_group_get_first_port(bf_dev_id_t dev,
                                                 bf_dev_pipe_t pipe,
                                                 bf_tm_pg_t pg_id,
                                                 bf_dev_port_t *port_id);
/**
 * @brief bf_tm_port_group_get
 * Get Port Group of a port.
 *
 * @param[in]   dev         ASIC device identifier.
 * @param[in]   port_id     Device port identifier (pipe id and port number).
 * @param[out]  pg_id       Device port group (a.k.a. 'quad').
 * @param[out]  pg_port_nr  Port number in its group.
 * @return                  Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_group_get(bf_dev_id_t dev,
                                 bf_dev_port_t port_id,
                                 bf_tm_pg_t *pg_id,
                                 uint8_t *pg_port_nr);

/**
 * @brief Get base Port Group queue for a Port.
 *
 * Related APIs: bf_tm_q_get_descriptor()
 *               bf_tm_pg_port_queue_get()
 *
 * @param[in]  dev        ASIC device identifier.
 * @param[in]  port_id    Device port identifier (pipe id and port number).
 * @param[out] pg_id      Device port group (a.k.a. 'quad') of the port.
 * @param[out] pg_queue   Port group queue number (physical queue) which
 *                        corresponds to the very first queue of the port.
 * @param[out] is_mapped  True if the physical queue (identified by pg_id and
 *                        pg_queue) is currently mapped as the very first queue
 *                        of the port_id, otherwise it is either not carved or
 *                        it belongs to the next port in the same port group.
 * @return                Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_port_base_queue_get(bf_dev_id_t dev,
                                      bf_dev_port_t port_id,
                                      bf_tm_pg_t *pg_id,
                                      uint8_t *pg_queue,
                                      bool *is_mapped);

/**
 * @brief bf_tm_pg_port_queue_get
 * Get Port Queue number by its Port Group physical queue.
 *
 * Related APIs: bf_tm_q_get_descriptor()
 *
 * @param[in]  dev_tgt    Pipeline on an ASIC device.
 * @param[in]  pg_id      Device port group (a.k.a. 'quad').
 * @param[in]  pg_queue   Port group queue number (physical queue).
 * @param[out] port_id    Device port identifier (pipe id and port number).
 * @param[out] queue_nr   Port queue number to use with bf_tm_q_* APIs.
 *                        If the Port group queue is currently not mapped to
 *                        any port queue, then queue_nr is equal to pg_queue,
 *                        port_id is set to the first port in the port group,
 *                        and is_mapped equals to false.
 * @param[out] is_mapped  True if the physical queue (identified by pg_id and
 *                        pg_queue) is currently mapped to the queue_nr at
 *                        port_id, otherwise the port_id is set to the
 *                        port's group first port and queue_nr is equal to
 *                        the pg_queue.
 * @return                Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_pg_port_queue_get(bf_dev_target_t *dev_tgt,
                                    bf_tm_pg_t pg_id,
                                    uint8_t pg_queue,
                                    bf_dev_port_t *port_id,
                                    bf_tm_queue_t *queue_nr,
                                    bool *is_mapped);

/**
 * @brief bf_tm_q_pfc_cos_mapping_get
 * When egress queues need to honour received PFC from downstream,
 * by mapping cos to queue using the API below, queues
 * will not participate in scheduling until PFC gets cleared.
 *
 * Default: All queues are mapping CoS zero.
 *
 * Related APIs: bf_tm_q_pfc_cos_mapping_set()
 *
 * @param[in]  dev    ASIC device identifier.
 * @param[in]  port   Device port id.
 * @param[in]  queu   Port Queue number.
 * @param[out] cos    PFC CoS associated with the queue.
 * @return            Status of the API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_q_pfc_cos_mapping_get(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_queue_t queue,
                                        uint8_t *cos);

/*
 * Get total PFC levels
 *
 * Related APIs: bf_tm_pool_pfc_limit_get()
 *
 * @param dev[in]        ASIC device identifier.
 * @param levels[out]    Number of PFC levels.
 * @return               Status of API call.
 */
bf_status_t bf_tm_max_pfc_levels_get(bf_dev_id_t dev, uint32_t *levels);

/*
 * Get L1 Node the Queue is associated to.
 *
 * Related APIs: bf_tm_sched_q_l1_set()
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  port        Device port id.
 * @param[in]  queue       Port Queue number.
 * @param[out] l1_node     L1 Node number (in the same port group).
 * @return                 Status of API call.
 */
bf_status_t bf_tm_sched_q_l1_get(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bf_tm_queue_t queue,
                                 bf_tm_l1_node_t *l1_node);

/**
 * Get L1 Node the Queue is associated by default.
 *
 * Related APIs: bf_tm_sched_q_l1_get()
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  port        Device port id.
 * @param[in]  queue       Port Queue number.
 * @param[out] l1_node     Default L1 Node number in the port group.
 * @return                 Status of API call.
 */
bf_status_t bf_tm_sched_q_l1_get_default(bf_dev_id_t dev,
                                         bf_dev_port_t port,
                                         bf_tm_queue_t queue,
                                         bf_tm_l1_node_t *l1_node);

/**
 * Get L1 Node port asssignment status.
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  pipe        Pipe Identifier.
 * @param[in]  pg_id       Device port group (a.k.a. 'quad').
 * @param[in]  l1_node     L1 Node number (in the port group).
 * @param[out] in_use      True if L1 Node is associated to some port.
 * @param[out] pg_port_nr  Associated port number in the port group.
 * @param[out] port_id     Device port identifier (pipe id and port number)
 *                         of the associated port.
 * @return                 Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_port_assignment_get(bf_dev_id_t dev,
                                               bf_dev_pipe_t pipe,
                                               bf_tm_pg_t pg_id,
                                               bf_tm_l1_node_t l1_node,
                                               bool *in_use,
                                               uint8_t *pg_port_nr,
                                               bf_dev_port_t *port_id);

/**
 * Get L1 Node default port asssignment status.
 *
 * @param[in]  dev         ASIC device identifier.
 * @param[in]  pipe        Pipe Identifier.
 * @param[in]  pg_id       Device port group (a.k.a. 'quad').
 * @param[in]  l1_node     L1 Node number (in the port group).
 * @param[out] in_use      True if L1 Node is associated to some port by
 * default.
 * @param[out] pg_port_nr  Associated default port number in the port group.
 * @param[out] port_id     Device port identifier (pipe id and port number)
 *                         of the port associated by default.
 * @return                 Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_port_assignment_get_default(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       bf_tm_pg_t pg_id,
                                                       bf_tm_l1_node_t l1_node,
                                                       bool *in_use,
                                                       uint8_t *pg_port_nr,
                                                       bf_dev_port_t *port_id);

/**
 * Get Port to L1 Nodes assignment status.
 *
 * @param[in]  dev           ASIC device identifier.
 * @param[in]  port_id       Device port identifier (pipe id and port number).
 * @param[in]  in_use_only   Count only active L1 Nodes.
 * @param[out] l1_nodes_cnt  Number of items in l1_nodes.
 * @param[out] l1_nodes      Array of Port group L1 Node ids associated
 *                           with the port_id. Caller has to provide memory
 *                           for 32 max items (BF_TM_TOF2_SCH_L1_PER_PG).
 * @return                   Status of API call.
 *
 */
bf_status_t bf_tm_port_l1_node_assignment_get(bf_dev_id_t dev,
                                              bf_dev_port_t port_id,
                                              bool in_use_only,
                                              uint8_t *l1_nodes_cnt,
                                              bf_tm_l1_node_t *l1_nodes);

/**
 * Get L1 Node queue asssignments.
 *
 * @param[in]  dev            ASIC device identifier.
 * @param[in]  pipe           Pipe Identifier.
 * @param[in]  pg_id          Device port group (a.k.a. 'quad').
 * @param[in]  l1_node        L1 Node number (in the port group).
 * @param[out] l1_queues_cnt  Number of items in l1_queues for port group
 *                            queue ids associated with the L1 Node.
 * @param[out] l1_queues      Array of Port group queue ids associated
 *                            with the L1 Node. Caller has to provide memory
 *                            for 128 max items (BF_TM_TOF2_QUEUES_PER_PG).
 * @return                    Status of API call.
 *
 */
bf_status_t bf_tm_sched_l1_queue_assignment_get(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                bf_tm_pg_t pg_id,
                                                bf_tm_l1_node_t l1_node,
                                                uint8_t *l1_queues_cnt,
                                                bf_tm_queue_t *l1_queues);

/**
 * Get l1 node DWRR weight.
 * These weights are used by l1 nodes at same priority level.
 * Across priority these weights serve as ratio to
 * share excess or remaining bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_dwrr_weight_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] weight         Weight value
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_dwrr_weight_get(bf_dev_id_t dev,
                                           bf_dev_port_t port,
                                           bf_tm_l1_node_t l1_node,
                                           uint16_t *weight);
/**
 * Get L1 Node DWRR weight default.
 *
 * Related APIs: bf_tm_sched_l1_dwrr_weight_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         L1 Node
 * @param[out] weight         Default weight value
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_dwrr_weight_get_default(bf_dev_id_t dev_id,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   uint16_t *weight);

/**
 * Get l1 scheduler enable status.
 *
 * Related APIs: bf_tm_sched_l1_enable()
 *               bf_tm_sched_l1_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_enable_get(bf_dev_id_t dev,
                                      bf_dev_port_t port,
                                      bf_tm_l1_node_t l1_node,
                                      bool *enable);

/**
 * Get enable status for token bucket that assures l1 node guaranteed rate
 * (pps or bps)
 *
 * Default: l1 node guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable()
 *               bf_tm_sched_l1_guaranteed_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_enable_get(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *enable);

/*
 * Get L1 Node guaranteed min rate defaults.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] l1_node    L1 Node
 * @param[out] enable    True if the L1 Node guaranteed rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_l1_guaranteed_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority);

/**
 * Get enable status for token bucket that assures l1 node shaping rate
 * (pps or bps)
 *
 * Default: l1 node shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_l1_max_shaping_rate_enable()
 *               bf_tm_sched_l1_max_shaping_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_shaping_rate_enable_get(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   bool *enable);
/*
 * Get L1 Node shaping max rate defaults.
 *
 * Related APIs: bf_tm_sched_l1_shaping_rate_enable_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[in] port       Port
 * @param[in] l1_node    L1 Node
 * @param[out] enable    True if the L1 Node max shaping rate
 *                       is enabled by default.
 * @param[out] priority  Default scheduling priority.
 * @return               Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_sched_l1_shaping_enable_get_default(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bool *enable,
    bf_tm_sched_prio_t *priority);

/**
 * Get enable status for priority propagation from children queues
 *
 * Default: priority propagation is disabled
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *               bf_tm_sched_l1_priority_prop_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_prop_enable_get(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *enable);

/**
 * Get default status for priority propagation for children queues
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *               bf_tm_sched_l1_priority_prop_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_prop_get_default(bf_dev_id_t dev,
                                                     bf_dev_port_t port,
                                                     bf_tm_l1_node_t l1_node,
                                                     bool *enable);

/**
 * Get l1 node scheduling priority. Scheduling priority level used when
 * serving guaranteed bandwidth. Higher the number, higher the  priority to
 * select the l1 node for scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_priority_set()
 *
 * @param[in] dev                   ASIC device identifier
 * @param[in] port                  Port
 * @param[in] l1_node               l1 node
 * @param[out] priority             Scheduling priority of l1 node
 */
bf_status_t bf_tm_sched_l1_priority_get(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bf_tm_sched_prio_t *priority);

/**
 * Get scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the l1 node for
 * scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_remaining_bw_priority_set()
 *
 * @param[in] dev             ASIC device identifier
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] priority       Scheduling priority of l1 node
 */
bf_status_t bf_tm_sched_l1_remaining_bw_priority_get(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t *priority);

/**
 * Get l1 node guaranteed rate in terms of pps or kbps.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_get(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_l1_node_t l1_node,
                                               bool *pps,
                                               uint32_t *burst_size,
                                               uint32_t *rate);

/*
 * Get L1 Node default guaranteed rate.
 * Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_get()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] l1_node       l1 node
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_get_default(bf_dev_id_t dev,
                                                       bf_dev_port_t port,
                                                       bf_tm_l1_node_t l1_node,
                                                       bool *pps,
                                                       uint32_t *burst_size,
                                                       uint32_t *rate);

/**
 * Get l1 node shaping rate in units of kbps or pps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_shaping_rate_get(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node,
                                            bool *pps,
                                            uint32_t *burst_size,
                                            uint32_t *rate);

/*
 * Get L1 Node default shaping rate.
 * Rate is in units of kbps or pps.
 *
 * Related APIs: bf_tm_sched_l1_shaping_rate_get()
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] port          Port
 * @param[in] l1_node       l1 node
 * @param[out] pps          If set to true, values are in terms of pps
 *                          and packets, else in terms of kbps and bytes.
 * @param[out] burst_size   Burst size in packets or bytes.
 * @param[out] rate         Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_shaping_rate_get_default(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *pps,
                                                    uint32_t *burst_size,
                                                    uint32_t *rate);

/**
 * Get enable status for token bucket that assures l1 node guaranteed rate
 * (pps or bps)
 *
 * Default: l1 node guaranteed shaping rate is disabled
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_enable()
 *               bf_tm_sched_l1_guaranteed_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_enable_get(bf_dev_id_t dev,
                                                      bf_dev_port_t port,
                                                      bf_tm_l1_node_t l1_node,
                                                      bool *enable);

/**
 * Get enable status for token bucket that assures l1 node shaping rate
 * (pps or bps)
 *
 * Default: l1 node shaping rate is enabled
 *
 * Related APIs: bf_tm_sched_l1_max_shaping_rate_enable()
 *               bf_tm_sched_l1_max_shaping_rate_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_shaping_rate_enable_get(bf_dev_id_t dev,
                                                   bf_dev_port_t port,
                                                   bf_tm_l1_node_t l1_node,
                                                   bool *enable);

/**
 * Get enable status for priority propagation for children queues
 *
 * Default: priority propagation is disabled
 *
 * Related APIs: bf_tm_sched_l1_priority_prop_enable()
 *               bf_tm_sched_l1_priority_prop_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] enable         Enable status
 * @return                    Status of API call
 */
bf_status_t bf_tm_sched_l1_priority_prop_enable_get(bf_dev_id_t dev,
                                                    bf_dev_port_t port,
                                                    bf_tm_l1_node_t l1_node,
                                                    bool *enable);

/**
 * Get l1 node scheduling priority. Scheduling priority level used when
 * serving guaranteed bandwidth. Higher the number, higher the  priority to
 * select the l1 node for scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_priority_set()
 *
 * @param[in] dev                   ASIC device identifier
 * @param[in] port                  Port
 * @param[in] l1_node               l1 node
 * @param[out] priority             Scheduling priority of l1 node
 */
bf_status_t bf_tm_sched_l1_priority_get(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        bf_tm_l1_node_t l1_node,
                                        bf_tm_sched_prio_t *priority);

/**
 * Get scheduling priority when serving remaining bandwidth.
 * Higher the number, higher the  priority to select the l1 node for
 * scheduling.
 *
 * Default: l1 node scheduling priority set to BF_TM_SCH_PRIO_7
 *
 * Related APIs: bf_tm_sched_l1_remaining_bw_priority_set()
 *
 * @param[in] dev             ASIC device identifier
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] priority       Scheduling priority of l1 node
 */
bf_status_t bf_tm_sched_l1_remaining_bw_priority_get(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    bf_tm_l1_node_t l1_node,
    bf_tm_sched_prio_t *priority);

/**
 * Get l1 node guaranteed rate in terms of pps or kbps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_guaranteed_rate_get(bf_dev_id_t dev,
                                               bf_dev_port_t port,
                                               bf_tm_l1_node_t l1_node,
                                               bool *pps,
                                               uint32_t *burst_size,
                                               uint32_t *rate);

/**
 * Get l1 node shaping rate in units of kbps or pps.
 *
 * Default: l1 node shaping rate set to match port bandwidth.
 *
 * Related APIs: bf_tm_sched_l1_guaranteed_rate_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[in] port            Port
 * @param[in] l1_node         l1 node
 * @param[out] pps            If set to true, values are in terms of pps
 *                            and packets, else in terms of kbps and bytes.
 * @param[out] burst_size     Burst size in packets or bytes.
 * @param[out] rate           Shaper value in pps or kbps.
 */
bf_status_t bf_tm_sched_l1_shaping_rate_get(bf_dev_id_t dev,
                                            bf_dev_port_t port,
                                            bf_tm_l1_node_t l1_node,
                                            bool *pps,
                                            uint32_t *burst_size,
                                            uint32_t *rate);

/**
 * @brief Get Ingress global limit threshold.
 * This threshold determines the max usage of ingress buffers.
 * This function supported for Tofino-2 and Tofino-3. Not supported
 * for Tofino-1.
 *
 * Related APIs: bf_tm_global_max_limit_set()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] cells          Size in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_get(bf_dev_id_t dev, uint32_t *cells);

/**
 * @brief Get default Ingress global limit threshold.
 * Default value of the max usage of ingress buffers.
 * This function supported for Tofino-2 and Tofino-3. Not supported
 * for Tofino-1.
 *
 * Related APIs: bf_tm_global_max_limit_set(), bf_tm_global_max_limit_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] cells          Size in terms of cells.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_get_default(bf_dev_id_t dev,
                                               uint32_t *cells);

/**
 * @brief Get Ingress global limit state.
 * This state determines the usage of ingress max limit.
 * This function supported for Tofino-2 and Tofino-3. Not supported
 * for Tofino-1.
 *
 * Related APIs: bf_tm_global_max_limit_enable(),
 * bf_tm_global_max_limit_disable()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] state          Enabled or Disabled limit.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_state_get(bf_dev_id_t dev, bool *state);

/**
 * @brief Get Default Ingress global limit state.
 * Default state of the usage of ingress max limit.
 * This function supported for Tofino-2 and Tofino-3. Not supported
 * for Tofino-1.
 *
 * Related APIs: bf_tm_global_max_limit_enable(),
 * bf_tm_global_max_limit_disable(), bf_tm_global_max_limit_state_get()
 *
 * @param[in] dev             ASIC device identifier.
 * @param[out] state          Enabled or Disabled limit.
 * @return                    Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_global_max_limit_state_get_default(bf_dev_id_t dev,
                                                     bool *state);

/* @} */

#endif
