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

from abc import ABC, abstractmethod
from enum import Enum
from pathlib import Path
from typing import List, Callable, Any

from utils.collections import diff, as_list_of_strings
from utils.filesystem_utils import list_all_files, safe_read_lines, write_lines_with_newlines, unlink
from workspace import current_workspace
from workspace.workspace import Workspace


class InstallDirCategory(Enum):
    DEPENDENCIES = "dependencies"


class InstallDirInfoManager(ABC):

    @abstractmethod
    def files_of_category(self, category: InstallDirCategory) -> List[str]:
        pass

    @abstractmethod
    def append_files_of_category(self, category: InstallDirCategory, new_deps: List[str]) -> None:
        pass

    @abstractmethod
    def record_changes(self, fn: Callable, manifest_category: InstallDirCategory,
                       install_dir: Path,
                       *args: Any) -> None:
        pass

    def clear_category(self, category: InstallDirCategory) -> None:
        pass


class ManifestFileInstallDirInfoManager(InstallDirInfoManager):
    category_filename_mapping = {
        InstallDirCategory.DEPENDENCIES: ".dependencies-manifest"
    }

    def __init__(self, workspace: Workspace) -> None:
        self.manifests_location = workspace.default_install_dir

    def _ensure_path_to_manifests_location_exists(self) -> None:
        self.manifests_location.mkdir(parents=True, exist_ok=True)

    def append_files_of_category(self, category: InstallDirCategory, new_files: List[str]) -> None:
        self._ensure_path_to_manifests_location_exists()
        path_to_manifest_file = self._get_manifest_location(category)
        lines = set(safe_read_lines(path_to_manifest_file)).union(set(new_files))
        write_lines_with_newlines(path_to_manifest_file, lines)

    def files_of_category(self, category: InstallDirCategory) -> List[str]:
        return safe_read_lines(self._get_manifest_location(category)) + [str(self._get_manifest_location(category))]

    def _get_manifest_location(self, category: InstallDirCategory) -> Path:
        return self.manifests_location / self.category_filename_mapping[category]

    def clear_category(self, category: InstallDirCategory) -> None:
        unlink(self._get_manifest_location(category), missing_ok=True)

    def record_changes(self, fn: Callable, manifest_category: InstallDirCategory, install_dir: Path,
                       *args: Any, **kwargs: Any) -> None:
        before_files = list_all_files(install_dir, True)
        fn(*args, **kwargs)
        after_files = list_all_files(install_dir, True)

        new_files = diff(as_list_of_strings(after_files), as_list_of_strings(before_files))
        self.append_files_of_category(manifest_category, new_files)


def install_dir_info_manager() -> InstallDirInfoManager:
    return ManifestFileInstallDirInfoManager(current_workspace())
