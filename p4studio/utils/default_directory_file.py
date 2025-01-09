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
from typing import List, Optional, Any

import click
from click import Parameter, Context

from utils.exceptions import ApplicationException
from utils.terminal import print_normal


class DefaultDirectoryFile(click.File):
    def __init__(self, default_directory: str, accepted_extensions: List[str] = None, logging_name: str = 'file',
                 mode: str = 'r',
                 encoding: str = None,
                 errors: str = 'strict', lazy: bool = False,
                 atomic: bool = False):
        super().__init__(mode, encoding, errors, lazy, atomic)
        self._default_directory = default_directory
        self._accepted_extensions = accepted_extensions
        self._logging_name = logging_name

    def convert(self, value: Any, param: Optional[Parameter], ctx: Optional[Context]) -> Any:
        path_to_file = self._get_path_or_find_in_default_dir(value)
        return super(DefaultDirectoryFile, self).convert(path_to_file, param, ctx)

    def _get_path_or_find_in_default_dir(self, value: str) -> str:
        if not is_only_filename(value) or not self._default_directory:
            return value
        print_normal("{0}: {1} not found. Checking in {2}"
                     .format(self._logging_name.capitalize(), value, self._default_directory))
        return self._find_in_default_dir(value)

    def _find_in_default_dir(self, filename: str) -> str:
        files_in_default_directory = os.listdir(self._default_directory)
        processed_filename = self._get_existing_file_with_same_basename(files_in_default_directory, filename)
        return os.path.join(self._default_directory, processed_filename)

    def _get_existing_file_with_same_basename(self, files_in_default_dir: List[str], filename: str) -> str:
        possible_filenames = self._get_possible_filenames(filename, files_in_default_dir)
        if len(possible_filenames) > 1:
            raise ApplicationException(
                "Given {0} name is ambiguous. Found more than one {0} with this "
                "name and accepted extension{1}. Please provide full path to this file."
                .format(self._logging_name, self._accepted_extensions_message()))
        elif len(possible_filenames) == 0:
            raise ApplicationException(
                "Not found any {0} with given name and accepted extension{1} in {2}."
                .format(self._logging_name, self._accepted_extensions_message(), self._default_directory))
        return possible_filenames[0]

    def _get_possible_filenames(self, basename: str, available_filenames: List[str]) -> List[str]:
        result = []
        for available_filename in available_filenames:
            available_basename, available_filename_extension = os.path.splitext(available_filename)
            if available_basename == basename:
                if not self._accepted_extensions:
                    result.append(available_filename)
                elif available_filename_extension in self._accepted_extensions:
                    result.append(available_filename)
        return result

    def _accepted_extensions_message(self) -> str:
        return "({})".format(", ".join(self._accepted_extensions)) if self._accepted_extensions else ""


def is_only_filename(value: str) -> bool:
    _, extension = os.path.splitext(value)
    return not (os.path.isfile(value) or '/' in value or extension != '')
