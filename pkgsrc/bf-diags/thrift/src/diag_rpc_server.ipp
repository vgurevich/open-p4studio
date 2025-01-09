
#include <iostream>

#include "diag_rpc.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
extern "C" {
#include "diags/bf_diag_api.h"
}

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace ::diag_rpc;

class diag_rpcHandler : virtual public diag_rpcIf {
 public:
  diag_rpcHandler() {}

  diag_sess_hdl_t diag_loopback_test_setup(
      const diag_device_t device,
      const std::vector<int32_t> &port_list,
      const int32_t num_ports,
      const int32_t loop_mode) {
    bf_diag_sess_hdl_t sess_hdl = 0;
    int status =
        bf_diag_loopback_test_setup(device,
                                    (bf_dev_port_t *)(&port_list[0]),
                                    num_ports,
                                    (bf_diag_port_lpbk_mode_e)loop_mode,
                                    &sess_hdl);
    return sess_hdl;
  }

  int32_t diag_loopback_test_start(const int32_t num_pkt,
                                   const int32_t pkt_size,
                                   const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_test_start(num_pkt, pkt_size, sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_test_abort(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_test_abort(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_test_status_get(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_test_status_get(sess_hdl);
    return status;
  }

  void diag_loopback_test_port_status_get(diag_port_status_t &rtn_status,
                                          const diag_sess_hdl_t sess_hdl,
                                          const diag_port_t port) {
    bf_diag_port_stats_t stats;

    memset(&stats, 0, sizeof(stats));
    int status = bf_diag_loopback_test_port_status_get(sess_hdl, port, &stats);
    rtn_status.status = status;
    rtn_status.tx_total = stats.tx_total;
    rtn_status.rx_total = stats.rx_total;
    rtn_status.rx_good = stats.rx_good;
    rtn_status.rx_bad = stats.rx_bad;
  }

  int32_t diag_loopback_test_cleanup(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_test_cleanup(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  diag_sess_hdl_t diag_loopback_snake_test_setup(
      const diag_device_t device,
      const std::vector<int32_t> &port_list,
      const int32_t num_ports,
      const int32_t loop_mode) {
    bf_diag_sess_hdl_t sess_hdl = 0;
    int status =
        bf_diag_loopback_snake_test_setup(device,
                                          (bf_dev_port_t *)(&port_list[0]),
                                          num_ports,
                                          (bf_diag_port_lpbk_mode_e)loop_mode,
                                          &sess_hdl);
    return sess_hdl;
  }

  int32_t diag_loopback_snake_test_start(const diag_sess_hdl_t sess_hdl,
                                         const int32_t num_pkt,
                                         const int32_t pkt_size,
                                         const bool bidir) {
    int status =
        bf_diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, bidir);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_snake_test_stop(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_snake_test_stop(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_snake_test_status_get(const diag_sess_hdl_t sess_hdl) {
    bf_diag_port_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    int status = bf_diag_loopback_snake_test_status_get(sess_hdl, &stats);
    return status;
  }

  int32_t diag_loopback_snake_test_cleanup(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_snake_test_cleanup(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  diag_sess_hdl_t diag_loopback_pair_test_setup(
      const diag_device_t device,
      const std::vector<int32_t> &port_list,
      const int32_t num_ports,
      const int32_t loop_mode) {
    bf_diag_sess_hdl_t sess_hdl = 0;
    int status =
        bf_diag_loopback_pair_test_setup(device,
                                         (bf_dev_port_t *)(&port_list[0]),
                                         num_ports,
                                         (bf_diag_port_lpbk_mode_e)loop_mode,
                                         &sess_hdl);
    return sess_hdl;
  }
  int32_t diag_loopback_pair_test_start(const diag_sess_hdl_t sess_hdl,
                                        const int32_t num_pkt,
                                        const int32_t pkt_size,
                                        const bool bidir) {
    int status =
        bf_diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, bidir);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_pair_test_stop(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_pair_test_stop(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_loopback_pair_test_status_get(const diag_sess_hdl_t sess_hdl) {
    bf_diag_loopback_pair_test_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    int status = bf_diag_loopback_pair_test_status_get(sess_hdl, &stats);
    return status;
  }

  int32_t diag_loopback_pair_test_cleanup(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_loopback_pair_test_cleanup(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  bool diag_session_valid(const diag_sess_hdl_t sess_hdl) {
    bool valid = bf_diag_session_valid(sess_hdl);
    return valid;
  }

  int32_t diag_port_loopback_mode_set(const diag_device_t device,
                                      const diag_port_t port,
                                      const int32_t loop_mode) {
    int status = bf_diag_port_loopback_mode_set(
        device, (bf_dev_port_t)port, (bf_diag_port_lpbk_mode_e)loop_mode);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_loopback_mode_get(const diag_device_t device,
                                      const diag_port_t port) {
    bf_diag_port_lpbk_mode_e loop_mode;
    int status =
        bf_diag_port_loopback_mode_get(device, (bf_dev_port_t)port, &loop_mode);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return loop_mode;
  }

  diag_entry_hdl_t diag_forwarding_rule_add(const diag_device_t device,
                                            const diag_port_t ig_port,
                                            const diag_port_t eg_port,
                                            const int32_t tcp_dstPort_start,
                                            const int32_t tcp_dstPort_end,
                                            const int32_t priority) {
    p4_pd_entry_hdl_t entry_hdl = 0;
    int status = bf_diag_forwarding_rule_add(device,
                                             (bf_dev_port_t)ig_port,
                                             (bf_dev_port_t)eg_port,
                                             tcp_dstPort_start,
                                             tcp_dstPort_end,
                                             priority,
                                             &entry_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return entry_hdl;
  }

  int32_t diag_forwarding_rule_del(const diag_device_t device,
                                   const diag_entry_hdl_t entry_hdl) {
    int status = bf_diag_forwarding_rule_del(device, entry_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_mac_aging_set(const diag_device_t device, const int32_t ttl) {
    int status = bf_diag_mac_aging_set(device, ttl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_mac_aging_reset(const diag_device_t device) {
    int status = bf_diag_mac_aging_reset(device);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_mac_aging_get(const diag_device_t device) {
    uint32_t ttl = 0;
    int status = bf_diag_mac_aging_get(device, &ttl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return ttl;
  }

  int32_t diag_learning_timeout_set(const diag_device_t device,
                                    const int32_t timeout) {
    int status = bf_diag_learning_timeout_set(device, timeout);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_default_vlan_set(const diag_device_t device,
                                     const diag_port_t port,
                                     const diag_vlan_t vlan) {
    int status = bf_diag_port_default_vlan_set(device, port, vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_default_vlan_reset(const diag_device_t device,
                                       const diag_port_t port) {
    int status = bf_diag_port_default_vlan_reset(device, port);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_default_vlan_get(const diag_device_t device,
                                     const diag_port_t port) {
    int32_t vlan = 0;
    int status = bf_diag_port_default_vlan_get(device, port, &vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return vlan;
  }

  int32_t diag_vlan_create(const diag_device_t device, const diag_vlan_t vlan) {
    int status = bf_diag_vlan_create(device, vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_vlan_destroy(const diag_device_t device,
                            const diag_vlan_t vlan) {
    int status = bf_diag_vlan_destroy(device, vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_vlan_add(const diag_device_t device,
                             const diag_port_t port,
                             const diag_vlan_t vlan) {
    int status = bf_diag_port_vlan_add(device, port, vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_port_vlan_del(const diag_device_t device,
                             const diag_port_t port,
                             const diag_vlan_t vlan) {
    int status = bf_diag_port_vlan_del(device, port, vlan);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_packet_inject_from_cpu(const diag_device_t device,
                                      const std::vector<int32_t> &port_list,
                                      const int32_t num_ports,
                                      const int32_t num_pkt,
                                      const int32_t pkt_size) {
    int status = bf_diag_packet_inject_from_cpu(
        device, (bf_dev_port_t *)(&port_list[0]), num_ports, num_pkt, pkt_size);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  diag_port_t diag_cpu_port_get(const diag_device_t device) {
    diag_port_t cpu_port = 0;
    int status = bf_diag_cpu_port_get(device, &cpu_port);

    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return cpu_port;
  }

  void diag_cpu_stats_get(diag_port_status_t &rtn_status,
                          const diag_device_t device,
                          const diag_port_t port) {
    bf_diag_port_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    int status = bf_diag_cpu_stats_get(device, port, &stats);
    rtn_status.status = status;
    rtn_status.tx_total = stats.tx_total;
    rtn_status.rx_total = stats.rx_total;
    rtn_status.rx_good = stats.rx_good;
    rtn_status.rx_bad = stats.rx_bad;
  }

  int32_t diag_cpu_stats_clear(const diag_device_t device,
                               const diag_port_t port,
                               const int8_t all_ports) {
    int status = bf_diag_cpu_stats_clear(device, port, all_ports);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  diag_sess_hdl_t diag_multicast_loopback_test_setup(
      const diag_device_t device,
      const std::vector<int32_t> &port_list,
      const int32_t num_ports,
      const int32_t loop_mode) {
    bf_diag_sess_hdl_t sess_hdl = 0;
    int status = bf_diag_multicast_loopback_test_setup(
        device,
        (bf_dev_port_t *)(&port_list[0]),
        num_ports,
        (bf_diag_port_lpbk_mode_e)loop_mode,
        &sess_hdl);
    return sess_hdl;
  }

  int32_t diag_multicast_loopback_test_start(const diag_sess_hdl_t sess_hdl,
                                             const int32_t num_pkt,
                                             const int32_t pkt_size) {
    int status =
        bf_diag_multicast_loopback_test_start(sess_hdl, num_pkt, pkt_size);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_multicast_loopback_test_stop(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_multicast_loopback_test_stop(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_multicast_loopback_test_status_get(
      const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_multicast_loopback_test_status_get(sess_hdl);
    return status;
  }

  int32_t diag_multicast_loopback_test_cleanup(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_multicast_loopback_test_cleanup(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_data_pattern_set(const diag_sess_hdl_t sess_hdl,
                                const diag_data_pattern_t::type mode,
                                const int32_t start_pat,
                                const int32_t start_pat_len,
                                const int32_t pat_a,
                                const int32_t pat_b,
                                const int32_t pattern_len) {
    bf_diag_data_pattern_t c_mode = BF_DIAG_DATA_PATTERN_RANDOM;
    if (mode == diag_data_pattern_t::DIAG_DATA_PATTERN_RANDOM) {
      c_mode = BF_DIAG_DATA_PATTERN_RANDOM;
    } else if (mode == diag_data_pattern_t::DIAG_DATA_PATTERN_FIXED) {
      c_mode = BF_DIAG_DATA_PATTERN_FIXED;
    } else {
      InvalidDiagOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }
    int status = bf_diag_data_pattern_set(
        sess_hdl, c_mode, start_pat, start_pat_len, pat_a, pat_b, pattern_len);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_packet_payload_set(const diag_sess_hdl_t sess_hdl,
                                  const diag_packet_payload_t::type mode,
                                  const std::string &payload,
                                  const std::string &payload_file_path) {
    bf_diag_packet_payload_t c_mode = BF_DIAG_PACKET_PAYLOAD_RANDOM;
    if (mode == diag_packet_payload_t::DIAG_PACKET_PAYLOAD_RANDOM) {
      c_mode = BF_DIAG_PACKET_PAYLOAD_RANDOM;
    } else if (mode == diag_packet_payload_t::DIAG_PACKET_PAYLOAD_FIXED) {
      c_mode = BF_DIAG_PACKET_PAYLOAD_FIXED;
    } else if (mode == diag_packet_payload_t::DIAG_PACKET_PAYLOAD_RANDOM_FLIP) {
      c_mode = BF_DIAG_PACKET_PAYLOAD_RANDOM_FLIP;
    } else {
      InvalidDiagOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }
    int status = bf_diag_packet_payload_set(sess_hdl,
                                            c_mode,
                                            (char *)payload.c_str(),
                                            (char *)payload_file_path.c_str());
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_packet_full_set(const diag_sess_hdl_t sess_hdl,
                               const diag_packet_full_t::type mode,
                               const std::string &pkt,
                               const std::string &pkt_file_path) {
    bf_diag_packet_full_t c_mode = BF_DIAG_PACKET_FULL_RANDOM;
    if (mode == diag_packet_full_t::DIAG_PACKET_FULL_RANDOM) {
      c_mode = BF_DIAG_PACKET_FULL_RANDOM;
    } else if (mode == diag_packet_full_t::DIAG_PACKET_FULL_FIXED) {
      c_mode = BF_DIAG_PACKET_FULL_FIXED;
    } else {
      InvalidDiagOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }
    int status = bf_diag_packet_full_set(
        sess_hdl, c_mode, (char *)pkt.c_str(), (char *)pkt_file_path.c_str());
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_sessions_max_set(const diag_device_t device,
                                const int32_t max_sessions) {
    int status = bf_diag_sessions_max_set(device, max_sessions);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_min_packet_size_enable(const bool enable) {
    int status = bf_diag_min_packet_size_enable(enable);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  diag_sess_hdl_t diag_stream_setup(const diag_device_t device,
                                    const int32_t num_pkts,
                                    const int32_t pkt_size,
                                    const diag_port_t dst_port) {
    bf_diag_sess_hdl_t sess_hdl = 0;
    int status =
        bf_diag_stream_setup(device, num_pkts, pkt_size, dst_port, &sess_hdl);
    return sess_hdl;
  }

  int32_t diag_stream_start(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_stream_start(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_stream_adjust(const diag_sess_hdl_t sess_hdl,
                             const int32_t num_pkts,
                             const int32_t pkt_size) {
    int status = bf_diag_stream_adjust(sess_hdl, num_pkts, pkt_size);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int32_t diag_stream_stop(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_stream_stop(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }

  int64_t diag_stream_counter_get(const diag_sess_hdl_t sess_hdl) {
    uint64_t counter = 0;
    int status = bf_diag_stream_counter_get(sess_hdl, &counter);
    return counter;
  }

  int32_t diag_stream_cleanup(const diag_sess_hdl_t sess_hdl) {
    int status = bf_diag_stream_cleanup(sess_hdl);
    if (status != 0) {
      InvalidDiagOperation iop;
      iop.code = status;
      throw iop;
    }
    return 0;
  }
};
