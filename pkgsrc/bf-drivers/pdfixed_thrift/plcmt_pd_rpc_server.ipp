#include "gen-cpp/plcmt.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_plcmt.h>
#include <tofino/pdfixed/pd_helper.h>
}

using namespace ::plcmt_pd_rpc;
using namespace ::res_pd_rpc;

class plcmtHandler : virtual public plcmtIf {
 public:
  plcmtHandler() {}

  static inline const void* find_mat_data(const mat_tbl_update_t &u) {
    const void *rv = nullptr;
    switch (u.update_type) {
      case mat_update_type::type::MAT_UPDATE_ADD:
        rv = static_cast<const void*>(u.update_params.add.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_ADD_MULTI:
        rv = static_cast<const void*>(u.update_params.add_multi.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_SET_DFLT:
        rv = static_cast<const void*>(u.update_params.set_dflt.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_MOD:
        rv = static_cast<const void*>(u.update_params.mod.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_MOV:
        rv = static_cast<const void*>(u.update_params.mov.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_MOV_MULTI:
        rv = static_cast<const void*>(u.update_params.mov_multi.drv_data.data());
        break;
      case mat_update_type::type::MAT_UPDATE_DEL:
      case mat_update_type::type::MAT_UPDATE_CLR_DFLT:
        break;
    }
    return rv;
  }

  static uint8_t *append_mat_plcmt_data(struct p4_pd_plcmt_info *x,
                                        const mat_tbl_update_t &u) {
    p4_pd_status_t sts = PIPE_SUCCESS;

    /* Create an unpacked copy of the data. */
    const void *packed_data = find_mat_data(u);
    uint8_t *unpacked_data = nullptr;
    size_t unpacked_sz = 0;
    if (packed_data) {
      sts = p4_pd_plcmt_unpack_data_size(packed_data, &unpacked_sz);
      bf_sys_assert(!sts);
      unpacked_data = new uint8_t[unpacked_sz];
      bf_sys_assert(unpacked_data);
      sts = p4_pd_plcmt_copy_unpack(packed_data, unpacked_data);
      bf_sys_assert(!sts);
    }

    switch (u.update_type) {
      case mat_update_type::type::MAT_UPDATE_ADD:
        sts = p4_pd_set_one_plcmt_op_mat_add(
            x,
            u.update_params.add.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.add.entry_index,
            u.update_params.add.selection_index,
            u.update_params.add.action_index,
            static_cast<void *>(unpacked_data));
        break;
      case mat_update_type::type::MAT_UPDATE_ADD_MULTI: {
        struct p4_pd_multi_index *loc =
            new struct p4_pd_multi_index[u.update_params.add_multi.locations.size()];
        for (unsigned int i = 0; i < u.update_params.add_multi.locations.size();
             ++i) {
          loc[i].logical_index_base =
              u.update_params.add_multi.locations[i].base_index;
          loc[i].logical_index_count =
              u.update_params.add_multi.locations[i].index_count;
        }
        sts = p4_pd_set_one_plcmt_op_mat_add_multi(
            x,
            u.update_params.add_multi.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.add_multi.selection_index,
            u.update_params.add_multi.action_index,
            u.update_params.add_multi.locations.size(),
            loc,
            static_cast<void *>(unpacked_data));
        delete[] loc;
        break;
      }
      case mat_update_type::type::MAT_UPDATE_SET_DFLT:
        sts = p4_pd_set_one_plcmt_op_mat_set_dflt(
            x,
            u.update_params.set_dflt.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.set_dflt.selection_index,
            u.update_params.set_dflt.action_index,
            static_cast<void *>(unpacked_data));
        break;
      case mat_update_type::type::MAT_UPDATE_CLR_DFLT:
        sts = p4_pd_set_one_plcmt_op_mat_clr_dflt(
            x,
            u.update_params.clr_dflt.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
        break;
      case mat_update_type::type::MAT_UPDATE_DEL:
        sts =
            p4_pd_set_one_plcmt_op_mat_del(x, u.update_params.remove.entry_hdl);
        break;
      case mat_update_type::type::MAT_UPDATE_MOD:
        sts =
            p4_pd_set_one_plcmt_op_mat_mod(x,
                                           u.update_params.mod.entry_hdl,
                                           u.update_params.mod.selection_index,
                                           u.update_params.mod.action_index,
                                           static_cast<void *>(unpacked_data));
        break;
      case mat_update_type::type::MAT_UPDATE_MOV:
        sts =
            p4_pd_set_one_plcmt_op_mat_mov(x,
                                           u.update_params.mov.entry_hdl,
                                           u.update_params.mov.entry_index,
                                           u.update_params.mov.selection_index,
                                           u.update_params.mov.action_index,
                                           static_cast<void *>(unpacked_data));
        break;
      case mat_update_type::type::MAT_UPDATE_MOV_MULTI: {
        struct p4_pd_multi_index *loc =
            new struct p4_pd_multi_index[u.update_params.mov_multi.locations.size()];
        for (unsigned int i = 0; i < u.update_params.mov_multi.locations.size();
             ++i) {
          loc[i].logical_index_base =
              u.update_params.mov_multi.locations[i].base_index;
          loc[i].logical_index_count =
              u.update_params.mov_multi.locations[i].index_count;
        }
        sts = p4_pd_set_one_plcmt_op_mat_mov_multi(
            x,
            u.update_params.mov_multi.entry_hdl,
            u.update_params.mov_multi.selection_index,
            u.update_params.mov_multi.action_index,
            u.update_params.mov_multi.locations.size(),
            loc,
            static_cast<void *>(unpacked_data));
        delete[] loc;
        break;
      }
      default:
        bf_sys_assert(0);
        break;
    }
    if (sts) {
      InvalidPlcmtOperation iop;
      iop.code = sts;
      throw iop;
    }
    return unpacked_data;
  }

  static inline const void *find_adt_data(const adt_tbl_update_t &u) {
    const void *rv = nullptr;
    switch (u.update_type) {
      case adt_update_type::type::ADT_UPDATE_ADD:
        rv = static_cast<const void *>(u.update_params.add.drv_data.data());
        break;
      case adt_update_type::type::ADT_UPDATE_MOD:
        rv = static_cast<const void *>(u.update_params.mod.drv_data.data());
        break;
      case adt_update_type::type::ADT_UPDATE_DEL:
        break;
    }
    return rv;
  }
  static uint8_t *append_adt_plcmt_data(struct p4_pd_plcmt_info *x,
                                        const adt_tbl_update_t &u) {
    p4_pd_status_t sts = PIPE_SUCCESS;

    /* Create an unpacked copy of the data. */
    const void *packed_data = find_adt_data(u);
    uint8_t *unpacked_data = nullptr;
    size_t unpacked_sz = 0;
    if (packed_data) {
      sts = p4_pd_plcmt_unpack_data_size(packed_data, &unpacked_sz);
      bf_sys_assert(!sts);
      unpacked_data = new uint8_t[unpacked_sz];
      bf_sys_assert(unpacked_data);
      sts = p4_pd_plcmt_copy_unpack(packed_data, unpacked_data);
      bf_sys_assert(!sts);
    }

    switch (u.update_type) {
      case adt_update_type::type::ADT_UPDATE_ADD:
        sts = p4_pd_set_one_plcmt_op_adt_add(
            x,
            u.update_params.add.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            static_cast<void *>(unpacked_data));
        break;
      case adt_update_type::type::ADT_UPDATE_DEL:
        sts =
            p4_pd_set_one_plcmt_op_adt_del(x, u.update_params.remove.entry_hdl);
        break;
      case adt_update_type::type::ADT_UPDATE_MOD:
        sts =
            p4_pd_set_one_plcmt_op_adt_mod(x,
                                           u.update_params.mod.entry_hdl,
                                           static_cast<void *>(unpacked_data));
        break;
      default:
        bf_sys_assert(0);
        break;
    }
    if (sts) {
      InvalidPlcmtOperation iop;
      iop.code = sts;
      throw iop;
    }
    return unpacked_data;
  }

  static void process_sel_plcmt_data(struct p4_pd_plcmt_info *x,
                                     const sel_tbl_update_t &u) {
    p4_pd_status_t sts = PIPE_SUCCESS;
    void *drv_data = nullptr;
    switch (u.update_type) {
      case sel_update_type::type::SEL_UPDATE_GROUP_CREATE: {
        struct p4_pd_multi_index *loc =
            new struct p4_pd_multi_index[u.update_params.grp_create.locations
                                             .size()];
        for (unsigned int i = 0;
             i < u.update_params.grp_create.locations.size();
             ++i) {
          loc[i].logical_index_base =
              u.update_params.grp_create.locations[i].base_index;
          loc[i].logical_index_count =
              u.update_params.grp_create.locations[i].index_count;
        }
        sts = p4_pd_set_one_plcmt_op_sel_grp_create(
            x,
            u.update_params.grp_create.group_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.grp_create.num_indexes,
            u.update_params.grp_create.max_members,
            u.update_params.grp_create.base_logical_index,
            u.update_params.grp_create.locations.size(),
            loc);
        delete[] loc;
        break;
      }
      case sel_update_type::type::SEL_UPDATE_GROUP_DESTROY:
        sts = p4_pd_set_one_plcmt_op_sel_grp_destroy(
            x, u.update_params.grp_destroy.group_hdl, i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
        break;
      case sel_update_type::type::SEL_UPDATE_ADD:
        sts = p4_pd_plcmt_duplicate(
            const_cast<char *>(u.update_params.add.drv_data.data()), &drv_data);
        bf_sys_assert(!sts);
        sts = p4_pd_set_one_plcmt_op_sel_add(x,
                                             u.update_params.add.group_hdl,
                                             u.update_params.add.entry_hdl,
                                             i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
                                             u.update_params.add.entry_index,
                                             u.update_params.add.entry_subindex,
                                             drv_data);
        break;
      case sel_update_type::type::SEL_UPDATE_DEL:
        sts = p4_pd_set_one_plcmt_op_sel_del(
            x,
            u.update_params.remove.group_hdl,
            u.update_params.remove.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.remove.entry_index,
            u.update_params.remove.entry_subindex);
        break;
      case sel_update_type::type::SEL_UPDATE_ACTIVATE:
        sts = p4_pd_set_one_plcmt_op_sel_activate(
            x,
            u.update_params.activate.group_hdl,
            u.update_params.activate.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.activate.entry_index,
            u.update_params.activate.entry_subindex);
        break;
      case sel_update_type::type::SEL_UPDATE_DEACTIVATE:
        sts = p4_pd_set_one_plcmt_op_sel_deactivate(
            x,
            u.update_params.deactivate.group_hdl,
            u.update_params.deactivate.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            u.update_params.deactivate.entry_index,
            u.update_params.deactivate.entry_subindex);
        break;
      case sel_update_type::type::SEL_UPDATE_SET_FALLBACK:
        sts = p4_pd_plcmt_duplicate(
            const_cast<char *>(u.update_params.set_fallback.drv_data.data()),
            &drv_data);
        bf_sys_assert(!sts);
        sts = p4_pd_set_one_plcmt_op_sel_set_fallback(
            x,
            u.update_params.set_fallback.entry_hdl,
            i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
            drv_data);
        break;
      case sel_update_type::type::SEL_UPDATE_CLR_FALLBACK:
        sts = p4_pd_set_one_plcmt_op_sel_clr_fallback(x, i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
        break;
      default:
        bf_sys_assert(0);
        break;
    }
    if (sts) {
      InvalidPlcmtOperation iop;
      iop.code = sts;
      throw iop;
    }
  }

  void process_plcmt_data(const ::res_pd_rpc::SessionHandle_t sess_hdl,
                          const int8_t dev_id,
                          const std::vector<tbl_update_t> &updates) {
    struct p4_pd_plcmt_info *x = p4_pd_create_plcmt_info();
    bf_sys_assert(x);
    int tbl_hdl = 0;
    uint32_t num_found = 0;
    p4_pd_status_t sts = PIPE_SUCCESS;
    std::vector<uint8_t*> to_free(updates.size());

    for (auto &u : updates) {
      const mat_tbl_update_t &mat = u.update_data.mat;
      const adt_tbl_update_t &adt = u.update_data.adt;
      const sel_tbl_update_t &sel = u.update_data.sel;
      uint8_t *data = nullptr;
      switch (u.update_type) {
        case tbl_update_type::type::MAT_UPDATE_TYPE:
          if (tbl_hdl && tbl_hdl != mat.tbl_hdl) {
            // This update is for a new table, push what is already collected
            // and start a new list.
            uint32_t num_processed = 0;
            sts = p4_pd_process_plcmt_info(
                sess_hdl, dev_id, tbl_hdl, x, 0, &num_processed);
            if (sts != 0 || num_processed != num_found) {
              InvalidPlcmtOperation iop;
              iop.code = sts;
              throw iop;
            }
            p4_pd_destroy_plcmt_info(x);
            x = p4_pd_create_plcmt_info();
            bf_sys_assert(x);
            num_found = 0;
          }
          tbl_hdl = mat.tbl_hdl;
          data = append_mat_plcmt_data(x, mat);
          to_free.push_back(data);
          break;
        case tbl_update_type::type::ADT_UPDATE_TYPE:
          if (tbl_hdl && tbl_hdl != adt.tbl_hdl) {
            // This update is for a new table, push what is already collected
            // and start a new list.
            uint32_t num_processed = 0;
            sts = p4_pd_process_plcmt_info(
                sess_hdl, dev_id, tbl_hdl, x, 0, &num_processed);
            if (sts != 0 || num_processed != num_found) {
              InvalidPlcmtOperation iop;
              iop.code = sts;
              throw iop;
            }
            p4_pd_destroy_plcmt_info(x);
            x = p4_pd_create_plcmt_info();
            bf_sys_assert(x);
            num_found = 0;
          }
          tbl_hdl = adt.tbl_hdl;
          data = append_adt_plcmt_data(x, adt);
          to_free.push_back(data);
          break;
        case tbl_update_type::type::SEL_UPDATE_TYPE:
          if (tbl_hdl && tbl_hdl != sel.tbl_hdl) {
            // This update is for a new table, push what is already collected
            // and start a new list.
            uint32_t num_processed = 0;
            sts = p4_pd_process_plcmt_info(
                sess_hdl, dev_id, tbl_hdl, x, 0, &num_processed);
            if (sts != 0 || num_processed != num_found) {
              InvalidPlcmtOperation iop;
              iop.code = sts;
              throw iop;
            }
            p4_pd_destroy_plcmt_info(x);
            x = p4_pd_create_plcmt_info();
            bf_sys_assert(x);
            num_found = 0;
          }
          tbl_hdl = sel.tbl_hdl;
          process_sel_plcmt_data(x, sel);
          break;
      }
      ++num_found;
    }
    if (num_found) {
      uint32_t num_processed = 0;
      sts = p4_pd_process_plcmt_info(
          sess_hdl, dev_id, tbl_hdl, x, 0, &num_processed);
      if (sts != 0 || num_processed != num_found) {
        InvalidPlcmtOperation iop;
        iop.code = sts;
        throw iop;
      }
      p4_pd_destroy_plcmt_info(x);
    }
    for (auto &p : to_free) {
      if (p != nullptr) {
        delete[] p;
      }
    }
  }

  void replace_adt_ent_hdl(std::string &ret,
                           const std::string &drv_data,
                           int32_t adt_ent_hdl) {
    void *data = NULL;
    size_t data_size = 0;
    p4_pd_status_t sts = PIPE_SUCCESS;
    sts = p4_pd_plcmt_duplicate(const_cast<char *>(drv_data.data()), &data);
    bf_sys_assert(!sts);
    p4_pd_plcmt_data_size(data, &data_size);
    sts = p4_pd_plcmt_set_adt_ent_hdl(data, adt_ent_hdl);
    bf_sys_assert(sts == 0);
    ret.assign(static_cast<const char *>(data), data_size);
    return;
  }

  void replace_sel_grp_hdl(std::string &ret,
                           const std::string &drv_data,
                           int32_t sel_grp_hdl) {
    void *data = NULL;
    size_t data_size = 0;
    p4_pd_status_t sts = PIPE_SUCCESS;
    sts = p4_pd_plcmt_duplicate(const_cast<char *>(drv_data.data()), &data);
    bf_sys_assert(!sts);
    p4_pd_plcmt_data_size(data, &data_size);
    sts = p4_pd_plcmt_set_sel_grp_hdl(data, sel_grp_hdl);
    bf_sys_assert(sts == 0);
    ret.assign(static_cast<const char *>(data), data_size);
    return;
  }

  void replace_ttl(std::string &ret, const std::string &drv_data, int32_t ttl) {
    void *data = NULL;
    size_t data_size = 0;
    p4_pd_status_t sts = PIPE_SUCCESS;
    sts = p4_pd_plcmt_duplicate(const_cast<char *>(drv_data.data()), &data);
    bf_sys_assert(!sts);
    p4_pd_plcmt_data_size(data, &data_size);
    sts = p4_pd_plcmt_set_ttl(data, ttl);
    bf_sys_assert(sts == 0);
    ret.assign(static_cast<const char *>(data), data_size);
    return;
  }
};
