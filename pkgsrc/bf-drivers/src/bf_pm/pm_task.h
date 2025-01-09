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


#ifndef __PM_TASK_H__
#define __PM_TASK_H__
/*-------------------- pm_task.h -----------------------------*/
#include <stdint.h>

#define TASK_DONE 0xffffffff
typedef uint32_t (*tasklet_fn)(void *context);
typedef enum { LO_PRI = 0, HI_PRI, MAX_PRI } tasklet_pri_t;

typedef enum { STATE_RUNNING = 0, STATE_REMOVE, STATE_DEFAULT } tasklet_state_t;

void pm_fsm_queues_init();
void pm_tasklet_scheduler(void);
void pm_tasklet_new(tasklet_fn fn, void *context, tasklet_pri_t priority);
void pm_tasklet_rmv(void *context);
bool pm_is_current_tasklet_valid(void *context);

// debug facility
void pm_tasklet_free_run_set(bool st);
bool pm_tasklet_free_run_get(void);
void pm_tasklet_single_step_set(void);

#endif /* __PM_TASK_H__ */
