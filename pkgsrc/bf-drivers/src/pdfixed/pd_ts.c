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


#include <tofino/pdfixed/pd_ts.h>
#include <lld/bf_ts_if.h>
#include <port_mgr/bf_port_if.h>

p4_pd_status_t p4_pd_ts_global_ts_state_set(p4_pd_ts_dev_t dev, bool enable) {
  return bf_ts_global_ts_state_set(dev, enable);
}

p4_pd_status_t p4_pd_ts_global_ts_state_get(p4_pd_ts_dev_t dev, bool *enable) {
  return bf_ts_global_ts_state_get(dev, enable);
}

p4_pd_status_t p4_pd_ts_global_ts_value_set(p4_pd_ts_dev_t dev,
                                            uint64_t global_ts) {
  return bf_ts_global_ts_value_set(dev, global_ts);
}

p4_pd_status_t p4_pd_ts_global_ts_value_get(p4_pd_ts_dev_t dev,
                                            uint64_t *global_ts) {
  return bf_ts_global_ts_value_get(dev, global_ts);
}

p4_pd_status_t p4_pd_ts_global_ts_inc_value_set(p4_pd_ts_dev_t dev,
                                                uint32_t global_inc_ns) {
  return bf_ts_global_ts_inc_value_set(dev, global_inc_ns);
}

p4_pd_status_t p4_pd_ts_global_ts_inc_value_get(p4_pd_ts_dev_t dev,
                                                uint32_t *global_inc_ns) {
  return bf_ts_global_ts_inc_value_get(dev, global_inc_ns);
}

p4_pd_status_t p4_pd_ts_global_ts_offset_value_set(p4_pd_ts_dev_t dev,
                                                   uint64_t global_ts) {
  return bf_ts_global_ts_offset_set(dev, global_ts);
}

p4_pd_status_t p4_pd_ts_global_ts_offset_value_get(p4_pd_ts_dev_t dev,
                                                   uint64_t *global_ts) {
  return bf_ts_global_ts_offset_get(dev, global_ts);
}

p4_pd_status_t p4_pd_ts_global_baresync_ts_get(p4_pd_ts_dev_t dev,
                                               uint64_t *global_ts,
                                               uint64_t *baresync_ts) {
  return bf_ts_global_baresync_ts_get(dev, global_ts, baresync_ts);
}

p4_pd_status_t p4_pd_ts_1588_timestamp_delta_tx_set(p4_pd_ts_dev_t dev,
                                                    p4_pd_ts_port_t port,
                                                    uint16_t delta) {
  return bf_port_1588_timestamp_delta_tx_set(dev, port, delta);
}

p4_pd_status_t p4_pd_ts_1588_timestamp_delta_tx_get(p4_pd_ts_dev_t dev,
                                                    p4_pd_ts_port_t port,
                                                    uint16_t *delta) {
  return bf_port_1588_timestamp_delta_tx_get(dev, port, delta);
}

p4_pd_status_t p4_pd_ts_1588_timestamp_delta_rx_set(p4_pd_ts_dev_t dev,
                                                    p4_pd_ts_port_t port,
                                                    uint16_t delta) {
  return bf_port_1588_timestamp_delta_rx_set(dev, port, delta);
}

p4_pd_status_t p4_pd_ts_1588_timestamp_delta_rx_get(p4_pd_ts_dev_t dev,
                                                    p4_pd_ts_port_t port,
                                                    uint16_t *delta) {
  return bf_port_1588_timestamp_delta_rx_get(dev, port, delta);
}

p4_pd_status_t p4_pd_ts_1588_timestamp_tx_get(p4_pd_ts_dev_t dev,
                                              p4_pd_ts_port_t port,
                                              uint64_t *ts,
                                              bool *ts_valid,
                                              int *ts_id) {
  return bf_port_1588_timestamp_tx_get(dev, port, ts, ts_valid, ts_id);
}
