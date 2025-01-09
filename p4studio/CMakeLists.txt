###############################################################################
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
#################################################################################

###############################################################################
#
# This is a utility to ease building P4 programs with P4Studio
#
# NOTE: Before using this tool, please ensure SDE is installed at $SDE_INSTALL
#
# This file can be used as is from the current location or copied to an 
# external location. The standard CMake build semantics apply here.
# cmake <location-of-this-file> <option1> <option2>
#
# This utility uses the P4Build.cmake module and other modules supplied with SDE.
# Additionally, the location of SDE artifacts like P4 compiler and other tools
# are also resolved automatically based on $SDE_INSTALL.
#
# CMAKE_MODULE_PATH is a required option when invoking this file
#   - This option allows cmake to find and use the functions defined in 
#     P4Build.cmake and find other modules like FindThrift.cmake.
# CMAKE_INSTALL_PREFIX is the location of bf-p4c and other artifacts
#   - This is typically the location where the SDE build artifacts are installed
#     Tools like bf-p4c, p4c-gen-bfrt-conf, etc are present in this location in
#     addition to built libraries like libdriver.so.
#
# P4-14 PTF python test scripts need Python-Thrift bindings to invoke PD API
# calls over Thrift-RPC. Similarly the process that hosts the PD API library will
# need C-Thrift bindings to process Thrift-RPC messages and make 'C' PD API calls.
#
# Using option WITHPD=on, will disable generating BF-RT schema and instead
# generate PD artifacts like libpd.so, libpdcli.so, etc.
#
# Using THRIFT-DRIVER=on, will enable building libpdthrift.so.
# Please ensure the third-party Thrift library is installed if this option is used
#
# Example
# =======
# User program location: /myprogram/test.p4
# SDE location: /sde
# SDE_INSTALL location: /sde/install
# This file location: /sde/p4studio/
#
# mkdir /build && cd /build
# cmake /sde/p4studio/ \
#       -DCMAKE_INSTALL_PREFIX=$SDE_INSTALL \
#       -DCMAKE_MODULE_PATH=$SDE/cmake      \
#       -DP4_NAME="myprogram"               \
#       -DP4_PATH=/myprogram/test.p4
# make myprogram && make install
#
# The built artifacts are installed to $SDE_INSTALL
#
# The majority of options for this tool are available as CMake options. The
# rest are to be supplied as -D option strings. Please refer to the list of
# available options below
#
# List of string options
# ======================
# Option        Description             Default     Comment
# ------------------------------------------------------------
# P4_NAME       P4 program name         None        Mandatory
# P4_PATH       P4 program path         None        Mandatory
# P4_LANG       p4-14/p4-16             p4-16       Optional
# P4FLAGS       P4 compiler flags       ""          Optional
# P4PPFLAGS     P4 preprocessor flags   ""          Optional
# PDFLAGS       Program dependent flags ""          Optional
#
# Artifacts installed
# ===================
#  - P4 program artifacts
#      * data files generated for bf-drivers
#        to $SDE_INSTALL/share/p4/<$P4_NAME>/
#          - context.json: pipeline resource context for the driver
#          - tofino.bin/tofnio2.bin: configuration based on Tofino device
#
#      * conf files needed by switchd application for PTF tests
#        to $SDE_INSTALL/share/p4/targets/<$P4_NAME>*.conf
#
#  - BF-Runtime generated artifacts (when compiled with P4_LANG=p4-16)
#      * $SDE_INSTALL/share/p4/<$P4_NAME>/bf-rt.json
#
#
# The following shared libraries are installed to $SDE_INSTALL/lib/<$P4_NAME>/
#
# Additional build artifacts installed when built with WITHPD=on
#  - P4 program dependent auto-generated artifacts
#      * header files for auto-generated P4 dependent (PD) library
#        to $SDE_INSTALL/include/<$P4_NAME>/pd
#      * libraries for auto-generated P4 dependent sources
#          - libpd.so
#      * libraries for auto-generated CLI
#          - libpdcli.so
#
# Additional build artifacts installed when built with THRIFT-DRIVER=on
#  - P4 program dependent auto-generated artifacts
#      * libraries for auto-generated Thrift-RPC bindings
#          - libpdthrift.so
#      * Python modules for calling from PTF
#
# Artifact install tree
# =====================
#  ├── bin
#  ├── include
#  │   └── tofinopd
#  │       └── <$P4_NAME>
#  │           └── pd
#  │               └── pd.h
#  ├── lib
#  │   ├── tofinopd
#  │   │   └── <$P4_NAME>
#  │   │       ├── libpd.so
#  │   │       ├── libpdcli.so
#  │   │       └── libpdthrift.so
#  │   └── python2.7
#  │       └── site-packages
#  │           └── tofinopd
#  │               └── <$P4_NAME>
#  │                   └── p4_pd_rpc
#  │                       ├── constants.py
#  │                       ├── __init__.py
#  │                       ├── <$P4_NAME>.py
#  │                       └── ttypes.py
#  └── share
#      ├── p4
#      │   └── targets
#      │       ├── tofino
#      │       │    └── <$P4_NAME>.conf
#      │       └── tofino2
#      │            └── <$P4_NAME>.conf
#      ├── tofinopd
#      │   └── <$P4_NAME>
#      │       ├── bf-rt.json
#      │       └── <pipe name>
#      │           ├── context.json
#      │           └── tofino.bin
#      └── tofino2pd
#          └── <$P4_NAME>
#              ├── bf-rt.json
#              └── <pipe name>
#                  ├── context.json
#                  └── tofino2.bin
#
###############################################################################

cmake_minimum_required(VERSION 3.1)
project(p4builder VERSION 0.1 LANGUAGES C CXX)

if((NOT DEFINED ENV{SDE}) AND (NOT DEFINED SDE))
  message(FATAL_ERROR "SDE environment variable is not set. Please set it and try again")
else()
  set(SDE $ENV{SDE})
endif()
message(STATUS "\nUsing SDE: ${SDE}")

set(SDE_CMAKE_MODULE_PATH ${SDE}/cmake)
list(APPEND CMAKE_MODULE_PATH ${SDE_CMAKE_MODULE_PATH})

if(NOT DEFINED ENV{SDE_INSTALL})
  set(SDE_INSTALL "${SDE}/install/")
  message(WARNING "\nSDE_INSTALL environment variable is not set. Automatically setting SDE_INSTALL to ${SDE_INSTALL}")
else()
  set(SDE_INSTALL $ENV{SDE_INSTALL})
  message(STATUS "\nUsing SDE_INSTALL: ${SDE_INSTALL}")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${SDE_INSTALL} CACHE PATH "..." FORCE)
endif()
set(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}")

# options
option(TOFINO        "Build P4 for tofino target" OFF)
option(TOFINO2       "Build P4 for tofino2 target" OFF)
option(TOFINO2M      "Build P4 for tofino2m target" OFF)
option(PSA           "Build P4 for PSA architecture" OFF)
option(V1MODEL       "Build P4 for v1model architecture" OFF)
option(THRIFT-DRIVER "Build with thrift support" OFF)
option(WITHPD        "Build with PD artifacts" OFF)

set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(PythonDependencies)

if (NOT TOFINO AND NOT TOFINO2 AND NOT TOFINO2M AND NOT TOFINO3)
  set(TOFINO ON CACHE BOOL "Enable Tofino as default" FORCE)
  message(STATUS "\nTarget option missing. Using TOFINO\n")
endif()

# dummy targets to satisfy dependencies
add_custom_target(bf-p4c)
add_custom_target(driver)

# p4_lang
if(NOT P4_LANG)
  set(P4_LANG "p4-16" CACHE STRING "" FORCE)
  message(STATUS "\nP4_LANG missing. Using ${P4_LANG}")
else()
  message(STATUS "\nP4_LANG: ${P4_LANG}")
endif()

if (${P4_LANG} STREQUAL "p4-14")
  set(THRIFT-DRIVER ON CACHE BOOL "Build with Thrift support as default for p4-14" FORCE)
  set(WITHPD ON CACHE BOOL "Build with PD artifacts as default for p4-14" FORCE)
  set(V1MODEL ON CACHE BOOL "V1MODEL only supported arch for p4-14" FORCE)
endif()

if(THRIFT-DRIVER)
  find_package(Thrift REQUIRED)
endif()

find_program(P4C bf-p4c REQUIRED)
message("P4C: " ${P4C})
find_program(P4C-GEN-BFRT-CONF p4c-gen-bfrt-conf REQUIRED)
message("P4C-GEN_BRFT-CONF: " ${P4C-GEN-BFRT-CONF})
find_program(P4C-MANIFEST-CONFIG p4c-manifest-config REQUIRED)
message("P4C-MANIFEST-CONFIG: " ${P4C-MANIFEST-CONFIG})
if (WITHPD)
  find_program(PDGEN generate_tofino_pd REQUIRED)
  set(PDGEN_COMMAND ${PYTHON_COMMAND} ${PDGEN})
  message("PDGEN: " ${PDGEN})
  find_program(PDGENCLI gencli REQUIRED)
  set(PDGENCLI_COMMAND ${PYTHON_COMMAND} ${PDGENCLI})
  message("PDGENCLI: " ${PDGENCLI})
  find_program(PDSPLIT split_pd_thrift.py REQUIRED)
  set(PDSPLIT_COMMAND ${PYTHON_COMMAND} ${PDSPLIT})
  message("PDSPLIT: " ${PDSPLIT})

endif()

include(P4Build)

if (TOFINO)
  set(P4_tofino_ARCHITECTURE "tna")
endif()
if (TOFINO2 OR TOFINO2M)
  set(P4_tofino2_ARCHITECTURE "t2na")
endif()

if (PSA)
  set(P4_tofino_ARCHITECTURE "psa")
  set(P4_tofino2_ARCHITECTURE "psa")
endif()

if (V1MODEL)
  set(P4_tofino_ARCHITECTURE "v1model")
  set(P4_tofino2_ARCHITECTURE "v1model")
endif()

# p4_path is mandatory
if(NOT P4_PATH)
  message(FATAL_ERROR "P4 program path (P4_PATH) missing")
else()
  message(STATUS "\nP4_PATH: ${P4_PATH}")
endif()
if(NOT EXISTS ${P4_PATH})
  message(FATAL_ERROR "Invalid P4_PATH: ${P4_PATH}")
endif()

if(NOT P4_NAME)
  # NAME_WE is a mode used to get file name without directory/longest extension
  get_filename_component(P4_NAME "${P4_PATH}" NAME_WE)
endif()
message("P4_NAME: ${P4_NAME}")

if(NOT P4FLAGS)
  set(P4FLAGS "" CACHE STRING "" FORCE)
else()
  message(STATUS "\nP4FLAGS: ${P4FLAGS}")
endif()

if(NOT P4PPFLAGS)
  set(P4PPFLAGS "" CACHE STRING "" FORCE)
else()
  message(STATUS "\nP4PPFLAGS: ${P4PPFLAGS}")
endif()

if(NOT PDFLAGS)
  set(PDFLAGS "" CACHE STRING "" FORCE)
else()
  message(STATUS "\nPDFLAGS: ${PDFLAGS}")
endif()

add_custom_target(${P4_NAME} ALL)
if (TOFINO)
  if (WITHPD)
    p4_build_pd_target(${P4_NAME} ${P4_tofino_ARCHITECTURE} "tofino" ${P4_PATH})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${P4_NAME}/tofino/${P4_NAME}.conf DESTINATION share/p4/targets/tofino)
  else()
    p4_build_target(${P4_NAME} ${P4_tofino_ARCHITECTURE} "tofino" ${P4_PATH})
  endif()
  add_dependencies(${P4_NAME} ${P4_NAME}-tofino)
endif()

if (TOFINO2)
  if (WITHPD)
    p4_build_pd_target(${P4_NAME} ${P4_tofino2_ARCHITECTURE} "tofino2" ${P4_PATH})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${P4_NAME}/tofino2/${P4_NAME}.conf DESTINATION share/p4/targets/tofino2)
  else()
    p4_build_target(${P4_NAME} ${P4_tofino2_ARCHITECTURE} "tofino2" ${P4_PATH})
  endif()
  add_dependencies(${P4_NAME} ${P4_NAME}-tofino2)
endif()

if (TOFINO2M)
  if (WITHPD)
    p4_build_pd_target(${P4_NAME} ${P4_tofino2_ARCHITECTURE} "tofino2m" ${P4_PATH})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${P4_NAME}/tofino2m/${P4_NAME}.conf DESTINATION share/p4/targets/tofino2m)
  else()
    p4_build_target(${P4_NAME} ${P4_tofino2_ARCHITECTURE} "tofino2m" ${P4_PATH})
  endif()
  add_dependencies(${P4_NAME} ${P4_NAME}-tofino2m)
endif()



add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR="${CMAKE_BINARY_DIR}" -DCMAKE_COMMAND="${CMAKE_COMMAND}" -P ${SDE_CMAKE_MODULE_PATH}/CmakeUninstall.cmake)
