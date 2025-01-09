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


#ifndef __BF_SWITCHD_LOG_H__
#define __BF_SWITCHD_LOG_H__
/*-------------------- bf_switchd_log.h -----------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <bf_types/bf_types.h>

typedef enum {
  BF_SWITCHD_LOG_WRITE = 0,
  BF_SWITCHD_LOG_READ_BEFORE,
  BF_SWITCHD_LOG_READ_AFTER,
} bf_switchd_log_acc_type;

// for logging register accesses
void bf_switchd_log_init(bool init_mtx);
void bf_switchd_log_access(bf_switchd_log_acc_type acc_typ,
                           bf_dev_id_t dev_id,
                           bf_dev_family_t family,
                           uint32_t reg,
                           uint32_t data);
void bf_switchd_log_dump_access_log(void);
void bf_switchd_log_dump_access_log_last_n(int num_logs);
void bf_switchd_log_dump_access_log_w_filter(char *pattern);

#endif /* __BF_SWITCHD_LOG_H__ */
