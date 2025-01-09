#include "gen-cpp/pal.h"

extern "C" {
#include <tofino/bf_pal/bf_pal_types.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
}

using namespace ::pal_rpc;

class palHandler : virtual public palIf {
 private:
  pal_status_t pal_check_valid_rmon_counter(
      const pal_rmon_counter_t::type ctr_type, bf_rmon_counter_t *cnt_type) {
    // private
    switch (ctr_type) {
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedOK:
        *cnt_type = bf_mac_stat_FramesReceivedOK;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedAll:
        *cnt_type = bf_mac_stat_FramesReceivedAll;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedwithFCSError:
        *cnt_type = bf_mac_stat_FramesReceivedwithFCSError;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FrameswithanyError:
        *cnt_type = bf_mac_stat_FrameswithanyError;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_OctetsReceivedinGoodFrames:
        *cnt_type = bf_mac_stat_OctetsReceivedinGoodFrames;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_OctetsReceived:
        *cnt_type = bf_mac_stat_OctetsReceived;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedwithUnicastAddresses:
        *cnt_type = bf_mac_stat_FramesReceivedwithUnicastAddresses;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedwithMulticastAddresses:
        *cnt_type = bf_mac_stat_FramesReceivedwithMulticastAddresses;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedwithBroadcastAddresses:
        *cnt_type = bf_mac_stat_FramesReceivedwithBroadcastAddresses;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedoftypePAUSE:
        *cnt_type = bf_mac_stat_FramesReceivedoftypePAUSE;  // 9
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedwithLengthError:
        *cnt_type = bf_mac_stat_FramesReceivedwithLengthError;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedUndersized:
        *cnt_type = bf_mac_stat_FramesReceivedUndersized;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedOversized:
        *cnt_type = bf_mac_stat_FramesReceivedOversized;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FragmentsReceived:
        *cnt_type = bf_mac_stat_FragmentsReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_JabberReceived:
        *cnt_type = bf_mac_stat_JabberReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_PriorityPauseFrames:
        *cnt_type = bf_mac_stat_PriorityPauseFrames;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_CRCErrorStomped:
        *cnt_type = bf_mac_stat_CRCErrorStomped;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FrameTooLong:
        *cnt_type = bf_mac_stat_FrameTooLong;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_RxVLANFramesGood:
        *cnt_type = bf_mac_stat_RxVLANFramesGood;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesDroppedBufferFull:
        *cnt_type = bf_mac_stat_FramesDroppedBufferFull;  // 19
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_lt_64:
        *cnt_type = bf_mac_stat_FramesReceivedLength_lt_64;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_eq_64:
        *cnt_type = bf_mac_stat_FramesReceivedLength_eq_64;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_65_127:
        *cnt_type = bf_mac_stat_FramesReceivedLength_65_127;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_128_255:
        *cnt_type = bf_mac_stat_FramesReceivedLength_128_255;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_256_511:
        *cnt_type = bf_mac_stat_FramesReceivedLength_256_511;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_512_1023:
        *cnt_type = bf_mac_stat_FramesReceivedLength_512_1023;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedLength_1024_1518:
        *cnt_type = bf_mac_stat_FramesReceivedLength_1024_1518;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedLength_1519_2047:
        *cnt_type = bf_mac_stat_FramesReceivedLength_1519_2047;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedLength_2048_4095:
        *cnt_type = bf_mac_stat_FramesReceivedLength_2048_4095;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedLength_4096_8191:
        *cnt_type = bf_mac_stat_FramesReceivedLength_4096_8191;  // 29
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesReceivedLength_8192_9215:
        *cnt_type = bf_mac_stat_FramesReceivedLength_8192_9215;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesReceivedLength_9216:
        *cnt_type = bf_mac_stat_FramesReceivedLength_9216;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedOK:
        *cnt_type = bf_mac_stat_FramesTransmittedOK;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedAll:
        *cnt_type = bf_mac_stat_FramesTransmittedAll;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedwithError:
        *cnt_type = bf_mac_stat_FramesTransmittedwithError;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_OctetsTransmittedwithouterror:
        *cnt_type = bf_mac_stat_OctetsTransmittedwithouterror;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_OctetsTransmittedTotal:
        *cnt_type = bf_mac_stat_OctetsTransmittedTotal;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedUnicast:
        *cnt_type = bf_mac_stat_FramesTransmittedUnicast;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedMulticast:
        *cnt_type = bf_mac_stat_FramesTransmittedMulticast;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedBroadcast:
        *cnt_type = bf_mac_stat_FramesTransmittedBroadcast;  // 39
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedPause:
        *cnt_type = bf_mac_stat_FramesTransmittedPause;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedPriPause:
        *cnt_type = bf_mac_stat_FramesTransmittedPriPause;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedVLAN:
        *cnt_type = bf_mac_stat_FramesTransmittedVLAN;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedLength_lt_64:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_lt_64;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedLength_eq_64:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_eq_64;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_65_127:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_65_127;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_128_255:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_128_255;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_256_511:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_256_511;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_512_1023:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_512_1023;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_1024_1518:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_1024_1518;  // 49
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_1519_2047:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_1519_2047;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_2048_4095:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_2048_4095;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_4096_8191:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_4096_8191;
        break;
      case pal_rmon_counter_t::type::
          pal_mac_stat_FramesTransmittedLength_8192_9215:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_8192_9215;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTransmittedLength_9216:
        *cnt_type = bf_mac_stat_FramesTransmittedLength_9216;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri0FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri0FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri1FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri1FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri2FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri2FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri3FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri3FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri4FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri4FramesTransmitted;  // 59
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri5FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri5FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri6FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri6FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri7FramesTransmitted:
        *cnt_type = bf_mac_stat_Pri7FramesTransmitted;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri0FramesReceived:
        *cnt_type = bf_mac_stat_Pri0FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri1FramesReceived:
        *cnt_type = bf_mac_stat_Pri1FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri2FramesReceived:
        *cnt_type = bf_mac_stat_Pri2FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri3FramesReceived:
        *cnt_type = bf_mac_stat_Pri3FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri4FramesReceived:
        *cnt_type = bf_mac_stat_Pri4FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri5FramesReceived:
        *cnt_type = bf_mac_stat_Pri5FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri6FramesReceived:
        *cnt_type = bf_mac_stat_Pri6FramesReceived;  // 69
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_Pri7FramesReceived:
        *cnt_type = bf_mac_stat_Pri7FramesReceived;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri0Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri0Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri1Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri1Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri2Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri2Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri3Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri3Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri4Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri4Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri5Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri5Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri6Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri6Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_TransmitPri7Pause1USCount:
        *cnt_type = bf_mac_stat_TransmitPri7Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri0Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri0Pause1USCount;  // 79
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri1Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri1Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri2Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri2Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri3Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri3Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri4Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri4Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri5Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri5Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri6Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri6Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceivePri7Pause1USCount:
        *cnt_type = bf_mac_stat_ReceivePri7Pause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_ReceiveStandardPause1USCount:
        *cnt_type = bf_mac_stat_ReceiveStandardPause1USCount;
        break;
      case pal_rmon_counter_t::type::pal_mac_stat_FramesTruncated:
        *cnt_type = bf_mac_stat_FramesTruncated;
        break;
      case pal_rmon_counter_t::type::PAL_NUM_RMON_COUNTERS:
        *cnt_type = BF_NUM_RMON_COUNTERS;  // 89
        break;
      default:
        return BF_INVALID_ARG;
    }
    return BF_SUCCESS;
  }

 public:
  palHandler() {}

  pal_status_t pal_port_add(const pal_device_t dev,
                            const pal_dev_port_t dev_port,
                            const pal_port_speed_t::type ps,
                            const pal_fec_type_t::type fec) {
    bf_port_speed_t port_speed;
    bf_fec_type_t fec_type;
    switch (ps) {
      case pal_port_speed_t::type::BF_SPEED_NONE:
        port_speed = BF_SPEED_NONE;
        break;
      case pal_port_speed_t::type::BF_SPEED_1G:
        port_speed = BF_SPEED_1G;
        break;
      case pal_port_speed_t::type::BF_SPEED_10G:
        port_speed = BF_SPEED_10G;
        break;
      case pal_port_speed_t::type::BF_SPEED_25G:
        port_speed = BF_SPEED_25G;
        break;
      case pal_port_speed_t::type::BF_SPEED_40G:
        port_speed = BF_SPEED_40G;
        break;
      case pal_port_speed_t::type::BF_SPEED_50G:
        port_speed = BF_SPEED_50G;
        break;
      case pal_port_speed_t::type::BF_SPEED_100G:
        port_speed = BF_SPEED_100G;
        break;
      case pal_port_speed_t::type::BF_SPEED_200G:
        port_speed = BF_SPEED_200G;
        break;
      case pal_port_speed_t::type::BF_SPEED_400G:
        port_speed = BF_SPEED_400G;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    switch (fec) {
      case pal_fec_type_t::type::BF_FEC_TYP_NONE:
        fec_type = BF_FEC_TYP_NONE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_FIRECODE:
        fec_type = BF_FEC_TYP_FIRECODE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_REED_SOLOMON:
        fec_type = BF_FEC_TYP_REED_SOLOMON;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    bf_status_t sts;
    sts = bf_pal_port_add(dev, dev_port, port_speed, fec_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_add_with_lanes(const pal_device_t dev,
                            const pal_dev_port_t dev_port,
                            const pal_port_speed_t::type ps,
                            const int32_t lanes,
                            const pal_fec_type_t::type fec) {
    bf_port_speed_t port_speed;
    bf_fec_type_t fec_type;
    switch (ps) {
      case pal_port_speed_t::type::BF_SPEED_NONE:
        port_speed = BF_SPEED_NONE;
        break;
      case pal_port_speed_t::type::BF_SPEED_1G:
        port_speed = BF_SPEED_1G;
        break;
      case pal_port_speed_t::type::BF_SPEED_10G:
        port_speed = BF_SPEED_10G;
        break;
      case pal_port_speed_t::type::BF_SPEED_25G:
        port_speed = BF_SPEED_25G;
        break;
      case pal_port_speed_t::type::BF_SPEED_40G:
        port_speed = BF_SPEED_40G;
        break;
      case pal_port_speed_t::type::BF_SPEED_50G:
        port_speed = BF_SPEED_50G;
        break;
      case pal_port_speed_t::type::BF_SPEED_100G:
        port_speed = BF_SPEED_100G;
        break;
      case pal_port_speed_t::type::BF_SPEED_200G:
        port_speed = BF_SPEED_200G;
        break;
      case pal_port_speed_t::type::BF_SPEED_400G:
        port_speed = BF_SPEED_400G;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    switch (fec) {
      case pal_fec_type_t::type::BF_FEC_TYP_NONE:
        fec_type = BF_FEC_TYP_NONE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_FIRECODE:
        fec_type = BF_FEC_TYP_FIRECODE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_REED_SOLOMON:
        fec_type = BF_FEC_TYP_REED_SOLOMON;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    bf_status_t sts;
    sts = bf_pal_port_add_with_lanes(dev, dev_port, port_speed, lanes, fec_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_add_all(const pal_device_t dev,
                                const pal_port_speed_t::type ps,
                                const pal_fec_type_t::type fec) {
    bf_port_speed_t port_speed;
    bf_fec_type_t fec_type;
    switch (ps) {
      case pal_port_speed_t::type::BF_SPEED_NONE:
        port_speed = BF_SPEED_NONE;
        break;
      case pal_port_speed_t::type::BF_SPEED_1G:
        port_speed = BF_SPEED_1G;
        break;
      case pal_port_speed_t::type::BF_SPEED_10G:
        port_speed = BF_SPEED_10G;
        break;
      case pal_port_speed_t::type::BF_SPEED_25G:
        port_speed = BF_SPEED_25G;
        break;
      case pal_port_speed_t::type::BF_SPEED_40G:
        port_speed = BF_SPEED_40G;
        break;
      case pal_port_speed_t::type::BF_SPEED_50G:
        port_speed = BF_SPEED_50G;
        break;
      case pal_port_speed_t::type::BF_SPEED_100G:
        port_speed = BF_SPEED_100G;
        break;
      case pal_port_speed_t::type::BF_SPEED_200G:
        port_speed = BF_SPEED_200G;
        break;
      case pal_port_speed_t::type::BF_SPEED_400G:
        port_speed = BF_SPEED_400G;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    switch (fec) {
      case pal_fec_type_t::type::BF_FEC_TYP_NONE:
        fec_type = BF_FEC_TYP_NONE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_FIRECODE:
        fec_type = BF_FEC_TYP_FIRECODE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_REED_SOLOMON:
        fec_type = BF_FEC_TYP_REED_SOLOMON;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    bf_status_t sts;
    sts = bf_pal_port_add_all(dev, port_speed, fec_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_del(const pal_device_t dev,
                            const pal_dev_port_t dev_port) {
    bf_status_t sts;
    sts = bf_pal_port_del(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_del_all(const pal_device_t dev) {
    bf_status_t sts;
    sts = bf_pal_port_del_all(dev);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_enable(const pal_device_t dev,
                               const pal_dev_port_t dev_port) {
    bf_status_t sts;
    sts = bf_pal_port_enable(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_enable_all(const pal_device_t dev) {
    bf_status_t sts;
    sts = bf_pal_port_enable_all(dev);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_dis(const pal_device_t dev,
                            const pal_dev_port_t dev_port) {
    bf_status_t sts;
    sts = bf_pal_port_disable(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_oper_status_t::type pal_port_oper_status_get(
      const pal_device_t dev, const pal_dev_port_t dev_port) {
    bf_status_t sts;
    bool oper_status;
    sts = bf_pal_port_oper_state_get(dev, dev_port, &oper_status);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    pal_oper_status_t::type oper = oper_status
                                       ? pal_oper_status_t::type::BF_PORT_UP
                                       : pal_oper_status_t::type::BF_PORT_DOWN;
    return oper;
  }

  bool pal_port_is_valid(const pal_device_t dev,
                         const pal_dev_port_t dev_port) {
    bf_status_t sts = BF_SUCCESS;
    sts = bf_pal_port_is_valid(dev, dev_port);
    if (sts != BF_SUCCESS) {
      return false;
    }
    return true;
  }

  pal_autoneg_policy_t::type pal_port_an_get(const pal_device_t dev,
                               const pal_dev_port_t dev_port) {
    bf_status_t sts;
    bf_pm_port_autoneg_policy_e an_policy;
    pal_autoneg_policy_t::type an; 
    sts = bf_pal_port_autoneg_policy_get(dev, dev_port, &an_policy);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
 
    switch (an_policy) {
      case PM_AN_DEFAULT:
        an = pal_autoneg_policy_t::type::BF_AN_DEFAULT;
        break;
      case PM_AN_FORCE_ENABLE:
        an = pal_autoneg_policy_t::type::BF_AN_FORCE_ENABLE;
        break;
      case PM_AN_FORCE_DISABLE:
        an = pal_autoneg_policy_t::type::BF_AN_FORCE_DISABLE;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
 
    return an;
  }

  pal_status_t pal_port_an_set(const pal_device_t dev,
                               const pal_dev_port_t dev_port,
                               const pal_autoneg_policy_t::type an) {
    bf_status_t sts;
    bf_pm_port_autoneg_policy_e an_policy;

    switch (an) {
      case pal_autoneg_policy_t::type::BF_AN_DEFAULT:
        an_policy = PM_AN_DEFAULT;
        break;
      case pal_autoneg_policy_t::type::BF_AN_FORCE_ENABLE:
        an_policy = PM_AN_FORCE_ENABLE;
        break;
      case pal_autoneg_policy_t::type::BF_AN_FORCE_DISABLE:
        an_policy = PM_AN_FORCE_DISABLE;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }

    sts = bf_pal_port_autoneg_policy_set(dev, dev_port, an_policy);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  pal_status_t pal_port_an_set_all(const pal_device_t dev,
                                   const pal_autoneg_policy_t::type an) {
    bf_status_t sts;
    bf_pm_port_autoneg_policy_e an_policy;

    switch (an) {
      case pal_autoneg_policy_t::type::BF_AN_DEFAULT:
        an_policy = PM_AN_DEFAULT;
        break;
      case pal_autoneg_policy_t::type::BF_AN_FORCE_ENABLE:
        an_policy = PM_AN_FORCE_ENABLE;
        break;
      case pal_autoneg_policy_t::type::BF_AN_FORCE_DISABLE:
        an_policy = PM_AN_FORCE_DISABLE;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }

    sts = bf_pal_port_autoneg_policy_set_all(dev, an_policy);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  pal_dev_port_t pal_port_get_first(const pal_device_t dev) {
    bf_status_t sts;
    bf_dev_port_t dev_port;

    sts = bf_pal_port_get_first(dev, &dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (pal_dev_port_t)dev_port;
  }
  pal_dev_port_t pal_port_get_next(const pal_device_t dev,
                                   const pal_dev_port_t curr_dev_port) {
    bf_status_t sts;
    bf_dev_port_t next_dev_port;

    sts =
        bf_pal_port_get_next(dev, (bf_dev_port_t)curr_dev_port, &next_dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (pal_dev_port_t)next_dev_port;
  }

  void pal_port_stats_direct_get(pal_stats_t &_return,
                              const pal_device_t dev,
                              const pal_dev_port_t dev_port, const pal_rmon_counters_array_t &ctr_type) { 
    uint64_t *lstats = NULL;
    bf_rmon_counter_t *ctr_type_array = NULL;
    bf_status_t sts;
    int32_t num_of_ctr = ctr_type.array_count;
    if ((num_of_ctr <= 0) || (num_of_ctr > BF_NUM_RMON_COUNTERS)) {
      InvalidPalOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }
    lstats =
        (uint64_t *)bf_sys_calloc(1, sizeof(uint64_t) * num_of_ctr);
    if (lstats == NULL) {
      InvalidPalOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }
    ctr_type_array = (bf_rmon_counter_t *)bf_sys_calloc(1, sizeof(bf_rmon_counter_t) * num_of_ctr);
    if (ctr_type_array == NULL) {
      InvalidPalOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      if (lstats != NULL) {
        bf_sys_free(lstats);
      }
      throw iop;
    }
    for (int32_t i = 0; i < num_of_ctr; i++) {
      sts = pal_check_valid_rmon_counter(ctr_type.array[i], &(ctr_type_array[i]));
      if (sts != BF_SUCCESS) {
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        if (ctr_type_array != NULL) {
          bf_sys_free(ctr_type_array);
        }
        if (lstats != NULL) {
          bf_sys_free(lstats);
        }
        throw iop;
      }
    }
    sts = bf_pal_port_stat_direct_get(dev, dev_port, ctr_type_array, lstats, num_of_ctr);
    _return.status = sts;
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      if (ctr_type_array != NULL) {
        bf_sys_free(ctr_type_array);
      }
      if (lstats != NULL) {
        bf_sys_free(lstats);
      }
      throw iop;
    }
    _return.entry.resize(num_of_ctr);
    for (int32_t i = 0; i < num_of_ctr; i++) {
      _return.entry[i] = (int64_t)lstats[i];
    }
    _return.entry_count = num_of_ctr;
    if (ctr_type_array != NULL) {
      bf_sys_free(ctr_type_array);
    }
    if (lstats != NULL) {
      bf_sys_free(lstats);
    }
  }

  void pal_port_all_stats_get(pal_stats_t &_return,
                              const pal_device_t dev,
                              const pal_dev_port_t dev_port) {
    uint64_t *lstats = NULL;
    lstats =
        (uint64_t *)bf_sys_calloc(1, sizeof(uint64_t) * BF_NUM_RMON_COUNTERS);
    if (lstats == NULL) {
      InvalidPalOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }
    bf_status_t sts = bf_pal_port_all_stats_get(dev, dev_port, lstats);
    _return.status = sts;
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      if (lstats != NULL) {
        bf_sys_free(lstats);
      }
      throw iop;
    }
    _return.entry.resize(BF_NUM_RMON_COUNTERS);
    for (uint32_t i = 0; i < BF_NUM_RMON_COUNTERS; i++) {
      _return.entry[i] = (int64_t)lstats[i];
    }
    _return.entry_count = BF_NUM_RMON_COUNTERS;
    if (lstats != NULL) {
      bf_sys_free(lstats);
    }
  }

  void pal_port_all_stats_get_with_ts(pal_stats_with_ts_t &_return,
                              const pal_device_t dev,
                              const pal_dev_port_t dev_port) {
    uint64_t *lstats = NULL;
    int64_t ts_s = 0, ts_ns = 0;
    lstats =
        (uint64_t *)bf_sys_calloc(1, sizeof(uint64_t) * BF_NUM_RMON_COUNTERS);
    if (lstats == NULL) {
      InvalidPalOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }
    bf_status_t sts = bf_pal_port_all_pure_stats_get_with_timestamp(dev, dev_port, lstats, &ts_s, &ts_ns);
    _return.status = sts;
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      if (lstats != NULL) {
        bf_sys_free(lstats);
      }
      throw iop;
    }
    _return.entry.resize(BF_NUM_RMON_COUNTERS);
    for (uint32_t i = 0; i < BF_NUM_RMON_COUNTERS; i++) {
      _return.entry[i] = (int64_t)lstats[i];
    }
    _return.entry_count = BF_NUM_RMON_COUNTERS;
    _return.timestamp_s = ts_s;
    _return.timestamp_ns = ts_ns;
    if (lstats != NULL) {
      bf_sys_free(lstats);
    }
  }

  int64_t pal_port_this_stat_get(const pal_device_t dev,
                                 const pal_dev_port_t dev_port,
                                 const pal_rmon_counter_t::type ctr_type) {
    bf_status_t sts;
    bf_rmon_counter_t cnt_type;
    sts = pal_check_valid_rmon_counter(ctr_type, &cnt_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }

    uint64_t stat_val;
    sts = bf_pal_port_this_stat_get(dev, dev_port, cnt_type, &stat_val);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (int64_t)stat_val;
  }
  void pal_port_this_stat_id_to_str(pal_string_t &_return,
                                    const pal_rmon_counter_t::type ctr_type) {
    bf_status_t sts;
    bf_rmon_counter_t cnt_type;
    sts = pal_check_valid_rmon_counter(ctr_type, &cnt_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }

    char *c_str = NULL;
    sts = bf_pal_port_this_stat_id_to_str(cnt_type, &c_str);
    _return.status = sts;
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    _return.entry.resize(strlen(c_str) + 1);
    for (uint32_t i = 0; i <= strlen(c_str); i++) {
      _return.entry[i] = c_str[i];
    }
    _return.entry_count = strlen(c_str) + 1;
  }

  pal_status_t pal_port_this_stat_clear(
      const pal_device_t dev,
      const pal_dev_port_t dev_port,
      const pal_rmon_counter_t::type ctr_type) {
    bf_status_t sts;
    bf_rmon_counter_t cnt_type;
    sts = pal_check_valid_rmon_counter(ctr_type, &cnt_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
    }

    sts = bf_pal_port_this_stat_clear(dev, dev_port, cnt_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  pal_status_t pal_port_all_stats_clear(const pal_device_t dev,
                                        const pal_dev_port_t dev_port) {
    bf_status_t sts;

    sts = bf_pal_port_all_stats_clear(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  int32_t pal_port_stats_poll_intvl_get(const pal_device_t dev) {
    uint32_t poll_intvl_ms;
    bf_status_t sts;

    sts = bf_pal_port_stats_poll_intvl_get(dev, &poll_intvl_ms);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop; 
    }            
    
    return (int32_t)poll_intvl_ms;
  }

  pal_status_t pal_port_stats_poll_intvl_set(const pal_device_t dev,
                                             const int32_t poll_intvl_ms) {
    bf_status_t sts;

    sts = bf_pal_port_stats_poll_intvl_set(dev, (uint32_t)poll_intvl_ms);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }
  pal_status_t pal_port_all_stats_update(const pal_device_t dev,
                                         const pal_dev_port_t dev_port) {
    bf_status_t sts;

    sts = bf_pal_port_all_stats_update(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  int32_t pal_port_num_lanes_get(const pal_device_t dev,
                                 const pal_dev_port_t dev_port) {
    bf_status_t sts;
    int num_lanes;

    sts = bf_pal_port_num_lanes_get(dev, dev_port, &num_lanes);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (int32_t)num_lanes;
  }

  void pal_port_mtu_get(pal_mtu_t &_return, const pal_device_t dev,
                                const pal_dev_port_t dev_port) {
    bf_status_t sts;
    uint32_t tx_mtu, rx_mtu;
    sts =
        bf_pal_port_mtu_get(dev, dev_port, &tx_mtu, &rx_mtu);
    _return.status = sts;
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    _return.tx_mtu = (int32_t)tx_mtu;
    _return.rx_mtu = (int32_t)rx_mtu;
  }

  pal_status_t pal_port_mtu_set(const pal_device_t dev,
                                const pal_dev_port_t dev_port,
                                const int32_t tx_mtu,
                                const int32_t rx_mtu) {
    bf_status_t sts;

    sts =
        bf_pal_port_mtu_set(dev, dev_port, (uint32_t)tx_mtu, (uint32_t)rx_mtu);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  pal_status_t pal_port_flow_control_pfc_set(const pal_device_t dev,
                                             const pal_dev_port_t dev_port,
                                             const int32_t tx_en_map,
                                             const int32_t rx_en_map) {
    bf_status_t sts;

    sts = bf_pal_port_flow_control_pfc_set(
        dev, dev_port, (uint32_t)tx_en_map, (uint32_t)rx_en_map);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  void pal_port_flow_control_pfc_get(pal_pfc_t &_return, const pal_device_t dev,
                                             const pal_dev_port_t dev_port) {
    bf_status_t sts;                         
    uint32_t tx_en_map, rx_en_map;
    sts = bf_pal_port_flow_control_pfc_get(
        dev, dev_port, &tx_en_map, &rx_en_map);
    _return.status = sts;
    if (sts != BF_SUCCESS) {             
      InvalidPalOperation iop;
      iop.code = sts;                        
      throw iop;                             
    }                                        
    _return.tx_en_map = (int32_t)tx_en_map;
    _return.rx_en_map = (int32_t)rx_en_map;
  }

  pal_status_t pal_port_flow_control_link_pause_set(
      const pal_device_t dev,
      const pal_dev_port_t dev_port,
      const bool tx_en,
      const bool rx_en) {
    bf_status_t sts;

    sts = bf_pal_port_flow_control_link_pause_set(dev, dev_port, tx_en, rx_en);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  void pal_port_flow_control_link_pause_get(pal_link_pause_t &_return,
      const pal_device_t dev,
      const pal_dev_port_t dev_port) {
    bf_status_t sts;                                
    bool tx_en, rx_en;                                                
    sts = bf_pal_port_flow_control_link_pause_get(dev, dev_port, &tx_en, &rx_en);
    _return.status = sts;
    if (sts != BF_SUCCESS) {                    
      InvalidPalOperation iop;                  
      iop.code = sts;
      throw iop;                                    
    }                                               
    _return.tx_en = tx_en;
    _return.rx_en = rx_en;
  }

  pal_status_t pal_port_fec_set(const pal_device_t dev,
                                const pal_dev_port_t dev_port,
                                const pal_fec_type_t::type fec) {
    bf_fec_type_t fec_type;

    switch (fec) {
      case pal_fec_type_t::type::BF_FEC_TYP_NONE:
        fec_type = BF_FEC_TYP_NONE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_FIRECODE:
        fec_type = BF_FEC_TYP_FIRECODE;
        break;
      case pal_fec_type_t::type::BF_FEC_TYP_REED_SOLOMON:
        fec_type = BF_FEC_TYP_REED_SOLOMON;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    bf_status_t sts;
    sts = bf_pal_port_fec_set(dev, dev_port, fec_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_fec_type_t::type pal_port_fec_get(const pal_device_t dev,
                                const pal_dev_port_t dev_port) {
    bf_fec_type_t fec_type;
    pal_fec_type_t::type fec;
    bf_status_t sts;
    sts = bf_pal_port_fec_get(dev, dev_port, &fec_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    switch (fec_type) {
      case BF_FEC_TYP_NONE:
        fec = pal_fec_type_t::type::BF_FEC_TYP_NONE;
        break;
      case BF_FEC_TYP_FIRECODE:
        fec = pal_fec_type_t::type::BF_FEC_TYP_FIRECODE;
        break;
      case BF_FEC_TYP_REED_SOLOMON:
        fec = pal_fec_type_t::type::BF_FEC_TYP_REED_SOLOMON;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    return fec;
  }

  pal_media_type_t::type pal_port_media_type_get(
      const pal_device_t dev, const pal_dev_port_t dev_port) {
    bf_media_type_t bf_media_type;
    bf_status_t sts;
    sts = bf_pal_port_media_type_get(dev, dev_port, &bf_media_type);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    switch (bf_media_type) {
      case BF_MEDIA_TYPE_COPPER:
        return pal_media_type_t::type::BF_MEDIA_TYPE_COPPER;
      case BF_MEDIA_TYPE_OPTICAL:
        return pal_media_type_t::type::BF_MEDIA_TYPE_OPTICAL;
        break;
      default:
        return pal_media_type_t::type::BF_MEDIA_TYPE_UNKNOWN;
    }
    return pal_media_type_t::type::BF_MEDIA_TYPE_UNKNOWN;
  }

  pal_status_t pal_port_cut_through_enable(const pal_device_t dev,
                                           const pal_dev_port_t dev_port) {
    bf_status_t sts;

    sts = bf_pal_port_cut_through_enable(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  pal_status_t pal_port_cut_through_disable(const pal_device_t dev,
                                            const pal_dev_port_t dev_port) {
    bf_status_t sts;

    sts = bf_pal_port_cut_through_disable(dev, dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return 0;
  }

  bool pal_port_cut_through_enable_status_get(const pal_device_t dev,
                                              const pal_dev_port_t dev_port) {
    bf_status_t sts;
    bool ct_enabled;

    sts = bf_pal_port_cut_through_enable_status_get(dev, dev_port, &ct_enabled);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return ct_enabled;
  }

  int32_t pal_num_pipes_get(const pal_device_t dev) {
    bf_status_t sts;
    uint32_t num_pipes;

    sts = bf_pal_num_pipes_get(dev, &num_pipes);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return (int32_t)num_pipes;
  }

  pal_status_t pal_port_loopback_mode_set(const pal_device_t dev,
                                          const pal_dev_port_t dev_port,
                                          const pal_loopback_mod_t::type mode) {
    bf_status_t sts;
    bf_loopback_mode_e bf_mode;

    switch (mode) {
      case pal_loopback_mod_t::type::BF_LPBK_NONE:
        bf_mode = BF_LPBK_NONE;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_MAC_NEAR:
        bf_mode = BF_LPBK_MAC_NEAR;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_MAC_FAR:
        bf_mode = BF_LPBK_MAC_FAR;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_PCS_NEAR:
        bf_mode = BF_LPBK_PCS_NEAR;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_SERDES_NEAR:
        bf_mode = BF_LPBK_SERDES_NEAR;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_SERDES_FAR:
        bf_mode = BF_LPBK_SERDES_FAR;
        break;
      case pal_loopback_mod_t::type::BF_LPBK_PIPE:
        bf_mode = BF_LPBK_PIPE;
        break;
      default:
        InvalidPalOperation iop;
        iop.code = BF_INVALID_ARG;
        throw iop;
    }
    sts = bf_pal_port_loopback_mode_set(dev, dev_port, bf_mode);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return 0;
  }

  pal_loopback_mod_t::type pal_port_loopback_mode_get(const pal_device_t dev,
                                          const pal_dev_port_t dev_port) {
    bf_status_t sts;
    pal_loopback_mod_t::type mode;
    bf_loopback_mode_e bf_mode;
    sts = bf_pal_port_loopback_mode_get(dev, dev_port, &bf_mode);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    switch (bf_mode) {
      case BF_LPBK_NONE:
        mode = pal_loopback_mod_t::type::BF_LPBK_NONE;
        break;
      case BF_LPBK_MAC_NEAR:
        mode = pal_loopback_mod_t::type::BF_LPBK_MAC_NEAR;
        break;
      case BF_LPBK_MAC_FAR:
        mode = pal_loopback_mod_t::type::BF_LPBK_MAC_FAR;
        break;
      case BF_LPBK_PCS_NEAR:
        mode = pal_loopback_mod_t::type::BF_LPBK_PCS_NEAR;
        break;
      case BF_LPBK_SERDES_NEAR:
        mode = pal_loopback_mod_t::type::BF_LPBK_SERDES_NEAR;
        break;
      case BF_LPBK_SERDES_FAR:
        mode = pal_loopback_mod_t::type::BF_LPBK_SERDES_FAR;
        break;
      case BF_LPBK_PIPE:
        mode = pal_loopback_mod_t::type::BF_LPBK_PIPE;
        break;
      default:
        InvalidPalOperation iop;         
        iop.code = BF_INVALID_ARG;       
        throw iop;
    }
    return mode;
  }

  pal_dev_port_t pal_port_front_panel_port_to_dev_port_get(
      const pal_device_t dev,
      const pal_front_port_t front_port,
      const pal_front_chnl_t front_chnl) {
    bf_status_t sts;
    bf_pal_front_port_handle_t port_hdl;
    bf_dev_port_t dev_port = 0;

    port_hdl.conn_id = front_port;
    port_hdl.chnl_id = front_chnl;

    sts = bf_pal_front_port_to_dev_port_get(dev, &port_hdl, &dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return dev_port;
}

  void pal_port_dev_port_to_front_panel_port_get(pal_front_panel_port_t
&front_panel_port,
                                               const pal_device_t dev,
                                               const pal_dev_port_t dev_port) {
    bf_status_t sts;
    bf_pal_front_port_handle_t port_hdl;

    sts = bf_pal_dev_port_to_front_port_get(dev, dev_port, &port_hdl);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    front_panel_port.pal_front_port = port_hdl.conn_id;
    front_panel_port.pal_front_chnl = port_hdl.chnl_id;
  }

  int32_t pal_max_ports_get(const pal_device_t dev) {
    bf_status_t sts;
    uint32_t max_ports;

    sts = bf_pal_max_ports_get(dev, &max_ports);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (int32_t)max_ports;
  }

int32_t pal_port_get_serdes_lane_count_per_mac(const pal_device_t dev, const pal_dev_port_t dev_port) {
  bf_status_t sts;
  uint32_t serdes_lane_count;

  sts = bf_pal_get_serdes_lane_count_per_mac(dev, dev_port,&serdes_lane_count);
  if (sts != BF_SUCCESS) {
    InvalidPalOperation iop;
    iop.code = sts;
    throw iop;
  }

  return (int32_t)serdes_lane_count;
}

pal_port_serdes_mode_t::type pal_port_serdes_mode_get(const pal_device_t dev, const pal_dev_port_t dev_port) {
  bf_status_t sts;
  pal_port_serdes_mode_t::type serdesMode;
  bf_port_serdes_mode_t serdes_mode;

  sts = bf_pal_get_serdes_mode(dev, dev_port,&serdes_mode);
  if (sts != BF_SUCCESS) {
    InvalidPalOperation iop;
    iop.code = sts;
    throw iop;
  }
  switch(serdes_mode)
  {
    case BF_SERDES_MODE_112G:
      serdesMode = pal_port_serdes_mode_t::type::BF_SERDES_MODE_112G; 
      break;
    case BF_SERDES_MODE_56G:
      serdesMode = pal_port_serdes_mode_t::type::BF_SERDES_MODE_56G;
      break;
    default:
      InvalidPalOperation iop;
      iop.code = BF_INVALID_ARG;
      throw iop;
  }

  return serdesMode;
}

  pal_dev_port_t pal_fp_idx_to_dev_port_map(const pal_device_t dev, const int32_t fp_idx) {
    bf_status_t sts;
    bf_dev_port_t dev_port;

    sts = bf_pal_fp_idx_to_dev_port_map(dev, (uint32_t)fp_idx, &dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (pal_dev_port_t)dev_port;
  }

  pal_dev_port_t pal_recirc_port_to_dev_port_map(const pal_device_t dev, const int32_t recirc_port) {
    bf_status_t sts;
    bf_dev_port_t dev_port;

    sts = bf_pal_recirc_port_to_dev_port_map(dev, (uint32_t)recirc_port, &dev_port);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }

    return (pal_dev_port_t)dev_port;
  }

  void pal_recirc_port_range_get(pal_recirc_ports_t &_return, const pal_device_t dev) {
    bf_status_t sts;
    uint32_t start, end;
    sts = bf_pal_recirc_port_range_get(dev, &start, &end);
    _return.status = sts;
    if (sts == BF_SUCCESS) {
      _return.start_recirc_port = start;
      _return.end_recirc_port = end;
    } else {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
  }

  bool pal_is_port_internal(const pal_device_t dev,
                         const pal_dev_port_t dev_port) {
    bf_status_t sts = BF_SUCCESS;
    bool is_internal = false;
    sts = bf_pal_is_port_internal(dev, dev_port, &is_internal);
    if (sts != BF_SUCCESS) {
      InvalidPalOperation iop;
      iop.code = sts;
      throw iop;
    }
    return is_internal;
  }



};
