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


#ifndef __TM_ECC_API_H__
#define __TM_ECC_API_H__

#include <traffic_mgr/traffic_mgr_types.h>

/*
 * Correct single-bit ECC error at the parameterized global address
 *
 * @param dev                   ASIC device identifier.
 * @param addr                  Global address of error'ed TM memory
 */

bf_status_t bf_tm_ecc_correct_addr(bf_dev_id_t dev, uint64_t addr);

/*
 * Correct single-bit ECC error at the local wac_ppg_map address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param addr                  Local address of error'ed wac_ppg_map memory.
 */

bf_status_t bf_tm_ecc_correct_wac_ppg_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local wac_qid_map address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param addr                  Local address of error'ed wac_qid_map memory.
 */

bf_status_t bf_tm_ecc_correct_wac_qid_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local qac_qid_map address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param addr                  Local address of error'ed qac_qid_map memory.
 */

bf_status_t bf_tm_ecc_correct_qac_qid_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_tdm address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param addr                  Local address of error'ed sch_tdm memory.
 */

bf_status_t bf_tm_ecc_correct_sch_q_minrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_q_excrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_q_excrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_q_excrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_q_maxrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_q_maxrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_q_maxrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_l1_minrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_l1_minrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_l1_minrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_l1_excrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_l1_excrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_l1_excrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_l1_maxrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_l1_maxrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_l1_maxrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr);

/*
 * Correct single-bit ECC error at the local sch_p_maxrate address
 *
 * @param dev                   ASIC device identifier.
 * @param pipe                  Physical pipe identifier.
 * @param dyn                   Dynamic or static memory.
 * @param addr                  Local address of error'ed sch_p_maxrate memory.
 */

bf_status_t bf_tm_ecc_correct_sch_p_maxrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr);

#endif
