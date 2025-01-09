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

#ifndef _BF_IOCTL_H_
#define _BF_IOCTL_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>

#ifndef phys_addr_t
typedef uint64_t phys_addr_t;
#endif

#endif /* __KERNEL__ */

#define BF_IOC_MAGIC 'b'
#define BF_TBUS_MSIX_INDICES_MAX   3
#define BF_TBUS_MSIX_INDICES_MIN   1  

typedef struct bf_dma_bus_map_s
{
  phys_addr_t phy_addr;
  void *dma_addr;
  size_t size;
} bf_dma_bus_map_t;

typedef struct bf_tbus_msix_indices_s
{
  int cnt;
  int indices[BF_TBUS_MSIX_INDICES_MAX];
} bf_tbus_msix_indices_t;

enum bf_intr_mode {
  BF_INTR_MODE_NONE = 0,
  BF_INTR_MODE_LEGACY,
  BF_INTR_MODE_MSI,
  BF_INTR_MODE_MSIX,
};

typedef struct bf_intr_mode_s {
  enum bf_intr_mode intr_mode;
} bf_intr_mode_t;

#define BF_IOCMAPDMAADDR    _IOWR(BF_IOC_MAGIC, 0, bf_dma_bus_map_t)
#define BF_IOCUNMAPDMAADDR  _IOW(BF_IOC_MAGIC, 1, bf_dma_bus_map_t)
#define BF_TBUS_MSIX_INDEX  _IOW(BF_IOC_MAGIC, 2, bf_tbus_msix_indices_t)
#define BF_GET_INTR_MODE    _IOR(BF_IOC_MAGIC, 3, bf_intr_mode_t)

#endif /* _BF_IOCTL_H_ */
