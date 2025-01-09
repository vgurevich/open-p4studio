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

from typing import List, Tuple

import click
from click import Choice, Context
from pathlib import Path
from utils.pip_utils import python_packages_target_path
from utils.collections import group_by_to_dict
from utils.log import logging_options
from utils.terminal import print_green
from workspace import current_workspace, in_workspace
from .cmake import cmake, BUILD_TYPES
from .configuration_manager import current_configuration_manager


def _allowed_options() -> List[str]:
    if not in_workspace():
        return []
    options = current_configuration_manager().known_p4studio_options_including_negated()
    return sorted(options, key=lambda x: x.replace("^", "z"))


def _describe_configure_command() -> str:
    result = 'Configure SDE build options'

    definitions = current_configuration_manager().definitions
    definition_by_category = group_by_to_dict(definitions, lambda d: d.category)

    result += "\n\nBuild can be configured with following CONFIG options:\n\n\b\n"
    for category, options in definition_by_category.items():
        result += " {}\n".format(category)
        for option in options:
            default = 'enabled' if option.default else 'disabled'
            result += "  - {name:<27} {desc}. ".format(
                name=option.p4studio_name,
                desc=option.description,
            ) + ("Default: {}\n".format(default) if option.default is not None else "\n")
    return result


@click.command('configure', help=_describe_configure_command(), short_help='Configure SDE build options')
@click.argument('options', type=Choice(_allowed_options()), nargs=-1, metavar="[CONFIG|^CONFIG]...")
@logging_options()
@click.option('--build-type', type=Choice(BUILD_TYPES), default='Release', help="Build type")
@click.option('--install-prefix', type=click.Path(file_okay=False, writable=True), default=None,
              help="Install files in DIRECTORY")
@click.option('--dependencies-dir', type=click.Path(file_okay=False, resolve_path=True), default=None,
              help="Use dependencies installed in DIRECTORY")
@click.option('--bsp-path', type=click.Path(exists=True), help="Install BSP package")
@click.option('--p4ppflags', help="P4 preprocessor flags")
@click.option('--extra-cppflags', help="Extra C++ compiler flags")
@click.option('--p4flags', help="P4 compiler flags")
@click.option('--kdir', type=click.Path(file_okay=False), help="Path to Kernel headers")
@click.pass_context
def configure_command(
        context: Context,
        build_type: str,
        install_prefix: str,
        dependencies_dir: str,
        options: Tuple[str, ...],
        bsp_path: str,
        p4ppflags: str,
        p4flags: str,
        extra_cppflags: str,
        kdir: str) -> None:
    print_green("Configuring {} build...", current_workspace().name)

    all_options = list(options)
    if bsp_path:
        all_options.append("bsp")

    if not install_prefix:
        install_prefix = str(current_workspace().default_install_dir)

    cm = current_configuration_manager()
    args = cm.convert_p4studio_args_to_cmake_options(all_options)
    add_arg_if_not_none(args, "CMAKE_BUILD_TYPE", build_type)
    add_arg_if_not_none(args, "CMAKE_INSTALL_PREFIX", install_prefix)
    add_arg_if_not_none(args, "SDE_DEPENDENCIES", dependencies_dir)
    add_arg_if_not_none(args, "EXTRA_CPPFLAGS", extra_cppflags)
    add_arg_if_not_none(args, "P4FLAGS", p4flags)
    add_arg_if_not_none(args, "P4PPFLAGS", p4ppflags)
    add_arg_if_not_none(args, "KDIR", kdir)
    add_arg_if_not_none(args, "CMAKE_LINKER", "lld")
    build_dir = current_workspace().root_path / 'build'
    # FIXME(#18): This pathing should be set up much earlier. Also remove, the TODO.
    py_path = python_packages_target_path(Path(install_prefix), "TODO")
    cmake(build_dir, py_path, args)
    print_green("{} build configured.", current_workspace().name)


def add_arg_if_not_none(args: List[str], arg: str, value: str) -> None:
    if value is not None:
        args.append("-D{}='{}'".format(arg, value))
