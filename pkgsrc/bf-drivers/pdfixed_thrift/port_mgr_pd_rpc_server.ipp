#include "gen-cpp/port_mgr.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_port_mgr.h>
}

using namespace ::port_mgr_pd_rpc;

class port_mgrHandler : virtual public port_mgrIf {
 public:
  port_mgrHandler() {}

  void port_mgr_mtu_set(const int32_t dev_id,
                        const int32_t port_id,
                        const int32_t tx_mtu,
                        const int32_t rx_mtu) {
    int status = p4_port_mgr_mtu_set(dev_id, port_id, tx_mtu, rx_mtu);
    if (status != 0) {
      InvalidPortMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }
};
