#!/bin/bash

# Set up hugepages used for DMA buffer allocation
ID=$(id -u)
if [ $ID != 0 ]; then
  echo "ERROR: Run this script as root or with sudo"
  exit 1
else
  dflt_huge_pages=196
  cur_huge_pages=$(sysctl --values vm.nr_hugepages)
  if [[ $dflt_huge_pages -gt $cur_huge_pages ]]; then
    if grep -q "^vm.nr_hugepages\>" /etc/sysctl.conf; then
      sed -i "s/^vm.nr_hugepages\>.*$/vm.nr_hugepages = $dflt_huge_pages/" /etc/sysctl.conf
    else
      echo "vm.nr_hugepages = $dflt_huge_pages" >> /etc/sysctl.conf
    fi
    sysctl -p /etc/sysctl.conf
  else
    echo $cur_huge_pages huge pages already configured.
  fi
fi
