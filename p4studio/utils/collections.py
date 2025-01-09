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

from collections import OrderedDict
from itertools import groupby
from typing import Dict, Any, Callable, List, TypeVar, Sequence, Tuple

T = TypeVar('T')


def group_by_to_dict(items: List[T], key_function: Callable[[T], object]) -> OrderedDict:
    result = OrderedDict()
    for k, value in groupby(items, key_function):
        if k not in result:
            result[k] = []
        result[k] += list(value)
    return result


def nested_get(dictionary: Dict[str, Any], path: str, default: object) -> object:
    segments = path.split('/')
    current = dictionary
    current_path = []
    for segment in segments:
        current_path.append(segment)
        if not isinstance(current, dict):
            raise _not_dict_exception(current_path)
        if segment not in current:
            return default
        current = current[segment]
    return current


def nested_set(dictionary: Dict[str, Any], path: str, value: object) -> Dict[str, Any]:
    segments = path.split('/')
    current = dictionary
    current_path = []
    for segment in segments[:-1]:
        current_path.append(segment)
        if not isinstance(current, dict):
            raise _not_dict_exception(current_path)
        current = current.setdefault(segment, OrderedDict())

    if not isinstance(current, dict):
        raise _not_dict_exception(current_path)
    current[segments[-1]] = value
    return dictionary


def nested_del(dictionary: Dict[str, Any], path: str) -> Dict[str, Any]:
    segments = path.split('/')
    current = dictionary
    current_path = []
    for segment in segments[:-1]:
        if not isinstance(current, dict):
            raise _not_dict_exception(current_path)
        if segment not in current:
            return dictionary
        current_path.append(segment)
        current = current[segment]

    if not isinstance(current, dict):
        raise _not_dict_exception(current_path)
    del current[segments[-1]]
    return dictionary


def _not_dict_exception(path: Sequence[str]) -> Exception:
    message = "value of '{}' should be an instance of dict".format('/'.join(path))
    return Exception(message)


def partition(collection: List[T], predicate: Callable[[T], bool]) -> Tuple[List[T], List[T]]:
    a = []
    b = []
    for item in collection:
        if predicate(item):
            a.append(item)
        else:
            b.append(item)
    return a, b


def flatten(nested_list: List[List[T]]) -> List[T]:
    return [element for inside_list in nested_list for element in inside_list]


def diff(list1: List[T], list2: List[T]) -> List[T]:
    return [x for x in list1 if x not in list2]


def as_list_of_strings(objects: Sequence[object]) -> List[str]:
    return [str(x) for x in objects]
