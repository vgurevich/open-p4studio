include(CTest)

add_subdirectory(../../../third-party/googletest third-party/googletest)

add_executable(bf_rt_utest
  main.cpp
  bf_rt_matchers.cpp
  bf_rt_entry_gen.cpp
  bf_rt_table_test.cpp
  bf_rt_info_test.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../bf_rt_common/")

target_compile_options(bf_rt_utest PRIVATE
  #TODO remove compiler detected warning
  -Wno-error -Wno-unused-but-set-variable -Wno-unused-variable
  -Wno-unused-parameter -Wno-maybe-uninitialized
  #TODO put them in config.h
  "-DINSTALLDIR=\"${CMAKE_INSTALL_PREFIX}/share\""
  "-DBFRT_SHARED_INSTALLDIR=\"${CMAKE_INSTALL_PREFIX}/share/bf_rt_shared\""
)

target_link_libraries (bf_rt_utest
  gtest  # gtest_main
  gmock
  bfrt
  driver
  target_utils
  target_sys
)

add_test(BFRT-UT-ALL bf_rt_utest)
add_custom_target(checkbfrt
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS
    bf_rt_utest
)
add_custom_target(checkbfrtprep
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
