project(libdru_sim VERSION 0.1 LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_C_FLAGS "${OLD_CMAKE_C_FLAGS}")
add_library(dru_sim_o OBJECT
dru_intf_fifo.c
dru_mti.c
dru_parse.c
dru_sim.c
dma_sim_intf.c
dru_intf_tcp.c
)
add_library(dru_sim SHARED $<TARGET_OBJECTS:dru_sim_o>)
separate_arguments(DRV_CFLAGS UNIX_COMMAND "${DRV_CFLAGS}")
target_compile_options(dru_sim_o PRIVATE ${DRV_CFLAGS})

add_library(dru_model STATIC
dru_intf_fifo.c
dru_mti.c
dru_parse.c
dru_sim.c
dma_sim_intf.c
dru_intf_tcp.c
../lld/lld_dr_regs_tof.c
../lld/lld_dr_regs_tof2.c
../lld/lld_dr_regs_tof3.c

../lld/lld_dr_regs.c)
target_compile_definitions(dru_model PRIVATE TARGET_IS_MODEL HARLYN_DEBUG_MODE=0 DEBUG_MODE=0 DEBUG_MODE_PARSE=0)

# Building DRU sim doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-dru-sim
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating DRU simulation Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-dru-sim PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/dru_sim OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/dru_sim OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
