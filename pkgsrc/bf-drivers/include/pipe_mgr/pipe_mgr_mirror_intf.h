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


/*!
 * @file pipe_mgr_mirror_intf.h
 * @date
 *
 * Definitions for interfaces to pipeline manager
 */

#ifndef _PIPE_MGR_MIRROR_INTF_H
#define _PIPE_MGR_MIRROR_INTF_H

/* Standard includes */
#include <stdint.h>
#include <stdbool.h>

/* Module header files */
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <traffic_mgr/traffic_mgr_types.h>
#include <tofino/pdfixed/pd_common.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif
/*!
 * APIs for mirror session management
 * Only for Tofino.
 * Please use functions:
 * bf_mirror_get_max_sessions() to get session max.
 * bf_mirror_get_base_session_id() to get base session id
 */
#define BF_MIRROR_SESSION_MAX 1024
#define BF_MIRROR_COAL_SESSION_MAX 8
#define BF_MIRROR_COAL_BASE_SID \
  (BF_MIRROR_SESSION_MAX - BF_MIRROR_COAL_SESSION_MAX)
#define BF_MIRROR_NEG_SID (BF_MIRROR_COAL_BASE_SID - 1)
/*
 * Mirroring session type and directions
 */
typedef enum {
  BF_MIRROR_TYPE_NORM = PD_MIRROR_TYPE_NORM, /*!< Normal Mirroring Session */
  BF_MIRROR_TYPE_COAL =
      PD_MIRROR_TYPE_COAL, /*!< Coalescing Mirroring Session (e2e only) */
  BF_MIRROR_TYPE_MAX = PD_MIRROR_TYPE_MAX
} bf_mirror_type_e;

typedef enum {
  BF_DIR_NONE = PD_DIR_NONE,
  BF_DIR_INGRESS =
      PD_DIR_INGRESS, /*!< Ingress to Egress (i2e) Mirroring Session */
  BF_DIR_EGRESS =
      PD_DIR_EGRESS,        /*!< Egress to Egress (e2e) Mirroring Session */
  BF_DIR_BOTH = PD_DIR_BOTH /*!< Both i2e and e2e Mirroring Session */
} bf_mirror_direction_e;

typedef enum {
  BF_HASH_CFG =
      PD_HASH_CFG, /*!< Hash cfg from session table(1,default) or MAU(0) */
  BF_HASH_CFG_P = PD_HASH_CFG_P, /*!< Vaild only Hash cfg from session table
                                  *   from hash1(1) or from hash2(0) */
  BF_ICOS_CFG = PD_ICOS_CFG, /*!< ICOS cfg from table(1,default) or MAU(0) */
  BF_DOD_CFG = PD_DOD_CFG,   /*!< Deflect_on_drop cfg from session
                              * table(1,default) or MAU(0)
                              */
  BF_C2C_CFG =
      PD_C2C_CFG, /*!< Copy to cpu cfg from session table(1,default) or MAU(0)
                   */
  BF_MC_CFG =
      PD_MC_CFG, /*!< Multicast cfg from session table(1,default) or MAU(0) */
  BF_EPIPE_CFG =
      PD_EPIPE_CFG /*!< Epipe cfg from session table(1,default) or MAU(0) */
} bf_mirror_meta_flag_e;

typedef struct bf_mirror_session_info {
  bf_mirror_type_e mirror_type; /*!< Mirror type */
  bf_mirror_direction_e dir;    /*!< Mirror direction */
  // parameters that tofino needs per norm mirroring session
  uint32_t pipe_mask;              /*!< output pipes - used with mcast */
  uint32_t ucast_egress_port;      /*!< mirror destination port */
  bool ucast_egress_port_v;        /*!< mirror destination port - valid */
  bf_tm_queue_t egress_port_queue; /*!< mirror destination port queue */
  uint32_t ingress_cos;            /*!< cos assigned to mirrored copy */
  bf_tm_color_t packet_color;      /*!< color used for mirrored copy */
  uint32_t level1_mcast_hash;      /*!< outer hash value - to select path */
  uint32_t level2_mcast_hash;      /*!< inner hash value - to select path */
  uint16_t mcast_grp_a;            /*!< multicast group id - outer */
  bool mcast_grp_a_v;              /*!< multicast group id outer - valid */
  uint16_t mcast_grp_b;            /*!< multicast group id inner */
  bool mcast_grp_b_v;              /*!< multicast group id inner - valid */
  uint16_t mcast_l1_xid;           /*!< multicast level 1 exclusion id */
  uint16_t mcast_l2_xid;           /*!< multicast level 2 exclusion id */
  uint16_t mcast_rid;              /*!< multicast ingress RID */
  uint32_t icos_for_copy_to_cpu;   /*!< cos for packet to cpu */
  bool copy_to_cpu;                /*!< copy mirrored packet to cpu */
  uint32_t deflect_on_drop; /*!< TM can defelct the packet incase of drop */
  uint16_t max_pkt_len;     /*!< Maximum packet len of a mirrored copy */
  // coalescing session parameters
  // valid/used only if type is coalesing
  uint32_t header[4];    /*!< Internal Header upto 16Bytes, added to
                          *   coalesced packet. This header is
                          *   removed/processed by egress parser
                          *   Two ms-bytes of header[0] are reserved
                          *   for internal use
                          *   For TOF2, header[0] is reserved
                          *   for internal use
                          */
  uint32_t header_len;   /*!< Length of the internal header,
                          *   in terms of words
                          */
  uint32_t timeout_usec; /*!< N*BaseTime, after which coalesced packet
                          *  is schedule for transmittion
                          */
  uint32_t extract_len;  /*!< Number of bytes extracted from
                          *   each packet sample, including a sample_hdr
                          *   (max 80B or 240B, see Tofino documentation)
                          */
  uint32_t ver;
  bool extract_len_from_p4; /*!< Extraction length provided by
                             * P4 program, This overrides the default
                             */
} bf_mirror_session_info_t;

typedef struct bf_mirror_get_id_t {
  bf_mirror_id_t sid;    /*!< Mirror Session Id */
  bf_dev_pipe_t pipe_id; /*!< Pipe id the session was configured with */
} bf_mirror_get_id_t;

/*!
 * Set a normal or non-coalescing mirror session
 *
 * @param sess_hdl: API session
 * @param dev_target : Device and pipe where this session is being created
 * @param type : Mirror type - Normal/coalescing session
 * @param dir : I2E, E2E or both
 * @param sid : session id, must be between
 *          0-1014 for normal, 1016-1023 for coalesing
 * @param enable : enable the session after setting the parameters
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_set(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_target,
                                    bf_mirror_id_t sid,
                                    bf_mirror_session_info_t *s_info,
                                    bool enable);

/*!
 * Reset a mirror session created earlier
 * The session information is cleared
 *
 * @param sess_hdl: API session
 * @param sid : session_id to be reset
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_reset(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_target,
                                      bf_mirror_id_t sid);

/*!
 * Enable a mirror session created earlier
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param dir : i2e, e2e, both
 * @param sid : session_id to be enabled
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_enable(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_target,
                                       bf_mirror_direction_e dir,
                                       bf_mirror_id_t sid);

/*!
 * Disable a mirror session created earlier
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param dir : i2e, e2e, both
 * @param sid : session_id to be disabled
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_disable(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_target,
                                        bf_mirror_direction_e dir,
                                        bf_mirror_id_t sid);

/*!
 * Set the multicast pipe vector for a mirror session.  This is a bitmap of
 * pipes for ports in the multicast groups used by the session.  For example, if
 * the session used multicast groups 1 and 2 and group 1 replicated to port 5
 * and group 2 replicated to ports 12 and 130 then the total set of ports would
 * be (5, 12, 130) and by taking the pipe each of those ports is in would give a
 * set of (0, 1) since 5 and 12 are in pipe 0 and 130 is in pipe 1.  This leads
 * to a pipe vector with bits zero and one set, 0x3
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to program.
 * @param logical_pipe_vector : The pipe vector to program
 * @return : The API status.
 */
bf_status_t bf_mirror_session_mcast_pipe_vector_set(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_id_t sid,
                                                    int logical_pipe_vector);

/*!
 * Get the multicast pipe vector for a mirror session.
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to query.
 * @param logical_pipe_vector : The pipe vector is returned here.
 * @return : The API status.
 */
bf_status_t bf_mirror_session_mcast_pipe_vector_get(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_id_t sid,
                                                    int *logical_pipe_vector);

/*!
 * Get max sessions
 *
 * @param sess_hdl: API session
 * @param device_id : Target device
 * @param mirror_type : Normal or Coalesing
 * @param sid : Max session id to get
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_max_mirror_sessions_get(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                bf_mirror_type_e mirror_type,
                                                bf_mirror_id_t *sid);

/*!
 * Get total number of mirror sessions configured
 *
 * @param sess_hdl: API session
 * @param device_id : Target device
 * @param count : total number of sessions to get
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_get_count(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_target,
                                          uint32_t *count);

/*!
 * Get bse session id
 *
 * @param sess_hdl: API session
 * @param device_id : Target device
 * @param mirror_type : Normal or Coalesing
 * @param sid : base session id to get
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_base_mirror_session_id_get(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t device_id,
                                                   bf_mirror_type_e mirror_type,
                                                   bf_mirror_id_t *sid);

/*!
 * Update flags in session meta entry. Only for Tofino2.
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to be updated
 * @param mirror_flag : Update which flag
 * @param value: 1/0
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_meta_flag_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool value);

/*!
 * Get flags in session meta entry. Only for Tofino2.
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to be updated
 * @param mirror_flag : Get the specificed flag
 * @param value: output paramter for the specified flag
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_meta_flag_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_target,
                                              bf_mirror_id_t sid,
                                              bf_mirror_meta_flag_e mirror_flag,
                                              bool *value);

/*!
 * Update priority, default low priority. Only for Tofino2.
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id
 * @param value: true:low priority, false:high priority
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_priority_update(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_target,
                                                bf_mirror_id_t sid,
                                                bool value);

/*!
 * Get priority, default low priority. Only for Tofino2.
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id
 * @param value: output paramter, true:low priority, false:high priority
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_priority_get(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_target,
                                             bf_mirror_id_t sid,
                                             bool *value);

/*!
 * Update coalescing mode
 *
 * Coalesced packets are formed as a sequence of slices of data from
 * different packets, and Tofino only have one mode of coalescing,
 * so the api is specific to tofino2
 *
 * The default value is true, which is the Tofino1 way.
 * If the value is set to false, tofino2 specific mode is applied.
 * Tofino2 (Jbay) supports Coalescing with a user defined header
 * for static session configuration information
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to be updated
 * @param value: true:same with tofino, false: tofino2 mode
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_coal_mode_update(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 bool value);

/*!
 * Get coalescing mode specific for Tofino2.
 *
 * Coalesced packets are formed as a sequence of slices of data from
 * different packets, and Tofino only have one mode of coalescing,
 * so the api is specific to tofino2
 *
 * The default value is true, which is the Tofino1 way.
 * If the value is false, tofino2 specific mode is applied.
 * Tofino2 (Jbay) supports Coalescing with a user defined header
 * for static session configuration information
 *
 * @param sess_hdl: API session
 * @param dev_target : target device, pipe information
 * @param sid : session_id to be updated
 * @param value: ouput paramter, true:same with tofino, false: tofino2 mode
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_coal_mode_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_target,
                                              bf_mirror_id_t sid,
                                              bool *value);

/*!
 * Get the Mirror Session info for given session id
 *
 * @param sess_hdl: API session
 * @param dev_target : Target device and pipe, use BF_DEV_PIPE_ALL to get
 *                     sessions configured with any pipe value.  Use specific
 *                     pipe values (e.g. 0, 1, ...) to get sessions only on that
 *                     specific pipe.
 * @param sid : mirror session_id
 * @return : s_info : Mirror Session Info
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_get(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_target,
                                    bf_mirror_id_t sid,
                                    bf_mirror_session_info_t *s_info);

/*!
 * Get the Mirror session_enable Field info for given session id
 *
 * @param sess_hdl: API session
 * @param dev_target : Target device and pipe, use BF_DEV_PIPE_ALL to get
 *                     sessions enabled flag configured with any pipe value.
 * Use specific pipe values (e.g. 0, 1, ...) to get sessions only on that
 *                     specific pipe.
 * @param sid : mirror session_id
 * @return : session_enable : Mirror Session Enabled Info
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_enable_get(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_target,
                                           bf_mirror_id_t sid,
                                           bool *session_enable);

/*!
 * Get the first existing Mirror Session info
 *
 * @param sess_hdl: API session
 * @param dev_target : Target device and pipe, use BF_DEV_PIPE_ALL to get
 *                     sessions configured with any pipe value.  Use specific
 *                     pipe values (e.g. 0, 1, ...) to get sessions only on that
 *                     specific pipe.
 * @return : s_info : Mirror Session Info
 * @return : first : Mirror Session Id and Pipe Id the session was configured
 *                   with.
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_get_first(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_target,
                                          bf_mirror_session_info_t *s_info,
                                          bf_mirror_get_id_t *first);

/*!
 * Get the next existing Mirror Session info
 *
 * @param sess_hdl: API session
 * @param dev_target : Target device and pipe, use BF_DEV_PIPE_ALL to get
 *                     sessions configured with any pipe value.  Use specific
 *                     pipe values (e.g. 0, 1, ...) to get sessions only on that
 *                     specific pipe.
 * @param current : The value returned in 'first' from
 *                  bf_mirror_session_get_first or the value returned in 'next'
 *                  from bf_mirror_session_get_next.  This is the mirror
 *                  session id and pipe-id it was configured with from which
 *                  the get-next search will begin.
 * @return : next_info, Mirror Session Info
 * @return : next, Mirror Session Id and Pipe Id the session was configured
 *                 with.
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_session_get_next(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_target,
                                         bf_mirror_get_id_t current,
                                         bf_mirror_session_info_t *next_info,
                                         bf_mirror_get_id_t *next);
/*!
 * Mirror reconfiguration
 *
 * @param sess_hdl: API session
 * @param device_id : target device
 * @param phy_pipe_id: physical pipe id to correct
 * @param sid : session_id to correct
 * @return : pipe manager status codes
 */
pipe_status_t bf_mirror_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t device_id,
                                    bf_dev_pipe_t phy_pipe_id,
                                    bf_mirror_id_t sid);
#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _PIPE_MGR_MIRROR_INTF_H */
