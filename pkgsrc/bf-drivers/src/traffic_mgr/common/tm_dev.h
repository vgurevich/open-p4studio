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


/*
 *    This file contains all data strcutures and parameters
 *    related to TM device
 */

#ifndef __TM_DEV_H__
#define __TM_DEV_H__

#include <stdint.h>

typedef bf_tm_status_t (*bf_tm_dev_wr_fptr)(bf_dev_id_t);
typedef bf_tm_status_t (*bf_tm_dev_uint8_wr_fptr)(bf_dev_id_t, uint8_t);
typedef bf_tm_status_t (*bf_tm_dev_uint8_rd_fptr)(bf_dev_id_t, uint8_t *);

typedef struct _bf_tm_dev_hw_funcs {
  bf_tm_dev_uint8_wr_fptr timestamp_shift_wr_fptr;
  bf_tm_dev_uint8_rd_fptr timestamp_shift_rd_fptr;
  bf_tm_dev_wr_fptr ddr_train_wr_fptr;
} bf_tm_dev_hw_funcs_tbl;

bf_status_t bf_tm_set_timestamp_shift(bf_dev_id_t dev, uint8_t shift);
bf_status_t bf_tm_get_timestamp_shift(bf_dev_id_t dev, uint8_t *shift);
bf_status_t bf_tm_set_ddr_train(bf_dev_id_t dev);

#endif
