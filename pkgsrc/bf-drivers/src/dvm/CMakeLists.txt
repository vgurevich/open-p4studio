project(libdvm VERSION 0.1 LANGUAGES C)

add_library(dvm_o OBJECT
  dvm.c
  dvm_clients.c
  dvm.h
  dvm_log.h
  dvm_err_events.c
  dvm_ucli.c
  bf_drv_shell.c
)
add_library(dvm SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:dvm_o>)

# Building DVM doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-dvm
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating knet Manager Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-dvm PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/dvm OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/dvm OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
