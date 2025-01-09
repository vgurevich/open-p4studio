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

//:: # Generate the handler routines for the interfaces

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include <iostream>
#include "bfn_pd_rpc_server.h"
#include <thrift/processor/TMultiplexedProcessor.h>

using namespace ::apache::thrift;

using ::std::shared_ptr;

#include "p4_pd_rpc_server.ipp"

// processor needs to be of type TMultiplexedProcessor,
// I am keeping a void * pointer for 
// now, in case this function is called from C code
int add_to_rpc_server(void *processor) {
  std::cerr << "Adding Thrift service for P4 program ${p4_prefix} to server\n";

  shared_ptr<${p4_prefix}Handler> ${p4_prefix}_handler(new ${p4_prefix}Handler());

  TMultiplexedProcessor *processor_ = (TMultiplexedProcessor *) processor;
  processor_->registerProcessor(
    "${p4_prefix}",
    shared_ptr<TProcessor>(new ${p4_prefix}Processor(${p4_prefix}_handler))
  );

  return 0;
}
int rmv_from_rpc_server(void *processor) {
  std::cerr << "Removing Thrift service for P4 program ${p4_prefix} from server\n";

  TMultiplexedProcessor *processor_ = (TMultiplexedProcessor *) processor;
  processor_->registerProcessor(
    "${p4_prefix}",
    shared_ptr<TProcessor>()
  );

  return 0;
}
