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



#ifndef port_mgr_tof3_tmac_h
#define port_mgr_tof3_tmac_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#define TMAC_LINK_REMOTE_FAULT_BASE 12 
#define TMAC_LINK_DOWN_BASE 8
#define TF3_INTR_CH0_LINK_DOWN_RFAULT 0x1100

#define TMAC_BIT_SET(val, pos) ((val) |= (1 << (pos)))
#define TMAC_BIT_CLR(val, pos) ((val) &= ~(1 << (pos)))
#define TMAC_BIT_GET(val, pos) ((val) = (((val) >> (pos)) & 1))
#define TMAC_TOF3_MAX 33
enum tmac_stats_ctrs_ {
  tmac_TxPacketOK,
  tmac_TxPacketAll,
  tmac_TxPacketErr,
  tmac_TxOctetOK,
  tmac_TxOctetAll,
  tmac_TxPacketUnicast,
  tmac_TxPacketMulticast,
  tmac_TxPacketBroadcast,
  tmac_TxPacketPause,
  tmac_TxPacketControl,
  tmac_TxPacketVLAN,
  tmac_TxPacketErrLong,
  tmac_TxPacketErrLength,
  tmac_TxPacketErrShort,
  tmac_TxPacketErrFCS,
  tmac_TxPacketErrFraming,
  tmac_TxPacketLessThan64B,
  tmac_TxPacketEqualTo64B,
  tmac_TxPacket65BTo127B,
  tmac_TxPacket128BTo255B,
  tmac_TxPacket256BTo511B,
  tmac_TxPacket512BTo1023B,
  tmac_TxPacket1024BTo1518B,
  tmac_TxPacket1519BTo2047B,
  tmac_TxPacket2048BTo4095B,
  tmac_TxPacket4096BTo8191B,
  tmac_TxPacket8192BTo9215B,
  tmac_TxPacketGreaterThan9216B,
  tmac_TxPacketPortXOFF,
  tmac_TxPacketPFC0XOFF,
  tmac_TxPacketPFC1XOFF,
  tmac_TxPacketPFC2XOFF,
  tmac_TxPacketPFC3XOFF,
  tmac_TxPacketPFC4XOFF,
  tmac_TxPacketPFC5XOFF,
  tmac_TxPacketPFC6XOFF,
  tmac_TxPacketPFC7XOFF,
  tmac_TxTime1USPortXOFF,
  tmac_TxTime1USPFC0XOFF,
  tmac_TxTime1USPFC1XOFF,
  tmac_TxTime1USPFC2XOFF,
  tmac_TxTime1USPFC3XOFF,
  tmac_TxTime1USPFC4XOFF,
  tmac_TxTime1USPFC5XOFF,
  tmac_TxTime1USPFC6XOFF,
  tmac_TxTime1USPFC7XOFF,
  tmac_TxPTP1StepErr,
  tmac_TxFifoUnderrun,  // 47

  tmac_RxPacketOK,
  tmac_RxPacketAll,
  tmac_RxPacketErr,
  tmac_RxOctetOK,
  tmac_RxOctetAll,
  tmac_RxPacketUnicast,
  tmac_RxPacketMulticast,
  tmac_RxPacketBroadcast,
  tmac_RxPacketPause,
  tmac_RxPacketControl,
  tmac_RxPacketVLAN,
  tmac_RxPacketErrLong,
  tmac_RxPacketErrLength,
  tmac_RxPacketErrShort,
  tmac_RxPacketErrFCS,
  tmac_RxPacketErrFraming,
  tmac_RxPacketLessThan64B,
  tmac_RxPacketEqualTo64B,
  tmac_RxPacket65BTo127B,
  tmac_RxPacket128BTo255B,
  tmac_RxPacket256BTo511B,
  tmac_RxPacket512BTo1023B,
  tmac_RxPacket1024BTo1518B,
  tmac_RxPacket1519BTo2047B,
  tmac_RxPacket2048BTo4095B,
  tmac_RxPacket4096BTo8191B,
  tmac_RxPacket8192BTo9215B,
  tmac_RxPacketGreaterThan9216B,
  tmac_RxPacketPortXOFF,
  tmac_RxPacketPFC0XOFF,
  tmac_RxPacketPFC1XOFF,
  tmac_RxPacketPFC2XOFF,
  tmac_RxPacketPFC3XOFF,
  tmac_RxPacketPFC4XOFF,
  tmac_RxPacketPFC5XOFF,
  tmac_RxPacketPFC6XOFF,
  tmac_RxPacketPFC7XOFF,
  tmac_RxTime1USPortXOFF,
  tmac_RxTime1USPFC0XOFF,
  tmac_RxTime1USPFC1XOFF,
  tmac_RxTime1USPFC2XOFF,
  tmac_RxTime1USPFC3XOFF,
  tmac_RxTime1USPFC4XOFF,
  tmac_RxTime1USPFC5XOFF,
  tmac_RxTime1USPFC6XOFF,
  tmac_RxTime1USPFC7XOFF,
  tmac_RxSFDError,
  tmac_Reserved,  // 95
  TMAC_MAX_RMON_COUNTERS
};

typedef union csr_stats_ {
  uint64_t data64;
  uint32_t data32[2];
} csr_stats_t;

void port_mgr_tof3_tmac_init(bf_dev_id_t dev_id, uint32_t clk_div);
void port_mgr_tof3_tmac_deinit(bf_dev_id_t dev_id);

void port_mgr_tof3_tmac_config(bf_dev_id_t dev_id,
                               uint32_t tmac,
                               uint32_t ch,
                               bf_port_speed_t speed,
                               bf_fec_types_t fec,
                               uint32_t n_lanes,
                               uint32_t n_serdes_lanes);

void port_mgr_tof3_tmac_de_config(bf_dev_id_t dev_id,
                                   uint32_t mac,
                                   uint32_t ch,
                                   bf_port_speed_t speed,
                                   uint32_t n_lanes);

void port_mgr_tof3_tmac_enable(bf_dev_id_t dev_id, uint32_t tmac, uint32_t ch);
void port_mgr_tof3_tmac_disable(bf_dev_id_t dev_id, uint32_t tmac, uint32_t ch);
void port_mgr_tof3_tmac_link_state_get(bf_dev_id_t dev_id,
                                       uint32_t tmac,
                                       uint32_t ch,
                                       bool *up);
void port_mgr_tof3_tmac_sw_reset_tx_set(bf_dev_id_t dev_id,
                                        uint32_t tmac,
                                        uint32_t ch,
                                        bool assert_reset);

void port_mgr_tof3_tmac_sw_reset_rx_set(bf_dev_id_t dev_id,
                                        uint32_t tmac,
                                        uint32_t ch,
                                        bool assert_reset);
void port_mgr_tof3_tmac_sw_reset_set(bf_dev_id_t dev_id,
                                     uint32_t tmac,
                                     uint32_t ch,
                                     bool assert_reset);
bf_status_t port_mgr_tof3_tmac_read_counter(bf_dev_id_t dev_id,
                                            uint32_t tmac,
                                            uint32_t ch,
                                            bf_rmon_counter_t ctr_id,
                                            uint64_t *ctr_value);
void port_mgr_tof3_tmac_lane_map_set(bf_dev_id_t dev_id,
                                     uint32_t tmac,
                                     uint32_t phys_tx_ln[8],
                                     uint32_t phys_rx_ln[8],
                                     uint32_t nlanes);
void port_mgr_tof3_tmac_sigovrd_set(bf_dev_id_t dev_id,
                                    uint32_t tmac,
                                    uint32_t ch,
                                    uint32_t n_lanes,
                                    bf_sigovrd_fld_t ovrd_val);
bf_status_t port_mgr_tof3_tmac_loopback_pipe(bf_dev_id_t dev_id,
                                             uint32_t mac,
                                             uint32_t ch,
                                             bool enable);
bf_status_t port_mgr_tof3_tmac_loopback_pcs_near_set(bf_dev_id_t dev_id,
                                                     uint32_t mac,
                                                     uint32_t ch,
                                                     bool enable);
bf_status_t port_mgr_tof3_tmac_loopback_set(bf_dev_id_t dev_id,
                                            uint32_t tmac,
                                            uint32_t ch,
                                            bf_loopback_mode_e mode);
void port_mgr_tof3_tmac_forced_sigok_get(bf_dev_id_t dev_id,
                                         uint32_t tmac,
                                         uint32_t ch,
                                         uint32_t n_lanes,
                                         uint32_t *force_hi_raw_val,
                                         uint32_t *force_lo_raw_val,
                                         uint32_t *force_hi,
                                         uint32_t *force_lo);
void port_mgr_tof3_tmac_dis_all_set(bf_dev_id_t dev_id,bf_subdev_id_t subdev_id);
void port_mgr_tof3_tmac_int_en_set(bf_dev_id_t dev_id, uint32_t tmac,int ch, bool on);
void port_mgr_tof3_tmac_local_fault_int_en_set(bf_dev_id_t dev_id,
                                               uint32_t tmac,
                                               uint32_t ch,
                                               bool en);
void port_mgr_tof3_tmac_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                uint32_t tmac,
                                                uint32_t ch,
                                                bool en);
void port_mgr_tof3_tmac_tx_local_fault_set(bf_dev_id_t dev_id,
                                           uint32_t tmac,
                                           uint32_t ch,
                                           bool on);
void port_mgr_tof3_tmac_tx_remote_fault_set(bf_dev_id_t dev_id,
                                            uint32_t tmac,
                                            uint32_t ch,
                                            bool on);
void port_mgr_tof3_tmac_tx_idle_set(bf_dev_id_t dev_id,
                                    uint32_t tmac,
                                    uint32_t ch,
                                    bool on);
void port_mgr_tof3_tmac_rx_enable(bf_dev_id_t dev_id,
                                  uint32_t tmac,
                                  uint32_t ch,
                                  bool rx_en);
void port_mgr_tof3_tmac_tx_reset_set(bf_dev_id_t dev_id,
                                     uint32_t tmac,
                                     uint32_t ch);
void port_mgr_tof3_tmac_rx_reset_set(bf_dev_id_t dev_id,
                                     uint32_t tmac,
                                     uint32_t ch);
bf_status_t port_mgr_tof3_tmac_flowcontrol_set(bf_dev_id_t dev_id,
                                               uint32_t tmac,
                                               uint32_t ch);

bf_status_t port_mgr_tof3_tmac_handle_interrupts(bf_dev_id_t dev_id,
                                                 bf_subdev_id_t subdev_id,
                                                 uint32_t ch,
                                                 uint32_t physical_tmac,
                                                 uint32_t int_stats_val);

bf_status_t port_mgr_tof3_tmac_to_rmon_ctr_copy(uint64_t *src_ctr_array,
                                                uint64_t *dst_ctr_array);

void port_mgr_tof3_tmac_clear_stats(bf_dev_id_t dev_id,
                                    uint32_t tmac,
                                    uint32_t ch);

void port_mgr_tof3_tmac_pcs_status_get(bf_dev_id_t dev_id,
                                       uint32_t tmac,
                                       uint32_t ch,
                                       bf_tof3_pcs_status_t *pcs);

void port_mgr_tof3_tmac_fec_status_get(bf_dev_id_t dev_id,
                                       uint32_t tmac,
                                       uint32_t ch,
                                       bf_tof3_fec_status_t *fec);

void port_mgr_tof3_tmac_reset_set(bf_dev_id_t dev_id,
                                  uint32_t mac,
                                  uint32_t ch,
                                  uint32_t val);
void port_mgr_tof3_tmac_fec_lane_symb_err_counter_get(bf_dev_id_t dev_id,
                                  uint32_t mac,
                                  uint32_t ch,
                                  uint32_t n_ctrs,
                                  uint64_t symb_err[16],
				  int lanes_per_ch);
bf_status_t port_mgr_tof3_tmac_enable_interrupts(bf_dev_id_t dev_id,
                                                 uint32_t mac,
                                                 uint32_t ch,
                                                 bool flag);
void port_mgr_tof3_tmac_rs_fec_status_and_counters_get(
    bf_dev_id_t dev_id,
    uint32_t mac,
    uint32_t ch,
    int lanes_per_ch,
    bool *hi_ser,
    bool *fec_align_status,
    uint32_t *fec_corr_cnt,
    uint32_t *fec_uncorr_cnt,
    uint32_t *fec_ser_lane_0,
    uint32_t *fec_ser_lane_1,
    uint32_t *fec_ser_lane_2,
    uint32_t *fec_ser_lane_3,
    uint32_t *fec_ser_lane_4,
    uint32_t *fec_ser_lane_5,
    uint32_t *fec_ser_lane_6,
    uint32_t *fec_ser_lane_7);

void port_mgr_tof3_tmac_tx_enable(bf_dev_id_t dev_id,
                                  uint32_t mac,
                                  uint32_t ch,
                                  bool tx_en);
void port_mgr_tof3_tmac_tx_drain_set(bf_dev_id_t dev_id,
                                  uint32_t mac,
                                  uint32_t ch,
                                  bool tx_drain_en);

bf_status_t port_mgr_tof3_tmac_1588_timestamp_delta_rx_set(bf_dev_id_t dev_id,bf_dev_port_t dev_port,
		uint16_t delta);
bf_status_t port_mgr_tof3_tmac_1588_timestamp_delta_tx_set(bf_dev_id_t dev_id,bf_dev_port_t dev_port,
		uint16_t delta);
bf_status_t port_mgr_tof3_tmac_1588_timestamp_delta_rx_get(bf_dev_id_t dev_id,bf_dev_port_t dev_port,
		uint16_t *delta);
bf_status_t port_mgr_tof3_tmac_1588_timestamp_delta_tx_get(bf_dev_id_t dev_id,bf_dev_port_t dev_port,
		uint16_t *delta);
bf_status_t port_mgr_tof3_tmac_1588_timestamp_tx_get(bf_dev_id_t dev_id,bf_dev_port_t dev_port,
		uint64_t *ts,
		bool *ts_valid,
		int *ts_id);
bf_status_t port_mgr_tof3_tmac_clear_pfc_data_path(bf_dev_id_t dev_id,
                                                  uint32_t mac,
                                                  uint32_t ch);
void port_mgr_tof3_tmac_ignore_fault(bf_dev_id_t dev_id,uint32_t mac,uint32_t ch, bool en);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
