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
from collections import OrderedDict
from typing import TypeVar, Set, Iterable, Iterator

T = TypeVar('T')


class OrderedSet(Set[T], Iterable[T]):

    def __init__(self, *args: T):
        self._internal = OrderedDict()  # type: ignore
        for arg in args:
            self.add(arg)

    def add(self, element: T) -> None:
        self._internal[element] = None

    def __iter__(self) -> Iterator[T]:
        return self._internal.keys().__iter__()

    def __eq__(self, other: object) -> bool:
        return self._internal_set() == other

    def _internal_set(self) -> Set[T]:
        return set(self._internal.keys())

    def __bool__(self) -> bool:
        return bool(self._internal_set())

    def __contains__(self, item: object) -> bool:
        return self._internal_set().__contains__(item)

    def __len__(self) -> int:
        return self._internal_set().__len__()

    def __repr__(self) -> str:
        return '{' + ', '.join(str(item) for item in self) + '}'

    def __str__(self) -> str:
        return self.__repr__()
