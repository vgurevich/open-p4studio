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

import locale
import logging
import os
from datetime import datetime
from pathlib import Path
from typing import cast, Any, Callable, Optional

import click
import click_logging
from click import Choice
from click_logging import ClickHandler

from utils.decorators import multiple_decorators
from utils.p4studio_path import p4studio_path
from utils.types import PathLike

_DEFAULT_LOGGER = None
_GREEN_LOGGER = None
VERBOSITY = 'DEBUG'


def logging_verbosity() -> str:
    global VERBOSITY
    return VERBOSITY


def default_logger() -> logging.Logger:
    global _DEFAULT_LOGGER
    return cast(logging.Logger, _DEFAULT_LOGGER)


def green_logger() -> logging.Logger:
    global _GREEN_LOGGER
    return cast(logging.Logger, _GREEN_LOGGER)


def initialize_loggers() -> None:
    global _DEFAULT_LOGGER
    global _GREEN_LOGGER

    _DEFAULT_LOGGER = logging.getLogger("default-logger")
    _GREEN_LOGGER = logging.getLogger("green-logger")

    for logger in [_DEFAULT_LOGGER, _GREEN_LOGGER]:
        logger.setLevel(logging.DEBUG)
        click_logging.basic_config(logger)

    _GREEN_LOGGER.handlers[0].formatter = SingleColorFormatter('green')


class SingleColorFormatter(logging.Formatter):
    def __init__(self, color: str):
        self.color = color

    def format(self, record: logging.LogRecord) -> str:
        if not record.exc_info:
            msg = record.getMessage()
            return click.style(msg, fg=self.color)
        return logging.Formatter.format(self, record)


def _set_verbose(ctx: click.Context, param: click.Parameter, value: str) -> None:
    global VERBOSITY
    VERBOSITY = value
    level = logging.getLevelName(value)
    for logger in [default_logger(), green_logger()]:
        for handler in logger.handlers:
            if isinstance(handler, ClickHandler):
                handler.setLevel(level)


def _set_log_file(ctx: click.Context, param: click.Parameter, value: Any) -> None:
    if value is None:
        return

    filename = str(value)

    os.makedirs(os.path.dirname(filename), exist_ok=True)
    file_handler = logging.FileHandler(filename, encoding=locale.getpreferredencoding(True))
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(logging.Formatter('%(asctime)s: %(message)s'))

    for logger in [default_logger(), green_logger()]:
        logger.addHandler(file_handler)


def p4studio_logs_dir() -> Path:
    return p4studio_path() / 'logs'


def default_log_file_name() -> str:
    return datetime.now().strftime(str(p4studio_logs_dir() / 'p4studio_%Y-%m-%d_%H-%M-%S.log'))


def logging_options(verbosity: str = 'DEBUG', log_file: Optional[PathLike] = None) -> Callable:
    return multiple_decorators(
        click.option(
            '--verbosity',
            type=Choice(['INFO', 'DEBUG']),
            default=verbosity,
            help='Show more information',
            show_default=True,
            callback=_set_verbose,
            expose_value=False
        ),
        click.option(
            '--log-file',
            type=click.Path(writable=True, dir_okay=False),
            default=log_file,
            help='Save logs to file',
            show_default=True,
            callback=_set_log_file,
            expose_value=False
        )
    )
