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
from logging import FileHandler
from typing import Dict

import click
from click import ClickException, Abort

from app import app_command
from build import build_command
from clean.clean_command import clean_command
from config import configure_command
from dependencies import dependencies_command
from interactive.interactive_command import interactive_command
from profile import profile_command
from system.check_system_command import check_system_command
from utils.click import OrderedGroup
from utils.exceptions import ApplicationException
from utils.log import initialize_loggers, default_logger
from utils.p4studio_path import p4studio_path
from utils.yaml_utils import read_yaml_from_file
from workspace import configure_env_variables, in_workspace


@click.group(cls=OrderedGroup)
def p4studio_cli() -> None:
    """
    \b
    p4studio helps to manage SDE and its environment by:
    \b
    - installing dependencies,
    - building and installing SDE components,
    - building and installing P4 programs.

    \b
    If you do not know where to start, run:
      p4studio interactive
    """
    initialize_loggers()


P4STUDIO_COMMANDS = [
    interactive_command,
    profile_command,
    dependencies_command,
    configure_command,
    build_command,
    clean_command,
    check_system_command,
    app_command,
]

for command in P4STUDIO_COMMANDS:
    p4studio_cli.add_command(command)


def default_options() -> Dict[str, object]:
    try:
        return read_yaml_from_file(p4studio_path().joinpath("defaults.yaml").as_posix())
    except FileNotFoundError:
        return {}


def p4studio_main() -> None:
    if in_workspace():
        configure_env_variables()
    try:
        p4studio_cli.main(
            sys.argv[1:],
            standalone_mode=False,
            default_map=default_options(),
            auto_envvar_prefix='P4STUDIO'
        )
    except (
            ClickException,
            ApplicationException,
            PermissionError
    ) as e:
        click.secho('Error: {}'.format(e), err=True, fg='red')

        for handler in default_logger().handlers:
            if isinstance(handler, FileHandler):
                click.secho("\nFor more details, see: {}".format(handler.baseFilename))

        sys.exit(1)
    except Abort:
        sys.exit(1)
