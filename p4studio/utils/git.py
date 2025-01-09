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

import subprocess

from utils.processes import execute
from utils.types import PathLike


def git_clone(repo: str, branch: str, path: PathLike, recursive: bool = True) -> None:
    command = ["git", "clone", "--depth 1"]
    if recursive:
        command += ["--shallow-submodules", "--recursive", "--jobs 21"]

    command += [repo, "'{}'".format(path), "-b '{}'".format(branch)]
    command_str = " ".join(command)
    execute(command_str)


def git_update_submodules(path: PathLike) -> None:
    command = ['git', '-C', "'{}'".format(path), 'submodule', 'update', '--init', '--recursive']
    advanced_options_available = '--depth' in subprocess.getoutput("git -C '{}' submodule update -h".format(path))
    if advanced_options_available:
        command += ['--jobs 21', '--depth 1']

    command_str = " ".join(command)
    execute(command_str)
