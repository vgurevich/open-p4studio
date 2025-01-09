#include "gen-cpp/pkt.h"
#include "gen-cpp/pkt_pd_rpc_types.h"

extern "C" {
#include <tofino/pdfixed/pd_pkt.h>
}

using namespace ::pkt_pd_rpc;
using namespace ::res_pd_rpc;

static char pkt_pkt_buff[9100];  // holds the last received  packet buffer

class pktHandler : virtual public pktIf {
 public:
  pktHandler() {}

  void echo(const std::string &s) { std::cerr << "Echo: " << s << std::endl; }

  void init() { pkt_pd_init(); }

  void cleanup() { pkt_pd_cleanup(); }

  SessionHandle_t client_init() {
    p4_pd_sess_hdl_t sess_hdl;
    int status = pkt_pd_client_init(&sess_hdl);
    if (status != 0) {
      InvalidPktOperation iop;
      iop.code = status;
      throw iop;
    }
    return sess_hdl;
  }

  void client_cleanup(const SessionHandle_t sess_hdl) {
    int status = pkt_pd_client_cleanup(sess_hdl);
    if (status != 0) {
      InvalidPktOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void test_send_pkt(const SessionHandle_t sess_hdl,
                     const std::string &buf,
                     const int32_t size,
                     const int32_t tx_ring) {
    int status = pkt_pd_pkt_tx(sess_hdl, (uint8_t *)buf.c_str(), size, tx_ring);
    if (status != 0) {
      InvalidPktOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void test_verify_pkt_tx(const SessionHandle_t sess_hdl) {
    int status = pkt_pd_verify_tx(sess_hdl);
    if (status != 0) {
      InvalidPktOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void test_get_pkt_rx(pkt_pkt_data &_return, const SessionHandle_t sess_hdl) {
    uint32_t size, rx_ring;
    size = sizeof(pkt_pkt_buff);
    pkt_pd_get_rx(sess_hdl, pkt_pkt_buff, &size, &rx_ring);
    _return.buf.assign((const char *)pkt_pkt_buff, size);
    _return.size = size;
    _return.rx_ring = rx_ring;
  }
};
