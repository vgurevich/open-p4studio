project(libpipe_mgr VERSION 0.1 LANGUAGES C)

add_library(bfpipe_mgr_o OBJECT
bf_pipe_mgr_mirror_buffer.c
cuckoo_move.c
cuckoo_move.h
cuckoo_move_init.c
cuckoo_move_init.h
pipe_mgr_act_tbl.h
pipe_mgr_adt_drv_workflows.c
pipe_mgr_adt_drv_workflows.h
pipe_mgr_adt_init.h
pipe_mgr_adt_mgr.c
pipe_mgr_adt_mgr_dump.c
pipe_mgr_adt_mgr_dump.h
pipe_mgr_adt_mgr_init.c
pipe_mgr_adt_mgr_int.h
pipe_mgr_adt_tofino.c
pipe_mgr_adt_tofino.h
pipe_mgr_adt_transaction.c
pipe_mgr_adt_transaction.h
pipe_mgr_adt_ha_hlp.c
pipe_mgr_adt_ha_llp.c
pipe_mgr_adt_mgr_ha_int.h
pipe_mgr_adt_ucli.c
pipe_mgr_adt_ucli.h
pipe_mgr_alpm.c
pipe_mgr_alpm.h
pipe_mgr_alpm_ha.h
pipe_mgr_alpm_hlp_ha.c
pipe_mgr_bitmap.c
pipe_mgr_drv.c
pipe_mgr_drv.h
pipe_mgr_drv_intf.h
pipe_mgr_exm_drv_workflows.c
pipe_mgr_exm_drv_workflows.h
pipe_mgr_exm_hash.c
pipe_mgr_exm_hash.h
pipe_mgr_exm_tbl_dump.c
pipe_mgr_exm_tbl_dump.h
pipe_mgr_exm_tbl_init.c
pipe_mgr_exm_tbl_init.h
pipe_mgr_exm_tbl_mgr.c
pipe_mgr_exm_tbl_mgr.h
pipe_mgr_exm_tbl_mgr_int.h
pipe_mgr_exm_tof.h
pipe_mgr_exm_transaction.c
pipe_mgr_exm_transaction.h
pipe_mgr_exm_llp_ha.c
pipe_mgr_exm_hlp_ha.c
pipe_mgr_exm_ucli.c
pipe_mgr_exm_ucli.h
pipe_mgr_exm_utest.c
pipe_mgr_exm_utest.h
pipe_mgr_fake_rmt_cfg.c
pipe_mgr_hw_dump.c
pipe_mgr_hw_dump.h
pipe_mgr_ibuf.c
pipe_mgr_ibuf.h
pipe_mgr_idle_api.h
pipe_mgr_idle.c
pipe_mgr_idle.h
pipe_mgr_idle_sweep.c
pipe_mgr_idle_sweep.h
pipe_mgr_idle_ucli.c
pipe_mgr_instr.h
pipe_mgr_int.c
pipe_mgr_intf.c
pipe_mgr_int.h
pipe_mgr_tof2_cfg.c
pipe_mgr_tof2_cfg.h
pipe_mgr_tof3_cfg.c
pipe_mgr_tof3_cfg.h


pipe_mgr_tof2_mau_snapshot.c
pipe_mgr_tof2_mau_snapshot.h
pipe_mgr_tof_parde.c
pipe_mgr_tof_parde.h
pipe_mgr_tof3_mau_snapshot.c
pipe_mgr_tof3_mau_snapshot.h
pipe_mgr_tof2_parde.c
pipe_mgr_tof2_parde.h
pipe_mgr_tof3_parde.c
pipe_mgr_tof3_parde.h
pipe_mgr_learn.c
pipe_mgr_learn_ucli.c
pipe_mgr_learn.h
pipe_mgr_log.h
pipe_mgr_mc_intf.c
pipe_mgr_mutex.h
pipe_mgr_tof_mc_intf.h
pipe_mgr_tof_mc_intf.c
pipe_mgr_tof2_mc_intf.h
pipe_mgr_tof2_mc_intf.c
pipe_mgr_tof3_mc_intf.h
pipe_mgr_tof3_mc_intf.c
pipe_mgr_mirror_buffer_comm.h
pipe_mgr_tof_mirror_buffer.c
pipe_mgr_tof_mirror_buffer.h
pipe_mgr_tof2_mirror_buffer.c
pipe_mgr_tof2_mirror_buffer.h
pipe_mgr_tof3_mirror_buffer.c
pipe_mgr_tof3_mirror_buffer.h
pipe_mgr_mirror_buffer.c
pipe_mgr_mirror_buffer.h
pipe_mgr_mirror_buffer_ha.c
pipe_mgr_mirror_buffer_ha.h
pipe_mgr_move_list.h
pipe_mgr_parde.c
pipe_mgr_parde.h
pipe_mgr_parb.c
pipe_mgr_parb.h
pipe_mgr_phase0_tbl_mgr.c
pipe_mgr_phase0_tbl_mgr.h
pipe_mgr_pipeline_cfg_dnld.c
pipe_mgr_rmt_cfg.c
pipe_mgr_select_ha.h
pipe_mgr_select_ha_hlp.c
pipe_mgr_select_ha_llp.c
pipe_mgr_select_tbl.c
pipe_mgr_select_tbl.h
pipe_mgr_select_tbl_transaction.c
pipe_mgr_select_tbl_transaction.h
pipe_mgr_sel_tbl_ucli.c
pipe_mgr_session.c
pipe_mgr_session_int.h
pipe_mgr_sm.c
pipe_mgr_sm.h
pipe_mgr_stat_drv_workflows.c
pipe_mgr_stat_drv_workflows.h
pipe_mgr_stat_lrt.c
pipe_mgr_stat_lrt.h
pipe_mgr_stat_mgr_dump.c
pipe_mgr_stat_mgr_dump.h
pipe_mgr_stat_mgr_int.h
pipe_mgr_stats_tbl.h
pipe_mgr_stat_tbl_init.c
pipe_mgr_stat_tbl_init.h
pipe_mgr_stat_tbl_mgr.c
pipe_mgr_stat_trace.c
pipe_mgr_stat_trace.h
pipe_mgr_stat_ucli.c
pipe_mgr_stat_ucli.h
pipe_mgr_meter_tbl_mgr.c
pipe_mgr_meter_tbl.h
pipe_mgr_meter_tbl_init.c
pipe_mgr_meter_tbl_init.h
pipe_mgr_meter_tbl.h
pipe_mgr_meter_mgr_int.h
pipe_mgr_meter_drv_workflows.c
pipe_mgr_meter_drv_workflows.h
pipe_mgr_meter_ucli.c
pipe_mgr_meter_ucli.h
pipe_mgr_stful_tbl_mgr.c
pipe_mgr_stful_tbl_mgr.h
pipe_mgr_stful_ucli.c
pipe_mgr_stful_ucli.h
pipe_mgr_tbl.c
pipe_mgr_tbl.h
pipe_mgr_tcam.c
pipe_mgr_tcam.h
pipe_mgr_tcam_hw.c
pipe_mgr_tcam_hw.h
pipe_mgr_tcam_tbl_ucli.c
pipe_mgr_tcam_transaction.c
pipe_mgr_tcam_transaction.h
pipe_mgr_tcam_hlp_ha.c
pipe_mgr_tcam_llp_ha.c
pipe_mgr_tind.c
pipe_mgr_tind.h
pipe_mgr_tof_deprsr.c
pipe_mgr_tof_deprsr.h
pipe_mgr_tof_ebuf.c
pipe_mgr_tof_ebuf.h
pipe_mgr_tof_ibuf.c
pipe_mgr_tof_ibuf.h
pipe_mgr_tof_mau_snapshot.c
pipe_mgr_tof_mau_snapshot.h
pipe_mgr_tof_parb.c
pipe_mgr_tof_parb.h
pipe_mgr_tof_prsr.c
pipe_mgr_tof_prsr.h
pipe_mgr_tofino_cfg.c
pipe_mgr_tofino_cfg.h
pipe_mgr_ucli.c
pipe_mgr_pktgen_comm.h
pipe_mgr_pktgen_comm.c
pipe_mgr_tof_pktgen.h
pipe_mgr_tof_pktgen.c
pipe_mgr_tof2_pktgen.h
pipe_mgr_tof2_pktgen.c
pipe_mgr_tof3_pktgen.h
pipe_mgr_tof3_pktgen.c
pipe_mgr_pktgen.h
pipe_mgr_pktgen.c
pipe_mgr_phy_mem_map.h
pipe_mgr_phy_mem_map.c
pipe_mgr_mau_tbl_dbg_counters.h
pipe_mgr_mau_tbl_dbg_counters.c
pipe_mgr_mau_snapshot.h
pipe_mgr_mau_snapshot.c
pipe_mgr_tof_db.h
pipe_mgr_tof_db.c
pipe_mgr_tof2_db.h
pipe_mgr_tof2_db.c
pipe_mgr_tof3_db.h
pipe_mgr_tof3_db.c
pipe_mgr_db.h
pipe_mgr_db.c
pipe_mgr_interrupt_comm.h
pipe_mgr_interrupt_comm.c
pipe_mgr_tof_interrupt.c
pipe_mgr_tof_interrupt.h
pipe_mgr_tof2_interrupt.h
pipe_mgr_tof2_interrupt.c
pipe_mgr_tof3_interrupt.h
pipe_mgr_tof3_interrupt.c
pipe_mgr_tof2_deprsr_interrupt.c
pipe_mgr_tof2_lfltr_interrupt.c
pipe_mgr_tof2_mau_interrupt.c
pipe_mgr_tof2_mirror_interrupt.c
pipe_mgr_tof2_parde_interrupt.c
pipe_mgr_tof2_parser_interrupt.c
pipe_mgr_tof2_pgr_interrupt.c
pipe_mgr_tof2_sbc_interrupt.c
pipe_mgr_tof2_tm_interrupt.c
pipe_mgr_interrupt.h
pipe_mgr_interrupt.c
bf_packetpath_counter.c
pipe_mgr_tof2_pkt_path_counter_display.c
pipe_mgr_tof2_pkt_path_counter_display.h
pipe_mgr_tof3_pkt_path_counter_display.c
pipe_mgr_tof3_pkt_path_counter_display.h
pipe_mgr_tof_pkt_path_counter_display.c
pipe_mgr_tof_pkt_path_counter_display.h
pipe_mgr_tof_counters.c
pipe_mgr_table_packing.h
pipe_mgr_hash_compute.c
pipe_mgr_hash_compute_json.h
pipe_mgr_entry_format_json.h
pipe_mgr_entry_format.c
pipe_mgr_p4parser.c
pipe_mgr_hitless_ha.c
pipe_mgr_ctx_json_hash.c
pipe_mgr_ctx_json_entry_format.c
pipe_mgr_ctx_json_rmt_cfg.c
pipe_mgr_ctx_json_parser.c
pipe_mgr_ctx_json_dkm.c
pipe_mgr_dkm_json.h
pipe_mgr_dkm.h
pipe_mgr_tof_dkm.c
pipe_mgr_tof_dkm.h
pipe_mgr_tof2_dkm.c
pipe_mgr_tof2_dkm.h
pipe_mgr_tof3_dkm.c
pipe_mgr_tof3_dkm.h
pipe_mgr_dkm.c
pkt_path_ucli.c
)
set_source_files_properties(pipe_mgr_entry_format.c PROPERTIES COMPILE_FLAGS -Wno-stringop-truncation)

add_library(bfpipe_mgr SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:bfpipe_mgr_o>)
target_link_libraries(bfpipe_mgr lld ctx_json target_sys target_utils bfutils)

if (NOT STANDALONE)
add_library(bfshell_plugin_pipemgr_o OBJECT pipe_mgr_cli.c)
add_library(bfshell_plugin_pipemgr SHARED $<TARGET_OBJECTS:bfshell_plugin_pipemgr_o>)
target_link_libraries(bfshell_plugin_pipemgr clish)
SET_TARGET_PROPERTIES(bfshell_plugin_pipemgr PROPERTIES PREFIX "")
install(FILES xml/pipemgr.xml DESTINATION share/cli/xml)
endif()

# Building pipe-mgr doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${doxyfile_in} ${doxyfile} @ONLY)
  add_custom_target(doc-driver-pipe-mgr
                    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating Pipe Manager Doxygen"
                    VERBATIM)
  set_target_properties(doc-driver-pipe-mgr PROPERTIES EXCLUDE_FROM_ALL TRUE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION
    share/doc/bf-drivers/pipe_mgr OPTIONAL)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex DESTINATION
    share/doc/bf-drivers/pipe_mgr OPTIONAL)
else()
  message(WARNING "Doxygen is needed to build the documentation. Please install doxygen.")
endif()
