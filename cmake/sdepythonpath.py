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

import sys
from distutils.sysconfig import get_python_lib
from pathlib import Path
from typing import Optional

_MY_DIR = Path(__file__).parent.resolve()
_IS_INSTALLED = _MY_DIR.name == 'bin'


def get_sde_python_dependencies(sde_install: Optional[Path], sde_dependencies: Optional[Path]) -> Path:
    if sde_install is None:
        sde_install = _MY_DIR.parent
    if sde_dependencies is None:
        environment_file = (sde_install / 'share/environment').read_text()
        variables = {k: v for k, v in (line.split('=', 1) for line in environment_file.splitlines())}
        sde_dependencies = variables['SDE_DEPENDENCIES']

    sde_dependencies = sde_install.joinpath(sde_dependencies)
    python_lib = get_python_lib(prefix='', standard_lib=True, plat_specific=True)
    return (sde_dependencies / python_lib / 'site-packages')


if __name__ != '__main__' and not _IS_INSTALLED:
    raise ImportError("only installed version of sdepythonpath can be imported")

if _IS_INSTALLED:
    SDE_PYTHON_DEPENDENCIES = get_sde_python_dependencies(None, None).as_posix()
    sys.path.insert(0, SDE_PYTHON_DEPENDENCIES)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Print path to python third-party dependencies')
    if _IS_INSTALLED:
        args = parser.parse_args()
        print(sys.path[0])
    else:
        parser.add_argument(
            '--sde-install',
            type=Path,
            metavar='DIR',
            required=True,
            help='Path to SDE Installation directory'
        )
        parser.add_argument(
            '--sde-dependencies',
            metavar='DIR',
            type=Path,
            required=True,
            help='Path to SDE Dependencies directory (can be relative to --sde-install)'
        )
        args = parser.parse_args()
        path = get_sde_python_dependencies(args.sde_install, args.sde_dependencies)
        print(path)
