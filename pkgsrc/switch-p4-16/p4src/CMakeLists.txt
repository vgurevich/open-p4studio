set(P4NAME "switch")
set(BF_RT_JSON "${CMAKE_CURRENT_BINARY_DIR}/${P4NAME}/bf-rt.json")

set(TOFINOFLAGS "--std" "p4-16" "--target" "tofino" "--arch" "tna")
set(TOFINO2FLAGS "--std" "p4-16" "--target" "tofino2" "--arch" "t2na")

# append command line flags to internal
separate_arguments(COMPUTED_P4FLAGS UNIX_COMMAND ${P4FLAGS})
separate_arguments(COMPUTED_P4PPFLAGS UNIX_COMMAND ${P4PPFLAGS})
set(P4FLAGS_INTERNAL "${COMPUTED_P4FLAGS}")
set(P4PPFLAGS_INTERNAL "${COMPUTED_P4PPFLAGS}")

set(P4FLAGS_INTERNAL "${P4FLAGS_INTERNAL}" "-o" "${CMAKE_CURRENT_BINARY_DIR}/${P4NAME}")
set(P4FLAGS_INTERNAL "${P4FLAGS_INTERNAL}" "--bf-rt-schema" "${BF_RT_JSON}")
set(P4FLAGS_INTERNAL "${P4FLAGS_INTERNAL}" "-I" "${CMAKE_CURRENT_SOURCE_DIR}/shared")
set(P4FLAGS_INTERNAL "${P4FLAGS_INTERNAL}" "--display-power-budget")
set(P4FLAGS_INTERNAL "${P4FLAGS_INTERNAL}" "-g")

# FIXME: remove disabling of parser min/max depth limits (P4C-4170)
set(P4PPFLAGS_INTERNAL_TF1 "${P4PPFLAGS_INTERNAL}" "-Xp4c='--disable-power-check" "--disable-parse-depth-limit" "-T" "stage_advance:2>stage_advance.log'")
set(P4PPFLAGS_INTERNAL_TF2 "${P4PPFLAGS_INTERNAL}" "-Xp4c='-T" "stage_advance:2>stage_advance.log'")
set(P4PP_WARNING_FLAGS "-Xp4c='--Wdisable=uninitialized_out_param" "--Wdisable=uninitialized_use" "--Wdisable=unused" "--Wdisable=substitution" "--Wdisable=table-placement'")

set(TOFINO_BIN "${CMAKE_CURRENT_BINARY_DIR}/${P4NAME}/*/*.bin")
set(CONTEXT_JSON "${CMAKE_CURRENT_BINARY_DIR}/${P4NAME}/*/context.json")

set(TOFINO_PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/switch-tofino")
set(TOFINO2_PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/switch-tofino2")

set(X1_TOFINO "${TOFINO_PROGRAMS}/switch_tofino_x1.p4")
add_custom_command(OUTPUT x1_compile COMMAND ${P4C} ${TOFINOFLAGS} ${P4FLAGS_INTERNAL} ${P4PP_WARNING_FLAGS} ${P4PPFLAGS_INTERNAL_TF1} ${X1_TOFINO} DEPENDS ${X1_TOFINO})
add_custom_target(x1_tofino DEPENDS x1_compile ${X1_TOFINO} bf-p4c)

set(X2_TOFINO "${TOFINO_PROGRAMS}/switch_tofino_x2.p4")
add_custom_command(OUTPUT x2_compile COMMAND ${P4C} ${TOFINOFLAGS} ${P4FLAGS_INTERNAL} ${P4PP_WARNING_FLAGS} ${P4PPFLAGS_INTERNAL_TF1} ${X2_TOFINO} DEPENDS ${X2_TOFINO})
add_custom_target(x2_tofino DEPENDS x2_compile ${X2_TOFINO} bf-p4c)

set(X9_TOFINO "${TOFINO_PROGRAMS}/switch_tofino_x9.p4")
add_custom_command(OUTPUT x9_compile COMMAND ${P4C} ${TOFINOFLAGS} ${P4FLAGS_INTERNAL} ${P4PP_WARNING_FLAGS} ${P4PPFLAGS_INTERNAL_TF1} ${X9_TOFINO} DEPENDS ${X9_TOFINO})
add_custom_target(x9_tofino DEPENDS x9_compile ${X9_TOFINO} bf-p4c)

set(Y2_TOFINO2 "${TOFINO2_PROGRAMS}/switch_tofino2_y2.p4")
add_custom_command(OUTPUT y2_compile COMMAND ${P4C} ${TOFINO2FLAGS} ${P4FLAGS_INTERNAL} ${P4PP_WARNING_FLAGS} ${P4PPFLAGS_INTERNAL_TF2} ${Y2_TOFINO2} DEPENDS ${Y2_TOFINO2})
add_custom_target(y2_tofino2 DEPENDS y2_compile ${Y2_TOFINO2} bf-p4c)

# install 3 files only
# bf-rt.json, tofino.bin/tofino2.bin, context.json
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/switch/ DESTINATION share/switch
  FILES_MATCHING
  FILES_MATCHING
  PATTERN "*.json"
  PATTERN "*.bin"
  PATTERN "logs" EXCLUDE
  PATTERN "graphs" EXCLUDE
  PATTERN "*dynhash*" EXCLUDE
  PATTERN "*prim*" EXCLUDE
  )
