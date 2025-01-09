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


#ifndef _bf_switch_cli_h_
#define _bf_switch_cli_h_

#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include <unordered_map>
#include <functional>
#include <iomanip>
#include <sstream>

#include "bf_switch/bf_switch_types.h"
#include "bf_switch/bf_switch.h"
#include "s3/meta/meta.h"
#include "s3/attribute_util.h"

namespace smi {

namespace switchcli {

/****
 *  FIXME:
 *  MOVE following to Python script for auto generation.
 */
#define MAX_CLISH_VAR_NAME 50
#define MAX_CLISH_MAX_CMD_STR_LEN 100

#define BF_SWITCH_CLI_CLISH_IP_ADDR_SC_PNAME "address_sc"
#define BF_SWITCH_CLI_CLISH_IP4_ADDR_PNAME "ipv4_addr"
#define BF_SWITCH_CLI_CLISH_IP4_ADDR_LIST_PNAME "ipv4_addr_list"
#define BF_SWITCH_CLI_CLISH_IP6_ADDR_PNAME "ipv6_addr"
#define BF_SWITCH_CLI_CLISH_IP6_ADDR_LIST_PNAME "ipv6_addr_list"

#define BF_SWITCH_CLI_CLISH_IP_PREFIX_SC_PNAME "prefix_sc"
#define BF_SWITCH_CLI_CLISH_IP4_PREFIX_PNAME "ipv4_prefix"
#define BF_SWITCH_CLI_CLISH_IP4_PREFIX_LIST_PNAME "ipv4_prefix_list"
#define BF_SWITCH_CLI_CLISH_IP6_PREFIX_PNAME "ipv6_prefix"
#define BF_SWITCH_CLI_CLISH_IP6_PREFIX_LIST_PNAME "ipv6_prefix_list"

#define BF_SWITCH_CLI_CLISH_PNAME_OBJ_HANDLE "handle"
#define BF_SWITCH_CLI_CLISH_PNAME_OBJ_OID "oid"
#define BF_SWITCH_CLI_CLISH_PNAME_OBJ_NAME "object_name"
#define BF_SWITCH_CLI_RET_HANDLE_PNAME "p_entry_hdl"

#define BF_SWITCH_CLI_CLISH_UINT32_LIST_PNAME "lst"

enum smi_cli_operation_type_t {
  SMI_CLI_CREATE_OPERATION,
  SMI_CLI_DELETE_OPERATION,
  SMI_CLI_GET_OPERATION,
  SMI_CLI_SET_OPERATION,
  SMI_CLI_ALL_OPERATION
};

/* ENUMs */
typedef enum bf_switch_cli_status_ {
  BF_SWITCH_CLI_STATUS_SUCCESS_E = 0,               /* Status Success */
  BF_SWITCH_CLI_STATUS_FAILURE_E = 1,               /* General Failure */
  BF_SWITCH_CLI_STATUS_NO_MEMORY_E = 2,             /* No Memory */
  BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E = 3, /* clish pname not found */
  BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E = 4, /* clish invalid input value */
  BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E = 5, /* clish invalid device id */
  BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E =
      6, /* object not found (invalid key or create object before set)*/
  BF_SWITCH_CLI_STATUS_OBJ_KEY_PARAM_NOT_FOUND = 7, /* key group not found */
  BF_SWITCH_CLI_STATUS_OBJ_INVALID_KEY = 8, /* object not found , invalid key */
  BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E = 9, /* Invalid object 'handle */
  BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E = 10,  /* Invalid object type */
  BF_SWITCH_CLI_STATUS_CLISH_SHELL_VAR_NOT_FOUND_E = 11, /* Invalid shell var */
  BF_SWITCH_CLI_STATUS_CLISH_INV_IPV6_ADDR_E = 12, /* Invalid IPv6 address */
  BF_SWITCH_CLI_STATUS_OVERLAP_RANGE_ERROR_E = 13, /* Overlapping range input */
  BF_SWITCH_CLI_STATUS_NOT_SUPPORTED = 14,         /* Not supported */
  BF_SWITCH_CLI_STATUS_RANGE_EXCEEDS_MAX_LIMIT_E =
      15, /* range exceeds max limit */
  BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E =
      16,                                          /* attr type not supported */
  BF_SWITCH_CLI_STATUS_INV_COUNTER_HANDLE_E = 17,  /* invalid counter handle */
  BF_SWITCH_CLI_STATUS_INV_COUNTER_ID_E = 18,      /* invalid counter id */
  BF_SWITCH_CLI_STATUS_OBJECT_ALREADY_EXISTS = 19, /* Object already exists */
  BF_SWITCH_CLI_STATUS_OBJECT_IN_USE = 20,         /* Object in-use */
  BF_SWITCH_CLI_STATUS_MAX_E
} bf_switch_cli_status_t;

#define BF_SWITCH_CLI_CMD_DELIMITER " "

/*****/

/* Macros */

#define BF_SWITCH_CLI_MAC_ADDR_SIZE 6
#define BF_SWITCH_CLI_IP4_ADDR_SIZE 4
#define BF_SWITCH_CLI_IP6_ADDR_SIZE 16  // in bytes
#define BF_SWITCH_CLI_IPV6_MAX_SWORDS 8
#define BF_SWITCH_CLI_IPV6_MAX_DOUBLE_COLON_SWORDS (8 - 2)

#define BF_SWITCH_CLI_GET_ULONG(byte_array)                         \
  (byte_array[0] << 24 | byte_array[1] << 16 | byte_array[2] << 8 | \
   byte_array[3])

#define BF_SWITCH_CLI_IPV4_MASK_LEN_TO_IP_ADDR(mask_len) \
  (0xffffffff << (32 - mask_len))

#define BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE 200

#define BF_SWITCH_CLI_ASSERT(x) assert(x);

#define BF_SWITCH_CLI_MEMSET memset

/* show <object_name> <key> tabs */
#define BF_SWITCH_CLI_PRINT_OBJ_TAB "  "
#define BF_SWITCH_CLI_PRINT_OBJ_MD_TAB "    "
#define BF_SWITCH_CLI_PRINT_ATTR_MD_TAB "      "

/* show object tabs */
#define TABLE_HEADER_DECORATOR '='
#define OBJECT_NAME_TAB "  "
#define OBJECT_DESCRIPTION_TAB 11
#define OBJECT_TYPES_TABLE_HEADER_DELM \
  "======================================================"

/*
 * show all table specifications.
 *  ex: show vlan all
 *        ========================================================
 *        object_handle    vlan_id    attr2    attr3    attr4
 *        ========================================================
 */

#define DUMP_OBJECT_ALL_TABLE_COL1_OFFSET 2

#define TABLE_COL_SPACE 2

/* column width sepecfitions */
#define ATTR_TYPE_OBJECT_HANDLE_MAX_SIZE 8
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_HANDLE_COLUMN \
  ATTR_TYPE_OBJECT_HANDLE_MAX_SIZE

#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_ID_COLUMN 5

#define DUMP_OBJECT_RESERVED_COL_WIDTH ATTR_TYPE_OBJECT_HANDLE_MAX_SIZE

/* Note: all attribute max size includes space width of 2 */
#define ATTR_TYPE_UINT8_MAX_SIZE 5
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT8_COLUMN ATTR_TYPE_UINT8_MAX_SIZE

#define ATTR_TYPE_UINT16_MAX_SIZE 7
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT16_COLUMN ATTR_TYPE_UINT16_MAX_SIZE

#define ATTR_TYPE_UINT32_MAX_SIZE 12
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT32_COLUMN ATTR_TYPE_UINT32_MAX_SIZE

#define ATTR_TYPE_UINT64_MAX_SIZE 12
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT64_COLUMN ATTR_TYPE_UINT64_MAX_SIZE

#define ATTR_TYPE_MAC_ADDR_MAX_SIZE 22
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_MAC_ADDR_COLUMN \
  ATTR_TYPE_MAC_ADDR_MAX_SIZE

#define ATTR_TYPE_IP_ADDRESS_MAX_SIZE 22
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_IP_ADDRESS_COLUMN \
  ATTR_TYPE_IP_ADDRESS_MAX_SIZE

#define ATTR_TYPE_IP_PREFIX_MAX_SIZE 27
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_IP_PREFIX_COLUMN \
  ATTR_TYPE_IP_PREFIX_MAX_SIZE

#define ATTR_TYPE_BOOL_MAX_SIZE 6
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_BOOL_COLUMN ATTR_TYPE_BOOL_MAX_SIZE

#define ATTR_TYPE_ENUM_MAX_SIZE 32
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_ENUM_COLUMN ATTR_TYPE_ENUM_MAX_SIZE

#define ATTR_TYPE_STRING_MAX_SIZE 20
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_STRING_COLUMN ATTR_TYPE_STRING_MAX_SIZE

/*range dump format: <min-max> 10+1+10+2 */
#define ATTR_TYPE_RANGE_MAX_SIZE 23
#define DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_RANGE_COLUMN ATTR_TYPE_RANGE_MAX_SIZE

/* We can fine to this based on the user expeirience. */
#define TABLE_MAX_WIDTH 120

#define BF_SWITCH_CLI_PRINT_MAX_ELEM_PER_LINE 20

/* couner table width specifications */

#define DUMP_COUNTER_TBL_COUNTER_TYPE_COL ATTR_TYPE_UINT8_MAX_SIZE

#define DUMP_COUNTER_TBL_PKT_COUNT_COL 22

#define DUMP_COUNTER_TBL_PKTS_CHANGED_COUNT_COL 20

#define DUMP_COUNTER_TBL_COUNTER_ID_COL_DEF_WIDTH 30

#define DUMP_COUNTER_PKT_COL_WIDTH \
  DUMP_COUNTER_TBL_PKT_COUNT_COL + DUMP_COUNTER_TBL_PKTS_CHANGED_COUNT_COL

/* show all tabs */
#define CLI_ATTR_TABLE_HEADER_TAB "  "
#define CLI_ATTR_TABLE_HEADER \
  "======================================================"
#define CLI_ATTR_TABLE_TAB "    "

#define BF_SWITCH_CLI_CLISH_TBL_COL_PRINT(clish_context, width, str) \
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%-*s", width, str)

/*
 * describe object's attribute table specifications.
 */
#define DESC_OBJECT_OFFSET 2
#define DESC_OBJECT_ATTR_TABLE_COL1_OFFSET DESC_OBJECT_OFFSET + 2

/*
 *  Attribute table header fields.
 *      =================================
 *      name   type flags description
 *      =================================
 */

/* col_1: name width */
#define ATTR_NAME_MAX_SIZE 18
#define DESC_OBJECT_ATTR_TABLE_COL1_NAME_WIDTH ATTR_NAME_MAX_SIZE

/* col_2: type width */
#define ATTR_TYPE_MAX_SIZE 23
#define DESC_OBJECT_ATTR_TABLE_COL2_TYPE_WIDTH ATTR_TYPE_MAX_SIZE

/* col_3: flags width */
#define ATTR_FLAGS_MAX_SIZE 19
#define DESC_OBJECT_ATTR_TABLE_COL3_FLAGS_WIDTH ATTR_FLAGS_MAX_SIZE

#define ATTR_DESCRIPTION_SIZE 15
/* header footprint size */
#define DESC_OBJECT_ATTR_TABLE_HDR_SIZE                           \
  ATTR_NAME_MAX_SIZE + ATTR_TYPE_MAX_SIZE + ATTR_FLAGS_MAX_SIZE + \
      ATTR_DESCRIPTION_SIZE

/* describe attribute tabs */
#define DESC_OBJECT_ATTR_NAME_TAB "  "
#define DESC_OBJECT_LIST_ATTR_NAME_TAB "    "

#define BF_SWITCH_CLI_MAX_OBJ_PRINT_PER_CYCLE 1024
#define MAX_CLI_RANGE 10

#define BF_SWITCH_CLI_IPV4_MASK_BITS_SIZE 32

static inline uint32_t bf_switch_cli_ipv4_mask_addr_to_len(
    uint32_t ipv4_mask_addr) {
  uint32_t mask_len = 0;

  while (ipv4_mask_addr) {
    mask_len++;
    ipv4_mask_addr = ipv4_mask_addr << 0x01;
  }

  return mask_len;
}

static inline void smi_cli_clish_get_object_key_v_prefix(
    const char *ref_attr_name,
    const char *object_name,
    std::string &key_prefix) {
  key_prefix = "p_";
  key_prefix += ref_attr_name;
  key_prefix += "obj_";
  key_prefix += object_name;
}

static inline bf_switch_cli_status_t bf_switch_cli_switch_status_to_cli_status(
    switch_status_t status) {
  switch (status) {
    case SWITCH_STATUS_SUCCESS:
      return BF_SWITCH_CLI_STATUS_SUCCESS_E;
    case SWITCH_STATUS_ITEM_ALREADY_EXISTS:
      return BF_SWITCH_CLI_STATUS_OBJECT_ALREADY_EXISTS;
    case SWITCH_STATUS_ITEM_NOT_FOUND:
      return BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E;
    case SWITCH_STATUS_RESOURCE_IN_USE:
      return BF_SWITCH_CLI_STATUS_OBJECT_IN_USE;
    case SWITCH_STATUS_INVALID_PARAMETER:
      return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    default:
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
}

static inline const char *bf_switch_cli_error_to_string(
    bf_switch_cli_status_t status) {
  switch (status) {
    case BF_SWITCH_CLI_STATUS_FAILURE_E:
      return "Internal failure";
    case BF_SWITCH_CLI_STATUS_SUCCESS_E:
      return "Status success";
    case BF_SWITCH_CLI_STATUS_NO_MEMORY_E:
      return "No memory";
    case BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E:
      return "Attribute not found, user not configured";
    case BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E:
      return "Invalid input";
    case BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E:
      return "Device id null";
    case BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E:
      return "Object not found";
    case BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E:
      return "Invalid object handle";
    case BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E:
      return "Invalid object type (enter valid object handle)";
    case BF_SWITCH_CLI_STATUS_CLISH_SHELL_VAR_NOT_FOUND_E:
      return "Invalid shell variable";
    case BF_SWITCH_CLI_STATUS_CLISH_INV_IPV6_ADDR_E:
      return "Invalid IPv6 address";
    case BF_SWITCH_CLI_STATUS_OVERLAP_RANGE_ERROR_E:
      return "Overlapping range input error";
    case BF_SWITCH_CLI_STATUS_NOT_SUPPORTED:
      return "Not supported";
    case BF_SWITCH_CLI_STATUS_INV_COUNTER_HANDLE_E:
      return "Invalid counter handle";
    case BF_SWITCH_CLI_STATUS_INV_COUNTER_ID_E:
      return "Invalid counter id";
    case BF_SWITCH_CLI_STATUS_OBJECT_ALREADY_EXISTS:
      return "Object already exists";
    case BF_SWITCH_CLI_STATUS_OBJECT_IN_USE:
      return "Object in-use";
    case BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E:
      return "attr type not supported";
    default:
      return "Unknown failure";
  }
}

static inline const char *bf_switch_cli_attr_type_str(switch_attr_type_t t) {
  const char *type_str;
  switch (t) {
    case SWITCH_TYPE_NONE:
      type_str = "NONE";
      break;
    case SWITCH_TYPE_BOOL:
      type_str = "BOOL";
      break;
    case SWITCH_TYPE_UINT8:
      type_str = "UINT8";
      break;
    case SWITCH_TYPE_UINT16:
      type_str = "UINT16";
      break;
    case SWITCH_TYPE_UINT32:
      type_str = "UINT32";
      break;
    case SWITCH_TYPE_UINT64:
      type_str = "UINT64";
      break;

    case SWITCH_TYPE_INT64:
      type_str = "INT64";
      break;

    case SWITCH_TYPE_ENUM:
      type_str = "ENUM";
      break;

    case SWITCH_TYPE_MAC:
      type_str = "MAC";
      break;

    case SWITCH_TYPE_STRING:
      type_str = "STRING";
      break;

    case SWITCH_TYPE_IP_ADDRESS:
      type_str = "IP_ADDRESS";
      break;

    case SWITCH_TYPE_IP_PREFIX:
      type_str = "IP_PREFIX";
      break;

    case SWITCH_TYPE_OBJECT_ID:
      type_str = "OBJECT_ID";
      break;

    case SWITCH_TYPE_LIST:
      type_str = "LIST";
      break;
    case SWITCH_TYPE_MAX:
    default:
      type_str = "Unknown type";
      break;
  }
  return type_str;
}

/* Structs */

struct cli_range {
  uint32_t start;
  uint32_t end;
};

typedef struct tbl_view_attr_info_ {
  switch_attr_id_t attr_id;
  uint32_t attr_width;
} tbl_view_attr_info_t;

typedef struct tbl_view_attrs_ {
  std::vector<tbl_view_attr_info_t> view_attrs;
  uint32_t total_tbl_width;
} tbl_view_attrs_t;

typedef bf_switch_cli_status_t (*__fn_obj_key_parse_and_get_handler)(
    void *,
    const char *,
    switch_object_type_t,
    const char *,
    switch_object_id_t &);

/*
 * Function prototypes.
 */
int bf_switch_cli_replay_file_internal(void *clish_context);

int bf_switch_cli_add_object_internal(void *clish_context);

int bf_switch_cli_del_object_handle_internal(void *clish_context);

int bf_switch_cli_del_object_internal(void *clish_context);

int bf_switch_cli_set_object_internal(void *clish_context);

int bf_switch_cli_show_object_internal(void *clish_context);

int bf_switch_cli_show_object_all_internal(void *clish_context);

int bf_switch_cli_show_object_handle_internal(void *clish_context);

int bf_switch_cli_add_var_internal(void *clish_context);

int bf_switch_cli_get_var_internal(void *clish_context);

int bf_switch_cli_set_var_internal(void *clish_context);

int bf_switch_cli_show_table_info_internal(void *clish_context);

int bf_switch_cli_describe_object_internal(void *clish_context);

int bf_switch_cli_describe_attribute_internal(void *clish_context);

int bf_switch_cli_show_counter_internal(void *clish_context);

int bf_switch_cli_clear_counter_internal(void *clish_context);

const char *bf_switch_cli_error_to_string(bf_switch_cli_status_t status);

bf_switch_cli_status_t bf_switch_cli_lookup_object_handle_by_key(
    void *clish_context,
    const char *object_name,
    switch_object_id_t &object_handle);

bf_switch_cli_status_t bf_switch_cli_parse_range(void *clish_context,
                                                 const char *p_clish_pname_val,
                                                 cli_range *range_out,
                                                 uint32_t &count);

bf_switch_cli_status_t bf_switch_cli_dump_object_attrs_table_fmt(
    void *clish_context,
    uint16_t device,
    switch_object_id_t object_handle,
    const ObjectInfo *obj_info,
    tbl_view_attrs_t const &dump_tbl_attrs);
bf_switch_cli_status_t dump_object_types(void *clish_context, uint16_t device);

bf_switch_cli_status_t chk_overlap_range(const void *clish_context,
                                         cli_range *curr_range,
                                         uint32_t count,
                                         uint32_t start,
                                         uint32_t end);

bf_switch_cli_status_t bf_switch_cli_describe_object_attribute(
    void *clish_context,
    uint16_t device,
    const char *object_name,
    const char *attr_name);

bf_switch_cli_status_t bf_switch_cli_get_attr_col_width(
    const AttributeMetadata *p_attr_md, uint32_t &col_width);

bf_switch_cli_status_t bf_switch_cli_dump_object_dependencies(
    void *clish_context,
    uint16_t device,
    const ObjectInfo *obj_info,
    switch_object_id_t input_object_handle);

bf_switch_cli_status_t bf_switch_cli_scan_and_trim_tbl_attr_list(
    const ObjectInfo *obj_info,
    std::vector<switch_attr_id_t> &tbl_view_attr_ids,
    tbl_view_attrs_t &dump_tbl_attrs);
bf_switch_cli_status_t bf_switch_cli_populate_tbl_attrs(
    const ObjectInfo *obj_info, tbl_view_attrs_t &dump_tbl_attrs);

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_value(
    void *clish_context,
    const AttributeMetadata *p_attr_md,
    attr_w &value_attr,
    const char *clish_attr_value_pname);
bf_switch_cli_status_t bf_switch_cli_clish_parse_object_keys_and_get_handle(
    void *clish_context,
    switch_object_type_t object_type,
    const char *attr_name,
    switch_object_id_t &object_handle);

bf_switch_cli_status_t clish_parse_keys_and_lookup_handle_object_type_default(
    void *clish_context,
    switch_object_type_t object_type,
    const char *attr_name,
    switch_object_id_t &object_handle);

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_object_id(
    void *clish_context,
    const char *handle_attr_name,
    const AttributeMetadata *p_attr_md,
    switch_object_id_t &object_handle);

int bf_switch_cli_debug_operation_set_internal(void *clish_context);

int bf_switch_cli_packet_log_level_set_internal(void *clish_context);

int bf_switch_cli_bfd_log_level_set_internal(void *clish_context);

int bf_switch_cli_pkt_path_counter_print_internal(void *clish_context);

int bf_switch_cli_show_features_internal(void *clish_context);

int bf_switch_cli_log_level_show_internal(void *clish_context);

int bf_switch_cli_debug_log_level_set_internal(void *clish_context);

int bf_switch_cli_show_version_internal(void *clish_context);

int bf_switch_cli_show_cpu_rx_counter_internal(void *clish_context);
int bf_switch_cli_clear_cpu_rx_counter_internal(void *clish_context);

int bf_switch_cli_show_tech_support_internal(void *clish_context);

int bf_switch_cli_state_save_internal(void *clish_context);

} /* namespace switchcli */
} /* namespace smi */

#endif /* end of _bf_switch_cli_h_ */
