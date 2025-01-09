#!/bin/bash

# Install additional pkgs needed to build Barefoot Networks SDE (BF-SDE) switch
# package with bf-switch library

trap 'exit' ERR

osinfo=`cat /etc/issue.net`

if [[ $osinfo =~ "Fedora" ]]; then
    # Needed for building netlink library
    sudo yum install -y pkgconfig
    sudo yum install -y libnl3-devel
else
    # Needed for building netlink library
    sudo apt-get install -y pkg-config
    sudo apt-get install -y libnl-route-3-dev
fi

sudo pip install --upgrade tenjin pysubnettree ipaddress

# Needed to build SAI metadata
# sudo apt-get install -y libxml-simple-perl
# sudo apt-get install -y aspell
# sudo apt-get install -y aspell-en
# sudo apt-get install -y doxygen
