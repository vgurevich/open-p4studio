
#include "gen-cpp/pipemgr_api.h"

extern "C" {
#include <bf_types/bf_types.h>
#include <pipe_mgr/bf_packetpath_counter.h>
}

using namespace ::pipemgr_api_rpc;
using namespace ::res_pd_rpc;

#define PP_THROW_STATUS()     \
  {                           \
    if (status != 0) {        \
      InvalidPpOperation top; \
      top.code = status;      \
      throw top;              \
    }                         \
  }

class pipemgr_apiHandler : virtual public pipemgr_apiIf {
 private:
  void copy_ibp_cntrs(pp_bf_ibp_drop_cntrs &pp_ibp_cntrs,
                      bf_ibp_cntrs_t *pd_ibp) {
    pp_ibp_cntrs.total_pkts_drop = pd_ibp->total_pkts_drop;
    pp_ibp_cntrs.total_pkts_disc = pd_ibp->total_pkts_disc;
    pp_ibp_cntrs.total_recirc_pkt_disc = pd_ibp->total_recirc_pkt_disc;
    pp_ibp_cntrs.total_prsr_pkt_disc = pd_ibp->total_prsr_pkt_disc;
  }

  void copy_iprsr_cntrs(pp_bf_iprsr_drop_cntrs &pp_iprsr_cntrs,
                        bf_iprsr_cntrs_t *pd_iprsr) {
    pp_iprsr_cntrs.fcs_err_count = pd_iprsr->fcs_err_count;
    pp_iprsr_cntrs.csum_err_count = pd_iprsr->csum_err_count;
    pp_iprsr_cntrs.tcam_parity_err_count = pd_iprsr->tcam_parity_err_count;
    pp_iprsr_cntrs.total_pkts_drop = pd_iprsr->total_pkts_drop;
  }

  void copy_idprsr_cntrs(pp_bf_idprsr_drop_cntrs &pp_idprsr_cntrs,
                         bf_idprsr_cntrs_t *pd_idprsr) {
    pp_idprsr_cntrs.crc_err.reserve(72);

    pp_idprsr_cntrs.pkts_disc = pd_idprsr->pkts_disc;
    pp_idprsr_cntrs.pkts_disc_at_tm = pd_idprsr->pkts_disc_at_tm;
    pp_idprsr_cntrs.err_pkts_to_tm = pd_idprsr->err_pkts_to_tm;
    pp_idprsr_cntrs.err_pkts_to_ictm = pd_idprsr->err_pkts_to_ictm;

    for (int i = 0; i < 72; i++) {
      pp_idprsr_cntrs.crc_err.push_back(pd_idprsr->crc_err[i]);
    }
  }

  void copy_eprsr_cntrs(pp_bf_eprsr_drop_cntrs &pp_eprsr_cntrs,
                        bf_eprsr_cntrs_t *pd_eprsr) {
    pp_eprsr_cntrs.fcs_err_count = pd_eprsr->fcs_err_count;
    pp_eprsr_cntrs.csum_err_count = pd_eprsr->csum_err_count;
    pp_eprsr_cntrs.tcam_parity_err_count = pd_eprsr->tcam_parity_err_count;
    pp_eprsr_cntrs.total_pkts_drop = pd_eprsr->total_pkts_drop;
  }

  void copy_edprsr_cntrs(pp_bf_edprsr_drop_cntrs &pp_edprsr_cntrs,
                         bf_edprsr_cntrs_t *pd_edprsr) {
    pp_edprsr_cntrs.crc_err.reserve(72);

    pp_edprsr_cntrs.pkts_disc = pd_edprsr->pkts_disc;
    pp_edprsr_cntrs.err_pkts_to_ebuf = pd_edprsr->err_pkts_to_ebuf;
    pp_edprsr_cntrs.err_pkts_to_ectm = pd_edprsr->err_pkts_to_ectm;

    for (int i = 0; i < 72; i++) {
      pp_edprsr_cntrs.crc_err.push_back(pd_edprsr->crc_err[i]);
    }
  }

 public:
  pipemgr_apiHandler() {}

  //////////////////   PIPE MGR PACKET PATH APIs ///////////////////////////

  void pp_ibp_drop_cntr_get(pp_bf_ibp_drop_cntrs &_return,
                            const pp_dev_t dev,
                            const pp_port_t port) {
    (void)_return;
    pipe_status_t status;
    bf_ibp_cntrs_t ibp_cntrs;

    status = bf_pkt_path_ibp_drop_cntr_get(dev, port, &ibp_cntrs);
    PP_THROW_STATUS();
    copy_ibp_cntrs(_return, &ibp_cntrs);
  }

  void pp_iprsr_drop_cntr_get(pp_bf_iprsr_drop_cntrs &_return,
                              const pp_dev_t dev,
                              const pp_port_t port) {
    (void)_return;
    pipe_status_t status;
    bf_iprsr_cntrs_t iprsr_cntrs;

    status = bf_pkt_path_iprsr_drop_cntr_get(dev, port, &iprsr_cntrs);
    PP_THROW_STATUS();
    copy_iprsr_cntrs(_return, &iprsr_cntrs);
  }
  void pp_idprsr_drop_cntr_get(pp_bf_idprsr_drop_cntrs &_return,
                               const pp_dev_t dev,
                               const pp_pipe_t pipe) {
    (void)_return;
    pipe_status_t status;
    bf_idprsr_cntrs_t idprsr_cntrs;

    status = bf_pkt_path_idprsr_drop_cntr_get(dev, pipe, &idprsr_cntrs);
    PP_THROW_STATUS();
    copy_idprsr_cntrs(_return, &idprsr_cntrs);
  }
  void pp_eprsr_drop_cntr_get(pp_bf_eprsr_drop_cntrs &_return,
                              const pp_dev_t dev,
                              const pp_port_t port) {
    (void)_return;
    pipe_status_t status;
    bf_eprsr_cntrs_t eprsr_cntrs;

    status = bf_pkt_path_eprsr_drop_cntr_get(dev, port, &eprsr_cntrs);
    PP_THROW_STATUS();
    copy_eprsr_cntrs(_return, &eprsr_cntrs);
  }
  void pp_edprsr_drop_cntr_get(pp_bf_edprsr_drop_cntrs &_return,
                               const pp_dev_t dev,
                               const pp_pipe_t pipe) {
    (void)_return;
    pipe_status_t status;
    bf_edprsr_cntrs_t edprsr_cntrs;

    status = bf_pkt_path_edprsr_drop_cntr_get(dev, pipe, &edprsr_cntrs);
    PP_THROW_STATUS();
    copy_edprsr_cntrs(_return, &edprsr_cntrs);
  }
};
