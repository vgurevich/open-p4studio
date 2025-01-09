project(libtraffic_mgr VERSION 0.1 LANGUAGES C)

add_library(bftraffic_mgr_o OBJECT
api/tm_api_cached_counters.c
api/tm_api_counter.c
api/tm_api_dev.c
api/tm_api_ecc.c
api/tm_api_helper.c
api/tm_api_helper.h
api/tm_api_mcast.c
api/tm_api_mode.c
api/tm_api_pipe.c
api/tm_api_pool.c
api/tm_api_port.c
api/tm_api_ppg.c
api/tm_api_queue.c
api/tm_api_read.c
api/tm_api_sch.c
common/tm_ctx.h
common/tm_dev.c
common/tm_dev.h
common/tm_eg_pools.c
common/tm_eg_pools.h
common/tm_error.h
common/tm_hw_access.c
common/tm_hw_access.h
common/tm_ig_pools.c
common/tm_ig_pools.h
common/tm_ig_ppg.c
common/tm_ig_ppg.h
common/tm_init.c
common/tm_intf.c
common/tm_mcast.c
common/tm_mcast.h
common/tm_path_counters.c
common/tm_path_counters.h
common/tm_pipe.c
common/tm_pipe.h
common/tm_port.c
common/tm_port.h
common/tm_queue.c
common/tm_queue.h
common/tm_restart.c
common/tm_restart_ut.c
common/tm_sch.c
common/tm_sch.h
common/traffic_mgr_int.h
common/traffic_mgr_log.h
hw_intf/tm_tof2_hw_intf.c
hw_intf/tm_tof2_hw_intf.h
hw_intf/tm_tof3_hw_intf.c
hw_intf/tm_tof3_hw_intf.h








hw_intf/tm_tofino_hw_intf.c
hw_intf/tm_tofino_hw_intf.h
init/tm_tof2.c
init/tm_tof2.h
init/tm_tof2_default.c
init/tm_tof3.c
init/tm_tof3.h
init/tm_tof3_default.c




init/tm_tofino.c
init/tm_tofino.h
init/tm_tofino_default.c
init/tm_tofinolite.c
init/tm_tofinolite.h
init/tm_tofinolite_default.c
ucli/traffic_mgr_ucli.c
ucli/traffic_mgr_ucli_apis.c
ucli/traffic_mgr_ucli_tables.c
)
set_source_files_properties(tm_restart_ut.c PROPERTIES COMPILE_FLAGS -Wno-absolute-value)

add_library(bftraffic_mgr SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bftraffic_mgr_o>)
target_link_libraries(bftraffic_mgr lld)

# Building traffic manager doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  # Tag file for external links to this manuail
  set(tagfile ${CMAKE_CURRENT_BINARY_DIR}/Tagfile_traffic_mgr)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-traffic-mgr
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating Traffic Manager Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-traffic-mgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/traffic_mgr OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/traffic_mgr OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
