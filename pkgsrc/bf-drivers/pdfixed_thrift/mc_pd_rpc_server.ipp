#include "gen-cpp/mc.h"

extern "C" {
#include <bf_types/bf_types.h>
#include <mc_mgr/mc_mgr_types.h>
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mc.h>
}

#include <string>

using namespace ::mc_pd_rpc;
using namespace ::res_pd_rpc;

class mcHandler : virtual public mcIf {
 public:
  mcHandler() {}

  void mc_init() {
    int status = p4_pd_mc_init();
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  SessionHandle_t mc_create_session() {
    SessionHandle_t sess_hdl;
    int status = p4_pd_mc_create_session((uint32_t *)&sess_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return sess_hdl;
  }

  void mc_destroy_session(const SessionHandle_t sess_hdl) {
    int status = p4_pd_mc_destroy_session(sess_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_complete_operations(const SessionHandle_t sess_hdl) {
    int status = p4_pd_mc_complete_operations(sess_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_begin_batch(const SessionHandle_t sess_hdl) {
    int status = p4_pd_mc_begin_batch(sess_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_flush_batch(const SessionHandle_t sess_hdl) {
    int status = p4_pd_mc_flush_batch(sess_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_end_batch(const SessionHandle_t sess_hdl, const bool hwSynchronous) {
    int status = p4_pd_mc_end_batch(sess_hdl, hwSynchronous);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_mgrp_create(const SessionHandle_t sess_hdl,
                            const int dev,
                            const int16_t mgid) {
    McHandle_t grp_hdl;
    int status =
        p4_pd_mc_mgrp_create(sess_hdl, dev, mgid, (uint32_t *)&grp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return grp_hdl;
  }

  void mc_mgrp_get_attr(mc_mgrp_attr &attr,
                        const SessionHandle_t sess_hdl,
                        const int dev,
                        McHandle_t mgrp_hdl) {
    int status =
        p4_pd_mc_mgrp_get_attr(sess_hdl, dev, mgrp_hdl, (uint16_t *)&attr.mgid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_mgrp_destroy(const SessionHandle_t sess_hdl,
                       const int dev,
                       const McHandle_t grp_hdl) {
    int status = p4_pd_mc_mgrp_destroy(sess_hdl, dev, grp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_mgrp_get_first(const SessionHandle_t sess_hdl, const int dev) {
    McHandle_t grp_hdl;
    int status = p4_pd_mc_mgrp_get_first(sess_hdl, dev, (uint32_t *)&grp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return grp_hdl;
  }

  int mc_mgrp_get_count(const SessionHandle_t sess_hdl, const int dev) {
    uint32_t count;
    int status = p4_pd_mc_mgrp_get_count(sess_hdl, dev, &count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return count;
  }

  void mc_mgrp_get_next_i(std::vector<McHandle_t> &mgrp_hdls,
                          const SessionHandle_t sess_hdl,
                          const int dev,
                          const McHandle_t mgrp_hdl,
                          const int n) {
    mgrp_hdls = std::vector<int32_t>(n);
    int status = p4_pd_mc_mgrp_get_next_i(
        sess_hdl, dev, mgrp_hdl, n, (uint32_t *)mgrp_hdls.data());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_mgrp_get_first_node_mbr(mc_mgrp_node_mbr &_return,
                                  const ::res_pd_rpc::SessionHandle_t sess_hdl,
                                  const int32_t dev_id,
                                  const McHandle_t mgrp_hdl) {
    p4_pd_entry_hdl_t node_hdl;
    bool xid_valid = false;
    uint16_t xid;
    int status = p4_pd_mc_mgrp_get_first_node_mbr(
        sess_hdl, dev_id, mgrp_hdl, &node_hdl, &xid_valid, &xid);
    _return.node_hdl = node_hdl;
    _return.xid_valid = xid_valid;
    _return.xid = xid;
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int32_t mc_mgrp_get_node_mbr_count(
      const ::res_pd_rpc::SessionHandle_t sess_hdl,
      const int32_t dev_id,
      const McHandle_t mgrp_hdl) {
    uint32_t c = 0;
    int status =
        p4_pd_mc_mgrp_get_node_mbr_count(sess_hdl, dev_id, mgrp_hdl, &c);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return c;
  }

  void mc_mgrp_get_next_i_node_mbr(std::vector<mc_mgrp_node_mbr> &_return,
                                   const ::res_pd_rpc::SessionHandle_t sess_hdl,
                                   const int32_t dev_id,
                                   const McHandle_t mgrp_hdl,
                                   const McHandle_t node_hdl,
                                   const int32_t n) {
    _return = std::vector<mc_mgrp_node_mbr>(n);
    p4_pd_entry_hdl_t *hdls = new p4_pd_entry_hdl_t[n];
    bool *xid_valids = new bool[n];
    uint16_t *xids = new uint16_t[n];
    int status = p4_pd_mc_mgrp_get_next_i_node_mbr(
        sess_hdl, dev_id, mgrp_hdl, node_hdl, n, hdls, xid_valids, xids);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    for (int i = 0; i < n; ++i) {
      _return[i].node_hdl = hdls[i];
      _return[i].xid_valid = xid_valids[i];
      _return[i].xid = xids[i];
    }
    delete[] hdls;
    delete[] xid_valids;
    delete[] xids;
  }

  void mc_mgrp_get_first_ecmp_mbr(mc_mgrp_ecmp_mbr &_return,
                                  const ::res_pd_rpc::SessionHandle_t sess_hdl,
                                  const int32_t dev_id,
                                  const McHandle_t mgrp_hdl) {
    p4_pd_entry_hdl_t ecmp_hdl;
    bool xid_valid = false;
    uint16_t xid;
    int status = p4_pd_mc_mgrp_get_first_ecmp_mbr(
        sess_hdl, dev_id, mgrp_hdl, &ecmp_hdl, &xid_valid, &xid);
    _return.ecmp_hdl = ecmp_hdl;
    _return.xid_valid = xid_valid;
    _return.xid = xid;
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }
  int32_t mc_mgrp_get_ecmp_mbr_count(
      const ::res_pd_rpc::SessionHandle_t sess_hdl,
      const int32_t dev_id,
      const McHandle_t mgrp_hdl) {
    uint32_t c = 0;
    int status =
        p4_pd_mc_mgrp_get_ecmp_mbr_count(sess_hdl, dev_id, mgrp_hdl, &c);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return c;
  }
  void mc_mgrp_get_next_i_ecmp_mbr(std::vector<mc_mgrp_ecmp_mbr> &_return,
                                   const ::res_pd_rpc::SessionHandle_t sess_hdl,
                                   const int32_t dev_id,
                                   const McHandle_t mgrp_hdl,
                                   const McHandle_t ecmp_hdl,
                                   const int32_t n) {
    p4_pd_entry_hdl_t *hdls = new p4_pd_entry_hdl_t[n];
    bool *xid_valids = new bool[n];
    uint16_t *xids = new uint16_t[n];
    _return = std::vector<mc_mgrp_ecmp_mbr>(n);
    int status = p4_pd_mc_mgrp_get_next_i_ecmp_mbr(
        sess_hdl, dev_id, mgrp_hdl, ecmp_hdl, n, hdls, xid_valids, xids);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    for (int i = 0; i < n; ++i) {
      _return[i].ecmp_hdl = hdls[i];
      _return[i].xid_valid = xid_valids[i];
      _return[i].xid = xids[i];
    }
    delete[] hdls;
    delete[] xid_valids;
    delete[] xids;
  }

  McHandle_t mc_node_create(const SessionHandle_t sess_hdl,
                            const int dev,
                            const int16_t rid,
                            const std::string &port_map,
                            const std::string &lag_map) {
    McHandle_t l1_hdl;
    int status = p4_pd_mc_node_create(sess_hdl,
                                      dev,
                                      rid,
                                      (uint8_t *)port_map.c_str(),
                                      (uint8_t *)lag_map.c_str(),
                                      (uint32_t *)&l1_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return l1_hdl;
  }

  void mc_node_get_attr(mc_node_attr &attr,
                        const SessionHandle_t sess_hdl,
                        const int dev,
                        const McHandle_t node_hdl) {
    std::string port_buf(BF_MC_PORT_ARRAY_SIZE, '\n');
    std::string lag_buf(32, '\n');
    int status = p4_pd_mc_node_get_attr(sess_hdl,
                                        dev,
                                        node_hdl,
                                        (uint16_t *)&attr.rid,
                                        (uint8_t *)port_buf.c_str(),
                                        (uint8_t *)lag_buf.c_str());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    attr.port_map = port_buf;
    attr.lag_map = lag_buf;
  }

  void mc_node_update(const SessionHandle_t sess_hdl,
                      const int dev,
                      const McHandle_t node_hdl,
                      const std::string &port_map,
                      const std::string &lag_map) {
    int status = p4_pd_mc_node_update(sess_hdl,
                                      dev,
                                      node_hdl,
                                      (uint8_t *)port_map.c_str(),
                                      (uint8_t *)lag_map.c_str());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_node_destroy(const SessionHandle_t sess_hdl,
                       const int dev,
                       const McHandle_t node_hdl) {
    int status = p4_pd_mc_node_destroy(sess_hdl, dev, node_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_node_get_first(const SessionHandle_t sess_hdl, const int dev) {
    McHandle_t node_hdl;
    int status = p4_pd_mc_node_get_first(sess_hdl, dev, (uint32_t *)&node_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return node_hdl;
  }

  int mc_node_get_count(const SessionHandle_t sess_hdl, const int dev) {
    uint32_t count;
    int status = p4_pd_mc_node_get_count(sess_hdl, dev, &count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return count;
  }

  void mc_node_get_next_i(std::vector<McHandle_t> &node_hdls,
                          const SessionHandle_t sess_hdl,
                          const int dev,
                          const McHandle_t node_hdl,
                          const int n) {
    node_hdls = std::vector<int32_t>(n);
    int status = p4_pd_mc_node_get_next_i(
        sess_hdl, dev, node_hdl, n, (uint32_t *)node_hdls.data());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_node_get_association(mc_node_assoc_attr &attr,
                               const SessionHandle_t sess_hdl,
                               const int dev,
                               const McHandle_t node_hdl) {
    int status = p4_pd_mc_node_get_association(sess_hdl,
                                               dev,
                                               node_hdl,
                                               &attr.is_associated,
                                               (uint32_t *)&attr.mgrp_hdl,
                                               (bool *)&attr.xid_valid,
                                               (uint16_t *)&attr.xid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_node_is_mbr(mc_node_is_member &mbr,
                      const SessionHandle_t sess_hdl,
                      const int dev,
                      const McHandle_t node_hdl) {
    int status = p4_pd_mc_node_is_mbr(
        sess_hdl, dev, node_hdl, &mbr.is_ecmp_mbr, (uint32_t *)&mbr.ecmp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_associate_node(const SessionHandle_t sess_hdl,
                         const int dev,
                         const McHandle_t grp_hdl,
                         const McHandle_t l1_hdl,
                         const int16_t xid,
                         const int8_t xid_valid) {
    int status =
        p4_pd_mc_associate_node(sess_hdl, dev, grp_hdl, l1_hdl, xid, xid_valid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_dissociate_node(const SessionHandle_t sess_hdl,
                          const int dev,
                          const McHandle_t grp_hdl,
                          const McHandle_t l1_hdl) {
    int status = p4_pd_mc_dissociate_node(sess_hdl, dev, grp_hdl, l1_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_ecmp_create(const SessionHandle_t sess_hdl, const int dev) {
    McHandle_t ecmp_hdl;
    int status = p4_pd_mc_ecmp_create(sess_hdl, dev, (uint32_t *)&ecmp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return ecmp_hdl;
  }

  void mc_ecmp_destroy(const SessionHandle_t sess_hdl,
                       const int dev,
                       const McHandle_t ecmp_hdl) {
    int status = p4_pd_mc_ecmp_destroy(sess_hdl, dev, ecmp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_ecmp_get_first(const SessionHandle_t sess_hdl, const int dev) {
    McHandle_t ecmp_hdl;
    int status = p4_pd_mc_ecmp_get_first(sess_hdl, dev, (uint32_t *)&ecmp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return ecmp_hdl;
  }

  int mc_ecmp_get_count(const SessionHandle_t sess_hdl, const int dev) {
    uint32_t count;
    int status = p4_pd_mc_ecmp_get_count(sess_hdl, dev, &count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return count;
  }

  void mc_ecmp_get_next_i(std::vector<McHandle_t> &ecmp_hdls,
                          const SessionHandle_t sess_hdl,
                          const int dev,
                          const McHandle_t ecmp_hdl,
                          const int n) {
    ecmp_hdls = std::vector<int32_t>(n);
    int status = p4_pd_mc_ecmp_get_next_i(
        sess_hdl, dev, ecmp_hdl, n, (uint32_t *)ecmp_hdls.data());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_ecmp_mbr_add(const SessionHandle_t sess_hdl,
                       const int dev,
                       const McHandle_t ecmp_hdl,
                       const McHandle_t l1_hdl) {
    int status = p4_pd_mc_ecmp_mbr_add(sess_hdl, dev, ecmp_hdl, l1_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_ecmp_mbr_rem(const SessionHandle_t sess_hdl,
                       const int dev,
                       const McHandle_t ecmp_hdl,
                       const McHandle_t l1_hdl) {
    int status = p4_pd_mc_ecmp_mbr_rem(sess_hdl, dev, ecmp_hdl, l1_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_ecmp_get_first_mbr(const SessionHandle_t sess_hdl,
                                   const int dev,
                                   const McHandle_t ecmp_hdl) {
    McHandle_t node_hdl;
    int status = p4_pd_mc_ecmp_get_first_mbr(
        sess_hdl, dev, ecmp_hdl, (uint32_t *)&node_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return node_hdl;
  }

  int mc_ecmp_get_mbr_count(const SessionHandle_t sess_hdl,
                            const int dev,
                            const McHandle_t ecmp_hdl) {
    uint32_t count;
    int status = p4_pd_mc_ecmp_get_mbr_count(sess_hdl, dev, ecmp_hdl, &count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return count;
  }

  void mc_ecmp_get_next_i_mbr(std::vector<McHandle_t> &node_hdls,
                              const SessionHandle_t sess_hdl,
                              const int dev,
                              const McHandle_t ecmp_hdl,
                              const McHandle_t node_hdl,
                              const int n) {
    node_hdls = std::vector<int32_t>(n);
    int status = p4_pd_mc_ecmp_get_next_i_mbr(
        sess_hdl, dev, ecmp_hdl, node_hdl, n, (uint32_t *)node_hdls.data());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_ecmp_get_mbr_from_hash(mc_ecmp_mbr &mbr,
                                 const SessionHandle_t sess_hdl,
                                 const int dev,
                                 const McHandle_t mgrp_hdl,
                                 const McHandle_t ecmp_hdl,
                                 const int16_t level1_mcast_hash,
                                 const int16_t pkt_xid) {
    int status = p4_pd_mc_ecmp_get_mbr_from_hash(sess_hdl,
                                                 dev,
                                                 mgrp_hdl,
                                                 ecmp_hdl,
                                                 level1_mcast_hash,
                                                 pkt_xid,
                                                 (uint32_t *)&mbr.node_hdl,
                                                 &mbr.is_pruned);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  McHandle_t mc_ecmp_get_first_assoc(const SessionHandle_t sess_hdl,
                                     const int dev,
                                     const McHandle_t ecmp_hdl) {
    McHandle_t mgrp_hdl;
    int status = p4_pd_mc_ecmp_get_first_assoc(
        sess_hdl, dev, ecmp_hdl, (uint32_t *)&mgrp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return mgrp_hdl;
  }

  int mc_ecmp_get_assoc_count(const SessionHandle_t sess_hdl,
                              const int dev,
                              const McHandle_t ecmp_hdl) {
    uint32_t count;
    int status = p4_pd_mc_ecmp_get_assoc_count(sess_hdl, dev, ecmp_hdl, &count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return count;
  }

  void mc_ecmp_get_next_i_assoc(std::vector<McHandle_t> &mgrp_hdls,
                                const SessionHandle_t sess_hdl,
                                const int dev,
                                const McHandle_t ecmp_hdl,
                                const McHandle_t mgrp_hdl,
                                const int n) {
    mgrp_hdls = std::vector<int32_t>(n);
    int status = p4_pd_mc_ecmp_get_next_i_assoc(
        sess_hdl, dev, ecmp_hdl, mgrp_hdl, n, (uint32_t *)mgrp_hdls.data());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_associate_ecmp(const SessionHandle_t sess_hdl,
                         const int dev,
                         const McHandle_t grp_hdl,
                         const McHandle_t ecmp_hdl,
                         const int16_t xid,
                         const int8_t xid_valid) {
    int status = p4_pd_mc_associate_ecmp(
        sess_hdl, dev, grp_hdl, ecmp_hdl, xid, xid_valid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_ecmp_get_assoc_attr(mc_ecmp_assoc_attr &attr,
                              const SessionHandle_t sess_hdl,
                              const int dev,
                              const McHandle_t grp_hdl,
                              const McHandle_t ecmp_hdl) {
    int status = p4_pd_mc_ecmp_get_assoc_attr(sess_hdl,
                                              dev,
                                              grp_hdl,
                                              ecmp_hdl,
                                              (bool *)&attr.xid_valid,
                                              (uint16_t *)&attr.xid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_dissociate_ecmp(const SessionHandle_t sess_hdl,
                          const int dev,
                          const McHandle_t grp_hdl,
                          const McHandle_t ecmp_hdl) {
    int status = p4_pd_mc_dissociate_ecmp(sess_hdl, dev, grp_hdl, ecmp_hdl);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_lag_membership(const SessionHandle_t sess_hdl,
                             const int dev,
                             const int8_t lag,
                             const std::string &port_map) {
    int32_t status = p4_pd_mc_set_lag_membership(
        sess_hdl, dev, lag, (uint8_t *)port_map.c_str());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_get_lag_membership(std::string &port_map,
                             const SessionHandle_t sess_hdl,
                             const int dev,
                             const int8_t lag) {
    std::string port_buf(BF_MC_PORT_ARRAY_SIZE, '\n');
    int32_t status = p4_pd_mc_get_lag_membership(
        sess_hdl, dev, lag, (uint8_t *)port_buf.c_str());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    port_map = port_buf;
  }

  void mc_get_lag_member_from_hash(mc_lag_mbr &mbr,
                                   const SessionHandle_t sess_hdl,
                                   const int dev,
                                   const McHandle_t l1_hdl,
                                   const int8_t lag,
                                   const int16_t level2_mcast_hash,
                                   const int16_t pkt_xid,
                                   const int16_t pkt_yid,
                                   const int16_t pkt_rid) {
    int32_t status = p4_pd_mc_get_lag_member_from_hash(sess_hdl,
                                                       dev,
                                                       l1_hdl,
                                                       lag,
                                                       level2_mcast_hash,
                                                       pkt_xid,
                                                       pkt_yid,
                                                       pkt_rid,
                                                       &mbr.port,
                                                       &mbr.is_pruned);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_remote_lag_member_count(const SessionHandle_t sess_hdl,
                                      const int dev,
                                      const int8_t lag,
                                      const int32_t left,
                                      const int32_t right) {
    int32_t status =
        p4_pd_mc_set_remote_lag_member_count(sess_hdl, dev, lag, left, right);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_update_port_prune_table(const SessionHandle_t sess_hdl,
                                  const int dev,
                                  const int16_t yid,
                                  const std::string &port_map) {
    int32_t status = p4_pd_mc_update_port_prune_table(
        sess_hdl, dev, yid, (uint8_t *)port_map.c_str());
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_get_port_prune_table(std::string &_return,
                               const ::res_pd_rpc::SessionHandle_t sess_hdl,
                               const int32_t dev_id,
                               const int16_t yid,
                               const bool from_hw) {
    unsigned char values[BF_MC_PORT_ARRAY_SIZE];
    int32_t status =
        p4_pd_mc_get_port_prune_table(sess_hdl, dev_id, yid, values, from_hw);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.assign((char *)values, sizeof values);
  }

  void mc_set_global_rid(const SessionHandle_t sess_hdl,
                         const int dev,
                         const int16_t rid) {
    int32_t status = p4_pd_mc_set_global_rid(sess_hdl, dev, rid);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_port_mc_fwd_state(const SessionHandle_t sess_hdl,
                                const int dev,
                                const int16_t port,
                                const int8_t is_active) {
    int status = p4_pd_mc_set_port_mc_fwd_state(sess_hdl, dev, port, is_active);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_enable_port_ff(const SessionHandle_t sess_hdl, const int dev) {
    int status = p4_pd_mc_enable_port_ff(sess_hdl, dev);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_disable_port_ff(const SessionHandle_t sess_hdl, const int dev) {
    int status = p4_pd_mc_disable_port_ff(sess_hdl, dev);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_clr_port_ff_state(const SessionHandle_t sess_hdl,
                            const int dev,
                            const int16_t port) {
    int status = p4_pd_mc_clr_port_ff_state(sess_hdl, dev, port);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_enable_port_protection(const SessionHandle_t sess_hdl,
                                 const int dev) {
    int status = p4_pd_mc_enable_port_protection(sess_hdl, dev);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }
  void mc_disable_port_protection(const SessionHandle_t sess_hdl,
                                  const int dev) {
    int status = p4_pd_mc_disable_port_protection(sess_hdl, dev);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_port_protection(const SessionHandle_t sess_hdl,
                              const int dev,
                              const int16_t pport,
                              const int16_t bport) {
    int status = p4_pd_mc_set_port_protection(sess_hdl, dev, pport, bport);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_clear_port_protection(const SessionHandle_t sess_hdl,
                                const int dev,
                                const int16_t pport) {
    int status = p4_pd_mc_clear_port_protection(sess_hdl, dev, pport);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_max_nodes_before_yield(const SessionHandle_t sess_hdl,
                                     const int dev,
                                     const int count) {
    int status = p4_pd_mc_set_max_nodes_before_yield(sess_hdl, dev, count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mc_set_max_node_threshold(const SessionHandle_t sess_hdl,
                                 const int32_t dev,
                                 const int32_t node_count,
                                 const int32_t lag_count) {
    int status =
        p4_pd_mc_set_max_node_threshold(sess_hdl, dev, node_count, lag_count);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int32_t mc_get_pipe_vec(const SessionHandle_t sess_hdl,
                          const int32_t dev_id,
                          const int16_t mgid) {
    int ret = 0;
    int status = p4_pd_mc_get_pipe_vector(sess_hdl, dev_id, mgid, &ret);
    if (status != 0) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return ret;
  }
};
