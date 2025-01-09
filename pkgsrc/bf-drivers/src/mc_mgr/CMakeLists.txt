project(bfmc_mgr VERSION 0.1 LANGUAGES C)

add_library(bfmc_mgr_o OBJECT
mc_mgr.c
mc_mgr.h
mc_mgr_bh.c
mc_mgr_bh.h
mc_mgr_drv.c
mc_mgr_drv.h
mc_mgr_handle.h
mc_mgr_handle.c
mc_mgr_ha.h
mc_mgr_ha_cd.c
mc_mgr_ha_rd.c
mc_mgr_int.h
mc_mgr_intf.c
mc_mgr_log.h
mc_mgr_mem.c
mc_mgr_mem.h
mc_mgr_node.c
mc_mgr_node.h
mc_mgr_rdm.c
mc_mgr_rdm_tof.c
mc_mgr_rdm_tof2.c
mc_mgr_rdm_tof3.c
mc_mgr_rdm.h
mc_mgr_reg.c
mc_mgr_reg.h
mc_mgr_shared_dr_wrapper.c
mc_mgr_test.c
mc_mgr_ucli.c)

add_library(bfmc_mgr SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bfmc_mgr_o>)
target_link_libraries(bfmc_mgr lld)

# Building multicast manager doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-mc-mgr
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating Multicast Manager Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-mc-mgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/mc_mgr OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/mc_mgr OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
