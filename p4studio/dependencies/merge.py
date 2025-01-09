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
from pathlib import Path
from typing import Any, List, Dict

import yaml


def merge(a: Any, b: Any) -> Any:
    """
    Takes two containers (dict or list) and merges their element together without duplicates.
    For dictionaries, in case of key collision:
    if both values are containers, they are merged recursively;
    otherwise the second one is assigned to this key.
    """
    if isinstance(a, dict) and isinstance(b, dict):
        result = a.copy()
        for (key, value) in b.items():
            result[key] = merge(result.get(key), value)
        return result
    elif isinstance(a, list) and isinstance(b, list):
        return a + b
    else:
        return b


def merge_all(*items: Any) -> Any:
    if len(items) == 0:
        return None
    else:
        result = make_copy_if_needed(items[0])
        for item in items[1:]:
            result = merge(result, item)
        return result


def make_copy_if_needed(obj: Any) -> Any:
    if isinstance(obj, list) or isinstance(obj, dict):
        return obj.copy()
    else:
        return obj


def merge_files(paths: List[Path]) -> Dict[str, Any]:
    result = {}
    for path in paths:
        with path.open() as f:
            next_yaml = yaml.load(f, Loader=yaml.BaseLoader)
            result = merge(result, next_yaml)
    return result
