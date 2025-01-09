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
import importlib
import os
from pathlib import Path
from typing import List

from utils.exceptions import ApplicationException
from utils.processes import command_output
from workspace.workspace import Workspace


def _possible_workspaces() -> List[Workspace]:
    result = []
    for file in Path(__file__).parent.iterdir():
        if file.name.endswith("_workspace.py"):
            result.append(_create_instance_from_module(file))
    return result


def _create_instance_from_module(path: Path) -> Workspace:
    module_name = path.name[:-3]
    module = importlib.import_module('{}.{}'.format(__package__, module_name))
    return getattr(module, 'get_{}'.format(module_name))()


_CURRENT_WORKSPACE = next((w for w in _possible_workspaces() if w.is_valid))


def in_workspace() -> bool:
    """
    Indicates if current working directory is inside workspace
    """
    return _CURRENT_WORKSPACE is not None


def current_workspace() -> Workspace:
    if not in_workspace():
        message = "{} is not a SDE directory".format(os.getcwd())
        raise ApplicationException(message)
    return _CURRENT_WORKSPACE


def configure_env_variables() -> None:
    if 'LANG' not in os.environ:
        supported_locales = command_output(['locale', '-a'])
        preferred_locales = [b'C.UTF-8', b'C.utf8', b'en_US.utf8']
        for locale in preferred_locales:
            if locale in supported_locales:
                os.environ['LANG'] = locale.decode("utf-8")
                break
    if 'LC_ALL' not in os.environ:
        os.environ['LC_ALL'] = os.environ['LANG']


def _add_path(variable_name: str, path: Path) -> None:
    current = os.environ.get(variable_name)
    if not current or path not in current.split(os.pathsep):
        os.environ[variable_name] = (current + os.pathsep if current else '') + str(path)
