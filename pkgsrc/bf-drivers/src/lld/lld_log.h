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


#ifndef LLD_LOG_INCLUDED
#define LLD_LOG_INCLUDED

#include <target-sys/bf_sal/bf_sys_intf.h>

#define lld_log_critical(...) \
  bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_CRIT, __VA_ARGS__)
#define lld_log_error(...) \
  bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_ERR, __VA_ARGS__)
#define lld_log_warn(...) \
  bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_WARN, __VA_ARGS__)
#define lld_log_trace(...) \
  bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_INFO, __VA_ARGS__)
#define lld_log_debug(...) \
  bf_sys_log_and_trace(BF_MOD_LLD, BF_LOG_DBG, __VA_ARGS__)

#define lld_log lld_log_debug

typedef enum {
  LOG_TYP_GLBL = 0,
  LOG_TYP_CHIP,
  LOG_TYP_MAC,
  LOG_TYP_PORT,
  LOG_TYP_DMA,
} lld_log_type_e;

int lld_log_worthy(lld_log_type_e typ, int p1, int p2, int p3);
int lld_log_set(lld_log_type_e typ, int p1, int p2, int p3);
void lld_log_settings(void);
void lld_log_internal(const char *fmt, ...);

void lld_log_dma(int dir,
                 bf_dev_id_t dev_id,
                 bf_subdev_id_t subdev_id,
                 uint32_t dr,
                 uint64_t *data,
                 int n_wds,
                 uint64_t head,
                 uint64_t tail);
void lld_dma_log_init(void);
char *get_full_reg_path_name(bf_dev_id_t dev_id, uint32_t offset);
char *lld_reg_parse_get_full_reg_path_name(bf_dev_family_t dev_family,
                                           uint32_t offset);

#endif  // LLD_LOG_INCUDED
