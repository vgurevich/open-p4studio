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
import shutil
import tempfile
from urllib import request

from utils.exceptions import ApplicationException
from utils.terminal import compact_log


def download(url: str) -> str:
    file_name = tempfile.mkdtemp() + '/' + url.split("/")[-1]
    try:
        compact_log().log("downloading: {}".format(url))
        with request.urlopen(url) as response, open(file_name, 'wb') as out_file:
            shutil.copyfileobj(response, out_file)
    except Exception as e:
        compact_log().log(str(e))
        raise ApplicationException from e
    return file_name
