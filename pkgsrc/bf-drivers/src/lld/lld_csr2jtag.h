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


#ifndef LLD_CSR2JTAG_H_INCLUDED
#define LLD_CSR2JTAG_H_INCLUDED

bf_status_t lld_csr2jtag_run_pre_efuse(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id);
bf_status_t lld_csr2jtag_run_post_efuse(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id);
bf_status_t lld_csr2jtag_run_one_file(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      const char *fname);
bf_status_t lld_csr2jtag_run_suite(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   const char *fname);

#endif  // LLD_CSR2JTAG_H_INCLUDED
