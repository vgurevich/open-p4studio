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

from typing import List, Dict, Tuple

import click


def get_full_cmd_str(command: click.Command, commands: Dict[str, click.Command], result: List[str]) -> \
        Tuple[bool, List[str]]:
    found = False
    for cmd_name, cmd in commands.items():
        if cmd == command:
            result.append(cmd_name)
            return True, result
        elif isinstance(cmd, click.Group):
            result.append(cmd_name)
            found, _ = get_full_cmd_str(command, cmd.commands, result)
            if not found:
                result.remove(cmd_name)
    return found, result
