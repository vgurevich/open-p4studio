include(P4Build)

set (program_name "tna_32q_multiprogram")

function(multiprogram_target target_suffix)
  add_custom_target(${program_name}-${target_suffix} DEPENDS ${program_name}_a-${target_suffix} ${program_name}_b-${target_suffix})
endfunction()

if (TOFINO)
  p4_build_target(${program_name}_a ${P4_tofino_ARCHITECTURE} "tofino" "${CMAKE_CURRENT_SOURCE_DIR}/program_a/tna_32q_multiprogram_a.p4")
  p4_build_target(${program_name}_b ${P4_tofino_ARCHITECTURE} "tofino" "${CMAKE_CURRENT_SOURCE_DIR}/program_b/tna_32q_multiprogram_b.p4")
  multiprogram_target("tofino")
endif()
if (TOFINO2)
  p4_build_target(${program_name}_a ${P4_tofino2_ARCHITECTURE} "tofino2" "${CMAKE_CURRENT_SOURCE_DIR}/program_a/tna_32q_multiprogram_a.p4")
  p4_build_target(${program_name}_b ${P4_tofino2_ARCHITECTURE} "tofino2" "${CMAKE_CURRENT_SOURCE_DIR}/program_b/tna_32q_multiprogram_b.p4")
  multiprogram_target("tofino2")
endif()
if (TOFINO2M)
  p4_build_target(${program_name}_a ${P4_tofino2_ARCHITECTURE} "tofino2m" "${CMAKE_CURRENT_SOURCE_DIR}/program_a/tna_32q_multiprogram_a.p4")
  p4_build_target(${program_name}_b ${P4_tofino2_ARCHITECTURE} "tofino2m" "${CMAKE_CURRENT_SOURCE_DIR}/program_b/tna_32q_multiprogram_b.p4")
  multiprogram_target("tofino2m")
endif()
if (TOFINO3)
  p4_build_target(${program_name}_a ${P4_tofino3_ARCHITECTURE} "tofino3" "${CMAKE_CURRENT_SOURCE_DIR}/program_a/tna_32q_multiprogram_a.p4")
  p4_build_target(${program_name}_b ${P4_tofino3_ARCHITECTURE} "tofino3" "${CMAKE_CURRENT_SOURCE_DIR}/program_b/tna_32q_multiprogram_b.p4")
  multiprogram_target("tofino3")
endif()

add_custom_target(${program_name} DEPENDS
  $<$<BOOL:${TOFINO}>:${program_name}-tofino>
  $<$<BOOL:${TOFINO2}>:${program_name}-tofino2>
  $<$<BOOL:${TOFINO2M}>:${program_name}-tofino2m>
  $<$<BOOL:${TOFINO3}>:${program_name}-tofino3>
)
