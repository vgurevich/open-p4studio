
#SET(THRIFT_HOME ${CMAKE_SOURCE_DIR}/third-party/thrift)
#find_package(Thrift)
include_directories(SYSTEM ${THRIFT_INCLUDE_DIR})
#Make sure gen-cpp files can be included
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gen-cpp")
include_directories("${PROJECT_SOURCE_DIR}/lib/cpp/src")

set(THRIFT_CPP_SRC_LIST
#bfn_pd_rpc_server.h
bfn_pd_rpc_server.cpp
mc_pd_rpc_server.ipp
mirror_pd_rpc_server.ipp
sd_pd_rpc_server.ipp
pkt_pd_rpc_server.ipp
conn_mgr_pd_rpc_server.ipp
devport_mgr_pd_rpc_server.ipp
port_mgr_pd_rpc_server.ipp
plcmt_pd_rpc_server.ipp
tm_api_rpc_server.ipp
pal_rpc_server.ipp
knet_mgr_pd_rpc_server.ipp 
pipemgr_api_rpc_server.ipp
ts_pd_rpc_server.ipp
)
set(THRIFT_CPP_GEN_SRC_LIST
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/res_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/res_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm_api_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm_api_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr_pd_rpc_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api_types.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts_pd_rpc_types.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts_pd_rpc_types.h)

if ( THRIFT_VERSION_STRING VERSION_LESS 0.14.0 )
list(APPEND THRIFT_CPP_GEN_SRC_LIST
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/conn_mgr_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/devport_mgr_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/port_mgr_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mc_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/mirror_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sd_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/res_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/res_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pkt_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/plcmt_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm_api_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/tm_api_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pal_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/knet_mgr_pd_rpc_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/pipemgr_api_constants.h
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts_pd_rpc_constants.cpp
${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/ts_pd_rpc_constants.h
)
endif()

add_library(pdfixed_thrift_o OBJECT ${THRIFT_CPP_SRC_LIST} ${THRIFT_CPP_GEN_SRC_LIST})

add_library(pdfixed_thrift SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:pdfixed_thrift_o>)
target_link_libraries(pdfixed_thrift ${THRIFT_LIBRARIES})

SET(THRIFT_FILES
thrift/conn_mgr_pd_rpc.thrift
thrift/port_mgr_pd_rpc.thrift
thrift/mc_pd_rpc.thrift
thrift/mirror_pd_rpc.thrift
thrift/sd_pd_rpc.thrift
thrift/pkt_pd_rpc.thrift
thrift/res.thrift
thrift/tm_api.thrift
thrift/pal_rpc.thrift
thrift/plcmt_pd_rpc.thrift
thrift/knet_mgr_pd_rpc.thrift
thrift/devport_mgr_pd_rpc.thrift
thrift/pipemgr_api.thrift
thrift/ts_pd_rpc.thrift)

file(GLOB thrift_files ${CMAKE_CURRENT_SOURCE_DIR}/thrift/*thrift)
add_custom_command(OUTPUT ${THRIFT_CPP_GEN_SRC_LIST}
    DEPENDS ${THRIFT_COMPILER}
    DEPENDS ${thrift_files}
    COMMAND echo "DIR is: ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/conn_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/conn_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/port_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/port_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/mc_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/mc_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/mirror_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/mirror_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/sd_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/sd_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pkt_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pkt_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/res.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/res.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/tm_api.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/tm_api.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pal_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pal_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/plcmt_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/plcmt_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/knet_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/knet_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/devport_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/devport_mgr_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pipemgr_api.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/pipemgr_api.thrift
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/ts_pd_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/thrift/ts_pd_rpc.thrift
)
include(PythonDependencies)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen-py/ DESTINATION ${SDE_PYTHON_DEPENDENCIES_DIR}/tofino)
