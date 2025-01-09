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

#ifndef _DIAG_PKT_DATABASE_H_
#define _DIAG_PKT_DATABASE_H_

#ifdef __cplusplus
extern "C" {
#endif

// namespace diag {
#define PKT_DATABASE_ERR UINT64_MAX

typedef uint64_t pkt_id_type;

int diag_add_pkt_to_db(const pkt_id_type pkt_id,
                       const uint32_t pkt_size,
                       const uint8_t *pkt_buf);
int diag_get_pkt_from_db(const pkt_id_type pkt_id,
                         const uint32_t pkt_size,
                         uint32_t *cached_pkt_size,
                         uint8_t *pkt_buf);
pkt_id_type diag_encode_pkt_id(const bf_diag_sess_hdl_t sess_hdl,
                               const bf_dev_port_t dev_port,
                               const uint32_t pkt_size,
                               uint8_t *pkt_buf);
pkt_id_type diag_decode_pkt_id(const uint32_t pkt_size,
                               const uint8_t *pkt_buf,
                               pkt_id_type *pkt_id,
                               bf_diag_sess_hdl_t *sess_hdl,
                               bf_dev_port_t *dev_port);
void diag_cleanup_pkt_database(const bf_diag_sess_hdl_t sess_hdl);

bf_status_t diag_compare_pkt_with_database(
    const uint32_t size,
    const uint8_t *received_pkt,
    const pkt_id_type pkt_id,
    const bf_diag_sess_hdl_t sess_hdl,
    const bf_dev_port_t ingress_port,
    uint8_t *exp_bytes,
    uint32_t *cached_pkt_size,
    uint64_t *num_bytes_mismatch,
    uint64_t *num_bits_mismatch,
    uint64_t *num_1_to_0_flips,
    uint64_t *num_0_to_1_flips,
    diag_slt_failure_type_e *failure_type);
//} // namespace diag
#ifdef __cplusplus
}
#endif

#endif  // _DIAG_PKT_DATABASE_H_
