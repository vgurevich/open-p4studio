include_directories(.)

add_subdirectory(dvm)
add_subdirectory(lld)
add_subdirectory(mc_mgr)
add_subdirectory(port_mgr)
add_subdirectory(traffic_mgr)
add_subdirectory(pkt_mgr)
add_subdirectory(pipe_mgr)
add_subdirectory(knet_mgr)
if (BFRT)
  add_subdirectory(bf_rt)
endif()
if(TDI)
  add_subdirectory(tdi_tofino)
endif()
if (PI)
  add_subdirectory(pi)
endif()
if (P4RT)
  add_subdirectory(p4_rt)
endif()

add_subdirectory(ctx_json)
add_subdirectory(pdfixed)
add_subdirectory(bf_pm)
add_subdirectory(dru_sim)
add_subdirectory(diag)
add_subdirectory(perf)

# copy the files to install. this happens at cmake configuration time
file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/sbus_master.rom
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/pcie_serdes.rom
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/serdes_A0.rom
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/serdes_B0.rom
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/tofino_sds_fw/avago/firmware)
file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/tof2_A0_grp_0_7_serdes.bin
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/tof2_A0_grp_8_serdes.bin
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/tofino_sds_fw/credo/firmware)
file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/microp_fw.bin
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/microp_fw/microp/tof2/fw)
file(COPY
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/tof3_serdes_1_32_fw.hex
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/tof3_serdes_0_fw.hex
  ${CMAKE_CURRENT_SOURCE_DIR}/firmware/tof3_serdes_0_pcie_fw.hex
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/tofino_sds_fw/alphawave/firmware)
