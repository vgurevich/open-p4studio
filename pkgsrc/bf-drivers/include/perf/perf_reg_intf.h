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


/*!
 * @file perf_reg_intf.h
 * @date
 *
 * Performance registers handling definitions.
 */

#ifndef _PERF_REG_INTF_H
#define _PERF_REG_INTF_H

extern struct test_description reg_indir_test;
extern struct test_description reg_dir_test;

enum registers_enum_res { RES_REG_BUS };
enum registers_int_res { RES_REG_IT };
enum registers_double_res {
  RES_REG_WRITE_NS,
  RES_REG_WRITE_OP,
  RES_REG_READ_NS,
  RES_REG_READ_OP
};

struct register_result {
  bool status;
  double write_ns;
  double write_op;
  double read_ns;
  double read_op;
};

/**
 * @brief Run performance test that will indirect write/read to/from regitser,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @param it number of iterations
 * @return register_result
 */
struct test_results reg_indir(bf_dev_id_t dev_id,
                              perf_bus_t_enum bus_type,
                              int it);

/**
 * @brief Run performance test that will direct write/read to/from regitser,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @param it number of iterations
 * @return register_result
 */
struct test_results reg_dir(bf_dev_id_t dev_id,
                            perf_bus_t_enum bus_type,
                            int it);

#endif
