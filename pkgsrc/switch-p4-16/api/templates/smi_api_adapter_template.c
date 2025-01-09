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


#ifndef _SMI_API_ADAPTER_C_
#define _SMI_API_ADAPTER_C_

#include "smi_api_adapter.h"
#include "bf_switch/bf_switch.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//:: import template_generator as template
//:: prefix = "bf_switch"
//:: attribute_iterator = template.attribute_generator(_context, False)                                    
//:: object_dict = template.get_object_dict(_context)
//:: hasSeen = False
//:: oldObjectName = ""
//:: for type, key, default, type_enum, field, id, objectname in attribute_iterator:
//::   if objectname != oldObjectName:
//::      if hasSeen:
  return ${prefix}_object_create_c(${object_type},
                                     ${attribute_count},
                                     (switch_attribute_t * const) &switch_attributes,
                                     object_handle);
}
//::      #endif
//::      hasSeen = True
//::      oldObjectName = objectname
//::      counter = 0
//::      extension = ""
//::      attribute_count, object_type = object_dict[objectname]
//::      if attribute_count != 0:
//::        extension = ", switch_api_" + objectname + "_t *struct_object" 
//::      #endif

switch_status_t switch_api_${objectname}_create(
    switch_object_id_t *const object_handle${extension}){
  switch_attribute_t switch_attributes[${attribute_count}];
  switch_attribute_t switch_attribute;
  switch_attribute_value_t value;
//::  #endif
  value.${field} = struct_object->${key}; 
  value.type = ${type_enum};
  switch_attribute.id  = ${id};
  switch_attribute.value = value;
  switch_attributes[${counter}] = switch_attribute; 
//:: counter +=1
//:: #endfor
  return ${prefix}_object_create_c(${object_type}, ${attribute_count}, (switch_attribute_t * const) &switch_attributes,  object_handle);
}

//:: object_iterator = template.object_generator_with_type(_context)
//:: for objectname, counter, object_type in object_iterator:
switch_status_t switch_api_${objectname}_delete(switch_object_id_t const object_handle){
  return ${prefix}_object_delete_c(object_handle);
}

//:: if counter == 0:
switch_status_t switch_api_${objectname}_create(
    switch_object_id_t *const object_handle){
  switch_attribute_t switch_attributes[0];
  return ${prefix}_object_create_c(${object_type}, 0, (switch_attribute_t * const) &switch_attributes,  object_handle);
}

//:: #endif
//:: #endfor

//:: attribute_iterator = template.attribute_generator(_context, False)                                    
//:: for type, key, default, type_enum, field, id, objectname in attribute_iterator:
switch_status_t switch_api_${objectname}_set_${key}(
    switch_object_id_t const object_handle, ${type} ${key}){
  switch_attribute_value_t value;
  value.${field} = ${key}; 
  value.type = ${type_enum};
  switch_attribute_t attribute_struct = {${id}, value}; 
  return ${prefix}_attribute_set_c(object_handle, &attribute_struct);
}

switch_status_t switch_api_${objectname}_get_${key}(
        switch_object_id_t const object_handle, ${type} *${key}){
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_attribute_t attribute;
  status = ${prefix}_attribute_get_c(object_handle, ${id}, &attribute);
  *${key} = attribute.value.${field};
  return status;
}

//:: #endfor
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* header guard */
