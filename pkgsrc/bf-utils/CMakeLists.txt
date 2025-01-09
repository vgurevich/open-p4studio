# -- Following lines are needed if the project is not compiled inside
#    the P4-Factory.
# cmake_policy(SET CMP0048 NEW)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(libbfutils VERSION 0.1 LANGUAGES C)

add_subdirectory(third-party)
add_subdirectory(src)
add_subdirectory(examples)

file(COPY include/bfutils DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  PATTERN "*.doxy" EXCLUDE
  PATTERN "*.am" EXCLUDE)

