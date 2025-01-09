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
from typing import Iterable

from utils.processes import command_output

class OsInfo:
    CANONICAL_NAMES = {
        'ubuntu': 'Ubuntu',
        'centos': 'CentOS',
        'debian': 'Debian',
        'pop': 'Ubuntu'
    }

    @staticmethod
    def os_release() -> 'OsInfo':
        with open('/etc/os-release', 'r') as file:
            return OsInfo(line for line in file)

    def __init__(self, lines: Iterable[str]):
        self.data = {}
        for line in lines:
            if not line or line.isspace():
                continue
            name, value = line.rstrip().split('=')
            self.data[name] = value.strip('"')

    @property
    def name(self) -> str:
        os_name = self.data['ID'].lower()
        return self.canonicalize(os_name)

    @property
    def version(self) -> str:
        return self.data['VERSION_ID']

    def canonicalize(self, os_name: str) -> str:
        return self.CANONICAL_NAMES.get(os_name, os_name)


os_info = OsInfo.os_release()


def get_system_kdir_path() -> Path:
    kernel_version = command_output(["uname", "-r"]).decode().strip()
    kdir = Path("/lib/modules/{}/build".format(kernel_version))
    return kdir
