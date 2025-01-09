project(libpkt_mgr VERSION 0.1 LANGUAGES C)

add_library(bfpkt_mgr_o OBJECT
pkt_mgr_drv.c
pkt_mgr_log.h
pkt_mgr_pkt.c
pkt_mgr_txrx.c
pkt_mgr_ucli.c)

add_library(bfpkt_mgr SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bfpkt_mgr_o>)
target_link_libraries(bfpkt_mgr lld)

# Building packet manager doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-pkt-mgr
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating Packet Manager Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-pkt-mgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/pkt_mgr OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/pkt_mgr OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
