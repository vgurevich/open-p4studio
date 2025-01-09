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
from typing import Optional

import click

from system.check_system_utils import print_multiple_checks
from system.checks import check_kernel_headers, get_initial_checks
from utils.exceptions import ApplicationException
from utils.terminal import print_green


@click.command("check-system", short_help="Verify that system is capable to build and install SDE")
@click.option("--install-dir", help="Directory where SDE should be installed")
@click.option("--asic", is_flag=True, default=False, help="Check if system can be used to build for ASIC")
@click.option('--kdir', type=click.Path(file_okay=False), help="Path to Kernel headers")
def check_system_command(install_dir: Optional[str], asic: bool, kdir: Optional[str]) -> None:
    """
    Perform basic checks to verify that system is capable to build and install SDE
    """

    checks = get_initial_checks(Path(install_dir) if install_dir is not None else None)

    if asic:
        checks.append(check_kernel_headers(Path(kdir) if kdir else None))

    print_multiple_checks(checks)

    if any(not check.ok for check in checks):
        raise ApplicationException("At least one check failed")
    else:
        print_green("Checking system completed successfully.")
