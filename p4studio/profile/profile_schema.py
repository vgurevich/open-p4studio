#!/usr/bin/env python3

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

import build
from config.configuration_manager import ConfigurationManager
from dependencies.dependency_manager import ALL_SOURCE_PACKAGES
from typing import Dict, List, Union, Mapping, cast


def create_profile_schema(configuration_manager: ConfigurationManager) -> Dict[str, object]:
    return {
        'type': 'object',
        'required': ['global-options', 'features', 'architectures'],
        'properties': {
            'dependencies': _create_dependencies_schema(),
            'global-options': _create_global_options_schema(configuration_manager),
            'features': _create_features_schema(configuration_manager),
            'architectures': _create_architectures_schema(configuration_manager),
            'install-prefix': string_schema(),
        },
        "additionalProperties": False,
    }


def _create_dependencies_schema() -> Dict[str, object]:
    return object_schema({
        'source-packages': array_schema(
            enum_schema(ALL_SOURCE_PACKAGES)
        )
    })


def _create_global_options_schema(configuration_manager: ConfigurationManager) -> Dict[str, object]:
    options = cast(Dict[str, object], {
        d.p4studio_name: boolean_schema()
        for d in configuration_manager.definitions_by_category('Global')
    })

    options.update({
        'p4ppflags': nullable_string_schema(),
        'p4flags': nullable_string_schema(),
        'extra-cppflags': nullable_string_schema(),
        'kdir': nullable_string_schema(),
    })

    return object_schema(options)


def _create_features_schema(configuration_manager: ConfigurationManager) -> Dict[str, object]:
    return object_schema({
        category.lower(): _create_feature_schema(configuration_manager, category)
        for category in (configuration_manager.categories() | {'p4-examples'})
        if category not in ['Global', 'Architecture']
    })


def _create_feature_schema(configuration_manager: ConfigurationManager, category: str) -> Mapping[str, object]:
    if category == 'p4-examples':
        return array_schema(string_schema())
    else:
        feature = object_schema({
            option.p4studio_name: boolean_schema()
            for option in configuration_manager.definitions_by_category(category)
        })

        if category == "BF-Platforms":
            cast(Dict[str, object], feature['properties'])['bsp-path'] = string_schema()
        elif category == 'Switch':
            cast(Dict[str, object], feature['properties'])['profile'] = enum_schema(build.all_switch_profiles())

        result = {'oneOf': [cast(Mapping[str, object], feature)]}
        result['oneOf'].append(boolean_schema())
        return result


def _create_architectures_schema(configuration_manager: ConfigurationManager) -> Dict[str, object]:
    return array_schema(
        enum_schema([
            d.p4studio_name
            for d in configuration_manager.definitions_by_category('Architecture')
        ])
    )


def object_schema(properties: Dict[str, object]) -> Dict[str, object]:
    return {
        'type': 'object',
        'properties': dict(properties),
        'additionalProperties': False
    }


def array_schema(item_schema: Mapping[str, Union[str, List[str]]]) -> Dict[str, object]:
    return {'type': 'array', 'items': item_schema}


def boolean_schema() -> Mapping[str, str]:
    return {'type': 'boolean'}


def string_schema() -> Mapping[str, str]:
    return {'type': 'string'}


def nullable_string_schema() -> Dict[str, List[str]]:
    return {'type': ['string', 'null']}


def enum_schema(values: List[str]) -> Dict[str, Union[str, List[str]]]:
    return {'type': 'string', 'enum': list(values)}
