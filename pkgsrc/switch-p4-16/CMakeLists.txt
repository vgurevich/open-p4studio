cmake_minimum_required(VERSION 3.5)
project(libbf_switch VERSION 0.1 LANGUAGES C CXX)

# for reference
# 1. Release: `-O3 -DNDEBUG`
# 2. Debug: `-O0 -g`
# 3. RelWithDebInfo: `-O2 -g -DNDEBUG'

option(STANDALONE "Build outside of pkgsrc" OFF)
if (STANDALONE)
  option(FORCE32 "Force a 32bit compile on 64bit" OFF)
  option(PROFILER "Build with CPU profiler" OFF)
  option(THRIFT-SWITCH "Build with thrift support" ON)
  option(CPUVETH "Use CPU eth port" ON)
  option(RESOLVEALL "Static check for all symbols" OFF)
  option(SAI "Build SAI" OFF)

  set(CMAKE_BUILD_TYPE "Release")
  if(FORCE32)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  endif()

  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD 17)
  set(CXX_STANDARD_REQUIRED ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")

  set(C_CXX_FLAGS "-Wall -Wextra -Werror -Wundef -Wshadow -DBF_SYS_LOG_FORMAT_CHECK")
  if(COVERAGE)
    set(C_CXX_FLAGS "${C_CXX_FLAGS} --coverage")
  endif()
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${C_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-missing-field-initializers")
  # TODO(bfn) -Wexit-time-destructors
  if(RESOLVEALL)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,defs")
  endif()

  link_directories(AFTER ${CMAKE_INSTALL_PREFIX}/lib)

  find_program(P4C bf-p4c)
  message(STATUS "P4C: ${P4C}")
  add_custom_target(bf-p4c DEPENDS ${P4C})

  set(P4C ${CMAKE_INSTALL_PREFIX}/bin/bf-p4c)
endif()

if (THRIFT-SWITCH)
  find_package(Thrift REQUIRED)
  include_directories(SYSTEM ${THRIFT_INCLUDE_DIR})
endif()

if(ASAN)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(PROFILER)
  add_definitions(-D__CPU_PROFILER__)
endif()

if (NOT PYTHON_EXECUTABLE)
  set(PYTHON_EXECUTABLE python3)
endif()

# find system libraries
find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3)
find_library(NETLINKGENL3 nl-genl-3)
find_path(LIBNL_INCLUDE_DIR NAMES libnl3/netlink/socket.h)
include_directories(${LIBNL_INCLUDE_DIR})
include_directories(${LIBNL_INCLUDE_DIR}/libnl3)

# setup header file paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/api)
include_directories(SYSTEM ${CMAKE_INSTALL_PREFIX}/include)

if(SAL)
  add_definitions(-DSAL_API)
endif()
add_subdirectory(s3)
add_subdirectory(api)
if(SAI)
  add_subdirectory(sai)
endif()
if(SAL)
  add_subdirectory(sal)
endif()
add_subdirectory(p4src)
add_subdirectory(targets)

# make install targets
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/bf_switch DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/s3 DESTINATION include)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/doxygen)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  set(builddir ${CMAKE_CURRENT_BINARY_DIR})

  configure_file(${doxyfile_in} ${doxyfile} @ONLY)

  add_custom_target(doc-switch
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating bf-switch top level Doxygen"
                    VERBATIM)
  set_target_properties(doc-switch PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
