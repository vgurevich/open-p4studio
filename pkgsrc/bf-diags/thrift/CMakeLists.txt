include(PythonDependencies)

find_package(Thrift REQUIRED)

include_directories(SYSTEM ${THRIFT_INCLUDE_DIR})
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/gen-cpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

set(THRIFT_CPP_GEN_SRC_LIST
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc.h
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc_types.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc_types.h
        )
if ( THRIFT_VERSION_STRING VERSION_LESS 0.14.0 )
    list(APPEND THRIFT_CPP_GEN_SRC_LIST
        ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc_constants.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/diag_rpc_constants.h
    )
endif()

add_library(diag_thrift_o OBJECT src/diag_rpc_server.cpp ${THRIFT_CPP_GEN_SRC_LIST})
target_compile_options(diag_thrift_o PRIVATE -Wno-unused-parameter -Wno-unused-variable)

add_library(diag_thrift SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:diag_thrift_o>)
target_link_libraries(diag_thrift ${THRIFT_LIBRARIES})
SET(THRIFT_FILES diag_rpc.thrift)

add_custom_command(OUTPUT ${THRIFT_CPP_GEN_SRC_LIST}
    DEPENDS ${THRIFT_COMPILER} ${THRIFT_FILES}
    COMMAND ${THRIFT_COMPILER} --gen cpp:templates,cob_style -r ${CMAKE_CURRENT_SOURCE_DIR}/diag_rpc.thrift
    COMMAND ${THRIFT_COMPILER} --gen py -r ${CMAKE_CURRENT_SOURCE_DIR}/diag_rpc.thrift
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen-py/ DESTINATION ${SDE_PYTHON_DEPENDENCIES_DIR}/tofino)
