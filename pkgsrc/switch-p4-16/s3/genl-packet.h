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

#ifndef __UAPI_GENL_PACKET_H
#define __UAPI_GENL_PACKET_H

enum {
  /* packet metadata */
  GENL_PACKET_ATTR_IIFINDEX,
  GENL_PACKET_ATTR_OIFINDEX,
  GENL_PACKET_ATTR_CONTEXT,
  GENL_PACKET_ATTR_DATA,

  __GENL_PACKET_ATTR_MAX
};

enum genl_packet_command {
  GENL_PACKET_CMD_PACKET,
};

/* Can be overridden at runtime by module option */
#define GENL_PACKET_ATTR_MAX (__GENL_PACKET_ATTR_MAX - 1)

#define GENL_PACKET_MCGRP_NAME "packets"
#define GENL_PACKET_NAME "genl_packet"
#define GENL_PACKET_VERSION 1

#endif
