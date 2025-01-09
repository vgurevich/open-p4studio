
#include "gen-cpp/tm.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_tm.h>
#include <traffic_mgr/tm_intf.h>
}
#include <memory>

using namespace ::tm_api_rpc;
using namespace ::res_pd_rpc;

#define TM_THROW_STATUS()     \
  {                           \
    if (status != 0) {        \
      InvalidTmOperation top; \
      top.code = status;      \
      throw top;              \
    }                         \
  }

class tmHandler : virtual public tmIf {
 private:
  void copy_blklvl_cntrs(tm_blklvl_cntrs &blklvl_cntrs,
                         p4_pd_tm_blklvl_cntrs_t *pd_blklvl_cntrs) {
    blklvl_cntrs.wac_no_dest_drop = pd_blklvl_cntrs->wac_no_dest_drop;
    blklvl_cntrs.qac_no_dest_drop = pd_blklvl_cntrs->qac_no_dest_drop;
    blklvl_cntrs.wac_buf_full_drop = pd_blklvl_cntrs->wac_buf_full_drop;
    blklvl_cntrs.egress_pipe_total_drop =
        pd_blklvl_cntrs->egress_pipe_total_drop;
    blklvl_cntrs.psc_pkt_drop = pd_blklvl_cntrs->psc_pkt_drop;
    blklvl_cntrs.pex_total_disc = pd_blklvl_cntrs->pex_total_disc;
    blklvl_cntrs.qac_total_disc = pd_blklvl_cntrs->qac_total_disc;
    blklvl_cntrs.total_disc_dq = pd_blklvl_cntrs->total_disc_dq;
    blklvl_cntrs.pre_total_drop = pd_blklvl_cntrs->pre_total_drop;
    blklvl_cntrs.qac_pre_mc_drop = pd_blklvl_cntrs->qac_pre_mc_drop;
  }

  void copy_pre_fifo_cntrs(tm_pre_fifo_cntrs &pre_fifo_cntrs,
                           p4_pd_tm_pre_fifo_cntrs_t *pd_pre_fifo_cntrs) {
    pre_fifo_cntrs.wac_drop_cnt_pre0_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre1_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre2_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre3_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre4_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre5_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre6_fifo.reserve(BF_PRE_FIFO_COUNT);
    pre_fifo_cntrs.wac_drop_cnt_pre7_fifo.reserve(BF_PRE_FIFO_COUNT);

    for (int i = 0; i < BF_PRE_FIFO_COUNT; i++) {
      pre_fifo_cntrs.wac_drop_cnt_pre0_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre0_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre1_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre1_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre2_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre2_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre3_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre3_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre4_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre4_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre5_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre5_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre6_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre6_fifo[i]);
      pre_fifo_cntrs.wac_drop_cnt_pre7_fifo.push_back(
          pd_pre_fifo_cntrs->wac_drop_cnt_pre7_fifo[i]);
    }
  }

 public:
  tmHandler() {}

  //////////////////   PPG  APIs ///////////////////////////

  tm_ppg_t tm_allocate_ppg(const tm_dev_t dev, const tm_port_t port) {
    p4_pd_tm_ppg_t ppg;
    int status = p4_pd_tm_allocate_ppg(dev, port, &ppg);
    TM_THROW_STATUS();
    return ((uint32_t)ppg);
  }

  void tm_free_ppg(const tm_dev_t dev, const tm_ppg_t ppg) {
    int status = p4_pd_tm_free_ppg(dev, ppg);
    TM_THROW_STATUS();
  }

  tm_ppg_t tm_get_default_ppg(const tm_dev_t dev, const tm_port_t port) {
    p4_pd_tm_ppg_t ppg;
    int status = p4_pd_tm_get_default_ppg(dev, port, &ppg);
    TM_THROW_STATUS();
    return ((uint32_t)ppg);
  }

  void tm_set_ppg_icos_mapping(const tm_dev_t dev,
                               const tm_ppg_t ppg,
                               const tm_icos_t icos_map) {
    int status = p4_pd_tm_set_ppg_icos_mapping(dev, ppg, icos_map);
    TM_THROW_STATUS();
  }

  void tm_enable_lossless_treatment(const tm_dev_t dev, const tm_ppg_t ppg) {
    int status = p4_pd_tm_enable_lossless_treatment(dev, ppg);
    TM_THROW_STATUS();
  }

  void tm_disable_lossless_treatment(const tm_dev_t dev, const tm_ppg_t ppg) {
    int status = p4_pd_tm_disable_lossless_treatment(dev, ppg);
    TM_THROW_STATUS();
  }

  void tm_set_ppg_app_pool_usage(const tm_dev_t dev,
                                 const tm_ppg_t ppg,
                                 const tm_pool_t pool,
                                 const tm_limit_t base_use_limit,
                                 const tm_ppg_baf_t dynamic_baf,
                                 const tm_limit_t hysteresis) {
    int status =
        p4_pd_tm_set_ppg_app_pool_usage(dev,
                                        ppg,
                                        (p4_pd_pool_id_t)pool,
                                        base_use_limit,
                                        (p4_pd_tm_ppg_baf_t)dynamic_baf,
                                        hysteresis);
    TM_THROW_STATUS();
  }

  void tm_disable_ppg_app_pool_usage(const tm_dev_t dev,
                                     const tm_pool_t pool,
                                     const tm_ppg_t ppg) {
    int status =
        p4_pd_tm_disable_ppg_app_pool_usage(dev, (p4_pd_pool_id_t)pool, ppg);
    TM_THROW_STATUS();
  }

  void tm_set_ppg_guaranteed_min_limit(const tm_dev_t dev,
                                       const tm_ppg_t ppg,
                                       const tm_limit_t cells) {
    int status = p4_pd_tm_set_ppg_guaranteed_min_limit(dev, ppg, cells);
    TM_THROW_STATUS();
  }

  void tm_set_ppg_skid_limit(const tm_dev_t dev,
                             const tm_ppg_t ppg,
                             const tm_limit_t cells) {
    int status = p4_pd_tm_set_ppg_skid_limit(dev, ppg, cells);
    TM_THROW_STATUS();
  }

  void tm_set_guaranteed_min_skid_hysteresis(const tm_dev_t dev,
                                             const tm_ppg_t ppg,
                                             const tm_limit_t cells) {
    int status = p4_pd_tm_set_guaranteed_min_skid_hysteresis(dev, ppg, cells);
    TM_THROW_STATUS();
  }

  //////////////////   Q  APIs ///////////////////////////

  void tm_set_port_q_mapping(const tm_dev_t dev,
                             const tm_port_t port,
                             const int16_t q_count,
                             const tm_q_map_t &q_map) {
    uint8_t q_mapping[32];

    q_mapping[0] = q_map.md_qid0_to_tm_q;
    q_mapping[1] = q_map.md_qid1_to_tm_q;
    q_mapping[2] = q_map.md_qid2_to_tm_q;
    q_mapping[3] = q_map.md_qid3_to_tm_q;
    q_mapping[4] = q_map.md_qid4_to_tm_q;
    q_mapping[5] = q_map.md_qid5_to_tm_q;
    q_mapping[6] = q_map.md_qid6_to_tm_q;
    q_mapping[7] = q_map.md_qid7_to_tm_q;
    q_mapping[8] = q_map.md_qid8_to_tm_q;
    q_mapping[9] = q_map.md_qid9_to_tm_q;
    q_mapping[10] = q_map.md_qid10_to_tm_q;
    q_mapping[11] = q_map.md_qid11_to_tm_q;
    q_mapping[12] = q_map.md_qid12_to_tm_q;
    q_mapping[13] = q_map.md_qid13_to_tm_q;
    q_mapping[14] = q_map.md_qid14_to_tm_q;
    q_mapping[15] = q_map.md_qid15_to_tm_q;
    q_mapping[16] = q_map.md_qid16_to_tm_q;
    q_mapping[17] = q_map.md_qid17_to_tm_q;
    q_mapping[18] = q_map.md_qid18_to_tm_q;
    q_mapping[19] = q_map.md_qid19_to_tm_q;
    q_mapping[20] = q_map.md_qid20_to_tm_q;
    q_mapping[21] = q_map.md_qid21_to_tm_q;
    q_mapping[22] = q_map.md_qid22_to_tm_q;
    q_mapping[23] = q_map.md_qid23_to_tm_q;
    q_mapping[24] = q_map.md_qid24_to_tm_q;
    q_mapping[25] = q_map.md_qid25_to_tm_q;
    q_mapping[26] = q_map.md_qid26_to_tm_q;
    q_mapping[27] = q_map.md_qid27_to_tm_q;
    q_mapping[28] = q_map.md_qid28_to_tm_q;
    q_mapping[29] = q_map.md_qid29_to_tm_q;
    q_mapping[30] = q_map.md_qid30_to_tm_q;
    q_mapping[31] = q_map.md_qid31_to_tm_q;
    int status = p4_pd_tm_set_port_q_mapping(dev, port, q_count, q_mapping);
    TM_THROW_STATUS();
  }

  void tm_set_port_q_mapping_adv(const tm_dev_t dev,
                                 const tm_port_t port,
                                 const int16_t q_count,
                                 const tm_q_map_adv_t &q_map) {
    uint8_t *q_mapping = new uint8_t[q_map.size()];
    if (!q_mapping) {
      int status = BF_NO_SYS_RESOURCES;
      TM_THROW_STATUS();
    }

    for (uint i = 0; i < q_map.size(); ++i) {
      q_mapping[i] = q_map[i];
    }
    int status = p4_pd_tm_set_port_q_mapping(dev, port, q_count, q_mapping);
    delete[] q_mapping;
    TM_THROW_STATUS();
  }

  void tm_set_q_app_pool_usage(const tm_dev_t dev,
                               const tm_port_t port,
                               const tm_q_t q,
                               const tm_pool_t pool,
                               const tm_limit_t base_use_limit,
                               const tm_q_baf_t dynamic_baf,
                               const tm_limit_t hysteresis) {
    int status =
        p4_pd_tm_set_q_app_pool_usage(dev,
                                      port,
                                      q,
                                      (p4_pd_pool_id_t)pool,
                                      base_use_limit,
                                      (p4_pd_tm_queue_baf_t)dynamic_baf,
                                      hysteresis);
    TM_THROW_STATUS();
  }

  void tm_disable_q_app_pool_usage(const tm_dev_t dev,
                                   const tm_port_t port,
                                   const tm_q_t q) {
    int status = p4_pd_tm_disable_q_app_pool_usage(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_set_q_guaranteed_min_limit(const tm_dev_t dev,
                                     const tm_port_t port,
                                     const tm_q_t q,
                                     const tm_limit_t cells) {
    int status = p4_pd_tm_set_q_guaranteed_min_limit(dev, port, q, cells);
    TM_THROW_STATUS();
  }

  void tm_set_q_color_limit(const tm_dev_t dev,
                            const tm_port_t port,
                            const tm_q_t q,
                            const tm_color_t color,
                            const tm_limit_t cells) {
    int status = p4_pd_tm_set_q_color_limit(
        dev, port, q, (p4_pd_color_t)color, (p4_pd_color_limit_t)cells);
    TM_THROW_STATUS();
  }

  void tm_set_q_color_hysteresis(const tm_dev_t dev,
                                 const tm_port_t port,
                                 const tm_q_t q,
                                 const tm_color_t color,
                                 const tm_cells_t cells) {
    int status = p4_pd_tm_set_q_color_hysteresis(
        dev, port, q, (p4_pd_color_t)color, cells);
    TM_THROW_STATUS();
  }

  void tm_enable_q_tail_drop(const tm_dev_t dev,
                             const tm_port_t port,
                             const tm_q_t q) {
    int status = p4_pd_tm_enable_q_tail_drop(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_disable_q_tail_drop(const tm_dev_t dev,
                              const tm_port_t port,
                              const tm_q_t q) {
    int status = p4_pd_tm_disable_q_tail_drop(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_enable_q_color_drop(const tm_dev_t dev,
                              const tm_port_t port,
                              const tm_q_t q) {
    int status = p4_pd_tm_enable_q_color_drop(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_disable_q_color_drop(const tm_dev_t dev,
                               const tm_port_t port,
                               const tm_q_t q) {
    int status = p4_pd_tm_disable_q_color_drop(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_set_negative_mirror_dest(const tm_dev_t dev,
                                   const tm_pipe_t pipe,
                                   const tm_port_t port,
                                   const tm_q_t q) {
    int status = p4_pd_tm_set_negative_mirror_dest(dev, pipe, port, q);
    TM_THROW_STATUS();
  }

  void tm_get_negative_mirror_dest(tm_neg_mirror_dest &_return,
                                   const tm_dev_t dev,
                                   const tm_pipe_t pipe) {
    p4_pd_tm_port_t port;
    p4_pd_tm_queue_t queue;
    int status = p4_pd_tm_get_negative_mirror_dest(dev, pipe, &port, &queue);
    TM_THROW_STATUS();
    _return.port = port;
    _return.queue = queue;
  }

  void tm_set_q_pfc_cos_mapping(const tm_dev_t dev,
                                const tm_port_t port,
                                const tm_q_t q,
                                const tm_icos_t cos) {
    int status = p4_pd_tm_set_q_pfc_cos_mapping(dev, port, q, cos);
    TM_THROW_STATUS();
  }

  void tm_q_visible_set(const tm_dev_t dev,
                        const tm_port_t port,
                        const tm_q_t q,
                        const bool visible) {
    int status = p4_pd_tm_q_visible_set(dev, port, q, visible);
    TM_THROW_STATUS();
  }

  ///////////////// POOL APIS //////////////////

  void tm_set_app_pool_size(const tm_dev_t dev,
                            const tm_pool_t pool,
                            const tm_cells_t cells) {
    int status = p4_pd_tm_set_app_pool_size(dev, (p4_pd_pool_id_t)pool, cells);
    TM_THROW_STATUS();
  }

  void tm_enable_app_pool_color_drop(const tm_dev_t dev, const tm_pool_t pool) {
    int status =
        p4_pd_tm_enable_app_pool_color_drop(dev, (p4_pd_pool_id_t)pool);
    TM_THROW_STATUS();
  }

  void tm_disable_app_pool_color_drop(const tm_dev_t dev,
                                      const tm_pool_t pool) {
    int status =
        p4_pd_tm_disable_app_pool_color_drop(dev, (p4_pd_pool_id_t)pool);
    TM_THROW_STATUS();
  }

  void tm_set_app_pool_color_drop_limit(const tm_dev_t dev,
                                        const tm_pool_t pool,
                                        const tm_color_t color,
                                        const tm_cells_t cells) {
    int status = p4_pd_tm_set_app_pool_color_drop_limit(
        dev, (p4_pd_pool_id_t)pool, (p4_pd_color_t)color, cells);
    TM_THROW_STATUS();
  }

  void tm_set_app_pool_color_drop_hysteresis(const tm_dev_t dev,
                                             const tm_color_t color,
                                             const tm_cells_t cells) {
    int status = p4_pd_tm_set_app_pool_color_drop_hysteresis(
        dev, (p4_pd_color_t)color, cells);
    TM_THROW_STATUS();
  }

  void tm_set_app_pool_pfc_limit(const tm_dev_t dev,
                                 const tm_pool_t pool,
                                 const tm_icos_t icos,
                                 const tm_cells_t cells) {
    int status = p4_pd_tm_set_app_pool_pfc_limit(
        dev, (p4_pd_pool_id_t)pool, icos, cells);
    TM_THROW_STATUS();
  }

  void tm_set_skid_pool_size(const tm_dev_t dev, const tm_cells_t cells) {
    int status = p4_pd_tm_set_skid_pool_size(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_set_skid_pool_hysteresis(const tm_dev_t dev, const tm_cells_t cells) {
    int status = p4_pd_tm_set_skid_pool_hysteresis(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_set_negative_mirror_pool_size(const tm_dev_t dev,
                                        const tm_cells_t cells) {
    int status = p4_pd_tm_set_negative_mirror_pool_size(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_set_uc_cut_through_pool_size(const tm_dev_t dev,
                                       const tm_cells_t cells) {
    int status = p4_pd_tm_set_uc_cut_through_pool_size(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_set_mc_cut_through_pool_size(const tm_dev_t dev,
                                       const tm_cells_t cells) {
    int status = p4_pd_tm_set_mc_cut_through_pool_size(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_set_ingress_buffer_limit(const tm_dev_t dev,
                                      const tm_cells_t cells) {
    int status = p4_pd_tm_set_ingress_buffer_limit(dev, cells);
    TM_THROW_STATUS();
  }

  void tm_enable_ingress_buffer_limit(const tm_dev_t dev) {
    int status = p4_pd_tm_enable_ingress_buffer_limit(dev);
    TM_THROW_STATUS();
  }

  void tm_disable_ingress_buffer_limit(const tm_dev_t dev) {
    int status = p4_pd_tm_disable_ingress_buffer_limit(dev);
    TM_THROW_STATUS();
  }

  void tm_set_egress_pipe_limit(const tm_dev_t dev,
                                const tm_pipe_t pipe,
                                const tm_cells_t cells) {
    int status = p4_pd_tm_set_egress_pipe_limit(dev, pipe, cells);
    TM_THROW_STATUS();
  }

  void tm_set_egress_pipe_hysteresis(const tm_dev_t dev,
                                     const tm_pipe_t pipe,
                                     const tm_cells_t cells) {
    int status = p4_pd_tm_set_egress_pipe_hysteresis(dev, pipe, cells);
    TM_THROW_STATUS();
  }

  void tm_qstat_report_mode_set(const tm_dev_t dev,
                                const tm_pipe_t pipe,
                                const bool mode) {
    int status = p4_pd_tm_qstat_report_mode_set(dev, pipe, mode);
    TM_THROW_STATUS();
  }

  void tm_pipe_deflection_port_enable_set(const tm_dev_t dev,
                                          const tm_pipe_t pipe,
                                          const bool enable) {
    int status = pd_tm_pipe_deflection_port_enable_set(dev, pipe, enable);
    TM_THROW_STATUS();
  }

  bool tm_pipe_deflection_port_enable_get(const tm_dev_t dev,
                                          const tm_pipe_t pipe) {
    bool enable;
    int status = pd_tm_pipe_deflection_port_enable_get(dev, pipe, &enable);
    TM_THROW_STATUS();
    return enable;
  }

  ///////////////// PORT APIS //////////////////

  void tm_set_ingress_port_drop_limit(const tm_dev_t dev,
                                      const tm_port_t port,
                                      const tm_cells_t cells) {
    int status = p4_pd_tm_set_ingress_port_drop_limit(dev, port, cells);
    TM_THROW_STATUS();
  }

  void tm_set_egress_port_drop_limit(const tm_dev_t dev,
                                     const tm_port_t port,
                                     const tm_cells_t cells) {
    int status = p4_pd_tm_set_egress_port_drop_limit(dev, port, cells);
    TM_THROW_STATUS();
  }

  void tm_set_ingress_port_hysteresis(const tm_dev_t dev,
                                      const tm_port_t port,
                                      const tm_cells_t cells) {
    int status = p4_pd_tm_set_ingress_port_hysteresis(dev, port, cells);
    TM_THROW_STATUS();
  }

  void tm_set_egress_port_hysteresis(const tm_dev_t dev,
                                     const tm_port_t port,
                                     const tm_cells_t cells) {
    int status = p4_pd_tm_set_egress_port_hysteresis(dev, port, cells);
    TM_THROW_STATUS();
  }

  void tm_set_port_uc_cut_through_limit(const tm_dev_t dev,
                                        const tm_port_t port,
                                        const tm_cells_t cells) {
    int status = p4_pd_tm_set_port_uc_cut_through_limit(dev, port, cells);
    TM_THROW_STATUS();
  }

  void tm_set_port_flowcontrol_mode(const tm_dev_t dev,
                                    const tm_port_t port,
                                    const tm_flow_ctrl_t fctype) {
    int status = p4_pd_tm_set_port_flowcontrol_mode(
        dev, port, (p4_pd_tm_flow_ctrl_type_t)fctype);
    TM_THROW_STATUS();
  }

  void tm_set_port_pfc_cos_mapping(const tm_dev_t dev,
                                   const tm_port_t port,
                                   const tm_pfc_cos_map_t &cos_icos_map) {
    uint8_t cos_map[8];
    cos_map[0] = cos_icos_map.CoS0_to_iCos;
    cos_map[1] = cos_icos_map.CoS1_to_iCos;
    cos_map[2] = cos_icos_map.CoS2_to_iCos;
    cos_map[3] = cos_icos_map.CoS3_to_iCos;
    cos_map[4] = cos_icos_map.CoS4_to_iCos;
    cos_map[5] = cos_icos_map.CoS5_to_iCos;
    cos_map[6] = cos_icos_map.CoS6_to_iCos;
    cos_map[7] = cos_icos_map.CoS7_to_iCos;
    int status = p4_pd_tm_set_port_pfc_cos_mapping(dev, port, cos_map);
    TM_THROW_STATUS();
  }

  void tm_set_cpuport(const tm_dev_t dev, const tm_port_t port) {
    int status = p4_pd_tm_set_cpuport(dev, port);
    TM_THROW_STATUS();
  }

  void tm_reset_cpuport(const tm_dev_t dev) {
    int status = p4_pd_tm_reset_cpuport(dev);
    TM_THROW_STATUS();
  }

  ///////////////// SCH APIS //////////////////

  void tm_set_q_sched_priority(const tm_dev_t dev,
                               const tm_port_t port,
                               const tm_q_t q,
                               const tm_sched_prio_t prio) {
    int status = p4_pd_tm_set_q_sched_priority(dev, port, q, (uint16_t)prio);
    TM_THROW_STATUS();
  }

  void tm_set_q_dwrr_weight(const tm_dev_t dev,
                            const tm_port_t port,
                            const tm_q_t q,
                            const int16_t weight) {
    int status = p4_pd_tm_set_q_dwrr_weight(dev, port, q, weight);
    TM_THROW_STATUS();
  }

  void tm_set_q_shaping_rate(const tm_dev_t dev,
                             const tm_port_t port,
                             const tm_q_t q,
                             const bool pps,
                             const tm_limit_t burstsize,
                             const tm_limit_t rate) {
    int status =
        p4_pd_tm_set_q_shaping_rate(dev, port, q, pps, burstsize, rate);
    TM_THROW_STATUS();
  }

  void tm_set_q_shaping_rate_provisioning(
      const tm_dev_t dev,
      const tm_port_t port,
      const tm_q_t q,
      const bool pps,
      const tm_limit_t burstsize,
      const tm_limit_t rate,
      const tm_sched_shaper_provisioning_type_t::type prov_type) {
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type =
        (p4_pd_tm_sched_shaper_prov_type_t)prov_type;

    int status = p4_pd_tm_set_q_shaping_rate_provisioning(
        dev, port, q, pps, burstsize, rate, pd_prov_type);
    TM_THROW_STATUS();
  }

  void tm_set_q_guaranteed_rate(const tm_dev_t dev,
                                const tm_port_t port,
                                const tm_q_t q,
                                const bool pps,
                                const tm_limit_t burstsize,
                                const tm_limit_t rate) {
    int status =
        p4_pd_tm_set_q_guaranteed_rate(dev, port, q, pps, burstsize, rate);
    TM_THROW_STATUS();
  }

  void tm_set_q_remaining_bw_sched_priority(const tm_dev_t dev,
                                            const tm_port_t port,
                                            const tm_q_t q,
                                            const tm_sched_prio_t prio) {
    int status = p4_pd_tm_set_q_remaining_bw_sched_priority(
        dev, port, q, (uint16_t)prio);
    TM_THROW_STATUS();
  }

  void tm_sched_q_l1_set(tm_dev_t dev,
                         tm_port_t port,
                         tm_l1_node_t l1_node,
                         tm_q_t q) {
    int status = p4_pd_tm_sched_q_l1_set(dev, port, l1_node, q);
    TM_THROW_STATUS();
  }

  void tm_sched_q_l1_reset(tm_dev_t dev, tm_port_t port, tm_q_t q) {
    int status = p4_pd_tm_sched_q_l1_reset(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_priority_set(tm_dev_t dev,
                                tm_port_t port,
                                tm_l1_node_t l1_node,
                                tm_sched_prio_t priority) {
    int status =
        p4_pd_tm_sched_l1_priority_set(dev, port, l1_node, (uint16_t)priority);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_dwrr_weight_set(tm_dev_t dev,
                                   tm_port_t port,
                                   tm_l1_node_t l1_node,
                                   int16_t weight) {
    int status =
        p4_pd_tm_sched_l1_dwrr_weight_set(dev, port, l1_node, (uint16_t)weight);
    TM_THROW_STATUS();
  }

  int16_t tm_sched_l1_dwrr_weight_get(tm_dev_t dev,
                                      tm_port_t port,
                                      tm_l1_node_t l1_node) {
    uint16_t weight;
    int status = p4_pd_tm_sched_l1_dwrr_weight_get(dev, port, l1_node, &weight);
    TM_THROW_STATUS();
    return weight;
  }

  void tm_sched_l1_shaping_rate_set(tm_dev_t dev,
                                    tm_port_t port,
                                    tm_l1_node_t l1_node,
                                    bool pps,
                                    int32_t burst_size,
                                    int32_t rate) {
    int status = p4_pd_tm_sched_l1_shaping_rate_set(
        dev, port, l1_node, pps, (uint32_t)burst_size, (uint32_t)rate);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_max_shaping_rate_enable(tm_dev_t dev,
                                           tm_port_t port,
                                           tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_max_shaping_rate_enable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_max_shaping_rate_disable(tm_dev_t dev,
                                            tm_port_t port,
                                            tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_max_shaping_rate_disable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_priority_prop_enable(tm_dev_t dev,
                                        tm_port_t port,
                                        tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_priority_prop_enable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_priority_prop_disable(tm_dev_t dev,
                                         tm_port_t port,
                                         tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_priority_prop_disable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_guaranteed_rate_set(tm_dev_t dev,
                                       tm_port_t port,
                                       tm_l1_node_t l1_node,
                                       bool pps,
                                       int32_t burst_size,
                                       int32_t rate) {
    int status = p4_pd_tm_sched_l1_guaranteed_rate_set(
        dev, port, l1_node, pps, (uint32_t)burst_size, (uint32_t)rate);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_remaining_bw_priority_set(tm_dev_t dev,
                                             tm_port_t port,
                                             tm_l1_node_t l1_node,
                                             tm_sched_prio_t priority) {
    int status = p4_pd_tm_sched_l1_remaining_bw_priority_set(
        dev, port, l1_node, (uint16_t)priority);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_guaranteed_rate_enable(tm_dev_t dev,
                                          tm_port_t port,
                                          tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_guaranteed_rate_enable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_guaranteed_rate_disable(tm_dev_t dev,
                                           tm_port_t port,
                                           tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_guaranteed_rate_disable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_enable(tm_dev_t dev, tm_port_t port, tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_enable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_disable(tm_dev_t dev, tm_port_t port, tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_disable(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_sched_l1_free(tm_dev_t dev, tm_port_t port, tm_l1_node_t l1_node) {
    int status = p4_pd_tm_sched_l1_free(dev, port, l1_node);
    TM_THROW_STATUS();
  }

  void tm_set_port_shaping_rate(const tm_dev_t dev,
                                const tm_port_t port,
                                const bool pps,
                                const tm_limit_t burstsize,
                                const tm_limit_t rate) {
    int status =
        p4_pd_tm_set_port_shaping_rate(dev, port, pps, burstsize, rate);
    TM_THROW_STATUS();
  }

  void tm_set_port_shaping_rate_provisioning(
      const tm_dev_t dev,
      const tm_port_t port,
      const bool pps,
      const tm_limit_t burstsize,
      const tm_limit_t rate,
      const tm_sched_shaper_provisioning_type_t::type prov_type) {
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type =
        (p4_pd_tm_sched_shaper_prov_type_t)prov_type;

    int status = p4_pd_tm_set_port_shaping_rate_provisioning(
        dev, port, pps, burstsize, rate, pd_prov_type);
    TM_THROW_STATUS();
  }

  void tm_set_shaper_pkt_ifg_compensation(const tm_dev_t dev,
                                          const tm_pipe_t pipe,
                                          const int16_t adjustment) {
    int status =
        p4_pd_tm_set_shaper_pkt_ifg_compensation(dev, pipe, adjustment);
    TM_THROW_STATUS();
  }

  void tm_enable_q_sched(const tm_dev_t dev,
                         const tm_port_t port,
                         const tm_q_t q) {
    int status = p4_pd_tm_enable_q_sched(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_disable_q_sched(const tm_dev_t dev,
                          const tm_port_t port,
                          const tm_q_t q) {
    int status = p4_pd_tm_disable_q_sched(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_enable_port_shaping(const tm_dev_t dev, const tm_port_t port) {
    int status = p4_pd_tm_enable_port_shaping(dev, port);
    TM_THROW_STATUS();
  }

  void tm_disable_port_shaping(const tm_dev_t dev, const tm_port_t port) {
    int status = p4_pd_tm_disable_port_shaping(dev, port);
    TM_THROW_STATUS();
  }

  void tm_enable_port_sched(const tm_dev_t dev,
                            const tm_port_t port,
                            tm_port_speed_t port_speed) {
    int status = p4_pd_tm_enable_port_sched(dev, port, port_speed);
    TM_THROW_STATUS();
  }

  void tm_disable_port_sched(const tm_dev_t dev, const tm_port_t port) {
    int status = p4_pd_tm_disable_port_sched(dev, port);
    TM_THROW_STATUS();
  }

  // misc APIs

  void tm_complete_operations(const tm_dev_t dev) {
    p4_pd_tm_complete_operations(dev);
  }

  void tm_set_timestamp_shift(const tm_dev_t dev, const int16_t shift) {
    uint8_t _shift = (uint8_t)shift;
    int status = p4_pd_tm_set_timestamp_shift(dev, _shift);
    TM_THROW_STATUS();
  }

  void tm_enable_q_max_shaping_rate(const tm_dev_t dev,
                                    const tm_port_t port,
                                    const tm_q_t q) {
    int status = p4_pd_tm_q_max_rate_shaper_enable(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_disable_q_max_shaping_rate(const tm_dev_t dev,
                                     const tm_port_t port,
                                     const tm_q_t q) {
    int status = p4_pd_tm_q_max_rate_shaper_disable(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_enable_q_min_shaping_rate(const tm_dev_t dev,
                                    const tm_port_t port,
                                    const tm_q_t q) {
    int status = p4_pd_tm_q_min_rate_shaper_enable(dev, port, q);
    TM_THROW_STATUS();
  }

  void tm_disable_q_min_shaping_rate(const tm_dev_t dev,
                                     const tm_port_t port,
                                     const tm_q_t q) {
    int status = p4_pd_tm_q_min_rate_shaper_disable(dev, port, q);
    TM_THROW_STATUS();
  }
  void tm_sched_q_adv_fc_mode_set(const tm_dev_t dev,
                                  const tm_port_t port,
                                  const tm_q_t q,
                                  const tm_sched_adv_fc_mode_t::type mode) {
    p4_pd_tm_sched_adv_fc_mode_t adv_fc_mode =
        (p4_pd_tm_sched_adv_fc_mode_t)mode;
    int status = p4_pd_tm_sched_q_adv_fc_mode_set(dev, port, q, adv_fc_mode);
    TM_THROW_STATUS();
  }
  void tm_sched_adv_fc_mode_enable_set(const tm_dev_t dev,
                                       const tm_pipe_t pipe,
                                       const bool enable) {
    int status = p4_pd_tm_sched_adv_fc_mode_enable_set(dev, pipe, enable);
    TM_THROW_STATUS();
  }

  // CFG Fetch APIs

  tm_sched_prio_t tm_get_q_sched_priority(const tm_dev_t dev,
                                          const tm_port_t port,
                                          const tm_q_t q) {
    p4_pd_tm_sched_prio_t prio;
    int status = p4_pd_tm_get_q_sched_priority(dev, port, q, &prio);
    TM_THROW_STATUS();
    return ((tm_sched_prio_t)prio);
  }

  int16_t tm_get_q_dwrr_weight(const tm_dev_t dev,
                               const tm_port_t port,
                               const tm_q_t q) {
    uint16_t wt;
    int status = p4_pd_tm_get_q_dwrr_weight(dev, port, q, &wt);
    TM_THROW_STATUS();
    return (wt);
  }

  void tm_get_q_shaping_rate(tm_shaper_rate_t &_return,
                             const tm_dev_t dev,
                             const tm_port_t port,
                             const tm_q_t q) {
    bool pps;
    uint32_t bsize;
    uint32_t rate;
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type;
    int status = p4_pd_tm_get_q_shaping_rate_provisioning(
        dev, port, q, &pps, &bsize, &rate, &pd_prov_type);
    TM_THROW_STATUS();
    _return.pps = pps;
    _return.burst_size = bsize;
    _return.rate = rate;
    _return.provisioning_type =
        (tm_sched_shaper_provisioning_type_t::type)pd_prov_type;
  }

  void tm_get_q_guaranteed_rate(tm_shaper_rate_t &_return,
                                const tm_dev_t dev,
                                const tm_port_t port,
                                const tm_q_t q) {
    bool pps;
    uint32_t bsize;
    uint32_t rate;
    int status =
        p4_pd_tm_get_q_guaranteed_rate(dev, port, q, &pps, &bsize, &rate);
    TM_THROW_STATUS();
    _return.pps = pps;
    _return.burst_size = bsize;
    _return.rate = rate;
  }

  tm_sched_prio_t tm_get_q_remaining_bw_sched_priority(const tm_dev_t dev,
                                                       const tm_port_t port,
                                                       const tm_q_t q) {
    p4_pd_tm_sched_prio_t prio;
    int status =
        p4_pd_tm_get_q_remaining_bw_sched_priority(dev, port, q, &prio);
    TM_THROW_STATUS();
    return (prio);
  }

  void tm_get_port_shaping_rate(tm_shaper_rate_t &_return,
                                const tm_dev_t dev,
                                const tm_port_t port) {
    bool pps;
    uint32_t bsize;
    uint32_t rate;
    p4_pd_tm_sched_shaper_prov_type_t pd_prov_type;

    int status = p4_pd_tm_get_port_shaping_rate_provisioning(
        dev, port, &pps, &bsize, &rate, &pd_prov_type);
    TM_THROW_STATUS();
    _return.pps = pps;
    _return.burst_size = bsize;
    _return.rate = rate;
    _return.provisioning_type =
        (tm_sched_shaper_provisioning_type_t::type)pd_prov_type;
  }

  int16_t tm_get_shaper_pkt_ifg_compensation(const tm_dev_t dev,
                                             const tm_pipe_t pipe) {
    uint8_t adjust;
    int status = p4_pd_tm_get_shaper_pkt_ifg_compensation(dev, pipe, &adjust);
    TM_THROW_STATUS();
    return ((uint16_t)adjust);
  }

  tm_cells_t tm_get_egress_pipe_limit(const tm_dev_t dev,
                                      const tm_pipe_t pipe) {
    uint32_t cells;
    int status = p4_pd_tm_get_egress_pipe_limit(dev, pipe, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_egress_pipe_hysteresis(const tm_dev_t dev,
                                           const tm_pipe_t pipe) {
    uint32_t cells;
    int status = p4_pd_tm_get_egress_pipe_hysteresis(dev, pipe, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  void tm_get_port_q_mapping(tm_q_map_t &_return,
                             const tm_dev_t dev,
                             const tm_port_t port) {
    uint8_t q_count;
    uint8_t q_map[32];
    int status = p4_pd_tm_get_port_q_mapping(dev, port, &q_count, q_map);
    TM_THROW_STATUS();
    _return.q_count = q_count;
    _return.md_qid0_to_tm_q = q_map[0];
    _return.md_qid1_to_tm_q = q_map[1];
    _return.md_qid2_to_tm_q = q_map[2];
    _return.md_qid3_to_tm_q = q_map[3];
    _return.md_qid4_to_tm_q = q_map[4];
    _return.md_qid5_to_tm_q = q_map[5];
    _return.md_qid6_to_tm_q = q_map[6];
    _return.md_qid7_to_tm_q = q_map[7];
    _return.md_qid8_to_tm_q = q_map[8];
    _return.md_qid9_to_tm_q = q_map[9];
    _return.md_qid10_to_tm_q = q_map[10];
    _return.md_qid11_to_tm_q = q_map[11];
    _return.md_qid12_to_tm_q = q_map[12];
    _return.md_qid13_to_tm_q = q_map[13];
    _return.md_qid14_to_tm_q = q_map[14];
    _return.md_qid15_to_tm_q = q_map[15];
    _return.md_qid16_to_tm_q = q_map[16];
    _return.md_qid17_to_tm_q = q_map[17];
    _return.md_qid18_to_tm_q = q_map[18];
    _return.md_qid19_to_tm_q = q_map[19];
    _return.md_qid20_to_tm_q = q_map[20];
    _return.md_qid21_to_tm_q = q_map[21];
    _return.md_qid22_to_tm_q = q_map[22];
    _return.md_qid23_to_tm_q = q_map[23];
    _return.md_qid24_to_tm_q = q_map[24];
    _return.md_qid25_to_tm_q = q_map[25];
    _return.md_qid26_to_tm_q = q_map[26];
    _return.md_qid27_to_tm_q = q_map[27];
    _return.md_qid28_to_tm_q = q_map[28];
    _return.md_qid29_to_tm_q = q_map[29];
    _return.md_qid30_to_tm_q = q_map[30];
    _return.md_qid31_to_tm_q = q_map[31];
  }

  void tm_get_port_q_mapping_adv(tm_q_map_adv_t &_return,
                                 const tm_dev_t dev,
                                 const tm_port_t port,
                                 const int32_t num_of_entries) {
    p4_pd_tm_dev_cfg_t cfg;
    int status = p4_pd_tm_dev_config_get(dev, &cfg);
    uint8_t queues_per_pg = cfg.q_per_pg;
    // Number of entries should match the number of queues per port group
    if (num_of_entries != queues_per_pg) {
      status = BF_INVALID_ARG;
      TM_THROW_STATUS();
    }

    uint8_t q_count;
    std::unique_ptr<uint8_t[]> q_map(new uint8_t[num_of_entries]);
    if (!q_map) {
      status = BF_NO_SYS_RESOURCES;
      TM_THROW_STATUS();
    }
    status = p4_pd_tm_get_port_q_mapping(dev, port, &q_count, q_map.get());
    TM_THROW_STATUS();
    for (int i = 0; i < num_of_entries; ++i) {
      _return.push_back(q_map[i]);
    }
  }

  void tm_get_q_app_pool_usage(tm_pool_usage_t &_return,
                               const tm_dev_t dev,
                               const tm_port_t port,
                               const tm_q_t q) {
    p4_pd_pool_id_t pool;
    uint32_t base_use;
    p4_pd_tm_queue_baf_t dynamic_baf;
    uint32_t hyst;
    int status = p4_pd_tm_get_q_app_pool_usage(
        dev, port, q, &pool, &base_use, &dynamic_baf, &hyst);
    TM_THROW_STATUS();
    _return.pool = pool;
    _return.base_use_limit = base_use;
    _return.dynamic_baf = dynamic_baf;
    _return.hysteresis = hyst;
  }

  tm_cells_t tm_get_q_guaranteed_min_limit(const tm_dev_t dev,
                                           const tm_port_t port,
                                           const tm_q_t q) {
    uint32_t cells;
    int status = p4_pd_tm_get_q_guaranteed_min_limit(dev, port, q, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_limit_t tm_get_q_color_limit(const tm_dev_t dev,
                                  const tm_port_t port,
                                  const tm_q_t q,
                                  const tm_color_t color) {
    p4_pd_color_limit_t limit;
    int status =
        p4_pd_tm_get_q_color_limit(dev, port, q, (p4_pd_color_t)color, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  tm_cells_t tm_get_q_color_hysteresis(const tm_dev_t dev,
                                       const tm_port_t port,
                                       const tm_q_t q,
                                       const tm_color_t color) {
    p4_pd_tm_thres_t cells = 0;
    int status = p4_pd_tm_get_q_color_hysteresis(
        dev, port, q, (p4_pd_color_t)color, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_port_uc_cut_through_limit(const tm_dev_t dev,
                                              const tm_port_t port) {
    uint8_t limit;
    int status = p4_pd_tm_get_port_uc_cut_through_limit(dev, port, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  int16_t tm_get_total_ppg(const tm_dev_t dev, const tm_pipe_t pipe) {
    uint32_t total_ppg;
    int status = p4_pd_tm_get_total_ppg(dev, pipe, &total_ppg);
    TM_THROW_STATUS();
    return ((int16_t)total_ppg);
  }

  int16_t tm_get_unused_ppg_count(const tm_dev_t dev, const tm_pipe_t pipe) {
    uint32_t unused_ppg;
    int status = p4_pd_tm_get_unused_ppg_count(dev, pipe, &unused_ppg);
    TM_THROW_STATUS();
    return ((int16_t)unused_ppg);
  }

  void tm_get_ppg_app_pool_usage(tm_pool_usage_t &_return,
                                 const tm_dev_t dev,
                                 const tm_ppg_t ppg,
                                 const tm_pool_t pool) {
    uint32_t base_use;
    p4_pd_tm_ppg_baf_t dynamic_baf;
    uint32_t hyst;
    int status = p4_pd_tm_get_ppg_app_pool_usage(
        dev, ppg, (p4_pd_pool_id_t)pool, &base_use, &dynamic_baf, &hyst);
    TM_THROW_STATUS();
    _return.pool = pool;
    _return.base_use_limit = base_use;
    _return.dynamic_baf = dynamic_baf;
    _return.hysteresis = hyst;
  }

  tm_cells_t tm_get_ppg_guaranteed_min_limit(const tm_dev_t dev,
                                             const tm_ppg_t ppg) {
    uint32_t cells;
    int status = p4_pd_tm_get_ppg_guaranteed_min_limit(dev, ppg, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_ppg_skid_limit(const tm_dev_t dev, const tm_ppg_t ppg) {
    uint32_t cells;
    int status = p4_pd_tm_get_ppg_skid_limit(dev, ppg, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_ppg_guaranteed_min_skid_hysteresis(const tm_dev_t dev,
                                                       const tm_ppg_t ppg) {
    uint32_t cells;
    int status =
        p4_pd_tm_get_ppg_guaranteed_min_skid_hysteresis(dev, ppg, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_app_pool_size(const tm_dev_t dev, const tm_pool_t pool) {
    uint32_t cells;
    int status = p4_pd_tm_get_app_pool_size(dev, (p4_pd_pool_id_t)pool, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_app_pool_color_drop_limit(const tm_dev_t dev,
                                              const tm_pool_t pool,
                                              const tm_color_t color) {
    uint32_t limit;
    int status = p4_pd_tm_get_app_pool_color_drop_limit(
        dev, (p4_pd_pool_id_t)pool, (p4_pd_color_t)color, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  tm_cells_t tm_get_app_pool_color_drop_hysteresis(const tm_dev_t dev,
                                                   const tm_color_t color) {
    uint32_t limit;
    int status = p4_pd_tm_get_app_pool_color_drop_hysteresis(
        dev, (p4_pd_color_t)color, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  tm_cells_t tm_get_app_pool_pfc_limit(const tm_dev_t dev,
                                       const tm_pool_t pool,
                                       const tm_icos_t icos) {
    uint32_t limit;
    int status = p4_pd_tm_get_app_pool_pfc_limit(
        dev, (p4_pd_pool_id_t)pool, icos, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  tm_cells_t tm_get_skid_pool_size(const tm_dev_t dev) {
    uint32_t cells;
    int status = p4_pd_tm_get_skid_pool_size(dev, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_skid_pool_hysteresis(const tm_dev_t dev) {
    uint32_t limit;
    int status = p4_pd_tm_get_skid_pool_hysteresis(dev, &limit);
    TM_THROW_STATUS();
    return ((tm_limit_t)limit);
  }

  tm_cells_t tm_get_negative_mirror_pool_size(const tm_dev_t dev) {
    uint32_t cells;
    int status = p4_pd_tm_get_negative_mirror_pool_size(dev, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_uc_cut_through_pool_size(const tm_dev_t dev,
                                             const tm_pool_t pool) {
    uint32_t cells;
    int status = p4_pd_tm_get_uc_cut_through_pool_size(
        dev, (p4_pd_pool_id_t)pool, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_mc_cut_through_pool_size(const tm_dev_t dev,
                                             const tm_pool_t pool) {
    uint32_t cells;
    int status = p4_pd_tm_get_mc_cut_through_pool_size(
        dev, (p4_pd_pool_id_t)pool, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_ingress_buffer_limit(const tm_dev_t dev) {
    uint32_t cells;
    int status = p4_pd_tm_get_ingress_buffer_limit(dev, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  bool tm_get_ingress_buffer_limit_state(const tm_dev_t dev) {
    bool state;
    int status = p4_pd_tm_get_ingress_buffer_limit_state(dev, &state);
    TM_THROW_STATUS();
    return state;
  }

  tm_cells_t tm_get_ingress_port_drop_limit(const tm_dev_t dev,
                                            const tm_port_t port) {
    uint32_t cells;
    int status = p4_pd_tm_get_ingress_port_drop_limit(dev, port, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_egress_port_drop_limit(const tm_dev_t dev,
                                           const tm_port_t port) {
    uint32_t cells;
    int status = p4_pd_tm_get_egress_port_drop_limit(dev, port, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_ingress_port_hysteresis(const tm_dev_t dev,
                                            const tm_port_t port) {
    uint32_t cells;
    int status = p4_pd_tm_get_ingress_port_hysteresis(dev, port, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_cells_t tm_get_egress_port_hysteresis(const tm_dev_t dev,
                                           const tm_port_t port) {
    uint32_t cells;
    int status = p4_pd_tm_get_egress_port_hysteresis(dev, port, &cells);
    TM_THROW_STATUS();
    return ((tm_cells_t)cells);
  }

  tm_flow_ctrl_t tm_get_port_flowcontrol_mode(const tm_dev_t dev,
                                              const tm_port_t port) {
    p4_pd_tm_flow_ctrl_type_t type;
    int status = p4_pd_tm_get_port_flowcontrol_mode(dev, port, &type);
    TM_THROW_STATUS();
    return ((tm_flow_ctrl_t)type);
  }

  void tm_get_port_pfc_cos_mapping(tm_pfc_cos_map_t &_return,
                                   const tm_dev_t dev,
                                   const tm_port_t port) {
    uint8_t cos_to_icos[8];
    int status = p4_pd_tm_get_port_pfc_cos_mapping(dev, port, cos_to_icos);
    TM_THROW_STATUS();
    _return.CoS0_to_iCos = cos_to_icos[0];
    _return.CoS1_to_iCos = cos_to_icos[1];
    _return.CoS2_to_iCos = cos_to_icos[2];
    _return.CoS3_to_iCos = cos_to_icos[3];
    _return.CoS4_to_iCos = cos_to_icos[4];
    _return.CoS5_to_iCos = cos_to_icos[5];
    _return.CoS6_to_iCos = cos_to_icos[6];
    _return.CoS7_to_iCos = cos_to_icos[7];
  }

  int16_t tm_get_ppg_icos_mapping(const tm_dev_t dev, const tm_ppg_t ppg) {
    uint8_t icos_bmap;
    int status = p4_pd_tm_ppg_icos_mapping_get(dev, ppg, &icos_bmap);
    TM_THROW_STATUS();
    return (icos_bmap);
  }

  int16_t tm_get_timestamp_shift(const tm_dev_t dev) {
    uint8_t shift;
    int status = p4_pd_tm_get_timestamp_shift(dev, &shift);
    TM_THROW_STATUS();
    return (shift);
  }

  int64_t tm_get_ppg_drop(const tm_dev_t dev,
                          const tm_pipe_t pipe,
                          const tm_ppg_t ppg) {
    uint64_t count;
    int status = p4_pd_tm_ppg_drop_get(dev, pipe, ppg, &count);
    TM_THROW_STATUS();
    return (count);
  }

  int64_t tm_get_q_drop(const tm_dev_t dev,
                        const tm_pipe_t pipe,
                        const tm_port_t port,
                        const tm_q_t q) {
    uint64_t count;
    int status = p4_pd_tm_q_drop_get(dev, pipe, port, q, &count);
    TM_THROW_STATUS();
    return (count);
  }

  void tm_get_ppg_usage(tm_ppg_usage_t &_return,
                        const tm_dev_t dev,
                        const tm_pipe_t pipe,
                        const tm_ppg_t ppg) {
    uint32_t gmin_count;
    uint32_t shared_count;
    uint32_t skid_count;
    uint32_t wm;
    int status = p4_pd_tm_ppg_usage_get(
        dev, pipe, ppg, &gmin_count, &shared_count, &skid_count, &wm);
    TM_THROW_STATUS();
    _return.gmin_count = gmin_count;
    _return.shared_count = shared_count;
    _return.skid_count = skid_count;
    _return.wm = wm;
  }

  void tm_get_q_usage(tm_usage_t &_return,
                      const tm_dev_t dev,
                      const tm_pipe_t pipe,
                      const tm_port_t port,
                      const tm_q_t q) {
    uint32_t count;
    uint32_t wm;
    int status = p4_pd_tm_q_usage_get(dev, pipe, port, q, &count, &wm);
    TM_THROW_STATUS();
    _return.count = count;
    _return.wm = wm;
  }

  void tm_get_port_drop(tm_port_drop_t &_return,
                        const tm_dev_t dev,
                        tm_pipe_t pipe,
                        const tm_port_t port) {
    uint64_t ig_count;
    uint64_t eg_count;
    int status = p4_pd_tm_port_drop_get(dev, pipe, port, &ig_count, &eg_count);
    TM_THROW_STATUS();
    _return.ig_count = ig_count;
    _return.eg_count = eg_count;
  }

  void tm_get_pool_usage(tm_usage_t &_return,
                         const tm_dev_t dev,
                         const tm_pool_t pool) {
    uint32_t count;
    uint32_t wm;
    int status =
        p4_pd_tm_pool_usage_get(dev, (p4_pd_pool_id_t)pool, &count, &wm);
    TM_THROW_STATUS();
    _return.count = count;
    _return.wm = wm;
  }

  void tm_get_blklvl_drop_cntrs(tm_blklvl_cntrs &_return,
                                const tm_dev_t dev,
                                const tm_pipe_t pipe) {
    p4_pd_tm_blklvl_cntrs_t pd_blklvl_cntrs;
    int status = p4_pd_tm_blklvl_drop_get(dev, pipe, &pd_blklvl_cntrs);

    TM_THROW_STATUS();
    copy_blklvl_cntrs(_return, &pd_blklvl_cntrs);
  }

  void tm_get_pre_fifo_drop_cntrs(tm_pre_fifo_cntrs &_return,
                                  const tm_dev_t dev) {
    p4_pd_tm_pre_fifo_cntrs_t pd_pre_fifo_cntrs;
    int status = p4_pd_tm_pre_fifo_drop_get(dev, &pd_pre_fifo_cntrs);
    TM_THROW_STATUS();
    copy_pre_fifo_cntrs(_return, &pd_pre_fifo_cntrs);
  }

  bool tm_q_visible_get(const tm_dev_t dev,
                        const tm_port_t port,
                        const tm_q_t q) {
    bool q_visible;
    int status = p4_pd_tm_q_visible_get(dev, port, q, &q_visible);
    TM_THROW_STATUS();
    return q_visible;
  }
  bool tm_qstat_report_mode_get(const tm_dev_t dev, const tm_pipe_t pipe) {
    bool q_mode;
    int status = p4_pd_tm_qstat_report_mode_get(dev, pipe, &q_mode);
    TM_THROW_STATUS();
    return q_mode;
  }
  tm_sched_adv_fc_mode_t::type tm_sched_q_adv_fc_mode_get(const tm_dev_t dev,
                                                          const tm_port_t port,
                                                          const tm_q_t q) {
    p4_pd_tm_sched_adv_fc_mode_t adv_fc_mode;
    int status = p4_pd_tm_sched_q_adv_fc_mode_get(dev, port, q, &adv_fc_mode);
    TM_THROW_STATUS();
    return (tm_sched_adv_fc_mode_t::type)adv_fc_mode;
  }
  bool tm_sched_adv_fc_mode_enable_get(const tm_dev_t dev,
                                       const tm_pipe_t pipe) {
    bool adv_fc_mode_enable;
    int status =
        p4_pd_tm_sched_adv_fc_mode_enable_get(dev, pipe, &adv_fc_mode_enable);
    TM_THROW_STATUS();
    return adv_fc_mode_enable;
  }

  void tm_get_port_pipe_phys_q(tm_port_pipe_queue &_return,
                               const tm_dev_t dev,
                               const tm_port_t port,
                               const int32_t qid) {
    p4_pd_tm_pipe_t pipe_;
    p4_pd_tm_queue_t queue_;
    int status =
        p4_pd_tm_get_port_pipe_physical_queue(dev, port, qid, &pipe_, &queue_);
    TM_THROW_STATUS();
    _return.pipe = pipe_;
    _return.phys_queue = queue_;
  }
  void tm_get_pipe_queue_qid_list(tm_port_qid_list &_return,
                                  const tm_dev_t dev,
                                  const tm_pipe_t pipe,
                                  const tm_q_t p_queue) {
    p4_pd_tm_port_t port_ = 0;
    p4_pd_tm_queue_t qid_count = 0;
    p4_pd_tm_queue_t *qlist = new p4_pd_tm_queue_t[128];
    int status = p4_pd_tm_get_pipe_queue_qid_list(
        dev, pipe, p_queue, &port_, &qid_count, qlist);
    if (0 != status) {
      delete[] qlist;
    }
    TM_THROW_STATUS();
    _return.port = port_;
    _return.qid_count = qid_count;
    for (uint32_t i = 0; i < qid_count; ++i) {
      _return.qid_list.push_back(qlist[i]);
    }
    delete[] qlist;
  }

  //////////////////   UT APIs ///////////////////////////

  void tm_set_ut_mode_as_model(const tm_dev_t dev) {
    bf_tm_set_ut_mode_as_model(dev);
  }

  void tm_set_ut_mode_as_asic(const tm_dev_t dev) {
    bf_tm_set_ut_mode_as_asic(dev);
  }
};
