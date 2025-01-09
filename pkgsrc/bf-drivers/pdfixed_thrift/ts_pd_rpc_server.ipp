#include "gen-cpp/ts.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_ts.h>
}

namespace {
  inline void check_status(const int status)
  {
    if (status == 0) {
      return;
    }

    ::ts_pd_rpc::InvalidTimestampOperation top;
    top.code = status;
    throw top;
  }
}

using namespace ::ts_pd_rpc;

class tsHandler : virtual public tsIf {
 public:
  tsHandler() {}

  void ts_global_ts_state_set(const ts_dev_t dev_id, bool enable) {
    const int status = p4_pd_ts_global_ts_state_set(dev_id, enable);
    check_status(status);
  }

  bool ts_global_ts_state_get(const ts_dev_t dev_id) {
    bool enable = false;
    const int status = p4_pd_ts_global_ts_state_get(dev_id, &enable);
    check_status(status);
    return enable;
  }

  void ts_global_ts_value_set(const ts_dev_t dev_id, const int64_t global_ts) {
    const int status = p4_pd_ts_global_ts_value_set(dev_id, global_ts);
    check_status(status);
  }

  int64_t ts_global_ts_value_get(const ts_dev_t dev_id) {
    uint64_t ts = 0;
    const int status = p4_pd_ts_global_ts_value_get(dev_id, &ts);
    check_status(status);
    return static_cast<int64_t>(ts);
  }

  void ts_global_ts_inc_value_set(const ts_dev_t dev_id,
                                  const int32_t global_inc_ns) {
    const int status = p4_pd_ts_global_ts_inc_value_set(dev_id, global_inc_ns);
    check_status(status);
  }

  int64_t ts_global_ts_inc_value_get(const ts_dev_t dev_id) {
    uint32_t ts = 0;
    const int status = p4_pd_ts_global_ts_inc_value_get(dev_id, &ts);
    check_status(status);
    return static_cast<int64_t>(ts);
  }

  void ts_global_ts_offset_value_set(const ts_dev_t dev_id,
                                     const int64_t global_ts){
    const int status = p4_pd_ts_global_ts_offset_value_set(dev_id, global_ts);
    check_status(status);
  }

  int64_t ts_global_ts_offset_value_get(const ts_dev_t dev_id) {
    uint64_t ts = 0;
    const int status = p4_pd_ts_global_ts_offset_value_get(dev_id, &ts);
    check_status(status);
    return static_cast<int64_t>(ts);
  }

  void ts_global_baresync_ts_get(ts_global_baresync_t &_return,
                                 const ts_dev_t dev_id) {
    uint64_t global_ts = 0;
    uint64_t baresync_ts = 0;
    const int status = p4_pd_ts_global_baresync_ts_get(dev_id,
                                                       &global_ts,
                                                       &baresync_ts);
    _return.global_ts = static_cast<int64_t>(global_ts);
    _return.baresync_ts = static_cast<int64_t>(baresync_ts);
    check_status(status);
  }

  void ts_1588_timestamp_delta_tx_set(const ts_dev_t dev_id,
                                      const ts_port_t port_id,
                                      const int16_t delta) {
    const int status = p4_pd_ts_1588_timestamp_delta_tx_set(dev_id,
                                                            port_id,
                                                            delta);
    check_status(status);
  }

  int16_t ts_1588_timestamp_delta_tx_get(const ts_dev_t dev_id,
                                         const ts_port_t port_id) {
    uint16_t delta = 0;
    const int status = p4_pd_ts_1588_timestamp_delta_tx_get(dev_id,
                                                            port_id,
                                                            &delta);
    check_status(status);
    return static_cast<int16_t>(delta);
  }

  void ts_1588_timestamp_delta_rx_set(const ts_dev_t dev_id,
                                      const ts_port_t port_id,
                                      const int16_t delta) {
    const int status = p4_pd_ts_1588_timestamp_delta_rx_set(dev_id,
                                                            port_id,
                                                            delta);
    check_status(status);
  }

  int16_t ts_1588_timestamp_delta_rx_get(const ts_dev_t dev_id,
                                         const ts_port_t port_id) {
    uint16_t delta = 0;
    const int status = p4_pd_ts_1588_timestamp_delta_rx_get(dev_id,
                                                            port_id,
                                                            &delta);
    check_status(status);
    return static_cast<int16_t>(delta);
  }

  void ts_1588_timestamp_tx_get(ts_1588_timestamp_t &_return,
                                const ts_dev_t dev_id,
                                const ts_port_t port_id) {
    uint64_t ts = 0;
    const int status = p4_pd_ts_1588_timestamp_tx_get(dev_id,
                                                      port_id,
                                                      &ts,
                                                      &_return.ts_valid,
                                                      &_return.ts_id);
    _return.ts = static_cast<uint64_t>(ts);
    check_status(status);
  }
};
