#ifndef AW_LANE_CFG_INCLUDED
#define AW_LANE_CFG_INCLUDED

// static config for evb testing
typedef struct lane_cfg_t {
  uint32_t rx_term;
  uint32_t tx_rate;
  uint32_t rx_rate;
  uint32_t tx_width;
  uint32_t rx_width;
  uint32_t tx_pstate;
  uint32_t rx_pstate;
  uint32_t cm3;
  uint32_t cm2;
  uint32_t cm1;
  uint32_t c0;
  uint32_t c1;
  uint32_t prbs_mode;
  uint64_t user_data_pat;
  uint32_t prbs_gen_tx_en;
  uint32_t rx_prbs_chk_en;
  uint32_t tx_precoder_override;
  uint32_t tx_precoder_en_gc;
  uint32_t tx_precoder_en_pc;
  uint32_t rx_precoder_override;
  uint32_t rx_precoder_en_gc;
  uint32_t rx_precoder_en_pc;
  uint32_t tx_polarity;
  uint32_t rx_polarity;
  uint32_t loopback_mode;
  uint32_t rx_ctle_adapt_en;
  uint32_t rx_ctle_adapt_boost;
  uint32_t rx_vga_cap;
  uint32_t tx_disable;  // currently unused
  uint32_t rx_disable;  // currently unused
  uint32_t tx_reset;    // currently unused
  uint32_t rx_reset;    // currently unused

  // statuses (used by ucli)
  uint32_t init_status;
  uint32_t sig_det;
  uint32_t cdr_lock;
  uint32_t bist_lock;
  double ber;
  int32_t tx_ppm;
  int32_t rx_ppm;
  // AFE
  uint32_t ctle_rate;
  uint32_t ctle_boost;
  uint32_t vga_coarse;
  uint32_t vga_fine;
  uint32_t vga_offset;
} lane_cfg_t;

#endif  // AW_LANE_CFG_INCLUDED
