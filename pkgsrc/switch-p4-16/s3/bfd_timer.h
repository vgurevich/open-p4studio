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


#ifndef _BFD_TIMER_H_
#define _BFD_TIMER_H_

#include <stdint.h>

#include "bf_switch/bf_switch_types.h"

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

struct bfd_timer_s;

typedef void (*bfd_timeout_cb)(struct bfd_timer_s *timer, void *data);

typedef struct bfd_timer_s {
  void *timer;    /* OS abstracted context pointer */
  void *userdata; /* per loop userdata */
  bfd_timeout_cb cb_fn;
  void *cb_data;
} bfd_timer_t;

switch_status_t bfd_timer_create(bfd_timer_t *timer,
                                 uint32_t start_msecs,
                                 uint32_t period_msecs,
                                 bfd_timeout_cb cb_fn,
                                 void *cb_data);

switch_status_t bfd_timer_update(bfd_timer_t *t,
                                 uint32_t start_msecs,
                                 uint32_t period_msecs);

switch_status_t bfd_timer_sync(void *userdata);

switch_status_t bfd_timer_start(bfd_timer_t *timer);

switch_status_t bfd_timer_stop(bfd_timer_t *timer);

switch_status_t bfd_timer_del(bfd_timer_t *timer);

switch_status_t bfd_timer_init(void **userdata);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BFD_TIMER_H_ */
