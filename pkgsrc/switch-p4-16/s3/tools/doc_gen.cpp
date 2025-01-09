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


#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <memory>

#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "bf_switch/bf_switch_types.h"

#include "s3/meta/meta.h"

using namespace smi;

namespace {
void string_to_lower(std::string &s) {
  transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return tolower(c);
  });
}
void usage(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  std::cout << "Usage: doc_gen"
            << " --input <model_filename> --output <header_filename>"
            << " [--v verbose] \n";
}

std::string value_from_attr(switch_attribute_value_t value) {
  switch (value.type) {
    case SWITCH_TYPE_BOOL:
      return value.booldata ? std::string("true") : std::string("false");
    case SWITCH_TYPE_UINT8:
      return std::to_string(value.u8);
    case SWITCH_TYPE_UINT16:
      return std::to_string(value.u16);
    case SWITCH_TYPE_UINT32:
      return std::to_string(value.u32);
    case SWITCH_TYPE_UINT64:
      return std::to_string(value.u64);
    case SWITCH_TYPE_INT64:
      return std::to_string(value.s64);
    default:
      return std::string("Not implemented");
  }
}

const char *switch_attr_type_str(switch_attr_type_t t) {
  switch (t) {
    case SWITCH_TYPE_NONE:
      return "SWITCH_TYPE_NONE";
    case SWITCH_TYPE_BOOL:
      return "SWITCH_TYPE_BOOL";
    case SWITCH_TYPE_UINT8:
      return "SWITCH_TYPE_UINT8";
    case SWITCH_TYPE_UINT16:
      return "SWITCH_TYPE_UINT16";
    case SWITCH_TYPE_UINT32:
      return "SWITCH_TYPE_UINT32";
    case SWITCH_TYPE_UINT64:
      return "SWITCH_TYPE_UINT64";
    case SWITCH_TYPE_INT64:
      return "SWITCH_TYPE_INT64";
    case SWITCH_TYPE_ENUM:
      return "SWITCH_TYPE_ENUM";
    case SWITCH_TYPE_MAC:
      return "SWITCH_TYPE_MAC";
    case SWITCH_TYPE_STRING:
      return "SWITCH_TYPE_STRING";
    case SWITCH_TYPE_IP_ADDRESS:
      return "SWITCH_TYPE_IP_ADDRESS";
    case SWITCH_TYPE_IP_PREFIX:
      return "SWITCH_TYPE_IP_PREFIX";
    case SWITCH_TYPE_OBJECT_ID:
      return "SWITCH_TYPE_OBJECT_ID";
    case SWITCH_TYPE_LIST:
      return "SWITCH_TYPE_LIST";
    case SWITCH_TYPE_MAX:
      return "SWITCH_TYPE_MAX";
    default:
      return "Not implemented";
  }
}

const char *flag_desc(switch_attr_flags_t flags) {
  if (flags.is_mandatory) return "mandatory";
  if (flags.is_immutable) return "immutable";
  if (flags.is_read_only) return "read_only";
  if (flags.is_internal) return "internal";
  if (flags.is_counter) return "counter";
  return "NA";
}

int parse_options(int argc,
                  char **argv,
                  std::string &input_file,
                  std::string &output_file,
                  bool *verbose) {
  while (1) {
    int option_index = 0;
    /* Options without short equivalents */
    enum long_opts {
      OPT_START = 256,
      OPT_INPUT,
      OPT_OUTPUT,
    };
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"input", required_argument, 0, OPT_INPUT},
        {"output", required_argument, 0, OPT_OUTPUT},
        {"verbose", no_argument, 0, 'v'},

        {0, 0, 0, 0}};
    int c = getopt_long(argc, argv, "h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case OPT_INPUT:
        input_file = optarg;
        break;
      case OPT_OUTPUT:
        output_file = optarg;
        break;
      case 'v':
        *verbose = true;
        break;
      case 'h':
      case '?':
        usage(argc, argv);
        return c == 'h' ? 1 : 0;
        break;
    }
  }
  return 0;
}
}  // namespace

/*
void print_ref_ots(std::stringstream &obj_buffer,
                   switch_model_info_t *model_info,
                   switch_object_type_t ot,
                   std::unique_ptr<ref_dag> &refs) {
  for (const auto ref_ot : refs->inverse_refs_get(ot)) {
    const switch_object_type_info_t *ref_ot_info =
        model_utils_get_object_info(model_info, ref_ot);
    if (ref_ot_info->object_class == OBJECT_CLASS_USER) continue;
    obj_buffer << " * " << model_utils_get_object_name(model_info, ref_ot)
               << "[shape=box, color=red]"
                  "\n";
    obj_buffer << model_utils_get_object_name(model_info, ot) << " -> "
               << model_utils_get_object_name(model_info, ref_ot) << "\n";
    print_ref_ots(obj_buffer, model_info, ref_ot, refs);
  }
}
*/

int main(int argc, char *argv[]) {
  std::string input_file;
  std::string output_file;
  std::ofstream of;
  bool verbose = false;
  int rv = parse_options(argc, argv, input_file, output_file, &verbose);
  if (rv != 0) return rv;

  std::unique_ptr<ModelInfo> model_info =
      build_model_info_from_file(input_file.c_str(), verbose);
  if (model_info == nullptr) {
    return -1;
  }

  of.open(output_file.c_str());

  of << "/* THIS FILE IS AUTOGENERATED. DO NOT MODIFY MANUALLY! */\n\n";
  of << "/** @defgroup obj_model BF Switch Object model\n";
  of << " * @{\n";
  of << " */\n\n";

  /* enums: objects */
  std::stringstream obj_buffer;

  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    ObjectInfo object_info = *it;

    std::string object_name_fqn(object_info.get_object_name_fqn());

    if (object_info.get_object_class() != OBJECT_CLASS_USER) continue;

    // Set defgroup from object name
    obj_buffer << "/**\n";
    obj_buffer << " * @defgroup " << object_info.get_object_name() << "\n";

    /*
    obj_buffer << " * @dot\n";
    obj_buffer << " * digraph {\n";
    obj_buffer << object_info->object_name << "\n";
    print_ref_ots(obj_buffer, model_info, object_info->object_type, refs);
    obj_buffer << " * }\n";
    obj_buffer << " * @enddot\n";
    */

    obj_buffer << "* <H3>Description</H3>\n\n";
    obj_buffer << "* " << object_info.get_object_desc() << "\n\n";

    // Dump mandatory attributes
    obj_buffer << "* <H3>Attributes</H3>\n\n";
    obj_buffer << "<table>\n";
    bool mand_header = true;
    for (auto &attr_md : object_info.get_attribute_list()) {
      if ((attr_md.get_flags()).is_mandatory) {
        if (mand_header) {
          obj_buffer
              << "<caption id=\"multi_row\">Mandatory Attributes</caption>\n";
          obj_buffer << "<tr><th>Name<th>Type\n";
          mand_header = false;
        }
        obj_buffer << "<tr><td>" << attr_md.get_attr_name() << "<td>"
                   << switch_attr_type_str(attr_md.type) << "\n";
      }
    }
    obj_buffer << "</table>\n";

    // Attribute dump
    for (const auto &attr_md : object_info.get_attribute_list()) {
      const ValueMetadata *value_md = attr_md.get_value_metadata();
      std::string attr_name_fqn(attr_md.get_attr_name_fqn());

      // Print description, flags and type
      if (attr_md.get_attr_desc().length() == 0) continue;
      obj_buffer << "* \n<STRONG>>> " << attr_md.get_attr_name()
                 << "</STRONG>\n";
      obj_buffer << "* \n    Description : " << attr_md.get_attr_desc() << "\n";
      obj_buffer << "* \n          Flags : " << flag_desc(attr_md.get_flags())
                 << "\n";
      obj_buffer << "* \n      Data Type : "
                 << switch_attr_type_str(attr_md.type) << "\n";

      // Print default value
      if ((attr_md.type == SWITCH_TYPE_BOOL) ||
          (attr_md.type == SWITCH_TYPE_UINT8) ||
          (attr_md.type == SWITCH_TYPE_UINT16) ||
          (attr_md.type == SWITCH_TYPE_UINT32) ||
          (attr_md.type == SWITCH_TYPE_UINT64) ||
          (attr_md.type == SWITCH_TYPE_INT64)) {
        obj_buffer << "* \n        Default : "
                   << value_from_attr(value_md->get_default_value()) << "\n";
      }

      // Process allowed object types for oid type
      if (attr_md.type == SWITCH_TYPE_OBJECT_ID) {
        obj_buffer << "* \n    Allowed Objects : ";
        for (const auto &ot : value_md->get_allowed_object_types()) {
          obj_buffer << "\""
                     << (model_info->get_object_info(ot))->get_object_name()
                     << "\"  ";
        }
        obj_buffer << "\n";
      }

      // Process list type
      if (attr_md.type == SWITCH_TYPE_LIST) {
        obj_buffer << "* \n      List type : "
                   << switch_attr_type_str(value_md->type) << "\n";
      }

      // Process enum type
      if (attr_md.type == SWITCH_TYPE_ENUM) {
        std::string attr_name_fqn_lower(attr_name_fqn);
        string_to_lower(attr_name_fqn_lower);
        obj_buffer << "* \n           Enum : " << attr_name_fqn_lower << "\n";
        obj_buffer << "* \n    Enumerator \n";
        for (const auto &enumdata : value_md->get_enum_metadata()) {
          std::string val_name(enumdata.enum_name_fqn);
          obj_buffer << "* \n      " << val_name << " : " << enumdata.enum_value
                     << "\n";
        }
        obj_buffer << "* \n      " << attr_name_fqn << "_MAX\n";
        // obj_buffer << "* \n    Default : " << value_md->get_default_value()
        //           << "\n";
      }
    }
    obj_buffer << " * @{\n";
    obj_buffer << " */\n";
    obj_buffer << "/** @} */\n\n";
  }

  of << obj_buffer.str();

  of << "/** @} */\n";
  of.close();
  return 0;
}
