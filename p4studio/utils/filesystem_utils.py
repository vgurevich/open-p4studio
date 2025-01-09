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

import os
import re
from pathlib import Path
from typing import List, Optional, Iterable

from utils.collections import flatten


def find_file_in_directories(filename: str, directories: List[Path]) -> Optional[Path]:
    possible_file_paths = flatten(
        [[found_path for found_path in directory.iterdir() if
          re.match(filename, found_path.name)] for directory in directories])
    return possible_file_paths[0].absolute() if not len(possible_file_paths) == 0 else None


def check_read_access_rights(path: str) -> bool:
    return os.access(path, os.R_OK)


def ensure_path_is_absolute(path: str) -> str:
    return os.path.abspath(os.path.expanduser(path))


def list_all_files(path: Path, recursively: bool) -> List[Path]:
    all_elements = path.rglob("*") if recursively else path.glob("*")
    return [x for x in all_elements if x.is_file()]


def remove_empty_dirs_recursively(path: Path) -> None:
    files_in_dir = list(path.iterdir())
    dirs_in_dir = [f for f in files_in_dir if f.is_dir()]
    for directory in dirs_in_dir:
        remove_empty_dirs_recursively(directory)
    directories_after = list(path.iterdir())
    if not directories_after:
        path.rmdir()


def safe_read_lines(path: Path) -> List[str]:
    try:
        with open(str(path)) as file:
            return [x.strip() for x in file.readlines()]
    except IOError:
        return []


def write_lines_with_newlines(path: Path, lines: Iterable[str]) -> None:
    with open(str(path), "w") as file:
        for line in lines:
            file.write(line.strip() + "\n")


def unlink(path: Path, missing_ok: bool = False) -> None:
    try:
        path.unlink()
    except FileNotFoundError as e:
        if not missing_ok:
            raise e


def delete_files_and_clean_dir_structure(files_to_delete: List[Path]) -> None:
    common_directory = Path(os.path.commonprefix([str(it) for it in files_to_delete]))
    for f in files_to_delete:
        unlink(f, missing_ok=True)
    remove_empty_dirs_recursively(common_directory)
