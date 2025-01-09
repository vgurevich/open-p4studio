#!/usr/bin/env python3
import re
from typing import cast, List, Set, Tuple

from utils.exceptions import ApplicationException
from utils.ordered_set import OrderedSet
from .cmake import available_cmake_options
from .config_option import ConfigOptionDefinition, user_defined_options_definitions, \
    ConfigOption


# Copyright (C) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#
# SPDX-License-Identifier: Apache-2.0


class ConfigurationManager:
    """
    Allows to map p4studio config options to cmake options.
    Validates consistency between options (like switch and ^switch requested together).
    Checks if added options are defined in CMakeLists.txt.
    """

    def __init__(self, definitions: List[ConfigOptionDefinition]):
        self.definitions = definitions
        self.known_p4studio_options = [co.p4studio_name for co in definitions]

    def convert_p4studio_args_to_cmake_options(self, args: List[str]) -> List[str]:
        return [str(cmake_arg) for o in self.convert_to_config_options(args) for cmake_arg in o.cmake_args]

    def convert_to_config_options(self, args: List[str]) -> List[ConfigOption]:
        options = cast(OrderedSet[ConfigOption], OrderedSet())
        for arg in args:
            option = self.p4studio_arg_to_config_option(arg)
            cmake_args = option.cmake_args
            all_cmake_args = [cmake_arg
                              for o in options
                              for cmake_arg in o.cmake_args
                              ]
            if any([ca.will_be_ambiguous_in(all_cmake_args) for ca in cmake_args]):
                msg = "Ambiguous configuration for {} option".format(option.p4studio_name)
                raise ApplicationException(msg)
            options.add(option)

        return list(options)

    def known_p4studio_options_including_negated(self) -> List[str]:
        result = []
        for option in self.known_p4studio_options:
            result += [option] + (['^' + option] if self.definition(option).can_be_disabled else [])
        return result

    def p4studio_arg_to_config_option(self, arg: str) -> ConfigOption:
        name, enabled = parse_p4studio_arg(arg)
        option = next((d.option(enabled) for d in self.definitions if d.p4studio_name == name), None)
        if option is None:
            msg = "Unknown configuration option: {}".format(name)
            raise ApplicationException(msg)
        return option

    def config_option(self, name: str, value: bool) -> ConfigOption:
        option = next(d.option(value) for d in self.definitions if d.p4studio_name == name)
        if option is None:
            msg = "Unknown configuration option: {}".format(name)
            raise ApplicationException(msg)
        return option

    def definitions_by_category(self, name: str) -> List[ConfigOptionDefinition]:
        return [d for d in self.definitions if d.category == name]

    def definition(self, name: str) -> ConfigOptionDefinition:
        for definition in self.definitions:
            if definition.p4studio_name == name:
                return definition
        message = "Unknown '{}' option".format(name)
        raise ApplicationException(message)

    def categories(self) -> Set[str]:
        return {d.category for d in self.definitions}


_configuration_manager = None


def current_configuration_manager() -> ConfigurationManager:
    global _configuration_manager
    if _configuration_manager is None:
        options = [
                      ConfigOptionDefinition(
                          ConfigOption(o.name.lower(), [o.as_cmake_flag(True)], [o.as_cmake_flag(False)], o.default),
                          o.category, o.description)
                      for o in available_cmake_options()
                  ] + user_defined_options_definitions()
        _configuration_manager = ConfigurationManager(options)
    return _configuration_manager


def p4studio_arg_to_config_option(arg: str) -> ConfigOption:
    return current_configuration_manager().p4studio_arg_to_config_option(arg)


def config_option(name: str, value: bool) -> ConfigOption:
    return current_configuration_manager().config_option(name, value)


def parse_p4studio_arg(arg: str) -> Tuple[str, bool]:
    match = re.compile(r'^(?P<off>\^)?(?P<name>[a-zA-Z_][a-zA-Z0-9_-]*)$').search(arg)
    if match is None:
        message = "Incorrect format of configuration option: {}".format(arg)
        raise ApplicationException(message)
    groups = match.groupdict()
    return groups["name"], groups['off'] is None
