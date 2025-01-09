include(CTest)

#add_subdirectory(../../../third-party/googletest third-party/googletest)

add_executable(tdi_tofino_utest
  main.cpp
  tdi_tofino_context_json_test.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../tdi_common/")

target_compile_options(tdi_tofino_utest PRIVATE
  #TODO remove compiler detected warning
  -Wno-error -Wno-unused-but-set-variable -Wno-unused-variable
  -Wno-unused-parameter -Wno-maybe-uninitialized
  "-DJSONDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tofino_context_json_files\""
  #TODO put them in config.h
  "-DINSTALLDIR=\"${CMAKE_INSTALL_PREFIX}/share\""
  "-DTDI_SHARED_INSTALLDIR=\"${CMAKE_INSTALL_PREFIX}/share/tdi_shared\""
)

target_link_libraries (tdi_tofino_utest
  gtest  # gtest_main
  gmock
  tdi_tofino
  driver
  target_utils
  target_sys
)

add_test(TDI-TOFINO-UT-ALL tdi_tofino_utest)
add_custom_target(checktditofino
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS
    tdi_tofino_utest
)
add_custom_target(checktditofinoprep
  COMMAND make install
  DEPENDS
    # utillize josn file from below installed target
    x2_tofino
    tna_meter_lpf_wred
    tna_idletimeout
    tna_32q_2pipe
    tna_exact_match
    tna_counter
    tna_action_selector
    tna_action_profile
    tna_multicast
)
