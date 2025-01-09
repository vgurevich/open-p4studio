/**
 * Copyright (c) 2018 Microsoft Open Technologies, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *    THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 *    CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *    LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 *    FOR A PARTICULAR PURPOSE, MERCHANTABILITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 *    Microsoft would like to thank the following companies for their review and
 *    assistance with these files: Intel Corporation, Mellanox Technologies Ltd,
 *    Dell Products, L.P., Facebook, Inc., Marvell International Ltd.
 *
 * @file    saiaclextensions.h
 *
 * @brief   This module defines ACL extensions of the Switch Abstraction Interface (SAI)
 */

#ifndef __SAIACLEXTENSIONS_H_
#define __SAIACLEXTENSIONS_H_

#include <saitypes.h>
#include <saiacl.h>

/**
 * @brief ACL table attribute extensions
 *
 * @flags free
 */
typedef enum _sai_acl_table_attr_extensions_t
{
    /** Start of custom extensions range */
    SAI_ACL_TABLE_ATTR_FIELD_EXTENSIONS_START = SAI_ACL_TABLE_ATTR_END,

    /**
     * @brief Custom attr 0
     *
     * @type bool
     * @flags CREATE_ONLY
     * @default false
     */
    SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_0 = SAI_ACL_TABLE_ATTR_FIELD_EXTENSIONS_START,

    /**
     * @brief Custom attr 1
     *
     * @type bool
     * @flags CREATE_ONLY
     * @default false
     */
    SAI_ACL_TABLE_ATTR_FIELD_CUSTOM_1,

    /** End of custom extensions range */
    SAI_ACL_TABLE_ATTR_FIELD_EXTENSIONS_END

} sai_acl_table_attr_extensions_t;

/**
 * @brief ACL entry attribute extensions
 *
 * @flags free
 */
typedef enum _sai_acl_entry_attr_extensions_t
{
    /** Start of custom extensions range */
    SAI_ACL_ENTRY_ATTR_FIELD_EXTENSIONS_START = SAI_ACL_ENTRY_ATTR_ACTION_END + 0x1,

    /**
     * @brief Custom attr 0
     *
     * @type sai_acl_field_data_t sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ROUTER_INTERFACE
     * @default disabled
     */
    SAI_ACL_ENTRY_ATTR_FIELD_CUSTOM_0 = SAI_ACL_ENTRY_ATTR_FIELD_EXTENSIONS_START,

    /**
     * @brief Custom attr 1
     *
     * @type sai_acl_field_data_t sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ROUTER_INTERFACE
     * @default disabled
     */
    SAI_ACL_ENTRY_ATTR_FIELD_CUSTOM_1,

    /** End of custom extensions range */
    SAI_ACL_ENTRY_ATTR_FIELD_EXTENSIONS_END

} sai_acl_entry_attr_extensions_t;

#endif /* __SAIACLEXTENSIONS_H_ */
