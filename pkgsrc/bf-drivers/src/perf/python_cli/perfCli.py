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

import os
import sys
from tabulate import tabulate

from perfHelpers import *
from perfTest import PerfTest


class CPerf:
    def __init__(self, dev_id):
        self._dev_id = dev_id

        try:
            self._driver = CDLL(sys.prefix + "/lib/libdriver.so")
        except OSError as error:
            raise OSError(f"Unable to load the libdriver library."
                          f"\nDetails: {error}")

        self._driver.get_metrics_n_max.argtypes = []
        self._driver.get_metrics_n_max.restype = c_int
        self.struct_size = {
            "params_size": self._driver.get_params_n_max(),
            "results_size": self._driver.get_results_n_max(),
            "metrics_size": self._driver.get_metrics_n_max(),
            "env_param_size": self._driver.get_env_param_n_max(),
            "env_param_value_size": self._driver.get_env_param_value_l_max()
        }

        self.__prepare_test_structures()
        tests_list_double_pointer = POINTER(POINTER(test_description))
        driver_tests_list = self.__get_driver_list(self._driver.list_tests,
                                                   tests_list_double_pointer)
        enum_list_pointer = POINTER(enum_description)
        driver_enum_list = self.__get_driver_list(self._driver.describe_enum,
                                                  enum_list_pointer)

        self.tests_list = dict()
        self.__prepare_tests_list(driver_tests_list)
        self.enum_list = dict()
        self.__prepare_enum_list(driver_enum_list)
        self.results = None

        for test_name, test_info in self.tests_list.items():
            self.__prepare_driver_test_functions(test_name, test_info)
            if os.path.exists(test_info["filename"]):
                print(f"Removing old logfile: {test_info['filename']}")
                os.remove(test_info["filename"])
            setattr(self, test_name, PerfTest(test_name, self._driver, self._dev_id,
                                              self.tests_list, self.enum_list))

    # public methods -------------------------------------------------------------------

    def list(self):
        print("\nList of tests:")
        print("-------------")
        for test_name in self.tests_list:
            print(test_name)

    def env(self):
        self._driver.environment.argtypes = [c_int]
        self._driver.environment.restype = env_description
        # noinspection PyUnusedLocal
        env_info = env_description()
        env_info = self._driver.environment(c_int(self._dev_id))
        if not env_info.status:
            print_banner(f"Error occurred during getting environment for device "
                         f"{self._dev_id}.")
            return

        self.__pretty_print_env_info(env_info)

    @staticmethod
    def help():
        msg = """
---------------------------Python Performance API----------------------------

Create CPerf class object:

perf = CPerf(dev_id = 0)

-------------------------List of available functions-------------------------

perf.list()                  - prints the list of available tests;
perf.env()                   - prints environment information
                                   (like family, SKU, modules' versions, etc.);
perf.>test_name<.info()      - prints the detailed test description
                               and example of use;
perf.>test_name<.run(params) - runs specified test; params are optional;

-----------------------------------------------------------------------------"""
        print(msg)

    # init private functions -----------------------------------------------------------
    def __prepare_test_structures(self):
        try:
            test_description._fields_ = [
                ("test_name", c_char_p),
                ("description", c_char_p),
                ("params", param_set * self.struct_size["params_size"]),
                ("results", result_set * self.struct_size["results_size"])
            ]
            test_results._fields_ = [
                ("status", c_bool),
                ("res_enum", c_int * self.struct_size["metrics_size"]),
                ("res_int", c_int * self.struct_size["metrics_size"]),
                ("res_double", c_double * self.struct_size["metrics_size"])
            ]
            env_param._fields_ = [
                ("name", c_char_p),
                ("value", c_char * self.struct_size["env_param_value_size"])
            ]
            env_description._fields_ = [
                ("status", c_bool),
                ("num_params", c_int),
                ("param", env_param * self.struct_size["env_param_size"])
            ]
        except AttributeError:
            print("Tests description structure already defined.")

    @staticmethod
    def __get_driver_list(driver_fun, driver_list_pointer):
        driver_fun.argtypes = []
        driver_fun.restype = driver_list_pointer

        # NOTE: first, allocate ctypes pointer, then fill it with data
        # noinspection PyUnusedLocal
        driver_list = driver_list_pointer()
        driver_list = driver_fun()

        return driver_list

    def __prepare_tests_list(self, tests_list):
        test_no = 0
        while tests_list[test_no]:
            current_test = tests_list[test_no].contents
            test_name = current_test.test_name.decode()
            self.tests_list[test_name] = {
                "filename": get_csv_file_name(test_name),
                "description": current_test.description.decode(),
                "params": [],
                "results": []
            }
            index = 0
            while self.__attr_from_pointer(current_test.params, index, "name"):
                param = {
                    "name": self.__attr_from_pointer(current_test.params, index,
                                                     "name"),
                    "type": self.__attr_from_pointer(current_test.params, index,
                                                     "type"),
                    "defaults": cast_param(
                        current_test.params[index].defaults.decode(),
                        current_test.params[index].type.decode())
                }
                self.tests_list[test_name]["params"].append(param)
                index += 1

            index = 0
            while self.__attr_from_pointer(current_test.results, index, "header"):
                result = {
                    "header": self.__attr_from_pointer(current_test.results, index,
                                                       "header"),
                    "unit": self.__attr_from_pointer(current_test.results, index,
                                                     "unit"),
                    "type": self.__attr_from_pointer(current_test.results, index,
                                                     "type"),
                }
                self.tests_list[test_name]["results"].append(result)
                index += 1

            test_no += 1

    def __prepare_enum_list(self, enum_list):
        enum_no = 0
        while self.__attr_from_pointer(enum_list, enum_no, "enum_name"):
            name = self.__attr_from_pointer(enum_list, enum_no, "enum_name")
            self.enum_list[name] = dict()
            for enum in enum_list[enum_no].enum_desc.decode().split(","):
                self.enum_list[name][enum.split(":")[0]] = enum.split(":")[1]
            enum_no += 1

    @staticmethod
    def __attr_from_pointer(ptr, index, attr_name):
        return getattr(ptr[index], attr_name).decode()

    def __prepare_driver_test_functions(self, test_name, test_info):
        input_list = [CTYPES_MAPPING[item["type"]][1]
                      for item in test_info["params"]]
        input_list.insert(0, c_int)
        getattr(self._driver, test_name).argtypes = input_list
        getattr(self._driver, test_name).restype = test_results

    def __pretty_print_env_info(self, env_info):
        print_banner(f"Environment (device {self._dev_id})")
        env_info_to_print = [
            [
                env_info.param[param_index].name.decode(),
                ':',
                env_info.param[param_index].value.decode()
            ]
            for param_index in range(env_info.num_params)
        ]
        print(tabulate(env_info_to_print, tablefmt="plain"))
