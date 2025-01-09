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
from typing import List

from utils.exceptions import ApplicationException
from utils.processes import try_execute
from utils.terminal import compact_log


class SubprocessBuilder:
    def __init__(self, command_name: str = None):
        self._name = command_name
        self._args = []

    def args(self, *args: object) -> 'SubprocessBuilder':
        self._args += [str(arg) for arg in args]
        return self

    def sudo(self, home: bool = False) -> 'SubprocessBuilder':
        return self.args('sudo', '-HE') if home else self.args('sudo', '-E')

    def pip3(self) -> 'SubprocessBuilder':
        return self.args('env', 'pip3')

    def pip3_install(self, packages: List[str]) -> 'SubprocessBuilder':
        packages = packages or []
        return self.pip3().args('install').args(*packages)

    def python3(self, script: Path) -> 'SubprocessBuilder':
        return self.args('env', 'python3', script)

    def run_or_raise(self, failure_message: str = None, working_dir: Path = Path.cwd()) -> None:
        name = self._name or "running {}".format(self._args[0])
        compact_log().start_new("- {}: ".format(name))
        if not try_execute(self._args, working_dir):
            if not failure_message:
                failure_message = "Problem occurred while {}".format(name)
            compact_log().log("\n{}\n".format(failure_message))
            compact_log().done(False)
            raise ApplicationException(failure_message.format(' '.join(self._args)))
        compact_log().done(True)


def subprocess_builder(command_name: str = None) -> SubprocessBuilder:
    return SubprocessBuilder(command_name)
