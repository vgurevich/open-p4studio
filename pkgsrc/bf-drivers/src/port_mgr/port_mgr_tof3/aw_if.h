#ifndef AW_IF_INCLUDED
#define AW_IF_INCLUDED

#include <stddef.h>
#include "aw_lane_cfg.h"
#include "aw_mss.h"

// to switch between EVB and CB chip
#define USING_EVB 0

typedef enum {
  MSS_SECTION_CMN = 0,
  MSS_SECTION_RX,
  MSS_SECTION_TX,
} bf_tf3_section_t;

// BLOCK OFFSETS common to both 16ln and 4ln
// these MUST match the values in the corresponding
// aw_alphacore_csr_defines.h file(s)
//
#define LANE_OFFSET 0x02000000
#define CMN_OFFSET 0x00000000
#define RX0_OFFSET 0x02000000
#define TX0_OFFSET 0x02001000

typedef struct bf_tf3_sd_t {
  uint32_t dev_id;           // required to be copied into mss for I/O functions
  uint32_t subdev_id;        // ditto
  uint32_t macro_offset;     // offset of 16ln (or 4ln) macro base address
  uint32_t cmn_lane_offset;  // offset to commo lane (macro + CMN)
  uint32_t rx_lane_offset;   // offset to physical lane used for RX
  uint32_t tx_lane_offset;   // offset to physical lane used for TX

  uint32_t logical_lane;      // logical  lane. Really MAC channel 
  uint32_t physical_rx_lane;  // physical lane used for RX
  uint32_t physical_tx_lane;  // physical lane used for TX

  // storage for alll config items
  lane_cfg_t cfg;

} bf_tf3_sd_t;

bf_tf3_sd_t *bf_tof3_serdes_addr_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     mss_access_t *mss,
                                     bf_tf3_section_t section);
uint32_t map_aw_err_to_bf_err(uint32_t rc);
void bf_aw_trace(char *api,
                 uint32_t dev_id,
                 uint32_t dev_port,
                 uint32_t ln,
                 uint32_t argc,
                 ...);

#endif  // AW_IF_INCLUDED
