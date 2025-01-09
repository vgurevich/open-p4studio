project(libdynhash VERSION 0.1 LANGUAGES C)
add_library(dynhash_o OBJECT
  dynamic_hash.c
  bfn_hash_algorithm.c)
add_library(dynhash SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:dynhash_o>)

# static lib for compiler with no syslibs
add_library(dynhashStatic STATIC dynamic_hash.c bfn_hash_algorithm.c)
set_target_properties(dynhashStatic PROPERTIES OUTPUT_NAME dynhash)
target_compile_definitions(dynhashStatic PUBLIC NO_BFSYSLIBS)
# FIXME: The library include structure is messed up here.
# The include should not be exported this way.
target_include_directories(dynhashStatic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include/bfutils)
