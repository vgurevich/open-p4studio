#!/bin/bash
#
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

# this script should install OS specific dependencies that are required to run p4studio

set -e

echo
echo "WARNING: install-p4studio-dependencies.sh script is deprecated and will be removed in one of the future releases."
echo

readonly MY_PATH=$(realpath "$0")
readonly MY_DIR=$(dirname "$MY_PATH")

if command -v python3 &>/dev/null; then
  PYTHON3_VERSION=$(python3 -V 2>&1 | sed 's/.* \([0-9]\).\([0-9]\).*/\1\2/')
else
  PYTHON3_VERSION=0
fi

install_apt_dependencies() {
  apt-get update
  apt-get install -y \
    python3 \
    python3-pip \
    sudo
}

install_ubuntu_dependencies() {
  install_apt_dependencies
}

install_debian_dependencies() {
  apt-get update
  apt-get install -y \
    python3 \
    python3-pip \
    sudo

}

install_ubuntu_20.04_dependencies() {
  apt-get update
  apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    sudo
}

install_ubuntu_22.04_dependencies() {
  apt-get update
  apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    sudo
}

install_ubuntu_24.04_dependencies() {
  apt-get update
  apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    sudo
}

install_centos_8_dependencies() {
  yum --enablerepo=extras install -y epel-release
  yum clean expire-cache
  yum --setopt=skip_missing_names_on_install=False install -y \
    python3 \
    python3-pip \
    sudo

}

install_centos_9_dependencies() {
  yum clean expire-cache
  yum --setopt=skip_missing_names_on_install=False install -y \
    python3 \
    python3-pip \
    sudo

}

install_debian_9_dependencies() {
  install_debian_dependencies
}

install_debian_10_dependencies() {
  install_debian_dependencies
}

install_debian_11_dependencies() {
  install_debian_dependencies
}

# main

if [ "$EUID" -ne 0 ]
  then echo "ERROR: Please run as root"
  exit 1
fi

echo "Checking OS:"
. /etc/os-release
cat /etc/os-release

readonly INSTALLER="install_${ID}_${VERSION_ID}_dependencies"

if declare -f -F "$INSTALLER" >/dev/null; then
  $INSTALLER
else
  echo "ERROR: OS not supported"
  exit 1
fi
