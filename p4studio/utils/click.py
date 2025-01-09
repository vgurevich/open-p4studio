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
from typing import Any, Callable, Union, Optional, List, Mapping

import click
from click import Option, Context, Command
from utils.click_cmds import get_full_cmd_str
from utils.processes import cmd_args_to_str


class OrderedGroup(click.Group):
    def __init__(self, name: Optional[str] = None,
                 commands: Mapping[str, Command] = None, **attrs: Any) -> None:
        super().__init__(name, commands, **attrs)
        self.commands = commands or OrderedDict()

    def list_commands(self, ctx: Context) -> List[str]:
        return list(self.commands.keys())


class BetterOption(Option):
    def get_default(self, ctx: Context) -> Optional[Union[object, Callable[[], object]]]:
        if ctx.default_map and self.name in ctx.default_map:
            return ctx.default_map[self.name]
        else:
            return super().get_default(ctx)


def command_call_to_str(command: Command, **kwargs: Any) -> str:
    import main
    _, result = get_full_cmd_str(command, main.p4studio_cli.commands, ['p4studio'])

    params = {
        param.name: param
        for param in command.params
    }

    for arg, value in kwargs.items():
        if value in (None, ()):
            continue
        param = params[arg]
        if isinstance(param, Option):
            result += [param.opts[0]]

        result += list(value) if isinstance(value, tuple) else [value]

    return cmd_args_to_str(result)
