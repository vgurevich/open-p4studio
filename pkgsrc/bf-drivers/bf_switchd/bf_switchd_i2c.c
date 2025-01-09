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

#include "bf_switchd.h"
#include "bf_switchd_i2c.h"
/*
 * Register the functions defined in platforms i2c library to read and write
 * tofino
 * registers
 */
void bf_switchd_i2c_fn_reg(bf_pltfm_reg_dir_i2c_rd rd_fn,
                           bf_pltfm_reg_dir_i2c_wr wr_fn) {
  reg_dir_i2c_rd_func = rd_fn;
  reg_dir_i2c_wr_func = wr_fn;
}
