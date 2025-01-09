enable_testing()

include_directories(../ ${CMAKE_CURRENT_BINARY_DIR})

add_compile_options("-DTESTDATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/..\"" "-DTESTING" "-DEVENTS_ENABLED")
add_compile_options("-Wno-pedantic")
add_definitions(-UNDEBUG)

add_library(tests3 STATIC
  ../attribute.c
  ../attribute_util.cpp
  ../log.cpp
  ../event.cpp
  ../store.cpp
  ../switch_store.cpp
  ../factory.cpp
  ../smi.cpp
  ../record.cpp
  $<TARGET_OBJECTS:parserobj>
  $<TARGET_OBJECTS:fmtobj>
)
target_link_libraries(tests3 cjson target_utils target_sys)

add_custom_command(OUTPUT gen-model/test_model.h
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/gen-model
  COMMAND ../enum_gen --input ${CMAKE_CURRENT_SOURCE_DIR}/test_model.json --output gen-model/test_model.h
  DEPENDS ../enum_gen ${CMAKE_CURRENT_SOURCE_DIR}/test_model.json
)
add_custom_command(OUTPUT gen-model/replay.h
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/gen-model
  COMMAND ../enum_gen --input ${CMAKE_CURRENT_SOURCE_DIR}/replay.json --output gen-model/replay.h
  DEPENDS ../enum_gen ${CMAKE_CURRENT_SOURCE_DIR}/replay.json
)
add_custom_command(OUTPUT gen-model/reference_validation.h
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/gen-model
  COMMAND ../enum_gen --input ${CMAKE_CURRENT_SOURCE_DIR}/test_reference.json --output gen-model/reference_validation.h
  DEPENDS ../enum_gen ${CMAKE_CURRENT_SOURCE_DIR}/test_reference.json
)

add_custom_command(OUTPUT gen-model/test_keygroup_list.h
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/gen-model
  COMMAND ../enum_gen --input ${CMAKE_CURRENT_SOURCE_DIR}/test_keygroup_list.json --output gen-model/test_keygroup_list.h
  DEPENDS ../enum_gen ${CMAKE_CURRENT_SOURCE_DIR}/test_keygroup_list.json
)
add_custom_target(testheaders DEPENDS gen-model/test_model.h gen-model/replay.h gen-model/reference_validation.h gen-model/test_keygroup_list.h)
add_dependencies(tests3 testheaders)

add_executable(test_attribute_c ../attribute.c test_attribute.c)
add_test(attribute_c test_attribute_c)

add_executable(test_attribute_cpp test_attribute.cpp)
target_link_libraries(test_attribute_cpp tests3)
add_test(attribute_cpp test_attribute_cpp)

add_executable(test_switch_store test_switch_store.cpp gen-model/test_model.h)
target_link_libraries(test_switch_store tests3 pthread)
add_test(switch_store test_switch_store)

add_test(switch_store_lock test_switch_store test_lock)
set_tests_properties(switch_store_lock PROPERTIES TIMEOUT 10)

add_executable(test_smi test_smi.cpp gen-model/test_model.h)
target_link_libraries(test_smi tests3)
add_test(smi test_smi)

add_executable(test_auto_objects test_auto_objects.cpp gen-model/test_model.h)
target_link_libraries(test_auto_objects tests3)
add_test(auto_objects test_auto_objects)

add_executable(test_events test_events_common.cpp test_events.cpp gen-model/test_model.h)
target_link_libraries(test_events tests3)
target_compile_options(test_events PRIVATE "-DEVENTS_ENABLED")
add_test(events test_events)

add_executable(test_events_c test_events_common.cpp test_events.c gen-model/test_model.h)
target_link_libraries(test_events_c tests3)
target_compile_options(test_events_c PRIVATE "-DEVENTS_ENABLED")
add_test(events_c test_events_c)

add_executable(test_logging test_logging.cpp gen-model/test_model.h)
target_link_libraries(test_logging tests3)
add_test(logging test_logging)

add_executable(test_counters test_counters.cpp gen-model/test_model.h)
target_link_libraries(test_counters tests3)
add_test(counters test_counters)

add_executable(test_db test_db_store.cpp gen-model/test_model.h)
target_link_libraries(test_db tests3 pthread)
add_test(db test_db)

add_executable(test_perf test_perf.cpp gen-model/test_model.h)
target_link_libraries(test_perf tests3 pthread)
add_test(perf test_perf)

add_executable(test_replay test_replay.cpp gen-model/replay.h)
target_link_libraries(test_replay tests3)
add_test(replay test_replay)

add_executable(test_reference_validation test_reference_validation.cpp gen-model/reference_validation.h)
target_link_libraries(test_reference_validation tests3)
add_test(reference_validation test_reference_validation)

add_executable(test_ids test_ids.cpp)
add_test(ids test_ids)

add_executable(test_keygroup_list test_keygroup_list.cpp gen-model/test_keygroup_list.h)
target_link_libraries(test_keygroup_list tests3)
add_test(switch_store_key_group_list test_keygroup_list)

add_executable(test_packet test_packet.c bf_pkt_mock.c ../switch_packet.c ../switch_utils.c)
add_test(packet test_packet)
target_compile_options(test_packet PRIVATE -Wno-unused-parameter)
target_link_libraries(test_packet PUBLIC
  pthread
  ${NETLINK3}
  ${NETLINKROUTE3}
  ${NETLINKGENL3}
  target_utils
  target_sys
)
# allow execute without sudo
add_custom_command(TARGET test_packet POST_BUILD
                   COMMAND sudo setcap cap_net_raw,cap_net_admin+eip ${CMAKE_CURRENT_BINARY_DIR}/test_packet)

add_executable(test_bfdd
  test_bfdd.c
  bf_pkt_mock.c
  ../switch_bfdd.c
  ../switch_utils.c
  ../bfd_timer.c
  ../switch_packet.c
)
add_test(bfdd test_bfdd)
target_compile_options(test_bfdd PRIVATE -Wno-unused-parameter)
target_link_libraries(test_bfdd PUBLIC
  pthread
  ${NETLINK3}
  ${NETLINKROUTE3}
  ${NETLINKGENL3}
  target_utils
  target_sys
)

add_custom_target(check-switch-p4-16 COMMAND ${CMAKE_CTEST_COMMAND}
  DEPENDS
  test_attribute_c
  test_attribute_cpp
  test_switch_store
  test_smi
  test_auto_objects
  test_events
  test_events_c
  test_logging
  test_counters
  test_perf
  test_db
  test_replay
  test_ids
  test_reference_validation
  test_keygroup_list
  test_packet
  test_bfdd)
