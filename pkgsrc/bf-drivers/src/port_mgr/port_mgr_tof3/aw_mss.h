#ifndef AW_MSS_INCLUDED
#define AW_MSS_INCLUDED


/**
 * An Example struct for interacting with the Serdes and SOC. Alphawave will
 * use lane_offset to determine which lane to write to. Additionally for an
 * SOC with multiple macros, the user will want to leverage something like
 * phy_offset, which will address which macro the APIs are talking to.
 */
typedef struct mss_access_s {
  /**
   * These two properties are directly utilized in in the read/write driver
   * functions. Additional properties may be added to this struct for
   * convenience and customer flexibility.
   */
  uint32_t phy_offset;   // Address offset to a particular PHY Macro
  uint32_t lane_offset;  // Lane offset used to access a particular lane within
                         // a PHY Macro

  /**
   * bfn
   */
  uint32_t rx_lane_offset;
  uint32_t tx_lane_offset;
  uint32_t derived_from_name_offset;
  /*
   * bfn, used for output by debug cmds
   */
  uint32_t dev_port;
  uint32_t ln; // logical lane within dev_port

  // HW I/O interface
  uint32_t dev_id;
  uint32_t subdev_id;

} mss_access_t;

#endif  // AW_MSS_INCLUDED
