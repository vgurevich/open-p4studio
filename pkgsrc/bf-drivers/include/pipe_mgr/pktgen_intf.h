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
 * @file pktgen_intf.h
 * @date
 *
 * Definitions for configuring packet generator applications.
 */

#ifndef _PKTGEN_INTF_H
#define _PKTGEN_INTF_H

#include <inttypes.h>
#include <stdbool.h>
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/pdfixed/pd_common.h>

typedef uint32_t bf_session_hdl_t;
/**
 * @addtogroup pipe_mgr-recirc
 * @{
 * This is a description of the Recirculation driver APIs.
 */

/*!
 * Enable recirculation functionality.
 * Tofino: All ports in a port group are affected by this API
 *   and the first port in the group should be passed in.
 *   This is only valid for port group 16 in each pipe.
 * Tofino2: Can only configure per channel.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port_base
 *   Tofino: Identifies the port group configure, legal values 64, 192, 320,
 *448.
 *   Tofino2: Identifies the channel number, legal values 2, 3, 4, 5.
 * @return Status of the API call
 */
bf_status_t bf_recirculation_enable(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port_base);
/*!
 * Disable recirculation functionality.
 * Tofino: All ports in a port group are affected by this API
 *   and the first port in the group should be passed in.
 *   This is only valid for port group 16 in each pipe.
 * Tofino2: Can only configure per channel.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port_base
 *   Tofino: Identifies the port group configure, legal values 64, 192, 320,
 *448.
 *   Tofino2: Identifies the channel number, legal values 2, 3, 4, 5.
 * @return Status of the API call
 */
bf_status_t bf_recirculation_disable(bf_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_dev_port_t port_base);

/*!
 * Get recirculation status.
 * @param dev Identifies the ASIC.
 * @param port
 *   Tofino: Identifies the port group configure, legal values 64, 192, 320,
 * 448.
 *   Tofino2: Identifies the channel number, legal values 2, 3, 4, 5.
 * @param is_enabled
 *   recirculation is enabled or not
 * @return Status of the API call
 */

bf_status_t bf_recirculation_get(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool *is_enabled);

/*!
 * Set recirculation status.
 * @param dev Identifies the ASIC.
 * @param shdl BF-driver session handle.
 * @param port
 *   Tofino: Identifies the port group configure, legal values 64, 192, 320,
 * 448.
 *   Tofino2: Identifies the channel number, legal values 2, 3, 4, 5.
 * @param enable recirculation enable - true, disable - false
 * @return Status of the API call
 */
bf_status_t bf_recirculation_set(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool enable);

/**
 * @addtogroup pipe_mgr-pktgen
 * @{
 * This is a description of the Packet Generator driver APIs.
 */

/*!
  Enumerate events which can trigger packet generator applications to generate
  a packet.
 */
typedef enum bf_pktgen_trigger_type {
  BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT =
      PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT, /*!< A packet will be generated when a
            one-shot timer fires. */
  BF_PKTGEN_TRIGGER_TIMER_PERIODIC =
      PD_PKTGEN_TRIGGER_TIMER_PERIODIC, /*!< A packet will be generated when a
            periodic timer fires. */
  BF_PKTGEN_TRIGGER_PORT_DOWN = PD_PKTGEN_TRIGGER_PORT_DOWN, /*!< A packet will
                                      be generated on a port-down
                                      event. A packet generator can be triggered
                                      only by ports in the same pipe.  */
  BF_PKTGEN_TRIGGER_RECIRC_PATTERN =
      PD_PKTGEN_TRIGGER_RECIRC_PATTERN, /*!< Generate a packet on receiving a
       recirculated packet. A filter can be
       applied on the first 32b(for tofino)/128b(for tofino2) of the
       recirculated packet to determine if it
       should trigger the packet generator.  */
  BF_PKTGEN_TRIGGER_DPRSR = PD_PKTGEN_TRIGGER_DPRSR, /*!< Only for tofino2. A
                                  packet will be generated when the
                                  metadata(from deparser) matches the
                                  congiguration */
  BF_PKTGEN_TRIGGER_PFC = PD_PKTGEN_TRIGGER_PFC /*!< Only for tofino2, only one
                               application per pipe can be configured. A packet
                               will be generated when a PFC change event is
                               triggered, Only support
                               one pfc application every pipe */
} bf_pktgen_trigger_type_e;

/**
 * @struct bf_pktgen_pattern_trigger
 * @brief Defines the value and mask used in a @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN trigger.
 * Only for Tofino.
 * Note that this is a ternary comparison; zero bits in the mask indicate
 * "don't care".  This comparison is performed against the first 32 bits of the
 * recirculated packet.
 * Note that @ref bf_pktgen_enable_recirc_pattern_matching and
 * @ref bf_pktgen_disable_recirc_pattern_matching determine which recirculated
 * packets are checked.
 */
struct bf_pktgen_pattern_trigger {
  uint32_t value; /**< If the first 32 bits of the packet match this value
                        the event will fire. */
  uint32_t mask;  /**< This determines which bits of value are compared with
                       the first 32 bits of the recirculated packet.  Zero is
                       don't care, one is match. */
};
/**
 * @struct bf_tof2_pktgen_pattern_trigger
 * @brief Defines the value and mask used in a @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN trigger.
 * Only for Tofino2.
 * Note that this is a ternary comparison; zero bits in the mask indicate
 * "don't care".  This comparison is performed against the first 128 bits of the
 * recirculated packet.
 * Note that @ref bf_pktgen_enable_recirc_pattern_matching and
 * @ref bf_pktgen_disable_recirc_pattern_matching determine which recirculated
 * packets are checked.
 */
struct bf_tof2_pktgen_pattern_trigger {
  uint8_t value[16]; /**< If the first 128 bits of the packet match this value
                        the event will fire. value[0] is the first byte*/
  uint8_t mask[16];  /**< This determines which bits of value are compared with
                      the first 128 bits of the recirculated packet.  Zero is
                      don't care, one is match. mask[0] is the first byte*/
};
typedef struct bf_tof2_pktgen_pattern_trigger bf_tof3_pktgen_pattern_trigger_t;
/**
 * @struct bf_tof2_port_down_sel
 * @brief port down event mask
 * Only for Tofino2, Tofino3
 */
struct bf_tof2_port_down_sel {
  uint8_t port_mask[9]; /**< bit vector for ports within a pipe.
                              one bit for one port, 1:enable, 0:disable
                              within a port_mask byte, the least significant bit
                              corresponds to the smaller port number; for
                              example bit 0 of byte 1 corresponds to port 8 and
                              bit 7 of byte 1 corresponds to port 15.
                              port_mask[0]:port 0-7
                              port_mask[1]:port 8-15
                              port_mask[2]:port 16-23
                              port_mask[3]:port 24-31
                              port_mask[4]:port 32-39
                              port_mask[5]:port 40-47
                              port_mask[6]:port 48-55
                              port_mask[7]:port 56-63
                              port_mask[8]:port 64-71
                              (0-3 pcie, 4-7 eth cpu, 8-71 front panel ports)*/
  uint32_t val_0;
  uint32_t val_1;
  uint32_t val_2;
};
typedef struct bf_tof2_port_down_sel bf_tof3_port_down_sel_t;

/**
 * @
 * Only work for Tofino2/Tofino3
 */
struct bf_pktgen_tof2_cfg {
  bool offset_len_from_recir_pkt; /**< Only work for tofino2.
                                       When set, pkt_buffer_offset and length
                                       are from recirculation packet */
  uint8_t source_port_wrap_max;   /**< Only work for Tofino2. Sets max value of
                                     source port in the batch after which it will
                                     wrap back to pipe_local_source_port */
  uint8_t assigned_chnl_id; /**< Only work for Tofino2. Sets the channel that
                               the application belong to. legal value: 0-7 */
};
typedef struct bf_pktgen_tof2_cfg bf_pktgen_tof3_cfg_t;

/**
 * @
 * Only for Tofino-2 and Tofino-3
 * Special configuration for pfc trigger
 */
struct bf_tof2_pktgen_pfc_trigger {
  uint8_t pfc_hdr[16];   /**< it would be attached after pktgen_hdr in pktgen
                            pkts, pfc_hdr[0] would be the first byte */
  bool cfg_timer_en;     /**< enable timeout to trigger pfc event */
  uint16_t cfg_timer;    /**< timeout */
  uint16_t pfc_max_msgs; /**< Maximum number of PFC events to pack in a single
                            packet; legal values are 1-1023. */
};
typedef struct bf_tof2_pktgen_pfc_trigger bf_tof3_pktgen_pfc_trigger_t;

/**
 * @struct bf_pktgen_app_cfg_t
 * @brief Defines the configuration of one packet generator application.
 *
 * There are eight applications per pipeline, each with separate configuration.
 */
struct bf_pktgen_app_cfg_t {
  bf_pktgen_trigger_type_e trigger_type; /**< Sets the type of event to
                                              trigger the application. */
  uint16_t batch_count; /**< The number of batches to send when the application
                             is triggered.  Zero based.*/
  uint16_t packets_per_batch; /**< The number of packets within each batch.
                                   Zero based. */
  union {
    struct bf_pktgen_pattern_trigger
        pattern; /**< Only for tofino, when using @ref
                    BF_PKTGEN_TRIGGER_RECIRC_PATTERN trigger */
    struct bf_tof2_pktgen_pattern_trigger
        pattern_tof2; /**< Only for tofino2, Tofino3, when using @ref
                         BF_PKTGEN_TRIGGER_RECIRC_PATTERN or @ref
                         BF_PKTGEN_TRIGGER_DPRSR triggers */
    struct bf_tof2_pktgen_pfc_trigger pfc_cfg; /**< Only for
                                                       tofino2,tofino3 when
                                                       using @ref
                                                       BF_PKTGEN_TRIGGER_PFC
                                                       trigger */
    uint32_t port_mask_sel_tof2; /**< Only for tofino2, tofino3 legal value
                                    0/1, when
                                    using @ref BF_PKTGEN_TRIGGER_PORT_DOWN
                                    trigger*/
    uint32_t timer_nanosec;      /**< Represents the length of the timer when
                                      using @ref BF_PKTGEN_TRIGGER_TIMER_ONE_SHOT
                                      or @ref BF_PKTGEN_TRIGGER_TIMER_PERIODIC
                                      triggers. */
  } u;
  uint32_t ibg;        /**< The number of nanoseconds between the start of two
                            consecutive generated batches. */
  uint32_t ibg_jitter; /**< The average jitter, in nanoseconds, which will
                            be added to the ibg. */
  uint32_t ipg;        /**< The number of nanoseconds between the start of two
                            consecutive packets in a batch. */
  uint32_t ipg_jitter; /**< The average jitter, in nanoseconds, which will
                            be added to the ipg. */
  bf_dev_port_t
      pipe_local_source_port; /**< Sets the ingress src port number for
                                   generated packets. This is only the
                                   lower bits of the port number.
                                   the logical pipe id is fixed to the
                                   packet generator is assocaited with
                                   Note:
                                     Tofino: the generated
                                   packets will only consume space in input
                                   buffer 17, in the ingress pipeline.
                                     Tofino2: the generated
                                   packets will only consume space in input
                                   buffer 0, in the ingress pipeline.*/
  bool increment_source_port; /**< When true, the source port number is
                                   incremented by one for each packet in a
                                   batch. */
  uint16_t pkt_buffer_offset; /**< An offset into the packet generator's 16KB
                                   byte buffer holding packet templates.
                                   Generated packets will source the packet
                                   contents from this location.  Note this
                                   must be 16B aligned. */
  uint16_t length; /**< The length of the generated packet.  This controls
                        how much data is read from the packet generator's 16KB
                        buffer. */
  struct bf_pktgen_tof2_cfg tof2; /**< Only work for tofino2/tofino3. */
};

typedef struct bf_pktgen_app_cfg_t bf_pktgen_app_cfg_t;

void pd_cfg_to_bf_pktgen_app_cfg(bf_pktgen_app_cfg_t *dest,
                                 p4_pd_pktgen_app_cfg *const src);
void pd_cfg_to_bf_pktgen_app_cfg_tof2(bf_pktgen_app_cfg_t *dest,
                                      p4_pd_pktgen_app_cfg_tof2 *const src);
void bf_cfg_to_pd_pktgen_app_cfg(p4_pd_pktgen_app_cfg *dest,
                                 bf_pktgen_app_cfg_t *const src);
void bf_cfg_to_pd_pktgen_app_cfg_tof2(p4_pd_pktgen_app_cfg_tof2 *dest,
                                      bf_pktgen_app_cfg_t *const src);

/*!
 * Enable packet generator functionality on the specified port.  Generated
 * packets in a given pipe will be round robin scheduled across all enabled
 * ports in that pipe.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_enable(bf_session_hdl_t shdl,
                             bf_dev_id_t dev,
                             bf_dev_port_t port);

/*!
 * Disable packet generator functionality on the specified port.  Generated
 * packets in a given pipe will be round robin scheduled across all enabled
 * ports in the pipe.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_disable(bf_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_dev_port_t port);

/*!
 * Get packet generator enable/disable on the specified port.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to query;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @param *is_enabled packet generator enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_enable_get(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool *is_enabled);

/*!
 * Set packet generator enable/disable on the specified port.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to query;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @param enable packet generator enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_enable_set(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool enable);

/*!
 * Enable monitoring of recirculated packets on the specified port for use with
 * the @ref BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR(only
 * tofino2) type of event.
 * on ports with this monitoring enabled can trigger @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR
 * events.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_enable_recirc_pattern_matching(bf_session_hdl_t shdl,
                                                     bf_dev_id_t dev,
                                                     bf_dev_port_t port);
/*!
 * Disable monitoring of recirculated packets on the specified port for use with
 * the @ref BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR type of
 * event.
 * on ports with this monitoring enabled can trigger @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR
 * events.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_disable_recirc_pattern_matching(bf_session_hdl_t shdl,
                                                      bf_dev_id_t dev,
                                                      bf_dev_port_t port);

/*!
 * Get enable/disable state of monitoring recirculated packets on the specified
 *port
 * for use with the @ref
 *BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR(
 * only tofino2) type of event.
 * on ports with this monitoring enabled can trigger @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR events.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @param *is_enabled pattern matching enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_recirc_pattern_matching_get(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bool *is_enabled);

/*!
 * Set enable/disable state of monitoring recirculated packets on the specified
 * port for use with the
 * @ref BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR(only tofino2)
 * type of event.
 * On ports with this monitoring enabled can trigger @ref
 * BF_PKTGEN_TRIGGER_RECIRC_PATTERN/BF_PKTGEN_TRIGGER_DPRSR events.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to configure;
 * legal values tofino:  68-71, 196-199, 324-327, and 452-455,
 *              tofino2: 0-7, 128-135, 256-263, 384-391.
 * @param enable pattern matching enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_recirc_pattern_matching_set(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bool enable);

/*!
 * Packet generator @ref BF_PKTGEN_TRIGGER_PORT_DOWN triggers only happen for
 * the first time a port goes down.  Subsequent down events will not trigger
 * another packet generator event until this API is called.  It's also needed to
 * be called for the first time trigger. It should be called once all control
 * plane processing for the down event has been completed.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to clear.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_clear_port_down(bf_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_port_t port);

/*!
 * Get the status of whether port down event has been cleared or not
 * Packet generator @ref BF_PKTGEN_TRIGGER_PORT_DOWN triggers only happen for
 * the first time a port goes down.  Subsequent down events will not trigger
 * another packet generator if the port down event has not been cleared.
 *
 * @param shdl BF-driver session handle.
 * @param dev Identifies the ASIC.
 * @param port Identifies the port to clear.
 * @param *is_cleared port down event has been cleared or not
 * @return Status of the API call
 */
bf_status_t bf_pktgen_port_down_get(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool *is_cleared);

/*!
 * Configure a packet generator application.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param cfg Structure holding the configuration of the application.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_cfg_app(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bf_pktgen_app_cfg_t *cfg);

/*!
 * Get a packet generator application.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param *cfg Structure holding the configuration of the application.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_cfg_app_get(bf_session_hdl_t shdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  bf_pktgen_app_cfg_t *cfg);

/*!
 * Enable a packet generator application.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_app_enable(bf_session_hdl_t shdl,
                                 bf_dev_target_t dev_tgt,
                                 int app_id);

/*!
 * Disable a packet generator application. The packet generator application
 * needs to be disabled before any configuration of the application can be
 * changed.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_app_disable(bf_session_hdl_t shdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id);

/*!
 * Get a packet generator is enable/disable.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param *is_enabled packet generator is enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_app_get(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bool *is_enabled);

/*!
 * Set a packet generator to enable/disable.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param enable packet generator set to enable/disable
 * @return Status of the API call
 */
bf_status_t bf_pktgen_app_set(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bool enable);

/*!
 * Writes data to the 16KB buffer. This buffer is shared by all packet
 * generator apps in the same pipe. Every app sources its own payload from
 * this buffer using @ref bf_pktgen_cfg_app_payload.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies a pipe.
 * @param pktgen_byte_buf_offset The offset in the packet generator's buffer to
 *                               write.  This must be 16B aligned.
 * @param size The number of bytes to write.
 * @param buf A pointer to the data to be written.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                       bf_dev_target_t dev_tgt,
                                       uint32_t pktgen_byte_buf_offset,
                                       uint32_t size,
                                       const uint8_t *buf);

/*!
 * Clear data in buffer. This buffer is shared by all packet
 * generator apps in the same pipe. Every app sources its own payload from
 * this buffer using @ref bf_pktgen_cfg_app_payload.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies dev and pipe.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_clear_pkt_buffer(bf_session_hdl_t shdl,
                                       bf_dev_target_t dev_tgt);

/*!
 * Get data from the 16KB buffer. This buffer is shared by all packet
 * generator apps in the same pipe. Every app sources its own payload from
 * this buffer using @ref bf_pktgen_cfg_app_payload.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies a pipe.
 * @param pktgen_byte_buf_offset The offset in the packet generator's buffer to
 *                               get.  This must be 16B aligned.
 * @param size The number of bytes to get.
 * @param buf A pointer to the return data.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_pkt_buffer_get(bf_session_hdl_t shdl,
                                     bf_dev_target_t dev_tgt,
                                     uint32_t pktgen_byte_buf_offset,
                                     uint32_t size,
                                     uint8_t *buf);

/*!
 * Returns the number of batches of packets generated by the specified packet
 * generator app. This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param batch_cnt Pointer to return the count.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_get_batch_counter(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        uint64_t *batch_cnt);

/*!
 * Returns the number of packets generated by the specified packet generator
 * app. This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param pkt_cnt Pointer to return the count.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_get_pkt_counter(bf_session_hdl_t shdl,
                                      bf_dev_target_t dev_tgt,
                                      int app_id,
                                      uint64_t *pkt_cnt);

/*!
 * Returns the number of times the specified packet generator app was
 * triggered. This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param trigger_cnt Pointer to return the count.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_get_trigger_counter(bf_session_hdl_t shdl,
                                          bf_dev_target_t dev_tgt,
                                          int app_id,
                                          uint64_t *trigger_cnt);

/*!
 * Sets the number of batches of packets generated by the specified packet
 * generator app. This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param count Value to set.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_set_batch_counter(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        uint64_t count);

/*!
 * Sets the number of packets generated by the specified packet generator app.
 * This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param count Value to set.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_set_pkt_counter(bf_session_hdl_t shdl,
                                      bf_dev_target_t dev_tgt,
                                      int app_id,
                                      uint64_t count);

/*!
 * Sets the number of times the specified packet generator app was triggered.
 * This is a 48b counter.
 *
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe of the packet generator application.
 * @param app_id Identifies one of eight packet gen applications in the pipe;
 * legal values tofino: 0-7, tofino2: 0-15.
 * @param count Value to set.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_set_trigger_counter(bf_session_hdl_t shdl,
                                          bf_dev_target_t dev_tgt,
                                          int app_id,
                                          uint64_t count);

/*!
 * Sets port mask for BF_PKTGEN_TRIGGER_PORT_DOWN event. Only for Tofino2
 * @param shdl BF-driver session handle.
 * @param port_mask_sel, index of two port mask sets. It would be used in app
 * configuration. legal values are 0-1
 * @param port mask. follow the format of struct bf_tof2_port_down_sel;
 * @return Status of the API call
 */
bf_status_t bf_pktgen_cfg_port_down_mask(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         uint32_t port_mask_sel,
                                         struct bf_tof2_port_down_sel *mask);

bf_status_t bf_pktgen_port_down_mask_get(bf_dev_target_t dev_tgt,
                                         uint32_t port_mask_sel,
                                         struct bf_tof2_port_down_sel *mask);
typedef enum bf_pktgen_port_down_mode_t {
  BF_PKTGEN_PORT_DOWN_REPLAY_NONE =
      PD_PKTGEN_PORT_DOWN_REPLAY_NONE, /*!< No events will be generated upon
          app enable for currently down ports.*/
  BF_PKTGEN_PORT_DOWN_REPLAY_ALL =
      PD_PKTGEN_PORT_DOWN_REPLAY_ALL, /*!< An event will be triggered for all
           down ports upon app enable. */
  BF_PKTGEN_PORT_DOWN_REPLAY_MISSED =
      PD_PKTGEN_PORT_DOWN_REPLAY_MISSED /*!< An event will be triggered for all
         down ports which have not generated
         events upon app enable. */
} bf_pktgen_port_down_mode_t;

static inline const char *bf_pktgen_port_down_mode_string(
    bf_pktgen_port_down_mode_t mode) {
  switch (mode) {
    case BF_PKTGEN_PORT_DOWN_REPLAY_NONE:
      return "BF_PKTGEN_PORT_DOWN_REPLAY_NONE";
    case BF_PKTGEN_PORT_DOWN_REPLAY_ALL:
      return "BF_PKTGEN_PORT_DOWN_REPLAY_ALL";
    case BF_PKTGEN_PORT_DOWN_REPLAY_MISSED:
      return "BF_PKTGEN_PORT_DOWN_REPLAY_MISSED";
  }
  return "Unknown";
}

/*!
 * Configures the behavior of port down applications when then are enabled.
 * They can immediately trigger for all down ports, immediately trigger for all
 * down ports that have not already generated an event, or only trigger when a
 * port goes down (i.e. not generate triggers for ports that went down before
 * the app was enabled).
 * Note that Tofino only supports BF_PKTGEN_PORT_DOWN_REPLAY_NONE.
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe to configure, BF_DEV_PIPE_ALL for all
 *                pipes.
 * @param mode Specify the mode to configure.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_port_down_replay_mode_set(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode);

/*!
 * Gets the configured the behavior of port down applications when then are
 * enabled.  The corresponding set API is bf_pktgen_port_down_replay_mode_set.
 * @param shdl BF-driver session handle.
 * @param dev_tgt Identifies the pipe to query, BF_DEV_PIPE_ALL for all pipes.
 * @param mode The queried mode will be written here.
 * @return Status of the API call
 */
bf_status_t bf_pktgen_port_down_replay_mode_get(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode);

/*!
 * Gets the number of the last application id
 * enabled.
 * @param dev_id Identifies the device id to query.
 * @return Application count
 */
uint32_t bf_pktgen_get_app_count(bf_dev_id_t dev_id);

/*!
 * @brief Get pktgen port number on the device. Based on skew and
 * asci-type the API will return appropriate port number to be used as
 * pktgen port.
 * @param[in] dev_id The ASIC id.
 * @return Valid pktgen port. In case of error returns -1.
 */
bf_dev_port_t bf_pktgen_port_get(bf_dev_id_t dev_id);

/**
 * @brief Get next pktgen port number on the device. Based on skew and
 * asci-type the API will return appropriate port number to be used as
 * pktgen port.
 * @param[in] dev_id The ASIC id.
 * @param[in/out] port The port number.
 * @return Status of the API call.
 */
bf_status_t bf_pktgen_get_next_port(bf_dev_id_t dev_id, bf_dev_port_t *port);

/**
 * @brief Get maximum pktgen port number on the device. Based on skew and
 * asci-type the API will return appropriate port number to be used as
 * pktgen port.
 * @param[in] dev_id The ASIC id.
 * @return Valid max pktgen port. In case of error returns -1.
 */
bf_dev_port_t bf_pktgen_max_port_get(bf_dev_id_t dev_id);

#endif /* _PKTGEN_INTF_H */
