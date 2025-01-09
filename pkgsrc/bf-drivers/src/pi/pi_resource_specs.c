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


#include <PI/p4info.h>
#include <PI/pi.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>

void convert_from_pipe_meter_spec(const pi_p4info_t *p4info,
                                  pi_p4_id_t meter_id,
                                  const pipe_meter_spec_t *pipe_meter_spec,
                                  pi_meter_spec_t *meter_spec) {
  const pipe_meter_rate_t *cir = &pipe_meter_spec->cir;
  const pipe_meter_rate_t *pir = &pipe_meter_spec->pir;
  pi_p4info_meter_unit_t meter_unit =
      pi_p4info_meter_get_unit(p4info, meter_id);
  switch (meter_unit) {
    case PI_P4INFO_METER_UNIT_PACKETS:
      // not set properly by pipe_mgr at the moment
      // bf_sys_assert(cir->type == METER_RATE_TYPE_PPS);
      meter_spec->cir = cir->value.pps;
      meter_spec->cburst = pipe_meter_spec->cburst;
      // bf_sys_assert(pir->type == METER_RATE_TYPE_PPS);
      meter_spec->pir = pir->value.pps;
      meter_spec->pburst = pipe_meter_spec->pburst;
      break;
    case PI_P4INFO_METER_UNIT_BYTES:
      // bf_sys_assert(cir->type == METER_RATE_TYPE_KBPS);
      // kbits per second -> bytes per second
      meter_spec->cir = cir->value.kbps * 1000 / 8;
      // kbits -> bytes
      meter_spec->cburst = pipe_meter_spec->cburst * 1000 / 8;
      // bf_sys_assert(pir->type == METER_RATE_TYPE_KBPS);
      // kbits per second -> bytes per second
      meter_spec->pir = pir->value.kbps * 1000 / 8;
      // kbits -> bytes
      meter_spec->pburst = pipe_meter_spec->pburst * 1000 / 8;
      break;
  }
  switch (pipe_meter_spec->meter_type) {
    case METER_TYPE_COLOR_AWARE:
      meter_spec->meter_type = PI_METER_TYPE_COLOR_AWARE;
      break;
    case METER_TYPE_COLOR_UNAWARE:
      meter_spec->meter_type = PI_METER_TYPE_COLOR_UNAWARE;
      break;
  }
}

void convert_to_pipe_meter_spec(const pi_p4info_t *p4info,
                                pi_p4_id_t meter_id,
                                const pi_meter_spec_t *meter_spec,
                                pipe_meter_spec_t *pipe_meter_spec) {
  pipe_meter_rate_t *cir = &pipe_meter_spec->cir;
  pipe_meter_rate_t *pir = &pipe_meter_spec->pir;
  pi_p4info_meter_unit_t meter_unit =
      pi_p4info_meter_get_unit(p4info, meter_id);
  bf_sys_assert((int)meter_unit == (int)meter_spec->meter_unit);
  switch (meter_unit) {
    case PI_P4INFO_METER_UNIT_PACKETS:
      cir->type = METER_RATE_TYPE_PPS;
      cir->value.pps = meter_spec->cir;
      pipe_meter_spec->cburst = meter_spec->cburst;
      pir->type = METER_RATE_TYPE_PPS;
      pir->value.pps = meter_spec->pir;
      pipe_meter_spec->pburst = meter_spec->pburst;
      break;
    case PI_P4INFO_METER_UNIT_BYTES:
      cir->type = METER_RATE_TYPE_KBPS;
      // bytes per second -> kbits per second
      cir->value.kbps = meter_spec->cir * 8 / 1000;
      // bytes -> kbits
      pipe_meter_spec->cburst = meter_spec->cburst * 8 / 1000;
      pir->type = METER_RATE_TYPE_KBPS;
      // bytes per second -> kbits per second
      pir->value.kbps = meter_spec->pir * 8 / 1000;
      // bytes -> kbits
      pipe_meter_spec->pburst = meter_spec->pburst * 8 / 1000;
      break;
  }
  pi_p4info_meter_type_t meter_type =
      pi_p4info_meter_get_type(p4info, meter_id);
  // sanity check, guaranteed by common code
  if (meter_type == PI_P4INFO_METER_TYPE_COLOR_UNAWARE)
    bf_sys_assert(meter_spec->meter_type == PI_METER_TYPE_COLOR_UNAWARE);
  switch (meter_spec->meter_type) {
    case PI_METER_TYPE_COLOR_AWARE:
      pipe_meter_spec->meter_type = METER_TYPE_COLOR_AWARE;
      break;
    case PI_METER_TYPE_COLOR_UNAWARE:
    case PI_METER_TYPE_DEFAULT:  // temporary, will be removed from PI
      pipe_meter_spec->meter_type = METER_TYPE_COLOR_UNAWARE;
      break;
  }
}

void convert_to_counter_data(const pi_p4info_t *p4info,
                             pi_p4_id_t counter_id,
                             const pipe_stat_data_t *stat_data,
                             pi_counter_data_t *counter_data) {
  pi_p4info_counter_unit_t counter_unit =
      pi_p4info_counter_get_unit(p4info, counter_id);
  switch (counter_unit) {
    case PI_P4INFO_COUNTER_UNIT_BYTES:
      counter_data->valid = PI_COUNTER_UNIT_BYTES;
      break;
    case PI_P4INFO_COUNTER_UNIT_PACKETS:
      counter_data->valid = PI_COUNTER_UNIT_PACKETS;
      break;
    case PI_P4INFO_COUNTER_UNIT_BOTH:
      counter_data->valid = PI_COUNTER_UNIT_BYTES | PI_COUNTER_UNIT_PACKETS;
      break;
  }
  if (counter_data->valid & PI_COUNTER_UNIT_BYTES)
    counter_data->bytes = stat_data->bytes;
  if (counter_data->valid & PI_COUNTER_UNIT_PACKETS)
    counter_data->packets = stat_data->packets;
}
