#!/bin/bash

# Install additional pkgs needed to build Barefoot Networks SDE (BF-SDE) Diags
# package

trap 'exit' ERR

echo "Installing diag dependencies..."

osinfo=`cat /etc/issue.net`

if [[ $osinfo =~ "Fedora" ]]; then
  sudo yum install -y libpcap-devel
else
  # Install libpcal
  echo "Installing libpcap..."
  sudo apt-get install -y libpcap-dev
fi

echo "Done!"
