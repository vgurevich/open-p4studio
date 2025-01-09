
project(libpdfixed VERSION 0.1 LANGUAGES C)

add_library(pdfixed_o OBJECT
pd_conn_mgr.c
pd_devport_mgr.c
pd_port_mgr.c
pd_mau_snapshot.c
pd_mau_tbl_dbg_counters.c
pd_mc.c
pd_sd.c
pd_mirror.c
pd_ms.c
pd_pkt.c
pd_plcmt.c
pd_tm.c
pd_knet_mgr.c
pd_ts.c
../bf_pal/bf_pal_port_intf.c
../bf_pal/dev_intf.c
../bf_pal/pltfm_func_mgr.c
../bf_pal/bf_pal_pltfm_porting.c
)
set_source_files_properties(pd_mau_tbl_dbg_counters.c PROPERTIES COMPILE_FLAGS -Wno-stringop-truncation)
add_library(pdfixed SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:pdfixed_o>)

# Building pdfixed doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-pdfixed
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating PD Fixed Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-pdfixed PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/pdfixed OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/pdfixed OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
