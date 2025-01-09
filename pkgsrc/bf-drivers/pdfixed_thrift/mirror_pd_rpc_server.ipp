#include "gen-cpp/mirror.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_mirror.h>
#include <tofino/pdfixed/pd_helper.h>
}

using namespace ::mirror_pd_rpc;
using namespace ::res_pd_rpc;

class mirrorHandler : virtual public mirrorIf {
 private:
  void copy_mirror_session_params(
      const MirrorSessionInfo_t &mirr_sess_info,
      p4_pd_mirror_session_info_t *lmirr_sess_info) {
    memset(lmirr_sess_info, 0, sizeof(p4_pd_mirror_session_info_t));
    switch (mirr_sess_info.mir_type) {
      case PD_MIRROR_TYPE_NORM:
        lmirr_sess_info->type = PD_MIRROR_TYPE_NORM;
        break;
      case PD_MIRROR_TYPE_COAL:
        lmirr_sess_info->type = PD_MIRROR_TYPE_COAL;
        break;
      case PD_MIRROR_TYPE_MAX:
        lmirr_sess_info->type = PD_MIRROR_TYPE_MAX;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }

    switch (mirr_sess_info.direction) {
      case PD_DIR_NONE:
        lmirr_sess_info->dir = PD_DIR_NONE;
        break;
      case PD_DIR_INGRESS:
        lmirr_sess_info->dir = PD_DIR_INGRESS;
        break;
      case PD_DIR_EGRESS:
        lmirr_sess_info->dir = PD_DIR_EGRESS;
        break;
      case PD_DIR_BOTH:
        lmirr_sess_info->dir = PD_DIR_BOTH;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }

    lmirr_sess_info->id = mirr_sess_info.mir_id;
    lmirr_sess_info->egr_port = mirr_sess_info.egr_port;
    lmirr_sess_info->egr_port_v = mirr_sess_info.egr_port_v;
    lmirr_sess_info->egr_port_queue = mirr_sess_info.egr_port_queue;
    lmirr_sess_info->packet_color = (p4_pd_color_t)mirr_sess_info.packet_color;
    lmirr_sess_info->mcast_grp_a = mirr_sess_info.mcast_grp_a;
    lmirr_sess_info->mcast_grp_a_v = mirr_sess_info.mcast_grp_a_v;
    lmirr_sess_info->mcast_grp_b = mirr_sess_info.mcast_grp_b;
    lmirr_sess_info->mcast_grp_b_v = mirr_sess_info.mcast_grp_b_v;
    lmirr_sess_info->max_pkt_len = mirr_sess_info.max_pkt_len;
    lmirr_sess_info->level1_mcast_hash = mirr_sess_info.level1_mcast_hash;
    lmirr_sess_info->level2_mcast_hash = mirr_sess_info.level2_mcast_hash;
    lmirr_sess_info->mcast_l1_xid = mirr_sess_info.mcast_l1_xid;
    lmirr_sess_info->mcast_l2_xid = mirr_sess_info.mcast_l2_xid;
    lmirr_sess_info->mcast_rid = mirr_sess_info.mcast_rid;
    lmirr_sess_info->cos = mirr_sess_info.cos;
    lmirr_sess_info->c2c = mirr_sess_info.c2c;
    lmirr_sess_info->extract_len = mirr_sess_info.extract_len;
    lmirr_sess_info->timeout_usec = mirr_sess_info.timeout_usec;
    lmirr_sess_info->int_hdr_len = mirr_sess_info.int_hdr_len;

    lmirr_sess_info->int_hdr = new uint32_t[4];
    for (unsigned int i = 0; i < mirr_sess_info.int_hdr.size(); i++) {
      lmirr_sess_info->int_hdr[i] = (uint32_t)mirr_sess_info.int_hdr[i];
    }
  }

  void copy_mirror_session_params_back(
                          MirrorSessionInfo_t &mirr_sess_info,
                          const p4_pd_mirror_session_info_t *lmirr_sess_info) {
    switch (lmirr_sess_info->type) {
      case PD_MIRROR_TYPE_NORM:
        mirr_sess_info.mir_type = MirrorType_e::PD_MIRROR_TYPE_NORM;
        break;
      case PD_MIRROR_TYPE_COAL:
        mirr_sess_info.mir_type = MirrorType_e::PD_MIRROR_TYPE_COAL;
        break;
      case PD_MIRROR_TYPE_MAX:
        mirr_sess_info.mir_type = MirrorType_e::PD_MIRROR_TYPE_MAX;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }

    switch (lmirr_sess_info->dir) {
      case PD_DIR_NONE:
        mirr_sess_info.direction = Direction_e::PD_DIR_NONE;
        break;
      case PD_DIR_INGRESS:
        mirr_sess_info.direction = Direction_e::PD_DIR_INGRESS;
        break;
      case PD_DIR_EGRESS:
        mirr_sess_info.direction = Direction_e::PD_DIR_EGRESS;
        break;
      case PD_DIR_BOTH:
        mirr_sess_info.direction = Direction_e::PD_DIR_BOTH;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }

    mirr_sess_info.mir_id = lmirr_sess_info->id;
    mirr_sess_info.egr_port = lmirr_sess_info->egr_port;
    mirr_sess_info.egr_port_v = lmirr_sess_info->egr_port_v;
    mirr_sess_info.egr_port_queue = lmirr_sess_info->egr_port_queue;
    mirr_sess_info.packet_color = (uint16_t)lmirr_sess_info->packet_color;
    mirr_sess_info.mcast_grp_a = lmirr_sess_info->mcast_grp_a;
    mirr_sess_info.mcast_grp_a_v = lmirr_sess_info->mcast_grp_a_v;
    mirr_sess_info.mcast_grp_b = lmirr_sess_info->mcast_grp_b;
    mirr_sess_info.mcast_grp_b_v = lmirr_sess_info->mcast_grp_b_v;
    mirr_sess_info.max_pkt_len = lmirr_sess_info->max_pkt_len;
    mirr_sess_info.level1_mcast_hash = lmirr_sess_info->level1_mcast_hash;
    mirr_sess_info.level2_mcast_hash = lmirr_sess_info->level2_mcast_hash;
    mirr_sess_info.cos = lmirr_sess_info->cos;
    mirr_sess_info.c2c = lmirr_sess_info->c2c;
    mirr_sess_info.extract_len = lmirr_sess_info->extract_len;
    mirr_sess_info.timeout_usec = lmirr_sess_info->timeout_usec;
    mirr_sess_info.int_hdr_len = lmirr_sess_info->int_hdr_len;

    mirr_sess_info.int_hdr = std::vector<int32_t>();
    for (int i = 0; i < mirr_sess_info.int_hdr_len; i++) {
      mirr_sess_info.int_hdr.push_back( (uint32_t)lmirr_sess_info->int_hdr[i] );
    }
  }

 public:
  mirrorHandler() {}

  void mirror_session_create(SessionHandle_t sess_hdl,
                             const DevTarget_t &dev_tgt,
                             const MirrorSessionInfo_t &mirr_sess_info) {
    p4_pd_mirror_session_info_t lmirr_sess_info;
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};

    copy_mirror_session_params(mirr_sess_info, &lmirr_sess_info);

    int status = p4_pd_mirror_session_create(sess_hdl,
                                             d,
                                             &lmirr_sess_info);
    delete[] lmirr_sess_info.int_hdr;
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_update(SessionHandle_t sess_hdl,
                             const DevTarget_t &dev_tgt,
                             const MirrorSessionInfo_t &mirr_sess_info,
                             bool enable) {
    p4_pd_mirror_session_info_t lmirr_sess_info;
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};

    copy_mirror_session_params(mirr_sess_info, &lmirr_sess_info);

    int status =
        p4_pd_mirror_session_update(sess_hdl, d, &lmirr_sess_info, enable);
    delete[] lmirr_sess_info.int_hdr;
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_delete(SessionHandle_t sess_hdl,
                             const DevTarget_t &dev_tgt,
                             MirrorId_t id) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_mirror_session_delete(sess_hdl, d, id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_disable(SessionHandle_t sess_hdl,
                              Direction_e::type dir,
                              const DevTarget_t &dev_tgt,
                              MirrorId_t id) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_direction_t di;
    switch (dir) {
      case PD_DIR_NONE:
        di = PD_DIR_NONE;
        break;
      case PD_DIR_INGRESS:
        di = PD_DIR_INGRESS;
        break;
      case PD_DIR_EGRESS:
        di = PD_DIR_EGRESS;
        break;
      case PD_DIR_BOTH:
        di = PD_DIR_BOTH;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }
    int status = p4_pd_mirror_session_disable(sess_hdl, di, d, id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_enable(SessionHandle_t sess_hdl,
                             Direction_e::type dir,
                             const DevTarget_t &dev_tgt,
                             MirrorId_t id) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_direction_t di;
    switch (dir) {
      case PD_DIR_NONE:
        di = PD_DIR_NONE;
        break;
      case PD_DIR_INGRESS:
        di = PD_DIR_INGRESS;
        break;
      case PD_DIR_EGRESS:
        di = PD_DIR_EGRESS;
        break;
      case PD_DIR_BOTH:
        di = PD_DIR_BOTH;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }
    int status = p4_pd_mirror_session_enable(sess_hdl, di, d, id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_pipe_vec_set(const SessionHandle_t sess_hdl,
                                   const DevTarget_t &dev_tgt,
                                   const MirrorId_t mir_id,
                                   const int32_t pipe_vec) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status =
        p4_pd_mirror_session_pipe_vector_set(sess_hdl, d, mir_id, pipe_vec);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  int32_t mirror_session_pipe_vec_get(const SessionHandle_t sess_hdl,
                                      const DevTarget_t &dev_tgt,
                                      const MirrorId_t mir_id) {
    int ret = 0;
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_mirror_session_pipe_vector_get(sess_hdl, d, mir_id, &ret);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return ret;
  }

  void mirror_session_meta_flag_update(SessionHandle_t sess_hdl,
                                       const DevTarget_t &dev_tgt,
                                       MirrorId_t id,
                                       MetaFlag_e::type flag,
                                       bool value) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    p4_pd_mirror_meta_flag_e mirr_flag;
    switch (flag) {
      case PD_HASH_CFG:
        mirr_flag = PD_HASH_CFG;
        break;
      case PD_HASH_CFG_P:
        mirr_flag = PD_HASH_CFG_P;
        break;
      case PD_ICOS_CFG:
        mirr_flag = PD_ICOS_CFG;
        break;
      case PD_DOD_CFG:
        mirr_flag = PD_DOD_CFG;
        break;
      case PD_C2C_CFG:
        mirr_flag = PD_C2C_CFG;
        break;
      case PD_MC_CFG:
        mirr_flag = PD_MC_CFG;
        break;
      case PD_EPIPE_CFG:
        mirr_flag = PD_EPIPE_CFG;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }
    int status = p4_pd_mirror_session_meta_flag_update(sess_hdl, d, id, mirr_flag, value);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_priority_update(SessionHandle_t sess_hdl,
                                       const DevTarget_t &dev_tgt,
                                       MirrorId_t id,
                                       bool value) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_mirror_session_priority_update(sess_hdl, d, id, value);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_coal_mode_update(SessionHandle_t sess_hdl,
                                       const DevTarget_t &dev_tgt,
                                       MirrorId_t id,
                                       bool value) {
    p4_pd_dev_target_t d = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    int status = p4_pd_mirror_session_coal_mode_update(sess_hdl, d, id, value);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
  }

  void mirror_session_get_first(MirrorSessionGetResult_t &_return,
                                const ::res_pd_rpc::SessionHandle_t sess_hdl,
                                const ::res_pd_rpc::DevTarget_t &dev_tgt) {
    p4_pd_mirror_session_info_t mirr_sess_info;
    mirr_sess_info.int_hdr = new uint32_t[4];
    p4_pd_dev_target_t dt = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    bf_dev_pipe_t pipe_id;
    int status =
        p4_pd_mirror_session_get_first(sess_hdl, dt, &mirr_sess_info, &pipe_id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      delete[] mirr_sess_info.int_hdr;
      throw iop;
    }
    copy_mirror_session_params_back(_return.info, &mirr_sess_info);
    _return.pipe_id = pipe_id;
    delete[] mirr_sess_info.int_hdr;
  }

  void mirror_session_get_next(MirrorSessionGetResult_t &_return,
                               const ::res_pd_rpc::SessionHandle_t sess_hdl,
                               const ::res_pd_rpc::DevTarget_t &dev_tgt,
                               const MirrorId_t cur_id,
                               const int16_t cur_pipe_id) {
    p4_pd_mirror_session_info_t next_mirr_sess_info;
    next_mirr_sess_info.int_hdr = new uint32_t[4];
    p4_pd_dev_target_t dt = {dev_tgt.dev_id, i16_to_bf_pipe(dev_tgt.dev_pipe_id)};
    bf_dev_pipe_t pipe_id;
    int status = p4_pd_mirror_session_get_next(sess_hdl,
                                               dt,
                                               (p4_pd_mirror_id_t)cur_id,
                                               i16_to_bf_pipe(cur_pipe_id),
                                               &next_mirr_sess_info,
                                               &pipe_id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      delete[] next_mirr_sess_info.int_hdr;
      throw iop;
    }
    copy_mirror_session_params_back(_return.info, &next_mirr_sess_info);
    _return.pipe_id = pipe_id;
    delete[] next_mirr_sess_info.int_hdr;
  }

  MirrorId_t mirror_session_get_max_session_id(SessionHandle_t sess_hdl,
                                               const int dev,
                                               const MirrorType_e::type mir_type) {
    p4_pd_mirror_type_e type;
    p4_pd_mirror_id_t id;
    switch (mir_type) {
      case PD_MIRROR_TYPE_NORM:
        type = PD_MIRROR_TYPE_NORM;
        break;
      case PD_MIRROR_TYPE_COAL:
        type = PD_MIRROR_TYPE_COAL;
        break;
      case PD_MIRROR_TYPE_MAX:
        type = PD_MIRROR_TYPE_MAX;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }
    int status = p4_pd_mirror_session_max_session_id_get(sess_hdl, dev, type, &id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return id;
  }

  MirrorId_t mirror_session_get_base_session_id(SessionHandle_t sess_hdl,
                                                const int dev,
                                                const MirrorType_e::type mir_type) {
    p4_pd_mirror_type_e type;
    p4_pd_mirror_id_t id;
    switch (mir_type) {
      case PD_MIRROR_TYPE_NORM:
        type = PD_MIRROR_TYPE_NORM;
        break;
      case PD_MIRROR_TYPE_COAL:
        type = PD_MIRROR_TYPE_COAL;
        break;
      case PD_MIRROR_TYPE_MAX:
        type = PD_MIRROR_TYPE_MAX;
        break;
      default:
        InvalidPipeMgrOperation iop;
        iop.code = 1;
        throw iop;
    }
    int status = p4_pd_mirror_session_base_session_id_get(sess_hdl, dev, type, &id);
    if (status != 0) {
      InvalidPipeMgrOperation iop;
      iop.code = status;
      throw iop;
    }
    return id;
  }
};
