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

#define ETHERTYPE_BF_FABRIC     0x9000
#define FABRIC_HEADER_TYPE_CPU 5

header ethernet_t ethernet;
header fabric_header_t fabric_header;
header fabric_header_cpu_t fabric_header_cpu;
header fabric_payload_header_t fabric_payload_header;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_BF_FABRIC : parse_fabric_header;
        default: ingress;
    }
}

parser parse_fabric_header {
  extract(fabric_header);
  return select(latest.packetType)  {
  	default : parse_fabric_header_cpu;
  }
}

parser parse_fabric_header_cpu {
  extract(fabric_header_cpu);
  return select(latest.ingressPort) {
  	default: parse_fabric_payload_header;
  }
}

parser parse_fabric_payload_header {
  extract(fabric_payload_header);
  return select(latest.etherType) {
	  default: ingress;
  }
}
