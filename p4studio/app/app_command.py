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

from os.path import abspath, dirname
from pathlib import Path
from typing import Union

import click
from click._bashcomplete import get_completion_script

import main
from workspace import current_workspace


@click.group('app')
def app_command() -> None:
    """
    Manage p4studio application and its environment
    """


@click.command('activate')
@click.option('--with-workspace', is_flag=True, default=False,
              help="Configure workspace specific environment variables")
def activate_command(with_workspace: bool) -> None:
    """
    Enable bash completion and configure environment variables
    """
    print(get_completion_script('p4studio', '_P4STUDIO_COMPLETE', 'bash'))
    print()
    print(_get_update_path_script('PATH', dirname(abspath(main.__file__))))

    if with_workspace:
        print(_get_update_path_script('PATH', current_workspace().default_install_dir / 'bin'))
        print(_get_update_path_script('CMAKE_LIBRARY_PATH', current_workspace().default_install_dir / 'lib'))
        print(_get_update_path_script('CMAKE_INCLUDE_PATH', current_workspace().default_install_dir / 'include'))
        print(_get_update_path_script('LIBRARY_PATH', current_workspace().default_install_dir / 'lib'))
        print(_get_update_path_script('LD_RUN_PATH', current_workspace().default_install_dir / 'lib'))
        print(_get_update_path_script('CPLUS_INCLUDE_PATH', current_workspace().default_install_dir / 'include'))
        print(_get_update_path_script('PKG_CONFIG_PATH', current_workspace().default_install_dir / 'lib/pkgconfig'))


app_command.add_command(activate_command)


def _get_update_path_script(var_name: str, added_dir: Union[str, Path]) -> str:
    template = 'if [ ":${var_name}:" != *":{added_dir}:"* ]; then export {var_name}="{added_dir}:${var_name}"; fi'
    return template.format(var_name=var_name, added_dir=added_dir)
