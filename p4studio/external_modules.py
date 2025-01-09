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
import sys


def get_third_party_dir() -> str:
    my_dir = os.path.dirname(os.path.realpath(__file__))
    return my_dir + '/third_party'


def get_requirements_file_path() -> str:
    return get_third_party_dir() + "/requirements.txt"


def add_third_party_deps() -> None:
    sys.path.insert(0, get_third_party_dir())
