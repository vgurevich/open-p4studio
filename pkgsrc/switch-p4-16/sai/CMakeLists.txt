INCLUDE(ExternalProject)
include(PythonDependencies)
set(SAIMAPGEN ${CMAKE_CURRENT_SOURCE_DIR}/../s3/tools/sai_map_gen.py)
set(SAIMAPGEN_COMMAND ${PYTHON_COMMAND} ${SAIMAPGEN})
set(SAIMAPDIR ${CMAKE_CURRENT_SOURCE_DIR}/maps)
set(SAI_THRIFT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/generated/sai.thrift)
set(SAI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/SAI/inc)
set(SAI_INCLUDE_DIR_EX ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/SAI/experimental)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pedantic -Wnon-virtual-dtor")

add_custom_command(OUTPUT sai_map.cpp
  COMMAND ${SAIMAPGEN_COMMAND} ${SAIMAPDIR} .
  DEPENDS ${SAIMAPDIR}/*.json ${SAIMAPGEN}
)
add_custom_target(saimap DEPENDS sai_map.cpp)

set(SAI_THRIFT_SERVER_CPP "")
if(THRIFT-SWITCH)
  set(SAI_THRIFT_CPP
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sai_rpc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sai_types.cpp
  )
  if ( THRIFT_VERSION_STRING VERSION_LESS 0.14.0 )
    list(APPEND SAI_THRIFT_CPP
      ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp/sai_constants.cpp
  )
  endif()
  add_custom_command(OUTPUT ${SAI_THRIFT_CPP}
    COMMAND ${THRIFT_COMPILER} -o . --gen cpp -r ${SAI_THRIFT_FILE}
    DEPENDS ${SAI_THRIFT_FILE}
  )
  add_custom_target(sai_thrift_cpp_sources DEPENDS ${SAI_THRIFT_CPP})

  set(SAI_THRIFT_PY
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/constants.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/__init__.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/sai_rpc.py
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/sai_rpc-remote
    ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/ttypes.py
  )
  add_custom_command(OUTPUT ${SAI_THRIFT_PY}
    COMMAND ${THRIFT_COMPILER} -o . --gen py -r ${SAI_THRIFT_FILE}
    DEPENDS ${SAI_THRIFT_FILE}
  )
  add_custom_target(sai_thrift_py_sources DEPENDS ${SAI_THRIFT_PY})

  find_program(CTYPESGEN ctypesgen REQUIRED)
  add_custom_command(OUTPUT sai_headers.py
    COMMAND ${PYTHON_COMMAND} ${CTYPESGEN} -I${SAI_INCLUDE_DIR} -I${SAI_INCLUDE_DIR_EX} ${SAI_INCLUDE_DIR}/*.h -o sai_headers.py
    DEPENDS ${SAI_INCLUDE_DIR}/*.h
  )
  set(SAI_THRIFT_SERVER_CPP sai_rpc_frontend.cpp)
  add_custom_target(saidata ALL DEPENDS saimap switch sai_thrift_cpp_sources sai_thrift_py_sources sai_headers.py)
else()
  add_custom_target(saidata ALL DEPENDS saimap switch)
endif()

set(PATCH_FILES)
FILE(GLOB var "${CMAKE_CURRENT_SOURCE_DIR}/patch/*.patch")
foreach(FILE ${var})
  list(APPEND PATCH_FILES 
    "patch -p0 < ${FILE};"
  )
endforeach(FILE)

ExternalProject_Add(saimetadata
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/SAI/
  BUILD_IN_SOURCE 1
  BUILD_ALWAYS 1
  UPDATE_COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saiextensions.h experimental/ #overwrite
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/sainexthopextensions.h experimental/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saibridgeextensions.h experimental/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/sairouterinterfaceextensions.h experimental/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saiswitchextensions.h experimental/  #overwrite
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saivlanextensions.h experimental/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saiipmcgroupextensions.h experimental/
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/extensions/saiaclextensions.h experimental/
  PATCH_COMMAND bash -c "${PATCH_FILES}"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND cd meta && make
  INSTALL_COMMAND cp meta/saimetadata.c ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND cp meta/saimetadata.h ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND sed -i "1s,^,// clang-format off," ${CMAKE_CURRENT_SOURCE_DIR}/saimetadata.h
  COMMAND sed -i "1s,^,// clang-format off," ${CMAKE_CURRENT_SOURCE_DIR}/saimetadata.c
  COMMAND echo "// clang-format on" >> ${CMAKE_CURRENT_SOURCE_DIR}/saimetadata.h
  COMMAND echo "// clang-format on" >> ${CMAKE_CURRENT_SOURCE_DIR}/saimetadata.c
  COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gensairpc.pl --clean --no-meta-build
  TEST_AFTER_INSTALL 1
  TEST_COMMAND make clean && git reset --hard && git clean -fxd
)
ExternalProject_Add_StepDependencies(saimetadata build ${CMAKE_CURRENT_SOURCE_DIR}/sai.cpp)
set_target_properties(saimetadata PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_library(sai_o OBJECT
  saimetadata.c
  saimetadata.h
  ../submodules/SAI/meta/saimetadatautils.h
  ../submodules/SAI/meta/saimetadatautils.c
  ../submodules/SAI/meta/saiserialize.h
  ../submodules/SAI/meta/saiserialize.c
  ${CMAKE_CURRENT_BINARY_DIR}/sai_map.cpp
  sai_bflib.cpp
  sai.cpp
  saiacl.cpp
  saibfd.cpp
  saibridge.cpp
  saibuffer.cpp
  saidebugcounter.cpp
  saidtel.cpp
  saifdb.cpp
  saihash.cpp
  saihostif.cpp
  saiipmc.cpp
  saiipmcgroup.cpp
  sail2mc.cpp
  sail2mcgroup.cpp
  saiisolationgroup.cpp
  sailag.cpp
  saimirror.cpp
  saimpls.cpp
  sainat.cpp
  sainexthop.cpp
  sainexthopgroup.cpp
  saineighbor.cpp
  saiobject.cpp
  saiport.cpp
  saipolicer.cpp
  saiqosmap.cpp
  saiqueue.cpp
  sairoute.cpp
  sairouterinterface.cpp
  sairpfgroup.cpp
  saistp.cpp
  saisamplepacket.cpp
  saischeduler.cpp
  saischedulergroup.cpp
  saiswitch.cpp
  saitunnel.cpp
  saiudf.cpp
  saiutils.cpp
  saivlan.cpp
  saivirtualrouter.cpp
  saiwred.cpp
  saisrv6.cpp
  saicounter.cpp
  saimymac.cpp
  $<$<BOOL:${THRIFT-SWITCH}>:sai_rpc_frontend.cpp>
  $<$<BOOL:${THRIFT-SWITCH}>:${SAI_THRIFT_CPP}>
)
set_source_files_properties(saimetadata.c PROPERTIES COMPILE_FLAGS "-Wno-enum-conversion -Wno-tautological-pointer-compare")
add_dependencies(sai_o saidata)
target_include_directories(sai_o PRIVATE .)
target_include_directories(sai_o PRIVATE extensions)
target_include_directories(sai_o PRIVATE ${SAI_INCLUDE_DIR})
target_include_directories(sai_o PRIVATE ${SAI_INCLUDE_DIR}/../)
target_include_directories(sai_o PRIVATE ${SAI_INCLUDE_DIR}/../meta)
target_include_directories(sai_o PRIVATE ${SAI_INCLUDE_DIR}/../experimental)
target_include_directories(sai_o PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(sai_o PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen-cpp)
target_include_directories(sai_o PRIVATE ${CMAKE_BINARY_DIR}/api)

target_compile_options(sai_o PRIVATE -Wno-unused-parameter -Wno-gnu-designator)
if (NOT STATIC-LINK-LIB)
  add_library(sai SHARED $<TARGET_OBJECTS:sai_o>)
  target_link_libraries(sai PUBLIC bf_switch)
endif()

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', standard_lib=True, plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(THRIFT-SWITCH)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gen-py/sai/ DESTINATION ${PYTHON_SITE}/site-packages/sai_thrift)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sai_headers.py DESTINATION ${PYTHON_SITE}/site-packages/sai_thrift)
endif()

install(DIRECTORY ${SAI_INCLUDE_DIR}/ DESTINATION include/sai)
install(DIRECTORY ${SAI_INCLUDE_DIR}/../experimental/ DESTINATION include/sai)
