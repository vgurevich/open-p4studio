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

from typing import List

from utils.terminal import print_normal, print_green


class CheckResult:
    def __init__(self, name: str, value: str, ok: bool):
        self.ok = ok
        self.name = name
        self.value = value

    def __str__(self) -> str:
        return " {} {}: {}".format('✓' if self.ok else '✗', self.name, self.value)


def print_multiple_checks(check_results: List[CheckResult]) -> None:
    print_green("Checking system capabilities to build and install SDE:")
    for check_result in check_results:
        print_normal(str(check_result))
