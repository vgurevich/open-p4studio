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


/** @file tdi_tofino_warm_init.h
 *
 *  @brief C frontend for tofino specific warm-init
 */
#ifndef _TDI_TOFINO_WARM_INIT_H_
#define _TDI_TOFINO_WARM_INIT_H_

// tdi includes
#include <tdi/common/tdi_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pipeline_ {
  char *name;
  char *context_path;
  char *binary_path;
  uint32_t *scope_vec;
  int num_pipes;
} tdi_pipeline_t;

tdi_status_t device_warm_init_begin(tdi_dev_id_t device_id,
                                    char *init_mode,
                                    char *serdes_mode,
                                    bool upgrade_agents,
                                    tdi_dev_config_hdl *prog_config);

tdi_status_t device_warm_init_end(tdi_dev_id_t dev_id);

tdi_status_t dev_config_allocate(int num_programs,
                                 tdi_dev_config_hdl **dev_config_hdl);

tdi_status_t dev_config_deallocate(tdi_dev_config_hdl *dev_config_hdl);

tdi_status_t set_program_name(tdi_dev_config_hdl *dev_config_hdl,
                              int index,
                              char *program_name);
tdi_status_t set_base_path(tdi_dev_config_hdl *dev_config_hdl,
                           int index,
                           char *base_path);
tdi_status_t set_pipeline(tdi_dev_config_hdl *dev_config_hdl,
                          int prog_index,
                          tdi_pipeline_t pipeline);
tdi_status_t set_tdi_info_path(tdi_dev_config_hdl *dev_config_hdl,
                               int prog_index,
                               char *tdi_info_path);

#ifdef __cplusplus
}
#endif

#endif  // _TDI_TOFINO_WARM_INIT_H_
