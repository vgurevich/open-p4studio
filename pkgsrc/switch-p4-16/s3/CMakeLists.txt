set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")

add_library(fmtobj OBJECT third_party/fmtlib/format.cc)

add_library(parserobj OBJECT parser/model_parser.cpp)
add_library(parserlib SHARED $<TARGET_OBJECTS:parserobj>)
target_link_libraries(parserlib PUBLIC cjson target_utils target_sys)

add_library(s3 OBJECT
  parser/model_parser.cpp
  third_party/fmtlib/format.cc
  attribute.c
  attribute_util.cpp
  bf_rt_backend.cpp
  log.cpp
  event.cpp
  store.cpp
  switch_store.cpp
  factory.cpp
  smi.cpp
  record.cpp
  switch_packet.c
  switch_utils.c
  switch_lpm.c
  cli/bf_switch_cli_api.cpp
  switch_bfdd.c
  bfd_timer.c
)
target_include_directories(s3 PRIVATE .)
target_compile_options(s3 PRIVATE -Wno-pedantic)

add_executable(enum_gen tools/enum_gen.cpp)
target_link_libraries(enum_gen parserlib)

add_executable(doc_gen tools/doc_gen.cpp)
target_link_libraries(doc_gen parserlib)

add_executable(aug_model_json_gen tools/aug_model_json_gen.cpp)
target_link_libraries(aug_model_json_gen parserlib)

add_executable(graphgen tools/graphgen.cpp)
target_link_libraries(graphgen parserlib)

add_subdirectory(test EXCLUDE_FROM_ALL)
