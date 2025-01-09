project(bfthird_party VERSION 0.1 LANGUAGES CXX)

message("Building drivers third_party")
if(TDI)
  add_subdirectory(tdi)
endif()
if(GRPC)
  set(CMAKE_CXX_STANDARD 17)
  set(CXX_STANDARD_REQUIRED ON)
  find_package(Protobuf REQUIRED)
  find_package(GRPC REQUIRED)
  include(PythonDependencies)
  include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})
  include_directories(SYSTEM ${GRPC_INCLUDE_DIR})

  set(PROTO_CXX_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpp_out")
  file(MAKE_DIRECTORY "${PROTO_CXX_DST_DIR}/google/rpc")
  set(PROTO_SRC
    "google/rpc/status.proto"
    "google/rpc/code.proto")

  set(PROTO_CPP_SRC
    "cpp_out/google/rpc/status.pb.cc"
    "cpp_out/google/rpc/status.pb.h"
    "cpp_out/google/rpc/code.pb.cc"
    "cpp_out/google/rpc/code.pb.h")

  set(PROTO_GRPC_SRC
    "cpp_out/google/rpc/status.grpc.pb.cc"
    "cpp_out/google/rpc/status.grpc.pb.h"
    "cpp_out/google/rpc/code.grpc.pb.cc"
    "cpp_out/google/rpc/code.grpc.pb.h")

  set(PROTOBUF_INCLUDE "-I${CMAKE_CURRENT_SOURCE_DIR}")
  set(${SRCS} "")
  set(${GSRCS} "")
  foreach(FIL ${PROTO_SRC})
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_D ${FIL} DIRECTORY)
    list(APPEND GSRCS "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.grpc.pb.cc")
    list(APPEND GSRCS "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.grpc.pb.h")
    list(APPEND SRCS  "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.pb.cc")
    list(APPEND SRCS  "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.pb.h")
    add_custom_command(
      OUTPUT "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.grpc.pb.cc"
      "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.grpc.pb.h"
      "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.pb.cc"
      "${PROTO_CXX_DST_DIR}/${FIL_D}/${FIL_WE}.pb.h"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --cpp_out ${PROTO_CXX_DST_DIR} --grpc_out ${PROTO_CXX_DST_DIR}
      ${PROTOBUF_INCLUDE} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${FIL}
      DEPENDS "${FIL}" ${PROTOBUF_PROTOC_EXECUTABLE} ${GRPC_CPP_PLUGIN}
      COMMENT "Running C++ gRPC and protobuf compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${SRCS} ${GSRCS} PROPERTIES GENERATED TRUE)
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/cpp_out")
  add_library(bf_google_protobuf_o OBJECT ${SRCS})
  add_library(bf_google_protobuf SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bf_google_protobuf_o>)
  add_library(bf_google_grpc_o OBJECT ${GSRCS})
  add_library(bf_google_grpc SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bf_google_grpc_o>)

  # GRPC PROTO Python
  set(PROTO_PYTHON_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/python_out")
  file(MAKE_DIRECTORY "${PROTO_PYTHON_DST_DIR}/google/rpc")
  foreach(FIL ${PROTO_SRC})
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_D ${FIL} DIRECTORY)
    add_custom_command(
      OUTPUT "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2_grpc.py"
             "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2.py"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --python_out ${PROTO_PYTHON_DST_DIR} --grpc_out ${PROTO_PYTHON_DST_DIR}
      ${PROTOBUF_INCLUDE} --plugin=protoc-gen-grpc=${GRPC_PYTHON_PLUGIN} ${FIL}
      DEPENDS "${FIL}" ${PROTOBUF_PROTOC_EXECUTABLE} ${GRPC_PYTHON_PLUGIN}
      COMMENT "Running gRPC and protobuf compiler on ${FIL} to generate python files"
      VERBATIM
    )
    add_custom_target(${FIL_WE}_pb2_py_target ALL DEPENDS "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2.py")
    add_custom_target(${FIL_WE}_pb2_grpc_py_target ALL DEPENDS "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2_grpc.py")
    install(FILES "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2_grpc.py"
            DESTINATION ${SDE_PYTHON_DEPENDENCIES_DIR}/tofino/google/rpc
    )
    install(FILES "${PROTO_PYTHON_DST_DIR}/${FIL_D}/${FIL_WE}_pb2.py"
            DESTINATION ${SDE_PYTHON_DEPENDENCIES_DIR}/tofino/google/rpc
    )
  endforeach()
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/python_out/__init__.py "")
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/python_out/__init__.py
    DESTINATION ${SDE_PYTHON_DEPENDENCIES_DIR}/tofino/google/rpc)
endif()
