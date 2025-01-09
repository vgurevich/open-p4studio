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


#include "s3/switch_packet.h"
#include "switch_tna/utils.h"
#include "../../s3/cli/bf_switch_cli_api.h"

namespace smi {
using namespace smi::bf_rt;  // NOLINT(build/namespaces)
using ::smi::logging::switch_log;

switch_status_t switch_init(const char *model_json,
                            const char *cpu_port,
                            bool use_pcie,
                            bool override_log_level,
                            bool warm_init,
                            const char *const warm_init_file) {
  ENTER();
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  /* Initialize switch_store */
  status |= switch_store::object_info_init(
      model_json, warm_init, warm_init_file, override_log_level);

  /* Initialize bf_rt backend */
  status |= switch_bf_rt_init();

  /* Fetch smi_ids from bf_rt */
  smi_id::init_bf_rt_ids();

  /* @fixme: initialize bf_switch_cli - will be move here after cli moved to
   * infra. */
  //  status |= switchcli::bf_switch_cli_init();

  /* Initialize feature list */
  feature::init_features();

  /* Initialize all p4 modules */
  status |= device_initialize();
  status |= tables_init();
  status |= validation_init();
  status |= validation_fp_init();
  status |= port_init();
  status |= packet_path_init();
  status |= l2_init();
  status |= l3_init();
  status |= rmac_init();
  status |= nexthop_init();
  status |= rewrite_init();
  status |= acl_init();
  status |= afp_init();
  status |= fp_init();
  status |= multicast_init();
  status |= mirror_init();
  status |= qos_init();
  status |= meter_init();
  status |= wred_init();
  status |= tunnel_init();
  status |= dtel_init();
  status |= pfc_wd_init();
  status |= hash_initialize();
  status |= etrap_init();
  status |= nat_init();
  status |= mpls_init();

  /* Initialize other non-p4 modules */
  status |= hostif_init();
  status |= hostif_trap_init();
  status |= triggers_init();
  status |= pal_init();
  status |= qos_pdfixed_init();
  status |= sflow_init();

  /* Initialize switch_packet */
  status |=
      switch_packet_init(cpu_port, use_pcie, SWITCH_CONTEXT.is_kpkt_enabled());

  status |= bfd_init();

  // start replay
  status |= switch_store::object_replay(warm_init);

  // restore counter objects from stats cache
  status |= switch_store::object_restore_stats_cache(warm_init);

  /* Initialize bf_switch_cli */
  status |= switchcli::bf_switch_cli_init();

  return status;
  EXIT();
}

switch_status_t switch_clean(bool warm_shut, const char *const warm_shut_file) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  /*
   * Among other things, bf_switch_clean closes hostif fds.
   * Hostifs that are members of port channels could be quite slow to close
   * in linux. Depending on configuration, by closing all netdevs syncd can
   * reach SONiC timeout for graceful syncd shutdown and get killed with
   * SIGTERM.
   * So bf_switch_clean implementation needs to make critical tasks first
   * before time consuming.
   * This is important in cases like storing switch state on warm-reboot.
   * We cannot count that bf_switch_clean call will be completed
   * on SONiC shutdown
   */
  if (warm_shut) {
    status |= switch_store::object_info_dump(warm_shut_file);
  }

  // clear non-p4 modules
  status |= sflow_clean();
  status |= qos_pdfixed_clean();
  status |= pal_clean();
  status |= triggers_clean();
  status |= bfd_clean();
  status |= hostif_trap_clean();
  status |= hostif_clean();

  // clear packet driver
  status |= switch_packet_clean();

  status |= etrap_clean();
  status |= pfc_wd_clean();
  status |= dtel_clean();
  status |= tunnel_clean();
  status |= wred_clean();
  status |= meter_clean();
  status |= qos_clean();
  status |= mirror_clean();
  status |= multicast_clean();
  status |= fp_clean();
  status |= afp_clean();
  status |= acl_clean();
  status |= rewrite_clean();
  status |= nexthop_clean();
  status |= rmac_clean();
  status |= l3_clean();
  status |= l2_clean();
  status |= packet_path_clean();
  status |= port_clean();
  status |= validation_fp_clean();
  status |= validation_clean();
  status |= tables_clean();
  status |= device_clean();
  status |= hash_clean();

  status |= switch_bf_rt_clean();

  status |= switch_store::object_info_clean();

  feature::clear_features();

  return SWITCH_STATUS_SUCCESS;
}

}  // namespace smi
