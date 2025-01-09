# Copyright (c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from setuptools import find_packages, setup

setup(
    name="bf_pktpy",
    version="0.3",
    author="Intel",
    author_email="info@intel.com",
    description="Intel traffic generator tool",
    url="https://github.com/barefootnetworks/bf-pktpy",
    include_package_data=True,
    packages=find_packages(exclude=("tests/integration",)),
    python_requires=">=2.7",
    license="Apache License Version 2.0",
    install_requires=[],
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
    ],
    entry_points={
        "console_scripts": [
            "pktpy = bf_pktpy.main:run_interactive",
            "bf-pktpy = bf_pktpy.main:run_interactive",
            "bf_pktpy = bf_pktpy.main:run_interactive",
        ]
    },
)
