#!/bin/bash

# Extract all the SDE modules

[ -z ${SDE} ] && echo "Environment variable SDE not set" && exit 1

cd ${SDE}
find -name "*.tgz" -exec tar xzvf '{}' ';'
