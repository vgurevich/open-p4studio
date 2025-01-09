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


#ifndef LLD_EFUSE_TOF3_H_INCLUDED
#define LLD_EFUSE_TOF3_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

int lld_efuse_tof3_load(bf_dev_id_t dev_id,
                        bf_subdev_id_t subdev_id,
                        bf_dev_init_mode_t warm_init_mode);
void lld_efuse_tof3_wafer_str_get(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id,
                                  int s_len,
                                  char *s);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // LLD_EFUSE_TOF3_H_INCLUDED
