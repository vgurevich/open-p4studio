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


#ifndef lld_map_h
#define lld_map_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

lld_dev_t *lld_map_dev_id_to_dev_p_allow_unassigned(bf_dev_id_t dev_id);
lld_dev_t *lld_map_dev_id_to_dev_p(bf_dev_id_t dev_id);
lld_dr_view_t *lld_map_dev_id_and_dr_to_view(bf_dev_id_t asic,
                                             bf_dma_dr_id_t dr);
lld_dr_view_t *lld_map_dev_id_and_dr_to_view_allow_unassigned(
    bf_dev_id_t dev_id, bf_dma_dr_id_t dr);

lld_dev_t *lld_map_subdev_id_to_dev_p_allow_unassigned(
    bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);
lld_dev_t *lld_map_subdev_id_to_dev_p(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id);
lld_dr_view_t *lld_map_subdev_id_and_dr_to_view(bf_dev_id_t asic,
                                                bf_subdev_id_t subdev_id,
                                                bf_dma_dr_id_t dr);
lld_dr_view_t *lld_map_subdev_id_and_dr_to_view_allow_unassigned(
    bf_dev_id_t dev_id, bf_subdev_id_t subdev_id, bf_dma_dr_id_t dr);

static inline bf_subdev_id_t lld_map_subdev_id_get(lld_dev_t *dev_p) {
  if (!dev_p) {
    return 0;
  } else {
    return dev_p->subdev_id;
  }
}

#ifdef __cplusplus
}
#endif /* C++ */

#endif
