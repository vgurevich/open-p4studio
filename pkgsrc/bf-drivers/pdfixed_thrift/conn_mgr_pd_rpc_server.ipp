#include "gen-cpp/conn_mgr.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_helper.h>
}

using namespace ::conn_mgr_pd_rpc;
using namespace ::res_pd_rpc;

class conn_mgrHandler : virtual public conn_mgrIf {
 public:
  conn_mgrHandler() {}

  void echo(const std::string &s) { std::cerr << "Echo: " << s << std::endl; }

  void init() { check_status(p4_pd_init(), InvalidConnMgrOperation()); }

  void cleanup() { p4_pd_cleanup(); }

  SessionHandle_t client_init() {
    p4_pd_sess_hdl_t sess_hdl;
    int status = p4_pd_client_init(&sess_hdl);
    check_status(status, InvalidConnMgrOperation());
    return sess_hdl;
  }

  void client_cleanup(const SessionHandle_t sess_hdl) {
    int status = p4_pd_client_cleanup(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void begin_txn(const SessionHandle_t sess_hdl, const bool isAtomic) {
    int status = p4_pd_begin_txn(sess_hdl, isAtomic);
    check_status(status, InvalidConnMgrOperation());
  }

  void verify_txn(const SessionHandle_t sess_hdl) {
    int status = p4_pd_verify_txn(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void abort_txn(const SessionHandle_t sess_hdl) {
    int status = p4_pd_abort_txn(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void commit_txn(const SessionHandle_t sess_hdl, const bool hwSynchronous) {
    int status = p4_pd_commit_txn(sess_hdl, hwSynchronous);
    check_status(status, InvalidConnMgrOperation());
  }

  void complete_operations(const SessionHandle_t sess_hdl) {
    int status = p4_pd_complete_operations(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void begin_batch(const SessionHandle_t sess_hdl) {
    int status = p4_pd_begin_batch(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void flush_batch(const SessionHandle_t sess_hdl) {
    int status = p4_pd_flush_batch(sess_hdl);
    check_status(status, InvalidConnMgrOperation());
  }

  void end_batch(const SessionHandle_t sess_hdl, const bool hwSynchronous) {
    int status = p4_pd_end_batch(sess_hdl, hwSynchronous);
    check_status(status, InvalidConnMgrOperation());
  }

  void log_state(const int32_t device_id, const std::string &filepath) {
    int status = p4_pd_log_state(device_id, filepath.c_str());
    check_status(status, InvalidConnMgrOperation());
  }

  void restore_state(const int32_t device_id, const std::string &filepath) {
    int status = p4_pd_restore_state(device_id, filepath.c_str());
    check_status(status, InvalidConnMgrOperation());
  }

  void advance_model_time(const SessionHandle_t sess_hdl,
                          const int32_t device_id,
                          const int64_t tick_time) {
    int status = p4_pd_advance_model_time(sess_hdl, device_id, tick_time);
    check_status(status, InvalidConnMgrOperation());
  }

  void recirculation_enable(const SessionHandle_t sess_hdl,
                            const int32_t dev,
                            const int32_t port) {
    int status = p4_pd_recirculation_enable(sess_hdl, dev, port);
    check_status(status, InvalidConnMgrOperation());
  }

  void recirculation_disable(const SessionHandle_t sess_hdl,
                             const int32_t dev,
                             const int32_t port) {
    int status = p4_pd_recirculation_disable(sess_hdl, dev, port);
    check_status(status, InvalidConnMgrOperation());
  }

  void pktgen_enable(const SessionHandle_t sess_hdl,
                     const int32_t dev,
                     const int32_t port) {
    int status = p4_pd_pktgen_enable(sess_hdl, dev, port);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_disable(const SessionHandle_t sess_hdl,
                      const int32_t dev,
                      const int32_t port) {
    int status = p4_pd_pktgen_disable(sess_hdl, dev, port);
    check_status(status, InvalidPktGenOperation());
  }

  bool pktgen_enable_state_get(const SessionHandle_t sess_hdl,
                               const int32_t dev,
                               const int32_t port) {
    bool enabled = false;
    int status = p4_pd_pktgen_enable_state_get(sess_hdl, dev, port, &enabled);
    check_status(status, InvalidPktGenOperation());
    return enabled;
  }

  void pktgen_enable_recirc_pattern_matching(const SessionHandle_t sess_hdl,
                                             const int32_t dev,
                                             const int32_t port) {
    int status =
        p4_pd_pktgen_enable_recirc_pattern_matching(sess_hdl, dev, port);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_disable_recirc_pattern_matching(const SessionHandle_t sess_hdl,
                                              const int32_t dev,
                                              const int32_t port) {
    int status =
        p4_pd_pktgen_disable_recirc_pattern_matching(sess_hdl, dev, port);
    check_status(status, InvalidPktGenOperation());
  }

  bool pktgen_recirc_pattern_matching_state_get(const SessionHandle_t sess_hdl,
                                                const int32_t dev,
                                                const int32_t port) {
    bool enabled = false;
    int status = p4_pd_pktgen_recirc_pattern_matching_state_get(
        sess_hdl, dev, port, &enabled);
    check_status(status, InvalidPktGenOperation());
    return enabled;
  }

  void pktgen_clear_port_down(const SessionHandle_t sess_hdl,
                              const int32_t dev,
                              const int32_t port) {
    int status = p4_pd_pktgen_clear_port_down(sess_hdl, dev, port);
    check_status(status, InvalidPktGenOperation());
  }

  bool pktgen_port_down_state_get(const SessionHandle_t sess_hdl,
                                  const int32_t dev,
                                  const int32_t port) {
    bool enabled = false;
    int status = p4_pd_pktgen_port_down_get(sess_hdl, dev, port, &enabled);
    check_status(status, InvalidPktGenOperation());
    return enabled;
  }

  void pktgen_cfg_app(const SessionHandle_t sess_hdl,
                      const DevTarget_t &dev_tgt,
                      const int32_t app_id,
                      const PktGenAppCfg_t &cfg) {
    p4_pd_pktgen_app_cfg pd_cfg = {};
    pd_cfg.trigger_type = (p4_pd_pktgen_trigger_type_e)cfg.trigger_type;
    pd_cfg.batch_count = cfg.batch_count;
    pd_cfg.packets_per_batch = cfg.pkt_count;
    pd_cfg.pattern_value = cfg.pattern_key;
    pd_cfg.pattern_mask = cfg.pattern_msk;
    pd_cfg.timer_nanosec = cfg.timer;
    pd_cfg.ibg = cfg.ibg;
    pd_cfg.ibg_jitter = cfg.ibg_jitter;
    pd_cfg.ipg = cfg.ipg;
    pd_cfg.ipg_jitter = cfg.ipg_jitter;
    pd_cfg.source_port = cfg.src_port;
    pd_cfg.increment_source_port = cfg.src_port_inc;
    pd_cfg.pkt_buffer_offset = cfg.buffer_offset;
    pd_cfg.length = cfg.length;
    p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                     i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_cfg_app(sess_hdl, pd_dev_tgt, app_id, pd_cfg);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_cfg_app_tof2(const SessionHandle_t sess_hdl,
                                const DevTarget_t &dev_tgt,
                                const int32_t app_id,
                                const PktGenAppCfg_tof2_t &cfg) {
    p4_pd_pktgen_app_cfg_tof2 pd_cfg = {};
    const bool is_size_err =
        (cfg.pattern_key.size() > 16 || cfg.pattern_msk.size() > 16 ||
         cfg.pfc_hdr.size() > 16);
    int status = is_size_err ? 3 : 0;
    check_status(status, InvalidPktGenOperation());

    pd_cfg.trigger_type =
        static_cast<p4_pd_pktgen_trigger_type_e>(cfg.trigger_type);
    pd_cfg.batch_count = static_cast<uint16_t>(cfg.batch_count);
    pd_cfg.packets_per_batch = static_cast<uint16_t>(cfg.pkt_count);
    memset(pd_cfg.pattern_value_long,
           0,
           sizeof(pd_cfg.pattern_value_long) /
               sizeof(pd_cfg.pattern_value_long[0]));
    memcpy(pd_cfg.pattern_value_long,
           cfg.pattern_key.data(),
           cfg.pattern_key.size());
    memset(
        pd_cfg.pattern_mask_long,
        0,
        sizeof(pd_cfg.pattern_mask_long) / sizeof(pd_cfg.pattern_mask_long[0]));
    memcpy(pd_cfg.pattern_mask_long,
           cfg.pattern_msk.data(),
           cfg.pattern_msk.size());
    memcpy(pd_cfg.pfc_hdr, cfg.pfc_hdr.data(), cfg.pfc_hdr.size());
    pd_cfg.pfc_timer_en = cfg.pfc_timer_en;
    pd_cfg.pfc_timer = cfg.pfc_timer;
    pd_cfg.pfc_max_msgs = cfg.pfc_max_msgs;
    pd_cfg.port_mask_sel = static_cast<uint8_t>(cfg.port_mask_sel);
    pd_cfg.source_port_wrap_max =
        static_cast<uint8_t>(cfg.source_port_wrap_max);
    pd_cfg.assigned_chnl_id = static_cast<uint8_t>(cfg.assigned_chnl_id);
    pd_cfg.offset_len_from_recir_pkt = cfg.offset_len_from_recir_pkt;
    pd_cfg.timer_nanosec = cfg.timer;
    pd_cfg.ibg = cfg.ibg;
    pd_cfg.ibg_jitter = cfg.ibg_jitter;
    pd_cfg.ipg = cfg.ipg;
    pd_cfg.ipg_jitter = cfg.ipg_jitter;
    pd_cfg.source_port = static_cast<uint16_t>(cfg.src_port);
    pd_cfg.increment_source_port = cfg.src_port_inc;
    pd_cfg.pkt_buffer_offset = static_cast<uint16_t>(cfg.buffer_offset);
    pd_cfg.length = static_cast<uint16_t>(cfg.length);
    p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                     i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    status =
        p4_pd_pktgen_cfg_app_tof2(sess_hdl, pd_dev_tgt, app_id, pd_cfg);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_cfg_port_mask_tof2(const SessionHandle_t sess_hdl,
                                      const DevTarget_t &dev_tgt,
                                      const int32_t sel,
                                      const PortMask_t &mask_in) {
    p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                     i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_port_down_mask_tof2 pd_mask = {};
    int status = (mask_in.mask.size() > 9) ? 3 : 0;
    check_status(status, InvalidPktGenOperation());
    memcpy(pd_mask.port_mask, mask_in.mask.data(), mask_in.mask.size());
    status = p4_pd_pktgen_port_down_msk_tof2(
        sess_hdl, pd_dev_tgt, sel, pd_mask);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_cfg_port_mask_tof2_get(PortMask_t &_return,
                                          const DevTarget_t &dev_tgt,
                                          const int32_t sel) {
    p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                     i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_port_down_mask_tof2 pd_mask = {};
    int status =
        p4_pd_pktgen_port_down_msk_tof2_get(pd_dev_tgt, sel, &pd_mask);
    check_status(status, InvalidPktGenOperation());
    constexpr int size =
        sizeof(pd_mask.port_mask) / sizeof(pd_mask.port_mask[0]);
    _return.mask.assign(reinterpret_cast<char *>(&pd_mask.port_mask[0]), size);
  }

  void pktgen_cfg_app_get(PktGenAppCfg_t &_return,
                          const SessionHandle_t sess_hdl,
                          const DevTarget_t &dev_tgt,
                          const int32_t app_id) {
    p4_pd_pktgen_app_cfg cfg = {};
    const p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                           i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_cfg_app_get(sess_hdl, pd_dev_tgt, app_id, &cfg);
    check_status(status, InvalidPktGenOperation());

    _return.trigger_type =
        static_cast<PktGenTriggerType_t::type>(cfg.trigger_type);
    _return.batch_count = cfg.batch_count;
    _return.pkt_count = cfg.packets_per_batch;
    _return.pattern_key = cfg.pattern_value;
    _return.pattern_msk = cfg.pattern_mask;
    _return.timer = cfg.timer_nanosec;
    _return.ibg = cfg.ibg;
    _return.ibg_jitter = cfg.ibg_jitter;
    _return.ipg = cfg.ipg;
    _return.ipg_jitter = cfg.ipg_jitter;
    _return.src_port = cfg.source_port;
    _return.src_port_inc = cfg.increment_source_port;
    _return.buffer_offset = cfg.pkt_buffer_offset;
    _return.length = cfg.length;
  }

  void pktgen_cfg_app_tof2_get(PktGenAppCfg_tof2_t &_return,
                                    const SessionHandle_t sess_hdl,
                                    const DevTarget_t &dev_tgt,
                                    const int32_t app_id) {
    p4_pd_pktgen_app_cfg_tof2 cfg = {};
    const p4_pd_dev_target_t pd_dev_tgt = {dev_tgt.dev_id,
                                           i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status =
        p4_pd_pktgen_cfg_app_tof2_get(sess_hdl, pd_dev_tgt, app_id, &cfg);
    check_status(status, InvalidPktGenOperation());

    _return.trigger_type =
        static_cast<PktGenTriggerType_t::type>(cfg.trigger_type);
    _return.batch_count = static_cast<int32_t>(cfg.batch_count);
    _return.pkt_count = static_cast<int32_t>(cfg.packets_per_batch);
    uint32_t size =
        sizeof(cfg.pattern_value_long) / sizeof(cfg.pattern_value_long[0]);
    _return.pattern_key.assign(reinterpret_cast<char *>(&cfg.pattern_value_long[0]), size);

    size = sizeof(cfg.pattern_mask_long) / sizeof(cfg.pattern_mask_long[0]);
    _return.pattern_msk.assign(reinterpret_cast<char *>(&cfg.pattern_mask_long[0]), size);

    size = sizeof(cfg.pfc_hdr) / sizeof(cfg.pfc_hdr[0]);
    _return.pfc_hdr.assign(reinterpret_cast<char *>(&cfg.pfc_hdr[0]), size);

    _return.pfc_timer_en = cfg.pfc_timer_en;
    _return.pfc_timer = cfg.pfc_timer;
    _return.pfc_max_msgs = cfg.pfc_max_msgs;
    _return.port_mask_sel = static_cast<int16_t>(cfg.port_mask_sel);
    _return.source_port_wrap_max =
        static_cast<int16_t>(cfg.source_port_wrap_max);
    _return.assigned_chnl_id = static_cast<int16_t>(cfg.assigned_chnl_id);
    _return.offset_len_from_recir_pkt = cfg.offset_len_from_recir_pkt;
    _return.timer = cfg.timer_nanosec;
    _return.ibg = cfg.ibg;
    _return.ibg_jitter = cfg.ibg_jitter;
    _return.ipg = cfg.ipg;
    _return.ipg_jitter = cfg.ipg_jitter;
    _return.src_port = static_cast<int32_t>(cfg.source_port);
    _return.src_port_inc = cfg.increment_source_port;
    _return.buffer_offset = static_cast<int32_t>(cfg.pkt_buffer_offset);
    _return.length = static_cast<int32_t>(cfg.length);
  }

  void pktgen_app_enable(const SessionHandle_t sess_hdl,
                         const DevTarget_t &dev_tgt,
                         const int32_t app_id) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_app_enable(sess_hdl, tgt, app_id);
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_app_disable(const SessionHandle_t sess_hdl,
                          const DevTarget_t &dev_tgt,
                          const int32_t app_id) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_app_disable(sess_hdl, tgt, app_id);
    check_status(status, InvalidPktGenOperation());
  }

  bool pktgen_app_enable_state_get(const SessionHandle_t sess_hdl,
                                   const DevTarget_t &dev_tgt,
                                   const int32_t app_id) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    bool enable = false;
    int status =
        p4_pd_pktgen_app_enable_state_get(sess_hdl, tgt, app_id, &enable);
    check_status(status, InvalidPktGenOperation());
    return enable;
  }

  void pktgen_write_pkt_buffer(const SessionHandle_t sess_hdl,
                               const DevTarget_t &dev_tgt,
                               const int32_t offset,
                               int32_t size,
                               const std::string &buf) {
    int status;
    if (size > static_cast<int32_t>(buf.length())) {
      status = BF_INVALID_ARG;
      check_status(status, InvalidPktGenOperation());
    }

    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    status = p4_pd_pktgen_write_pkt_buffer(
        sess_hdl, tgt, offset, size, (uint8_t *)buf.c_str());
    check_status(status, InvalidPktGenOperation());
  }

  void pktgen_read_pkt_buffer(std::string &_return,
                              const SessionHandle_t sess_hdl,
                              const DevTarget_t &dev_tgt,
                              const int32_t offset,
                              const int32_t size) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    std::vector<uint8_t> buff(size);
    int status =
        p4_pd_pktgen_read_pkt_buffer(sess_hdl, tgt, offset, size, buff.data());
    check_status(status, InvalidPktGenOperation());
    _return = std::string(buff.begin(), buff.end());
  }

  int64_t pktgen_get_batch_counter(const SessionHandle_t sess_hdl,
                                   const DevTarget_t &dev_tgt,
                                   const int32_t app_id) {
    uint64_t rv = 0;
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_get_batch_counter(sess_hdl, tgt, app_id, &rv);
    check_status(status, InvalidPktGenOperation());
    return rv;
  }

  int64_t pktgen_get_pkt_counter(const SessionHandle_t sess_hdl,
                                 const DevTarget_t &dev_tgt,
                                 const int32_t app_id) {
    uint64_t rv = 0;
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_get_pkt_counter(sess_hdl, tgt, app_id, &rv);
    check_status(status, InvalidPktGenOperation());
    return rv;
  }

  int64_t pktgen_get_trigger_counter(const SessionHandle_t sess_hdl,
                                     const DevTarget_t &dev_tgt,
                                     const int32_t app_id) {
    uint64_t rv = 0;
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_pktgen_get_trigger_counter(sess_hdl, tgt, app_id, &rv);
    check_status(status, InvalidPktGenOperation());
    return rv;
  }

  void pktgen_set_batch_counter(const SessionHandle_t sess_hdl,
                                const DevTarget_t &dev_tgt,
                                const int32_t app_id,
                                const int64_t count) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_pktgen_set_batch_counter(sess_hdl, tgt, app_id, count);
  }

  void pktgen_set_pkt_counter(const SessionHandle_t sess_hdl,
                              const DevTarget_t &dev_tgt,
                              const int32_t app_id,
                              const int64_t count) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_pktgen_set_pkt_counter(sess_hdl, tgt, app_id, count);
  }

  void pktgen_set_trigger_counter(const SessionHandle_t sess_hdl,
                                  const DevTarget_t &dev_tgt,
                                  const int32_t app_id,
                                  const int64_t count) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_pktgen_set_trigger_counter(sess_hdl, tgt, app_id, count);
  }
  void pktgen_port_down_replay_mode_set(
      const SessionHandle_t sess_hdl,
      const DevTarget_t &dev_tgt,
      const PktGenPortDownReplay_t::type mode) {
    p4_pd_dev_target_t tgt = {dev_tgt.dev_id,
                              i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_pktgen_port_down_mode_t p4_pd_mode;
    switch (mode) {
      case PktGenPortDownReplay_t::REPLAY_NONE:
        p4_pd_mode = PD_PKTGEN_PORT_DOWN_REPLAY_NONE;
        break;
      case PktGenPortDownReplay_t::REPLAY_ALL:
        p4_pd_mode = PD_PKTGEN_PORT_DOWN_REPLAY_ALL;
        break;
      case PktGenPortDownReplay_t::REPLAY_MISSED:
        p4_pd_mode = PD_PKTGEN_PORT_DOWN_REPLAY_MISSED;
        break;
      default:
        p4_pd_mode = PD_PKTGEN_PORT_DOWN_REPLAY_NONE;
    }
    p4_pd_pktgen_port_down_replay_mode_set(sess_hdl, tgt, p4_pd_mode);
  }

  PktGenPortDownReplay_t::type pktgen_port_down_replay_mode_get(
      const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id,
                            i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_pktgen_port_down_mode_t p4_pd_mode;
    p4_pd_pktgen_port_down_replay_mode_get(sess_hdl, d, &p4_pd_mode);
    switch (p4_pd_mode) {
      case PD_PKTGEN_PORT_DOWN_REPLAY_NONE:
        return PktGenPortDownReplay_t::REPLAY_NONE;
      case PD_PKTGEN_PORT_DOWN_REPLAY_ALL:
        return PktGenPortDownReplay_t::REPLAY_ALL;
      case PD_PKTGEN_PORT_DOWN_REPLAY_MISSED:
        return PktGenPortDownReplay_t::REPLAY_MISSED;
      default:
        return PktGenPortDownReplay_t::REPLAY_NONE;
    }
  }

  void reg_wr(const int32_t dev, const int32_t addr, const int32_t data) {
    p4_pd_reg_wr(dev, addr, data);
  }
  int32_t reg_rd(const int32_t dev, const int32_t addr) {
    return p4_pd_reg_rd(dev, addr);
  }
  void ind_reg_wr(const int32_t dev,
                  const int64_t addr,
                  const indirect_reg_data_t &data) {
    p4_pd_ind_reg_wr(dev, addr, data.hi, data.lo);
  }
  void ind_reg_rd(indirect_reg_data_t &_return,
                  const int32_t dev,
                  const int64_t addr) {
    uint64_t hi = 0, lo = 0;
    p4_pd_ind_reg_rd(dev, addr, &hi, &lo);
    _return.hi = hi;
    _return.lo = lo;
  }

  void tcam_scrub_timer_set(const int32_t dev, const int32_t msec_timer) {
    int status = p4_pd_tcam_scrub_timer_set(dev, msec_timer);
    check_status(status, InvalidConnMgrOperation());
  }

  int32_t tcam_scrub_timer_get(const int32_t dev) {
    return p4_pd_tcam_scrub_timer_get(dev);
  }

  void flow_lrn_set_intr_mode(const SessionHandle_t sess_hdl,
                              const int32_t dev,
                              const bool en) {
    int status = p4_pd_flow_lrn_set_intr_mode(sess_hdl, dev, en);
    check_status(status, InvalidConnMgrOperation());
  }

  bool flow_lrn_get_intr_mode(const SessionHandle_t sess_hdl,
                              const int32_t dev) {
    bool r = false;
    int status = p4_pd_flow_lrn_get_intr_mode(sess_hdl, dev, &r);
    check_status(status, InvalidConnMgrOperation());
    return r;
  }

 private:
  template <typename T>
  static void check_status(int status, T exception) {
    if (status != 0) {
      exception.code = status;
      throw exception;
    }
  }
};
