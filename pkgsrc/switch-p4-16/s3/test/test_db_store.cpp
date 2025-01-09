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

#include <cassert>
#include <iostream>
#include <chrono>

#include "time.h"
#include "../store.h"
#include "../id_gen.h"
#include "s3/switch_store.h"
#include "gen-model/replay.h"

// #define __CPU_PROFILER__
#ifdef __CPU_PROFILER__
#include "gperftools/profiler.h"
#endif

using namespace smi;
using namespace std::chrono;
static ModelInfo *model_info = NULL;

uint64_t iter = 1000000;

void test_store(const std::vector<switch_object_id_t> &oids,
                const ObjectInfo *object_info) {
  auto start = high_resolution_clock::now();
  for (const auto &oid : oids) db::object_create(oid, object_info);
  auto end = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(end - start);
  std::cout << "Time: " << duration.count() << std::endl;
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/replay.json";
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  const ObjectInfo *object_info =
      model_info->get_object_info(SWITCH_OBJECT_TYPE_ROUTE);
  assert(object_info != NULL);

  std::vector<switch_object_id_t> oids;
  for (uint64_t i = 0; i < iter; i++) {
    switch_object_id_t oid = {};
    switch_store::oid_create(SWITCH_OBJECT_TYPE_ROUTE, oid, false);
    oids.push_back(oid);
  }

#ifdef __CPU_PROFILER__
  ProfilerStart("./perf_data.txt");
  ProfilerFlush();
#endif

  test_store(oids, object_info);

#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif

  printf("\n\nAll tests passed!\n");
  return 0;
}
