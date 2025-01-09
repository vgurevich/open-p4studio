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
#ifndef __PACKETPATH_API_H__
#define __PACKETPATH_API_H__
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_err.h>

/**
 * Enumeration values to categorize counter
 */
typedef enum bf_packetpath_counter_type_en_ {
  /* When counter value is qualified by this enum,
   * counter value is count of packets.
   */
  BF_PACKETPATH_COUNTER_TYPE_PACKETCOUNT = 0,
  /* When counter value is qualified by this enum,
   * counter value is count of packet drops.
   */
  BF_PACKETPATH_COUNTER_TYPE_PACKETDROP,
  /* When counter value is qualified by this enum,
   * counter value is count of packet errors occurred.
   */
  BF_PACKETPATH_COUNTER_TYPE_PACKETERROR,
  /* When counter value is qualified by this enum,
   * counter value is count of internal errors.
   * Example: Internal error could like internal FIFO full count.
   */
  BF_PACKETPATH_COUNTER_TYPE_INTERNALERROR,
  /* When counter value is qualified by this enum,
   * counter value is count of internal resource count.
   * Example: PHV processed count by a packet path block.
   */
  BF_PACKETPATH_COUNTER_TYPE_INTERNALCOUNT,
  /* When counter value is qualified by this enum,
   * counter value is count of bytes.
   */
  BF_PACKETPATH_COUNTER_TYPE_BYTECOUNT
} bf_packetpath_counter_type_en;

/*
 *                  IBUF Counters per PIPE per Port
 */

typedef struct _bf_ibp_cntrs {
  uint64_t total_pkts_drop;
  uint64_t total_pkts_disc;
  uint64_t total_recirc_pkt_disc;
  uint64_t total_prsr_pkt_disc;

} bf_ibp_cntrs_t;

/*
 *                  IPRSR Counters per PIPE per Port
 */

typedef struct _bf_iprsr_cntrs {
  uint64_t fcs_err_count;
  uint64_t csum_err_count;
  uint64_t tcam_parity_err_count;
  uint64_t total_pkts_drop;

} bf_iprsr_cntrs_t;

/*
 *                  IDPRSR Counters per PIPE per Port
 */

typedef struct _bf_idprsr_cntrs {
  uint64_t pkts_disc;
  uint64_t pkts_disc_at_tm;
  uint64_t err_pkts_to_tm;
  uint64_t err_pkts_to_ictm;
  uint64_t crc_err[72];

} bf_idprsr_cntrs_t;

/*
 *                  EPB_EBUF Counters per PIPE per Port
 */

typedef struct _bf_epb_ebuf_cntrs {
  uint64_t pkts_bypass_epipe;
  uint64_t pkts_sent_epipe;

} bf_epb_ebuf_cntrs_t;

/*
 *                  EPRSR Counters per PIPE per Port
 */

typedef struct _bf_eprsr_cntrs {
  uint64_t fcs_err_count;
  uint64_t csum_err_count;
  uint64_t tcam_parity_err_count;
  uint64_t total_pkts_drop;

} bf_eprsr_cntrs_t;

/*
 *                  EDPRSR Counters per PIPE
 */

typedef struct _bf_edprsr_cntrs {
  uint64_t pkts_disc;
  uint64_t err_pkts_to_ebuf;
  uint64_t err_pkts_to_ectm;
  uint64_t crc_err[72];

} bf_edprsr_cntrs_t;

/**
 * For each counter obtained from device/asic, this structure
 * is used to describe the counter as well as provide counter value.
 */
typedef struct bf_packetpath_counter_ {
  /* Counter description index. Using this index value as index into
   * counter description string table; a complete description string can
   * be obtained to provide meaning to counter value
   *
   * Use API bf_packet_path_counter_description_get()
   */
  uint32_t description_index;
  /* global pointer to Counter description for Tofino2
   * no need to free.
   */
  const char *description;
  /* Counter type enum. Counter value is qualified with by type. This
   * enum provides that information.
   * Counter is qualified as per bf_packetpath_counter_type_en
   */
  bf_packetpath_counter_type_en counter_type;

  /* Counter value */
  uint64_t value;
} bf_packetpath_counter_t;

/*
 *      ASIC blocks/Modules along packet path.
 *
 *    +---------+   +-------+   +-------+   +--------+   +--------+   +-------+
 *    |Ingress  |   |Ingress|   |Ingress|   |Pipe/Mau|   |Ingress |   |Traffic|
 * -->|Port/Mac |-->|Buffer |-->|Parser |-->|        |-->|Deparser|-->|Manager|
 *    +---------+   +-------+   +-------+   +--------+   +--------+   +-------+
 *                                                                       |
 *                                                                       |
 *                                                                       v
 *    +---------+   +-------+   +--------+   +---------+   +-------+   +------+
 *    |Egress   |   |Egress |   |Egress  |   |Pipe/Mau |   |Egress |   |Egress|
 * <--|port/Mac |<--|Buffer |<--|Deparser|<--|         |<--|Parser |<--|Parser|
 *    +---------+   +-------+   +--------+   +---------+   +-------+   |Buffer|
 *                                                                     +------+
 *
 * APIs to collect counters and clear counters are provided for following blocks
 *   - Ingress Buffer (ibuf) at pipe, port granularity
 *   - Ingress Parser (iprsr) at pipe, port granularity
 *   - Ingress DeParser (idprsr) at pipe granularity
 *   - Egress Parser (eprsr) at pipe, port granularity
 *   - Egress DeParser (edprsr) at pipe granularity
 *   - Egress Parser Buffer(epb) and Egress buffer (ebuf)
 *     at pipe, port granularity using single api.
 */

/**
 * Read all counters from Ingress Buffer block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_buffer_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);

/**
 * Read all counters from Ingress Parser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_parser_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);

/**
 * Read all counters from Ingress DeParser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_deparser_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count);
/**
 * Read all counters from Parser to MAU. Only for Tofino2
 *
 * @param devid      ASIC device identifier.
 * @param pipe       Pipe Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 *                   description pointer no need to free.
 */
bf_packetpath_counter_t *bf_packet_path_parser2mau_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count);
/**
 * Read all counters for the traffic go to TM. Only for Tofino2
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 *                   description pointer no need to free.
 */
bf_packetpath_counter_t *bf_packet_path_tm_ingress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);
/**
 * Read all counters for the traffic out of TM. Only for Tofino2
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 *                   description pointer no need to free.
 */
bf_packetpath_counter_t *bf_packet_path_tm_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);
/**
 * Read all counters from Egress buffer block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_buffer_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);

/**
 * Read all counters from Egress Parser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_parser_egress_counter_get(
    bf_dev_id_t devid, bf_dev_port_t port, int *count);

/**
 * Read all counters from Egress Deparser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param count      Number of counters returned.
 * @return           Array of counters or NULL
 *                   Memory allocated for counters should be
 *                   freed by caller of the API using bf_sys_free().
 */
bf_packetpath_counter_t *bf_packet_path_deparser_egress_counter_get(
    bf_dev_id_t devid, bf_dev_pipe_t pipe, int *count);

/**
 * Clear all counters in Ingress buffer block.
 *
 * @param description_index       Counter description index to fetch
 *                                counter description string.
 * @return                        Pointer to string.
 */
const char *bf_packet_path_counter_description_get(uint32_t description_index);

/**
 * Clear all counters in Ingress buffer block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_buffer_ingress_counter_clear(bf_dev_id_t devid,
                                                 bf_dev_port_t port);

/**
 * Clear all counters in Ingress Parser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_parser_ingress_counter_clear(bf_dev_id_t devid,
                                                 bf_dev_port_t port);

/**
 * Clear all counters in Ingress Deparser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_deparser_ingress_counter_clear(bf_dev_id_t devid,
                                                   bf_dev_pipe_t pipe);

/**
 * Clear all counters in Egress parser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_parser_egress_counter_clear(bf_dev_id_t devid,
                                                bf_dev_port_t port);

/**
 * Clear all counters in Egress deparser block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_deparser_egress_counter_clear(bf_dev_id_t devid,
                                                  bf_dev_pipe_t pipe);

/**
 * Clear all counters in Egress buffer block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_buffer_egress_counter_clear(bf_dev_id_t devid,
                                                bf_dev_port_t port);

/**
 * Clear all tm ingress counters.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_tm_ingress_counter_clear(bf_dev_id_t devid,
                                             bf_dev_port_t port);
/**
 * Clear all tm egress counters.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 */
void bf_packet_path_tm_egress_counter_clear(bf_dev_id_t devid,
                                            bf_dev_port_t port);
/**
 * Read drop  counters from IBP block.
 *
 * @param devid      ASIC device identifier.
 * @param port       Port Identifier.
 * @param ibp_cntrs  Drop/Error counters returned.
 * @return           PIPE_SUCCESS if successful or error status
 */
pipe_status_t bf_pkt_path_ibp_drop_cntr_get(bf_dev_id_t devid,
                                            bf_dev_port_t port,
                                            bf_ibp_cntrs_t *ibp_cntrs);

/**
 * Read drop  counters from Ingress Parser block.
 *
 * @param devid        ASIC device identifier.
 * @param port         Port Identifier.
 * @param iprsr_cntrs  Drop/Error counters returned.
 * @return             PIPE_SUCCESS if successful or error status
 */
pipe_status_t bf_pkt_path_iprsr_drop_cntr_get(bf_dev_id_t devid,
                                              bf_dev_port_t port,
                                              bf_iprsr_cntrs_t *iprsr_cntrs);

/**
 * Read drop  counters from Ingress Deparser block.
 *
 * @param devid        ASIC device identifier.
 * @param pipe         The Logical Pipe Identifier.
 * @param idprsr_cntrs  Drop/Error counters returned.
 * @return             PIPE_SUCCESS if successful or error status
 */
pipe_status_t bf_pkt_path_idprsr_drop_cntr_get(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               bf_idprsr_cntrs_t *idprsr_cntrs);

/**
 * Read drop  counters from Egress  Parser block.
 *
 * @param devid        ASIC device identifier.
 * @param port         The Logical Port Identifier.
 * @param eprsr_cntrs  Drop/Error counters returned.
 * @return             PIPE_SUCCESS if successful or error status
 */
pipe_status_t bf_pkt_path_eprsr_drop_cntr_get(bf_dev_id_t devid,
                                              bf_dev_port_t port,
                                              bf_eprsr_cntrs_t *eprsr_cntrs);

/**
 * Read drop counters from Egress Deparser block.
 *
 * @param devid        ASIC device identifier.
 * @param pipe         The Logical Pipe Identifier.
 * @param edprsr_cntrs  Drop/Error counters returned.
 * @return             PIPE_SUCCESS if successful or error status
 */
pipe_status_t bf_pkt_path_edprsr_drop_cntr_get(bf_dev_id_t devid,
                                               bf_dev_pipe_t pipe,
                                               bf_edprsr_cntrs_t *edprsr_cntrs);

#endif
