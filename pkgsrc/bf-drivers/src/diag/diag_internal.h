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


#ifndef DIAG_INTERNAL_H_INCLUDED
#define DIAG_INTERNAL_H_INCLUDED

#define DEVDIAG_NUM_DEVICES BF_MAX_DEV_COUNT
#define DEVDIAG_NUM_SUBDEVICES BF_MAX_SUBDEV_COUNT
#define diag_u64_to_void_ptr(u64) ((void *)((uintptr_t)u64))
#define diag_ptr_to_u64(ptr) ((uintptr_t)ptr)

#endif  // DIAG_INTERNAL_H_INCLUDED
