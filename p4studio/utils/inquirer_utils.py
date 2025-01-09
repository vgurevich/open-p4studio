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

from typing import Callable, Any, Optional, cast, TypeVar, Tuple, List, Sequence

import inquirer
from inquirer.events import KeyEventGenerator
from inquirer.questions import TaggedValue
from inquirer.render import ConsoleRender
from inquirer.render.console.base import BaseConsoleRender
from inquirer.themes import term, Default, Theme

from click import echo


class P4StudioTheme(Default):
    def __init__(self) -> None:
        super(P4StudioTheme, self).__init__()
        self.Question.mark_color = term.green
        self.Question.brackets_color = term.normal
        self.Question.default_color = term.normal
        self.Editor.opening_prompt_color = term.bright_black
        self.Checkbox.selection_color = term.yellow
        self.Checkbox.selection_icon = '➤'
        self.Checkbox.selected_icon = '✓'
        self.Checkbox.selected_color = term.green + term.bold
        self.Checkbox.unselected_color = term.normal
        self.Checkbox.unselected_icon = '✗'
        self.List.selection_color = term.yellow
        self.List.selection_cursor = '➤'
        self.List.unselected_color = term.normal


class MyConsoleRender(ConsoleRender):
    _message_template = "{t.move_up}{t.clear_eol}{t.normal}{msg}"

    def __init__(self, event_generator: Optional[KeyEventGenerator] = None, theme: Optional[Theme] = None, *args: Any,
                 **kwargs: Any) -> None:
        super(MyConsoleRender, self).__init__(event_generator=event_generator, theme=theme, *args, **kwargs)

    def _print_header(self, render: BaseConsoleRender) -> None:
        current_value = self._escape_value(render.get_current_value())
        default_value = self._get_default_value(render)
        show_default = render.show_default if render.question.default else False
        header = self._get_header(render) + (default_value if show_default else '')
        prepared_base = '\n{template}: {value}'.format(template=self._message_template, value=current_value)
        self.print_str(
            base=prepared_base,
            msg=header,
            lf=not render.title_inline,
            tq=self._theme.Question)

    def print_str(self, base: str, lf: bool = False, **kwargs: Any) -> None:
        if lf:
            self._position += 1
        echo(base.format(t=self.terminal, **kwargs), nl=lf)

    def _get_default_value(self, render: BaseConsoleRender) -> str:
        return ' ({color}{default}{normal})'.format(
            default=render.question.default,
            color=self._theme.Question.default_color,
            normal=self.terminal.normal
        )

    @staticmethod
    def _get_header(render: BaseConsoleRender) -> BaseConsoleRender:
        base = render.get_header()
        return base

    @staticmethod
    def _escape_value(value: TaggedValue) -> str:
        return (
            str(value)
            .replace('{', '{{')
            .replace('}', '}}')
        )


def prompt_with_custom_render(fun: Callable[..., object]) -> Callable[..., object]:
    def result(message: str, **kwargs: Any) -> object:
        render = MyConsoleRender(theme=P4StudioTheme())
        return fun(message, render=render, **kwargs)

    return result


class OnlyCustomValidationPath(inquirer.Path):
    def __init__(self, name: str, default: Optional[str] = None, **kwargs: Any):
        super(OnlyCustomValidationPath, self).__init__(name, default=default, **kwargs)

    def validate(self, current: Any) -> None:
        super(inquirer.Path, self).validate(current)


def path_with_only_custom_validation(message: str, render: ConsoleRender, **kwargs: Any) -> str:
    question = OnlyCustomValidationPath(name='', message=message, **kwargs)
    return render.render(question)


def path(message: str, **kwargs: Any) -> str:
    custom_path_fun = prompt_with_custom_render(path_with_only_custom_validation)
    try:
        return cast(str, custom_path_fun(message, **kwargs))
    except ValueError:
        del kwargs["default"]
        return cast(str, custom_path_fun(message, **kwargs))


T = TypeVar('T')


def checkbox(message: str, choices: Sequence[Tuple[str, T]], **kwargs: Any) -> List[T]:
    render = MyConsoleRender(theme=P4StudioTheme())
    return inquirer.shortcuts.checkbox(message, render=render, choices=choices, **kwargs)


def list_input(message: str, choices: Sequence[Tuple[str, T]], **kwargs: Any) -> T:
    render = MyConsoleRender(theme=P4StudioTheme())
    return inquirer.shortcuts.list_input(message, render=render, choices=choices, **kwargs)


def list_confirm(message: str, default: bool = True, **kwargs: Any) -> bool:
    answer = list_input(message, default=default, choices=[("Yes", True), ("No", False)], **kwargs)
    return answer
