/**
 * Copyright (c) 2022 Microsoft Open Technologies, Inc.
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
 * @file    sairouterinterfaceextensions.h
 *
 * @brief   This module defines router interface extensions of the Switch Abstraction Interface (SAI)
 */

#ifndef __SAIROUTERINTERFACEEXTENSIONS_H_
#define __SAIROUTERINTERFACEEXTENSIONS_H_

#include <saitypes.h>
#include <sairouterinterface.h>

/**
 * @brief SAI router interface attribute extensions
 *
 * @flags free
 */
typedef enum _sai_router_interface_attr_extensions_t
{
    /** Start of extensions range */
    SAI_ROUTER_INTERFACE_ATTR_EXTENSIONS_START = SAI_ROUTER_INTERFACE_ATTR_END,

    /**
     * @brief Custom attr 0
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_0 = SAI_ROUTER_INTERFACE_ATTR_EXTENSIONS_START,

    /**
     * @brief Custom attr 1
     *
     * @type sai_mac_t
     * @flags CREATE_AND_SET
     * @default vendor
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_1,

    /**
     * @brief Custom attr 2
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_QOS_MAP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_2,

    /**
     * @brief Custom attr 3
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_QOS_MAP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_3,

    /**
     * @brief Custom attr 4
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_QOS_MAP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_4,

    /**
     * @brief Custom attr 5
     *
     * @type sai_mac_t
     * @flags CREATE_AND_SET
     * @default vendor
     */
    SAI_ROUTER_INTERFACE_ATTR_CUSTOM_5,

    /** End of custom extensions range */
    SAI_ROUTER_INTERFACE_ATTR_EXTENSIONS_END

} sai_router_interface_attr_extensions_t;

#endif /* __SAIROUTERINTERFACEEXTENSIONS_H_ */
