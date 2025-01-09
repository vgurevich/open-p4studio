#!/usr/bin/env python3
from pathlib import Path
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
from typing import List

from workspace import current_workspace


class P4ProgramInfo:
    def __init__(self, name: str, group: str, path: Path):
        self.name = name
        self.group = group
        self.path = path


def all_p4_programs() -> List[P4ProgramInfo]:
    result = []
    for group_name, group_path in current_workspace().p4_dirs.items():
        if not group_path.exists():
            continue
        for path in group_path.iterdir():
            if path.is_dir() and path.name not in ['common']:
                program = P4ProgramInfo(path.name, group_name, path)
                result.append(program)
    result.sort(key=lambda p: (p.group, p.name))
    return result
