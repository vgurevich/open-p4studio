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

from typing import Dict
from utils.collections import nested_get, nested_set, nested_del
from utils.terminal import print_warning


def adjust_for_backward_compatibility(profile: Dict[str, object]) -> None:
    _move(profile, 'global-options/bsp', 'features/bf-platforms/bsp')
    _move(profile, 'global-options/newport', 'features/bf-platforms/newport')
    _move(profile, 'global-options/tclonly', 'features/bf-platforms/tclonly')
    _move(profile, 'global-options/accton-diags', 'features/bf-platforms/accton-diags')
    _move(profile, 'global-options/newport-diags', 'features/bf-platforms/newport-diags')


def _move(dictionary: Dict[str, object], old_path: str, new_path: str) -> None:
    value = nested_get(dictionary, old_path, None)
    if value is not None:
        print_warning("-" * 120)
        print_warning("'{}' has been deprecated and will be removed in future. Use '{}'.", old_path, new_path)
        print_warning("-" * 120)
        nested_set(dictionary, new_path, value)
        nested_del(dictionary, old_path)
