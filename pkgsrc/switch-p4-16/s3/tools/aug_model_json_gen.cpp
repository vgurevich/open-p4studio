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
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "bf_switch/bf_switch_types.h"

#include "s3/meta/meta.h"

#include <target-utils/third-party/cJSON/cJSON.h>

using namespace std;
using namespace smi;

namespace {

void usage(int argc, char *argv[]) {
  (void)argc;
  cout << "usage: " << argv[0]
       << " --input <model_filename> --output <header_filename> \n";
}

int parse_options(int argc,
                  char **argv,
                  std::string &input_file,
                  std::string &output_file) {
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
      case 'h':
      case '?':
        usage(argc, argv);
        return c == 'h' ? 0 : 1;
        break;
    }
  }
  return 0;
}
}  // namespace
int main(int argc, char *argv[]) {
  std::string input_file;
  std::string output_file;

  int rv = parse_options(argc, argv, input_file, output_file);
  if (rv != 0) return rv;

  std::unique_ptr<ModelInfo> model_info =
      build_model_info_from_file(input_file.c_str(), false);
  if (model_info == nullptr) {
    return -1;
  }

  ofstream o_f;
  ifstream in_f(input_file.c_str());
  stringstream input_buffer;
  input_buffer << in_f.rdbuf();

  cJSON *root_cjson = cJSON_Parse(input_buffer.str().c_str());

  cJSON *objects_cjson = cJSON_GetObjectItem(root_cjson, "objects");
  cJSON *object_cjson = NULL;

  cJSON_ArrayForEach(object_cjson, objects_cjson) {
    const ObjectInfo *object_info =
        model_info->get_object_info_from_name(object_cjson->string);

    if (object_info == nullptr) return -1;

    switch_object_type_t object_type = object_info->object_type;

    cJSON_AddNumberToObject(object_cjson, "object_type", object_type);

    cJSON *attrs_cjson = cJSON_GetObjectItem(object_cjson, "attributes");
    cJSON *attr_cjson;
    cJSON_ArrayForEach(attr_cjson, attrs_cjson) {
      switch_attr_id_t attr_id = 0;

      attr_id = object_info->get_attr_id_from_name(attr_cjson->string);
      if (attr_id == 0) {
        if (root_cjson) cJSON_Delete(root_cjson);
        return -1;
      }

      cJSON_AddNumberToObject(attr_cjson, "attr_id", attr_id);
    }
  }

  o_f.open(output_file.c_str());
  char *out_buffer = cJSON_Print(root_cjson);
  o_f << out_buffer << std::endl;
  o_f.close();

  free(out_buffer);
  if (root_cjson) cJSON_Delete(root_cjson);
  return 0;
}
