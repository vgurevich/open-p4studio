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
from shutil import which

from dependencies.source.source_dependency_config import SourceDependencyConfig
from utils.processes import execute

_BRIDGE_UTILS_FILE = "bridge-utils.tar.xz"


def download_bridge_utils(config: SourceDependencyConfig) -> None:
    url = config.dependency_manager().source_dependency_attributes("bridge")["url"]
    download_dir = config.download_dir(ensure_exists=True)
    thrift_package = download_dir / _BRIDGE_UTILS_FILE
    if not thrift_package.exists():
        execute("wget {} -nv -O {}".format(url, thrift_package))


def install_bridge_utils(config: SourceDependencyConfig) -> None:

    if not config.force and _is_bridge_installed(config.install_dir):
        return

    download_bridge_utils(config)
    build_dir = config.build_dir()
    bridge_utils_tarball = config.download_dir(ensure_exists=False) / _BRIDGE_UTILS_FILE
    execute("tar xf {} --strip-components 1 -C {}".format(bridge_utils_tarball, build_dir))
    execute("autoconf", build_dir)
    execute("./configure --prefix={}".format(config.install_dir), build_dir)
    execute("make -j{} --silent CFLAGS=-w".format(config.jobs), build_dir)
    execute("make install -j{} --silent CFLAGS=-w".format(config.jobs), build_dir)
    execute("sudo ldconfig")


def _is_bridge_installed(path: Path) -> bool:
    return which((path / 'sbin/brctl').as_posix()) is not None
