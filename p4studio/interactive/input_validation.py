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
from pathlib import Path
from typing import Callable, Any, List

from inquirer.errors import ValidationError

from system.checks import check_kernel_headers
from utils.filesystem_utils import check_read_access_rights
from utils.os_info import get_system_kdir_path


def validate_minimum_selected_options(minimum_options_number: int) -> Callable[[object, List[object]], bool]:
    def result(_: object, selected: List[object]) -> bool:
        if len(selected) < minimum_options_number:
            raise ValidationError("", reason="You have to choose minimum {} option{}"
                                  .format(minimum_options_number, "s" if minimum_options_number > 1 else ""))
        return True

    return result


def validate_file_to_read(_: object, file_path: str) -> bool:
    path_to_validate = os.path.expanduser(file_path)
    _validate_not_empty(file_path)
    _validate_path_is_file(path_to_validate)
    _validate_read_rights(path_to_validate)
    return True


def validate_kernel_headers_input(_: object, file_path: str) -> bool:
    check_kernel_headers_result = check_kernel_headers(Path(file_path) if file_path else None)
    if not check_kernel_headers_result.ok:
        raise ValidationError(file_path,
                              reason="Cannot find headers for your kernel version in location: {}".format(
                                  file_path or get_system_kdir_path()))
    if not file_path:
        return True
    path_to_validate = os.path.expanduser(file_path)
    _validate_path_is_dir(path_to_validate)
    _validate_read_rights(path_to_validate)
    return True


def validate_dir_to_read(_: object, file_path: str) -> bool:
    path_to_validate = os.path.expanduser(file_path)
    _validate_not_empty(file_path)
    _validate_path_is_dir(path_to_validate)
    _validate_read_rights(path_to_validate)
    return True


def validate_path_to_write(_: object, file_path: str) -> bool:
    path_to_validate = os.path.expanduser(file_path)
    _validate_not_empty(file_path)
    _validate_existence_of_dir(path_to_validate)
    _validate_file_is_not_dir(path_to_validate)
    return True


def _validate_not_empty(value: str) -> None:
    if not value or value == "":
        raise ValidationError(value,
                              reason="Empty input.")


def _validate_path_is_file(path_to_validate: str) -> None:
    if not os.path.isfile(path_to_validate):
        raise ValidationError(path_to_validate,
                              reason="{} is not a file.".format(path_to_validate))


def _validate_path_is_dir(path_to_validate: str) -> None:
    if not os.path.isdir(path_to_validate):
        raise ValidationError(path_to_validate,
                              reason="{} is not a directory.".format(path_to_validate))


def _validate_read_rights(path_to_validate: str) -> None:
    if not check_read_access_rights(path_to_validate):
        raise ValidationError(path_to_validate,
                              reason='Cannot read {} due to insufficient permissions.'.format(path_to_validate))


def _validate_file_is_not_dir(path_to_validate: str) -> None:
    if os.path.isdir(path_to_validate):
        raise ValidationError(path_to_validate, reason="{} is a directory.".format(path_to_validate))


def _validate_existence_of_dir(path_to_validate: str) -> bool:
    dir_path = os.path.dirname(path_to_validate)
    if dir_path != "" and not os.path.exists(dir_path):
        raise ValidationError(path_to_validate,
                              reason="Directory {} doesn't exist.".format(dir_path))
    return True
