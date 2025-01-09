from typing import Sequence

from config.config_option import ConfigOption
from config.configuration_manager import current_configuration_manager

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


def sorted_by_parenthood(options: Sequence[ConfigOption]) -> Sequence[ConfigOption]:
    parent_options = [o for o in options if is_parent_option(o.p4studio_name)]
    parent_option_names = [o.p4studio_name for o in parent_options]
    rest = [o for o in options if o.p4studio_name not in parent_option_names]
    return parent_options + rest


def is_parent_option(option: str) -> bool:
    return current_configuration_manager().definition(option).category.lower() == option
