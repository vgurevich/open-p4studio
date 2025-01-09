#include "gen-cpp/sd.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_sd.h>
}

using namespace ::sd_pd_rpc;
using namespace ::res_pd_rpc;

#ifdef INCLUDE_SERDES_PACKAGE
uint32_t have_sd = 1;
#else
uint32_t have_sd = 0;
#endif

class sdHandler : virtual public sdIf {
 public:
  sdHandler() {}

  bf_status_t sd_mgmt_clksel_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 int lane,
                                 int clk_sel) {
    int status = p4_pd_sd_mgmt_clksel_set(dev_id, dev_port, lane, clk_sel);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_mgmt_clksel_get(i32_value &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    int clk_src;
    int status = p4_pd_sd_mgmt_clksel_get(dev_id, dev_port, lane, &clk_src);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = clk_src;
  }

  bf_status_t sd_mgmt_access_method_set(bf_dev_id_t dev_id, int method) {
    int status = p4_pd_sd_mgmt_access_method_set(dev_id, method);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_mgmt_access_method_get(i32_value &rtn_data, bf_dev_id_t dev_id) {
    int method;
    int status = p4_pd_sd_mgmt_access_method_get(dev_id, &method);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = method;
  }

  bf_status_t sd_mgmt_bcast_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                bool tx_dir,
                                bool en) {
    int status = p4_pd_sd_mgmt_bcast_set(dev_id, dev_port, lane, tx_dir, en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_mgmt_bcast_get(bool_value &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane,
                         bool tx_dir) {
    bool en;
    int status = p4_pd_sd_mgmt_bcast_get(dev_id, dev_port, lane, tx_dir, &en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = en;
  }

  bf_status_t sd_mgmt_reg_set(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              int lane,
                              bool tx_dir,
                              int reg,
                              int data) {
    int status =
        p4_pd_sd_mgmt_reg_set(dev_id, dev_port, lane, tx_dir, reg, data);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_mgmt_reg_get(i32_value &rtn_data,
                       bf_dev_id_t dev_id,
                       bf_dev_port_t dev_port,
                       int lane,
                       bool tx_dir,
                       int reg) {
    int data;
    int status =
        p4_pd_sd_mgmt_reg_get(dev_id, dev_port, lane, tx_dir, reg, &data);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = data;
  }

  void sd_mgmt_uc_int(i32_value &rtn_data,
                      bf_dev_id_t dev_id,
                      bf_dev_port_t dev_port,
                      int lane,
                      bool tx_dir,
                      int interrupt,
                      int int_data) {
    int data;
    int status = p4_pd_sd_mgmt_uc_int(
        dev_id, dev_port, lane, tx_dir, interrupt, int_data, &data);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = data;
  }

  bf_status_t sd_port_lane_map_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int tx_l0,
                                   int tx_l1,
                                   int tx_l2,
                                   int tx_l3,
                                   int rx_l0,
                                   int rx_l1,
                                   int rx_l2,
                                   int rx_l3) {
    int tx[4], rx[4];

    tx[0] = tx_l0;
    tx[1] = tx_l1;
    tx[2] = tx_l2;
    tx[3] = tx_l3;
    rx[0] = rx_l0;
    rx[1] = rx_l1;
    rx[2] = rx_l2;
    rx[3] = rx_l3;
    int status = p4_pd_sd_port_lane_map_set(dev_id, dev_port, tx, rx);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_port_lane_map_get(lane_map &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port) {
    int tx[4], rx[4];

    int status = p4_pd_sd_port_lane_map_get(dev_id, dev_port, tx, rx);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.tx_l0 = tx[0];
    rtn_data.tx_l1 = tx[1];
    rtn_data.tx_l2 = tx[2];
    rtn_data.tx_l3 = tx[3];
    rtn_data.rx_l0 = rx[0];
    rtn_data.rx_l1 = rx[1];
    rtn_data.rx_l2 = rx[2];
    rtn_data.rx_l3 = rx[3];
  }

  bf_status_t sd_dev_rx_eq_cal_rr_set(bf_dev_id_t dev_id,
                                      int fine_tune_lane_cnt) {
    int status = p4_pd_sd_dev_rx_eq_cal_rr_set(dev_id, fine_tune_lane_cnt);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_dev_rx_eq_cal_rr_get(i32_value &rtn_data, bf_dev_id_t dev_id) {
    int fine_tune_lane_cnt;
    int status = p4_pd_sd_dev_rx_eq_cal_rr_get(dev_id, &fine_tune_lane_cnt);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = fine_tune_lane_cnt;
  }

  bf_status_t sd_tx_pll_clksel_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int clk_source) {
    int status = p4_pd_sd_tx_pll_clksel_set(dev_id, dev_port, lane, clk_source);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_pll_clksel_get(i32_value &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    int clk_source;
    int status =
        p4_pd_sd_tx_pll_clksel_get(dev_id, dev_port, lane, &clk_source);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = clk_source;
  }

  bf_status_t sd_lane_init_run(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               int line_rate,
                               bool init_rx,
                               bool init_tx,
                               bool tx_drv_en,
                               bool phase_cal) {
    int status = p4_pd_sd_lane_init_run(dev_id,
                                        dev_port,
                                        lane,
                                        line_rate,
                                        init_rx,
                                        init_tx,
                                        tx_drv_en,
                                        phase_cal);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_pll_lock_get(bool_value &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    bool locked;
    int status = p4_pd_sd_tx_pll_lock_get(dev_id, dev_port, lane, &locked);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = locked;
  }

  void sd_rx_cdr_lock_get(bool_value &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    bool locked;
    int status = p4_pd_sd_rx_cdr_lock_get(dev_id, dev_port, lane, &locked);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = locked;
  }

  void sd_tx_pll_status_get(pll_status &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    bool locked;
    int div, freq;
    int status = p4_pd_sd_tx_pll_status_get(
        dev_id, dev_port, lane, &locked, &div, &freq);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.locked = locked;
    rtn_data.div = div;
    rtn_data.freq = freq;
  }

  void sd_rx_cdr_status_get(pll_status &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    bool locked;
    int div, freq;
    int status = p4_pd_sd_rx_cdr_status_get(
        dev_id, dev_port, lane, &locked, &div, &freq);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.locked = locked;
    rtn_data.div = div;
    rtn_data.freq = freq;
  }

  bf_status_t sd_lane_loopback_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int loopback_mode) {
    int status =
        p4_pd_sd_lane_loopback_set(dev_id, dev_port, lane, loopback_mode);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_lane_loopback_get(i32_value &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    int loopback_mode;
    int status =
        p4_pd_sd_lane_loopback_get(dev_id, dev_port, lane, &loopback_mode);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = loopback_mode;
  }

  bf_status_t sd_tx_en_set(bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane,
                           bool en) {
    int status = p4_pd_sd_tx_en_set(dev_id, dev_port, lane, en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_en_get(bool_value &rtn_data,
                    bf_dev_id_t dev_id,
                    bf_dev_port_t dev_port,
                    int lane) {
    bool en;
    int status = p4_pd_sd_tx_en_get(dev_id, dev_port, lane, &en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = en;
  }

  bf_status_t sd_tx_drv_en_set(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               bool en) {
    int status = p4_pd_sd_tx_drv_en_set(dev_id, dev_port, lane, en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_drv_en_get(bool_value &rtn_data,
                        bf_dev_id_t dev_id,
                        bf_dev_port_t dev_port,
                        int lane) {
    bool en;
    int status = p4_pd_sd_tx_drv_en_get(dev_id, dev_port, lane, &en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = en;
  }

  bf_status_t sd_tx_drv_inv_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                bool inv) {
    int status = p4_pd_sd_tx_drv_inv_set(dev_id, dev_port, lane, inv);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_drv_inv_get(bool_value &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    bool inv;
    int status = p4_pd_sd_tx_drv_inv_get(dev_id, dev_port, lane, &inv);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = inv;
  }

  bf_status_t sd_tx_drv_attn_is_valid(int attn_main,
                                      int attn_post,
                                      int attn_pre) {
    int status = p4_pd_sd_tx_drv_attn_is_valid(attn_main, attn_post, attn_pre);
    return status;
  }

  bf_status_t sd_tx_drv_attn_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 int lane,
                                 int attn_main,
                                 int attn_post,
                                 int attn_pre) {
    int status = p4_pd_sd_tx_drv_attn_set(
        dev_id, dev_port, lane, attn_main, attn_post, attn_pre);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_drv_attn_get(drv_attn &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    int attn_main, attn_post, attn_pre;
    int status = p4_pd_sd_tx_drv_attn_get(
        dev_id, dev_port, lane, &attn_main, &attn_post, &attn_pre);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.attn_main = attn_main;
    rtn_data.attn_post = attn_post;
    rtn_data.attn_pre = attn_pre;
  }

  bf_status_t sd_tx_drv_amp_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                int amp_main,
                                int amp_post,
                                int amp_pre) {
    int status = p4_pd_sd_tx_drv_amp_set(
        dev_id, dev_port, lane, amp_main, amp_post, amp_pre);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_drv_amp_get(drv_amp &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    int amp_main, amp_post, amp_pre;
    int status = p4_pd_sd_tx_drv_amp_get(
        dev_id, dev_port, lane, &amp_main, &amp_post, &amp_pre);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.amp_main = amp_main;
    rtn_data.amp_post = amp_post;
    rtn_data.amp_pre = amp_pre;
  }

  void sd_tx_drv_status_get(tx_drv_sts &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    bool tx_en;
    bool tx_drv_en;
    bool tx_inv;
    int amp_main;
    int amp_post;
    int amp_pre;

    int status = p4_pd_sd_tx_drv_status_get(dev_id,
                                            dev_port,
                                            lane,
                                            &tx_en,
                                            &tx_drv_en,
                                            &tx_inv,
                                            &amp_main,
                                            &amp_post,
                                            &amp_pre);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.tx_en = tx_en;
    rtn_data.tx_drv_en = tx_drv_en;
    rtn_data.tx_inv = tx_inv;
    rtn_data.amp_main = amp_main;
    rtn_data.amp_post = amp_post;
    rtn_data.amp_pre = amp_pre;
  }

  bf_status_t sd_rx_en_set(bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane,
                           bool en) {
    int status = p4_pd_sd_rx_en_set(dev_id, dev_port, lane, en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_en_get(bool_value &rtn_data,
                    bf_dev_id_t dev_id,
                    bf_dev_port_t dev_port,
                    int lane) {
    bool en;
    int status = p4_pd_sd_rx_en_get(dev_id, dev_port, lane, &en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = en;
  }

  bf_status_t sd_rx_afe_inv_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                bool inv) {
    int status = p4_pd_sd_rx_afe_inv_set(dev_id, dev_port, lane, inv);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_afe_inv_get(bool_value &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    bool inv;
    int status = p4_pd_sd_rx_afe_inv_get(dev_id, dev_port, lane, &inv);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = inv;
  }

  bf_status_t sd_rx_afe_term_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 int lane,
                                 int rx_term) {
    int status = p4_pd_sd_rx_afe_term_set(dev_id, dev_port, lane, rx_term);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_afe_term_get(i32_value &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    int rx_term;
    int status = p4_pd_sd_rx_afe_term_get(dev_id, dev_port, lane, &rx_term);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = rx_term;
  }

  bf_status_t sd_rx_afe_los_thres_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      int lane,
                                      bool rx_los_en,
                                      int rx_los_thres) {
    int status = p4_pd_sd_rx_afe_los_thres_set(
        dev_id, dev_port, lane, rx_los_en, rx_los_thres);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_afe_los_thres_get(rx_los_cfg &rtn_data,
                               bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane) {
    bool rx_los_en;
    int rx_los_thres;
    int status = p4_pd_sd_rx_afe_los_thres_get(
        dev_id, dev_port, lane, &rx_los_en, &rx_los_thres);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.rx_los_en = rx_los_en;
    rtn_data.rx_los_thres = rx_los_thres;
  }

  void sd_rx_afe_los_get(bool_value &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    bool rx_los;
    int status = p4_pd_sd_rx_afe_los_get(dev_id, dev_port, lane, &rx_los);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = rx_los;
  }

  void sd_rx_eq_cal_busy_get(bool_value &rtn_data,
                             bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             int lane,
                             int chk_cnt,
                             int chk_wait) {
    bool uc_busy;
    int status = p4_pd_sd_rx_eq_cal_busy_get(
        dev_id, dev_port, lane, chk_cnt, chk_wait, &uc_busy);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = uc_busy;
  }

  bf_status_t sd_rx_eq_ctle_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                int ctle_dc,
                                int ctle_lf,
                                int ctle_hf,
                                int ctle_bw) {
    int status = p4_pd_sd_rx_eq_ctle_set(
        dev_id, dev_port, lane, ctle_dc, ctle_lf, ctle_hf, ctle_bw);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_ctle_get(rx_eq_ctle &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    int ctle_dc, ctle_lf, ctle_hf, ctle_bw;
    int status = p4_pd_sd_rx_eq_ctle_get(
        dev_id, dev_port, lane, &ctle_dc, &ctle_lf, &ctle_hf, &ctle_bw);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.ctle_dc = ctle_dc;
    rtn_data.ctle_lf = ctle_lf;
    rtn_data.ctle_hf = ctle_hf;
    rtn_data.ctle_bw = ctle_bw;
  }

  bf_status_t sd_rx_eq_dfe_adv_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int tap_num,
                                   int tap_val) {
    int status =
        p4_pd_sd_rx_eq_dfe_adv_set(dev_id, dev_port, lane, tap_num, tap_val);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_dfe_adv_get(i32_value &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane,
                            int tap_num) {
    int tap_val;
    int status =
        p4_pd_sd_rx_eq_dfe_adv_get(dev_id, dev_port, lane, tap_num, &tap_val);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = tap_val;
  }

  bf_status_t sd_rx_eq_dfe_set(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               int dfe_gain,
                               int tap1,
                               int tap2,
                               int tap3,
                               int tap4) {
    int status = p4_pd_sd_rx_eq_dfe_set(
        dev_id, dev_port, lane, dfe_gain, tap1, tap2, tap3, tap4);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_dfe_get(dfe_taps &rtn_data,
                        bf_dev_id_t dev_id,
                        bf_dev_port_t dev_port,
                        int lane) {
    int dfe_gain, tap1, tap2, tap3, tap4;
    int status = p4_pd_sd_rx_eq_dfe_get(
        dev_id, dev_port, lane, &dfe_gain, &tap1, &tap2, &tap3, &tap4);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.dfe_gain = dfe_gain;
    rtn_data.tap1 = tap1;
    rtn_data.tap2 = tap2;
    rtn_data.tap3 = tap3;
    rtn_data.tap4 = tap4;
  }

  bf_status_t sd_rx_eq_cal_param_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     int lane,
                                     int ctle_dc_hint,
                                     int dfe_gain_range,
                                     int pcal_loop_cnt) {
    int status = p4_pd_sd_rx_eq_cal_param_set(
        dev_id, dev_port, lane, ctle_dc_hint, dfe_gain_range, pcal_loop_cnt);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_cal_param_get(eq_cal_params &rtn_data,
                              bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              int lane) {
    int ctle_dc_hint, dfe_gain_range, pcal_loop_cnt;
    int status = p4_pd_sd_rx_eq_cal_param_get(
        dev_id, dev_port, lane, &ctle_dc_hint, &dfe_gain_range, &pcal_loop_cnt);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.ctle_dc_hint = ctle_dc_hint;
    rtn_data.dfe_gain_range = dfe_gain_range;
    rtn_data.pcal_loop_cnt = pcal_loop_cnt;
  }

  bf_status_t sd_rx_eq_cal_adv_run(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int cal_cmd,
                                   int ctle_cal_cfg,
                                   int dfe_fixed) {
    int status = p4_pd_sd_rx_eq_cal_adv_run(
        dev_id, dev_port, lane, cal_cmd, ctle_cal_cfg, dfe_fixed);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  bf_status_t sd_rx_eq_ical_run(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane) {
    int status = p4_pd_sd_rx_eq_ical_run(dev_id, dev_port, lane);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_cal_eye_get(i32_value &rtn_data,
                            bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            int lane) {
    int cal_eye;
    int status = p4_pd_sd_rx_eq_cal_eye_get(dev_id, dev_port, lane, &cal_eye);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = cal_eye;
  }

  void sd_rx_eq_ical_eye_get(ical_result &rtn_data,
                             bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             int lane,
                             int cal_good_thres) {
    int cal_done, cal_good, cal_eye;
    int status = p4_pd_sd_rx_eq_ical_eye_get(
        dev_id, dev_port, lane, cal_good_thres, &cal_done, &cal_good, &cal_eye);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.cal_done = cal_done;
    rtn_data.cal_good = cal_good;
    rtn_data.cal_eye = cal_eye;
  }

  bf_status_t sd_rx_eq_pcal_run(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                int cal_count) {
    int status = p4_pd_sd_rx_eq_pcal_run(dev_id, dev_port, lane, cal_count);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eq_status_get(rx_eq_status &rtn_data,
                           bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane) {
    rx_eq_status_t st;
    int status = p4_pd_sd_rx_eq_status_get(dev_id, dev_port, lane, &st);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.cal_done = st.cal_done;
    rtn_data.cal_good = st.cal_good;
    rtn_data.cal_eye = st.cal_eye;
    rtn_data.ctle_dc = st.ctle_dc;
    rtn_data.ctle_lf = st.ctle_lf;
    rtn_data.ctle_hf = st.ctle_hf;
    rtn_data.ctle_bw = st.ctle_bw;
    rtn_data.dfe_taps_0 = st.dfe_taps[0];
    rtn_data.dfe_taps_1 = st.dfe_taps[1];
    rtn_data.dfe_taps_2 = st.dfe_taps[2];
    rtn_data.dfe_taps_3 = st.dfe_taps[3];
    rtn_data.dfe_taps_4 = st.dfe_taps[4];
    rtn_data.dfe_taps_5 = st.dfe_taps[5];
    rtn_data.dfe_taps_6 = st.dfe_taps[6];
    rtn_data.dfe_taps_7 = st.dfe_taps[7];
    rtn_data.dfe_taps_8 = st.dfe_taps[8];
    rtn_data.dfe_taps_9 = st.dfe_taps[9];
    rtn_data.dfe_taps_10 = st.dfe_taps[10];
    rtn_data.dfe_taps_11 = st.dfe_taps[11];
    rtn_data.dfe_taps_12 = st.dfe_taps[12];
    rtn_data.dfe_taps_13 = st.dfe_taps[13];
    rtn_data.dfe_taps_14 = st.dfe_taps[14];
    rtn_data.dfe_taps_15 = st.dfe_taps[15];
  }

  bf_status_t sd_rx_eye_offset_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int lane,
                                   int offset_en,
                                   int pos_x,
                                   int pos_y) {
    int status = p4_pd_sd_rx_eye_offset_set(
        dev_id, dev_port, lane, offset_en, pos_x, pos_y);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_eye_get(i32_value &rtn_data,
                     bf_dev_id_t dev_id,
                     bf_dev_port_t dev_port,
                     int lane,
                     int meas_mode,
                     int meas_ber) {
    int meas_eye;
    int status = p4_pd_sd_rx_eye_get(
        dev_id, dev_port, lane, meas_mode, meas_ber, &meas_eye);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = meas_eye;
  }

  void sd_rx_eye_3d_get(i32_value &rtn_data,
                        bf_dev_id_t dev_id,
                        bf_dev_port_t dev_port,
                        int lane,
                        int meas_ber) {
    char meas_eye[64 * 1024];
    int status = p4_pd_sd_rx_eye_3d_get(
        dev_id, dev_port, lane, meas_ber, meas_eye, sizeof(meas_eye));
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    // rtn_data.value = meas_eye;
  }

  bf_status_t sd_tx_err_inj_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                int num_bits) {
    int status = p4_pd_sd_tx_err_inj_set(dev_id, dev_port, lane, num_bits);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  bf_status_t sd_rx_err_inj_set(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                int lane,
                                int num_bits) {
    int status = p4_pd_sd_rx_err_inj_set(dev_id, dev_port, lane, num_bits);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  bf_status_t sd_tx_patsel_set(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               int tx_patsel) {
    int status = p4_pd_sd_tx_patsel_set(dev_id, dev_port, lane, tx_patsel);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_patsel_get(i32_value &rtn_data,
                        bf_dev_id_t dev_id,
                        bf_dev_port_t dev_port,
                        int lane) {
    int tx_patsel;
    int status = p4_pd_sd_tx_patsel_get(dev_id, dev_port, lane, &tx_patsel);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = tx_patsel;
  }

  bf_status_t sd_rx_patsel_set(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               int rx_patsel) {
    int status = p4_pd_sd_rx_patsel_set(dev_id, dev_port, lane, rx_patsel);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_rx_patsel_get(i32_value &rtn_data,
                        bf_dev_id_t dev_id,
                        bf_dev_port_t dev_port,
                        int lane) {
    int rx_patsel;
    int status = p4_pd_sd_rx_patsel_get(dev_id, dev_port, lane, &rx_patsel);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = rx_patsel;
  }

  void sd_rx_err_cnt_get(i32_value &rtn_data,
                         bf_dev_id_t dev_id,
                         bf_dev_port_t dev_port,
                         int lane) {
    uint32_t err_cnt;
    int status = p4_pd_sd_rx_err_cnt_get(dev_id, dev_port, lane, &err_cnt);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.value = err_cnt;
  }

  bf_status_t sd_tx_fixed_pat_set(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  int lane,
                                  int tx_fixed_pat_0,
                                  int tx_fixed_pat_1,
                                  int tx_fixed_pat_2,
                                  int tx_fixed_pat_3) {
    int status = p4_pd_sd_tx_fixed_pat_set(dev_id,
                                           dev_port,
                                           lane,
                                           tx_fixed_pat_0,
                                           tx_fixed_pat_1,
                                           tx_fixed_pat_2,
                                           tx_fixed_pat_3);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void sd_tx_fixed_pat_get(tx_fixed_pat &rtn_data,
                           bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane) {
    int tx_fixed_pat_0, tx_fixed_pat_1, tx_fixed_pat_2, tx_fixed_pat_3;
    int status = p4_pd_sd_tx_fixed_pat_get(dev_id,
                                           dev_port,
                                           lane,
                                           &tx_fixed_pat_0,
                                           &tx_fixed_pat_1,
                                           &tx_fixed_pat_2,
                                           &tx_fixed_pat_3);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.tx_fixed_pat_0 = tx_fixed_pat_0;
    rtn_data.tx_fixed_pat_1 = tx_fixed_pat_1;
    rtn_data.tx_fixed_pat_2 = tx_fixed_pat_2;
    rtn_data.tx_fixed_pat_3 = tx_fixed_pat_3;
  }

  void sd_rx_data_cap_get(rx_cap_pat &rtn_data,
                          bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          int lane) {
    int rx_cap_pat_0, rx_cap_pat_1, rx_cap_pat_2, rx_cap_pat_3;
    int status = p4_pd_sd_rx_data_cap_get(dev_id,
                                          dev_port,
                                          lane,
                                          &rx_cap_pat_0,
                                          &rx_cap_pat_1,
                                          &rx_cap_pat_2,
                                          &rx_cap_pat_3);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.rx_cap_pat_0 = rx_cap_pat_0;
    rtn_data.rx_cap_pat_1 = rx_cap_pat_1;
    rtn_data.rx_cap_pat_2 = rx_cap_pat_2;
    rtn_data.rx_cap_pat_3 = rx_cap_pat_3;
  }

  void sd_get_tx_eq(tx_eq_data &rtn_data,
                    bf_dev_id_t dev_id,
                    bf_dev_port_t dev_port,
                    int lane) {
    int32_t pre, atten, post;
    int status =
        p4_pd_sd_get_tx_eq(dev_id, dev_port, lane, &pre, &atten, &post);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.pre = pre;
    rtn_data.atten = atten;
    rtn_data.post = post;
  }

  bf_status_t sd_set_tx_eq(bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane,
                           int pre,
                           int atten,
                           int post) {
    int status = p4_pd_sd_set_tx_eq(dev_id, dev_port, lane, pre, atten, post);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  bf_status_t sd_get_pll_state(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               int lane,
                               int expected_divider) {
    int status =
        p4_pd_sd_get_pll_state(dev_id, dev_port, lane, expected_divider);
    return status;
  }

  void sd_get_tx_output_en(bool_state &rtn_data,
                           bf_dev_id_t dev_id,
                           bf_dev_port_t dev_port,
                           int lane) {
    bool en;
    int status = p4_pd_sd_get_tx_output_en(dev_id, dev_port, lane, &en);
    if (have_sd && status != 0) {
      InvalidSdOperation iop;
      iop.code = status;
      throw iop;
    }
    rtn_data.status = status;
    rtn_data.st = en;
  }
};
