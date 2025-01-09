project(libbfpi VERSION 0.1 LANGUAGES CXX)

if (GRPC)
  find_package(Protobuf REQUIRED)
  find_package(GRPC REQUIRED)
endif()

include_directories(.)
set(PI_SRCS
  pi_imp.c
  pi_tables_imp.c
  pi_act_prof_imp.c
  pi_counter_imp.c
  pi_meter_imp.c
  pi_learn_imp.c
  pi_mc_imp.c
  pi_clone_imp.c
  pi_log.h
  pi_state.h
  pi_state.c
  pi_helpers.h
  pi_helpers.c
  pi_packet.h
  pi_packet.c
  pi_allocators.h
  pi_allocators.c
  pi_resource_specs.h
  pi_resource_specs.c
)

add_library(bfpi_o OBJECT ${PI_SRCS})
target_include_directories(bfpi_o PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
target_include_directories(bfpi_o PRIVATE ${CMAKE_INSTALL_PREFIX}/lib)

if (GRPC)
  add_dependencies(bfpi_o bf_google_grpc_o bf_google_protobuf_o)
endif()
add_library(bfpi SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bfpi_o>)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
if (GRPC)
  target_link_libraries(bfpi PUBLIC protobuf grpc grpc++ gRPC::grpc++_reflection)
endif()
