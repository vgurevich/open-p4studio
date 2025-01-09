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

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>

#include <cassert>
#include <getopt.h>
#include <cstdlib>
#include <cstring>

#include "s3/meta/meta.h"

#include "../log.h"

using namespace smi;

std::unique_ptr<ModelInfo> model_info = nullptr;

void ref_graph_dot_dump(std::stringstream &o) {
  o << "digraph \"Attr Dependency Graph\" {" << std::endl;
  o << "graph [splines=curved, nodesep=1, overlap=scale]" << std::endl;

  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    auto object_info = *it;
    const auto ot = object_info.object_type;

    for (const auto referenced_object_type : model_info->refs_get(ot)) {
      std::string object_info_from(
          (model_info->get_object_info(ot))->get_object_name());
      std::string object_info_to(
          (model_info->get_object_info(referenced_object_type))
              ->get_object_name());
      const ObjectInfo *ref_object_info =
          model_info->get_object_info(referenced_object_type);
      if (ref_object_info == nullptr) continue;

      if (object_info.get_object_class() == OBJECT_CLASS_USER &&
          ref_object_info->get_object_class() == OBJECT_CLASS_USER) {
        continue;
      }
      o << object_info_from << " -> " << object_info_to
        << " [color=\"0.650 0.700 0.700\"];\n";
    }

    if (object_info.get_object_class() == OBJECT_CLASS_AUTO) {
      o << object_info.get_object_name() << "[shape=box, color=red]"
        << std::endl;
    } else {
      o << "subgraph cluster_0 {";
      o << object_info.get_object_name() << "[pencolor=blue]";
      o << "}" << std::endl;
    }
  }

  o << "}" << std::endl;
}

switch_status_t rec_dep_graph_dot_dump(const switch_object_type_t previous_ot,
                                       const switch_object_type_t current_ot,
                                       const switch_attr_id_t source_attr_id,
                                       std::stringstream &o) {
  thread_local std::set<switch_object_type_t> context;

  switch_status_t status = SWITCH_STATUS_SUCCESS;

  /* go over object types refering to this (attr id) */
  auto dep_ots = model_info->dep_ots_get(source_attr_id);
  auto dep_path_ots = model_info->dep_path_ots_get(source_attr_id);

  std::string prefix;
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    ObjectInfo object_info = *it;
    bool done = false;
    for (const auto &attr_md : object_info.get_attribute_list()) {
      if (attr_md.attr_id == source_attr_id) {
        std::string source_attr(attr_md.get_attr_name());
        const ObjectInfo *source_obj_info =
            model_info->get_object_info(attr_md.get_parent_object_type());
        std::string source_obj(source_obj_info->get_object_name());
        prefix = source_obj + "_" + source_attr + "__";
        done = true;
        break;
      }
    }
    if (done) break;
  }

  std::string object_current(
      prefix + (model_info->get_object_info(current_ot))->get_object_name());

  /* is current OT in deps? */
  if (dep_ots.find(current_ot) != dep_ots.end()) {
    /* this object type depend of source attr, capture */
    std::string object_prev(
        prefix + (model_info->get_object_info(previous_ot))->get_object_name());
    // o << object_current << " -> " << object_prev << " [color=\"0.650 0.700
    // 0.700\"];"<< std::endl;
  }

  // OTs that reference current OT
  const auto ref_ots = model_info->inverse_refs_get(current_ot);

  for (const auto ref_ot : ref_ots) {
    if (dep_path_ots.find(ref_ot) != dep_path_ots.end()) {
      const auto ret = context.insert(ref_ot);
      if (ret.second == true) {
        /* recurse with new current_ot */
        std::string object_ref(
            prefix + (model_info->get_object_info(ref_ot))->get_object_name());
        o << object_ref << " -> " << object_current
          << " [color=\"0.650 0.700 0.700\"];" << std::endl;
        rec_dep_graph_dot_dump(current_ot, ref_ot, source_attr_id, o);

        context.erase(ref_ot);
      }
    }
  }

  return status;
}

void dep_graph_dot_dump(std::stringstream &o) {
  o << "digraph \"Attr Dependency Graph\" {" << std::endl;
  o << "graph [splines=ortho, nodesep=1, overlap=scale]" << std::endl;

  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    ObjectInfo object_info = *it;
    const auto source_ot = object_info.object_type;
    for (const auto &attr_md : object_info.get_attribute_list()) {
      const auto source_attr_id = attr_md.attr_id;
      rec_dep_graph_dot_dump(0, source_ot, source_attr_id, o);
    }
  }
  o << "}" << std::endl;
}
static void parse_options(int argc,
                          char **argv,
                          std::string &model_json_filename,
                          std::string &directory_name) {
  while (1) {
    int option_index = 0;
    /* Options without short equivalents */
    enum long_opts { OPT_START = 256, OPT_MODEL, OPT_DIR };
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"model", required_argument, 0, OPT_MODEL},
        {"directory", required_argument, 0, OPT_DIR},
        {0, 0, 0, 0}};
    int c = getopt_long(argc, argv, "h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case OPT_MODEL:
        model_json_filename.append(optarg);
        break;
      case OPT_DIR:
        directory_name.append(optarg);
        break;
      case 'h':
      case '?':
        printf("graph gen \n");
        printf("Usage: drivers [OPTION]...\n");
        printf("\n");
        printf(" --model <model json file> \n");
        printf(" --directory <output directiry location> \n");
        printf(" -h,--help Display this help message and exit\n");
        exit(c == 'h' ? 0 : 1);
        break;
    }
  }
}

int main(int argc, char **argv) {
  std::string model_json_filename;
  std::string directory_name;

  parse_options(argc, argv, model_json_filename, directory_name);

  model_info = build_model_info_from_file(model_json_filename.c_str(), true);
  if (model_info == nullptr) return -1;

  std::stringstream ref_graph, dep_graph;
  ref_graph_dot_dump(ref_graph);
  // dep_graph_dot_dump(dep_graph);

  std::ofstream ref_graph_outFile;
  ref_graph_outFile.open(directory_name + "/ref_graph.dot");
  ref_graph_outFile << ref_graph.str() << std::endl;

  // std::ofstream dep_graph_outFile;
  // dep_graph_outFile.open(directory_name + "/dep_graph.dot");
  // dep_graph_outFile << dep_graph.str() << std::endl;

  return 0;
}
