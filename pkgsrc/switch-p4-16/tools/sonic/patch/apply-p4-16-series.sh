#! /bin/bash

pushd src/sonic-sairedis
git apply ../../sairedis.patch
popd
