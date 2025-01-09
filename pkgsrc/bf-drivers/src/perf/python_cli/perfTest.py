################################################################################
 #  Copyright (C) 2024 Intel Corporation
 #
 #  Licensed under the Apache License, Version 2.0 (the "License");
 #  you may not use this file except in compliance with the License.
 #  You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 #  Unless required by applicable law or agreed to in writing,
 #  software distributed under the License is distributed on an "AS IS" BASIS,
 #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 #  See the License for the specific language governing permissions
 #  and limitations under the License.
 #
 #
 #  SPDX-License-Identifier: Apache-2.0
################################################################################

import csv
import os
from tabulate import tabulate

from perfHelpers import *


class PerfTest:
    def __init__(self, test_name, driver, dev_id, tests_list, enum_list):
        self._driver = driver
        self._dev_id = dev_id
        self._tests_list = tests_list
        self._enum_list = enum_list

        if not self._validate_test_name(test_name):
            return
        self.test_name = test_name
        self.results = None

    def _validate_test_name(self, test_name):
        if not test_name:
            print("Please provide a test name")
            return False

        if test_name not in self._tests_list:
            print(f"\n{test_name} test not in the list\n")
            return False

        return True

    def info(self):
        headers = ["param", "type", "defaults"]
        table = [[item["name"], item["type"], item["defaults"]] for item in
                 self._tests_list[self.test_name]["params"]]
        for line in table:
            if line[1] == "enum":
                line[2] = self._enum_list[line[0]][str(line[2])]
        desc = self._tests_list[self.test_name]["description"]
        print(f"\nTest: {self.test_name}")
        print(f"\n{desc}")
        print(tabulate(table, headers, tablefmt="presto"))
        print("\nTo run the test, call: >obj_name<.>test_name<.run(test_params)")
        example = f"Example: >obj_name<.{self.test_name}.run("
        for item in table:
            quote = ""
            if item[1] in ["enum", "string"]:
                quote = "'"
            example += f", {item[0]}={quote}{item[2]}{quote}"
        example += ")"
        print(example)

        for key, value in self._enum_list.items():
            if key in [table[i][0] for i in range(len(table))]:
                print(f"\nenum mapping: {key}")
                for enum, val in value.items():
                    print(f"{enum} : {val}")

    def run(self, *args, **kwargs):
        try:
            params = self._parse_test_parameters(*args, **kwargs)
        except ArgumentError as exc:
            print(f"Problem occurred while parsing test parameters. "
                  f"Details: {str(exc)}")
            self.info()
            return

        self._run_test_function(params)

        self._print_results()
        if self.results.status:
            self._export_results_to_csv()

    def _parse_test_parameters(self, *args, **kwargs):
        if kwargs:
            return self._parse_kwargs(**kwargs)
        if args:
            return self._parse_args(*args)
        return self._default_params()

    def _parse_kwargs(self, **kwargs):
        valid_params = [param["name"]
                        for param in self._tests_list[self.test_name]["params"]]
        invalid_params = [param_name for param_name in kwargs
                          if param_name not in valid_params]
        if invalid_params:
            raise ArgumentError(f"Parameter{'s' if len(invalid_params) > 1 else ''} "
                                f"{','.join(invalid_params)} "
                                f"{'are' if len(invalid_params) > 1 else 'is'} invalid")

        parsed_params = []
        for param in self._tests_list[self.test_name]["params"]:
            ctypes_type = cast_to_ctypes(param["type"])
            if param["name"] not in kwargs:
                parsed_params.append(ctypes_type(param["defaults"]))
                continue

            if param["type"] == "enum" and not isinstance(kwargs[param["name"]], int):
                argument = next(
                    int(enum_int)
                    for enum_int, enum_str in self._enum_list[param["name"]].items()
                    if enum_str == kwargs[param["name"]]
                )
            else:
                argument = kwargs[param["name"]]
            parsed_params.append(ctypes_type(argument))
        return parsed_params

    def _parse_args(self, *args):
        params = []
        if len(args) != len(self._tests_list[self.test_name]["params"]):
            raise ArgumentError("Incorrect number of positional arguments")

        for arg, param in zip(args, self._tests_list[self.test_name]["params"]):
            ctypes_type = cast_to_ctypes(param["type"])
            if param["type"] == "enum" and not isinstance(arg, int):
                for key, value in self._enum_list[param["name"]].items():
                    if value == arg:
                        params.append(ctypes_type(key))
                        break
            else:
                params.append(ctypes_type(arg))
        return params

    def _default_params(self):
        params = []
        for param in self._tests_list[self.test_name]["params"]:
            ctypes_type = cast_to_ctypes(param["type"])
            params.append(ctypes_type(param["defaults"]))

        return params

    def _run_test_function(self, params):
        test_function = getattr(self._driver, self.test_name)
        self.results = test_results()
        self.results = test_function(c_int(self._dev_id), *params)

    def _print_results(self):
        if not self.results.status:
            print_banner(f"Test {self.test_name}: failed")
            return

        print_banner(self.test_name)
        results = [self._list_results()]
        headers = [f"{header_info['header'].capitalize()}\n{header_info['unit']}"
                   for header_info in self._tests_list[self.test_name]["results"]]
        print(tabulate(results, headers, floatfmt=".2f"))

    def _export_results_to_csv(self):
        filename = self._tests_list[self.test_name]["filename"]
        if not os.path.exists(filename):
            self._create_csv_logfile(filename,
                                     self._tests_list[self.test_name]["results"])

        results = self._list_results()
        with open(filename, 'a') as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(results)
        print(f"\nTest results have been written to {filename}\n")

    def _list_results(self):
        enum_index = 0
        int_index = 0
        double_index = 0
        results = list()

        for item in self._tests_list[self.test_name]["results"]:
            if item["type"] == "enum":
                results.append(self._enum_list[item["header"]][
                                   str(self.results.res_enum[enum_index])
                               ])
                enum_index += 1
            elif item["type"] == "int":
                results.append(self.results.res_int[int_index])
                int_index += 1
            elif item["type"] == "double":
                float_value = round(self.results.res_double[double_index], 2)
                results.append(float_value)
                double_index += 1

        return results

    def _create_csv_logfile(self, filename, headers):
        with open(filename, 'w') as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow([item["header"] for item in headers])
            writer.writerow([item["unit"] for item in headers])
