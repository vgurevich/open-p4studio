#!/usr/bin/env python3
from typing import Any

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

from utils.exceptions import ApplicationException


def user_provided_format(formatter: str, **kwargs: Any) -> str:
    try:
        return formatter.format(**kwargs)
    except KeyError as e:
        available_keys = ', '.join(kwargs.keys())
        message = "Incorrect key. {} does not match any of: {}.".format(e, available_keys)
        raise ApplicationException(message)
    except Exception as e:
        raise ApplicationException(e)
