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

from dependencies.dependency_manager import dependency_manager, DependencyManager
from utils.processes import execute


class SourceDependencyConfig:
    def __init__(
            self,
            os_name: str,
            os_version: str,
            build_dir: Path,
            download_dir: Path,
            install_dir: Path,
            jobs: int,
            with_proto: str,
            force: bool
    ):
        self.os_name = os_name
        self.os_version = os_version
        self._download_dir = download_dir
        self._build_dir = build_dir
        self.install_dir = install_dir
        self.jobs = jobs
        self.with_proto = with_proto
        self.force = force

    def download_dir(self, ensure_exists: bool) -> Path:
        if ensure_exists and not self._build_dir.exists():
            execute("mkdir -p {}".format(self._download_dir))
        return self._download_dir

    def build_dir(self, copy_download_dir: bool = False) -> Path:
        if not self._build_dir.exists():
            execute("mkdir -p {}".format(self._build_dir))

        if copy_download_dir:
            execute("cp -a {}/. {}".format(self._download_dir / '.', self._build_dir))

        return self._build_dir

    def dependency_manager(self) -> DependencyManager:
        return dependency_manager(self.os_name, self.os_version)

    @property
    def os(self) -> str:
        return "{}:{}".format(self.os_name, self.os_version)
