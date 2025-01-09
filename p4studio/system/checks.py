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

from os import statvfs
from pathlib import Path
from shutil import which
from typing import List, Optional

from dependencies.dependency_manager import is_os_supported
from system.check_system_utils import CheckResult
from utils.os_info import get_system_kdir_path, os_info
from workspace import current_workspace

BASIC_TOOLS = ["sudo"]


def get_initial_checks(install_dir: Optional[Path] = None) -> List[CheckResult]:
    if not install_dir:
        install_dir = current_workspace().p4studio_path
    checks = [check_disk_space(install_dir), check_tmp_space(), check_os(), check_basic_tools_are_installed()]
    return checks


def check_disk_space(path: Path) -> CheckResult:
    fs = statvfs(str(path))
    free_space_gb = fs.f_bsize * fs.f_bavail / 1024 / 1024 / 1024
    min_free_space_gb = 20

    name = "Free space >= {}GB".format(min_free_space_gb)
    info = "{:.2f}GB".format(free_space_gb)
    ok = free_space_gb >= min_free_space_gb
    return CheckResult(name, info, ok)


def check_tmp_space() -> CheckResult:
    fs = statvfs('/tmp')
    free_space = fs.f_bsize * fs.f_bavail / 1024 / 1024 / 1024
    min_tmp_space = 2.5
    name = "Free space in /tmp >= {}GB".format(min_tmp_space)
    info = "{:.2f}GB".format(free_space)
    ok = free_space >= min_tmp_space
    return CheckResult(name, info, ok)


def check_os() -> CheckResult:
    name = "OS is supported"
    ok = is_os_supported(os_info.name, os_info.version)
    info = "{} {}".format(os_info.name, os_info.version)

    return CheckResult(name, info, ok)


def check_kernel_headers(kdir: Optional[Path]) -> CheckResult:
    if not kdir:
        kdir = get_system_kdir_path()
    ok = kdir.exists()
    info = str(kdir) + (" exists" if ok else " not exist")

    return CheckResult("Kernel headers installed", info, ok)


def check_basic_tools_are_installed() -> CheckResult:
    name = "Basic tools are installed"
    basic_tools_existence = [(basic_tool, bool(which(basic_tool))) for basic_tool in BASIC_TOOLS]
    ok = all([do_exist for _, do_exist in basic_tools_existence])
    info = ", ".join(["{} {}".format(tool, ('✓' if do_exist else '✗')) for tool, do_exist in basic_tools_existence])

    return CheckResult(name, info, ok)
