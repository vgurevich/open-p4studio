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


#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <pthread.h>

#include <saiinternal.h>

#include <vector>

namespace smi {
void switch_ports_oper_status_update(
    std::vector<switch_port_oper_status_event_data_t> &status_change_events);
}

void sai_port_state_batch_change(uint32_t count,
                                 switch_port_oper_status_event_data_t *data);

#ifdef __cplusplus
extern "C" {
#endif

// globals
static sai_api_t api_id = SAI_API_UNSPECIFIED;

#include "bf_switchd/bf_switchd.h"
#include "tofino/bf_pal/dev_intf.h"
extern bf_status_t bf_pal_device_warm_init_end(bf_dev_id_t dev_id);

#define INSTALL_DIR ((const char *)"/bfn/install")
#define CONF_FILE ((const char *)"/usr/share/sonic/hwsku/switch-tna-sai.conf")

#define CONF_FILE_OLD \
  ((const char *)"/bfn/install/share/p4/targets/tofino/switch-sai.conf")
#define MODEL_FILE ((const char *)"/bfn/install/share/switch/aug_model.json")

#define SWITCH_NOS_APP_ID 1
#define SWITCH_WARM_INIT_PORT_CLIENT_CONFIG_APPLY_TIMEOUT 20
#define SWITCH_WARM_INIT_PORT_STATUS_UPDATE_INTERVAL 10
/**
 * @def SAI_KEY_BFN_MODEL
 *
 * 0: BFN hardware.
 * 1: BFN model.
 */
#define SAI_KEY_BFN_MODEL "SAI_BFN_MODEL"
/**
 * @def SAI_KEY_WARM_BOOT_WITH_HITLESS
 *
 * presence of this key indicates using hitless
 */
#define SAI_KEY_WARM_BOOT_WITH_HITLESS "SAI_WARM_BOOT_WITH_HITLESS"
/**
 * @def SAI_KEY_INNER_SRC_MAC_FROM_OVERLAY_VRF
 *
 * presence of this key indicates using src_mac from overlay vrf instead
 * of default global src_mac as the inner src_mac
 */
#define SAI_KEY_INNER_SRC_MAC_FROM_OVERLAY_VRF \
  "SAI_INNER_SRC_MAC_FROM_OVERLAY_VRF"

// NOS Loader specific context - holds all the necessary settings for Loader
typedef struct bf_switch_nos_context_s {
  bf_switchd_context_t switchd_ctx;
  unsigned int maxSysPorts;
  pthread_t init_threadId;  // warm init delay thread, when active
} bf_switch_nos_context_t;

static bf_switch_nos_context_t *switch_nos_ctx = NULL;

static void *warmInitDone(void *p) {
  /*
   * Before this sleep is over, switch client should push port configuration
   */
  sleep(SWITCH_WARM_INIT_PORT_CLIENT_CONFIG_APPLY_TIMEOUT);
  /*
   * Switch/SAI/NOS won't get notified about ports oper status from the drivers
   * until bf_pal_device_warm_init_end is called, so updating status from here.
   *
   * Also there is a SONiC bug that delays oper status events processing in
   * orchagent:
   * https://github.com/Azure/sonic-buildimage/issues/7140
   * https://github.com/Azure/sonic-buildimage/issues/8428
   * So currently to have SONiC notified about all port statuses before
   * warm-init-end, we need to notify in batches.
   */
  for (int i = 0; i < 5; ++i) {
    std::vector<switch_port_oper_status_event_data_t> change_events;
    smi::switch_ports_oper_status_update(change_events);
    sai_port_state_batch_change(change_events.size(), change_events.data());
    sleep(SWITCH_WARM_INIT_PORT_STATUS_UPDATE_INTERVAL);
  }
  /*
   * These sleeps are used to delay fast reconfig end. During this time window,
   * all configuration applied by clients will not be pushed down to the data
   * plane.
   * In return, during bf_pal_device_warm_init_end it will be compared to the
   * running data plane configuration and only delta will be pushed.
   */
  bf_pal_device_warm_init_end(0);
  switch_nos_ctx->init_threadId = 0;
  syslog(LOG_ERR,
         "BF_SAI: warm_init_end done: %d\n",
         SWITCH_START_TYPE_FAST_RECFG);
  return p;
}

static int bfn_sdk_init(const char *baseDir,
                        switch_start_type_t warmBoot,
                        const char *warmBootFile,
                        bool model,
                        bool use_hitless,
                        bool overlay_vrf_mac) {
  int ret = 0;
  static char dir[4096];
  static char conf[4096];
  static char model_file[4096];
  struct stat stat_buf;
  pthread_attr_t attr;
  pthread_attr_t *attrp = NULL;
  (void)use_hitless;

  /* Allocate memory to hold switchd configuration and state */
  if ((switch_nos_ctx = static_cast<bf_switch_nos_context_t *>(
           malloc(sizeof(bf_switch_nos_context_t)))) == NULL) {
    printf("BF_SAI: Failed to allocate memory for switch nos context\n");
    return -1;
  }
  memset(switch_nos_ctx, 0, sizeof(bf_switch_nos_context_t));
  // set required directories
  strncpy(dir, baseDir, 4095);
  strncat(dir, INSTALL_DIR, 4095);
  switch_nos_ctx->switchd_ctx.install_dir = dir;
  strncpy(conf, CONF_FILE, 4095);
  if (stat(conf, &stat_buf)) {
    strncpy(conf, baseDir, 4095);
    strncat(conf, CONF_FILE_OLD, 4095);
  }
  strncpy(model_file, baseDir, 4095);
  strncat(model_file, MODEL_FILE, 4095);
  switch_nos_ctx->switchd_ctx.conf_file = conf;
  switch_nos_ctx->switchd_ctx.running_in_background = true;

  if (model) {
    switch_nos_ctx->switchd_ctx.dev_sts_thread = true;
    switch_nos_ctx->switchd_ctx.dev_sts_port = 7777;
    switch_nos_ctx->switchd_ctx.skip_port_add = true;
  }

  bf_switch::switchOptions options;
  if (options.load_from_conf(0, conf, dir)) {
    printf("BF_SAI: Failed to parse conf file\n");
    syslog(LOG_ERR, "BF_SAI: Failed to parse conf file\n");
    return -1;
  }

  syslog(LOG_INFO, "BF_SAI: switchd_lib_init with warmBoot: %d\n", warmBoot);
  if (warmBoot == SWITCH_START_TYPE_WARM_BOOT) {
    switch_nos_ctx->switchd_ctx.init_mode = BF_DEV_WARM_INIT_HITLESS;
  } else if (warmBoot == SWITCH_START_TYPE_FAST_RECFG) {
    // set the fastreconfig flags
    switch_nos_ctx->switchd_ctx.init_mode = BF_DEV_WARM_INIT_FAST_RECFG;
  }
  ret = bf_switchd_lib_init(&(switch_nos_ctx->switchd_ctx));
  if (ret != 0) {
    syslog(LOG_ERR, "BF_SAI: Low-level driver init failed: %d\n", ret);
    return -1;
  }
  if (warmBoot != SWITCH_START_TYPE_COLD_BOOT) {
    syslog(LOG_ERR, "BF_SAI: warm_init_begin done: %d\n", warmBoot);
  }

  bf_switch_sai_mode_set(true);
  // Initialize BF SMI including device add for dev_id 0
  auto kernel_pkt_proc = bf_switchd_is_kernel_pkt_proc(0);
  ret = bf_switch_init(0,
                       conf,
                       dir,
                       warmBoot == SWITCH_START_TYPE_WARM_BOOT,
                       warmBootFile,
                       kernel_pkt_proc);
  if (ret != 0) {
    syslog(LOG_ERR, "BF_SAI: BMAI init failed\n");
    return -1;
  }

  switch_attribute_t dev_attrs[1];
  switch_object_id_t device_handle;
  // Get device handle and max ports
  dev_attrs[0].id = SWITCH_DEVICE_ATTR_DEV_ID;
  dev_attrs[0].value.type = SWITCH_TYPE_UINT16;
  dev_attrs[0].value.u16 = 0;
  ret = bf_switch_object_get_c(
      SWITCH_OBJECT_TYPE_DEVICE, 1, dev_attrs, &device_handle);
  if (ret != 0) {
    syslog(LOG_ERR, "BF_SAI: failed to get device handle\n");
    return -1;
  }

  switch_attribute_t non_def_ppg_attr;
  non_def_ppg_attr.id = SWITCH_DEVICE_ATTR_NUM_NON_DEFAULT_PPGS;
  non_def_ppg_attr.value.type = SWITCH_TYPE_UINT32;
  non_def_ppg_attr.value.u32 = options.non_default_ppgs_get();
  ret = bf_switch_attribute_set_c(device_handle, &non_def_ppg_attr);
  if (ret != 0) {
    syslog(LOG_ERR,
           "BF_SAI: failed to update device num non default ppgs attribute\n");
    return -1;
  }

  /*
  // Global PCP-TC not req as this can be config per port now
  if (warmBoot != SWITCH_START_TYPE_WARM_BOOT) {
    ret = sai_init_global_pcp_tc_qosmap(0);
    if (ret != SWITCH_STATUS_SUCCESS) {
      syslog(LOG_ERR, "BF_SAI: failed to install globalPcpTc qos map.\n");
      return -1;
    }
  }
  */

  // use delay secs to inform drivers that fast recfg is done - not notification
  // from SAI!
  if (warmBoot == SWITCH_START_TYPE_FAST_RECFG) {
    attrp = &attr;
    pthread_attr_init(&attr);
    if ((ret = pthread_create(
             &(switch_nos_ctx->init_threadId), &attr, warmInitDone, NULL)) !=
        0) {
      syslog(LOG_ERR,
             "BF_SAI: fast reboot timer thread creation failed: %d\n",
             ret);
      return -1;
    }
    // https://linux.die.net/man/3/pthread_attr_destroy
    if (attrp != NULL) {
      if ((ret = pthread_attr_destroy(attrp)) != 0) {
        syslog(LOG_ERR, "BF_SAI: pthread_attr_destory failed: %d\n", ret);
      }
    }
  } else if (warmBoot == SWITCH_START_TYPE_WARM_BOOT) {
    bf_pal_device_warm_init_end(0);
    syslog(LOG_ERR, "BF_SAI: warm_init_end done: %d\n", warmBoot);

    switch_attribute_t warm_shut_attr;
    warm_shut_attr.id = SWITCH_DEVICE_ATTR_WARM_SHUT;
    warm_shut_attr.value.type = SWITCH_TYPE_BOOL;
    warm_shut_attr.value.booldata = false;
    ret = bf_switch_attribute_set_c(device_handle, &warm_shut_attr);
    if (ret != 0) {
      syslog(LOG_ERR, "BF_SAI: failed to update warm_shut attr for device\n");
      return -1;
    }
  }

  if (overlay_vrf_mac) {
    switch_attribute_t overlay_vrf_mac_attr;
    overlay_vrf_mac_attr.id = SWITCH_DEVICE_ATTR_INNER_SRC_MAC_FROM_OVERLAY_VRF;
    overlay_vrf_mac_attr.value.type = SWITCH_TYPE_BOOL;
    overlay_vrf_mac_attr.value.booldata = true;
    ret = bf_switch_attribute_set_c(device_handle, &overlay_vrf_mac_attr);
    if (ret != 0) {
      syslog(LOG_ERR, "BF_SAI: failed to update warm_shut attr for device\n");
      return -1;
    }
  }

  bf_switch_init_packet_driver();

  return 0;
}

void smi_sai_init(const uint16_t dev_id,
                  const char *const conf_file,
                  const char *const file_path_prefix) {
  uint32_t num_ports = 0;
  const uint32_t PORT_ATTRS_NUM = 6;
  switch_attribute_t dev_attrs[1];
  switch_attribute_t port_attrs[PORT_ATTRS_NUM];
  switch_object_id_t device_handle;
  switch_attribute_t max_port_attr;
  switch_attribute_t max_recirc_port_attr;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Comment the records during init
  bf_switch_record_comment_mode_set(true);

  bf_switch_sai_mode_set(true);
  // Initialize BF SMI including device add for dev_id 0
  auto kernel_pkt_proc = bf_switchd_is_kernel_pkt_proc(0);
  status = bf_switch_init(
      dev_id, conf_file, file_path_prefix, false, NULL, kernel_pkt_proc);
  if (status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "ERROR: failed to initialize bfn SDK\n");
    return;
  }

  // Get device handle and max ports
  dev_attrs[0].id = SWITCH_DEVICE_ATTR_DEV_ID;
  dev_attrs[0].value.type = SWITCH_TYPE_UINT16;
  dev_attrs[0].value.u16 = dev_id;
  status = bf_switch_object_get_c(
      SWITCH_OBJECT_TYPE_DEVICE, 1, dev_attrs, &device_handle);
  if (status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "ERROR: failed to get device handle\n");
    return;
  }

  status = bf_switch_attribute_get_c(
      device_handle, SWITCH_DEVICE_ATTR_MAX_PORTS, &max_port_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "ERROR: failed to get max port attr from device handle\n");
    return;
  }

  status = bf_switch_attribute_get_c(device_handle,
                                     SWITCH_DEVICE_ATTR_MAX_RECIRC_PORTS,
                                     &max_recirc_port_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR,
           "ERROR: failed to get max recirc port attr from device handle\n");
    return;
  }

  bf_switch::switchOptions options;
  if (options.load_from_conf(0, conf_file, file_path_prefix)) {
    syslog(LOG_ERR, "BF_SAI: Failed to parse conf file\n");
    return;
  }
  // Now add ports using device_handle and max ports
  for (uint32_t port = 0; port < 32;
       // port < max_port_attr.value.u32 - max_recirc_port_attr.value.u32;
       port++) {
    switch_object_id_t port_handle;
    switch_attribute_value_t lane_list[1] = {};
    port_attrs[0].id = SWITCH_PORT_ATTR_DEVICE;
    port_attrs[0].value.type = SWITCH_TYPE_OBJECT_ID;
    port_attrs[0].value.oid = device_handle;

    port_attrs[1].id = SWITCH_PORT_ATTR_LANE_LIST;
    port_attrs[1].value.type = SWITCH_TYPE_LIST;
    port_attrs[1].value.list.list_type = SWITCH_TYPE_UINT32;
    port_attrs[1].value.list.count = 1;
    port_attrs[1].value.list.list = lane_list;
    port_attrs[1].value.list.list[0].u32 = port;

    port_attrs[2].id = SWITCH_PORT_ATTR_LEARNING;
    port_attrs[2].value.type = SWITCH_TYPE_BOOL;
    port_attrs[2].value.booldata = false;

    port_attrs[3].id = SWITCH_PORT_ATTR_TYPE;
    port_attrs[3].value.type = SWITCH_TYPE_ENUM;
    port_attrs[3].value.enumdata.enumdata = SWITCH_PORT_ATTR_TYPE_NORMAL;

    port_attrs[4].id = SWITCH_PORT_ATTR_QOS_CONFIG_PRECEDENCE;
    port_attrs[4].value.type = SWITCH_TYPE_BOOL;
    port_attrs[4].value.booldata = true;

    port_attrs[5].id = SWITCH_PORT_ATTR_SPEED;
    port_attrs[5].value.type = SWITCH_TYPE_UINT32;
    port_attrs[5].value.u32 = 10000;

    status = bf_switch_object_create_c(
        SWITCH_OBJECT_TYPE_PORT, PORT_ATTRS_NUM, port_attrs, &port_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      syslog(LOG_ERR, "ERROR: failed to create port of port_id %d\n", port);
      return;
    }
    num_ports++;
  }

  max_port_attr.id = SWITCH_DEVICE_ATTR_NUM_PORTS;
  max_port_attr.value.type = SWITCH_TYPE_UINT32;
  max_port_attr.value.u32 = num_ports;
  status = bf_switch_attribute_set_c(device_handle, &max_port_attr);
  if (status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "ERROR: failed to update num ports attribute\n");
    return;
  }

  bf_switch_init_packet_driver();

  sai_initialize(false);

  // Uncomment recording after init
  bf_switch_record_comment_mode_set(false);
  return;
}

static unsigned int initialized = 0;
sai_service_method_table_t sai_services = {};

sai_status_t sai_api_initialize(uint64_t flags,
                                const sai_service_method_table_t *services) {
  sai_status_t status = SAI_STATUS_SUCCESS;
  (void)flags;
  UNUSED(api_id);
  switch_start_type_t warmbootMode = SWITCH_START_TYPE_COLD_BOOT;
  const char *fileStr = NULL;
  bool model = false, use_hitless = false, overlay_vrf_mac = false;
  int ret = 0;

  if (!initialized) {
    SAI_LOG_WARN("Initializing device");
    if (services) {
      memcpy(&sai_services, services, sizeof(sai_services));
      if (services->profile_get_value) {
        fileStr = services->profile_get_value(0, SAI_KEY_WARM_BOOT_READ_FILE);
        const char *bootStr = services->profile_get_value(0, SAI_KEY_BOOT_TYPE);
        if (bootStr) {
          if (atoi(bootStr) == 3) {  // fastfast boot
            warmbootMode = SWITCH_START_TYPE_FAST_RECFG;
            syslog(LOG_ERR,
                   "BF_SAI: syncd get profile with fastfast reboot "
                   "(SDE:fastfastBoot): "
                   "%s\n",
                   bootStr);
          } else if (atoi(bootStr) == 2) {  // fast boot
            warmbootMode = SWITCH_START_TYPE_FAST_RECFG;
            syslog(LOG_ERR,
                   "BF_SAI: syncd get profile with fast reboot (SDE:fastBoot): "
                   "%s\n",
                   bootStr);
          } else if (atoi(bootStr) == 1) {  // warm boot
            warmbootMode = SWITCH_START_TYPE_WARM_BOOT;
            syslog(LOG_ERR,
                   "BF_SAI: syncd get profile with warm reboot (SDE:warmBoot): "
                   "%s\n",
                   bootStr);
          }
        } else {
          syslog(LOG_ERR, "BF_SAI: syncd get profile FAILED\n");
        }

        const char *modelStr =
            services->profile_get_value(0, SAI_KEY_BFN_MODEL);
        if (modelStr) {
          model = (atoi(modelStr) == 1);
        }

        const char *hitlessStr =
            services->profile_get_value(0, SAI_KEY_WARM_BOOT_WITH_HITLESS);
        if (hitlessStr) {
          use_hitless = true;
        }

        const char *overlayVrfMacStr = services->profile_get_value(
            0, SAI_KEY_INNER_SRC_MAC_FROM_OVERLAY_VRF);
        if (overlayVrfMacStr) {
          overlay_vrf_mac = true;
        }

      } else {
        syslog(LOG_ERR, "BF_SAI: syncd service NULL\n");
      }
    }

    // Comment the records during init
    bf_switch_record_comment_mode_set(true);

    // add device and switch init
    ret = bfn_sdk_init(
        "/opt", warmbootMode, fileStr, model, use_hitless, overlay_vrf_mac);
    if (ret != 0) {
      syslog(LOG_ERR, "BF_SAI: BFN SDK initialization FAILED\n");
      return SAI_STATUS_FAILURE;
    }
    initialized = 1;

    // SAI init
    status = sai_initialize(warmbootMode == SWITCH_START_TYPE_WARM_BOOT);
    if (status != SAI_STATUS_SUCCESS) {
      syslog(LOG_ERR, "BF_SAI: BFN SAI initialization FAILED\n");
      return status;
    }

    // Uncomment recording after init
    bf_switch_record_comment_mode_set(false);
  }
  return status;
}

sai_status_t sai_api_uninitialize() {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bool warm_shut = false;
  switch_attribute_t dev_attrs[1];
  switch_object_id_t device_handle;
  switch_attribute_t warm_shut_attr;

  dev_attrs[0].id = SWITCH_DEVICE_ATTR_DEV_ID;
  dev_attrs[0].value.type = SWITCH_TYPE_UINT16;
  dev_attrs[0].value.u16 = 0;
  switch_status = bf_switch_object_get_c(
      SWITCH_OBJECT_TYPE_DEVICE, 1, dev_attrs, &device_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    syslog(LOG_ERR, "sai_api_uninitialize: failed to get device handle\n");
    return status_switch_to_sai(switch_status);
  }
  switch_status = bf_switch_attribute_get_c(
      device_handle, SWITCH_DEVICE_ATTR_WARM_SHUT, &warm_shut_attr);
  if (switch_status != 0) {
    syslog(LOG_ERR,
           "sai_api_uninitialize: failed to get warm shut attr from device "
           "handle\n");
    return status_switch_to_sai(switch_status);
  }
  warm_shut = warm_shut_attr.value.booldata;

  if (warm_shut) {
    if (sai_services.profile_get_value) {
      const char *fileStr =
          sai_services.profile_get_value(0, SAI_KEY_WARM_BOOT_WRITE_FILE);
      syslog(LOG_ERR, "BF_SAI: Begin switch shutdown %s\n", fileStr);
      switch_status = bf_switch_clean(0, warm_shut, fileStr);
    }
  } else {
    switch_status = bf_switch_clean(0, false, nullptr);
  }
  return status_switch_to_sai(switch_status);
}

#ifdef __cplusplus
}
#endif
