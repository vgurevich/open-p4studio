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

#ifndef _DIAG_P4_TABLE_SIZES_
#define _DIAG_P4_TABLE_SIZES_

#ifdef DIAG_POWER_ENABLE
// DIAG_POWER

#define MAC_TABLE_SIZE 46000
#define BD_FLOOD_TABLE_SIZE 4096
#if defined(TOFINO3)
#define PORTMAP_TABLE_SIZE 1048
#else
#define PORTMAP_TABLE_SIZE 512
#endif
#define DST_OVERRIDE_TABLE_SIZE 2048
#define EXM_TBL_SIZE 26000
#define EXM_TBL_4_SIZE 12000
#define EXM_EXTRA_TBL_SIZE 5000
#define EXM_EXTRA_TBL_SIZE_MIN 1000

#if defined(TOFINO1)
/* Reduce table sizes for tofino1 to reduce power */
#define DIAG_POWER_TBL_WAYS @ways(3)
#define TCAM_TABLE_SIZE 3000
#else
#define DIAG_POWER_TBL_WAYS @ways(3)
#define TCAM_TABLE_SIZE 4000
#endif

#define TCAM_TABLE_SIZE_MIN 2000
#define PORT_VLAN_TABLE_SIZE 10000
#define VLAN_ENCAP_TABLE_SIZE 10000
#define EXM_TBL_6_SIZE 12000
#define EXM_TBL_8_SIZE 12000
#define EXM_TBL_10_SIZE 12000
#define EXM_TBL_SIZE_MAX 80000

#else
// !DIAG_POWER_ENABLE

#if defined(DIAG_PARDE_STRESS_POWER)
#define TCAM_TABLE_SIZE 12288
#else
#define TCAM_TABLE_SIZE 1000
#endif
#define TCAM_TABLE_SIZE_MIN 1000
#if defined(TOFINO3)
#define PORTMAP_TABLE_SIZE 1048
#else
#define PORTMAP_TABLE_SIZE 512
#endif
#define DST_OVERRIDE_TABLE_SIZE 2048
#define MAC_TABLE_SIZE 2048
#define BD_FLOOD_TABLE_SIZE 1024
#define PORT_VLAN_TABLE_SIZE 4096
#define VLAN_ENCAP_TABLE_SIZE 1024
#define EXM_TBL_SIZE 1000
#define EXM_TBL_4_SIZE 1000
#define EXM_TBL_6_SIZE 1000
#define EXM_TBL_8_SIZE 1000
#define EXM_TBL_10_SIZE 1000
#define EXM_TBL_SIZE_MAX 1000
#define EXM_EXTRA_TBL_SIZE 1000
#define EXM_EXTRA_TBL_SIZE_MIN 1000
#define DIAG_POWER_TBL_WAYS @ways(4)

#endif  //  DIAG_POWER_ENABLE

#define MAU_BUS_STRESS_EXM_TBL_SIZE 1000
#define MAU_BUS_STRESS_TCAM_TBL_SIZE 1000

#define LEARN_NOTIFY_TABLE_SIZE 2
#define VLAN_DECAP_TABLE_SIZE 2

#endif /* _DIAG_P4_TABLE_SIZES_ */
