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

#ifndef __UAPI_PSAMPLE_H
#define __UAPI_PSAMPLE_H

enum {
  /* sampled packet metadata */
  PSAMPLE_ATTR_IIFINDEX,
  PSAMPLE_ATTR_OIFINDEX,
  PSAMPLE_ATTR_ORIGSIZE,
  PSAMPLE_ATTR_SAMPLE_GROUP,
  PSAMPLE_ATTR_GROUP_SEQ,
  PSAMPLE_ATTR_SAMPLE_RATE,
  PSAMPLE_ATTR_DATA,

  /* commands attributes */
  PSAMPLE_ATTR_GROUP_REFCOUNT,

  __PSAMPLE_ATTR_MAX
};

enum psample_command {
  PSAMPLE_CMD_SAMPLE,
  PSAMPLE_CMD_GET_GROUP,
  PSAMPLE_CMD_NEW_GROUP,
  PSAMPLE_CMD_DEL_GROUP,
};

/* Can be overridden at runtime by module option */
#define PSAMPLE_ATTR_MAX (__PSAMPLE_ATTR_MAX - 1)

#define PSAMPLE_NL_MCGRP_CONFIG_NAME "config"
#define PSAMPLE_NL_MCGRP_SAMPLE_NAME "packets"
#define PSAMPLE_GENL_NAME "psample"
#define PSAMPLE_GENL_VERSION 1

#endif
