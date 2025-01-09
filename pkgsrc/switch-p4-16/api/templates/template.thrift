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



namespace py switch_model_api
namespace cpp switch_model_api

typedef i64 thrift_uint64_t
typedef i32 thrift_switch_status_t
typedef i32 thrift_int32_t
typedef i32 thrift_uint32_t
typedef i64 thrift_switch_object_id_t
typedef i16 thrift_int16_t
typedef i16 thrift_uint16_t
typedef i16 thrift_switch_handle_t
typedef string thrift_switch_mac_t
typedef bool thrift_bool




//:: import template_generator as template
//:: attribute_iterator = template.attribute_generator(_context, True)
//:: for enums, key, objectname in attribute_iterator:
enum thrift_switch_${objectname}_${key}_t {
//::    for enum in enums:
  SWITCH_${enum},
//::    #endfor
}

//:: #endfor
//:: attribute_iterator = template.attribute_generator(_context, False)
//:: counter = -1
//:: oldobjectname = ""
//:: for type, key, _, _, _, _, objectname in attribute_iterator:
//::   if oldobjectname != objectname:
//::      if counter !=-1:
}

//::      #endif
//::      counter = 1
//::      oldobjectname = objectname

struct thrift_switch_api_${objectname}_t {
//::    #endif
     ${counter}: thrift_${type} ${key},
//::  counter +=1
//:: #endfor
} 


struct thrift_switch_create_return_data_t {
	1: thrift_switch_status_t status
	2: thrift_switch_object_id_t object_id
}


//:: attribute_iterator = template.attribute_generator(_context, True)
//:: for _, key, objectname in attribute_iterator:

struct thrift_switch_return_${objectname}_${key}_t {
 1: thrift_switch_status_t status
 2: thrift_switch_${objectname}_${key}_t value 
}
//:: #endfor

//:: attribute_iterator = template.attribute_generator(_context, False)
//:: dic = dict()
//:: for type, _,_,_, _, _, _ in attribute_iterator:
//::  if type not in dic:
struct thrift_switch_return_${type} {
 1: thrift_switch_status_t status
 2: thrift_${type} value 
}
//::    dic[type] = 1
//::  #endif
//:: #endfor

service switch_model_api_rpc {



//:: object_iterator = template.object_generator(_context)
//:: for objectname, counter in object_iterator:
//::   extension = ""
//::   if counter !=0:
//::      extension = "1: thrift_switch_api_" + objectname + "_t structobject"
//::   #endif

thrift_switch_create_return_data_t switch_api_${objectname}_create(${extension});


thrift_switch_status_t switch_api_${objectname}_delete(1:thrift_switch_object_id_t  object_handle);

//:: #endfor

//:: attribute_iterator = template.attribute_generator(_context, False)
//:: for type, key, _, _, _, _, objectname in attribute_iterator:

thrift_switch_status_t switch_api_${objectname}_set_${key}(
        1: thrift_switch_object_id_t  object_handle, 2: thrift_${type} ${key});

thrift_switch_return_${type} switch_api_${objectname}_get_${key}(
        1: thrift_switch_object_id_t  object_handle);

//:: #endfor


}
