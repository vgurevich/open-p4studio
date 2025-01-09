project(libbfutils VERSION 0.1 LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")
add_subdirectory(dynamic_hash)
add_library(bfsysutil_o OBJECT
  bf_utils.c
)
add_library(bfutils SHARED
  $<TARGET_OBJECTS:bfsysutil_o>
  $<TARGET_OBJECTS:dynhash_o>
)
add_dependencies(bfutils build_edit)
ExternalProject_Get_Property(build_edit binary_dir)
target_link_libraries(bfutils PUBLIC ${binary_dir}/src/.libs/libedit.a target_sys)
