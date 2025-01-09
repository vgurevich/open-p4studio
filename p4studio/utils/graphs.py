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

from typing import TypeVar, List, cast, Dict

T = TypeVar('T')


def reverse_topological_sort(graph: Dict[T, List[T]]) -> List[T]:
    result = cast(List[T], [])
    stack = cast(List[T], list(graph.keys())[::-1])

    while stack:
        current = stack.pop()
        if current in result:
            continue
        not_in_result = [node for node in graph.get(current, []) if node not in result]
        if not_in_result:
            stack.append(current)
            stack.extend(not_in_result[::-1])
        else:
            result.append(current)

    return result
