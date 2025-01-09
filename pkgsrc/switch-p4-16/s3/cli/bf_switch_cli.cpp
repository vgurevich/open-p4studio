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


/*
 * bf_switchcli_clish.h - this contains inline functions like
 *         #define BF_SWITCH_CLI_CLISH_PNAME_ADDR_FAMILY "addr_family"
 *         #define BF_SWITCH_CLI_CLISH_PNAME_ADDR_FAMILY "ip_addr"
 */
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <arpa/inet.h>
#include <inttypes.h>

#include "bf_switch/bf_switch_types.h"
#include "common/utils.h"
#include "s3/attribute.h"
#include "s3/bf_rt_backend.h"
#include "s3/switch_bfdd.h"
#include "s3/switch_store.h"
#include "s3/switch_packet.h"
#include "s3/record.h"
#include "bf_switch/bf_switch.h"
#include "bf_switch_cli.h"
#include "bf_switch_cli_clish.h"
#include "bf_switch_cli_api.h"
#include "../log.h"
#include "../store.h"

namespace smi {
namespace switchcli {

using namespace ::smi;
using namespace smi::logging;

std::string clish_name_scratch_buff;

static inline const char *bf_switch_cli_clish_get_pname(const char *attr_name,
                                                        const char *postfix) {
  clish_name_scratch_buff.clear();
  clish_name_scratch_buff = fmt::format("{}_{}", attr_name, postfix);
  return clish_name_scratch_buff.c_str();
}

static inline const char *bf_switch_cli_clish_get_val_pname(
    const char *attr_name, const char *postfix) {
  clish_name_scratch_buff.clear();
  clish_name_scratch_buff = fmt::format("p_{}_{}", attr_name, postfix);
  return clish_name_scratch_buff.c_str();
}

static inline const char *bf_switch_cli_handle_to_string(
    switch_object_id_t object_handle) {
  clish_name_scratch_buff.clear();
  clish_name_scratch_buff = fmt::format("{}", object_handle.data);
  return clish_name_scratch_buff.c_str();
}

static inline const char *bf_switch_cli_map_attr_name_to_clish_val_pname(
    const char *attr_name) {
  clish_name_scratch_buff.clear();
  clish_name_scratch_buff = fmt::format("p_{}", attr_name);
  return clish_name_scratch_buff.c_str();
}

void static inline reset_buff_(char *buff_ptr) {
  if (NULL != buff_ptr) {
    bfshell_string_free(buff_ptr);
    buff_ptr = NULL;
  }
}

static bf_switch_cli_status_t bf_switch_cli_clish_get_hex_bytes(
    const char *p_clish_var, switch_mac_addr_t *mac, uint32_t size) {
  // 12 chars + 5 :
  if (strlen(p_clish_var) != ((size * 2) + 5)) {
    return (BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E);
  }

  unsigned int mac_tmp[6] = {};
  if (std::sscanf(p_clish_var,
                  "%x:%x:%x:%x:%x:%x",
                  &mac_tmp[0],
                  &mac_tmp[1],
                  &mac_tmp[2],
                  &mac_tmp[3],
                  &mac_tmp[4],
                  &mac_tmp[5]) != 6) {
    return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
  }
  for (size_t i = 0; i < 6; i++) {
    assert(mac_tmp[i] <= UINT8_MAX);
    mac->mac[i] = static_cast<char>(mac_tmp[i]);
  }
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

static std::vector<std::string> split(const std::string &str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

bf_switch_cli_status_t bf_switch_cli_clish_find_kw(void *clish_context,
                                                   const char *kw) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;

  parg = clish_pargv_find_arg(pargv, kw);
  if (parg) {
    return BF_SWITCH_CLI_STATUS_SUCCESS_E;
  }

  return BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_pname_value(
    void *clish_context, const char *p_name, char const *&p_value) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;
  const char *p_value_tmp;

  parg = clish_pargv_find_arg(pargv, p_name);
  if (parg) {
    p_value_tmp = clish_parg__get_value(parg);
    if (p_value_tmp) {
      if (p_value_tmp[0] == '$') {
        const char *shl_var_name = p_value_tmp;
        /* get value from shell variable. */
        p_value_tmp = clish_shell_strmap_get(
            static_cast<clish_context_t *>(clish_context), shl_var_name);
        if (p_value_tmp == (char *)0x01) {
          return (BF_SWITCH_CLI_STATUS_CLISH_SHELL_VAR_NOT_FOUND_E);
        }
      }
      p_value = p_value_tmp;
      return BF_SWITCH_CLI_STATUS_SUCCESS_E;
    }
  }
  return BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_type_object_handle(
    void *clish_context, switch_object_id_t &object_handle) {
  const char *p_clish_obj_handle = NULL;
  const char *clish_obj_hdl_val_pname;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  clish_obj_hdl_val_pname = bf_switch_cli_map_attr_name_to_clish_val_pname(
      BF_SWITCH_CLI_CLISH_PNAME_OBJ_HANDLE);

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, clish_obj_hdl_val_pname, p_clish_obj_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  object_handle.data = strtoull(p_clish_obj_handle, NULL, 0);
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

/*
 * Note: cli allow user to enter object handle input as 64 bit number or
 *       lower 48 bit number
 * in second case, this function will covert 48 bit number to 64 bit number.
 *
 */

bf_switch_cli_status_t bf_switch_cli_clish_fetch_oid_and_build_handle(
    void *clish_context,
    const char *object_name,
    switch_object_id_t &object_handle) {
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const char *p_clish_obj_oid = NULL;
  const char *clish_obj_oid_val_pname;
  uint64_t handle;
  switch_object_type_t handle_type;
  switch_object_type_t object_type;
  switch_object_id_t tmp_obj_handle;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  clish_obj_oid_val_pname = bf_switch_cli_map_attr_name_to_clish_val_pname(
      BF_SWITCH_CLI_CLISH_PNAME_OBJ_OID);

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, clish_obj_oid_val_pname, p_clish_obj_oid);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  const ObjectInfo *object_info =
      model_info->get_object_info_from_name(object_name);
  if (object_info == NULL) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
  object_type = object_info->object_type;
  // oid = strtoul(p_clish_obj_oid, NULL, 0);
  handle = strtoull(p_clish_obj_oid, NULL, 0);
  tmp_obj_handle.data = handle;
  handle_type = switch_store::object_type_query(tmp_obj_handle);
  if (!handle_type) {
    /* input is object_id, build handle */
    object_handle = switch_store::id_to_handle(object_type, handle);
  } else {
    /* qualify handle type */
    if (handle_type != object_type) {
      return BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E;
    }
    object_handle.data = handle;
  }
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_ip_prefix_list(
    void *clish_context,
    const char *attr_name,
    std::vector<switch_ip_prefix_t> &ip_prefix_list) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = NULL;
  const char *p_clish_pname_val = NULL;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const char *clish_ipv4_prefix_list_pname = NULL;
  const char *clish_ipv6_prefix_list_pname = NULL;
  const char *clish_prefix_sc_name = NULL;
  const char *p_clish_ipv4_list_val_pname = NULL;
  const char *p_clish_ipv6_list_val_pname = NULL;

  clish_prefix_sc_name = bf_switch_cli_clish_get_pname(
      attr_name, BF_SWITCH_CLI_CLISH_IP_PREFIX_SC_PNAME);
  parg = clish_pargv_find_arg(pargv, clish_prefix_sc_name);
  if (parg) {
    p_clish_pname_val = clish_parg__get_value(parg);
    if (p_clish_pname_val) {
      clish_ipv4_prefix_list_pname = bf_switch_cli_clish_get_val_pname(
          attr_name, BF_SWITCH_CLI_CLISH_IP4_PREFIX_LIST_PNAME);

      if (!strcmp(p_clish_pname_val, clish_ipv4_prefix_list_pname)) {
        status =
            bf_switch_cli_clish_get_pname_value(clish_context,
                                                clish_ipv4_prefix_list_pname,
                                                p_clish_ipv4_list_val_pname);
        if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
          return status;
        }

        std::vector<std::string> param_values =
            split(std::string(p_clish_ipv4_list_val_pname), ',');
        for (const auto &param_value : param_values) {
          switch_ip_prefix_t p_ip_prefix = {};
          p_ip_prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
          switch_ip4_t ipv4_addr = 0;
          std::vector<std::string> ipv4_addr_mask =
              split(std::string(param_value), '/');
          if (ipv4_addr_mask.size() == 2) {
            p_ip_prefix.len = static_cast<uint16_t>(
                std::stoul(ipv4_addr_mask[1], nullptr, 0));
            if (inet_pton(AF_INET,
                          ipv4_addr_mask[0].c_str(),
                          static_cast<void *>(&ipv4_addr)) != 1) {
              status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
              BF_SWITCH_CLI_CLISH_PRINT(
                  clish_context,
                  "Failed to parse IPv4 address for IP Prefix %s\n",
                  param_value.c_str());
              return status;
            }
          } else {
            // Default prefix length for ipv4 address is 32 ( Host IPv4 Address
            // )
            p_ip_prefix.len = 32;
            if (inet_pton(AF_INET,
                          param_value.c_str(),
                          static_cast<void *>(&ipv4_addr)) != 1) {
              status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
              BF_SWITCH_CLI_CLISH_PRINT(
                  clish_context,
                  "Failed to parse IPv4 address for IP Prefix %s\n",
                  param_value.c_str());
              return status;
            }
          }
          p_ip_prefix.addr.ip4 = ntohl(static_cast<uint32_t>(ipv4_addr));
          ip_prefix_list.push_back(p_ip_prefix);
        }
        status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
      } else {
        clish_ipv6_prefix_list_pname = bf_switch_cli_clish_get_val_pname(
            attr_name, BF_SWITCH_CLI_CLISH_IP6_PREFIX_LIST_PNAME);
        if (!strcmp(p_clish_pname_val, clish_ipv6_prefix_list_pname)) {
          /* ipv6 prefix type */
          status =
              bf_switch_cli_clish_get_pname_value(clish_context,
                                                  clish_ipv6_prefix_list_pname,
                                                  p_clish_ipv6_list_val_pname);
          if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
            return status;
          }
          std::vector<std::string> param_values =
              split(std::string(p_clish_ipv6_list_val_pname), ',');
          for (const auto &param_value : param_values) {
            switch_ip_prefix_t p_ip_prefix = {};
            p_ip_prefix.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
            sw_status =
                attr_util::parse_ip_prefix(param_value.c_str(), p_ip_prefix);
            if (sw_status != SWITCH_STATUS_SUCCESS) {
              BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                        "Failed to parse IPv6 prefix %s\n",
                                        param_value.c_str());
              return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
            }
            ip_prefix_list.push_back(p_ip_prefix);
          }
          status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
        }
      }
    }
  }

  return status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_uint32_list(
    void *clish_context,
    const char *attr_name,
    std::vector<uint32_t> &uint32_list) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = NULL;
  const char *p_clish_pname_val = NULL;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
  const char *clish_uint32_list_pname = NULL;
  const char *clish_uint32_list_name = NULL;
  const char *p_clish_uint32_list_val_pname = NULL;
  clish_uint32_list_name = bf_switch_cli_clish_get_pname(
      attr_name, BF_SWITCH_CLI_CLISH_UINT32_LIST_PNAME);
  parg = clish_pargv_find_arg(pargv, clish_uint32_list_name);
  if (parg) {
    p_clish_pname_val = clish_parg__get_value(parg);
    if (p_clish_pname_val) {
      clish_uint32_list_pname = bf_switch_cli_clish_get_val_pname(
          attr_name, BF_SWITCH_CLI_CLISH_UINT32_LIST_PNAME);
      status =
          bf_switch_cli_clish_get_pname_value(clish_context,
                                              clish_uint32_list_pname,
                                              p_clish_uint32_list_val_pname);
      if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        return status;
      }

      std::vector<std::string> param_values =
          split(std::string(p_clish_uint32_list_val_pname), ',');
      for (const auto &param_value : param_values) {
        uint32_list.push_back(stoi(param_value));
      }
    }
  }

  return status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_ip_prefix(
    void *clish_context,
    const char *attr_name,
    switch_ip_prefix_t *ipprefix_out) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;
  const char *p_clish_pname_val;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const char *clish_ipv4_prefix_pname;
  const char *clish_ipv6_prefix_pname;
  const char *clish_prefix_sc_name;
  const char *p_clish_ipv4_val_pname;
  const char *p_clish_ipv6_val_pname;

  clish_prefix_sc_name = bf_switch_cli_clish_get_pname(
      attr_name, BF_SWITCH_CLI_CLISH_IP_PREFIX_SC_PNAME);
  parg = clish_pargv_find_arg(pargv, clish_prefix_sc_name);
  if (parg) {
    p_clish_pname_val = clish_parg__get_value(parg);
    if (p_clish_pname_val) {
      clish_ipv4_prefix_pname = bf_switch_cli_clish_get_val_pname(
          attr_name, BF_SWITCH_CLI_CLISH_IP4_PREFIX_PNAME);

      if (!strcmp(p_clish_pname_val, clish_ipv4_prefix_pname)) {
        /* ipv4 prefix type */
        uint8_t ipv4_addr_array[BF_SWITCH_CLI_IP4_ADDR_SIZE];
        uint32_t mask_len = 32;

        status = bf_switch_cli_clish_get_pname_value(
            clish_context, clish_ipv4_prefix_pname, p_clish_ipv4_val_pname);
        if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
          return status;
        }

        /* if mask len not specified, default will be host address (i.e /32) */
        sscanf(p_clish_ipv4_val_pname,
               "%hhu.%hhu.%hhu.%hhu/%u",
               &ipv4_addr_array[0],
               &ipv4_addr_array[1],
               &ipv4_addr_array[2],
               &ipv4_addr_array[3],
               &mask_len);
        ipprefix_out->addr.ip4 = BF_SWITCH_CLI_GET_ULONG(ipv4_addr_array);
        ipprefix_out->len = mask_len;
        ipprefix_out->addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
        status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
      } else {
        clish_ipv6_prefix_pname = bf_switch_cli_clish_get_val_pname(
            attr_name, BF_SWITCH_CLI_CLISH_IP6_PREFIX_PNAME);
        if (!strcmp(p_clish_pname_val, clish_ipv6_prefix_pname)) {
          /* ipv6 prefix type */
          status = bf_switch_cli_clish_get_pname_value(
              clish_context, clish_ipv6_prefix_pname, p_clish_ipv6_val_pname);
          if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
            return status;
          }

          sw_status =
              attr_util::parse_ip_prefix(p_clish_ipv6_val_pname, *ipprefix_out);
          if (sw_status != SWITCH_STATUS_SUCCESS) {
            BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                      "Failed to parse IPv6 prefix %s\n",
                                      p_clish_ipv6_val_pname);
            return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
          }
          status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
        }
      }
    }
  }

  return status;
}

bf_switch_cli_status_t clish_parse_and_fetch_attr_range(
    void *clish_context,
    const char *attr_name,
    switch_attr_list_t &range_list_out) {
  cli_range range_out[MAX_CLI_RANGE];
  uint32_t count = 0;
  uint32_t itr = 0;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *p_range_value;
  const char *range_val_pname;

  range_val_pname = bf_switch_cli_clish_get_val_pname(attr_name, "_range");
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, range_val_pname, p_range_value);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  cli_status =
      bf_switch_cli_parse_range(clish_context, p_range_value, range_out, count);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "range parse failed, status: %s\n",
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  range_list_out.list = new switch_attribute_value_t[count];
  if (!range_list_out.list) {
    cli_status = BF_SWITCH_CLI_STATUS_NO_MEMORY_E;
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "range cli failed, status: %s\n",
        bf_switch_cli_error_to_string(BF_SWITCH_CLI_STATUS_NO_MEMORY_E));
    return (BF_SWITCH_CLI_STATUS_NO_MEMORY_E);
  }

  range_list_out.list_type = SWITCH_TYPE_RANGE;
  range_list_out.count = count;
  for (; itr < count; itr++) {
    range_list_out.list[itr].range.min = range_out->start;
    range_list_out.list[itr].range.max = range_out->end;
    range_list_out.list[itr].type = SWITCH_TYPE_RANGE;
  }
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_ip_addr(
    void *clish_context,
    const char *attr_name,
    switch_ip_address_t *ipaddr_out) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;
  const char *p_clish_pname_val;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const char *clish_ipv4_addr_pname;
  const char *clish_ipv6_addr_pname;
  const char *clish_addr_sc_name;
  const char *p_clish_ipv4_val_pname;
  const char *p_clish_ipv6_val_pname;

  clish_addr_sc_name = bf_switch_cli_clish_get_pname(
      attr_name, BF_SWITCH_CLI_CLISH_IP_ADDR_SC_PNAME);
  parg = clish_pargv_find_arg(pargv, clish_addr_sc_name);
  if (parg) {
    p_clish_pname_val = clish_parg__get_value(parg);
    if (p_clish_pname_val) {
      clish_ipv4_addr_pname = bf_switch_cli_clish_get_val_pname(
          attr_name, BF_SWITCH_CLI_CLISH_IP4_ADDR_PNAME);
      if (!strcmp(p_clish_pname_val, clish_ipv4_addr_pname)) {
        /* ipv4 addr type */
        uint8_t ipv4_addr_array[BF_SWITCH_CLI_IP4_ADDR_SIZE];

        status = bf_switch_cli_clish_get_pname_value(
            clish_context, clish_ipv4_addr_pname, p_clish_ipv4_val_pname);
        if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
          return status;
        }
        sscanf(p_clish_ipv4_val_pname,
               "%hhu.%hhu.%hhu.%hhu",
               &ipv4_addr_array[0],
               &ipv4_addr_array[1],
               &ipv4_addr_array[2],
               &ipv4_addr_array[3]);
        ipaddr_out->ip4 = BF_SWITCH_CLI_GET_ULONG(ipv4_addr_array);
        ipaddr_out->addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
        status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
      } else {
        clish_ipv6_addr_pname = bf_switch_cli_clish_get_val_pname(
            attr_name, BF_SWITCH_CLI_CLISH_IP6_ADDR_PNAME);
        if (!strcmp(p_clish_pname_val, clish_ipv6_addr_pname)) {
          /* ipv6 addr type */
          status = bf_switch_cli_clish_get_pname_value(
              clish_context, clish_ipv6_addr_pname, p_clish_ipv6_val_pname);
          if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
            return status;
          }

          sw_status =
              attr_util::parse_ip_address(p_clish_ipv6_val_pname, *ipaddr_out);
          if (sw_status != SWITCH_STATUS_SUCCESS) {
            BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                      "Failed to parse IPv6 address %s\n",
                                      p_clish_ipv6_val_pname);
            return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
          }

          ipaddr_out->addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
          status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
        }
      }
    }
  }

  return status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_ip_addr_list(
    void *clish_context,
    const char *attr_name,
    std::vector<switch_ip_address_t> &ip_addrs) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;
  const char *p_clish_pname_val;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;
  switch_status_t sw_status = SWITCH_STATUS_SUCCESS;
  const char *clish_ipv4_addr_list_pname;
  const char *clish_ipv6_addr_list_pname;
  const char *clish_addr_sc_name;
  const char *p_clish_ipv4_list_val_pname;
  const char *p_clish_ipv6_list_val_pname;

  clish_addr_sc_name = bf_switch_cli_clish_get_pname(
      attr_name, BF_SWITCH_CLI_CLISH_IP_ADDR_SC_PNAME);
  parg = clish_pargv_find_arg(pargv, clish_addr_sc_name);
  if (parg) {
    p_clish_pname_val = clish_parg__get_value(parg);
    if (p_clish_pname_val) {
      clish_ipv4_addr_list_pname = bf_switch_cli_clish_get_val_pname(
          attr_name, BF_SWITCH_CLI_CLISH_IP4_ADDR_LIST_PNAME);
      if (!strcmp(p_clish_pname_val, clish_ipv4_addr_list_pname)) {
        status =
            bf_switch_cli_clish_get_pname_value(clish_context,
                                                clish_ipv4_addr_list_pname,
                                                p_clish_ipv4_list_val_pname);
        if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
          return status;
        }
        std::vector<std::string> param_values =
            split(std::string(p_clish_ipv4_list_val_pname), ',');
        for (const auto &param_value : param_values) {
          switch_ip_address_t p_ip_addr = {.addr_family =
                                               SWITCH_IP_ADDR_FAMILY_IPV4};
          switch_ip4_t ipv4_addr = 0;
          if (inet_pton(AF_INET,
                        param_value.c_str(),
                        static_cast<void *>(&ipv4_addr)) != 1) {
            status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
            BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                      "Failed to parse IPv4 address %s\n",
                                      param_value.c_str());
            return status;
          }
          p_ip_addr.ip4 = ntohl(ipv4_addr);
          ip_addrs.push_back(p_ip_addr);
        }
        status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
      } else {
        clish_ipv6_addr_list_pname = bf_switch_cli_clish_get_val_pname(
            attr_name, BF_SWITCH_CLI_CLISH_IP6_ADDR_LIST_PNAME);
        if (!strcmp(p_clish_pname_val, clish_ipv6_addr_list_pname)) {
          /* ipv6 addr type */
          status =
              bf_switch_cli_clish_get_pname_value(clish_context,
                                                  clish_ipv6_addr_list_pname,
                                                  p_clish_ipv6_list_val_pname);
          if (status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
            return status;
          }

          std::vector<std::string> param_values =
              split(std::string(p_clish_ipv6_list_val_pname), ',');
          for (const auto &param_value : param_values) {
            switch_ip_address_t p_ip_addr = {.addr_family =
                                                 SWITCH_IP_ADDR_FAMILY_IPV6};
            sw_status =
                attr_util::parse_ip_address(param_value.c_str(), p_ip_addr);
            if (sw_status != SWITCH_STATUS_SUCCESS) {
              BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                        "Failed to parse IPv6 address %s\n",
                                        param_value.c_str());
              return BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
            }
            ip_addrs.push_back(p_ip_addr);
          }
          status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
        }
      }
    }
  }

  return status;
}

/*
 * Every object has oid (object id) .
 * Default key:  (oid)
 * derive list of object handles using (object_type, object id)
 */

bf_switch_cli_status_t clish_parse_keys_and_lookup_handle_object_type_list(
    void *clish_context,
    const char *handle_attr_name,
    switch_object_type_t lookup_object_type,
    const char *ref_attr_name,
    std::vector<switch_object_id_t> &object_handles) {
  std::string oid_clish_pname;
  const char *p_clish_pname_val = NULL;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  uint32_t id = 0;

  oid_clish_pname = "p_";
  oid_clish_pname += handle_attr_name;
  oid_clish_pname += "_";
  oid_clish_pname += ref_attr_name;
  oid_clish_pname += "_oid";

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, oid_clish_pname.c_str(), p_clish_pname_val);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  if (p_clish_pname_val != NULL) {
    std::vector<std::string> param_values =
        split(std::string(p_clish_pname_val), ',');
    for (const auto &param_value : param_values) {
      id = strtoul(param_value.c_str(), NULL, 0);
      object_handles.push_back(
          switch_store::id_to_handle(lookup_object_type, id));
    }
  }
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

/*
 * Every object has oid (object id) .
 * Default key:  (oid)
 * derive object handle using (object_type, object id)
 */

bf_switch_cli_status_t clish_parse_keys_and_lookup_handle_object_type_default(
    void *clish_context,
    const char *handle_attr_name,
    switch_object_type_t lookup_object_type,
    const char *ref_attr_name,
    switch_object_id_t &object_handle) {
  std::string oid_clish_pname;
  const char *p_clish_pname_val;
  bf_switch_cli_status_t cli_status;
  uint32_t id;

  oid_clish_pname = "p_";
  oid_clish_pname += handle_attr_name;
  oid_clish_pname += "_";
  oid_clish_pname += ref_attr_name;
  oid_clish_pname += "_oid";

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, oid_clish_pname.c_str(), p_clish_pname_val);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  id = strtoul(p_clish_pname_val, NULL, 0);
  object_handle = switch_store::id_to_handle(lookup_object_type, id);
  if (!db::object_exists(object_handle)) {
    cli_status = BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s %s.%lu %s\n",
                              handle_attr_name,
                              ref_attr_name,
                              id,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

bf_switch_cli_status_t bf_switch_cli_clish_parse_object_keys_and_get_handle(
    void *clish_context,
    const char *handle_attr_name,
    switch_object_type_t lookup_object_type,
    const char *ref_attr_name,
    switch_object_id_t &object_handle) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  __fn_obj_key_parse_and_get_handler parse_and_get_handle_fn_ptr;

  switch (lookup_object_type) {
    default:
      parse_and_get_handle_fn_ptr =
          clish_parse_keys_and_lookup_handle_object_type_default;
  }

  cli_status = (parse_and_get_handle_fn_ptr)(clish_context,
                                             handle_attr_name,
                                             lookup_object_type,
                                             ref_attr_name,
                                             object_handle);
  return cli_status;
}

bool bf_switch_cli_clish_is_object_handle_null(void *clish_context) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  return (bool)clish_pargv_find_arg(pargv, "0");
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_list(
    void *clish_context,
    const char *handle_attr_name,
    const AttributeMetadata *p_attr_md,
    attr_w &value_attr) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ValueMetadata *value_md = p_attr_md->get_value_metadata();

  switch (value_md->type) {
    case SWITCH_TYPE_OBJECT_ID: {
      std::vector<switch_object_id_t> object_handles;
      for (const auto allowed_object_type :
           value_md->get_allowed_object_types()) {
        const ObjectInfo *allowed_object_info =
            model_info->get_object_info(allowed_object_type);
        if (!allowed_object_info)
          return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
        cli_status = clish_parse_keys_and_lookup_handle_object_type_list(
            clish_context,
            handle_attr_name,
            allowed_object_type,
            allowed_object_info->get_object_name().c_str(),
            object_handles);
        if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
          /* lookup success. return handle. */
          value_attr.v_set(object_handles);
          break;
        }
      }
    } break;
    case SWITCH_TYPE_IP_ADDRESS: {
      std::vector<switch_ip_address_t> ip_addrs;
      cli_status = bf_switch_cli_clish_get_attr_val_type_ip_addr_list(
          clish_context, handle_attr_name, ip_addrs);
      if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        /* lookup success. return handle. */
        value_attr.v_set(ip_addrs);
      }
    } break;
    case SWITCH_TYPE_IP_PREFIX: {
      std::vector<switch_ip_prefix_t> ip_prefix_list;
      cli_status = bf_switch_cli_clish_get_attr_val_type_ip_prefix_list(
          clish_context, handle_attr_name, ip_prefix_list);
      if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        /* lookup success. return handle. */
        value_attr.v_set(ip_prefix_list);
      }
    } break;
    case SWITCH_TYPE_UINT32: {
      std::vector<uint32_t> uint32_list;
      cli_status = bf_switch_cli_clish_get_attr_val_type_uint32_list(
          clish_context, handle_attr_name, uint32_list);
      if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        /* lookup success. return handle. */
        value_attr.v_set(uint32_list);
      }
    } break;
    default:
      cli_status = BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E;
  }
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_val_type_object_id(
    void *clish_context,
    const char *handle_attr_name,
    const AttributeMetadata *p_attr_md,
    switch_object_id_t &object_handle) {
  bf_switch_cli_status_t cli_status =
      BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ValueMetadata *value_md = p_attr_md->get_value_metadata();
  for (const auto allowed_object_type : value_md->get_allowed_object_types()) {
    const ObjectInfo *allowed_object_info =
        model_info->get_object_info(allowed_object_type);
    if (!allowed_object_info) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    cli_status = bf_switch_cli_clish_parse_object_keys_and_get_handle(
        clish_context,
        handle_attr_name,
        allowed_object_type,
        allowed_object_info->get_object_name().c_str(),
        object_handle);
    if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      /* lookup success. return handle. */
      break;
    } else if (cli_status == BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E) {
      /* lookup failure. object type found in clish context but corresponding
       * object id does not exist in db.
       * This happens when user inputs a object id that has yet not been
       * created. */
      break;
    }
  }

  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clish_get_attr_value(
    void *clish_context,
    const AttributeMetadata *p_attr_md,
    attr_w &value_attr,
    const char *clish_attr_value_pname) {
  // clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const char *clish_val_pname;
  const char *p_clish_pname_val;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  std::string attr_name = p_attr_md->get_attr_name();
  switch_attr_type_t attr_type = p_attr_md->type;

  if ((attr_type != SWITCH_TYPE_IP_ADDRESS) &&
      (attr_type != SWITCH_TYPE_IP_PREFIX) &&
      (attr_type != SWITCH_TYPE_OBJECT_ID) && (attr_type != SWITCH_TYPE_LIST)) {
    if (!clish_attr_value_pname) {
      clish_val_pname =
          bf_switch_cli_map_attr_name_to_clish_val_pname(attr_name.c_str());
    } else {
      clish_val_pname = clish_attr_value_pname;
    }
    cli_status = bf_switch_cli_clish_get_pname_value(
        clish_context, clish_val_pname, p_clish_pname_val);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      return cli_status;
    }
  }

  switch (attr_type) {
    case SWITCH_TYPE_BOOL:
      value_attr.v_set(strtoul(p_clish_pname_val, NULL, 0) ? true : false);
      break;
    case SWITCH_TYPE_UINT8:
      value_attr.v_set(
          static_cast<uint8_t>(strtoul(p_clish_pname_val, NULL, 0)));
      break;
    case SWITCH_TYPE_UINT16:
      value_attr.v_set(
          static_cast<uint16_t>(strtoul(p_clish_pname_val, NULL, 0)));
      break;
    case SWITCH_TYPE_UINT32:
      value_attr.v_set(
          static_cast<uint32_t>(strtoul(p_clish_pname_val, NULL, 0)));
      break;
    case SWITCH_TYPE_UINT64:
      value_attr.v_set(
          static_cast<uint64_t>(strtoul(p_clish_pname_val, NULL, 0)));
      break;
    case SWITCH_TYPE_INT64:
      value_attr.v_set(
          static_cast<int64_t>(strtol(p_clish_pname_val, NULL, 0)));
      break;
    case SWITCH_TYPE_OBJECT_ID: {
      switch_object_id_t object_handle = {SWITCH_NULL_OBJECT_ID};
      switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
      if (attr_name.compare("device") == 0) {
        ModelInfo *model_info = switch_store::switch_model_info_get();
        const ObjectInfo *obj_info =
            model_info->get_object_info_from_name(attr_name);
        if (obj_info)
          switch_status = switch_store::object_get_first_handle(
              obj_info->object_type, object_handle);
        cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
      } else if (!bf_switch_cli_clish_is_object_handle_null(clish_context)) {
        cli_status = bf_switch_cli_clish_get_attr_val_type_object_id(
            clish_context, attr_name.c_str(), p_attr_md, object_handle);
      }
      if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) return cli_status;
      value_attr.v_set(object_handle);
      break;
    }
    case SWITCH_TYPE_ENUM: {
      switch_enum_t switch_enum_val;
      uint64_t clish_enum_val;
      clish_enum_val = strtoul(p_clish_pname_val, NULL, 0);
#if 0
        /*
         * FIXME, add following function and remove below enumdata assigment.
         * first convert clish_enum_val to model enum_name using clish API
         * then convert model enum_name to switch_store enum value
         */
        value->enumdata.enumdata = bf_switch_cli_clish_map_attr_enum_val_to_name(attr_name,
                                                                           clish_enum_val);

#endif
      switch_enum_val.enumdata = clish_enum_val;
      value_attr.v_set(switch_enum_val);
      break;
    }
    case SWITCH_TYPE_LIST: {
      const ValueMetadata *value_md = p_attr_md->get_value_metadata();
      cli_status = bf_switch_cli_clish_get_attr_val_type_list(
          clish_context, attr_name.c_str(), p_attr_md, value_attr);
      switch_attr_type_t elm_type = value_md->type;

      switch (elm_type) {
#if 0
          case SWITCH_TYPE_RANGE:
            {
          cli_status = clish_get_attr_val_type_list_elm_type_range(clish_context, elm_type,
                                                 p_clish_pname_val,
                                                 value->list);
           break;
        }
#endif
        default:
          break;
      }
      break;
    }
    case SWITCH_TYPE_MAC: {
      switch_mac_addr_t mac_addr = {};
      cli_status = bf_switch_cli_clish_get_hex_bytes(
          p_clish_pname_val, &(mac_addr), BF_SWITCH_CLI_MAC_ADDR_SIZE);
      if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                  "Invalid mac address format, status: %s\n",
                                  bf_switch_cli_error_to_string(cli_status));
      }
      value_attr.v_set(mac_addr);
    } break;
    case SWITCH_TYPE_IP_ADDRESS: {
      switch_ip_address_t ip_addr = {};
      cli_status = bf_switch_cli_clish_get_attr_val_type_ip_addr(
          clish_context, attr_name.c_str(), &ip_addr);
      value_attr.v_set(ip_addr);
    } break;
    case SWITCH_TYPE_IP_PREFIX: {
      switch_ip_prefix_t ip_prefix = {};
      cli_status = bf_switch_cli_clish_get_attr_val_type_ip_prefix(
          clish_context, attr_name.c_str(), &ip_prefix);
      value_attr.v_set(ip_prefix);
    } break;
    case SWITCH_TYPE_STRING: {
      switch_string_t switch_string = {};
      switch_strncpy(
          switch_string.text, p_clish_pname_val, SWITCH_MAX_STRING_LEN);
      value_attr.v_set(switch_string);
    } break;
    case SWITCH_TYPE_RANGE: {
      switch_range_t range_value = {};
      sscanf(p_clish_pname_val, "%u-%u", &range_value.min, &range_value.max);
      value_attr.v_set(range_value);
    } break;
    default:
      break;
  }

  return cli_status;
}

/*
 * Input: command string of format - ex: add <object_name>
 */
void bf_switch_cli_get_object_name(char *cmd_str, std::string &object_name) {
  char *save_ptr, *token = NULL;
  char cmd_scratch_buff[MAX_CLISH_MAX_CMD_STR_LEN];

  switch_strncpy(cmd_scratch_buff, cmd_str, MAX_CLISH_MAX_CMD_STR_LEN);
  strtok_r(cmd_scratch_buff, BF_SWITCH_CLI_CMD_DELIMITER, &save_ptr);
  token = strtok(save_ptr, BF_SWITCH_CLI_CMD_DELIMITER);
  if (token) {
    object_name = token;
  }
  return;
}

bool is_attr_input_in_range_format(const char *attr_name, void *clish_context) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg;
  const char *range_val_pname;

  range_val_pname = bf_switch_cli_clish_get_val_pname(attr_name, "_range");
  parg = clish_pargv_find_arg(pargv, range_val_pname);
  if (parg) {
    return true;
  }

  return false;
}

int bf_switch_cli_replay_file_internal(void *clish_context) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);

  const char *filename = "bmai.rec";
  const clish_parg_t *parg = clish_pargv_find_arg(pargv, "filename");
  if (parg) {
    filename = clish_parg__get_value(parg);
  }
  if (!filename) {
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  int stop_on_error = 1;
  parg = clish_pargv_find_arg(pargv, "stop_on_error");
  if (parg) {
    const char *i_str = clish_parg__get_value(parg);
    if (i_str && strcmp(i_str, "0") == 0) {
      stop_on_error = 0;
    }
  }
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "Replaying operations from %s with stop_on_error %d\n",
      filename,
      stop_on_error);
  smi::record::record_file_replay(filename);
  return cli_status;
}

/*
 * get model-info, object-info and attr names
 * for every attr_name, extract data from clish context,
 *  popuate attribute.id and attribute.value.
 * publish attribute into attribute array and create object using
 * bf_switchapi,coo
 */

int bf_switch_cli_add_object_internal(void *clish_context) {
  char *device_str;
  char *cmd_str;
  uint16_t device = 0;
  clish_pargv_t *pargv;
  const clish_parg_t *parg;
  std::string object_name;
  switch_object_id_t object_handle = {0};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> attr_list;

  pargv = clish_context__get_pargv(clish_context);

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  ModelInfo *model_info = switch_store::switch_model_info_get();

  bf_switch_cli_get_object_name(cmd_str, object_name);
  if (object_name.empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%s add cli failed on device %d\n", cmd_str, device);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
  reset_buff_(cmd_str);

  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (!obj_info) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
  /* populate attribute list using clish context */
  for (const auto &attr_md : obj_info->get_attribute_list()) {
    attr_w value_attr(attr_md.attr_id);
    cli_status = bf_switch_cli_clish_get_attr_value(
        clish_context, &attr_md, value_attr, NULL);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      if (cli_status == BF_SWITCH_CLI_STATUS_CLISH_PNAME_NOT_FOUND_E ||
          cli_status == BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E) {
        continue;
      } else {
        BF_SWITCH_CLI_CLISH_PRINT(
            clish_context,
            "%s add cli failed on device %d, status: %s\n",
            object_name.c_str(),
            device,
            bf_switch_cli_error_to_string(cli_status));
        return cli_status;
      }
    }
    attr_list.insert(value_attr);
  }

  /* create object */
  switch_status = switch_store::object_create(
      obj_info->object_type, attr_list, object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s add cli failed on device %d: status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  const char *shell_entry_hdl_buffer_var = "$entry_handle";
  const char *p_entry_hdl;

  parg = clish_pargv_find_arg(pargv, BF_SWITCH_CLI_RET_HANDLE_PNAME);
  if (parg) {
    p_entry_hdl = clish_parg__get_value(parg);
    if (p_entry_hdl) {
      if (p_entry_hdl[0] == '$') {
        shell_entry_hdl_buffer_var = p_entry_hdl;
      } else {
        BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                  "Invalid Shell variable: %s on device %d:"
                                  " (Variable names must start with a $) \n",
                                  p_entry_hdl,
                                  device);
      }
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "Shell variable NULL Pointer on device %d: \n",
                                device);
    }
  }

  if ((char *)0x01 ==
      clish_shell_strmap_get(static_cast<clish_context_t *>(clish_context),
                             shell_entry_hdl_buffer_var)) {
    clish_shell_strmap_insert(static_cast<clish_context_t *>(clish_context),
                              shell_entry_hdl_buffer_var);
  }

  const char *hdl_str = bf_switch_cli_handle_to_string(object_handle);
  clish_shell_strmap_set(static_cast<clish_context_t *>(clish_context),
                         shell_entry_hdl_buffer_var,
                         hdl_str);

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "%s add success device %d: handle: 0x%lx\n",
                            object_name.c_str(),
                            device,
                            object_handle.data);
  return cli_status;
}

int bf_switch_cli_del_object_handle_internal(void *clish_context) {
  char *device_str = NULL;
  uint16_t device = 0;
  // clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  switch_object_id_t object_handle = {};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object handle */
  cli_status = bf_switch_cli_clish_get_attr_type_object_handle(clish_context,
                                                               object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "del cli failed on device: %d, status: %s\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  /* delete object */
  switch_status = switch_store::object_delete(object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "delete handle cli failed on device %d: status: %s\n",
        device,
        bf_switch_cli_error_to_string(cli_status));

    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "delete success on device %d: handle: 0x%lx \n",
                            device,
                            object_handle.data);
  return cli_status;
}

int bf_switch_cli_del_object_internal(void *clish_context) {
  char *device_str = NULL;
  char *cmd_str = NULL;
  uint16_t device = 0;
  std::string object_name;
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  switch_object_id_t object_handle = {};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  (void)pargv;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  bf_switch_cli_get_object_name(cmd_str, object_name);
  if (object_name.empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%s del cli failed on device %d\n", cmd_str, device);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
  reset_buff_(cmd_str);

  /* fetch object id and build handle */
  cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
      clish_context, object_name.c_str(), object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "del cli failed on device: %d, status: %s\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  /* delete object */
  switch_status = switch_store::object_delete(object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "del cli failed on device %d: status: %s\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "delete success on device %d: handle: 0x%lx \n",
                            device,
                            object_handle.data);
  return cli_status;
}

/*
 * set command format:
 *      set <object_name> handle <handle_value> [<optional attr_name>
 * <attr_value> ...]
 *      set <object_name> [<mandatory attr_name> <attr_value>...] [<optional
 * attr_name> <attr_value>...]
 */
int bf_switch_cli_set_object_internal(void *clish_context) {
  char *device_str = NULL;
  char *cmd_str = NULL;
  uint16_t device = 0;
  std::string object_name;
  const char *attr_name;
  switch_object_id_t object_handle = {};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  bf_switch_cli_get_object_name(cmd_str, object_name);
  if (object_name.empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%s set cli failed on device %d\n", cmd_str, device);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
  reset_buff_(cmd_str);

  /*
   * set <object_name> handle <> attribute {[attribute_A <>],...}
   */

  /* chk if obj-handle attr type exists */
  cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
      clish_context, object_name.c_str(), object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    /*
     * handle not configured, derive handle using object key attributes.
     */
    cli_status = bf_switch_cli_lookup_object_handle_by_key(
        clish_context, object_name.c_str(), object_handle);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      cli_status = BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "%s set CLI failed on device: %d, status: %s\n",
                                object_name.c_str(),
                                device,
                                bf_switch_cli_error_to_string(cli_status));
      return cli_status;
    }
  }

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "attribute_sc", attr_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s set CLI failed on device: %d, status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s set CLI failed on device: %d, status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }
  switch_attr_id_t attr_id = obj_info->get_attr_id_from_name(attr_name);
  const AttributeMetadata *attr_md = obj_info->get_attr_metadata(attr_id);
  if (!attr_md) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s set CLI failed on device: %d, invalid "
                              "attribute, status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  attr_w attr(attr_id);
  cli_status =
      bf_switch_cli_clish_get_attr_value(clish_context, attr_md, attr, NULL);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s set CLI failed on device: %d, invalid "
                              "attribute value, status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }
  switch_status = switch_store::attribute_set(object_handle, attr);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s set cli failed on device %d: status: %s\n",
                              object_name.c_str(),
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "%s set success device %d: handle:0x%lx\n",
                            object_name.c_str(),
                            device,
                            object_handle.data);
  return cli_status;
}

void bf_switch_cli_mac_to_hex_string(uint8_t *array, char *string_out) {
  snprintf(string_out,
           BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
           "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
           array[0],
           array[1],
           array[2],
           array[3],
           array[4],
           array[5]);
  return;
}

void bf_switch_cli_attr_value_to_str(const AttributeMetadata *attr_md,
                                     const switch_attribute_t &attr,
                                     std::string &str) {
  ModelInfo *model_info = switch_store::switch_model_info_get();
  switch (attr.value.type) {
    case SWITCH_TYPE_BOOL:
      if (attr.value.booldata) {
        str = "True";
      } else {
        str = "False";
      }
      break;
    case SWITCH_TYPE_UINT8:
      str = fmt::to_string(attr.value.u8);
      break;
    case SWITCH_TYPE_UINT16:
      str = fmt::to_string(attr.value.u16);
      break;
    case SWITCH_TYPE_UINT32:
      str = fmt::to_string(attr.value.u32);
      break;
    case SWITCH_TYPE_UINT64:
      str = fmt::to_string(attr.value.u64);
      break;
    case SWITCH_TYPE_INT64:
      str = fmt::to_string(attr.value.s64);
      break;
    case SWITCH_TYPE_OBJECT_ID: {
      if (attr.value.oid.data == 0) {
        str = "0";
        break;
      }
      const switch_object_type_t ot =
          switch_store::object_type_query(attr.value.oid);
      str = fmt::format(
          "{}.{}",
          model_info->get_object_name_from_type(ot).c_str(),
          fmt::to_string(switch_store::handle_to_id(attr.value.oid)));
      break;
    }
    case SWITCH_TYPE_ENUM: {
      const ValueMetadata *value_md = attr_md->get_value_metadata();
      for (const auto &enums : value_md->get_enum_metadata()) {
        if (enums.enum_value == attr.value.enumdata.enumdata) {
          str = fmt::format("{}", enums.enum_name.c_str());
          break;
        }
      }
      break;
    }
    case SWITCH_TYPE_MAC: {
      uint8_t *array = (uint8_t *)(attr.value.mac.mac);
      str = fmt::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                        array[0],
                        array[1],
                        array[2],
                        array[3],
                        array[4],
                        array[5]);
      break;
    }
    case SWITCH_TYPE_IP_ADDRESS: {
      if (attr.value.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        struct in_addr ip_addr;
        ip_addr.s_addr = htonl(attr.value.ipaddr.ip4);
        str = inet_ntoa(ip_addr);
      } else if (attr.value.ipaddr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6) {
        char ipv6_addr[INET6_ADDRSTRLEN] = {'\0'};
        inet_ntop(AF_INET6, attr.value.ipaddr.ip6, ipv6_addr, INET6_ADDRSTRLEN);
        str = ipv6_addr;
      } else {
        str = "0";
      }
      break;
    }
    case SWITCH_TYPE_IP_PREFIX: {
      if (attr.value.ipprefix.addr.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4) {
        struct in_addr ip_addr;
        uint32_t mask_len = 0;
        ip_addr.s_addr = htonl(attr.value.ipprefix.addr.ip4);
        mask_len = attr.value.ipprefix.len;
        str = inet_ntoa(ip_addr);
        str += fmt::format("/{}", mask_len);
      } else if (attr.value.ipprefix.addr.addr_family ==
                 SWITCH_IP_ADDR_FAMILY_IPV6) {
        char ipv6_str[INET6_ADDRSTRLEN];
        uint32_t mask_len;
        inet_ntop(
            AF_INET6, attr.value.ipprefix.addr.ip6, ipv6_str, INET6_ADDRSTRLEN);
        mask_len = attr.value.ipprefix.len;
        str = fmt::format("{}/{:d}", ipv6_str, mask_len);
      } else {
        str = "0";
      }
    } break;
    case SWITCH_TYPE_STRING:
      str = attr.value.text.text;
      break;
    case SWITCH_TYPE_RANGE:
      str = fmt::format("{}-{}",
                        fmt::to_string(attr.value.range.min),
                        fmt::to_string(attr.value.range.max));
      break;
    default:
      break;
  }
  return;
}

bf_switch_cli_status_t bf_switch_cli_lookup_object_handle_by_key(
    void *clish_context,
    const char *object_name,
    switch_object_id_t &object_handle) {
  bf_switch_cli_status_t cli_status;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t obj_type;
  std::set<attr_w> key_attrs;
  bool kg_found = false;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (obj_info == NULL) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
  obj_type = obj_info->object_type;
  const std::vector<KeyGroup> &key_groups = obj_info->get_key_groups();
  for (auto key_group_list : key_groups) {
    kg_found = true;
    key_attrs.clear();
    for (auto attr_id : key_group_list.attr_list) {
      const AttributeMetadata *attr_md = obj_info->get_attr_metadata(attr_id);
      if (attr_md == NULL) {
        return BF_SWITCH_CLI_STATUS_OBJ_INVALID_KEY;
      }
      /*
       * build key-value set.
       * get attr_value for every key attrs and
       * then get object using key vector
       */
      attr_w attribute(attr_id);
      cli_status = bf_switch_cli_clish_get_attr_value(
          clish_context, attr_md, attribute, NULL);
      if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
        /* check for next key group */
        kg_found = false;
        break;
      }
      key_attrs.insert(attribute);
    }

    if (kg_found) {
      break;
    }
  }

  if (!kg_found) {
    return BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E;
  }

  /*
   * get object using key set.
   */
  switch_status =
      switch_store::object_id_get_wkey(obj_type, key_attrs, object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "handle lookup failed\n");
    return BF_SWITCH_CLI_STATUS_OBJ_INVALID_KEY;
  }

  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

/*
 *  CLI: show vlan <vlan_id> dependencies
 *  output format:
 *  object_type: < >
 *  class: USER
 *  object_handle.   <attr_1>.  <attr_2>. <attr_3>
 *  =================================
 *
 *
 * object_type: < >
 * class: USER
 * object_handle.   <attr_1>.  <attr_2>. <attr_3>
 * =================================
 *
 */

bf_switch_cli_status_t bf_switch_cli_dump_object_dependencies(
    void *clish_context,
    uint16_t device,
    const ObjectInfo *obj_info,
    switch_object_id_t input_object_handle) {
  switch_object_type_t object_type = obj_info->object_type;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  ModelInfo *model_info = switch_store::switch_model_info_get();

  /* get all the object types refering to this object.*/
  for (const auto ref_ot : model_info->inverse_refs_get(object_type)) {
    tbl_view_attrs_t dump_tbl_attrs;

    const ObjectInfo *local_obj_info = model_info->get_object_info(ref_ot);
    if (!local_obj_info) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;

    std::set<switch_object_id_t> ref_oids;
    switch_store::referencing_set_get(input_object_handle, ref_ot, ref_oids);
    if (!ref_oids.size()) continue;

    /* print object_type, class */
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n  object_type: %s",
                              (local_obj_info->get_object_name()).c_str());
    if (local_obj_info->get_object_class() == OBJECT_CLASS_USER) {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n  class: %s", "USER");
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n  class: %s", "AUTO");
    }

    cli_status =
        bf_switch_cli_populate_tbl_attrs(local_obj_info, dump_tbl_attrs);
    /* print header decorator */
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "\n%*s%s",
        DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
        "",
        std::string(dump_tbl_attrs.total_tbl_width, TABLE_HEADER_DECORATOR)
            .c_str());

    /* print first column header */
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%*s", DUMP_OBJECT_ALL_TABLE_COL1_OFFSET, "");

    /* print object_handle */
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "%-*s",
        DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_HANDLE_COLUMN,
        "oid");

    auto it = dump_tbl_attrs.view_attrs.begin();
    /* print per attr, column header */
    for (; it != dump_tbl_attrs.view_attrs.end(); it++) {
      const AttributeMetadata *attr_md =
          local_obj_info->get_attr_metadata(it->attr_id);
      if (!attr_md) return BF_SWITCH_CLI_STATUS_FAILURE_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "%-*s",
                                it->attr_width,
                                (attr_md->get_attr_name()).c_str());
    }

    /* print header decorator */
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "\n%*s%s",
        DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
        "",
        std::string(dump_tbl_attrs.total_tbl_width, TABLE_HEADER_DECORATOR)
            .c_str());

    for (const auto ref_oid : ref_oids) {
      bf_switch_cli_dump_object_attrs_table_fmt(
          clish_context, device, ref_oid, local_obj_info, dump_tbl_attrs);
    }

    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  }

  // BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_dump_object_attrs_table_fmt(
    void *clish_context,
    uint16_t device,
    switch_object_id_t object_handle,
    const ObjectInfo *obj_info,
    tbl_view_attrs_t const &dump_tbl_attrs) {
  switch_attribute_t attr;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  std::string value_str;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  /* print table offset */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%*s", DUMP_OBJECT_ALL_TABLE_COL1_OFFSET, "");

  bool internal_object = false;
  switch_attr_id_t internal_attr_id =
      obj_info->get_attr_id_from_name("internal_object");
  switch_status =
      switch_store::v_get(object_handle, internal_attr_id, internal_object);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show all cli failed on device %d: failed to get "
                              "internal_object (%d)\n",
                              device,
                              switch_status);
  }
  /* print object_handle */
  value_str = fmt::to_string(switch_store::handle_to_id(object_handle)) +
              (internal_object ? "*" : "");
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "%-*s",
      DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_HANDLE_COLUMN,
      value_str.c_str());
  value_str.clear();

  auto it = dump_tbl_attrs.view_attrs.begin();
  for (; it != dump_tbl_attrs.view_attrs.end(); it++) {
    if (it->attr_id == internal_attr_id) continue;
    memset(&attr, 0, sizeof(switch_attribute_t));
    attr_w attr_w_local(it->attr_id);
    switch_status =
        switch_store::attribute_get(object_handle, it->attr_id, attr_w_local);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context,
          "show all cli failed on device %d: Internal (%d)\n",
          device,
          switch_status);
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
    }

    const AttributeMetadata *attr_md = obj_info->get_attr_metadata(it->attr_id);
    if (!attr_md) return BF_SWITCH_CLI_STATUS_FAILURE_E;
    bf_switch_cli_attr_value_to_str(attr_md, attr_w_local.getattr(), value_str);
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%-*s", it->attr_width, value_str.c_str());
    value_str.clear();
  }

  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_dump_object_attributes(
    void *clish_context,
    uint16_t device,
    const ObjectInfo *obj_info,
    switch_object_id_t object_handle) {
  switch_attribute_t attr;
  switch_attr_type_t attr_type;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  std::string value_str;
  switch_status_t switch_status;
  std::vector<AttributeMetadata> attr_type_list;

  for (const auto &p_attr_md : obj_info->get_attribute_list()) {
    attr_type = p_attr_md.type;
    if (!strcmp((p_attr_md.get_attr_name()).c_str(), "device")) {
      continue;
    }
    if (attr_type == SWITCH_TYPE_LIST) {
      attr_type_list.push_back(p_attr_md);
      continue;
    }
    memset(&attr, 0, sizeof(switch_attribute_t));
    attr_w attr_w_local(p_attr_md.attr_id);
    switch_status = switch_store::attribute_get(
        object_handle, p_attr_md.attr_id, attr_w_local);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                " show cli failed on device %d: (%s)\n",
                                device,
                                bf_switch_cli_error_to_string(cli_status));
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
    }
    bf_switch_cli_attr_value_to_str(
        &p_attr_md, attr_w_local.getattr(), value_str);
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s%s: %s",
                              BF_SWITCH_CLI_PRINT_ATTR_MD_TAB,
                              (p_attr_md.get_attr_name()).c_str(),
                              value_str.c_str());
    value_str.clear();
  }

  /*
   * dump list attributes at the end  of attributes.
   * format: example:
   *  lane_list:
   *    element_type: UINT16
   *    list_len:  2
   *    list_elements:
   *       port_id1, port_id2
   */

  for (const auto &p_attr_md : attr_type_list) {
    memset(&attr, 0, sizeof(switch_attribute_t));
    attr_w attr_w_local(p_attr_md.attr_id);
    switch_status = switch_store::attribute_get(
        object_handle, p_attr_md.attr_id, attr_w_local);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                " show cli failed on device %d: (%s)\n",
                                device,
                                bf_switch_cli_error_to_string(cli_status));
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
    }

    switch_attr_type_t list_element_type = attr_w_local.list_type_get();

    /* print list name */
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s%s:",
                              BF_SWITCH_CLI_PRINT_ATTR_MD_TAB,
                              (p_attr_md.get_attr_name()).c_str());
    /* print element_type*/
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s  element_type: %s",
                              BF_SWITCH_CLI_PRINT_ATTR_MD_TAB,
                              bf_switch_cli_attr_type_str(list_element_type));

    /* print list len */
    /*
    size_t list_len;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s  list_len: %d",
                              BF_SWITCH_CLI_PRINT_ATTR_MD_TAB,
                              list_len);
     */

    /* print list_element(s) */
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s  list_element(s):",
                              BF_SWITCH_CLI_PRINT_ATTR_MD_TAB);
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%s    ", BF_SWITCH_CLI_PRINT_ATTR_MD_TAB);
    /* print every element */
    std::string list_string;
    attr_w_local.attr_to_string(list_string);
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%s", list_string.c_str());
  }

  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_dump_object_by_key(
    void *clish_context, uint16_t device, const char *object_name) {
  switch_object_id_t object_handle;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  char value_buf[BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE];
  const char *dump_catlog;
  bool oid_based_dump = false;

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "dump_oid_sc", dump_catlog);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  if (!strcmp(dump_catlog, "handle")) {
    /* oid based input */

    /* fetch object id and build handle */
    cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
        clish_context, object_name, object_handle);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "show cli failed on device: %d, (%s)\n",
                                device,
                                bf_switch_cli_error_to_string(cli_status));
      return cli_status;
    }
    oid_based_dump = true;
  } else { /* key based input */
    cli_status = bf_switch_cli_lookup_object_handle_by_key(
        clish_context, object_name, object_handle);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      cli_status = BF_SWITCH_CLI_STATUS_OBJ_NOT_FOUND_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "%s show CLI failed on device: %d, (%s)\n",
                                object_name,
                                device,
                                bf_switch_cli_error_to_string(cli_status));
      return cli_status;
    }
  }

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show cli failed on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  cli_status = bf_switch_cli_clish_find_kw(clish_context, "dependencies");
  if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    /* "dependencies" option. */
    cli_status = bf_switch_cli_dump_object_dependencies(
        clish_context, device, obj_info, object_handle);
    return cli_status;
  }

  /* print object_handle */
  snprintf(value_buf,
           BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
           "0x%" PRIx64,
           object_handle.data);
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "%s%s_handle: %s",
                            BF_SWITCH_CLI_PRINT_OBJ_MD_TAB,
                            (obj_info->get_object_name()).c_str(),
                            value_buf);
  if (!oid_based_dump) {
    /* print object_id */
    snprintf(value_buf,
             BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
             "%" PRIu64,
             switch_store::handle_to_id(object_handle));
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%sobject_id(oid): %s",
                              BF_SWITCH_CLI_PRINT_OBJ_MD_TAB,
                              value_buf);
  }

  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%sAttributes:", BF_SWITCH_CLI_PRINT_OBJ_MD_TAB);
  cli_status = bf_switch_cli_dump_object_attributes(
      clish_context, device, obj_info, object_handle);
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  return cli_status;
}

bf_switch_cli_status_t show_object_all_helper(void *clish_context,
                                              uint16_t device,
                                              const ObjectInfo *obj_info) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bool next_cycle;
  tbl_view_attrs_t dump_tbl_attrs;
  switch_object_type_t object_type = 0;
  switch_object_id_t object_handle = {0};

  cli_status = bf_switch_cli_populate_tbl_attrs(obj_info, dump_tbl_attrs);

  /* print header decorator */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s",
      DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
      "",
      std::string(dump_tbl_attrs.total_tbl_width, TABLE_HEADER_DECORATOR)
          .c_str());

  /* print first column header */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%*s", DUMP_OBJECT_ALL_TABLE_COL1_OFFSET, "");
  /* print object_handle */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "%-*s",
      DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_HANDLE_COLUMN,
      "oid");

  /* print per attr, column header */
  for (auto it = dump_tbl_attrs.view_attrs.begin();
       it != dump_tbl_attrs.view_attrs.end();
       it++) {
    const AttributeMetadata *attr_md = obj_info->get_attr_metadata(it->attr_id);
    if (!attr_md) return BF_SWITCH_CLI_STATUS_FAILURE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%-*s",
                              it->attr_width,
                              (attr_md->get_attr_name()).c_str());
  }
  /* print header decorator */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s",
      DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
      "",
      std::string(dump_tbl_attrs.total_tbl_width, TABLE_HEADER_DECORATOR)
          .c_str());

  object_type = obj_info->object_type;
  // Uncomment below code to enable internal object filtering from regular
  // bf_switch-view
  // auto internal_obj_attr_id =
  // obj_info->get_attr_id_from_name("internal_object");
  switch_status =
      switch_store::object_get_first_handle(object_type, object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
    /* no handles to dump, return */
    return BF_SWITCH_CLI_STATUS_SUCCESS_E;
  }

  // Uncomment below code to enable internal object filtering from regular
  // bf_switch-view
  // bool internal_object = false;
  // switch_status = switch_store::v_get(object_handle, internal_obj_attr_id,
  // internal_object);
  // if((switch_status == SWITCH_STATUS_SUCCESS) && ((internal_object &&
  // is_internal_view) || (!internal_object))) {
  bf_switch_cli_dump_object_attrs_table_fmt(
      clish_context, device, object_handle, obj_info, dump_tbl_attrs);
  //}
  do {
    std::vector<switch_object_id_t> next_object_handles;
    uint32_t out_num_handles = 0;

    next_cycle = false;
    switch_status = switch_store::object_get_next_handles(
        object_handle,
        BF_SWITCH_CLI_MAX_OBJ_PRINT_PER_CYCLE,
        next_object_handles,
        out_num_handles);
    if (switch_status != SWITCH_STATUS_SUCCESS) {
      /* no handles to dump, return */
      BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
      return BF_SWITCH_CLI_STATUS_SUCCESS_E;
    }

    for (auto it = next_object_handles.begin(); it != next_object_handles.end();
         it++) {
      object_handle = *it;
      // Uncomment below code to enable internal object filtering from regular
      // bf_switch-view
      // switch_status = switch_store::v_get(object_handle,
      // internal_obj_attr_id, internal_object);
      // if((switch_status == SWITCH_STATUS_SUCCESS) && ((internal_object &&
      // is_internal_view) || (!internal_object))) {
      bf_switch_cli_dump_object_attrs_table_fmt(
          clish_context, device, object_handle, obj_info, dump_tbl_attrs);
      //}
    }

    if (next_object_handles.size() == BF_SWITCH_CLI_MAX_OBJ_PRINT_PER_CYCLE) {
      /* need next cycle to finish dump task */
      next_cycle = true;
    }
  } while (next_cycle == true);

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return cli_status;
}

/*
 * show <object_name> all
 *  ex: show vlan all
 *
 *        object_handle  obj_id  vlan_id    attr2    attr3    attr4
 *        ========================================================
 *        0xf00000001       1    10         -         -       -
 *        0xf00000002       2    20         -         -       -
 *        0xf00000003       3    30         -         -       -
 *
 */

int bf_switch_cli_show_object_all_internal(void *clish_context) {
  char *device_str;
  uint16_t device = 0;
  std::string object_name;
  char *cmd_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  // Uncomment below code to enable internal object filtering from regular
  // bf_switch-view
  // clish_shell_t *curr_shell = clish_context__get_shell(clish_context);
  // clish_view_t *curr_view = NULL;
  // const char *curr_view_name = "bf_switch-view";
  // const char *internal_view_name = "bf_switch_internal-view";
  // bool is_internal_view = false;
  // if(curr_shell)
  //  curr_view = clish_shell__get_view(curr_shell);
  // if(curr_view)
  //  curr_view_name = clish_view__get_name(curr_view);
  // if(!strcmp(curr_view_name, internal_view_name))
  //    is_internal_view = true;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  bf_switch_cli_get_object_name(cmd_str, object_name);
  if (object_name.empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%s show all cli failed on device %d\n",
                              cmd_str,
                              device);
    reset_buff_(cmd_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
  reset_buff_(cmd_str);

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show all cli failed on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  cli_status = show_object_all_helper(clish_context, device, obj_info);

  return cli_status;
}

/*
 *  show <object-name> {key_attribute_A <>...}
 *    example:
 *      show vlan {vlan_id 10}
 *      show mac_entry { vlan_handle <> mac_address <>}
 *      show route {vrf <vrf_id> prefix <10.1.1.0/24> }
 */
int bf_switch_cli_show_object_internal(void *clish_context) {
  char *device_str = NULL;
  uint16_t device = 0;
  std::string object_name;
  char *cmd_str = NULL;
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  (void)pargv;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  bf_switch_cli_get_object_name(cmd_str, object_name);
  if (object_name.empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%s show cli failed on device %d\n", cmd_str, device);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }
  reset_buff_(cmd_str);

  if (!strcmp(object_name.c_str(), "object_types")) {
    cli_status = dump_object_types(clish_context, device);
  } else {
    cli_status = bf_switch_cli_dump_object_by_key(
        clish_context, device, object_name.c_str());
  }
  return cli_status;
}

/*
 *  show handle <>
 */
int bf_switch_cli_show_object_handle_internal(void *clish_context) {
  char *device_str = NULL;
  uint16_t device = 0;
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  switch_object_id_t object_handle;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_object_type_t object_type;
  char value_buf[BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE];

  (void)pargv;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null \n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object handle */
  cli_status = bf_switch_cli_clish_get_attr_type_object_handle(clish_context,
                                                               object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show CLI failed on device: %d, status: %s\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  object_type = switch_store::object_type_query(object_handle);
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info = model_info->get_object_info(object_type);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show cli failed on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

#if 0
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n%sobject_name: %s",
                            BF_SWITCH_CLI_PRINT_OBJ_TAB,
                            obj_info->object_name);
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n%sAttributes Count: %d",
                            BF_SWITCH_CLI_PRINT_OBJ_TAB,
                            obj_info->attr_count);
#endif
  /* print object_name */
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "%sobject_type: %s",
                            BF_SWITCH_CLI_PRINT_OBJ_MD_TAB,
                            (obj_info->get_object_name()).c_str());

  /* print object_id */
  snprintf(value_buf,
           BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
           "%" PRIu64,
           switch_store::handle_to_id(object_handle));
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%sobject_id(oid): %s",
                            BF_SWITCH_CLI_PRINT_OBJ_MD_TAB,
                            value_buf);

  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%sAttributes:", BF_SWITCH_CLI_PRINT_OBJ_MD_TAB);

  cli_status = bf_switch_cli_dump_object_attributes(
      clish_context, device, obj_info, object_handle);
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  return cli_status;
}

int bf_switch_cli_add_var_internal(void *clish_context) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = clish_pargv_find_arg(pargv, "varname");

  (void)pargv;

  const char *varname = NULL;
  if (parg) {
    varname = clish_parg__get_value(parg);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Error: the varname argument is required.\n");
    return -1;
  }

  int ret = -1;
  if (varname) {
    if (varname[0] == '$') {
      ret = clish_shell_strmap_insert(
          static_cast<clish_context_t *>(clish_context), varname);
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "Variable names must start with a $\n");
    }
  }
  if (ret == -1) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "Error declaring variable.\n");
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "Variable %s declared.\n", varname);
  }
  return ret;
}

int bf_switch_cli_get_var_internal(void *clish_context) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = NULL;

  const char *varname = NULL;
  parg = clish_pargv_find_arg(pargv, "varname");
  if (parg) {
    varname = clish_parg__get_value(parg);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Error: the varname argument is required.\n");
    return -1;
  }

  char *ret = NULL;
  if (varname) {
    ret = clish_shell_strmap_get(static_cast<clish_context_t *>(clish_context),
                                 varname);
  }
  if (ret == (char *)0x1) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "Error getting variable.\n");
    return -1;
  } else if (ret == NULL) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%s : NULL\n", varname);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%s : %s\n", varname, ret);
  }
  return 1;
}

int bf_switch_cli_set_var_internal(void *clish_context) {
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = NULL;
  int ret = -1;

  const char *varname = NULL;
  const char *value = NULL;
  parg = clish_pargv_find_arg(pargv, "varname");
  if (parg) {
    varname = clish_parg__get_value(parg);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Error: the varname argument is required.\n");
    return -1;
  }

  parg = clish_pargv_find_arg(pargv, "value");
  if (parg) {
    value = clish_parg__get_value(parg);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Error: the value argument is required.\n");
    return -1;
  }

  if (varname && value) {
    ret = clish_shell_strmap_set(
        static_cast<clish_context_t *>(clish_context), varname, value);
  }
  if (ret == -1) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "Error setting variable.\n");
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "Variable %s set successfully.\n", varname);
  }
  return ret;
}

/*
 *  CLI: show object_types
 *  output format:
 *       object_name   description
 *       ============================================
 *       rif           L3 Routed Interface Object
 *       port          port object
 *       vlan          Vlan object
 */

bf_switch_cli_status_t dump_object_types(void *clish_context, uint16_t device) {
  (void)device;
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  ModelInfo *model_info = switch_store::switch_model_info_get();

  /* print table header */
  size_t obj_name_col_width = model_info->object_name_max_len + TABLE_COL_SPACE;
  size_t desc_col_width = OBJECT_DESCRIPTION_TAB;

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-*s%-*s",
                            DESC_OBJECT_OFFSET,
                            "",
                            obj_name_col_width,
                            "object_name",
                            desc_col_width,
                            "description");

  switch_object_class_t object_class_type = OBJECT_CLASS_USER;
  status = bf_switch_cli_clish_find_kw(clish_context, "internal");
  if (status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    object_class_type = OBJECT_CLASS_AUTO;
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%s",
                            DESC_OBJECT_OFFSET,
                            "",
                            OBJECT_TYPES_TABLE_HEADER_DELM);
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    ObjectInfo object_info = *it;
    if (object_info.get_object_class() != object_class_type) {
      continue;
    }

    if ((object_info.get_object_desc()).empty()) {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%*s%-*s%-*s object",
                                DESC_OBJECT_OFFSET,
                                "",
                                obj_name_col_width,
                                (object_info.get_object_name()).c_str(),
                                desc_col_width,
                                (object_info.get_object_name()).c_str());
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%*s%-*s%-*s",
                                DESC_OBJECT_OFFSET,
                                "",
                                obj_name_col_width,
                                (object_info.get_object_name()).c_str(),
                                desc_col_width,
                                (object_info.get_object_desc()).c_str());
    }
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return status;
}

bf_switch_cli_status_t show_table_utilization_helper(void *clish_context,
                                                     const char *table_name) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  switch_table_info_t table_info = {};
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info = model_info->get_object_info_from_name("device");
  if (!obj_info) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
  const AttributeMetadata *attr_md =
      obj_info->get_attr_metadata_from_name("table");
  if (!attr_md) return BF_SWITCH_CLI_STATUS_FAILURE_E;
  const ValueMetadata *value_md = attr_md->get_value_metadata();
  if (strcmp("ALL", table_name) == 0) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%*s%-21s%-10s%-s",
                              TABLE_COL_SPACE,
                              "",
                              "Name",
                              "Size",
                              "Usage");
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%*s%s",
                              TABLE_COL_SPACE,
                              "",
                              std::string(40, TABLE_HEADER_DECORATOR).c_str());
    for (const auto &enums : value_md->get_enum_metadata()) {
      table_info = {};
      switch_status =
          bf_switch::bf_switch_table_info_get(enums.enum_value, table_info);
      if (switch_status == SWITCH_STATUS_INVALID_PARAMETER) continue;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%*s%-21s%-10d%-d",
                                TABLE_COL_SPACE,
                                "",
                                enums.enum_name.c_str(),
                                table_info.table_size,
                                table_info.table_usage);
    }
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
    return BF_SWITCH_CLI_STATUS_SUCCESS_E;
  }
  for (const auto &enums : value_md->get_enum_metadata()) {
    if (strcmp(enums.enum_name.c_str(), table_name) == 0) {
      switch_status =
          bf_switch::bf_switch_table_info_get(enums.enum_value, table_info);
      if (switch_status == SWITCH_STATUS_INVALID_PARAMETER) {
        return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
      }
      break;
    }
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*sTable Size : %zd",
                            DESC_OBJECT_OFFSET,
                            "",
                            table_info.table_size);
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*sUsage      : %d",
                            DESC_OBJECT_OFFSET,
                            "",
                            table_info.table_usage);

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  return cli_status;
}

/*
 *  CLI: show table_utilization <table_name>
 *  The table_name list comes from device object's table attribute
 *  output format:
 *   ex: show table_utilization ALL
 *  Name                 Size      Usage
 *  ========================================
 *  DMAC                 16384     0
 *  IPV4_HOST            65536     0
 *  IPV4_LPM             16384     0
 *   ex: show table_utlization DMAC
 *           Table Size : 16384
 *           Usage      : 0
 */
int bf_switch_cli_show_table_info_internal(void *clish_context) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *table_name;

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "table_name", table_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  cli_status = show_table_utilization_helper(clish_context, table_name);
  return cli_status;
}

/*
 *  CLI: describe object_type <object_name>
 *  output format:
 *   ex: describe object_type vlan
 *         object_name: vlan
 *         object_description:
 *         attributes_count:
 *         Attributes:
 *           name    type    flags    description
 *           =========================================
 *
 */

int bf_switch_cli_describe_object_internal(void *clish_context) {
  bf_switch_cli_status_t status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  char *device_str;
  char *cmd_str;
  uint16_t device = 0;
  const char *object_name;
  const char *attr_name;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  reset_buff_(cmd_str);

  ModelInfo *model_info = switch_store::switch_model_info_get();
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "desc_object_type_sc", object_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "attribute_sc", attr_name);
  if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    /* attribute case. */
    cli_status = bf_switch_cli_describe_object_attribute(
        clish_context, device, object_name, attr_name);
    return cli_status;
  }

  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (!obj_info) return BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*sobject_name: %s (%d)",
                            DESC_OBJECT_OFFSET,
                            "",
                            object_name,
                            obj_info->object_type);
  if ((obj_info->get_object_desc()).empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%*sobject_description: %s object",
                              DESC_OBJECT_OFFSET,
                              "",
                              object_name);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%*sobject_description: %s object",
                              DESC_OBJECT_OFFSET,
                              "",
                              (obj_info->get_object_desc()).c_str());
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*sNumber of attributes: %d",
                            DESC_OBJECT_OFFSET,
                            "",
                            (obj_info->get_attribute_list()).size());
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%*sAttributes:", DESC_OBJECT_OFFSET, "");

  /* print attr-table header */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%*s", DESC_OBJECT_ATTR_TABLE_COL1_OFFSET, "");

  uint32_t attr_name_col_width =
      model_info->attr_name_max_len + TABLE_COL_SPACE;
  uint32_t attr_type_col_width = 9 + TABLE_COL_SPACE;
  uint32_t attr_flag_col_width = 10 + TABLE_COL_SPACE;

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%-*s", attr_name_col_width, "name");
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%-*s", attr_type_col_width, "type");
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "%-*s", attr_flag_col_width, "flags");
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "%s", "description");

  uint32_t total_table_width = attr_name_col_width + attr_type_col_width +
                               attr_flag_col_width + ATTR_DESCRIPTION_SIZE;

  /* print header decorator */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s",
      DESC_OBJECT_ATTR_TABLE_COL1_OFFSET,
      "",
      std::string(total_table_width, TABLE_HEADER_DECORATOR).c_str());

  /* populate attribute list using clish context */
  for (const auto &p_attr_md : obj_info->get_attribute_list()) {
    if (p_attr_md.get_attr_name() == "internal_object") continue;
    if (p_attr_md.get_attr_name() == "id") continue;
    if (p_attr_md.get_attr_name() == "stats") continue;
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%*s", DESC_OBJECT_ATTR_TABLE_COL1_OFFSET, "");
    std::string attr_name_id = p_attr_md.get_attr_name() + " (" +
                               std::to_string(p_attr_md.attr_id) + ")";
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "%-*s", attr_name_col_width, attr_name_id.c_str());
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "%-*s",
                              attr_type_col_width,
                              bf_switch_cli_attr_type_str(p_attr_md.type));
    switch_attr_flags_t flags = p_attr_md.get_flags();
    if (flags.is_mandatory && flags.is_create_only) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "man | crea");
    } else if (flags.is_mandatory) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "mandatory");
    } else if (flags.is_create_only) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "create_only");
    } else if (flags.is_immutable) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "immutable");
    } else if (flags.is_internal) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "internal");
    } else if (flags.is_read_only) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "read_only");
    } else if (flags.is_counter) {
      continue;
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%-*s", attr_flag_col_width, "optional");
    }

    if ((p_attr_md.get_attr_desc()).empty()) {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%s attribute", (p_attr_md.get_attr_name()).c_str());
    } else {
      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "%s", (p_attr_md.get_attr_desc()).c_str());
    }
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return status;
}

/*
 *  describe object <vlan> attribute <attr_name>
 *         attr_name:
 *         description:
 *         attr_type:
 *           default_value:
 *           max_value:
 *           allowed object_types:
 *         attr_flags:
 *
 */
bf_switch_cli_status_t bf_switch_cli_describe_object_attribute(
    void *clish_context,
    uint16_t device,
    const char *object_name,
    const char *attr_name) {
  (void)device;
  switch_attribute_t def_attr;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  std::string value_str;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *obj_info =
      model_info->get_object_info_from_name(object_name);
  if (obj_info == NULL) {
    return cli_status;
  }

  switch_attr_id_t attr_id = obj_info->get_attr_id_from_name(attr_name);
  const AttributeMetadata *p_attr_md = obj_info->get_attr_metadata(attr_id);
  if (p_attr_md == NULL) {
    return cli_status;
  }

  if ((p_attr_md->get_attr_desc()).empty()) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%sdescription: %s ",
                              DESC_OBJECT_ATTR_NAME_TAB,
                              attr_name);
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%sdescription: %s ",
                              DESC_OBJECT_ATTR_NAME_TAB,
                              (p_attr_md->get_attr_desc()).c_str());
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%sdata_type: %s ",
                            DESC_OBJECT_ATTR_NAME_TAB,
                            bf_switch_cli_attr_type_str(p_attr_md->type));
  /* print default value*/
  const ValueMetadata *value_md = p_attr_md->get_value_metadata();
  def_attr.value = value_md->get_default_value();
  bf_switch_cli_attr_value_to_str(p_attr_md, def_attr, value_str);

  if (p_attr_md->type != SWITCH_TYPE_LIST) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%sdefault_value: %s ",
                              DESC_OBJECT_ATTR_NAME_TAB,
                              value_str.c_str());
    value_str.clear();
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%slist type: %s ",
                              DESC_OBJECT_LIST_ATTR_NAME_TAB,
                              bf_switch_cli_attr_type_str(value_md->type));
  }

  /* print enum values for enum_type */
  if (p_attr_md->type == SWITCH_TYPE_ENUM) {
    value_str = "allowed_values: { ";
    auto enums_md = value_md->get_enum_metadata();
    bool first_enum = true;
    for (const auto &enum_md : enums_md) {
      if (first_enum) {
        value_str.append(fmt::format("\"{}\"", enum_md.enum_name.c_str()));
      } else {
        value_str =
            value_str.append(fmt::format(",\"{}\"", enum_md.enum_name.c_str()));
      }
      first_enum = false;
    }
    value_str = value_str + " }";

    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%s%s", DESC_OBJECT_ATTR_NAME_TAB, value_str.c_str());
    value_str.clear();
  }

  if (p_attr_md->type == SWITCH_TYPE_OBJECT_ID) {
    value_str = "allowed_object_types: ";
    bool first_object = true;
    for (const auto &object_type : value_md->get_allowed_object_types()) {
      std::string local_object_name =
          model_info->get_object_name_from_type(object_type);
      if (first_object) {
        value_str =
            value_str + fmt::format("\"{}\"", local_object_name.c_str());
      } else {
        value_str =
            value_str + fmt::format(",\"{}\"", local_object_name.c_str());
      }
      first_object = false;
    }
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%s%s", DESC_OBJECT_ATTR_NAME_TAB, value_str.c_str());
    value_str.clear();
  }

  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%sflags: ", DESC_OBJECT_ATTR_NAME_TAB);

  switch_attr_flags_t flags = p_attr_md->get_flags();
  if (flags.is_mandatory && flags.is_create_only) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "mandatory | create_only");
  } else if (flags.is_mandatory) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "mandatory");
  } else if (flags.is_create_only) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "create_only");
  } else if (flags.is_immutable) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "immutable");
  } else if (flags.is_internal) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "internal");
  } else if (flags.is_read_only) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "read_only");
  } else {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "optional");
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return cli_status;
}

/*
 * range input format: 2-10,50-80,101-200,210,230-500
 *     example: port
 *
 */

bf_switch_cli_status_t bf_switch_cli_parse_range(void *clish_context,
                                                 const char *p_clish_pname_val,
                                                 cli_range *range_out,
                                                 uint32_t &count) {
  uint32_t start = 0, end = 0;
  uint32_t len;
  char input_string[MAX_CLISH_MAX_CMD_STR_LEN];
  const char *range_delm = "-";
  const char *comma_delm = ",";
  char *range_token;

  (void)range_delm;

  len = strlen(p_clish_pname_val) + 1;
  count = 0;
  switch_strncpy(input_string, p_clish_pname_val, len);
  range_token = strtok(input_string, comma_delm);
  while (range_token) {
    end = 0;
    sscanf(range_token, "%u-%u", &start, &end);
    if (end == 0) {
      end = start;
    }

    if (end < start) {
      /* invalid input */
    }

    if (chk_overlap_range(clish_context, range_out, count, start, end) !=
        BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      return BF_SWITCH_CLI_STATUS_OVERLAP_RANGE_ERROR_E;
    }
    range_out[count].start = start;
    range_out[count].end = end;
    count++;
    if (count >= MAX_CLI_RANGE) {
      return BF_SWITCH_CLI_STATUS_RANGE_EXCEEDS_MAX_LIMIT_E;
    }
    range_token = strtok(NULL, comma_delm);
  }

  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

bf_switch_cli_status_t chk_overlap_range(const void *clish_context,
                                         cli_range *curr_range,
                                         uint32_t count,
                                         uint32_t start,
                                         uint32_t end) {
  uint32_t itr = 0;
  bf_switch_cli_status_t cli_status;

  for (; itr < count; itr++) {
    if (((start >= curr_range[itr].start) && (start <= curr_range[itr].end)) ||
        ((end >= curr_range[itr].start) && (end <= curr_range[itr].end))) {
      cli_status = BF_SWITCH_CLI_STATUS_OVERLAP_RANGE_ERROR_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "parsing range input failed: (%s)\n",
                                bf_switch_cli_error_to_string(cli_status));
      return cli_status;
    }
  }
  return BF_SWITCH_CLI_STATUS_SUCCESS_E;
}

bf_switch_cli_status_t bf_switch_cli_scan_and_trim_tbl_attr_list(
    const ObjectInfo *obj_info,
    std::vector<switch_attr_id_t> &tbl_view_attr_ids,
    tbl_view_attrs_t &dump_tbl_attrs) {
  uint32_t total_col_width = DUMP_OBJECT_RESERVED_COL_WIDTH;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  uint32_t col_width = 0;
  std::vector<tbl_view_attr_info_t> &attrs = dump_tbl_attrs.view_attrs;
  std::vector<switch_attr_id_t>::iterator it;

  for (it = tbl_view_attr_ids.begin(); it != tbl_view_attr_ids.end(); it++) {
    switch_attr_id_t attr_id = *it;
    const AttributeMetadata *p_attr_md = obj_info->get_attr_metadata(attr_id);
    if (!p_attr_md) return BF_SWITCH_CLI_STATUS_FAILURE_E;
    cli_status = bf_switch_cli_get_attr_col_width(p_attr_md, col_width);
    if (cli_status == BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E) {
      continue;
    }

    auto len = (p_attr_md->get_attr_name()).size() + TABLE_COL_SPACE;
    if (len > col_width) {
      col_width = len;
    }
    total_col_width += col_width;
    if (total_col_width > TABLE_MAX_WIDTH) {
      total_col_width -= col_width;
      break;
    }
    tbl_view_attr_info_t attr_info = {};
    attr_info.attr_id = attr_id;
    attr_info.attr_width = col_width;
    attrs.push_back(attr_info);
  }

  dump_tbl_attrs.total_tbl_width = total_col_width;

  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_populate_tbl_attrs(
    const ObjectInfo *obj_info, tbl_view_attrs_t &dump_tbl_attrs) {
  std::vector<switch_attr_id_t> tbl_view_attr_ids;
  bf_switch_cli_status_t cli_status;

  const CliMetadata *cli_md = obj_info->get_cli_metadata();
  if ((cli_md->get_table_view_attr_list()).size() > 0) {
    for (const auto &table_view_attr_id : cli_md->get_table_view_attr_list()) {
      tbl_view_attr_ids.push_back(table_view_attr_id);
    }
  } else {
    for (const auto &p_attr_md : obj_info->get_attribute_list()) {
      if (!strcmp((p_attr_md.get_attr_name()).c_str(), "device")) {
        continue;
      }
      if (!strcmp((p_attr_md.get_attr_name()).c_str(), "internal_object")) {
        continue;
      }
      tbl_view_attr_ids.push_back(p_attr_md.attr_id);
    }
  }

  cli_status = bf_switch_cli_scan_and_trim_tbl_attr_list(
      obj_info, tbl_view_attr_ids, dump_tbl_attrs);
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_get_attr_col_width(
    const AttributeMetadata *p_attr_md, uint32_t &col_width) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  switch (p_attr_md->type) {
    case SWITCH_TYPE_BOOL:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_BOOL_COLUMN;
      break;
    case SWITCH_TYPE_UINT8:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT8_COLUMN;
      break;
    case SWITCH_TYPE_UINT16:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT16_COLUMN;
      break;
    case SWITCH_TYPE_UINT32:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT32_COLUMN;
      break;
    case SWITCH_TYPE_UINT64:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT64_COLUMN;
      break;
    case SWITCH_TYPE_INT64:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_UINT32_COLUMN;
      break;
    case SWITCH_TYPE_OBJECT_ID:
      col_width = p_attr_md->get_attr_name().size();
      // col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_OBJECT_HANDLE_COLUMN;
      break;
    case SWITCH_TYPE_ENUM: {
      const ValueMetadata *value_md = p_attr_md->get_value_metadata();
      size_t attr_name_size = p_attr_md->get_attr_name().size();
      col_width = value_md->get_max_enum_width() > attr_name_size
                      ? value_md->get_max_enum_width()
                      : attr_name_size;
      col_width += TABLE_COL_SPACE;
    } break;
    case SWITCH_TYPE_MAC:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_MAC_ADDR_COLUMN;
      break;
    case SWITCH_TYPE_IP_ADDRESS:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_IP_ADDRESS_COLUMN;
      break;
    case SWITCH_TYPE_IP_PREFIX:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_IP_PREFIX_COLUMN;
      break;
    case SWITCH_TYPE_STRING:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_STRING_COLUMN;
      break;
    case SWITCH_TYPE_RANGE:
      col_width = DUMP_OBJECT_ALL_TABLE_ATTR_TYPE_RANGE_COLUMN;
      break;
    default:
      col_width = 0;
      cli_status = BF_SWITCH_CLI_STATUS_ATTR_TYPE_NOT_SUPPORTED_E;
      break;
  }

  return cli_status;
}

/*
 *  cli: log_level show
 */
int bf_switch_cli_log_level_show_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  ModelInfo *model_info = switch_store::switch_model_info_get();

  const std::unordered_map<switch_object_type_t, switch_verbosity_t> &log_map =
      get_log_level_all_objects();

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-25s%-10s",
                            TABLE_COL_SPACE,
                            "",
                            "Object Type",
                            "Log Level");
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%s",
                            TABLE_COL_SPACE,
                            "",
                            std::string(40, TABLE_HEADER_DECORATOR).c_str());
  for (const auto &it : log_map) {
    const ObjectInfo *obj_info = model_info->get_object_info(it.first);
    if (obj_info) {
      if (obj_info->get_object_class() == OBJECT_CLASS_AUTO) continue;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%*s%-25s%-10s",
                                TABLE_COL_SPACE,
                                "",
                                obj_info->get_object_name().c_str(),
                                switch_verbosity_to_string(it.second));
    }
  }

  switch_verbosity_t default_verbosity = get_log_level();
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%s",
                            TABLE_COL_SPACE,
                            "",
                            std::string(40, '-').c_str());
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-25s%-s",
                            TABLE_COL_SPACE,
                            "",
                            "Global Log level",
                            switch_verbosity_to_string(default_verbosity));
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  bfshell_string_free(device_str);
  return cli_status;
}

void show_version_helper(void *clish_context) {
  std::string sde_version, sai_version;

  bf_switch::bf_switch_version_get(sde_version, sai_version);

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-25s%-s",
                            TABLE_COL_SPACE,
                            "",
                            "SDE: ",
                            sde_version.c_str());
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-25s%-s",
                            TABLE_COL_SPACE,
                            "",
                            "SAI: ",
                            sai_version.c_str());
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return;
}
/*
 *  cli: show version
 */
int bf_switch_cli_show_version_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  bfshell_string_free(device_str);
  show_version_helper(clish_context);
  return cli_status;
}

/*
 *  cli: show cpu_rx counter
 */
int bf_switch_cli_show_cpu_rx_counter_internal(void *clish_context) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  std::vector<std::string> reason_code_str;
  ModelInfo *model_info = switch_store::switch_model_info_get();

  const ObjectInfo *obj_info =
      model_info->get_object_info(SWITCH_OBJECT_TYPE_HOSTIF_TRAP);
  if (obj_info == NULL) {
    return cli_status;
  }

  // External Trap
  switch_attr_id_t attr_id = SWITCH_HOSTIF_TRAP_ATTR_TYPE;
  const AttributeMetadata *p_attr_md = obj_info->get_attr_metadata(attr_id);
  if (p_attr_md == NULL) {
    return cli_status;
  }

  const ValueMetadata *value_md = p_attr_md->get_value_metadata();
  if (p_attr_md->type != SWITCH_TYPE_ENUM) {
    return cli_status;
  }
  auto enums_md = value_md->get_enum_metadata();
  for (const auto &enum_md : enums_md) {
    reason_code_str.push_back(enum_md.enum_name.c_str());
  }

  // Internal Trap
  attr_id = SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP;
  p_attr_md = obj_info->get_attr_metadata(attr_id);
  if (p_attr_md == NULL) {
    return cli_status;
  }

  value_md = p_attr_md->get_value_metadata();
  if (p_attr_md->type != SWITCH_TYPE_ENUM) {
    return cli_status;
  }
  enums_md = value_md->get_enum_metadata();
  for (const auto &enum_md : enums_md) {
    reason_code_str.push_back(enum_md.enum_name.c_str());
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-36s%-10s%-s",
                            TABLE_COL_SPACE,
                            "",
                            "Reason code",
                            "Packets",
                            "Bytes");
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%s",
                            TABLE_COL_SPACE,
                            "",
                            std::string(51, TABLE_HEADER_DECORATOR).c_str());

  for (size_t i = 0; i < reason_code_str.size(); i++) {
    uint64_t pkts;
    uint64_t bytes;

    switch_status_t status =
        switch_pktdriver_reason_code_stats_get(0, i, &pkts, &bytes);
    if (status == SWITCH_STATUS_SUCCESS) {
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%*s%2d: %-32s%-10u%-u",
                                TABLE_COL_SPACE,
                                "",
                                i,
                                reason_code_str[i].c_str(),
                                pkts,
                                bytes);
    }
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  return cli_status;
}

/*
 *  cli: clear cpu_rx counter
 */
int bf_switch_cli_clear_cpu_rx_counter_internal(void *clish_context) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  for (auto i = 0; i < SWITCH_HOSTIF_TRAP_ATTR_TYPE_MAX +
                           SWITCH_HOSTIF_TRAP_ATTR_INTERNAL_TRAP_MAX;
       i++) {
    switch_pktdriver_reason_code_stats_clear(0, i);
  }
  return cli_status;
}

/*
 *  cli: log_level packet [on | off]
 */
int bf_switch_cli_packet_log_level_set_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *debug_packet_target;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  /* check for log_level target type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "debug_packet_sc", debug_packet_target);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse packet target type, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  if (!strcmp(debug_packet_target, "on")) {
    switch_pkt_dump_enable(true);
  } else if (!strcmp(debug_packet_target, "off")) {
    switch_pkt_dump_enable(false);
  }

  bfshell_string_free(device_str);
  return cli_status;
}

/*
 *  cli: log_level bfd [on | off]
 */
int bf_switch_cli_bfd_log_level_set_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *debug_packet_target;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  /* check for log_level target type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "debug_packet_sc", debug_packet_target);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse packet target type, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  if (!strcmp(debug_packet_target, "on")) {
    switch_bfdd_dump_enable(true);
  } else if (!strcmp(debug_packet_target, "off")) {
    switch_bfdd_dump_enable(false);
  }

  bfshell_string_free(device_str);
  return cli_status;
}

/*
 *  cli: show pkt_path port handle <>
 */
int bf_switch_cli_pkt_path_counter_print_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *object_name;
  switch_object_id_t object_handle = {};

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  /* check for log_level target type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "debug_pkt_path_sc", object_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse object type, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  /* fetch object id and build handle */
  cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
      clish_context, object_name, object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "pkt_path_counter cli failed on device: %s, "
                              "object type %s status: %s\n",
                              device_str,
                              object_name,
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return cli_status;
  }

  std::string data = switch_store::object_pkt_path_counter_print(object_handle);

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n%s\n", data.c_str());

  bfshell_string_free(device_str);
  return cli_status;
}

static int get_feature_name_column_width() {
  int max_width = 0;
  for (int feature = SWITCH_FEATURE_MIN; feature <= SWITCH_FEATURE_MAX;
       feature++) {
    auto &&feature_name = bf_switch_cli_feature_name_str(
        static_cast<switch_feature_id_t>(feature));
    int len = feature_name.length();
    if (len > max_width) max_width = len;
  }
  return max_width;
}

bf_switch_cli_status_t show_features_helper(void *clish_context,
                                            const char *feature_filter) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const std::string all = "all";
  const std::string enabled = "enabled";
  const std::string disabled = "disabled";

  auto FEATURE_NAME_COL_WIDTH =
      get_feature_name_column_width() + TABLE_COL_SPACE;
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\n%*s%-*s%-10s",
                            TABLE_COL_SPACE,
                            "",
                            FEATURE_NAME_COL_WIDTH,
                            "Feature",
                            "Status");
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s",
      TABLE_COL_SPACE,
      "",
      std::string(TABLE_COL_SPACE + FEATURE_NAME_COL_WIDTH + 10,
                  TABLE_HEADER_DECORATOR)
          .c_str());

  for (int feature = SWITCH_FEATURE_MIN; feature < SWITCH_FEATURE_MAX;
       feature++) {
    auto feature_enabled = bf_switch::bf_switch_is_feature_enabled(
        static_cast<switch_feature_id_t>(feature));
    auto &&feature_name = bf_switch_cli_feature_name_str(
        static_cast<switch_feature_id_t>(feature));

    if (!all.compare(feature_filter) ||
        (!disabled.compare(feature_filter) && !feature_enabled) ||
        (!enabled.compare(feature_filter) && feature_enabled)) {
      if (!feature_name.compare(UNKNOWN_FEATURE)) {
        BF_SWITCH_CLI_CLISH_PRINT(
            clish_context,
            "\n%*s(%d)%-*s%-10s",
            TABLE_COL_SPACE,
            "",
            feature,
            FEATURE_NAME_COL_WIDTH,
            feature_name.c_str(),
            feature_enabled ? enabled.c_str() : disabled.c_str());
      } else {
        BF_SWITCH_CLI_CLISH_PRINT(
            clish_context,
            "\n%*s%-*s%-10s",
            TABLE_COL_SPACE,
            "",
            FEATURE_NAME_COL_WIDTH,
            feature_name.c_str(),
            feature_enabled ? enabled.c_str() : disabled.c_str());
      }
    }
  }
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");

  return cli_status;
}

/*
 *  cli: show features [enabled | disabled | all]
 */
int bf_switch_cli_show_features_internal(void *clish_context) {
  char *device_str = nullptr;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *feature_filter = nullptr;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  /* check for log_level target type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "feature_filter", feature_filter);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E ||
      feature_filter == nullptr) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse featulre filter, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  bfshell_string_free(device_str);
  cli_status = show_features_helper(clish_context, feature_filter);
  return cli_status;
}

/*
 *  cli: log_level <log_level> [global | object_type <object type>]
 */
int bf_switch_cli_debug_log_level_set_internal(void *clish_context) {
  char *device_str;
  const char *object_name;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *p_log_level;
  switch_verbosity_t log_level;
  const char *debug_log_level_target;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  ModelInfo *model_info = switch_store::switch_model_info_get();

  /* fetch log_level */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "log_level", p_log_level);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "log_level parse failed, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  log_level = static_cast<switch_verbosity_t>(strtoul(p_log_level, NULL, 0));

  /* check for log_level target type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "debug_log_level_sc", debug_log_level_target);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse log_level target type, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  if (!strcmp(debug_log_level_target, "global")) {
    set_log_level(log_level);
  } else if (!strcmp(debug_log_level_target, "object_type")) {
    cli_status = bf_switch_cli_clish_get_pname_value(
        clish_context, "debug_log_level_object_type_sc", object_name);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "failed to parse object_type, %s \n",
                                bf_switch_cli_error_to_string(cli_status));
      bfshell_string_free(device_str);
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
    }
    if (!strcmp(object_name, "all")) {
      set_log_level_all_objects(log_level);
    } else {
      const ObjectInfo *obj_info =
          model_info->get_object_info_from_name(object_name);
      if (!obj_info) {
        cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
        BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
        return cli_status;
      }
      switch_object_type_t object_type = obj_info->object_type;
      set_log_level_object(object_type, log_level);
    }
  }

  bfshell_string_free(device_str);
  return cli_status;
}

bf_switch_cli_status_t smi_cli_operation_to_switch_operation(
    smi_cli_operation_type_t cli_operation,
    switch_operation_t &switch_operation) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch (cli_operation) {
    case SMI_CLI_CREATE_OPERATION:
      switch_operation = SMI_CREATE_OPERATION;
      break;
    case SMI_CLI_DELETE_OPERATION:
      switch_operation = SMI_DELETE_OPERATION;
      break;
    case SMI_CLI_GET_OPERATION:
      switch_operation = SMI_GET_OPERATION;
      break;
    case SMI_CLI_SET_OPERATION:
      switch_operation = SMI_SET_OPERATION;
      break;
    default:
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
  }

  return cli_status;
}

/*
 *  cli: debug object_operation <operation type> [enable | disable]
 */
int bf_switch_cli_debug_operation_set_internal(void *clish_context) {
  char *device_str;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *p_operation_type;
  const char *p_operation_toggle_type;
  smi_cli_operation_type_t cli_operation;
  bool isEnabled = false;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  /* fetch log_level */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "operation_type", p_operation_type);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "operation_type parse failed, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  cli_operation =
      static_cast<smi_cli_operation_type_t>(strtoul(p_operation_type, NULL, 0));
  /* check for operation toggle type */
  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "operation_toggle_select", p_operation_toggle_type);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "failed to parse opearation, %s \n",
                              bf_switch_cli_error_to_string(cli_status));
    bfshell_string_free(device_str);
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  if (!strcmp(p_operation_toggle_type, "enable")) {
    isEnabled = true;
  } else if (!strcmp(p_operation_toggle_type, "disable")) {
    isEnabled = false;
  }

  if (cli_operation == SMI_CLI_ALL_OPERATION) {
    toggle_all_operations(isEnabled);
  } else {
    switch_operation_t switch_operation;

    cli_status =
        smi_cli_operation_to_switch_operation(cli_operation, switch_operation);
    if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
      cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "failed to fetch opearation_type, %s \n",
                                bf_switch_cli_error_to_string(cli_status));
      return BF_SWITCH_CLI_STATUS_FAILURE_E;
    }
    toggle_operation(switch_operation, isEnabled);
  }

  bfshell_string_free(device_str);
  return cli_status;
}

/* print this
  ========================================================================
  Counter ID                    Count                 Delta
  ========================================================================
*/
void bf_switch_cli_print_counter_header(void *clish_context) {
  size_t total_col_width;
  total_col_width =
      DUMP_COUNTER_TBL_COUNTER_ID_COL_DEF_WIDTH + DUMP_COUNTER_PKT_COL_WIDTH;

  /* print tbl header decorator */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s",
      TABLE_COL_SPACE,
      "",
      std::string(total_col_width, TABLE_HEADER_DECORATOR).c_str());

  std::vector<std::pair<const char *, int>> counter_tbl_hdr_spec;
  counter_tbl_hdr_spec.push_back(
      std::make_pair("Count", DUMP_COUNTER_TBL_PKT_COUNT_COL));
  counter_tbl_hdr_spec.push_back(
      std::make_pair("Delta", DUMP_COUNTER_TBL_PKTS_CHANGED_COUNT_COL));
  counter_tbl_hdr_spec.push_back(
      std::make_pair("Counter ID", DUMP_COUNTER_TBL_COUNTER_ID_COL_DEF_WIDTH));

  /* print first column header */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\n%*s", DUMP_OBJECT_ALL_TABLE_COL1_OFFSET, "");

  for (const auto &tbl_col_md : counter_tbl_hdr_spec) {
    BF_SWITCH_CLI_CLISH_TBL_COL_PRINT(
        clish_context, tbl_col_md.second, tbl_col_md.first);
  }

  /* print tbl footer decorator */
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context,
      "\n%*s%s\n",
      DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
      "",
      std::string(total_col_width, TABLE_HEADER_DECORATOR).c_str());
  return;
}

/*
 * counter table format.
 *
 *   Counter ID      Count        Delta
 *   =====================================
 *    1              10000        +2
 *
 * Notes:
 *   skip zero conters.
 *   display counters that only changed since last query
 *
 */

bf_switch_cli_status_t bf_switch_cli_read_and_dump_counter(
    void *clish_context,
    uint16_t device,
    switch_object_id_t object_handle,
    const char *object_name) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  char value_buf[BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE];
  std::string counter_name;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t object_type = 0;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  object_type = switch_store::object_type_query(object_handle);
  const ObjectInfo *obj_info = model_info->get_object_info(object_type);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Show cli failed on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  if (!obj_info->get_counter()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "counter %s\n",
        bf_switch_cli_error_to_string(BF_SWITCH_CLI_STATUS_NOT_SUPPORTED));
    return cli_status;
  }

  /* fetch counter object data */
  std::vector<switch_cli_api_counter> counters;
  switch_status = bf_switch_cli_api_counters_get(object_handle, counters);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "counter_get cli failed on device %d: status: %s\n",
        device,
        bf_switch_cli_error_to_string(cli_status));
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  // print port handle header only if there is atleast 1 non-zero counter
  uint64_t sum = 0;
  for (const auto &api_counter : counters)
    sum += api_counter.switch_counter.count;
  if (sum > 0) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n  %s.%d",
                              object_name,
                              switch_store::handle_to_id(object_handle));
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "\n%*s%s",
        DUMP_OBJECT_ALL_TABLE_COL1_OFFSET,
        "",
        std::string(DUMP_COUNTER_TBL_COUNTER_ID_COL_DEF_WIDTH +
                        DUMP_COUNTER_PKT_COL_WIDTH,
                    '-')
            .c_str());
  } else {
    return cli_status;
  }

  for (const auto &api_counter : counters) {
    auto &counter = api_counter.switch_counter;
    /* skip zero counters */
    if (counter.count == 0) continue;

    for (const auto &attr_md : obj_info->get_attribute_list()) {
      bool done = false;
      switch_attr_flags_t flags = attr_md.get_flags();
      if (flags.is_counter == false) continue;
      const ValueMetadata *value_md = attr_md.get_value_metadata();
      for (const auto &enums : value_md->get_enum_metadata()) {
        if (enums.enum_value == counter.counter_id) {
          counter_name = enums.enum_name;
          done = true;
          break;
        }
      }
      if (done) break;
    }

    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context, "\n%*s", DUMP_OBJECT_ALL_TABLE_COL1_OFFSET, "");

    /*
     * print counter_type
     */
    /* print pkt_counter */
    BF_SWITCH_CLI_CLISH_TBL_COL_PRINT(clish_context,
                                      DUMP_COUNTER_TBL_PKT_COUNT_COL,
                                      formatNumber(counter.count).c_str());

    /* print packet_counters since last executed cli */
    snprintf(value_buf,
             BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
             "+%s",
             formatNumber(api_counter.delta).c_str());
    BF_SWITCH_CLI_CLISH_TBL_COL_PRINT(
        clish_context, DUMP_COUNTER_TBL_PKTS_CHANGED_COUNT_COL, value_buf);
    BF_SWITCH_CLI_CLISH_TBL_COL_PRINT(
        clish_context, counter_name.size(), counter_name.c_str());
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_dump_counter_by_handle(
    void *clish_context, uint16_t device, const char *object_name) {
  switch_object_id_t object_handle = {};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  /* fetch object id and build handle */
  cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
      clish_context, object_name, object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_HANDLE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show counter cli failed on device: %d, (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  bf_switch_cli_print_counter_header(clish_context);
  cli_status = bf_switch_cli_read_and_dump_counter(
      clish_context, device, object_handle, object_name);
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_dump_counter_all(void *clish_context,
                                                      uint16_t device,
                                                      const char *object_name) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *object_info =
      model_info->get_object_info_from_name(object_name);
  if (!object_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Invalid object name on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  std::vector<switch_object_id_t> object_handles;
  switch_status = switch_store::object_get_all_handles(object_info->object_type,
                                                       object_handles);
  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Invalid object type on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }
  bf_switch_cli_print_counter_header(clish_context);
  for (const auto object_handle : object_handles) {
    cli_status = bf_switch_cli_read_and_dump_counter(
        clish_context, device, object_handle, object_name);
  }
  return cli_status;
}

/*
 * CLI format:
 *  show counter <object_type> [ <object_key> | oid <> ]
 *  show counter <object_handle>
 */
int bf_switch_cli_show_counter_internal(void *clish_context) {
  char *device_str = NULL;
  uint16_t device;
  const char *object_name;
  char *cmd_str = NULL;
  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *all_or_handle;

  (void)pargv;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\n");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  reset_buff_(cmd_str);

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "dump_counter_sc", object_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "dump_oid_sc", all_or_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  if (!strcmp(all_or_handle, "all")) {
    cli_status =
        bf_switch_cli_dump_counter_all(clish_context, device, object_name);
  } else if (!strcmp(all_or_handle, "handle")) {
    cli_status = bf_switch_cli_dump_counter_by_handle(
        clish_context, device, object_name);
  }

  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clear_object_counter(
    void *clish_context, uint16_t device, switch_object_id_t object_handle) {
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  switch_object_type_t object_type;

  ModelInfo *model_info = switch_store::switch_model_info_get();
  object_type = switch_store::object_type_query(object_handle);
  const ObjectInfo *obj_info = model_info->get_object_info(object_type);
  if (!obj_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Clear cli failed on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  if (!obj_info->get_counter()) {
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "counter %s\n",
        bf_switch_cli_error_to_string(BF_SWITCH_CLI_STATUS_NOT_SUPPORTED));
    return cli_status;
  }

  /* fetch counter object data */
  switch_status = switch_store::object_counters_clear_all(object_handle);
  if (switch_status != SWITCH_STATUS_SUCCESS) {
    cli_status = bf_switch_cli_switch_status_to_cli_status(switch_status);
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "clear counter cli failed on device %d: status: %s\n",
        device,
        bf_switch_cli_error_to_string(cli_status));
    return BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  bf_switch_cli_api_notify_counter_event(COUNTER_EVENT_TYPE_CLEAR,
                                         object_handle);
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clear_counter_all(
    void *clish_context, uint16_t device, const char *object_name) {
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  ModelInfo *model_info = switch_store::switch_model_info_get();
  const ObjectInfo *object_info =
      model_info->get_object_info_from_name(object_name);
  if (!object_info) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Invalid object name on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  std::vector<switch_object_id_t> object_handles;
  switch_status = switch_store::object_get_all_handles(object_info->object_type,
                                                       object_handles);
  if (switch_status == SWITCH_STATUS_ITEM_NOT_FOUND) {
    cli_status = BF_SWITCH_CLI_STATUS_INVALID_OBJ_TYPE_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "Invalid object type on device %d: (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }
  for (const auto object_handle : object_handles) {
    cli_status = bf_switch_cli_clear_object_counter(
        clish_context, device, object_handle);
  }
  return cli_status;
}

bf_switch_cli_status_t bf_switch_cli_clear_counter_by_handle(
    void *clish_context, uint16_t device, const char *object_name) {
  switch_object_id_t object_handle = {};
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;

  /* fetch object id and build handle */
  cli_status = bf_switch_cli_clish_fetch_oid_and_build_handle(
      clish_context, object_name, object_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_INPUT_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "show counter cli failed on device: %d, (%s)\n",
                              device,
                              bf_switch_cli_error_to_string(cli_status));
    return cli_status;
  }

  cli_status =
      bf_switch_cli_clear_object_counter(clish_context, device, object_handle);
  return cli_status;
}

/*
 * CLI format:
 *  clear counter <object_handle>
 *  clear counter <object_type> [object_key | oid <>]
 */
int bf_switch_cli_clear_counter_internal(void *clish_context) {
  char *device_str = NULL;
  uint16_t device;
  const char *object_name;
  char *cmd_str = NULL;
  // clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  const char *all_or_handle;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null\ns");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  /* fetch object_name from cmd string */
  cmd_str = (char *)clish_shell_expand_var_ex(
      "_full_cmd",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_CONTEXT);
  if (!cmd_str) {
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "_full cmd string null\n");
    return 0;
  }

  reset_buff_(cmd_str);

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "clear_counter_sc", object_name);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  cli_status = bf_switch_cli_clish_get_pname_value(
      clish_context, "clear_oid_sc", all_or_handle);
  if (cli_status != BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    return cli_status;
  }

  if (!strcmp(all_or_handle, "all")) {
    cli_status =
        bf_switch_cli_clear_counter_all(clish_context, device, object_name);
  } else if (!strcmp(all_or_handle, "handle")) {
    /* object_handle based input */
    cli_status = bf_switch_cli_clear_counter_by_handle(
        clish_context, device, object_name);
  }

  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "clear counter success on device %d: \n", device);

  return cli_status;
}

/*
 *  cli: show tech-support
 */
int bf_switch_cli_show_tech_support_internal(void *clish_context) {
  char *device_str;
  uint16_t device = 0;
  bf_switch_cli_status_t cli_status = BF_SWITCH_CLI_STATUS_SUCCESS_E;
  switch_status_t switch_status = SWITCH_STATUS_SUCCESS;

  device_str = (char *)clish_shell_expand_var_ex(
      "device_id",
      static_cast<clish_context_t *>(clish_context),
      SHELL_EXPAND_VIEW);
  if (!device_str) {
    cli_status = BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "device id null");
    return BF_SWITCH_CLI_STATUS_CLISH_INV_DEVICE_ID_E;
  }

  device = atoi(device_str);
  reset_buff_(device_str);

  // show version
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\nVersion\n=======\n");
  show_version_helper(clish_context);

  // show table_utilization ALL
  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "\nTable Utilization\n=================\n");
  cli_status = show_table_utilization_helper(clish_context, "ALL");

  // show cpu_rx counter
  BF_SWITCH_CLI_CLISH_PRINT(
      clish_context, "\nCPU packet Rx counters\n======================\n");
  bf_switch_cli_show_cpu_rx_counter_internal(clish_context);

  // show features all
  BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\nFeatures\n========\n");
  cli_status = show_features_helper(clish_context, "all");

  ModelInfo *model_info = switch_store::switch_model_info_get();
  for (auto obj = model_info->begin(); obj != model_info->end(); obj++) {
    ObjectInfo &obj_info = *obj;
    switch_object_type_t object_type = obj_info.object_type;

    if (object_type == 0) continue;
    if (obj_info.get_object_class() == OBJECT_CLASS_AUTO) continue;

    std::vector<switch_object_id_t> object_handles;
    switch_status =
        switch_store::object_get_all_handles(object_type, object_handles);
    if (switch_status != SWITCH_STATUS_SUCCESS) continue;
    if (object_handles.size() == 0) continue;

    std::string obj_name_fqn = obj_info.get_object_name_fqn();
    BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                              "\n%s\n%s\n%s\n",
                              std::string(obj_name_fqn.length(), '=').c_str(),
                              obj_name_fqn.c_str(),
                              std::string(obj_name_fqn.length(), '=').c_str());

    // show object_type all
    BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\nBrief view\n==========\n");
    cli_status = show_object_all_helper(clish_context, device, &obj_info);

    // show object_type handle <>
    BF_SWITCH_CLI_CLISH_PRINT(
        clish_context,
        "\nDetailed per object view\n========================\n");
    for (auto &object_handle : object_handles) {
      char value_buf[BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE];

      /* print object_handle */
      snprintf(value_buf,
               BF_SWITCH_CLI_ATTR_VAL_BUFF_SIZE,
               "0x%" PRIx64 " (%s.%" PRIu64 ")",
               object_handle.data,
               obj_info.get_object_name().c_str(),
               switch_store::handle_to_id(object_handle));
      BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                                "\n%sOID: %s",
                                BF_SWITCH_CLI_PRINT_OBJ_MD_TAB,
                                value_buf);

      BF_SWITCH_CLI_CLISH_PRINT(
          clish_context, "\n%sAttributes:", BF_SWITCH_CLI_PRINT_OBJ_MD_TAB);
      cli_status = bf_switch_cli_dump_object_attributes(
          clish_context, device, &obj_info, object_handle);

      // show counter object_type handle <>
      if (obj_info.get_counter()) {
        bf_switch_cli_print_counter_header(clish_context);
        cli_status = bf_switch_cli_read_and_dump_counter(
            clish_context,
            device,
            object_handle,
            obj_info.get_object_name().c_str());
      }

      BF_SWITCH_CLI_CLISH_PRINT(clish_context, "\n");
    }
  }

  return cli_status;
}

/*
 * cli: state save <destination file path>
 */
int bf_switch_cli_state_save_internal(void *clish_context) {
  const char *filepath = NULL;
  bf_switch_cli_status_t cli_status =
      bf_switch_cli_clish_get_pname_value(clish_context, "filepath", filepath);

  if (cli_status == BF_SWITCH_CLI_STATUS_SUCCESS_E) {
    if (switch_store::object_info_dump(filepath) != SWITCH_STATUS_SUCCESS)
      cli_status = BF_SWITCH_CLI_STATUS_FAILURE_E;
  }

  BF_SWITCH_CLI_CLISH_PRINT(clish_context,
                            "switch state save status: '%s'\n",
                            bf_switch_cli_error_to_string(cli_status));

  return cli_status;
}

} /* namespace switchcli */
} /* namespace smi */
