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
import sys
from typing import Any, List

import click

from utils.log import green_logger, default_logger, logging_verbosity


def print_green(formatter: str, *args: Any) -> None:
    message = formatter.format(*args)
    green_logger().info(message)


def print_normal(formatter: str = "", *args: Any, **kwargs: Any) -> None:
    message = formatter.format(*args, **kwargs)
    default_logger().info(message)


def print_separator() -> None:
    print_normal()


def print_debug(formatter: str = "", *args: Any) -> None:
    message = formatter.format(*args)
    default_logger().debug(message)


def print_warning(formatter: str = "", *args: Any) -> None:
    message = formatter.format(*args)
    default_logger().warn(message)


def columnize(items: List[str], number_of_columns: int, gap: int = 1) -> str:
    # add empty items to make sure that all columns can have the sie sizes
    column_alignment = number_of_columns - len(items) % number_of_columns
    if column_alignment < number_of_columns:
        items = items + column_alignment * ['']

    column_size = int(len(items) / number_of_columns)
    max_item_len = max(len(item) for item in items)
    item_formatter = "{:<%s}" % max_item_len

    rows = []
    for row_index in range(0, column_size):
        row = []
        for column_index in range(0, number_of_columns):
            index = column_index * column_size + row_index
            fixed_width_item = item_formatter.format(items[index])
            row.append(fixed_width_item)
        rows.append(row)

    column_separator = gap * ' '
    lines = [column_separator.join(row) for row in rows]
    return '\n'.join(lines)


class CompactLog:
    def __init__(self) -> None:
        self.buffer = []
        self.prefix = ""
        self.width = 100

    def log(self, message: str, skip_prefix: bool = False) -> None:
        print_debug("{}", message)
        self.buffer.append(message)
        if logging_verbosity() != 'DEBUG' and sys.stdout.isatty():
            self._reset_line()
            line = message[:self.width - 3] + (message[self.width - 3:] and '...')
            prefix = "" if skip_prefix else self.prefix
            click.echo("{}{}".format(prefix, line), nl=False)

    def done(self, success: bool) -> None:
        if success:
            self.log("✓")
            self.reset(include_last=True)
        else:
            self.log("error")
            self.dump()

    def dump(self) -> None:
        self._reset_line()
        if self.buffer:
            for line in self.buffer[:-1]:
                click.echo(line)
            self._log_last_line()
        self.buffer = []

    def reset(self, include_last: bool = False) -> None:
        self._reset_line()
        if include_last and self.buffer:
            self._log_last_line()
        self.buffer = []

    def start_new(self, prefix: str = "") -> None:

        if self.buffer:
            raise RuntimeError("internal error")
        if prefix is not None:
            self.prefix = prefix
        self.log(prefix, skip_prefix=True)

    def _log_last_line(self) -> None:
        if self.buffer:
            last_line = self.buffer[-1]
            click.echo("{}{}".format(self.prefix, last_line))

    @staticmethod
    def _reset_line() -> None:
        if sys.stdout.isatty():
            click.echo("\x1b[1K\r", nl=False)


_GLOBAL_COMPACT_LOG = CompactLog()


def compact_log() -> CompactLog:
    global _GLOBAL_COMPACT_LOG
    return _GLOBAL_COMPACT_LOG
