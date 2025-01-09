project(libbfp4rt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(BFP4RT_SRCS
  server/server.cpp
  server/uint128.h
  server/uint128.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(SYSTEM ${CMAKE_INSTALL_PREFIX}/include)

add_library(bfp4rt_o OBJECT ${BFP4RT_SRCS})
add_library(bfp4rt SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bfp4rt_o>)
add_dependencies(bfp4rt bfpi)
target_link_libraries(bfp4rt PUBLIC bfpi)
