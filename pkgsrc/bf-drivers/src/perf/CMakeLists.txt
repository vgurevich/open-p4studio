project(libperf VERSION 0.1)

add_subdirectory(python_cli)

include_directories(.)

set(PERF_SRCS
perf_common.c
perf_env.c
perf_int.c
perf_reg.c
perf_mem.c
perf_util.c
perf_ucli.c
)

add_library(perf_o OBJECT ${PERF_SRCS})
target_include_directories(perf_o PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
target_include_directories(perf_o PRIVATE ${CMAKE_INSTALL_PREFIX}/lib)
