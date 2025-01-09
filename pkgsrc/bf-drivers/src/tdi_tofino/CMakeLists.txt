project(libtdi_tofino VERSION 0.1 LANGUAGES CXX)

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', standard_lib=True, plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

include_directories(.)

set(TDI_TOFINO_SRCS
  c_frontend/tdi_tofino_init_c.cpp
  c_frontend/tdi_tofino_warm_init_c.cpp
  tdi_common/tdi_tofino_init.cpp
  tdi_common/tdi_tofino_target.cpp
  tdi_common/tdi_context_info.cpp
  tdi_common/tdi_pipe_mgr_intf.cpp
  tdi_common/tdi_tofino_session.cpp
  tdi_common/tdi_table_attributes_impl.cpp
  tdi_p4/tdi_table_state.cpp
  tdi_p4/tdi_p4_table_impl.cpp
  tdi_p4/tdi_p4_table_data_impl.cpp
  tdi_p4/tdi_p4_table_key_impl.cpp
  tdi_p4/tdi_table_attributes_state.cpp
  tdi_pre/tdi_mc_mgr_intf.cpp
  tdi_port/tdi_port_mgr_intf.cpp
  tdi_port/tdi_port_table_data_impl.cpp
  tdi_port/tdi_port_table_key_impl.cpp
  tdi_port/tdi_port_table_impl.cpp
  tdi_port/tdi_port_table_attributes_state.cpp
  tdi_mirror/tdi_mirror_table_data_impl.cpp
  tdi_mirror/tdi_mirror_table_data_impl.hpp
  tdi_mirror/tdi_mirror_table_impl.cpp
  tdi_mirror/tdi_mirror_table_impl.hpp
  tdi_mirror/tdi_mirror_table_key_impl.cpp
  tdi_mirror/tdi_mirror_table_key_impl.hpp
)

add_library(tdi_tofino_o OBJECT ${TDI_TOFINO_SRCS})
add_library(tdi_tofino SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:tdi_tofino_o>)

target_link_libraries(tdi_tofino PUBLIC tdi_tna tdi_json_parser)

add_subdirectory(tests EXCLUDE_FROM_ALL)

## Install tdi_python files to python install directory
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/tdi/tdi_python/ DESTINATION lib/python${TDI_PYTHON_VER})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tdi_python/ DESTINATION lib/python${TDI_PYTHON_VER})

if(TDI-PYTHON)
  add_library(bfpythonlib UNKNOWN IMPORTED)
  set_property(TARGET bfpythonlib PROPERTY IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpython${TDI_PYTHON_VER}.so)
  add_dependencies(bfpythonlib libpython${TDI_PYTHON_VER})
  add_library(bfshell_plugin_tdi_o OBJECT cli/tdi_cli.c)
  include_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../include/python${TDI_PYTHON_VER})
  add_dependencies(bfshell_plugin_tdi_o bfpythonlib)
  add_library(bfshell_plugin_tdi SHARED $<TARGET_OBJECTS:bfshell_plugin_tdi_o>)
  target_link_libraries(bfshell_plugin_tdi bfpythonlib)
  SET_TARGET_PROPERTIES(bfshell_plugin_tdi PROPERTIES PREFIX "")
  install(FILES xml/tdi.xml DESTINATION share/cli/xml)
endif()

install(FILES
  tdi_port/tdi_port_tf1.json
  tdi_port/tdi_port_tf2.json
  tdi_port/tdi_port_tf3.json
  DESTINATION share/tdi_shared
)

install(FILES
  tdi_mirror/tdi_mirror_tf1.json
  tdi_mirror/tdi_mirror_tf2.json
  tdi_mirror/tdi_mirror_tf3.json
  DESTINATION share/tdi_shared
)

#set(TDI_SRCS
#tdi_common/tdi_pipe_mgr_intf.cpp
#tdi_mirror/tdi_mirror_table_data_impl.cpp
#tdi_mirror/tdi_mirror_table_impl.cpp
#tdi_mirror/tdi_mirror_table_key_impl.cpp
#tdi_p4/tdi_learn_impl.cpp
#tdi_p4/tdi_learn_state.cpp
#tdi_p4/tdi_p4_table_data_impl.cpp
#tdi_p4/tdi_p4_table_impl.cpp
#tdi_p4/tdi_p4_table_key_impl.cpp
#tdi_p4/tdi_table_attributes_state.cpp
#tdi_p4/tdi_table_state.cpp
#tdi_port/tdi_port_mgr_intf.cpp
#tdi_port/tdi_port_table_data_impl.cpp
#tdi_port/tdi_port_table_key_impl.cpp
#tdi_port/tdi_port_table_impl.cpp
#tdi_port/tdi_port_table_attributes_state.cpp
#tdi_dev/tdi_dev_table_data_impl.cpp
#tdi_dev/tdi_dev_table_impl.cpp
#tdi_pktgen/tdi_pktgen_table_data_impl.cpp
#tdi_pktgen/tdi_pktgen_table_key_impl.cpp
#tdi_pktgen/tdi_pktgen_table_impl.cpp
#tdi_pre/tdi_mc_mgr_intf.cpp
#tdi_pre/tdi_pre_state.cpp
#tdi_pre/tdi_pre_table_data_impl.cpp
#tdi_pre/tdi_pre_table_impl.cpp
#tdi_pre/tdi_pre_table_key_impl.cpp
#tdi_dev/tdi_dev_table_data_impl.cpp
#tdi_dev/tdi_dev_table_impl.cpp
#tdi_tm/tdi_tm_intf.cpp
#tdi_tm/tdi_tm_state.cpp
#tdi_tm/tdi_tm_table_data_impl.cpp
#tdi_tm/tdi_tm_table_key_impl.cpp
#tdi_tm/tdi_tm_table_impl.cpp
#tdi_tm/tdi_tm_table_impl_port.cpp
#tdi_tm/tdi_tm_table_impl_portgroup.cpp
#tdi_tm/tdi_tm_table_helper_ppg.cpp
#tdi_tm/tdi_tm_table_helper_sched.cpp
#tdi_tm/tdi_tm_table_helper_pipe.cpp
#tdi_tm/tdi_tm_table_impl_ppg.cpp
#tdi_tm/tdi_tm_table_impl_mirror.cpp
#tdi_tm/tdi_tm_table_impl_queue.cpp
#tdi_tm/tdi_tm_table_impl_l1_node.cpp
#tdi_tm/tdi_tm_table_impl_pool.cpp
#tdi_tm/tdi_tm_table_impl_counters.cpp
#tdi_tm/tdi_tm_table_impl_cfg.cpp
#tdi_tm/tdi_tm_table_impl_pipe.cpp
#c_frontend/tdi_session_c.cpp
#c_frontend/tdi_table_c.cpp
#c_frontend/tdi_table_data_c.cpp
#c_frontend/tdi_table_key_c.cpp
#c_frontend/tdi_table_attributes_c.cpp
#c_frontend/tdi_table_operations_c.cpp
#c_frontend/tdi_learn_c.cpp
#c_frontend/tdi_state_c.cpp
#c_frontend/tdi_init_c.cpp
#c_frontend/tdi_info_c.cpp
#)
#
#if(TDI-PYTHON)
#  add_library(bfpythonlib UNKNOWN IMPORTED)
#  set_property(TARGET bfpythonlib PROPERTY IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpython${TDI_PYTHON_VER}.so)
#  add_dependencies(bfpythonlib libpython${TDI_PYTHON_VER})
#  add_library(bfshell_plugin_tdi_o OBJECT cli/tdi_cli.c)
#  include_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../include/python${TDI_PYTHON_VER})
#  add_dependencies(bfshell_plugin_tdi_o bfpythonlib)
#  add_library(bfshell_plugin_tdi SHARED $<TARGET_OBJECTS:bfshell_plugin_tdi_o>)
#  target_link_libraries(bfshell_plugin_tdi bfpythonlib)
#  SET_TARGET_PROPERTIES(bfshell_plugin_tdi PROPERTIES PREFIX "")
#  install(FILES xml/tdi.xml DESTINATION share/cli/xml)
#endif()
#
#install(FILES
#  tdi_mirror/tdi_mirror_tf1.json
#  tdi_mirror/tdi_mirror_tf2.json
#  tdi_dev/tdi_dev_tf1.json
#  tdi_dev/tdi_dev_tf2.json
#  tdi_port/tdi_port_tf1.json
#  tdi_port/tdi_port_tf2.json
#  tdi_pktgen/tdi_pktgen_tf1.json
#  tdi_pktgen/tdi_pktgen_tf2.json
#  tdi_pre/tdi_pre_tf1.json
#  tdi_pre/tdi_pre_tf2.json
#  tdi_tm/tdi_tm_tf1.json
#  tdi_tm/tdi_tm_tf2.json
#  tdi_p4/tdi_p4_tf1.json
#  tdi_p4/tdi_p4_tf2.json
#  DESTINATION share/tdi_shared
#)
#
#set(PROTO_SRC "")
#set(TDI_GRPC_SRCS "")
#set(PROTO_CXX_SRC "")
#set(GRPC_CXX_SRC "")
#
#if(GRPC)
#  find_package(Protobuf REQUIRED)
#  find_package(GRPC REQUIRED)
#  include_directories(SYSTEM ${PROTOBUF_INCLUDE_DIR})
#  include_directories(SYSTEM ${GRPC_INCLUDE_DIR})
#
#  set(TDI_GRPC_SRCS
#    proto/tdi_server_impl.cpp
#    proto/tdi_server_mgr.cpp
#  )
#  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/cpp_out")
#  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/python_out")
#
#  set(PROTO_CXX_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpp_out")
#  set(PROTO_PYTHON_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/python_out")
#  set(PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/proto/bfruntime.proto")
#
#  if(TDI-GO)
#    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/go_out")
#    set(PROTO_GO_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/go_out")
#    set(PROTO_GO_SRC "${PROTO_GO_DST_DIR}/bfruntime.pb.go"
#               "${PROTO_GO_DST_DIR}/bfruntime_grpc.pb.go")
#    add_custom_command(
#        OUTPUT "${PROTO_GO_DST_DIR}/bfruntime.pb.go"
#               "${PROTO_GO_DST_DIR}/bfruntime_grpc.pb.go"
#        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#        ARGS --go_out ${PROTO_GO_DST_DIR} --go-grpc_out ${PROTO_GO_DST_DIR}
#             -I "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#             -I "${CMAKE_CURRENT_SOURCE_DIR}/proto" ${PROTO_SRC}
#        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#        DEPENDS "${PROTO_SRC}" ${PROTOBUF_PROTOC_EXECUTABLE}
#        COMMENT "Generating Golang gRPC and protobuf files from ${PROTO_SRC}"
#        VERBATIM
#    )
#    set_source_files_properties(${PROTO_GO_SRC} PROPERTIES GENERATED TRUE)
#    add_custom_target(bfruntime_pb_go_target ALL DEPENDS "${PROTO_GO_DST_DIR}/bfruntime.pb.go")
#    add_custom_target(bfruntime_grpc_pb_go_target ALL DEPENDS "${PROTO_GO_DST_DIR}/bfruntime_grpc.pb.go")
#  endif()
#
#  set(PROTO_CXX_SRC
#        "${PROTO_CXX_DST_DIR}/bfruntime.pb.cc"
#        "${PROTO_CXX_DST_DIR}/bfruntime.pb.h"
#     )
#  set(GRPC_CXX_SRC
#        "${PROTO_CXX_DST_DIR}/bfruntime.grpc.pb.cc"
#        "${PROTO_CXX_DST_DIR}/bfruntime.grpc.pb.h"
#     )
#
#  add_custom_command(
#      OUTPUT "${PROTO_CXX_DST_DIR}/bfruntime.pb.cc"
#             "${PROTO_CXX_DST_DIR}/bfruntime.pb.h"
#             "${PROTO_CXX_DST_DIR}/bfruntime.grpc.pb.cc"
#             "${PROTO_CXX_DST_DIR}/bfruntime.grpc.pb.h"
#      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#      ARGS --cpp_out ${PROTO_CXX_DST_DIR} --grpc_out ${PROTO_CXX_DST_DIR}
#           -I "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#           -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
#           --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_SRC}
#      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#      DEPENDS "${PROTO_SRC}" ${PROTOBUF_PROTOC_EXECUTABLE} ${GRPC_CPP_PLUGIN}
#      COMMENT "Generating cpp gRPC and protobuf files from ${PROTO_SRC}"
#      VERBATIM
#  )
#
#  set_source_files_properties(${PROTO_CXX_SRC} ${GRPC_CXX_SRC} PROPERTIES GENERATED TRUE)
#  include_directories("${CMAKE_CURRENT_BINARY_DIR}/cpp_out")
#  include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../third-party/cpp_out")
#  add_library(tdi_grpc_proto SHARED EXCLUDE_FROM_ALL ${PROTO_CXX_SRC} ${GRPC_CXX_SRC})
#
#  add_custom_command(
#      OUTPUT "${PROTO_PYTHON_DST_DIR}/bfruntime_pb2.py"
#             "${PROTO_PYTHON_DST_DIR}/bfruntime_pb2_grpc.py"
#      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#      ARGS --python_out ${PROTO_PYTHON_DST_DIR} --grpc_out ${PROTO_PYTHON_DST_DIR}
#           -I "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#           -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
#           --plugin=protoc-gen-grpc=${GRPC_PYTHON_PLUGIN} ${PROTO_SRC}
#      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/"
#      DEPENDS "${PROTO_SRC}" ${PROTOBUF_PROTOC_EXECUTABLE} ${GRPC_PYTHON_PLUGIN}
#      COMMENT "Generating python gRPC and protobuf files from ${PROTO_SRC}"
#      VERBATIM
#  )
#  add_custom_target(bfruntime_pb2_py_target ALL DEPENDS "${PROTO_PYTHON_DST_DIR}/bfruntime_pb2.py")
#  add_custom_target(bfruntime_pb2_grpc_py_target ALL DEPENDS "${PROTO_PYTHON_DST_DIR}/bfruntime_pb2_grpc.py")
#  install(FILES ${PROTO_PYTHON_DST_DIR}/bfruntime_pb2.py DESTINATION
#    ${PYTHON_SITE}/site-packages/tofino/tdi_grpc)
#  install(FILES ${PROTO_PYTHON_DST_DIR}/bfruntime_pb2_grpc.py DESTINATION
#    ${PYTHON_SITE}/site-packages/tofino/tdi_grpc)
#
#  add_library(tdi_o OBJECT
#    ${TDI_SRCS}
#    ${TDI_GRPC_SRCS}
#    ${PROTO_CXX_SRC}
#    ${GRPC_CXX_SRC}
#  )
#  add_dependencies(tdi_o bf_google_grpc_o bf_google_protobuf_o)
#  add_library(tdi SHARED EXCLUDE_FROM_ALL
#    $<TARGET_OBJECTS:tdi_o>
#    $<TARGET_OBJECTS:bf_google_grpc_o>
#    $<TARGET_OBJECTS:bf_google_protobuf_o>
#  )
#  target_link_libraries(tdi PUBLIC ${PROTOBUF_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY} ${GRPC_GRPC++_REFLECTION_LIBRARY})
#else()
#  add_library(tdi_o OBJECT ${TDI_SRCS})
#  add_library(tdi SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:tdi_o>)
#endif()
#
## Install tdi_python files to python install directory
#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tdi_python/ DESTINATION lib/python${TDI_PYTHON_VER})
#
##Install bfruntime python client files
#install(FILES
#  ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_grpc_client/python/client.py
#  DESTINATION ${PYTHON_SITE}/site-packages/tofino/tdi_grpc)
#install(FILES
#  ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_grpc_client/python/info_parse.py
#  DESTINATION ${PYTHON_SITE}/site-packages/tofino/tdi_grpc)
#file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/python_out/__init__.py "")
#install(FILES
#  ${CMAKE_CURRENT_BINARY_DIR}/python_out/__init__.py
#  DESTINATION ${PYTHON_SITE}/site-packages/tofino/tdi_grpc/)
#
#add_subdirectory(tests EXCLUDE_FROM_ALL)
#
## Building tdi doxygen
#find_package(Doxygen)
#find_package(Doxypy)
#if(DOXYGEN_FOUND)
#  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
#  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
#  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
#  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
#  add_custom_target(doc-driver-tdi
#                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
#                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#                    COMMENT "Generating TDI Doxygen"
#                    VERBATIM)
#  set_target_properties(doc-driver-tdi PROPERTIES EXCLUDE_FROM_ALL TRUE)
#  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
#    share/doc/bf-drivers/tdi OPTIONAL)
#  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
#    share/doc/bf-drivers/tdi OPTIONAL)
#
#  # TDI fixed function tables
#  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/tdi_fixed/docs)
#  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/tdi_fixed/docs/Doxyfile.in)
#  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/tdi_fixed/Doxyfile)
#  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
#  add_custom_target(doc-driver-tdi-fixed
#                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
#                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tdi_fixed
#                    COMMENT "Generating TDI Doxygen for fixed function tables"
#                    VERBATIM)
#  set_target_properties(doc-driver-tdi-fixed PROPERTIES EXCLUDE_FROM_ALL TRUE)
#  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tdi_fixed/html DESTINATION
#    share/doc/bf-drivers/tdi_fixed OPTIONAL)
#  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tdi_fixed/latex DESTINATION
#    share/doc/bf-drivers/tdi_fixed OPTIONAL)
#
#  # Building bfruntime_grpc_client doxygen
#  if(DOXYPY_FOUND)
#    set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_grpc_client/python/docs)
#    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/bfruntime_grpc_client/python/docs/Doxyfile.in)
#    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/bfruntime_grpc_client/python/Doxyfile)
#    configure_file(${doxyfile_in} ${doxyfile} @ONLY)
#    add_custom_target(doc-driver-tdi-python-grpc
#                      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
#                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bfruntime_grpc_client/python
#                      COMMENT "Generating bfruntime gRPC python client Doxygen"
#                      VERBATIM)
#    set_target_properties(doc-driver-tdi-python-grpc PROPERTIES EXCLUDE_FROM_ALL TRUE)
#    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bfruntime_grpc_client/python/html DESTINATION
#      share/doc/bf-drivers/tdi/bfruntime_grpc_client/python/ OPTIONAL)
#    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bfruntime_grpc_client/python/latex DESTINATION
#      share/doc/bf-drivers/tdi/bfruntime_grpc_client/python/ OPTIONAL)
#    else()
#      message(WARNING "Doxypy is needed to build TDI gRPC python client documentation. Please install doxypy.")
#    endif()
#else()
#  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
#endif()
#
