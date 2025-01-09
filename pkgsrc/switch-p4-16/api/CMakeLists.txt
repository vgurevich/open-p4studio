include(PythonDependencies)

set(GENCLI ${CMAKE_CURRENT_SOURCE_DIR}/../s3/tools/gen/gencli.py)
set(GENCLI_COMMAND ${PYTHON_COMMAND} ${GENCLI})
set(TEMPLATEDIR ${CMAKE_CURRENT_SOURCE_DIR}/templates)
set(TEMPLATEGEN ${CMAKE_CURRENT_SOURCE_DIR}/../s3/tools/gen/template_generator.py)
set(TEMPLATEGEN_COMMAND ${PYTHON_COMMAND} ${TEMPLATEGEN})
set(SWITCH_THRIFT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/common/bf_switcht_api.thrift)
set(SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../schema)
set(MERGEJSON ${SCHEMA_DIR}/mergejson.py)
set(MERGEJSON_COMMAND ${PYTHON_COMMAND} ${MERGEJSON})
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,defs")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic -Wnon-virtual-dtor")

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gen-cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../submodules/SAI/inc)

file(GLOB BF_SWITCH_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/bf_switch/*.h)

# build model artifacts
add_custom_command(OUTPUT switch.json
  COMMAND ${MERGEJSON_COMMAND} ${SCHEMA_DIR} switch.json
  DEPENDS ${SCHEMA_DIR}/appObj/*.json ${SCHEMA_DIR}/asicObj/*.json ${MERGEJSON}
)
add_custom_command(OUTPUT aug_model.json
  COMMAND aug_model_json_gen --input switch.json --output aug_model.json
  DEPENDS aug_model_json_gen switch.json
)
add_custom_command(OUTPUT model.h
  COMMAND enum_gen --input switch.json --output model.h
  DEPENDS enum_gen switch.json
)
add_custom_command(OUTPUT model.dox
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/doxygen
  COMMAND doc_gen --input switch.json --output ${CMAKE_CURRENT_BINARY_DIR}/doxygen/model.dox
  DEPENDS doc_gen switch.json
)
add_custom_command(OUTPUT bf_switchapi.xml
  COMMAND ${GENCLI_COMMAND} --template_dir ${TEMPLATEDIR} --template_file bf_switchapi.xml --model_json ${CMAKE_CURRENT_BINARY_DIR}/aug_model.json --xo .
  DEPENDS aug_model.json ${TEMPLATEDIR}/bf_switchapi.xml ${GENCLI}
)
add_custom_command(OUTPUT smi_api_adapter.c
  COMMAND ${TEMPLATEGEN_COMMAND} aug_model.json ${TEMPLATEDIR} smi_api_adapter_template.c smi_api_adapter.c
  DEPENDS aug_model.json
)
add_custom_command(OUTPUT smi_api_adapter.h
  COMMAND ${TEMPLATEGEN_COMMAND} aug_model.json ${TEMPLATEDIR} smi_api_adapter_template.h smi_api_adapter.h
  DEPENDS aug_model.json
)
add_custom_target(schema DEPENDS model.h model.dox aug_model.json bf_switchapi.xml smi_api_adapter.c smi_api_adapter.h)

set(THRIFT_SERVER_CPP "")
if(THRIFT-SWITCH)
  set(THRIFT_CPP
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/bf_switcht_api_rpc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/bf_switcht_api_types.cpp
  )
  if ( THRIFT_VERSION_STRING VERSION_LESS 0.14.0 )
    list(APPEND THRIFT_CPP
      ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/bf_switcht_api_constants.cpp
  )
  endif()
  add_custom_command(OUTPUT ${THRIFT_CPP}
    COMMAND ${THRIFT_COMPILER} -o . --gen cpp -r ${SWITCH_THRIFT_FILE}
    DEPENDS ${SWITCH_THRIFT_FILE}
  )
  add_custom_target(thrift_cpp_sources DEPENDS ${THRIFT_CPP})

  set(THRIFT_PY
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/constants.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/__init__.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/bf_switcht_api_rpc.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/bf_switcht_api_rpc-remote
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/ttypes.py
  )
  add_custom_command(OUTPUT ${THRIFT_PY}
    COMMAND ${THRIFT_COMPILER} -o . --gen py -r ${SWITCH_THRIFT_FILE}
    DEPENDS ${SWITCH_THRIFT_FILE}
  )
  add_custom_target(thrift_py_sources DEPENDS ${THRIFT_PY})

  add_custom_command(OUTPUT api_adapter.py
    COMMAND ${TEMPLATEGEN_COMMAND} aug_model.json ${TEMPLATEDIR} ptf_api_adapter_template.py.tt api_adapter.py
    DEPENDS aug_model.json)

  find_program(CTYPESGEN ctypesgen REQUIRED)
  add_custom_command(OUTPUT model_headers.py
    COMMAND ${PYTHON_COMMAND} ${CTYPESGEN} -I${CMAKE_CURRENT_BINARY_DIR} model.h -I${CMAKE_CURRENT_SOURCE_DIR}/../include/bf_switch ${BF_SWITCH_HEADER_FILES} -o model_headers.py &> /dev/null
    DEPENDS model.h)

  set(THRIFT_SERVER_CPP common/bf_switcht_api_rpc_server.cpp)
  set(THRIFT_INSTALL_PY ${THRIFT_PY} api_adapter.py model_headers.py)
  add_custom_target(switchdata ALL DEPENDS schema thrift_cpp_sources thrift_py_sources api_adapter.py model_headers.py)
else()
  add_custom_target(switchdata ALL DEPENDS schema)
endif()

add_library(switch OBJECT
  common/bfrt_tm.cpp
  common/hostif.cpp
  common/pal.cpp
  common/qos_pdfixed.cpp
  common/utils.cpp
  switch_tna/acl.cpp
  switch_tna/afp.cpp
  switch_tna/bf_rt_ids.cpp
  switch_tna/bf_rt_ids.h
  switch_tna/device.cpp
  switch_tna/dtel.cpp
  switch_tna/etrap.cpp
  switch_tna/features.cpp
  switch_tna/fp.cpp
  switch_tna/hash.cpp
  switch_tna/hostif_trap.cpp
  switch_tna/init.cpp
  switch_tna/l2.cpp
  switch_tna/l3.cpp
  switch_tna/meter.cpp
  switch_tna/multicast.cpp
  switch_tna/mirror.cpp
  switch_tna/mpls.cpp
  switch_tna/nat.cpp
  switch_tna/nexthop.cpp
  switch_tna/pfc_wd.cpp
  switch_tna/pktpath.cpp
  switch_tna/port.cpp
  switch_tna/qos.cpp
  switch_tna/rmac.cpp
  switch_tna/rewrite.cpp
  switch_tna/sflow.cpp
  switch_tna/triggers.cpp
  switch_tna/tunnel.cpp
  switch_tna/tables.cpp
  switch_tna/utils.cpp
  switch_tna/validation.cpp
  switch_tna/validation_fp.cpp
  switch_tna/wred.cpp
  switch_tna/bfd.cpp
  ${THRIFT_CPP}
  ${THRIFT_SERVER_CPP}
  common/bf_switch.cpp
  common/bf_switch_api.cpp
  $<$<BOOL:${THRIFT-SWITCH}>:test/test_api_perf.cpp>
)
add_dependencies(switch switchdata)
target_compile_options(switch PRIVATE -Wno-pedantic)

if (NOT STATIC-LINK-LIB)
  add_library(bf_switch SHARED
    $<TARGET_OBJECTS:s3>
    $<TARGET_OBJECTS:switch>
  )
  target_link_libraries(bf_switch PUBLIC
    pthread
    ${NETLINK3}
    ${NETLINKROUTE3}
    ${NETLINKGENL3}
    cjson
    driver
    target_utils
    target_sys
  )
  if(THRIFT-SWITCH)
    target_link_libraries(bf_switch PUBLIC ${THRIFT_LIBRARIES})
  endif()
endif()

add_library(bfshell_plugin_bf_switchapi_o OBJECT
  ../s3/cli/bf_switch_cli.cpp
  ../s3/cli/bf_switch_cli_clish.cpp
)
add_dependencies(bfshell_plugin_bf_switchapi_o switch)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(bfshell_plugin_bf_switchapi_o PRIVATE -Wno-unused-parameter -Wno-reserved-user-defined-literal)
else()
  target_compile_options(bfshell_plugin_bf_switchapi_o PRIVATE -Wno-unused-parameter)
endif()

if (NOT STATIC-LINK-LIB)
  add_library(bfshell_plugin_bf_switchapi SHARED $<TARGET_OBJECTS:bfshell_plugin_bf_switchapi_o>)
  SET_TARGET_PROPERTIES(bfshell_plugin_bf_switchapi PROPERTIES PREFIX "")
  target_link_libraries(bfshell_plugin_bf_switchapi PUBLIC
    bf_switch
    clish
  )
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aug_model.json DESTINATION share/switch)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/model.h DESTINATION include/bf_switch)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bf_switchapi.xml DESTINATION share/cli/xml)

if(THRIFT-SWITCH)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen-py/bf_switcht_api/ DESTINATION ${SDE_GENERATED_PYTHON_DIR}/bf_switcht_api_thrift)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/api_adapter.py DESTINATION ${SDE_GENERATED_PYTHON_DIR}/bf_switcht_api_thrift)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/model_headers.py DESTINATION ${SDE_GENERATED_PYTHON_DIR}/bf_switcht_api_thrift)
endif()
