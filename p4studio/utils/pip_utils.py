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
import glob
import operator
import re
import os
import subprocess
from pathlib import Path
from typing import Tuple, List, Optional, Union

from utils.distutils_utils import get_python_lib_dir
from utils.processes import execute, check_command


def pip_install(
        install_dir: Path,
        os_name: str,
        packages: List[str],
        download_cache_dir: Path,
        pythonpath: Optional[str] = None) -> None:
    pip_download(download_cache_dir, packages, pythonpath)

    target_path = python_packages_target_path(install_dir, os_name)
    command = ['python3', '-m', 'pip', 'install']
    command += ['--no-index', '--find-links', download_cache_dir.as_posix()]

    if install_dir != Path('/usr/local'):
        if pip_version(pythonpath) < (18, 0):
            # Parameter '--system' is needed for oldest versions of pip
            # https://stackoverflow.com/questions/4495120/combine-user-with-prefix-error-with-setup-py-install
            command += ['--system']
        command += ['--upgrade', '--target', target_path.as_posix()]

    command += list(packages)
    env = os.environ.copy()
    if pythonpath:
        # FIXME(#18): This pathing should be set up much earlier.
        current_pythonpath = env.get("PYTHONPATH", "")
        updated_pythonpath = os.pathsep.join([current_pythonpath, str(pythonpath)]) if current_pythonpath else str(pythonpath)
        env["PYTHONPATH"] = updated_pythonpath

    execute(command, override_env=env)


def pip_packages_in_cache(download_cache: Path, packages: List[str], pythonpath: Optional[str] = None) -> bool:
    command = ['python3', '-m', 'pip', 'download']
    command += ['--dest', download_cache.as_posix(), '--find-links', download_cache.as_posix(), '--no-index']
    command += packages
    env = os.environ.copy()
    if pythonpath:
        # FIXME(#18): This pathing should be set up much earlier.
        current_pythonpath = env.get("PYTHONPATH", "")
        updated_pythonpath = os.pathsep.join([current_pythonpath, str(pythonpath)]) if current_pythonpath else str(pythonpath)
        env["PYTHONPATH"] = updated_pythonpath
    return check_command(command, override_env=env)


def pip_download(destination_dir: Path, packages: List[str], pythonpath: Optional[str] = None) -> None:
    if not pip_packages_in_cache(destination_dir, packages, pythonpath):
        command = ['python3', '-m', 'pip', 'download', '--dest', destination_dir.as_posix()] + packages
        env = os.environ.copy()
        if pythonpath:
            # FIXME(#18): This pathing should be set up much earlier.
            current_pythonpath = env.get("PYTHONPATH", "")
            updated_pythonpath = os.pathsep.join([current_pythonpath, str(pythonpath)]) if current_pythonpath else str(pythonpath)
            env["PYTHONPATH"] = updated_pythonpath
        execute(command, override_env=env)


def python_packages_target_path(install_dir: Path, os_name: str) -> Path:
    system_wide_installation = install_dir == Path('/usr/local')
    python_packages = 'dist-packages' if system_wide_installation and os_name != 'CentOS' else 'site-packages'
    target_path = install_dir / get_python_lib_dir() / python_packages
    return target_path


def pip_list(install_dir: Path, os_name: str) -> List[str]:
    target_path = python_packages_target_path(install_dir, os_name)
    directories = glob.glob("{}/*.dist-info".format(target_path.as_posix()))
    return [Path(d).stem.replace("-", "==") for d in directories]


def pip_version(pythonpath: Optional[str]) -> Tuple[int, ...]:
    pattern = r"(\d+\.{1}\d+)"
    command = "python3 -m pip --version"
    if pythonpath is not None:
        command = "PYTHONPATH={} {}".format(pythonpath, command)

    pv = re.search(pattern, subprocess.getoutput(command)).group()  # type: ignore
    version = tuple(int(a) for a in pv.split("."))
    return version


def check_if_pip_packages_installed(install_dir: Path, os_name: str, packages: List[str]) -> bool:
    installed = [PythonPackageRequirement.parse(r) for r in pip_list(install_dir, os_name)]
    requirements = [PythonPackageRequirement.parse(r) for r in packages]
    for requirement in requirements:
        if not any(i.satisfies(requirement) for i in installed):
            return False
    return True


class PythonPackageRequirement:
    pattern = re.compile("(?P<name>[a-zA-Z0-9_-]+)(?P<constraints>.*)$")
    version_constraint_pattern = re.compile('(?P<operator>((>|<)=?)|(==))(?P<version>[0-9.]+)')

    def __init__(self, name: str, *version_constraints: Tuple[str, Tuple[int, ...]]):
        self.name = name
        self.version_constraints = version_constraints

    def satisfies(self, other: Union[str, 'PythonPackageRequirement']) -> bool:
        if len(self.version_constraints) != 1 or self.version_constraints[0][0] != '==':
            raise NotImplementedError()
        my_version = self.version_constraints[0][1]

        if isinstance(other, str):
            other = PythonPackageRequirement.parse(other)

        if self.name.lower() != other.name.lower():
            return False

        operators = {
            '==': operator.eq,
            '>': operator.gt,
            '>=': operator.ge,
            '<': operator.lt,
            '<=': operator.le,
        }
        for (operator_symbol, version) in other.version_constraints:
            if not operators[operator_symbol](my_version, version):
                return False
        return True

    def __eq__(self, other: object) -> bool:
        return isinstance(other, PythonPackageRequirement) and \
               self.name == other.name and \
               self.version_constraints == other.version_constraints

    def __repr__(self) -> str:
        result = self.name
        result += ','.join([n + '.'.join([str(x) for x in v]) for (n, v) in self.version_constraints])
        return result

    @staticmethod
    def parse(requirement: str) -> 'PythonPackageRequirement':
        match = PythonPackageRequirement.pattern.match(requirement)
        if not match:
            raise ValueError("Incorrect format of package requirement: {}".format(requirement))
        name = match.group("name")
        constraints = match.group("constraints")
        if constraints:
            version_constraints = [
                PythonPackageRequirement.parse_version_constraint(c)
                for c in constraints.split(',')
            ]
        else:
            version_constraints = []
        return PythonPackageRequirement(name, *version_constraints)

    @staticmethod
    def parse_version_constraint(constraint: str) -> Tuple[str, Tuple[int, ...]]:
        match = PythonPackageRequirement.version_constraint_pattern.match(constraint)
        if not match:
            raise ValueError("Incorrect format of version constraint: '{}'".format(constraint))
        operator = match.group('operator')
        version = tuple([int(x) for x in match.group('version').split('.')])
        return operator, version
