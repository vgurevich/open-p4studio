#include "gen-cpp/devport_mgr.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_devport_mgr.h>
#include <dvm/bf_drv_intf.h>
}

using namespace ::devport_mgr_pd_rpc;

class devport_mgrHandler : virtual public devport_mgrIf {
 public:
  devport_mgrHandler() {}

  void devport_mgr_add_port(const int32_t dev_id,
                            const int32_t port_id,
                            const int32_t port_speeds,
                            const int32_t port_fec_types) {
    int status =
        p4_devport_mgr_add_port(dev_id, port_id, port_speeds, port_fec_types);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_add_port_with_lanes(const int32_t dev_id,
                            const int32_t port_id,
                            const int32_t port_speeds,
                            const int32_t port_lanes,
                            const int32_t port_fec_types) {
    int status =
        p4_devport_mgr_add_port_with_lanes(dev_id, port_id, port_speeds, port_lanes, port_fec_types);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_remove_port(const int32_t dev_id, const int32_t port_id) {
    int status = p4_devport_mgr_remove_port(dev_id, port_id);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_enable_port(const int32_t dev_id,
                               const int32_t port_id,
                               const bool enable) {
    int status = p4_devport_mgr_enable_port(dev_id, port_id, enable);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int devport_mgr_port_ca_get(const int32_t dev_id, const int32_t port_id) {
    int ca = p4_devport_mgr_port_ca_get(dev_id, port_id);
    if (ca < 0) {
      InvalidDevportMgrOperation iop;
      iop.code = 2;  // Invalid parameter
      throw iop;
    }
    return ca;
  }

  int devport_mgr_serdes_ca_get(const int32_t dev_id, const int32_t port_id) {
    int ca = p4_devport_mgr_serdes_ca_get(dev_id, port_id);
    if (ca < 0) {
      InvalidDevportMgrOperation iop;
      iop.code = 2;  // Invalid parameter
      throw iop;
    }
    return ca;
  }

  void devport_mgr_warm_init_begin(
      const int32_t dev_id,
      const dev_init_mode::type warm_init_mode,
      const dev_serdes_upgrade_mode::type serdes_upgrade_mode,
      bool upgrade_agents) {
    int status = p4_devport_mgr_warm_init_begin(
        dev_id,
        (p4_devport_mgr_warm_init_mode_e)warm_init_mode,
        (p4_devport_mgr_serdes_upgrade_mode_e)serdes_upgrade_mode,
        upgrade_agents);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_warm_init_end(const int32_t dev_id) {
    int status = p4_devport_mgr_warm_init_end(dev_id);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_set_copy_to_cpu(const int dev,
                                   const int8_t enable,
                                   const int16_t port) {
    int status = p4_devport_mgr_set_copy_to_cpu(dev, enable, port);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int devport_mgr_pcie_cpu_port_get(const int dev) {
    int port = p4_devport_mgr_pcie_cpu_port_get(dev);
    if (port < 0) {
      InvalidDevportMgrOperation iop;
      iop.code = 2;  // Invalid parameter
      throw iop;
    }
    return port;
  }

  int devport_mgr_eth_cpu_port_get(const int dev) {
    int port = p4_devport_mgr_eth_cpu_port_get(dev);
    if (port < 0) {
      InvalidDevportMgrOperation iop;
      iop.code = 2;  // Invalid parameter
      throw iop;
    }
    return port;
  }

  int devport_mgr_get_parser_id(const int dev, const int port) {
    uint8_t parserid;
    int status = p4_pd_parser_id_get(dev, port, &parserid);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return parserid;
  }

  int devport_mgr_get_pipe_id(const int dev, const int port) {
    bf_dev_pipe_t pipe_id;
    int status = p4_pd_pipe_id_get(dev, port, &pipe_id);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return pipe_id;
  }

  void devport_mgr_set_virtual_dev_slave_mode(const int32_t dev_id) {
    int status = p4_devport_mgr_set_virtual_dev_slave_mode(dev_id);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_get_clock_speed(clock_speed &clock, const int32_t dev_id) {
    int status =
        p4_devport_mgr_get_clock_speed(dev_id,
                                       (uint64_t *)&clock.bps_clock_speed,
                                       (uint64_t *)&clock.pps_clock_speed);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_lrt_dr_timeout_set(const int32_t dev_id, const int32_t timeout_ms) {
    int status = p4_devport_mgr_lrt_dr_timeout_set(dev_id, timeout_ms);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int devport_mgr_lrt_dr_timeout_get(const int32_t dev_id) {
    uint32_t timeout_ms = 0;
    int status = p4_devport_mgr_lrt_dr_timeout_get(dev_id, &timeout_ms);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return timeout_ms;
  }

  std::vector<error_event_record> error_event_list;

  // Error event callback function.
  static bf_status_t error_event_cb(bf_error_sev_level_t severity,
                                    bf_dev_id_t dev_id,
                                    bf_dev_pipe_t pipe,
                                    uint8_t stage,
                                    uint64_t address,
                                    bf_error_type_t err_type,
                                    bf_error_block_t block,
                                    bf_error_block_location_t loc,
                                    const char *obj_name,
                                    const bf_dev_port_t *port_list,
                                    int num_ports,
                                    const char *err_string,
                                    void *cookie) {
    ((devport_mgrHandler *)cookie)
        ->error_event_receive(severity,
                              dev_id,
                              pipe,
                              stage,
                              address,
                              err_type,
                              block,
                              loc,
                              obj_name,
                              port_list,
                              num_ports,
                              err_string);
    return 0;
  }

  // Error event callback method.
  bf_status_t error_event_receive(bf_error_sev_level_t severity,
                                  bf_dev_id_t dev_id,
                                  bf_dev_pipe_t pipe,
                                  uint8_t stage,
                                  uint64_t address,
                                  bf_error_type_t err_type,
                                  bf_error_block_t block,
                                  bf_error_block_location_t loc,
                                  const char *obj_name,
                                  const bf_dev_port_t *port_list,
                                  int num_ports,
                                  const char *err_string) {
    (void)port_list;
    (void)num_ports;
    error_event_record e;
    switch (severity) {
      case BF_ERR_SEV_CORRECTABLE:
        e.sev = bf_error_sev_level::BF_ERR_SEV_CORRECTABLE;
        break;
      case BF_ERR_SEV_NON_CORRECTABLE:
        e.sev = bf_error_sev_level::BF_ERR_SEV_NON_CORRECTABLE;
        break;
      case BF_ERR_SEV_FATAL:
        e.sev = bf_error_sev_level::BF_ERR_SEV_FATAL;
        break;
    }
    e.dev_id = dev_id;
    e.pipe = pipe;
    e.stage = stage;
    e.address = address;

    switch (err_type) {
      case BF_ERR_TYPE_GENERIC:
        e.err_type = bf_error_type::BF_ERR_TYPE_GENERIC;
        break;
      case BF_ERR_TYPE_SINGLE_BIT_ECC:
        e.err_type = bf_error_type::BF_ERR_TYPE_SINGLE_BIT_ECC;
        break;
      case BF_ERR_TYPE_MULTI_BIT_ECC:
        e.err_type = bf_error_type::BF_ERR_TYPE_MULTI_BIT_ECC;
        break;
      case BF_ERR_TYPE_PARITY:
        e.err_type = bf_error_type::BF_ERR_TYPE_PARITY;
        break;
      case BF_ERR_TYPE_OVERFLOW:
        e.err_type = bf_error_type::BF_ERR_TYPE_OVERFLOW;
        break;
      case BF_ERR_TYPE_UNDERFLOW:
        e.err_type = bf_error_type::BF_ERR_TYPE_UNDERFLOW;
        break;
      case BF_ERR_TYPE_PKT_DROP:
        e.err_type = bf_error_type::BF_ERR_TYPE_PKT_DROP;
        break;
    }

    switch (block) {
      case BF_ERR_BLK_NONE:
        e.err_blk = bf_error_block::BF_ERR_BLK_NONE;
        break;
      case BF_ERR_BLK_MAU:
        e.err_blk = bf_error_block::BF_ERR_BLK_MAU;
        break;
      case BF_ERR_BLK_TCAM:
        e.err_blk = bf_error_block::BF_ERR_BLK_TCAM;
        break;
      case BF_ERR_BLK_SRAM:
        e.err_blk = bf_error_block::BF_ERR_BLK_SRAM;
        break;
      case BF_ERR_BLK_MAP_RAM:
        e.err_blk = bf_error_block::BF_ERR_BLK_MAP_RAM;
        break;
      case BF_ERR_BLK_STATS:
        e.err_blk = bf_error_block::BF_ERR_BLK_STATS;
        break;
      case BF_ERR_BLK_METERS:
        e.err_blk = bf_error_block::BF_ERR_BLK_METERS;
        break;
      case BF_ERR_BLK_SYNTH2PORT:
        e.err_blk = bf_error_block::BF_ERR_BLK_SYNTH2PORT;
        break;
      case BF_ERR_BLK_SELECTOR_ALU:
        e.err_blk = bf_error_block::BF_ERR_BLK_SELECTOR_ALU;
        break;
      case BF_ERR_BLK_IMEM:
        e.err_blk = bf_error_block::BF_ERR_BLK_IMEM;
        break;
      case BF_ERR_BLK_MIRROR:
        e.err_blk = bf_error_block::BF_ERR_BLK_MIRROR;
        break;
      case BF_ERR_BLK_TM_PRE:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_PRE;
        break;
      case BF_ERR_BLK_TM_WAC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_WAC;
        break;
      case BF_ERR_BLK_TM_QAC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_QAC;
        break;
      case BF_ERR_BLK_TM_CLC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_CLC;
        break;
      case BF_ERR_BLK_TM_PEX:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_PEX;
        break;
      case BF_ERR_BLK_TM_QLC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_QLC;
        break;
      case BF_ERR_BLK_TM_PRC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_PRC;
        break;
      case BF_ERR_BLK_TM_PSC:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_PSC;
        break;
      case BF_ERR_BLK_TM_CAA:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_CAA;
        break;
      case BF_ERR_BLK_TM_SCH:
        e.err_blk = bf_error_block::BF_ERR_BLK_TM_SCH;
        break;
      case BF_ERR_BLK_PRSR:
        e.err_blk = bf_error_block::BF_ERR_BLK_PRSR;
        break;
      case BF_ERR_BLK_DEPRSR:
        e.err_blk = bf_error_block::BF_ERR_BLK_DEPRSR;
        break;
      case BF_ERR_BLK_PKTGEN:
        e.err_blk = bf_error_block::BF_ERR_BLK_PKTGEN;
        break;
      case BF_ERR_BLK_GFM:
        e.err_blk = bf_error_block::BF_ERR_BLK_GFM;
        break;
      case BF_ERR_BLK_DMA:
        e.err_blk = bf_error_block::BF_ERR_BLK_DMA;
        break;
      case BF_ERR_BLK_LFLTR:
        e.err_blk = bf_error_block::BF_ERR_BLK_LFLTR;
        break;
      case BF_ERR_BLK_EBUF:
        e.err_blk = bf_error_block::BF_ERR_BLK_EBUF;
        break;
    }

    switch (loc) {
      case BF_ERR_LOC_NONE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_NONE;
        break;
      case BF_ERR_LOC_MAU_IDLETIME:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MAU_IDLETIME;
        break;
      case BF_ERR_LOC_MAU_STATEFUL_LOG:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MAU_STATEFUL_LOG;
        break;
      case BF_ERR_LOC_SELECTOR_ALU_ST_MINMAX:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_SELECTOR_ALU_ST_MINMAX;
        break;
      case BF_ERR_LOC_SELECTOR_ALU_DIV_BY0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_SELECTOR_ALU_DIV_BY0;
        break;
      case BF_ERR_LOC_SELECTOR_ALU_SALU_PRED:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_SELECTOR_ALU_SALU_PRED;
        break;
      case BF_ERR_LOC_MIRR_POINTER_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_POINTER_FIFO;
        break;
      case BF_ERR_LOC_MIRR_IG:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_IG;
        break;
      case BF_ERR_LOC_MIRR_EG:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_EG;
        break;
      case BF_ERR_LOC_MIRR_OUT_DESC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_OUT_DESC;
        break;
      case BF_ERR_LOC_MIRR_DATA_BUFFER:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_DATA_BUFFER;
        break;
      case BF_ERR_LOC_MIRR_DROP_NEG:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_DROP_NEG;
        break;
      case BF_ERR_LOC_MIRR_DROP_COAL:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_DROP_COAL;
        break;
      case BF_ERR_LOC_MIRR_IG_DIS_SESS:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_IG_DIS_SESS;
        break;
      case BF_ERR_LOC_MIRR_EG_DIS_SESS:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_EG_DIS_SESS;
        break;
      case BF_ERR_LOC_MIRR_OUT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_OUT;
        break;
      case BF_ERR_LOC_MIRR_CRC12:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_CRC12;
        break;
      case BF_ERR_LOC_MIRR_SESSION:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_SESSION;
        break;
      case BF_ERR_LOC_MIRR_S2P_CREDIT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_S2P_CREDIT;
        break;
      case BF_ERR_LOC_MIRR_IDPRSR_SOPEOP_MISMATCH:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_IDPRSR_SOPEOP_MISMATCH;
        break;
      case BF_ERR_LOC_MIRR_EDPRSR_SOPEOP_MISMATCH:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_EDPRSR_SOPEOP_MISMATCH;
        break;
      case BF_ERR_LOC_MIRR_DATA_MEM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_MIRR_DATA_MEM;
        break;
      case BF_ERR_LOC_TM_PRE_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_FIFO;
        break;
      case BF_ERR_LOC_TM_PRE_MIT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_MIT;
        break;
      case BF_ERR_LOC_TM_PRE_LIT0_BM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_LIT0_BM;
        break;
      case BF_ERR_LOC_TM_PRE_LIT1_BM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_LIT1_BM;
        break;
      case BF_ERR_LOC_TM_PRE_LIT0_NP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_LIT0_NP;
        break;
      case BF_ERR_LOC_TM_PRE_LIT1_NP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_LIT1_NP;
        break;
      case BF_ERR_LOC_TM_PRE_PMT0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_PMT0;
        break;
      case BF_ERR_LOC_TM_PRE_PMT1:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_PMT1;
        break;
      case BF_ERR_LOC_TM_PRE_RDM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_RDM;
        break;
      case BF_ERR_LOC_TM_PRE_BANKID_MEM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRE_BANKID_MEM;
        break;
      case BF_ERR_LOC_TM_WAC_PPG_MAP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_PPG_MAP;
        break;
      case BF_ERR_LOC_TM_WAC_DROP_CNT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_DROP_CNT;
        break;
      case BF_ERR_LOC_TM_WAC_PFC_VIS:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_PFC_VIS;
        break;
      case BF_ERR_LOC_TM_WAC_SCH_FCR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_SCH_FCR;
        break;
      case BF_ERR_LOC_TM_WAC_QID_MAP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_QID_MAP;
        break;
      case BF_ERR_LOC_TM_WAC_WAC2QAC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_WAC_WAC2QAC;
        break;
      case BF_ERR_LOC_TM_QAC_QUE_DROP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QAC_QUE_DROP;
        break;
      case BF_ERR_LOC_TM_QAC_PORT_DROP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QAC_PORT_DROP;
        break;
      case BF_ERR_LOC_TM_QAC_QID_MAP:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QAC_QID_MAP;
        break;
      case BF_ERR_LOC_TM_QAC_QAC2PRC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QAC_QAC2PRC;
        break;
      case BF_ERR_LOC_TM_QAC_PRC2PSC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QAC_PRC2PSC;
        break;
      case BF_ERR_LOC_TM_CLC_ENQ_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_CLC_ENQ_FIFO;
        break;
      case BF_ERR_LOC_TM_CLC_QAC_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_CLC_QAC_FIFO;
        break;
      case BF_ERR_LOC_TM_CLC_PH_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_CLC_PH_FIFO;
        break;
      case BF_ERR_LOC_TM_CLC_QAC_PH_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_CLC_QAC_PH_FIFO;
        break;
      case BF_ERR_LOC_TM_PEX_CLM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PEX_CLM;
        break;
      case BF_ERR_LOC_TM_PEX_PH_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PEX_PH_FIFO;
        break;
      case BF_ERR_LOC_TM_PEX_META_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PEX_META_FIFO;
        break;
      case BF_ERR_LOC_TM_PEX_PH_AFIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PEX_PH_AFIFO;
        break;
      case BF_ERR_LOC_TM_PEX_DISCARD_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PEX_DISCARD_FIFO;
        break;
      case BF_ERR_LOC_TM_QLC_QLM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QLC_QLM;
        break;
      case BF_ERR_LOC_TM_QLC_SCHDEQ:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QLC_SCHDEQ;
        break;
      case BF_ERR_LOC_TM_QLC_PH_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_QLC_PH_FIFO;
        break;
      case BF_ERR_LOC_TM_PRC_T3:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PRC_T3;
        break;
      case BF_ERR_LOC_TM_PSC_PSM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PSC_PSM;
        break;
      case BF_ERR_LOC_TM_PSC_COMM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_PSC_COMM;
        break;
      case BF_ERR_LOC_TM_CAA:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_CAA;
        break;
      case BF_ERR_LOC_TM_SCH_TDM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_TDM;
        break;
      case BF_ERR_LOC_TM_SCH_UPD_WAC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_UPD_WAC;
        break;
      case BF_ERR_LOC_TM_SCH_UPD_EDPRSR_ADVFC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_UPD_EDPRSR_ADVFC;
        break;
      case BF_ERR_LOC_TM_SCH_Q_MINRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_Q_MINRATE;
        break;
      case BF_ERR_LOC_TM_SCH_Q_EXCRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_Q_EXCRATE;
        break;
      case BF_ERR_LOC_TM_SCH_Q_MAXRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_Q_MAXRATE;
        break;
      case BF_ERR_LOC_TM_SCH_L1_MINRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_L1_MINRATE;
        break;
      case BF_ERR_LOC_TM_SCH_L1_EXCRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_L1_EXCRATE;
        break;
      case BF_ERR_LOC_TM_SCH_L1_MAXRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_L1_MAXRATE;
        break;
      case BF_ERR_LOC_TM_SCH_P_MAXRATE:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_P_MAXRATE;
        break;
      case BF_ERR_LOC_TM_SCH_UPD_PEX:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_UPD_PEX;
        break;
      case BF_ERR_LOC_TM_SCH_UPD_EDPRSR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_UPD_EDPRSR;
        break;
      case BF_ERR_LOC_TM_SCH_PEX_CREDIT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_PEX_CREDIT;
        break;
      case BF_ERR_LOC_TM_SCH_PEX_MAC_CREDIT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_PEX_MAC_CREDIT;
        break;
      case BF_ERR_LOC_TM_SCH_Q_WATCHDOG:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_TM_SCH_Q_WATCHDOG;
        break;
      case BF_ERR_LOC_PRSR_ACT_RAM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PRSR_ACT_RAM;
        break;
      case BF_ERR_LOC_PRSR_INP_BUFF:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PRSR_INP_BUFF;
        break;
      case BF_ERR_LOC_PRSR_OUT_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PRSR_OUT_FIFO;
        break;
      case BF_ERR_LOC_PRSR_TCAM_PARITY:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PRSR_TCAM_PARITY;
        break;
      case BF_ERR_LOC_PRSR_CSUM:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PRSR_CSUM;
        break;
      case BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL0;
        break;
      case BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL1:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_PIPE_VEC_TBL1;
        break;
      case BF_ERR_LOC_DEPRSR_MIRRTBL:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_MIRRTBL;
        break;
      case BF_ERR_LOC_DEPRSR_IPKT_MAC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_IPKT_MAC;
        break;
      case BF_ERR_LOC_DEPRSR_CMD_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_CMD_FIFO;
        break;
      case BF_ERR_LOC_DEPRSR_CRED_ERR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_CRED_ERR;
        break;
      case BF_ERR_LOC_DEPRSR_PKTST:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_PKTST;
        break;
      case BF_ERR_LOC_DEPRSR_META_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_META_FIFO;
        break;
      case BF_ERR_LOC_DEPRSR_PKTHDR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_PKTHDR;
        break;
      case BF_ERR_LOC_DEPRSR_MIRRHDR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_MIRRHDR;
        break;
      case BF_ERR_LOC_DEPRSR_DATAST:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_DATAST;
        break;
      case BF_ERR_LOC_DEPRSR_PKTDATA:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_PKTDATA;
        break;
      case BF_ERR_LOC_DEPRSR_TMSCH:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_TMSCH;
        break;
      case BF_ERR_LOC_DEPRSR_ARB_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_ARB_FIFO;
        break;
      case BF_ERR_LOC_DEPRSR_CTL_CHAN:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DEPRSR_CTL_CHAN;
        break;
      case BF_ERR_LOC_PKTGEN_BUFFER:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_BUFFER;
        break;
      case BF_ERR_LOC_PKTGEN_PFC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_PFC;
        break;
      case BF_ERR_LOC_PKTGEN_TBC_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_TBC_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_ETH_CPU_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_EBUF_P0_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_EBUF_P1_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_EBUF_P2_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_EBUF_P3_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_APP_EVT:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_APP_EVT;
        break;
      case BF_ERR_LOC_PKTGEN_IPB_CHNL_SEQ:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_IPB_CHNL_SEQ;
        break;
      case BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_ETH_CPU_TBC_SAMECHNL;
        break;
      case BF_ERR_LOC_PKTGEN_ETH_PORT_FIFO:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_ETH_PORT_FIFO;
        break;
      case BF_ERR_LOC_PKTGEN_PHASE0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_PKTGEN_PHASE0;
        break;
      case BF_ERR_LOC_GFM_INGRESS:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_GFM_INGRESS;
        break;
      case BF_ERR_LOC_GFM_EGRESS:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_GFM_EGRESS;
        break;
      case BF_ERR_LOC_DMA_PBC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DMA_PBC;
        break;
      case BF_ERR_LOC_DMA_CBC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DMA_CBC;
        break;
      case BF_ERR_LOC_DMA_MBC:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_DMA_MBC;
        break;
      case BF_ERR_LOC_LFLTR_BFT_CLR:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_BFT_CLR;
        break;
      case BF_ERR_LOC_LFLTR_BFT0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_BFT0;
        break;
      case BF_ERR_LOC_LFLTR_BFT1:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_BFT1;
        break;
      case BF_ERR_LOC_LFLTR_LQT0:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_LQT0;
        break;
      case BF_ERR_LOC_LFLTR_LQT1:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_LQT1;
        break;
      case BF_ERR_LOC_LFLTR_LBUF:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_LFLTR_LBUF;
        break;
      case BF_ERR_LOC_EBUF:
        e.err_loc = bf_error_block_location::BF_ERR_LOC_EBUF;
        break;
    }
    e.err_string = std::string(err_string);
    e.obj_name = std::string((obj_name) ? obj_name : "");
    error_event_list.push_back(e);
    return 0;
  }

  void devport_mgr_register_for_error_events(const int32_t dev_id) {
    (void)dev_id;
    int status = bf_register_error_events(dev_id, &error_event_cb, this);
    if (status != 0) {
      InvalidDevportMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void devport_mgr_get_error_events(std::vector<error_event_record> &_return,
                                    const int32_t dev_id) {
    (void)dev_id;
    _return.swap(error_event_list);
  }
};
