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


#include <iostream>

#include "switch_model_api_rpc.h"
#include "switch_model_api.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "arpa/inet.h"


#define SWITCH_API_MODEL_RPC_SERVER_PORT (9094)

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::switch_model_api;

unsigned int switch_string_to_mac(const std::string s, char *m) {
  unsigned int i, j = 0;
  memset(m, 0, 6);
  for (i = 0; i < s.size(); i++) {
    char let = s.c_str()[i];
    if (let >= '0' && let <= '9') {
      m[j / 2] = (m[j / 2] << 4) + (let - '0');
      j++;
    } else if (let >= 'a' && let <= 'f') {
      m[j / 2] = (m[j / 2] << 4) + (let - 'a' + 10);
      j++;
    } else if (let >= 'A' && let <= 'F') {
      m[j / 2] = (m[j / 2] << 4) + (let - 'A' + 10);
      j++;
    }
  }
  return (j == 12);
}
//:: import template_generator as template

class switch_model_api_rpcHandler : virtual public ::switch_model_api_rpcIf {
 public:
  switch_model_api_rpcHandler() {}
//:: attribute_iterator = template.attribute_generator(_context, False)
//:: object_dict = template.get_object_dict(_context)
//:: oldobjectname = ""
//:: for type, key, default, classType, fieldname, _,objectname in attribute_iterator:
//::   if oldobjectname != objectname:
//::      if oldobjectname != "":
   switch_status_t status = ::switch_api_${oldobjectname}_create(object_handle_pointer${suffix});
   data.status = status;
   data.object_id = object_handle_pointer->data;
   return;

};

//:: #endif   
thrift_switch_status_t switch_api_${objectname}_delete(const thrift_switch_object_id_t object_handle) {
   switch_object_id_t convertedHandle = {.data = static_cast<uint64_t> (object_handle)};
   return ::switch_api_${objectname}_delete(convertedHandle);
}

//:: count, _ = object_dict[objectname]
//:: prefix = ""
//:: suffix = ""
//:: if count != 0:
//::    prefix = ", const thrift_switch_api_" + objectname + "_t& structobject"
//::    suffix = ", &struct_object_copy"
//:: #endif

void switch_api_${objectname}_create(thrift_switch_create_return_data_t& data${prefix}) {
   switch_object_id_t object_id;  
   switch_api_${objectname}_t struct_object_copy;
   switch_object_id_t *object_handle_pointer = &object_id;
//:: #endif   
//:: oldobjectname = objectname    
//:: value = template.convert_struct_type(type, key)   
   ${value} 
//:: #endfor
   switch_status_t status = ::switch_api_${oldobjectname}_create(object_handle_pointer${suffix});
   data.status = status;
   data.object_id = object_handle_pointer->data;
   return;
}
//:: object_iterator = template.object_generator(_context)
//:: for objectname, counter in object_iterator:
//::   if counter == 0:
void switch_api_${objectname}_create(thrift_switch_create_return_data_t& data) {
   switch_object_id_t object_id;  
   switch_object_id_t *object_handle_pointer = &object_id;
   switch_status_t status = ::switch_api_${objectname}_create(object_handle_pointer);
   data.status = status;
   data.object_id = object_handle_pointer->data;
   return;
}

thrift_switch_status_t switch_api_${objectname}_delete(const thrift_switch_object_id_t object_handle) {
   switch_object_id_t convertedHandle = {.data = static_cast<uint64_t> (object_handle)};
   return ::switch_api_${objectname}_delete(convertedHandle);
}
//::   #endif
//::  #endfor


//:: attribute_iterator = template.attribute_generator(_context, False)
//:: for type, key, default, classType, _, _, objectname in attribute_iterator:
//:: extension = ""
//:: if classType == "SWITCH_TYPE_ENUM":
//::    extension = "::type"
//:: #endif
//:: if type == "switch_mac_addr_t":
//::    extension = "&"
//:: #endif

thrift_switch_status_t switch_api_${objectname}_set_${key}(
         const thrift_switch_object_id_t  object_handle, const thrift_${type}${extension} ${key}) {
    switch_object_id_t convertedHandle = {.data = static_cast<uint64_t> (object_handle)};
    ${type} convertedKey;
//:: convertedKey = template.convert_struct_type_setter(type, key)    
    ${convertedKey}
    return ::switch_api_${objectname}_set_${key}(convertedHandle, convertedKey);   
}

void switch_api_${objectname}_get_${key}(thrift_switch_return_${type}& returnValue, const thrift_switch_object_id_t  object_handle) {
   ${type} value;
   switch_object_id_t convertedHandle = {.data = static_cast<uint64_t> (object_handle)};

   switch_status_t status = ::switch_api_${objectname}_get_${key}(convertedHandle, &value);
//:: getString = template.convert_struct_type_getter(type, classType, key)   
   ${getString}
   returnValue.status = status;
   return ; 
}
//:: #endfor
};
static void *api_model_rpc_server_thread(void *args) {
  int port = SWITCH_API_MODEL_RPC_SERVER_PORT;
  shared_ptr<switch_model_api_rpcHandler> handler(new switch_model_api_rpcHandler());
  shared_ptr<TProcessor> processor(new switch_model_api_rpcProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(
      new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory,
                       protocolFactory);
  /* set thread name to "model api_thrift" */
  pthread_setname_np(pthread_self(), "model_api_thrift");
  server.serve();
  return NULL;
}

static pthread_t api_model_rpc_thread;

int thrift_start_switch_model_api_rpc_server(void) {
  std::cerr << "Starting API RPC server on port " << SWITCH_API_MODEL_RPC_SERVER_PORT
            << std::endl;

  return pthread_create(&api_model_rpc_thread, NULL, api_model_rpc_server_thread, NULL);
}
