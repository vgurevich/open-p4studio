//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include <iostream>

#include "p4_prefix.h"

#include <string.h>

extern "C" {
#include <pd/pd.h>
}

#include <list>
#include <map>
#include <pthread.h>

using namespace  ::p4_pd_rpc;
using namespace  ::res_pd_rpc;


class ${p4_prefix}Handler : virtual public ${p4_prefix}If {
private:
  struct PipeMgrSimpleCb {
    // Should I simply use to C++11 mutex / condvars
    pthread_mutex_t cb_mutex;
    pthread_cond_t cb_condvar;
    int cb_status;

    PipeMgrSimpleCb()
      : cb_status(0) {
      pthread_mutex_init(&cb_mutex, NULL);
      pthread_cond_init(&cb_condvar, NULL);
    }

    ~PipeMgrSimpleCb() {
      pthread_mutex_destroy(&cb_mutex);
      pthread_cond_destroy(&cb_condvar);
    }

    int wait() {
      pthread_mutex_lock(&cb_mutex);
      while(cb_status == 0) {
        pthread_cond_wait(&cb_condvar, &cb_mutex);
      }
      pthread_mutex_unlock(&cb_mutex);
      return 0;
    }

    void notify() {
      pthread_mutex_lock(&cb_mutex);
      assert(cb_status == 0);
      cb_status = 1;
      pthread_cond_signal(&cb_condvar);
      pthread_mutex_unlock(&cb_mutex);
    }

    static void cb_fn(int device_id, void *cookie) {
      (void) device_id;
      PipeMgrSimpleCb *inst = (PipeMgrSimpleCb *) cookie;
      inst->notify();
    }

    // C++11
    // PipeMgrSimpleCb(const PipeMgrSimpleCb &other) = delete;
    // PipeMgrSimpleCb &operator=(const PipeMgrSimpleCb &other) = delete;

    // PipeMgrSimpleCb(PipeMgrSimpleCb &&other) = delete;
    // PipeMgrSimpleCb &operator=(PipeMgrSimpleCb &&other) = delete;

  private:
    PipeMgrSimpleCb(const PipeMgrSimpleCb &other);
    PipeMgrSimpleCb &operator=(const PipeMgrSimpleCb &other);
  };

public:
    ${p4_prefix}Handler() {
        // Your initialization goes here
//:: for lq in learn_quanta:
        pthread_mutex_init(&${lq["name"]}_mutex, NULL);
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not t_info["timeout"]: continue
        pthread_mutex_init(&_${table}_update_hit_state_m, NULL);
        pthread_cond_init(&_${table}_update_hit_state_receive_cv, NULL);
        pthread_cond_init(&_${table}_update_hit_state_respond_cv, NULL);
        _${table}_update_hit_state_reg = 0;
        _${table}_update_hit_state_seq_id = 0;
        pthread_mutex_init(&_${table}_idle_tmo_expired_m, NULL);
//:: #endfor

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)
//::   if not select_hdl: continue
        pthread_mutex_init(&_${table}_sel_update_m, NULL);
//:: #endfor
    }

//:: # match_fields is list of tuples (name, type)
//:: def gen_match_params(match_fields, field_info):
//::   params = []
//::   for field, type in match_fields:
//::     if type[:6] == "valid_":
//::       params += [(field + "_valid", 1)]
//::       if type[6:] == "ternary":
//::         params += [(field + "_valid_mask", 1)]
//::       #endif
//::       continue
//::     #endif
//::     f_info = field_info[field]
//::     bytes_needed = (f_info["bit_width"] + 7 ) // 8
//::     if type != "range":
//::        params += [(field, bytes_needed)]
//::     else:
//::        params += [(field + "_start", bytes_needed)]
//::     #endif
//::     if type == "lpm": params += [(field + "_prefix_length", 2)]
//::     if type == "ternary" : params += [(field + "_mask", bytes_needed)]
//::     if type == "range" : params += [(field + "_end", bytes_needed)]
//::   #endfor
//::   return params
//:: #enddef
//::
//:: def gen_action_params(names, byte_widths):
//::   params = []
//::   for name, width in zip(names, byte_widths):
//::     name = "action_" + name
//::     params += [(name, width)]
//::   #endfor
//::   return params
//:: #enddef

    // Idle time config

    typedef struct _update_hit_state_cookie_s {
      ${p4_prefix}Handler *handler;
      uint64_t seq_id;
    } _update_hit_state_cookie_t;

    typedef struct _update_idle_tmo_expired_cookie_s {
      ${p4_prefix}Handler *handler;
      int32_t cookie;
    } _update_idle_tmo_expired_cookie_t;

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not t_info["timeout"]: continue

    pthread_mutex_t _${table}_idle_tmo_expired_m;
    std::map<int, std::vector<${api_prefix}idle_tmo_expired_t> > _${table}_idle_tmo_expired_entries;

    void _${table}_idle_tmo_expired_receive(int dev_id,
                                            p4_pd_entry_hdl_t entry_hdl,
                                            int32_t cookie) {
      pthread_mutex_lock(&_${table}_idle_tmo_expired_m);
      ${api_prefix}idle_tmo_expired_t expired_entry;
      expired_entry.dev_id = dev_id;
      expired_entry.entry = entry_hdl;
      expired_entry.cookie = cookie;
      _${table}_idle_tmo_expired_entries[dev_id].push_back(expired_entry);
      pthread_mutex_unlock(&_${table}_idle_tmo_expired_m);
    }

    static void _${table}_idle_tmo_expired_cb(int dev_id,
                                              p4_pd_entry_hdl_t entry_hdl,
                                              p4_pd_idle_time_hit_state_e hs,
                                              void *cookie) {
      // Legacy solutions and tests do not support or expect 2 way notifications.
      if (hs == ENTRY_ACTIVE) return;
      _update_idle_tmo_expired_cookie_t *data = (_update_idle_tmo_expired_cookie_t *) cookie;
      data->handler->_${table}_idle_tmo_expired_receive(dev_id, entry_hdl, data->cookie);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const " + api_prefix + "idle_time_params_t& params"]
//::   param_str = ", ".join(params)
//::   name = table + "_idle_tmo_enable"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_idle_time_params_t pd_params;
      pd_params.mode = (p4_pd_idle_time_mode_e) params.mode;
      if(pd_params.mode == PD_NOTIFY_MODE) {
        pd_params.params.notify.ttl_query_interval = params.ttl_query_interval;
        pd_params.params.notify.max_ttl = params.max_ttl;
        pd_params.params.notify.min_ttl = params.min_ttl;
        pd_params.params.notify.callback_fn = _${table}_idle_tmo_expired_cb;
        // this will leak
        _update_idle_tmo_expired_cookie_t *data =
          (_update_idle_tmo_expired_cookie_t *) malloc(sizeof(_update_idle_tmo_expired_cookie_t));
        data->handler = this;
        data->cookie = params.cookie;
        pd_params.params.notify.cookie = data;
      }
      int status = ${pd_name}(sess_hdl, dev_id, pd_params);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const int32_t cookie"]
//::   param_str = ", ".join(params)
//::   name = table + "_idle_register_tmo_cb"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
  // this will leak
      _update_idle_tmo_expired_cookie_t *data =
        (_update_idle_tmo_expired_cookie_t *) malloc(sizeof(_update_idle_tmo_expired_cookie_t));
      data->handler = this;
      data->cookie = cookie;
      int status = ${pd_name}(sess_hdl, dev_id, _${table}_idle_tmo_expired_cb, data);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   params = ["std::vector<" + api_prefix + "idle_tmo_expired_t> &expired",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_idle_tmo_get_expired"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      (void) sess_hdl;
      pthread_mutex_lock(&_${table}_idle_tmo_expired_m);
      expired.swap(_${table}_idle_tmo_expired_entries[dev_id]);
      pthread_mutex_unlock(&_${table}_idle_tmo_expired_m);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_idle_tmo_disable"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      int status = ${pd_name}(sess_hdl, dev_id);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   params = [api_prefix + "idle_time_params_t &params",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_idle_params_get"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_idle_time_params_t pd_params;
      int status = ${pd_name}(sess_hdl, dev_id, &pd_params);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
      switch(pd_params.mode) {
        case PD_POLL_MODE:
          params.mode = ${api_prefix}idle_time_mode::POLL_MODE;
          break;
        case PD_NOTIFY_MODE:
          params.mode = ${api_prefix}idle_time_mode::NOTIFY_MODE;
          break;
        case PD_INVALID_MODE:
          params.mode = ${api_prefix}idle_time_mode::INVALID_MODE;
          break;
      }
      if(pd_params.mode == PD_NOTIFY_MODE) {
        params.__set_ttl_query_interval(pd_params.params.notify.ttl_query_interval);
        params.__set_max_ttl(pd_params.params.notify.max_ttl);
        params.__set_min_ttl(pd_params.params.notify.min_ttl);
        params.__set_cookie(((_update_idle_tmo_expired_cookie_t *)pd_params.params.notify.cookie)->cookie);
      }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry",
//::             "const int32_t ttl"]
//::   param_str = ", ".join(params)
//::   name = table + "_set_ttl"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      int status = ${pd_name}(sess_hdl, dev_id, entry, ttl);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry"]
//::   param_str = ", ".join(params)
//::   name = table + "_get_ttl"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
      int32_t ttl;
      ${pd_name}(sess_hdl, dev_id, entry, (uint32_t *) &ttl);
      return ttl;
    }

    pthread_mutex_t _${table}_update_hit_state_m;
    pthread_cond_t _${table}_update_hit_state_respond_cv;
    pthread_cond_t _${table}_update_hit_state_receive_cv;
    uint64_t _${table}_update_hit_state_reg;
    uint64_t _${table}_update_hit_state_seq_id;

    void _${table}_update_hit_state_receive(int dev_id, uint64_t seq_id) {
      (void) dev_id;
      pthread_mutex_lock(&_${table}_update_hit_state_m);
      while(_${table}_update_hit_state_reg != 0) {
        pthread_cond_wait(&_${table}_update_hit_state_receive_cv,
                          &_${table}_update_hit_state_m);
      }
      _${table}_update_hit_state_reg = seq_id;
      pthread_cond_broadcast(&_${table}_update_hit_state_respond_cv);
      pthread_mutex_unlock(&_${table}_update_hit_state_m);
    }

    static void _${table}_update_hit_state_cb(int dev_id, void *cookie) {
      _update_hit_state_cookie_t *data = (_update_hit_state_cookie_t *) cookie;
      data->handler->_${table}_update_hit_state_receive(dev_id, data->seq_id);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry"]
//::   param_str = ", ".join(params)
//::   name = table + "_reset_ttl"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      int status = ${pd_name}(sess_hdl, dev_id, entry);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_update_hit_state"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      pthread_mutex_lock(&_${table}_update_hit_state_m);
      uint64_t my_seq_id = ++_${table}_update_hit_state_seq_id;
      _update_hit_state_cookie_t cookie = {this, my_seq_id};
      int status = ${pd_name}(sess_hdl, dev_id, _${table}_update_hit_state_cb, &cookie);
      if(status != 0) {
        pthread_mutex_unlock(&_${table}_update_hit_state_m);
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
      while(_${table}_update_hit_state_reg != my_seq_id) {
        pthread_cond_wait(&_${table}_update_hit_state_respond_cv,
                          &_${table}_update_hit_state_m);
      }
      _${table}_update_hit_state_reg = 0;
      pthread_cond_signal(&_${table}_update_hit_state_receive_cv);
      pthread_mutex_unlock(&_${table}_update_hit_state_m);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry"]
//::   param_str = ", ".join(params)
//::   name = table + "_get_hit_state"
//::   pd_name = pd_prefix + name
    ${api_prefix}idle_time_hit_state::type ${name}(${param_str}) {

      p4_pd_idle_time_hit_state_e hit_state;
      ${pd_name}(sess_hdl, dev_id, entry, &hit_state);
      return (${api_prefix}idle_time_hit_state::type) hit_state;
    }

//:: #endfor

//:: if gen_md_pd:
//::   queue_name = pd_prefix + "table_update_vec"
    static std::map<int, std::vector<${api_prefix}tbl_update_t> > ${queue_name};

//::   params = ["std::vector<" + api_prefix + "tbl_update_t> & _return",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = "get_tbl_updates"
    void ${name}(${param_str}) {
      _return.swap( ${queue_name}[dev_id] );
    }

    // MAT Update Tracking
    static uint8_t* pack_ent_data(const void *unpacked_data, size_t &packed_sz) {
      p4_pd_status_t sts;
      size_t unpacked_sz = 0;
      sts = p4_pd_plcmt_data_size(unpacked_data, &unpacked_sz);
      assert(0 == sts);
      sts = p4_pd_plcmt_pack_data_size(unpacked_data, &packed_sz);
      assert(0 == sts);
      assert(packed_sz < unpacked_sz);
      uint8_t *packed_data = new uint8_t[packed_sz];
      assert(packed_data != nullptr);
      sts = p4_pd_plcmt_copy_pack(unpacked_data, packed_data);
      assert(0 == sts);
      return packed_data;
    }
//::   cb_name = pd_prefix + "mat_table_update_cb"
    static void ${cb_name}(p4_pd_dev_target_t dev_tgt,
                           p4_pd_tbl_hdl_t tbl_hdl,
                           enum p4_pd_mat_update_type update_type,
                           union p4_pd_mat_update_params *update_params,
                           void *cookie) {
      (void)cookie;
      ${api_prefix}tbl_update_t info;
      ${api_prefix}tbl_update_data update_data;
      ${api_prefix}mat_tbl_update_t mat;

      ::res_pd_rpc::DevTarget_t dt;
      dt.__set_dev_id(dev_tgt.device_id);
      dt.__set_dev_pipe_id(dev_tgt.dev_pipe_id);
      mat.__set_dev_tgt(dt);
      mat.__set_tbl_hdl(tbl_hdl);
      size_t packed_sz;
      p4_pd_status_t sts = 0;
      switch (update_type) {
        case P4_PD_MAT_UPDATE_ADD: {
          void *unpacked_data = update_params->add.data;
          const uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_add_params add;
          add.__set_entry_hdl(update_params->add.ent_hdl);
          add.__set_priority(update_params->add.priority);
          add.__set_entry_index(update_params->add.logical_index);
          add.__set_action_profile_mbr(update_params->add.action_profile_mbr);
          add.__set_action_index(update_params->add.indirect_action_index);
          add.__set_action_profile_mbr_exists(update_params->add.action_profile_mbr_exists);
          add.__set_sel_grp_hdl(update_params->add.sel_grp_hdl);
          add.__set_selection_index(update_params->add.indirect_selection_index);
          add.__set_num_selector_indices(update_params->add.num_selector_indices);
          add.__set_sel_grp_exists(update_params->add.sel_grp_exists);
          add.__set_drv_data(s);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_ADD);
          mat.update_params.__set_add(add);
          break;
        }
        case P4_PD_MAT_UPDATE_ADD_MULTI: {
          void *unpacked_data = update_params->add_multi.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_add_multi_params add_multi;
          add_multi.__set_entry_hdl(update_params->add_multi.ent_hdl);
          add_multi.__set_priority(update_params->add_multi.priority);
          add_multi.__set_action_profile_mbr(update_params->add_multi.action_profile_mbr);
          add_multi.__set_action_index(update_params->add_multi.indirect_action_index);
          add_multi.__set_action_profile_mbr_exists(update_params->add_multi.action_profile_mbr_exists);
          add_multi.__set_sel_grp_hdl(update_params->add_multi.sel_grp_hdl);
          add_multi.__set_selection_index(update_params->add_multi.indirect_selection_index);
          add_multi.__set_num_selector_indices(update_params->add_multi.num_selector_indices);
          add_multi.__set_sel_grp_exists(update_params->add_multi.sel_grp_exists);
          add_multi.__set_drv_data(s);
          for (int i=0; i<update_params->add_multi.logical_index_array_length; ++i) {
            ${api_prefix}multi_index m;
            m.base_index = update_params->add_multi.location_array[i].logical_index_base;
            m.index_count = update_params->add_multi.location_array[i].logical_index_count;
            add_multi.locations.push_back(m);
          }
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_ADD_MULTI);
          mat.update_params.__set_add_multi(add_multi);
          break;
        }
        case P4_PD_MAT_UPDATE_SET_DFLT: {
          void *unpacked_data = update_params->set_dflt.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_set_dflt_params set_dflt;
          set_dflt.__set_entry_hdl(update_params->set_dflt.ent_hdl);
          set_dflt.__set_action_profile_mbr(update_params->set_dflt.action_profile_mbr);
          set_dflt.__set_action_index(update_params->set_dflt.indirect_action_index);
          set_dflt.__set_action_profile_mbr_exists(update_params->set_dflt.action_profile_mbr_exists);
          set_dflt.__set_sel_grp_hdl(update_params->set_dflt.sel_grp_hdl);
          set_dflt.__set_selection_index(update_params->set_dflt.indirect_selection_index);
          set_dflt.__set_num_selector_indices(update_params->set_dflt.num_selector_indices);
          set_dflt.__set_sel_grp_exists(update_params->set_dflt.sel_grp_exists);
          set_dflt.__set_drv_data(s);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_SET_DFLT);
          mat.update_params.__set_set_dflt(set_dflt);
          break;
        }
        case P4_PD_MAT_UPDATE_CLR_DFLT: {
          ${api_prefix}mat_update_clr_dflt_params clr_dflt;
          clr_dflt.__set_entry_hdl(update_params->clr_dflt.ent_hdl);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_CLR_DFLT);
          mat.update_params.__set_clr_dflt(clr_dflt);
          break;
        }
        case P4_PD_MAT_UPDATE_DEL: {
          ${api_prefix}mat_update_del_params del;
          del.__set_entry_hdl(update_params->del.ent_hdl);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_DEL);
          mat.update_params.__set_remove(del);
          break;
        }
        case P4_PD_MAT_UPDATE_MOD: {
          void *unpacked_data = update_params->mod.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_mod_params mod;
          mod.__set_entry_hdl(update_params->mod.ent_hdl);
          mod.__set_action_profile_mbr(update_params->mod.action_profile_mbr);
          mod.__set_action_index(update_params->mod.indirect_action_index);
          mod.__set_action_profile_mbr_exists(update_params->mod.action_profile_mbr_exists);
          mod.__set_sel_grp_hdl(update_params->mod.sel_grp_hdl);
          mod.__set_selection_index(update_params->mod.indirect_selection_index);
          mod.__set_num_selector_indices(update_params->mod.num_selector_indices);
          mod.__set_sel_grp_exists(update_params->mod.sel_grp_exists);
          mod.__set_drv_data(s);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_MOD);
          mat.update_params.__set_mod(mod);
          break;
        }
        case P4_PD_MAT_UPDATE_MOV: {
          void *unpacked_data = update_params->mov.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_mov_params mov;
          mov.__set_entry_hdl(update_params->mov.ent_hdl);
          mov.__set_entry_index(update_params->mov.logical_index);
          mov.__set_action_profile_mbr(update_params->mov.action_profile_mbr);
          mov.__set_action_index(update_params->mov.indirect_action_index);
          mov.__set_action_profile_mbr_exists(update_params->mov.action_profile_mbr_exists);
          mov.__set_sel_grp_hdl(update_params->mov.sel_grp_hdl);
          mov.__set_selection_index(update_params->mov.indirect_selection_index);
          mov.__set_num_selector_indices(update_params->mov.num_selector_indices);
          mov.__set_sel_grp_exists(update_params->mov.sel_grp_exists);
          mov.__set_drv_data(s);
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_MOV);
          mat.update_params.__set_mov(mov);
          break;
        }
        case P4_PD_MAT_UPDATE_MOV_MULTI: {
          void *unpacked_data = update_params->mov_multi.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          ${api_prefix}mat_update_mov_multi_params mov_multi;
          mov_multi.__set_entry_hdl(update_params->mov_multi.ent_hdl);
          mov_multi.__set_action_profile_mbr(update_params->mov_multi.action_profile_mbr);
          mov_multi.__set_action_index(update_params->mov_multi.indirect_action_index);
          mov_multi.__set_action_profile_mbr_exists(update_params->mov_multi.action_profile_mbr_exists);
          mov_multi.__set_sel_grp_hdl(update_params->mov_multi.sel_grp_hdl);
          mov_multi.__set_selection_index(update_params->mov_multi.indirect_selection_index);
          mov_multi.__set_num_selector_indices(update_params->mov_multi.num_selector_indices);
          mov_multi.__set_sel_grp_exists(update_params->mov_multi.sel_grp_exists);
          mov_multi.__set_drv_data(s);
          for (int i=0; i<update_params->mov_multi.logical_index_array_length; ++i) {
            ${api_prefix}multi_index m;
            m.__set_base_index(update_params->mov_multi.location_array[i].logical_index_base);
            m.__set_index_count(update_params->mov_multi.location_array[i].logical_index_count);
            mov_multi.locations.push_back(m);
          }
          mat.__set_update_type(${api_prefix}mat_update_type::type::MAT_UPDATE_MOV_MULTI);
          mat.update_params.__set_mov_multi(mov_multi);
          break;
        }
      }
      update_data.__set_mat(mat);
      info.__set_update_data(update_data);
      info.__set_update_type(${api_prefix}tbl_update_type::type::MAT_UPDATE_TYPE);
      ${queue_name}[dev_tgt.device_id].push_back(info);
    }

//::   if gen_hitless_ha_test_pd:
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id"]
//::     param_str = ", ".join(params)
//::     name = "enable_callbacks_for_hitless_ha"
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_sess_hdl_t p4_pd_sess_hdl = sess_hdl;
      int p4_pd_dev_id = dev_id;
      p4_pd_status_t sts = ${pd_name}(p4_pd_sess_hdl, p4_pd_dev_id);
      if(sts != 0) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
    }
//::   #endif
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id"]
//::     param_str = ", ".join(params)
//::     name = table + "_register_mat_update_cb"
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_sess_hdl_t p4_pd_sess_hdl = sess_hdl;
      int p4_pd_dev_id = dev_id;
      p4_pd_status_t sts = ${pd_name}(p4_pd_sess_hdl, p4_pd_dev_id, ${cb_name}, NULL);
      if(sts != 0) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
    }

//::   #endfor

    // ADT Update Tracking
//::   cb_name = pd_prefix + "adt_table_update_cb"
    static void ${cb_name}(p4_pd_dev_target_t dev_tgt,
                           p4_pd_tbl_hdl_t tbl_hdl,
                           enum p4_pd_adt_update_type update_type,
                           union p4_pd_adt_update_params *update_params,
                           void *cookie) {
      (void)cookie;
      ${api_prefix}tbl_update_t info;
      ${api_prefix}tbl_update_data update_data;
      ${api_prefix}adt_tbl_update_t adt;

      ::res_pd_rpc::DevTarget_t dt;
      dt.__set_dev_id(dev_tgt.device_id);
      dt.__set_dev_pipe_id(dev_tgt.dev_pipe_id);
      adt.__set_dev_tgt(dt);
      adt.__set_tbl_hdl(tbl_hdl);

      size_t data_size = 0, packed_sz = 0;
      p4_pd_status_t sts = 0;
      switch (update_type) {
        case P4_PD_ADT_UPDATE_ADD: {
          ${api_prefix}adt_update_add_params add;
          void *unpacked_data = update_params->add.data;
          const uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          add.__set_entry_hdl(update_params->add.ent_hdl);
          add.__set_drv_data(s);
          adt.__set_update_type(${api_prefix}adt_update_type::type::ADT_UPDATE_ADD);
          adt.update_params.__set_add(add);
          break;
        }
        case P4_PD_ADT_UPDATE_DEL: {
          ${api_prefix}adt_update_del_params remove;
          remove.__set_entry_hdl(update_params->del.ent_hdl);
          adt.__set_update_type(${api_prefix}adt_update_type::type::ADT_UPDATE_DEL);
          adt.update_params.__set_remove(remove);
          break;
        }
        case P4_PD_ADT_UPDATE_MOD: {
          ${api_prefix}adt_update_mod_params mod;
          void *unpacked_data = update_params->mod.data;
          uint8_t *packed_data = pack_ent_data(unpacked_data, packed_sz);
          std::string s;
          s.assign(reinterpret_cast<const char*>(packed_data), packed_sz);
          delete[] packed_data;
          mod.__set_entry_hdl(update_params->mod.ent_hdl);
          mod.__set_drv_data(s);
          adt.__set_update_type(${api_prefix}adt_update_type::type::ADT_UPDATE_MOD);
          adt.update_params.__set_mod(mod);
          break;
        }
      }

      update_data.__set_adt(adt);
      info.__set_update_data(update_data);
      info.__set_update_type(${api_prefix}tbl_update_type::type::ADT_UPDATE_TYPE);
      ${queue_name}[dev_tgt.device_id].push_back(info);
    }


//::   action_profiles = set()
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if not action_table_hdl: continue
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::     if act_prof in action_profiles: continue
//::     action_profiles.add(act_prof)

//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id"]
//::     param_str = ", ".join(params)
//::     name = act_prof + "_register_adt_update_cb"
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_sess_hdl_t p4_pd_sess_hdl = sess_hdl;
      int p4_pd_dev_id = dev_id;
      p4_pd_status_t sts = ${pd_name}(p4_pd_sess_hdl, p4_pd_dev_id, ${cb_name}, NULL);
      if(sts != 0) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
    }

//::   #endfor

    // SEL Update Tracking
//::   cb_name = pd_prefix + "sel_table_update_cb"
    static void ${cb_name}(p4_pd_dev_target_t dev_tgt,
                           p4_pd_tbl_hdl_t tbl_hdl,
                           enum p4_pd_sel_update_type update_type,
                           union p4_pd_sel_update_params *update_params,
                           void *cookie) {
      (void)cookie;
      ${api_prefix}tbl_update_t info;
      ${api_prefix}tbl_update_data update_data;
      ${api_prefix}sel_tbl_update_t sel;

      ::res_pd_rpc::DevTarget_t dt;
      dt.__set_dev_id(dev_tgt.device_id);
      dt.__set_dev_pipe_id(dev_tgt.dev_pipe_id);
      sel.__set_dev_tgt(dt);
      sel.__set_tbl_hdl(tbl_hdl);

      size_t data_size = 0;
      p4_pd_status_t sts = 0;
      switch (update_type) {
        case P4_PD_SEL_UPDATE_GROUP_CREATE: {
          ${api_prefix}sel_update_group_create_params create;
          create.group_hdl = update_params->grp_create.grp_hdl;
          create.num_indexes = update_params->grp_create.num_indexes;
          create.max_members = update_params->grp_create.max_members;
          create.base_logical_index = update_params->grp_create.base_logical_index;
          for (int i=0; i<update_params->grp_create.logical_adt_index_array_length; ++i) {
            ${api_prefix}multi_index m;
            m.base_index = update_params->grp_create.logical_adt_indexes[i].logical_index_base;
            m.index_count = update_params->grp_create.logical_adt_indexes[i].logical_index_count;
            create.locations.push_back(m);
          }
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_GROUP_CREATE);
          sel.update_params.__set_grp_create(create);
          break;
        }
        case P4_PD_SEL_UPDATE_GROUP_DESTROY: {
          ${api_prefix}sel_update_group_destroy_params grp_destroy;
          grp_destroy.group_hdl = update_params->grp_destroy.grp_hdl;
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_GROUP_DESTROY);
          sel.update_params.__set_grp_destroy(grp_destroy);
          break;
        }
        case P4_PD_SEL_UPDATE_ADD: {
          ${api_prefix}sel_update_add_params add;
          add.group_hdl = update_params->add.grp_hdl;
          add.entry_hdl = update_params->add.ent_hdl;
          add.entry_index = update_params->add.logical_index;
          add.entry_subindex = update_params->add.logical_subindex;
          sts = p4_pd_plcmt_data_size(update_params->add.data, &data_size);
          assert(0 == sts);
          add.drv_data.assign( static_cast<const char*>(update_params->add.data), data_size);
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_ADD);
          sel.update_params.__set_add(add);
          break;
        }
        case P4_PD_SEL_UPDATE_DEL: {
          ${api_prefix}sel_update_del_params remove;
          info.update_data.sel.update_type = ${api_prefix}sel_update_type::type::SEL_UPDATE_DEL;
          remove.group_hdl = update_params->del.grp_hdl;
          remove.entry_hdl = update_params->del.ent_hdl;
          remove.entry_index = update_params->del.logical_index;
          remove.entry_subindex = update_params->del.logical_subindex;
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_DEL);
          sel.update_params.__set_remove(remove);
          break;
        }
        case P4_PD_SEL_UPDATE_ACTIVATE: {
          ${api_prefix}sel_update_activate_params activate;
          activate.group_hdl = update_params->activate.grp_hdl;
          activate.entry_hdl = update_params->activate.ent_hdl;
          activate.entry_index = update_params->activate.logical_index;
          activate.entry_subindex = update_params->activate.logical_subindex;
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_ACTIVATE);
          sel.update_params.__set_activate(activate);
          break;
        }
        case P4_PD_SEL_UPDATE_DEACTIVATE: {
          ${api_prefix}sel_update_deactivate_params deactivate;
          deactivate.group_hdl = update_params->deactivate.grp_hdl;
          deactivate.entry_hdl = update_params->deactivate.ent_hdl;
          deactivate.entry_index = update_params->deactivate.logical_index;
          deactivate.entry_subindex = update_params->deactivate.logical_subindex;
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_DEACTIVATE);
          sel.update_params.__set_deactivate(deactivate);
          break;
        }
        case P4_PD_SEL_UPDATE_SET_FALLBACK: {
          ${api_prefix}sel_update_set_fallback_params set_fallback;
          set_fallback.entry_hdl = update_params->set_fallback.ent_hdl;
          sts = p4_pd_plcmt_data_size(update_params->set_fallback.data, &data_size);
          assert(0 == sts);
          set_fallback.drv_data.assign( static_cast<const char*>(update_params->set_fallback.data), data_size);
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_SET_FALLBACK);
          sel.update_params.__set_set_fallback(set_fallback);
          break;
        }
        case P4_PD_SEL_UPDATE_CLR_FALLBACK: {
          sel.__set_update_type(${api_prefix}sel_update_type::type::SEL_UPDATE_CLR_FALLBACK);
          break;
        }
      }
      update_data.__set_sel(sel);
      info.__set_update_data(update_data);
      info.__set_update_type(${api_prefix}tbl_update_type::type::SEL_UPDATE_TYPE);
      ${queue_name}[dev_tgt.device_id].push_back(info);
    }
//::   action_profiles = set()
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if not action_table_hdl: continue
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::     if act_prof in action_profiles: continue
//::     action_profiles.add(act_prof)
//::     if not select_hdl: continue

//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id"]
//::     param_str = ", ".join(params)
//::     name = act_prof + "_register_sel_update_cb"
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      p4_pd_sess_hdl_t p4_pd_sess_hdl = sess_hdl;
      int p4_pd_dev_id = dev_id;
      p4_pd_status_t sts = ${pd_name}(p4_pd_sess_hdl, p4_pd_dev_id, ${cb_name}, NULL);
      if(sts != 0) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
    }

//::   #endfor

    static inline const void* find_mat_data(const ${api_prefix}mat_tbl_update_t &u) {
      const void *rv = nullptr;
      switch (u.update_type) {
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_ADD:
          rv = static_cast<const void*>(u.update_params.add.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_ADD_MULTI:
          rv = static_cast<const void*>(u.update_params.add_multi.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_SET_DFLT:
          rv = static_cast<const void*>(u.update_params.set_dflt.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOD:
          rv = static_cast<const void*>(u.update_params.mod.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOV:
          rv = static_cast<const void*>(u.update_params.mov.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOV_MULTI:
          rv = static_cast<const void*>(u.update_params.mov_multi.drv_data.data());
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_DEL:
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_CLR_DFLT:
          break;
      }
      return rv;
    }

    static uint8_t *process_mat_plcmt_data(struct p4_pd_plcmt_info *x,
                                const ${api_prefix}mat_tbl_update_t &u) {
      p4_pd_status_t sts = 0;

      /* Create an unapcked copy of the data. */
      const void *packed_data = find_mat_data(u);
      uint8_t *unpacked_data = nullptr;
      size_t unpacked_sz = 0;
      if (packed_data) {
        sts = p4_pd_plcmt_unpack_data_size(packed_data, &unpacked_sz);
        assert(!sts);
        unpacked_data = new uint8_t[unpacked_sz];
        assert(unpacked_data);
        sts = p4_pd_plcmt_copy_unpack(packed_data, unpacked_data);
        assert(!sts);
      }

      switch (u.update_type) {
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_ADD:
          sts = p4_pd_set_one_plcmt_op_mat_add(
              x,
              u.update_params.add.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.add.entry_index,
              u.update_params.add.selection_index,
              u.update_params.add.action_index,
              unpacked_data);
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_ADD_MULTI: {
          struct p4_pd_multi_index *loc = new struct p4_pd_multi_index[u.update_params.add_multi.locations.size()];
          for (unsigned int i=0; i<u.update_params.add_multi.locations.size(); ++i) {
            loc[i].logical_index_base = u.update_params.add_multi.locations[i].base_index;
            loc[i].logical_index_count = u.update_params.add_multi.locations[i].index_count;
          }
          sts = p4_pd_set_one_plcmt_op_mat_add_multi(
              x,
              u.update_params.add_multi.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.add_multi.selection_index,
              u.update_params.add_multi.action_index,
              u.update_params.add_multi.locations.size(),
              loc,
              unpacked_data);
          delete[] loc;
          break; }
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_SET_DFLT:
          sts = p4_pd_set_one_plcmt_op_mat_set_dflt(
              x,
              u.update_params.set_dflt.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.set_dflt.selection_index,
              u.update_params.set_dflt.action_index,
              unpacked_data);
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_CLR_DFLT:
          sts = p4_pd_set_one_plcmt_op_mat_clr_dflt(
              x, u.update_params.clr_dflt.entry_hdl, i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_DEL:
          sts = p4_pd_set_one_plcmt_op_mat_del(
              x, u.update_params.remove.entry_hdl);
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOD:
          sts = p4_pd_set_one_plcmt_op_mat_mod(
              x,
              u.update_params.mod.entry_hdl,
              u.update_params.mod.selection_index,
              u.update_params.mod.action_index,
              unpacked_data);
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOV:
          sts = p4_pd_set_one_plcmt_op_mat_mov(
              x,
              u.update_params.mov.entry_hdl,
              u.update_params.mov.entry_index,
              u.update_params.mov.selection_index,
              u.update_params.mov.action_index,
              unpacked_data);
          break;
        case ${api_prefix}mat_update_type::type::MAT_UPDATE_MOV_MULTI: {
          struct p4_pd_multi_index *loc = new struct p4_pd_multi_index[u.update_params.mov_multi.locations.size()];
          for (unsigned int i=0; i<u.update_params.mov_multi.locations.size(); ++i) {
            loc[i].logical_index_base = u.update_params.mov_multi.locations[i].base_index;
            loc[i].logical_index_count = u.update_params.mov_multi.locations[i].index_count;
          }
          sts = p4_pd_set_one_plcmt_op_mat_mov_multi(
              x,
              u.update_params.mov_multi.entry_hdl,
              u.update_params.mov_multi.selection_index,
              u.update_params.mov_multi.action_index,
              u.update_params.mov_multi.locations.size(),
              loc,
              unpacked_data);
          delete[] loc;
          break; }
        default:
          assert(0);
          break;
      }
      if (sts) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
      return unpacked_data;
    }

    static inline const void *find_adt_data(const ${api_prefix}adt_tbl_update_t &u) {
      const void *rv = nullptr;
      switch (u.update_type) {
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_ADD:
          rv = static_cast<const void *>(u.update_params.add.drv_data.data());
          break;
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_MOD:
          rv = static_cast<const void *>(u.update_params.mod.drv_data.data());
          break;
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_DEL:
          break;
      }
      return rv;
    }

    static uint8_t* process_adt_plcmt_data(struct p4_pd_plcmt_info *x,
                                const ${api_prefix}adt_tbl_update_t &u) {
      p4_pd_status_t sts = 0;

      /* Create an unpacked copy of the data. */
      const void *packed_data = find_adt_data(u);
      uint8_t *unpacked_data = nullptr;
      size_t unpacked_sz = 0;
      if (packed_data) {
        sts = p4_pd_plcmt_unpack_data_size(packed_data, &unpacked_sz);
        assert(!sts);
        unpacked_data = new uint8_t[unpacked_sz];
        assert(unpacked_data);
        sts = p4_pd_plcmt_copy_unpack(packed_data, unpacked_data);
        assert(!sts);
      }

      switch (u.update_type) {
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_ADD:
          sts = p4_pd_set_one_plcmt_op_adt_add(x,
                                               u.update_params.add.entry_hdl,
                                               i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
                                               unpacked_data);
          break;
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_DEL:
          sts = p4_pd_set_one_plcmt_op_adt_del(
              x, u.update_params.remove.entry_hdl);
          break;
        case ${api_prefix}adt_update_type::type::ADT_UPDATE_MOD:
          sts = p4_pd_set_one_plcmt_op_adt_mod(
              x, u.update_params.mod.entry_hdl, unpacked_data);
          break;
        default:
          assert(0);
          break;
      }
      if (sts) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
      return unpacked_data;
    }

    static void process_sel_plcmt_data(struct p4_pd_plcmt_info *x,
                                const ${api_prefix}sel_tbl_update_t &u) {
      p4_pd_status_t sts = 0;
      void *drv_data = nullptr;
      switch (u.update_type) {
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_GROUP_CREATE: {
          struct p4_pd_multi_index *loc = new struct p4_pd_multi_index[u.update_params.grp_create.locations.size()];
          for (unsigned int i=0; i<u.update_params.grp_create.locations.size(); ++i) {
            loc[i].logical_index_base = u.update_params.grp_create.locations[i].base_index;
            loc[i].logical_index_count = u.update_params.grp_create.locations[i].index_count;
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
          break; }
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_GROUP_DESTROY:
          sts = p4_pd_set_one_plcmt_op_sel_grp_destroy(
              x, u.update_params.grp_destroy.group_hdl, i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_ADD:
          sts = p4_pd_plcmt_duplicate(
              const_cast<char *>(u.update_params.add.drv_data.data()),
              &drv_data);
          assert(!sts);
          sts =
              p4_pd_set_one_plcmt_op_sel_add(x,
                                             u.update_params.add.group_hdl,
                                             u.update_params.add.entry_hdl,
                                             i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
                                             u.update_params.add.entry_index,
                                             u.update_params.add.entry_subindex,
                                             drv_data);
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_DEL:
          sts = p4_pd_set_one_plcmt_op_sel_del(
              x,
              u.update_params.remove.group_hdl,
              u.update_params.remove.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.remove.entry_index,
              u.update_params.remove.entry_subindex);
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_ACTIVATE:
          sts = p4_pd_set_one_plcmt_op_sel_activate(
              x,
              u.update_params.activate.group_hdl,
              u.update_params.activate.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.activate.entry_index,
              u.update_params.activate.entry_subindex);
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_DEACTIVATE:
          sts = p4_pd_set_one_plcmt_op_sel_deactivate(
              x,
              u.update_params.deactivate.group_hdl,
              u.update_params.deactivate.entry_hdl,
              i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
              u.update_params.deactivate.entry_index,
              u.update_params.deactivate.entry_subindex);
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_SET_FALLBACK:
          sts = p4_pd_plcmt_duplicate(
              const_cast<char *>(u.update_params.set_fallback.drv_data.data()),
              &drv_data);
          assert(!sts);
          sts = p4_pd_set_one_plcmt_op_sel_set_fallback(x,
                                             u.update_params.set_fallback.entry_hdl,
                                             i16_to_bf_pipe(u.dev_tgt.dev_pipe_id),
                                             drv_data);
          break;
        case ${api_prefix}sel_update_type::type::SEL_UPDATE_CLR_FALLBACK:
          sts = p4_pd_set_one_plcmt_op_sel_clr_fallback(x, i16_to_bf_pipe(u.dev_tgt.dev_pipe_id));
          break;
        default:
          assert(0);
          break;
      }
      if (sts) {
        InvalidTableOperation iop;
        iop.code = sts;
        throw iop;
      }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const std::vector<" + api_prefix + "tbl_update_t> &updates"]
//::   param_str = ", ".join(params)
//::   name = "restore_virtual_dev_state"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      struct p4_pd_plcmt_info *x = p4_pd_create_plcmt_info();
      assert(x);
      int tbl_hdl = 0;
      uint32_t num_found = 0;
      p4_pd_status_t sts = 0;
      std::vector<uint8_t*> to_free(updates.size());
      uint8_t *data = nullptr;

      for (auto & u : updates) {
        const ${api_prefix}mat_tbl_update_t &mat = u.update_data.mat;
        const ${api_prefix}adt_tbl_update_t &adt = u.update_data.adt;
        const ${api_prefix}sel_tbl_update_t &sel = u.update_data.sel;
        switch (u.update_type) {
          case ${api_prefix}tbl_update_type::type::MAT_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != mat.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, dev_id, tbl_hdl, x, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
              num_found = 0;
            }
            tbl_hdl = mat.tbl_hdl;
            data = process_mat_plcmt_data(x, mat);
            to_free.push_back(data);
            break;
          case ${api_prefix}tbl_update_type::type::ADT_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != adt.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, dev_id, tbl_hdl, x, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
              num_found = 0;
            }
            tbl_hdl = adt.tbl_hdl;
            data = process_adt_plcmt_data(x, adt);
            to_free.push_back(data);
            break;
          case ${api_prefix}tbl_update_type::type::SEL_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != sel.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, dev_id, tbl_hdl, x, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
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
        sts = ${pd_name}(sess_hdl, dev_id, tbl_hdl, x, &num_processed);
        if (sts != 0 || num_processed != num_found) {
          InvalidTableOperation iop;
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

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t v_dev_id",
//::             "const int8_t p_dev_id"]
//::   param_str = ", ".join(params)
//::   name = "program_all_updates"
//::   pd_name = "p4_pd_process_plcmt_info"
    void ${name}(${param_str}) {
      std::vector<${api_prefix}tbl_update_t> updates;
      updates.swap( ${queue_name}[v_dev_id] );

      struct p4_pd_plcmt_info *x = p4_pd_create_plcmt_info();
      assert(x);
      int tbl_hdl = 0;
      uint32_t num_found = 0;
      p4_pd_status_t sts = 0;
      std::vector<uint8_t*> to_free(updates.size());
      uint8_t *data = nullptr;

      for (auto & u : updates) {
        const ${api_prefix}mat_tbl_update_t &mat = u.update_data.mat;
        const ${api_prefix}adt_tbl_update_t &adt = u.update_data.adt;
        const ${api_prefix}sel_tbl_update_t &sel = u.update_data.sel;
        switch (u.update_type) {
          case ${api_prefix}tbl_update_type::type::MAT_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != mat.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, p_dev_id, tbl_hdl, x, 0, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
              num_found = 0;
            }
            tbl_hdl = mat.tbl_hdl;
            data = process_mat_plcmt_data(x, mat);
            to_free.push_back(data);
            break;
          case ${api_prefix}tbl_update_type::type::ADT_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != adt.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, p_dev_id, tbl_hdl, x, 0, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
              num_found = 0;
            }
            tbl_hdl = adt.tbl_hdl;
            data = process_adt_plcmt_data(x, adt);
            to_free.push_back(data);
            break;
          case ${api_prefix}tbl_update_type::type::SEL_UPDATE_TYPE:
            if (tbl_hdl && tbl_hdl != sel.tbl_hdl) {
              // This update is for a new table, push what is already collected
              // and start a new list.
              uint32_t num_processed = 0;
              sts = ${pd_name}(sess_hdl, p_dev_id, tbl_hdl, x, 0, &num_processed);
              if (sts != 0 || num_processed != num_found) {
                InvalidTableOperation iop;
                iop.code = sts;
                throw iop;
              }
              p4_pd_destroy_plcmt_info(x);
              x = p4_pd_create_plcmt_info();
              assert(x);
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
        sts = ${pd_name}(sess_hdl, p_dev_id, tbl_hdl, x, 0, &num_processed);
        if (sts != 0 || num_processed != num_found) {
          InvalidTableOperation iop;
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

//:: #endif


//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0:
//::     continue
//::   #endif
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["const int32_t priority"]
//::   #endif
//::   param_str = ", ".join(params)
//::   name = table + "_match_spec_to_entry_hdl"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   for name, width in match_params:
//::     if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::     else:
  memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::     #endif
//::   #endfor

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }
//:: #endfor


    // Dynamic Key Mask Exm Table API.

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if len(t_info["match_fields"]) == 0:
//::     continue
//::   #endif
//::   if not t_info["dynamic_match_key_masks"]:
//::     continue
//::   #endif
//::   match_type = t_info["match_type"]
//::   if match_type == "ternary" or match_type == "range":
//::     continue
//::   #endif
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const " + api_prefix + table + "_match_key_mask_spec_t &match_key_mask_spec"]
//::   param_str = ", ".join(params)
//::   name = table + "_match_key_mask_spec_set"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {

        ${pd_prefix}${table}_match_key_mask_spec_t pd_match_key_mask_spec;
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   for name, width in match_params:
//::     name = name + "_mask"
//::     if width <= 4:
        pd_match_key_mask_spec.${name} = match_key_mask_spec.${name};
//::     else:
        memcpy(pd_match_key_mask_spec.${name}, match_key_mask_spec.${name}.c_str(), ${width});
//::     #endif
//::   #endfor
//::   pd_params = ["sess_hdl", "dev_id", "&pd_match_key_mask_spec"]
//::   pd_param_str = ", ".join(pd_params)

        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = [api_prefix + table + "_match_key_mask_spec_t &match_key_mask_spec",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_match_key_mask_spec_get"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {

        ${pd_prefix}${table}_match_key_mask_spec_t pd_match_key_mask_spec;
//::   pd_params = ["sess_hdl", "dev_id", "&pd_match_key_mask_spec"]
//::   pd_param_str = ", ".join(pd_params)

        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   for name, width in match_params:
//::     name = name + "_mask"
//::     if width <= 4:
        match_key_mask_spec.${name} = pd_match_key_mask_spec.${name};
//::     else:
        match_key_mask_spec.${name} = std::string((char*)pd_match_key_mask_spec.${name}, ${width});

//::     #endif
//::   #endfor
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = table + "_match_key_mask_spec_reset"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {

//::   pd_params = ["sess_hdl", "dev_id"]
//::   pd_param_str = ", ".join(pd_params)

        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }


//:: #endfor


    // Table entry add functions

//::
//:: def get_direct_parameter_specs(direct_resources, prefix, register_info):
//::   specs = []
//::   for res_name, res_type, _ in direct_resources:
//::     if res_type == "bytes_meter":
//::       specs += ["const " + prefix + "bytes_meter_spec_t &" + res_name + "_spec"]
//::     elif res_type == "packets_meter":
//::       specs += ["const " + prefix + "packets_meter_spec_t &" + res_name + "_spec"]
//::     elif res_type == "register":
//::       param_type = register_info[res_name]["v_thrift_type_imp"]
//::       if register_info[res_name]["layout"]:
//::         param_type += " &"
//::       #endif
//::       specs += ["const " + param_type + " " + res_name + "_spec"]
//::     elif res_type == "lpf":
//::       specs += ["const " + prefix + "lpf_spec_t &" + res_name + "_spec"]
//::     elif res_type == "wred":
//::       specs += ["const " + prefix + "wred_spec_t &" + res_name + "_spec"]
//::     #endif
//::   #endfor
//::   return specs
//:: #enddef
//::
//::
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt",
//::               "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::     if match_type == "ternary" or match_type == "range":
//::       params += ["const int32_t priority"]
//::     #endif
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     if t_info["timeout"]:
//::       params += ["const int32_t ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     param_str = ", ".join(params)
//::     name = table + "_table_add_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::     match_params = gen_match_params(t_info["match_fields"], field_info)
//::     for name, width in match_params:
//::       if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::       else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::       #endif
//::     #endfor

//::     if has_action_spec:
    ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a_info["param_names"],
//::                                         a_info["param_byte_widths"])
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::     if match_type == "ternary" or match_type == "range":
//::       pd_params += ["priority"]
//::     #endif
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     if t_info["timeout"]:
//::      pd_params += ["(uint32_t) ttl"]
//::     #endif
//::     # direct parameter specs
//::     for res_name, res_type, _ in table_direct_resources[table]:
//::       if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::       if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::     #endfor
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }

//::   #endfor
//:: #endfor


    // Table entry modify functions
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     for idx in range(2):
//::       name = table + "_table_modify_with_" + action
//::       params = ["const SessionHandle_t sess_hdl"]
//::       if idx:
//::         name += "_by_match_spec"
//::         params += ["const DevTarget_t &dev_tgt",
//::                    "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::         pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::         if match_type == "ternary" or match_type == "range":
//::           params += ["const int32_t priority"]
//::           pd_params += ["priority"]
//::         #endif
//::       else:
//::         params += ["const int8_t dev_id",
//::                    "const EntryHandle_t entry"]
//::         pd_params = ["sess_hdl", "dev_id", "entry"]
//::       #endif
//::       if has_action_spec:
//::         params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::       param_str = ", ".join(params)
//::       pd_name = pd_prefix + name
    void ${name}(${param_str}) {

//::       if idx:
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::         match_params = gen_match_params(t_info["match_fields"], field_info)
//::         for name, width in match_params:
//::           if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::           else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::           #endif
//::         #endfor

//::       #endif
//::       if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::         action_params = gen_action_params(a_info["param_names"],
//::                                         a_info["param_byte_widths"])
//::         for name, width in action_params:
//::           if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::           else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::           #endif
//::         #endfor

//::         pd_params += ["&pd_action_spec"]
//::       #endif
//::       # direct parameter specs
//::       for res_name, res_type, _ in table_direct_resources[table]:
//::         if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::           pd_params += ["&pd_" + res_name + "_spec"]
//::         #endif
//::         if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::           pd_params += ["&pd_" + res_name + "_spec"]
//::         #endif
//::       #endfor
//::       pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     #endfor
//::   #endfor
//:: #endfor

    // Table entry delete functions

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = table + "_table_delete"
//::     params = ["const SessionHandle_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["const DevTarget_t &dev_tgt",
//::                  "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["const int32_t priority"]
//::       #endif
//::     else:
//::       params += ["const int8_t dev_id",
//::                  "const EntryHandle_t entry"]
//::     #endif
//::     param_str = ", ".join(params)
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
//::     if idx:
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::       match_params = gen_match_params(t_info["match_fields"], field_info)
//::       for name, width in match_params:
//::         if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::         else:
  memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::       if match_type == "ternary" or match_type == "range":
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &pd_match_spec, priority);
//::       else:
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &pd_match_spec);
//::       #endif
//::     else:
        int status = ${pd_name}(sess_hdl, dev_id, entry);
//::     #endif
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   #endfor
//:: #endfor

    // Table default entry get functions

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = table + "_table_get_default_entry_handle"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    EntryHandle_t ${name}(${param_str}) {
        p4_pd_entry_hdl_t entry_hdl = 0;
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &entry_hdl);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return (EntryHandle_t)entry_hdl;
    }

//::   name = table + "_table_get_default_entry"
//::   pd_name = pd_prefix + name
//::   params = [api_prefix + table + "_entry_desc_t &desc",
//::             "const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const bool from_hw"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
//::   pd_params = ["sess_hdl", "pd_dev_tgt", "from_hw"]
//::   if not action_table_hdl:
        ${pd_prefix}action_specs_t pd_action_specs = {};
//::     pd_params += ["&pd_action_specs"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }

        switch(pd_action_specs.name) {
//::     for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            desc.action_desc.name = std::string("${action}");
            break;
//::     #endfor
        }
//::     cond = "if"
//::     for action, a_info in action_info.items():
//::       if not a_info["param_names"]:
            /* ${action} has no parameters */
//::         continue
//::       #endif
//::       action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
        ${cond} (desc.action_desc.name == "${action}") {
//::       cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::       for name, width in action_params:
                /* ${name} has byte width ${width} */
//::         if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::         else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::         #endif
//::       #endfor
          desc.action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::     #endfor
//::
//::   else:
        bool has_mbr_hdl = false;
        bool has_grp_hdl = false;
        p4_pd_grp_hdl_t grp_hdl = 0;
        p4_pd_mbr_hdl_t mbr_hdl = 0;
//::     if select_hdl:
//::       pd_params += ["&has_mbr_hdl"]
//::       pd_params += ["&has_grp_hdl"]
//::       pd_params += ["&grp_hdl"]
//::     #endif
//::     pd_params += ["&mbr_hdl"]
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       member = indirect_res_name + "_index"
        int ${member} = -1;
//::       pd_params += ["&" + member]
//::     #endfor
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});

        desc.has_grp_hdl = has_grp_hdl;
        desc.has_mbr_hdl = has_mbr_hdl;
        desc.selector_grp_hdl = grp_hdl;
        desc.action_mbr_hdl = mbr_hdl;
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       member = indirect_res_name + "_index"
        desc.${member} = ${member};
//::     #endfor
//::   #endif
    }

//:: #endfor

    // Table default entry clear functions

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = table + "_table_reset_default_entry"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }
//:: #endfor

//:: if gen_hitless_ha_test_pd:
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     name = table + "_get_ha_reconciliation_report"
//::     pd_name = pd_prefix + name
//::     params = [api_prefix + "ha_reconc_report_t &ha_report",
//::               "const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     param_str = ",".join(params)
    void ${name}(${param_str}) {
      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id; 
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
      ${pd_prefix}ha_reconc_report_t pd_ha_report;

      int status = ${pd_name}(sess_hdl, pd_dev_tgt, &pd_ha_report);
      if (status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      } 

      ha_report.num_entries_added = pd_ha_report.num_entries_added; 
      ha_report.num_entries_deleted = pd_ha_report.num_entries_deleted;
      ha_report.num_entries_modified = pd_ha_report.num_entries_modified; 
    }
//::   #endfor 
//:: #endif

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = table + "_get_entry_count"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        int32_t count = 0;
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, (uint32_t *)&count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return count;
    }
//:: #endfor

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)
//::
//::   name = act_prof + "_get_act_prof_entry_count"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        int32_t count = 0;
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, (uint32_t *)&count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return count;
    }
//::
//::   if not select_hdl: continue
//::   name = act_prof + "_get_selector_group_count"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        int32_t count = 0;
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, (uint32_t *)&count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return count;
    }

//::   name = act_prof + "_get_selector_group_member_count"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        int32_t count = 0;

        int status = ${pd_name}(sess_hdl, dev_id, grp, (uint32_t *)&count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return count;
    }

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::
//::   name = table + "_get_first_entry_handle"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    EntryHandle_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        p4_pd_entry_hdl_t entry_hdl = 0;

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &entry_hdl);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return (EntryHandle_t)entry_hdl;
    }

//::   name = table + "_get_next_entry_handles"
//::   pd_name = pd_prefix + name
//::   params = ["std::vector<int32_t> &next_entry_handles",
//::             "const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const EntryHandle_t entry_hdl",
//::             "const int32_t n"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        next_entry_handles = std::vector<EntryHandle_t>(n);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry_hdl, n, (p4_pd_entry_hdl_t *)next_entry_handles.data());
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   name = table + "_get_entry"
//::   pd_name = pd_prefix + name
//::   params = [api_prefix + table + "_entry_desc_t &desc",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry_hdl",
//::             "const bool from_hw"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        bool has_mbr_hdl = false;
        bool has_grp_hdl = false;
        p4_pd_grp_hdl_t grp_hdl = 0;
        p4_pd_mbr_hdl_t mbr_hdl = 0;
        (void) has_mbr_hdl;
        (void) has_grp_hdl;
        (void) grp_hdl;
        (void) mbr_hdl;
//::   if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::   #endif

//::   pd_params = ["sess_hdl", "dev_id", "entry_hdl", "from_hw"]
//::   if has_match_spec:
//::     pd_params += ["&pd_match_spec"]
//::   #endif
//::   if match_type == "ternary" or match_type == "range":
//::     pd_params += ["&desc.priority"]
//::   #endif
//::
//::   if not action_table_hdl:
        ${pd_prefix}action_specs_t pd_action_specs = {};
//::     pd_params += ["&pd_action_specs"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }

        switch(pd_action_specs.name) {
//::     for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            desc.action_desc.name = std::string("${action}");
            break;
//::     #endfor
        }
//::     cond = "if"
//::     for action, a_info in action_info.items():
//::       if not a_info["param_names"]:
        /* ${action} has no parameters */
//::         continue
//::       #endif
//::       action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
        ${cond} (desc.action_desc.name == "${action}") {
//::       cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::       for name, width in action_params:
          /* ${name} has byte width ${width} */
//::         if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::         else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::         #endif
//::       #endfor
          desc.action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::     #endfor
//::
//::   else:
//::     if select_hdl:
//::       pd_params += ["&has_mbr_hdl"]
//::       pd_params += ["&has_grp_hdl"]
//::       pd_params += ["&grp_hdl"]
//::     #endif
//::     pd_params += ["&mbr_hdl"]
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       member = indirect_res_name + "_index"
         int ${member} = -1;
//::       pd_params += ["&" + member]
//::     #endfor
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       member = indirect_res_name + "_index"
        desc.${member} = ${member};
//::     #endfor
//::   #endif
//::   if has_match_spec:
//::     match_params = gen_match_params(t_info["match_fields"], field_info)
//::     for name, width in match_params:
//::       if width <= 4:
        desc.match_spec.${name} = pd_match_spec.${name};
//::       else:
        desc.match_spec.${name} = std::string((char *) pd_match_spec.${name}, ${width});
//::       #endif
//::     #endfor
//::   #endif
       desc.has_grp_hdl = has_grp_hdl;
       desc.has_mbr_hdl = has_mbr_hdl;
       desc.selector_grp_hdl = grp_hdl;
       desc.action_mbr_hdl = mbr_hdl;
    }

//::   if gen_md_pd:
//::     name = table + "_get_entry_from_plcmt_data"
//::     pd_name = pd_prefix + name
//::     params = [api_prefix + table + "_entry_desc_t &desc",
//::               "const std::string& drv_data"]
//::     pd_params = ["const_cast<char*>(drv_data.c_str())"]
//::     param_str = ", ".join(params)
    void ${name}(${param_str}) {
//::     if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::       pd_params += ["&pd_match_spec"]
//::     #endif
//::     if match_type == "ternary" or match_type == "range":
//::       pd_params += ["&desc.priority"]
//::     #endif
//::     if not action_table_hdl:
        ${pd_prefix}action_specs_t pd_action_specs = {};
//::       pd_params += ["&pd_action_specs"]
//::       pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        switch(pd_action_specs.name) {
//::       for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            desc.action_desc.name = std::string("${action}");
            break;
//::       #endfor
        }
//::       cond = "if"
//::       for action, a_info in action_info.items():
//::         if not a_info["param_names"]:
        /* ${action} has no parameters */
//::           continue
//::         #endif
//::         action_params = gen_action_params(a_info["param_names"],
//::                                           a_info["param_byte_widths"])
        ${cond} (desc.action_desc.name == "${action}") {
//::         cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::         for name, width in action_params:
            /* ${name} has byte width ${width} */
//::           if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::           else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::           #endif
//::         #endfor
          desc.action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::       #endfor
//::     else:
        p4_pd_mbr_hdl_t mbr_hdl;
        p4_pd_grp_hdl_t grp_hdl;
        bool has_grp_hdl = false;
        bool has_mbr_hdl = false;
//::       pd_params += ["&mbr_hdl",
//::                     "&grp_hdl",
//::                     "&has_mbr_hdl",
//::                     "&has_grp_hdl"]
//::       pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        desc.members = std::vector<MemberHandle_t>(1);
        if (has_mbr_hdl) desc.members[0] = mbr_hdl;
        if (has_grp_hdl) desc.members[0] = grp_hdl;
//::     #endif

//::     if has_match_spec:
//::       match_params = gen_match_params(t_info["match_fields"], field_info)
//::       for name, width in match_params:
//::         if width <= 4:
        desc.match_spec.${name} = pd_match_spec.${name};
//::         else:
        desc.match_spec.${name} = std::string((char *) pd_match_spec.${name}, ${width});
//::         #endif
//::       #endfor
//::     #endif
    }

//::     name = table + "_get_plcmt_data"
//::     pd_name = pd_prefix + name
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id"]
//::     param_str = ", ".join(params)
    void ${name}(${param_str}) {
//::   pd_params = ["sess_hdl", "dev_id"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return;
    }
//::   #endif
//:: #endfor

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)
//::
//::   name = act_prof + "_get_member"
//::   pd_name = pd_prefix + name
//::   params = [api_prefix + "action_desc_t &action_desc",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const MemberHandle_t member_hdl",
//::             "const bool from_hw"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        ${pd_prefix}action_specs_t pd_action_specs = {};

        int status = ${pd_name}(sess_hdl, dev_id, member_hdl, from_hw,
                                &pd_action_specs);
        if (status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }

        switch(pd_action_specs.name) {
//::   for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            action_desc.name = std::string("${action}");
            break;
//::   #endfor
        }
//::   cond = "if"
//::   for action, a_info in action_info.items():
//::     if not a_info["param_names"]:
        /* ${action} has no parameters */
//::     continue
//::     #endif
//::     action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
        ${cond} (action_desc.name == "${action}") {
//::     cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::     for name, width in action_params:
            /* ${name} has byte width ${width} */
//::       if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::       else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::       #endif
//::     #endfor
          action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::   #endfor
    }
//::   if gen_md_pd:

//::   name = act_prof + "_get_member_from_plcmt_data"
//::   pd_name = pd_prefix + name
//::   params  = [api_prefix + "action_desc_t &action_desc",
//::              "const std::string& drv_data"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        ${pd_prefix}action_specs_t pd_action_specs = {};

        ${pd_name}(const_cast<char*>(drv_data.c_str()), &pd_action_specs);

        switch(pd_action_specs.name) {
//::   for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            action_desc.name = std::string("${action}");
            break;
//::   #endfor
        }
//::   cond = "if"
//::   for action, a_info in action_info.items():
//::     if not a_info["param_names"]:
        /* ${action} has no parameters */
//::     continue
//::     #endif
//::     action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
        ${cond} (action_desc.name == "${action}") {
//::     cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::     for name, width in action_params:
            /* ${name} has byte width ${width} */
//::       if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::       else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::       #endif
//::     #endfor
          action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::   #endfor
    }

//::   name = act_prof + "_get_full_member_info_from_plcmt_data"
//::   pd_name = pd_prefix + name
//::   params  = [api_prefix + "action_desc_t &action_desc",
//::              "const int8_t device_id",
//::              "const MemberHandle_t mbr",
//::              "const std::string& drv_data"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        ${pd_prefix}action_specs_t pd_action_specs = {};

        ${pd_name}(device_id, mbr, const_cast<char*>(drv_data.c_str()), &pd_action_specs);

        switch(pd_action_specs.name) {
//::   for action, a_info in action_info.items():
          case ${p4_pd_prefix}${action}:
            action_desc.name = std::string("${action}");
            break;
//::   #endfor
        }
//::   cond = "if"
//::   for action, a_info in action_info.items():
//::     if not a_info["param_names"]:
        /* ${action} has no parameters */
//::     continue
//::     #endif
//::     action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
        ${cond} (action_desc.name == "${action}") {
//::     cond = "else if"
          ${api_prefix}${action}_action_spec_t a_spec;
//::     for name, width in action_params:
            /* ${name} has byte width ${width} */
//::       if width <= 4:
          a_spec.__set_${name}(pd_action_specs.u.${p4_pd_prefix}${action}.${name});
//::       else:
          a_spec.__set_${name}(std::string((char*)pd_action_specs.u.${p4_pd_prefix}${action}.${name}, ${width}));
//::       #endif
//::     #endfor
          action_desc.data.__set_${api_prefix}${action}(a_spec);
        }
//::   #endfor
    }

//::   name = act_prof + "_get_plcmt_data"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl", "const int8_t dev_id"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
//::   pd_params = ["sess_hdl", "dev_id"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return;
    }
//::   #endif

//::   name = act_prof + "_get_first_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int32_t index = -1;
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &index);
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
        return index;
    }
//::   name = act_prof + "_get_next_members"
//::   pd_name = pd_prefix + name
//::   params = ["std::vector<int32_t> &next_entry_handles",
//::             "const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const EntryHandle_t entry_hdl",
//::             "const int32_t n"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        next_entry_handles = std::vector<int32_t>(n);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry_hdl, n, next_entry_handles.data());
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
    }
//::   if not select_hdl: continue
//::   name = act_prof + "_get_first_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int32_t index = -1;
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, &index);
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
        return index;
    }
//::   name = act_prof + "_get_next_groups"
//::   pd_name = pd_prefix + name
//::   params = ["std::vector<int32_t> &next_group_handles",
//::             "const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const EntryHandle_t entry_hdl",
//::             "const int32_t n"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        next_group_handles = std::vector<int32_t>(n);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry_hdl, n, next_group_handles.data());
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
    }
//::   name = act_prof + "_get_first_group_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const int8_t dev_id, const EntryHandle_t entry_hdl) {
       int32_t index = -1;
       int status = ${pd_name}(sess_hdl, dev_id, entry_hdl, &index);
       if (status != 0) {
         InvalidTableOperation iop;
         iop.code = status;
         throw iop;
       }
       return index;
    }
//::   name = act_prof + "_get_next_group_members"
//::   pd_name = pd_prefix + name
//::   params = ["std::vector<int32_t> &next_member_handles",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t group_hdl",
//::             "const EntryHandle_t mbr_hdl",
//::             "const int32_t n"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        next_member_handles = std::vector<int32_t>(n);
        int status = ${pd_name}(sess_hdl, dev_id, group_hdl, mbr_hdl, n, next_member_handles.data());
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
    }
//::   if gen_md_pd:
//::     name = act_prof + "_get_word_llp_active_member_count"
//::     pd_name = pd_prefix + name
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt",
//::               "const int32_t word_index"]
//::     param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int32_t count = 0;
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, word_index, &count);
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
        return count;
    }
//::     name = act_prof + "_get_word_llp_active_members"
//::     pd_name = pd_prefix + name
//::     params = ["std::vector<int32_t> &active_member_handles",
//::               "const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt",
//::               "const int32_t word_index",
//::               "const int32_t count"]
//::     param_str = ", ".join(params)
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        active_member_handles = std::vector<int32_t>(count);
        int status = ${pd_name}(sess_hdl, pd_dev_tgt, word_index, count, active_member_handles.data());
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
    }
//::     name = act_prof + "_sel_get_plcmt_data"
//::     pd_name = pd_prefix + name
//::     params = ["const SessionHandle_t sess_hdl", "const int8_t dev_id"]
//::     param_str = ", ".join(params)
    void ${name}(${param_str}) {
//::     pd_params = ["sess_hdl", "dev_id"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return;
    }
//::   #endif
//:: #endfor

    // set default action

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   for action in t_info["actions"]:
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     a_info = action_info[action]
//::     if not a_info['allowed_to_be_default_action'][table]: continue
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     param_str = ", ".join(params)
//::     name = table + "_set_default_action_" + action
//::     pd_name = pd_prefix + name
//::     has_meter_spec = False
    int32_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a_info["param_names"],
//::                                         a_info["param_byte_widths"])
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     # direct parameter specs
//::     for res_name, res_type, _ in table_direct_resources[table]:
//::       if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        p4_pd_${res_type}_spec_t *pd_${res_name}_spec_addr = NULL;
        if (${res_name}_spec.is_set == true) {
          ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
          pd_${res_name}_spec_addr = &pd_${res_name}_spec;
        }
//::         pd_params += ["pd_" + res_name + "_spec_addr"]
//::       #endif
//::       if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::     #endfor
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }

//::   #endfor
//:: #endfor

    // INDIRECT ACTION DATA AND MATCH SELECT

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof + "_add_member_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a_info["param_names"],
//::                                         a_info["param_byte_widths"])
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_mbr_hdl_t pd_mbr_hdl;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_params += ["&pd_mbr_hdl"]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_mbr_hdl;
    }

//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof + "_modify_member_with_" + action
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       action_params = gen_action_params(a_info["param_names"],
//::                                         a_info["param_byte_widths"])
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif

//::     pd_params = ["sess_hdl", "dev_id", "mbr"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   #endfor

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_member"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, mbr);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   if not select_hdl: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const int32_t max_grp_size"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_create_group"
//::   pd_name = pd_prefix + name
    GroupHandle_t ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        p4_pd_grp_hdl_t pd_grp_hdl;

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, max_grp_size, &pd_grp_hdl);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_grp_hdl;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_group"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, grp);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_add_member_to_group"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, grp, mbr);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_member_from_group"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, grp, mbr);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr",
//::             "const " + api_prefix + "grp_mbr_state::type mbr_state"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_group_member_state_set"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        enum p4_pd_grp_mbr_state_e pd_mbr_state;
        pd_mbr_state = (enum p4_pd_grp_mbr_state_e) mbr_state;
        int status = ${pd_name}(sess_hdl, dev_id, grp, mbr, pd_mbr_state);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_group_member_state_get"
//::   pd_name = pd_prefix + name
    ${api_prefix}grp_mbr_state::type ${name}(${param_str}) {
        enum p4_pd_grp_mbr_state_e mbr_state;
        int status = ${pd_name}(sess_hdl, dev_id, grp, mbr, &mbr_state);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return (${api_prefix}grp_mbr_state::type)mbr_state;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const std::vector<int8_t> &hash_val"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_group_member_get_from_hash"
//::   pd_name = pd_prefix + name
    MemberHandle_t ${name}(${param_str}) {
        p4_pd_mbr_hdl_t mbr;
        int status = ${pd_name}(sess_hdl, dev_id, grp, (uint8_t *)hash_val.data(), hash_val.size(), &mbr);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return (MemberHandle_t)mbr;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_set_dynamic_action_selection_fallback_member"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(sess_hdl, pd_dev_tgt, mbr);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_reset_dynamic_action_selection_fallback_member"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(sess_hdl, pd_dev_tgt);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: #endfor


    typedef struct _sel_update_cookie_s {
      ${p4_prefix}Handler *handler;
      int32_t cookie;
    } _sel_update_cookie_t;

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)

//::   if not select_hdl: continue
    pthread_mutex_t _${table}_sel_update_m;
    std::map<int, std::vector<${api_prefix}sel_update_t> > _${table}_sel_updates;

    void _${table}_sel_update_store(p4_pd_sess_hdl_t sess_hdl,
                                    p4_pd_dev_target_t dev_tgt,
                                    int32_t cookie,
                                    unsigned int grp_hdl,
                                    unsigned int mbr_hdl,
                                    int index,
                                    bool is_add) {
      DevTarget_t dt;
      dt.dev_id = dev_tgt.device_id;
      dt.dev_pipe_id = dev_tgt.dev_pipe_id;
      ${api_prefix}sel_update_t info;
      info.sess_hdl = sess_hdl;
      info.dev_tgt = dt;
      info.cookie  = cookie;
      info.grp_hdl = grp_hdl;
      info.mbr_hdl = mbr_hdl;
      info.index = index;
      info.is_add = is_add;
      pthread_mutex_lock(&_${table}_sel_update_m);
      _${table}_sel_updates[dt.dev_id].push_back(info);
      pthread_mutex_unlock(&_${table}_sel_update_m);
    }
    static int _${table}_sel_update_cb(p4_pd_sess_hdl_t sess_hdl,
                                       p4_pd_dev_target_t dev_tgt,
                                       void *cookie,
                                       unsigned int grp_hdl,
                                       unsigned int mbr_hdl,
                                       int index,
                                       bool is_add) {
      _sel_update_cookie_t *data = (_sel_update_cookie_t*)cookie;
      data->handler->_${table}_sel_update_store(sess_hdl, dev_tgt, data->cookie, grp_hdl, mbr_hdl, index, is_add);
      return 0;
    }

//::   params = ["const int8_t dev_id",
//::             "const int32_t cookie"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_sel_track_updates"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        _sel_update_cookie_t *data = (_sel_update_cookie_t *)malloc(sizeof(_sel_update_cookie_t));
        data->handler = this;
        data->cookie = cookie;
        int status = ${pd_name}(dev_id, _${table}_sel_update_cb, data);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["std::vector<" + api_prefix + "sel_update_t> &updates",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_sel_get_updates"
//::   pd_name = pd_prefix + name
    void ${name}(${param_str}) {
      pthread_mutex_lock(&_${table}_sel_update_m);
      updates.swap(_${table}_sel_updates[dev_id]);
      pthread_mutex_unlock(&_${table}_sel_update_m);
    }

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["const int32_t priority"]
//::   #endif
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   if t_info["timeout"]:
//::     params_wo += ["const int32_t ttl"]
//::   #endif
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::   param_str = ", ".join(params_wo)
//::   name = table + "_add_entry"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   for name, width in match_params:
//::     if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::     else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::     #endif
//::   #endfor

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["mbr"]
//::   if t_info["timeout"]:
//::     pd_params += ["(uint32_t) ttl"]
//::   #endif
//::   # direct parameter specs
//::   for res_name, res_type, _ in table_direct_resources[table]:
//::     if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::     if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::   #endfor
//::   pd_params += [r + "_index" for r in bound_indirect_res]
//::   pd_params += ["&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }

//::   if not select_hdl : continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   if t_info["timeout"]:
//::     params_w += ["const int32_t ttl"]
//::   #endif
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   params_w += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::   param_str = ", ".join(params_w)
//::   name = table + "_add_entry_with_selector"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   for name, width in match_params:
//::     if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::     else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::     #endif
//::   #endfor

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["grp"]
//::   if t_info["timeout"]:
//::     pd_params += ["(uint32_t) ttl"]
//::   #endif
//::   # direct parameter specs
//::   for res_name, res_type, _ in table_direct_resources[table]:
//::     if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::     if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::   #endfor
//::   pd_params += [r + "_index" for r in bound_indirect_res]
//::   pd_params += ["&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = table + "_modify_entry"
//::     params = ["const SessionHandle_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["const DevTarget_t &dev_tgt",
//::                  "const " + api_prefix + table + "_match_spec_t &match_spec"]
//::       pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["const int32_t priority"]
//::         pd_params += ["priority"]
//::       #endif
//::     else:
//::       params += ["const int8_t dev_id",
//::                  "const EntryHandle_t entry"]
//::       pd_params = ["sess_hdl", "dev_id", "entry"]
//::     #endif
//::     pd_params += ["mbr"]
//::     params_wo = params + ["const MemberHandle_t mbr"]
//::     params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       bound_indirect_res += [indirect_res_name]
//::     #endfor
//::     params_wo += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::     param_str = ", ".join(params_wo)
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {

//::     if idx:
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::       match_params = gen_match_params(t_info["match_fields"], field_info)
//::       for name, width in match_params:
//::         if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::         else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
//::
//::     # direct parameter specs
//::     for res_name, res_type, _ in table_direct_resources[table]:
//::       if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::       if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::     #endfor
//::     pd_params += [r + "_index" for r in bound_indirect_res]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     if not select_hdl : continue
//::     params_w = params + ["const GroupHandle_t grp"]
//::     params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     params_w += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::     param_str = ", ".join(params_w)
//::     name = table + "_modify_entry_with_selector"
//::     if idx:
//::       name += "_by_match_spec"
//::       pd_params = ["sess_hdl", "pd_dev_tgt", "&pd_match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         pd_params += ["priority"]
//::       #endif
//::     else:
//::       pd_params = ["sess_hdl", "dev_id", "entry"]
//::     #endif
//::     pd_params += ["grp"]
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {

//::     if idx:
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::       match_params = gen_match_params(t_info["match_fields"], field_info)
//::       for name, width in match_params:
//::         if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::         else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
//::
//::     # direct parameter specs
//::     for res_name, res_type, _ in table_direct_resources[table]:
//::       if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::       if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::         pd_params += ["&pd_" + res_name + "_spec"]
//::       #endif
//::     #endfor
//::     pd_params += [r + "_index" for r in bound_indirect_res]
//::     pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   #endfor
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::   param_str = ", ".join(params_wo)
//::   name = table + "_set_default_entry"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["mbr"]
//::   # direct parameter specs
//::   for res_name, res_type, _ in table_direct_resources[table]:
//::     if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::     if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::   #endfor
//::   pd_params += [r + "_index" for r in bound_indirect_res]
//::   pd_params += ["&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }

//::   if not select_hdl: continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   params_w += ["const int32_t " + r + "_index" for r in bound_indirect_res]
//::   param_str = ", ".join(params_w)
//::   name = table + "_set_default_entry_with_selector"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["grp"]
//::   # direct parameter specs
//::   for res_name, res_type, _ in table_direct_resources[table]:
//::     if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::     if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::       pd_params += ["&pd_" + res_name + "_spec"]
//::     #endif
//::   #endfor
//::   pd_params += [r + "_index" for r in bound_indirect_res]
//::   pd_params += ["&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return pd_entry;
    }
//:: #endfor

//:: if gen_exm_test_pd == 1:
      // Exact match entry activate functions
//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const EntryHandle_t entry"]
//::     param_str = ", ".join(params)
//::     name = table + "_activate_entry"
//::     pd_name = pd_prefix + name
      void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, entry);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
      }
//::   #endfor

      // Exact match entry de-activate functions
//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const EntryHandle_t entry"]
//::     param_str = ", ".join(params)
//::     name = table + "_deactivate_entry"
//::     pd_name = pd_prefix + name
      void ${name}(${param_str}) {
        int status = ${pd_name}(sess_hdl, dev_id, entry);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
      }
//::   #endfor
//:: #endif

     // Table set/get property

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = table + "_set_property"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const tbl_property_t::type property",
//::             "const tbl_property_value_t::type value",
//::             "const int32_t prop_args"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
      p4_pd_tbl_prop_type_t pipe_property = (p4_pd_tbl_prop_type_t)property;
      p4_pd_tbl_prop_value_t property_value;
      p4_pd_tbl_prop_args_t property_args;
      property_value.value = (uint32_t)value;
      property_args.value = (uint32_t)prop_args;
      int status = ${pd_name}(sess_hdl, dev_id, pipe_property, property_value, property_args);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }
    }

//::   name = table + "_get_property"
//::   pd_name = pd_prefix + name
//::   params = ["tbl_property_value_args_t& _return",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const tbl_property_t::type property"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
      p4_pd_tbl_prop_type_t pipe_property = (p4_pd_tbl_prop_type_t)property;
      p4_pd_tbl_prop_value_t property_value;
      p4_pd_tbl_prop_args_t property_args;

      int status = ${pd_name}(sess_hdl, dev_id, pipe_property, &property_value, &property_args);
      if(status != 0) {
        InvalidTableOperation iop;
        iop.code = status;
        throw iop;
      }

      switch (pipe_property)
      {
        case PD_TABLE_ENTRY_SCOPE:
        {
          if (property_value.value == 0) {
            _return.value = tbl_property_value_t::ENTRY_SCOPE_ALL_PIPELINES;
          } else if (property_value.value == 1) {
            _return.value = tbl_property_value_t::ENTRY_SCOPE_SINGLE_PIPELINE;
          } else {
            _return.value = tbl_property_value_t::ENTRY_SCOPE_USER_DEFINED;
          }
          break;
        }
        case PD_TERN_TABLE_ENTRY_PLACEMENT:
        {
          if (property_value.value) {
            _return.value = tbl_property_value_t::TERN_ENTRY_PLACEMENT_APP_MANAGED;
          } else {
            _return.value = tbl_property_value_t::TERN_ENTRY_PLACEMENT_DRV_MANAGED;
          }
          break;
        }
        case PD_DUPLICATE_ENTRY_CHECK:
        {
          if (property_value.value) {
            _return.value = tbl_property_value_t::DUPLICATE_ENTRY_CHECK_ENABLE;
          } else {
            _return.value = tbl_property_value_t::DUPLICATE_ENTRY_CHECK_DISABLE;
          }
          break;
        }
        case PD_IDLETIME_REPEATED_NOTIFICATION:
        {
          if (property_value.value) {
            _return.value = tbl_property_value_t::IDLETIME_REPEATED_NOTIFICATION_ENABLE;
          } else {
            _return.value = tbl_property_value_t::IDLETIME_REPEATED_NOTIFICATION_DISABLE;
          }
          break;
        }
      }
      _return.scope_args = property_args.value;
    }

//:: #endfor


//:: for lq in learn_quanta:
//::   rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
//::   rpc_entry_type = api_prefix + lq["name"] + "_digest_entry_t"
  std::map<SessionHandle_t, std::list<${rpc_msg_type}> > ${lq["name"]}_digest_queues;
  pthread_mutex_t ${lq["name"]}_mutex;

  p4_pd_status_t
  ${p4_prefix}${lq["name"]}_receive(const SessionHandle_t sess_hdl,
                                    const ${rpc_msg_type} &msg) {
    pthread_mutex_lock(&${lq["name"]}_mutex);
    assert(${lq["name"]}_digest_queues.find(sess_hdl) != ${lq["name"]}_digest_queues.end());
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq["name"]}_digest_queues.find(sess_hdl);
    digest_queue->second.push_back(msg);
    pthread_mutex_unlock(&${lq["name"]}_mutex);

    return 0;
  }

  static p4_pd_status_t
  ${p4_prefix}_${lq["name"]}_cb(p4_pd_sess_hdl_t sess_hdl,
                                ${lq["msg_type"]} *msg,
                                void *cookie) {
    ${rpc_msg_type} rpc_msg;
    rpc_msg.msg_ptr = (int64_t)msg;
    rpc_msg.dev_tgt.dev_id = msg->dev_tgt.device_id;
    rpc_msg.dev_tgt.dev_pipe_id = msg->dev_tgt.dev_pipe_id;
    for (int i = 0; (msg != NULL) && (i < msg->num_entries); ++i) {
      ${rpc_entry_type} entry;
//::   for field in lq["fields"]:
//::     field_name = field["field_name"]
//::     bit_width = field_info[field_name]["bit_width"]
//::     byte_width = (bit_width + 7 ) // 8
//::     if byte_width > 4:
      entry.${field_name}.insert(entry.${field_name}.end(), msg->entries[i].${field_name}, msg->entries[i].${field_name} + ${byte_width});
//::     else:
      entry.${field_name} = msg->entries[i].${field_name};
//::     #endif
//::   #endfor
      rpc_msg.msg.push_back(entry);
    }
    return ((${p4_prefix}Handler*)cookie)->${p4_prefix}${lq["name"]}_receive((SessionHandle_t)sess_hdl, rpc_msg);
  }

  void ${lq["name"]}_register( const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${lq["register_fn"]}(sess_hdl, dev_id, ${p4_prefix}_${lq["name"]}_cb, this);
    pthread_mutex_lock(&${lq["name"]}_mutex);
    ${lq["name"]}_digest_queues.insert(std::pair<SessionHandle_t, std::list<${rpc_msg_type}> >(sess_hdl, std::list<${rpc_msg_type}>()));
    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_deregister(const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${lq["deregister_fn"]}(sess_hdl, dev_id);
    pthread_mutex_lock(&${lq["name"]}_mutex);
    ${lq["name"]}_digest_queues.erase(sess_hdl);
    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_get_digest(${rpc_msg_type} &msg, const SessionHandle_t sess_hdl) {
    msg.msg_ptr = 0;
    msg.msg.clear();

    pthread_mutex_lock(&${lq["name"]}_mutex);
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq["name"]}_digest_queues.find(sess_hdl);
    if (digest_queue != ${lq["name"]}_digest_queues.end()) {
      if (digest_queue->second.size() > 0) {
        msg = digest_queue->second.front();
        digest_queue->second.pop_front();
      }
    }

    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_digest_notify_ack(const SessionHandle_t sess_hdl, const int64_t msg_ptr) {
    ${lq["notify_ack_fn"]}((p4_pd_sess_hdl_t)sess_hdl, (${lq["msg_type"]}*)msg_ptr);
  }
//:: #endfor

//:: name = "set_learning_timeout"
//:: pd_name = pd_prefix + name
    void ${name}(const SessionHandle_t sess_hdl,
                 const int8_t device_id,
                 const int32_t usecs){
        int status = ${pd_name}(sess_hdl, device_id, usecs);
        if(status != 0) {
          InvalidLearnOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "tbl_dbg_counter_type_set"
//:: pd_name = "p4_pd_" + name
    void ${name}(const DevTarget_t &dev_tgt,
                  const std::string& tbl_name,
                  const tbl_dbg_counter_type_t::type t_type) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(pd_dev_tgt, (char*)tbl_name.c_str(),
                (p4_pd_tbl_dbg_counter_type_t)t_type);
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "tbl_dbg_counter_get"
//:: pd_name = "p4_pd_" + name
    void ${name}(TblCntrInfo_t &info,
                 const DevTarget_t &dev_tgt,
                 const std::string& tbl_name) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        p4_pd_tbl_dbg_counter_type_t pd_type;
        uint32_t value;

        int status = ${pd_name}(pd_dev_tgt, (char*)tbl_name.c_str(),
                   &pd_type, &value);
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }
        info.type = (tbl_dbg_counter_type_t::type)pd_type;
        info.value = value;
    }

//:: name = "tbl_dbg_counter_clear"
//:: pd_name = "p4_pd_" + name
    void ${name}(const DevTarget_t &dev_tgt,
                    const std::string& tbl_name) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(pd_dev_tgt, (char*)tbl_name.c_str());
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "tbl_dbg_counter_type_stage_set"
//:: pd_name = "p4_pd_" + name
    void ${name}(const DevTarget_t &dev_tgt,
                    const int8_t stage,
                    const tbl_dbg_counter_type_t::type t_type) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(pd_dev_tgt, stage,
                (p4_pd_tbl_dbg_counter_type_t)t_type);
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "tbl_dbg_counter_stage_get"
//:: pd_name = "p4_pd_" + name
    void ${name}(TblDbgStageInfo_t &info,
                 const DevTarget_t &dev_tgt,
                 const int8_t stage) {
        p4_pd_stage_tbl_dbg_counters_t pd_cntrs;
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
        memset(&pd_cntrs, 0, sizeof(pd_cntrs));

        int status = ${pd_name}(pd_dev_tgt, stage, &pd_cntrs);
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }

        info.num_counters = pd_cntrs.num_counters;
        if (pd_cntrs.num_counters != 0) {
          info.tbl_name.resize(pd_cntrs.num_counters);
          info.type.resize(pd_cntrs.num_counters);
          info.value.resize(pd_cntrs.num_counters);

          for (int i = 0; i < pd_cntrs.num_counters; i++) {
            info.tbl_name[i] = std::string((char *)pd_cntrs.counter[i].tbl_name);
            info.type[i] =
                (tbl_dbg_counter_type_t::type)pd_cntrs.counter[i].cntr_type;
            info.value[i] = pd_cntrs.counter[i].value;
          }
        }

    }

//:: name = "tbl_dbg_counter_stage_clear"
//:: pd_name = "p4_pd_" + name
    void ${name}(const DevTarget_t &dev_tgt,
                    const int8_t stage) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int status = ${pd_name}(pd_dev_tgt, stage);
        if (status != 0) {
          InvalidDbgOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_create"
//:: pd_name = pd_prefix + name
    SnapshotHandle_t ${name}(const DevTarget_t &dev_tgt,
                             const int8_t start_stage, const int8_t end_stage,
                             const int8_t direction) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        int32_t handle = 0;

	int status = ${pd_name}(pd_dev_tgt, start_stage, end_stage,
                   (bf_snapshot_dir_t)direction,
                   (pipe_snapshot_hdl_t*)&handle);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
        return handle;
    }
//:: name = "snapshot_delete"
//:: pd_name = pd_prefix + name
    void ${name}(const SnapshotHandle_t handle) {
	int status = ${pd_name}((bf_snapshot_state_t)handle);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_state_set"
//:: pd_name = "p4_pd_" + name
    void ${name}(const SnapshotHandle_t handle, const int32_t state,
               const int32_t usecs) {
	int status = ${pd_name}((pipe_snapshot_hdl_t)handle,
                          (bf_snapshot_state_t)state, usecs);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_state_get"
//:: pd_name = "p4_pd_" + name
    int32_t ${name}(const SnapshotHandle_t handle, const int16_t pipe) {
        bf_snapshot_state_t state;

	int status = ${pd_name}((pipe_snapshot_hdl_t)handle,
                   pipe, &state);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
        return state;
    }

//:: name = "snapshot_timer_enable"
//:: pd_name = "p4_pd_" + name
    void ${name}(const SnapshotHandle_t handle, const int8_t disable) {
	int status = ${pd_name}((pipe_snapshot_hdl_t)handle, disable);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_capture_trigger_set"
//:: pd_name = pd_prefix + name
    void ${name}(const SnapshotHandle_t handle,
                    const ${api_prefix}snapshot_trig_spec_t& trig_spec,
                    const ${api_prefix}snapshot_trig_spec_t& trig_spec2) {
        ${pd_prefix}snapshot_trig_spec_t pd_trig_spec;
        ${pd_prefix}snapshot_trig_spec_t pd_trig_mask;
        int dir = (handle >> 1) & 0x1;  // dir is bit 1 of hdl

        memset(&pd_trig_spec, 0, sizeof(pd_trig_spec));
        memset(&pd_trig_mask, 0, sizeof(pd_trig_mask));

        int idx = 0;
        (void)idx;
//:: dir_str = ["ig", "eg"]
        switch (dir)
        {
//:: for dir in range(0,2):
           case ${dir}:
           {
//::   for name in PHV_Container_Fields[dir]:
//::     width = PHV_Container_Fields[dir][name]
//::     if width > 4:
            if (strcmp("${name}", (char*)trig_spec.field_name.c_str()) == 0) {
                for (idx = 0; idx < ${width}; idx++) {
                    pd_trig_spec.u.${dir_str[dir]}.${name}[idx] = (trig_spec.field_value >> (8*(${width} -idx -1))) & 0xff;
                    pd_trig_mask.u.${dir_str[dir]}.${name}[idx] = (trig_spec.field_mask >> (8*(${width} -idx -1))) & 0xff;
                }
            }
            if (strcmp("${name}", (char*)trig_spec2.field_name.c_str()) == 0) {
                for (idx = 0; idx < ${width}; idx++) {
                    pd_trig_spec.u.${dir_str[dir]}.${name}[idx] = (trig_spec2.field_value >> (8*(${width} -idx -1))) & 0xff;
                    pd_trig_mask.u.${dir_str[dir]}.${name}[idx] = (trig_spec2.field_mask >> (8*(${width} -idx -1))) & 0xff;
                }
            }
//::     else:
            if (strcmp("${name}", (char*)trig_spec.field_name.c_str()) == 0) {
                memcpy(&(pd_trig_spec.u.${dir_str[dir]}.${name}),
                            &(trig_spec.field_value), ${width});
                memcpy(&(pd_trig_mask.u.${dir_str[dir]}.${name}),
                            &(trig_spec.field_mask), ${width});
            }
            if (strcmp("${name}", (char*)trig_spec2.field_name.c_str()) == 0) {
                memcpy(&(pd_trig_spec.u.${dir_str[dir]}.${name}),
                            &(trig_spec2.field_value), ${width});
                memcpy(&(pd_trig_mask.u.${dir_str[dir]}.${name}),
                            &(trig_spec2.field_mask), ${width});
            }
//::     #endif
//::   #endfor
//::   for header_name in POV_Dict[dir]:
            if (strcmp("${header_name}_valid", (char*)trig_spec.field_name.c_str()) == 0) {
                pd_trig_spec.u.${dir_str[dir]}.${header_name}_valid =
                        trig_spec.field_value;
                pd_trig_mask.u.${dir_str[dir]}.${header_name}_valid =
                        trig_spec.field_mask;
            }
            if (strcmp("${header_name}_valid", (char*)trig_spec2.field_name.c_str()) == 0) {
                pd_trig_spec.u.${dir_str[dir]}.${header_name}_valid =
                        trig_spec2.field_value;
                pd_trig_mask.u.${dir_str[dir]}.${header_name}_valid =
                        trig_spec2.field_mask;
            }
//::   #endfor
            break;
           }
//:: #endfor
           default:
           {
              break;
           }
        }

        int status = ${pd_name}((bf_snapshot_state_t)handle,
                &pd_trig_spec, &pd_trig_mask);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_capture_data_get"
//:: pd_name = pd_prefix + name
    int64_t ${name}(const SnapshotHandle_t handle, const int16_t pipe,
                    const int16_t stage_id, const std::string& field_name) {
        int64_t field_value = 0;
        ${p4_pd_prefix}snapshot_capture_arr_t capture_arr;
        ${p4_pd_prefix}snapshot_capture_t *capture;
        bf_snapshot_capture_ctrl_info_arr_t capture_ctrl_arr;
        int32_t num_captures;
        int dir = (handle >> 1) & 0x1;  // dir is bit 1 of hdl
        int idx = 0;
        uint8_t *data_ptr;

        (void)idx;
        (void)data_ptr;

        memset(&capture_arr, 0, sizeof(capture_arr));
        memset(&capture_ctrl_arr, 0, sizeof(capture_ctrl_arr));

        int status = ${pd_name}((bf_snapshot_state_t)handle, pipe,
                &capture_arr, &capture_ctrl_arr, &num_captures);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
        capture = &(capture_arr.captures[0]);
        /* Go to the right stage */
        for (idx = 0; idx<BF_MAX_SNAPSHOT_CAPTURES; idx++) {
            if ((capture_ctrl_arr.ctrl[idx].valid) &&
                (capture_ctrl_arr.ctrl[idx].stage_id == stage_id)) {
                capture = &(capture_arr.captures[idx]);
                break;
            }
        }

//:: dir_str = ["ig", "eg"]
        switch (dir)
        {
//:: for dir in range(0,2):
           case ${dir}:
           {
//::   for name in PHV_Container_Fields[dir]:
//::     width = PHV_Container_Fields[dir][name]
//::     if width > 4:
            if (strcmp("${name}", (char*)field_name.c_str()) == 0) {
                for (idx = 0; idx < ${width}; idx++) {
                    field_value |= ((int64_t)capture->u.${dir_str[dir]}.${name}[idx]) << (8*(${width} -idx -1));
                }
            }
//::     else:
            if (strcmp("${name}", (char*)field_name.c_str()) == 0) {
                memcpy(&field_value, &(capture->u.${dir_str[dir]}.${name}),
                              (${width}<=8)?${width}:8);
            }
//::     #endif
//::   #endfor
//::   for header_name in POV_Dict[dir]:
            if (strcmp("${header_name}_valid", (char*)field_name.c_str()) == 0) {
                field_value = capture->u.${dir_str[dir]}.${header_name}_valid;
            }
//::   #endfor
              break;
           }
//:: #endfor
           default:
           {
              break;
           }
        }

        return field_value;
    }

//:: name = "snapshot_capture_tbl_data_get"
//:: pd_name = pd_prefix + "snapshot_capture_data_get"
    void ${name}(${api_prefix}snapshot_tbl_data_t& _return, const SnapshotHandle_t handle, const int16_t pipe, const std::string& table_name) {
      ${p4_pd_prefix}snapshot_capture_arr_t capture_arr;
      bf_snapshot_capture_ctrl_info_arr_t capture_ctrl_arr;
      int32_t num_captures = 0;

      memset(&capture_arr, 0, sizeof(capture_arr));
      memset(&capture_ctrl_arr, 0, sizeof(capture_ctrl_arr));

      int status = ${pd_name}((bf_snapshot_state_t)handle, pipe,
              &capture_arr, &capture_ctrl_arr, &num_captures);
      if (status != 0) {
        InvalidSnapshotOperation iop;
        iop.code = status;
        throw iop;
      }
      /* Check all stages. */
      for (int idx = 0; idx<BF_MAX_SNAPSHOT_CAPTURES; idx++) {
        bf_snapshot_capture_ctrl_info_t *ctrl = &capture_ctrl_arr.ctrl[idx];
        if (!ctrl->valid) continue;
        for (int lt=0; lt<BF_MAX_LOG_TBLS; ++lt) {
          bf_snapshot_tables_info_t *tbl = &ctrl->tables_info[lt];
          if (!strcmp((char*)table_name.c_str(), tbl->table_name)) {
            _return.hit = tbl->table_hit;
            _return.inhibited = tbl->table_inhibited;
            _return.executed = tbl->table_executed;
            _return.hit_entry_handle = tbl->hit_entry_handle;
            if (_return.hit || _return.inhibited) return;
          }
        }
      }
    }

//:: name = "snapshot_capture_trigger_fields_clr"
//:: pd_name = "p4_pd_" + name
    void ${name}(const SnapshotHandle_t handle) {

	int status = ${pd_name}((pipe_snapshot_hdl_t)handle);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//:: name = "snapshot_field_in_scope"
//:: pd_name = "p4_pd_" + name
    bool ${name}(const DevTarget_t &dev_tgt,
                 const int8_t stage, const int8_t direction,
                 const std::string& field_name) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        bool exists = false;

	int status = ${pd_name}(pd_dev_tgt, stage,
                   (bf_snapshot_dir_t)direction,
                   (char*)field_name.c_str(), &exists);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
        return exists;
    }

//:: name = "snapshot_trigger_field_in_scope"
//:: pd_name = "p4_pd_" + name
    bool ${name}(const DevTarget_t &dev_tgt,
                 const int8_t stage, const int8_t direction,
                 const std::string& field_name) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

        bool exists = false;

	int status = ${pd_name}(pd_dev_tgt, stage,
                   (bf_snapshot_dir_t)direction,
                   (char*)field_name.c_str(), &exists);
        if (status != 0) {
          InvalidSnapshotOperation iop;
          iop.code = status;
          throw iop;
        }
        return exists;
    }

  // COUNTERS
//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "counter_read_" + counter
//::     pd_name = pd_prefix + name
    void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_flags_t &flags) {
      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = {0};
      int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry, pd_flags, &value);
      if(status != 0) {
        InvalidCounterOperation iop;
        iop.code = status;
        throw iop;
      }
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
  }

//::     name = "counter_write_" + counter
//::     pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_value_t &counter_value) {
      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry, value);
      if(status != 0) {
        InvalidCounterOperation iop;
        iop.code = status;
        throw iop;
      }
  }

//::   else:
//::     name = "counter_read_" + counter
//::     pd_name = pd_prefix + name
  void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_flags_t &flags) {
      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = {0};
      int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, pd_flags, &value);
      if(status != 0) {
        InvalidCounterOperation iop;
        iop.code = status;
        throw iop;
      }
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
  }

//::     name = "counter_write_" + counter
//::     pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_value_t &counter_value) {
      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, value);
      if(status != 0) {
        InvalidCounterOperation iop;
        iop.code = status;
        throw iop;
      }
  }

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "counter_hw_sync_" + counter
//::   pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, bool blocking) {
    PipeMgrSimpleCb cb_data;
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int status = 0;
    /* NOTE : This call is always blocking from a thrift client perspective. But when a NULL
     * callback function is passed, pipe-mgr makes this call blocking as opposed to the
     * callback mechansim.
     */
    if (!blocking) {
      status = ${pd_name}(sess_hdl, pd_dev_tgt, PipeMgrSimpleCb::cb_fn, (void *) &cb_data);
    } else {
      status = ${pd_name}(sess_hdl, pd_dev_tgt, NULL, NULL);
    }
    if(status != 0) {
      InvalidCounterOperation iop;
      iop.code = status;
      throw iop;
    }
    if (!blocking) {
      cb_data.wait(); // blocking, until callback happens
    }
  }

//:: #endfor

  // REGISTERS
//:: for register, r_info in register_info.items():
//::   binding = r_info["binding"]
//::   imp_type = r_info["v_thrift_type_imp"]
//::   if r_info["layout"]:
//::     param_type = "const " + imp_type + " &"
//::   else:
//::     param_type = "const " + imp_type
//::   #endif
//::   table_type = r_info["table_type"]
//::   if table_type == "fifo":
//::     name = "register_occupancy_" + register
//::     pd_name = pd_prefix + name
  int32_t ${name}(const  ::res_pd_rpc::SessionHandle_t sess_hdl, const  ::res_pd_rpc::DevTarget_t& dev_tgt) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int occupancy;
    int status = ${pd_name}(sess_hdl, pd_dev_tgt, &occupancy);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
    return occupancy;
  }

//::     name = "register_reset_" + register
//::     pd_name = pd_prefix + name
  void ${name}(const  ::res_pd_rpc::SessionHandle_t sess_hdl, const  ::res_pd_rpc::DevTarget_t& dev_tgt) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int status = ${pd_name}(sess_hdl, pd_dev_tgt);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
    return;
  }

//::     if r_info['direction'] == "in":
//::       name = "register_dequeue_" + register
//::       pd_name = pd_prefix + name
  void ${name}(std::vector<${imp_type}> & _return, const ::res_pd_rpc::SessionHandle_t sess_hdl, const ::res_pd_rpc::DevTarget_t& dev_tgt, const int32_t num_to_dequeue) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int num_dequeued;

    ${r_info["v_type"]} *values = new ${r_info["v_type"]}[num_to_dequeue];
    int status = ${pd_name}(sess_hdl, pd_dev_tgt, num_to_dequeue, values, &num_dequeued);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.reserve(num_dequeued);
    for (int i=0; i<num_dequeued; ++i) {
      _return.push_back( register_${register}_spec_pd_to_thrift(values[i]) );
    }
    delete[] values;
  }
//::     #endif
//::     if r_info['direction'] == "out":
//::       name = "register_enqueue_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const ::res_pd_rpc::SessionHandle_t sess_hdl, const ::res_pd_rpc::DevTarget_t& dev_tgt, const std::vector<${imp_type}> & register_values) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    ${r_info["v_type"]} *pd_specs = new ${r_info["v_type"]}[register_values.size()];
    for (int i=0; i<register_values.size(); ++i) {
      pd_specs[i] = register_${register}_spec_thrift_to_pd( register_values[i] );
    }

    int status = ${pd_name}(sess_hdl, pd_dev_tgt, register_values.size(), pd_specs);
    delete[] pd_specs;
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
  }
//::     #endif
//::   else:
//::     name = "register_hw_sync_" + register
//::     pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
    PipeMgrSimpleCb cb_data;
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int status = ${pd_name}(sess_hdl, pd_dev_tgt, PipeMgrSimpleCb::cb_fn, (void *) &cb_data);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
    cb_data.wait(); // blocking, until callback happens
  }

//::     if binding[0] == "direct":
//::       name = "register_read_" + register
//::       pd_name = pd_prefix + name
  void ${name}(std::vector<${imp_type}> &register_values, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}register_flags_t &flags) {

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= REGISTER_READ_HW_SYNC;

      ${r_info["v_type"]} values[4];
      int value_count = 0;
      int status = ${pd_name}(sess_hdl, pd_dev_tgt, entry, pd_flags, values, &value_count);
      if(status != 0) {
        InvalidRegisterOperation iop;
        iop.code = status;
        throw iop;
      }
      int i;
      register_values.reserve(value_count);
      for(i = 0; i < value_count; i++) {
        register_values.push_back(register_${register}_spec_pd_to_thrift(values[i]));
      }
  }

//::       name = "register_write_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const int8_t dev_id, const EntryHandle_t entry, ${param_type} register_value) {
      auto value = register_${register}_spec_thrift_to_pd(register_value);

      int status = ${pd_name}(sess_hdl, dev_id, entry, &value);
      if(status != 0) {
        InvalidRegisterOperation iop;
        iop.code = status;
        throw iop;
      }
  }

//::     else:
//::       name = "register_read_" + register
//::       pd_name = pd_prefix + name
  void ${name}(std::vector<${imp_type}> &register_values, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}register_flags_t &flags) {

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= REGISTER_READ_HW_SYNC;

      ${r_info["v_type"]} values[4];
      int value_count = 0;
      int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, pd_flags, values, &value_count);
      if(status != 0) {
        InvalidRegisterOperation iop;
        iop.code = status;
        throw iop;
      }
      int i;
      register_values.reserve(value_count);
      for(i = 0; i < value_count; i++) {
        register_values.push_back(register_${register}_spec_pd_to_thrift(values[i]));
      }
  }

//::       name = "register_write_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, ${param_type} register_value) {

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      auto value = register_${register}_spec_thrift_to_pd(register_value);

      int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, &value);
      if(status != 0) {
        InvalidRegisterOperation iop;
        iop.code = status;
        throw iop;
      }
  }

//::       name = "register_reset_all_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    int status = ${pd_name}(sess_hdl, pd_dev_tgt);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
  }

//::       name = "register_range_reset_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const int32_t count) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, count);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
  }

//::       name = "register_write_all_" + register
//::       pd_name = pd_prefix + name
  void ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, ${param_type} register_value) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    auto value = register_${register}_spec_thrift_to_pd(register_value);

    int status = ${pd_name}(sess_hdl, pd_dev_tgt, &value);
    if(status != 0) {
      InvalidRegisterOperation iop;
      iop.code = status;
      throw iop;
    }
  }

//::       name = "register_range_read_" + register
//::       pd_name = pd_prefix + name
  void ${name}(std::vector<${imp_type}> &register_values, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const int32_t count, const ${api_prefix}register_flags_t &flags) {

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= REGISTER_READ_HW_SYNC;

      ${r_info["v_type"]} *values = new ${r_info["v_type"]}[4*count];
      int value_count = 0;
      int num_actually_read = 0;
      int status = ${pd_name}(sess_hdl, pd_dev_tgt, index, count, pd_flags, &num_actually_read, values, &value_count);
      if(status != 0) {
        InvalidRegisterOperation iop;
        iop.code = status;
        throw iop;
      }
      int i;
      register_values.reserve(value_count);
      for(i = 0; i < value_count; i++) {
        register_values.push_back(register_${register}_spec_pd_to_thrift(values[i]));
      }
      delete[] values;
  }
//::     #endif
//::   #endif

//:: #endfor

  void bytes_meter_spec_thrift_to_pd(const ${api_prefix}bytes_meter_spec_t &meter_spec,
                                     p4_pd_bytes_meter_spec_t *pd_meter_spec) {
    pd_meter_spec->cir_kbps = meter_spec.cir_kbps;
    pd_meter_spec->cburst_kbits = meter_spec.cburst_kbits;
    pd_meter_spec->pir_kbps = meter_spec.pir_kbps;
    pd_meter_spec->pburst_kbits = meter_spec.pburst_kbits;
    pd_meter_spec->meter_type = meter_spec.color_aware ?
      PD_METER_TYPE_COLOR_AWARE : PD_METER_TYPE_COLOR_UNAWARE;
  }

  void packets_meter_spec_thrift_to_pd(const ${api_prefix}packets_meter_spec_t &meter_spec,
                                       p4_pd_packets_meter_spec_t *pd_meter_spec) {
    pd_meter_spec->cir_pps = meter_spec.cir_pps;
    pd_meter_spec->cburst_pkts = meter_spec.cburst_pkts;
    pd_meter_spec->pir_pps = meter_spec.pir_pps;
    pd_meter_spec->pburst_pkts = meter_spec.pburst_pkts;
    pd_meter_spec->meter_type = meter_spec.color_aware ?
      PD_METER_TYPE_COLOR_AWARE : PD_METER_TYPE_COLOR_UNAWARE;
  }

  void bytes_meter_spec_pd_to_thrift(${api_prefix}bytes_meter_spec_t &meter_spec,
                                     const p4_pd_bytes_meter_spec_t *pd_meter_spec) {
    meter_spec.cir_kbps = pd_meter_spec->cir_kbps;
    meter_spec.cburst_kbits = pd_meter_spec->cburst_kbits;
    meter_spec.pir_kbps = pd_meter_spec->pir_kbps;
    meter_spec.pburst_kbits = pd_meter_spec->pburst_kbits;
    meter_spec.color_aware = pd_meter_spec->meter_type == PD_METER_TYPE_COLOR_AWARE ? true : false;
  }

  void packets_meter_spec_pd_to_thrift(${api_prefix}packets_meter_spec_t &meter_spec,
                                       const p4_pd_packets_meter_spec_t *pd_meter_spec) {
    meter_spec.cir_pps = pd_meter_spec->cir_pps;
    meter_spec.cburst_pkts = pd_meter_spec->cburst_pkts;
    meter_spec.pir_pps = pd_meter_spec->pir_pps;
    meter_spec.pburst_pkts = pd_meter_spec->pburst_pkts;
    meter_spec.color_aware = pd_meter_spec->meter_type == PD_METER_TYPE_COLOR_AWARE ? true : false;
  }

  void lpf_spec_thrift_to_pd(const ${api_prefix}lpf_spec_t &lpf_spec,
                             p4_pd_lpf_spec *pd_lpf_spec) {
    pd_lpf_spec->gain_decay_separate_time_constant =
      lpf_spec.gain_decay_separate_time_constant;
    pd_lpf_spec->gain_time_constant = lpf_spec.gain_time_constant;
    pd_lpf_spec->decay_time_constant = lpf_spec.decay_time_constant;
    pd_lpf_spec->time_constant = lpf_spec.time_constant;
    pd_lpf_spec->output_scale_down_factor = lpf_spec.output_scale_down_factor;
    pd_lpf_spec->lpf_type = (p4_pd_lpf_type_t) lpf_spec.lpf_type;
  }

  void lpf_spec_pd_to_thrift(${api_prefix}lpf_spec_t &lpf_spec,
                             const p4_pd_lpf_spec *pd_lpf_spec) {
    lpf_spec.gain_decay_separate_time_constant =
      pd_lpf_spec->gain_decay_separate_time_constant;
    lpf_spec.gain_time_constant = pd_lpf_spec->gain_time_constant;
    lpf_spec.decay_time_constant = pd_lpf_spec->decay_time_constant;
    lpf_spec.time_constant = pd_lpf_spec->time_constant;
    lpf_spec.output_scale_down_factor = pd_lpf_spec->output_scale_down_factor;
    lpf_spec.lpf_type = pd_lpf_spec->lpf_type == PD_LPF_TYPE_RATE ? ${api_prefix}lpf_type::TYPE_RATE : ${api_prefix}lpf_type::TYPE_SAMPLE;
  }

  void wred_spec_thrift_to_pd(const ${api_prefix}wred_spec_t &wred_spec,
                              p4_pd_wred_spec_t *pd_wred_spec) {
    pd_wred_spec->time_constant = wred_spec.time_constant;
    pd_wred_spec->red_min_threshold = wred_spec.red_min_threshold;
    pd_wred_spec->red_max_threshold = wred_spec.red_max_threshold;
    pd_wred_spec->max_probability = wred_spec.max_probability;
  }

  void wred_spec_pd_to_thrift(${api_prefix}wred_spec_t &wred_spec,
                              p4_pd_wred_spec_t *pd_wred_spec) {
    wred_spec.time_constant = pd_wred_spec->time_constant;
    wred_spec.red_min_threshold = pd_wred_spec->red_min_threshold;
    wred_spec.red_max_threshold = pd_wred_spec->red_max_threshold;
    wred_spec.max_probability = pd_wred_spec->max_probability;
  }

//:: for register, r_info in register_info.items():
//::   width = r_info["width"]
//::   param_type = r_info["v_thrift_type_imp"]
  ${r_info["v_type"]} register_${register}_spec_thrift_to_pd(const ${param_type} &thrift_spec) {
    ${r_info["v_type"]} pd_spec;
//::   if not r_info["layout"]:
    pd_spec = static_cast<decltype(pd_spec)>(thrift_spec);
//::   else:
//::     for f_name, _ in r_info["layout"]:
    pd_spec.${f_name} = static_cast<decltype(pd_spec.${f_name})>(thrift_spec.${f_name});
//::     #endfor
//::   #endif
    return pd_spec;
  }

  ${param_type} register_${register}_spec_pd_to_thrift(const ${r_info["v_type"]} &pd_spec) {
    ${param_type} thrift_spec;
//::   if not r_info["layout"]:
    thrift_spec = static_cast<decltype(thrift_spec)>(pd_spec);
//::   else:
//::     for f_name, _ in r_info["layout"]:
    thrift_spec.${f_name} = static_cast<decltype(thrift_spec.${f_name})>(pd_spec.${f_name});
//::     #endfor
//::   #endif
    return thrift_spec;
  }

//:: #endfor


//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   imp_type = api_prefix + type_ + "_meter_spec_t"
//::   pd_type = "p4_pd_" + type_ + "_meter_spec_t"
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["const EntryHandle_t entry"]
//::     pd_params += ["entry"]
//::   else:
//::     params += ["const int32_t index"]
//::     pd_params += ["index"]
//::   #endif
//::   read_params = [imp_type + " &meter_spec"] + params
//::   read_pd_params = pd_params + ["&pd_meter_spec"]
//::   param_str = ", ".join(read_params)
//::   pd_param_str = ", ".join(read_pd_params)
//::   name = "meter_read_" + meter
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    ${pd_type} pd_meter_spec = {0};
    int status = ${pd_name}(${pd_param_str});
    if(status != 0) {
        InvalidMeterOperation iop;
        iop.code = status;
        throw iop;
    }
    ${type_}_meter_spec_pd_to_thrift(meter_spec, &pd_meter_spec);
  }

//::   write_params = params + ["const " + imp_type + " &meter_spec"]
//::   pd_params += ["&pd_meter_spec"]
//::
//::   param_str = ", ".join(write_params)
//::   name = "meter_set_" + meter
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
//::   if type_ == "packets":
    p4_pd_packets_meter_spec_t pd_meter_spec;
    packets_meter_spec_thrift_to_pd(meter_spec, &pd_meter_spec);
//::   else:
    p4_pd_bytes_meter_spec_t pd_meter_spec;
    bytes_meter_spec_thrift_to_pd(meter_spec, &pd_meter_spec);
//::   #endif
    int status = ${pd_name}(${", ".join(pd_params)});
    if(status != 0) {
      InvalidMeterOperation iop;
      iop.code = status;
      throw iop;
    }
  }

//::   name = "meter_bytecount_adjust_set_" + meter
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const int32_t bytecount"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt", "bytecount"]
//::   param_str = ", ".join(params)
//::   pd_param_str = ", ".join(pd_params)
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    int status = ${pd_name}(${", ".join(pd_params)});
    if(status != 0) {
      InvalidMeterOperation iop;
      iop.code = status;
      throw iop;
    }
  }

//::   name = "meter_bytecount_adjust_get_" + meter
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt", "bytecount"]
//::   param_str = ", ".join(params)
//::   pd_param_str = ", ".join(pd_params)
  int32_t ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    int32_t bytecount;
    int status = ${pd_name}(sess_hdl, pd_dev_tgt, &bytecount);
    if(status != 0) {
      InvalidMeterOperation iop;
      iop.code = status;
      throw iop;
    }
    return bytecount;
  }

//:: #endfor


//:: for lpf, l_info in lpf_info.items():
//::   binding = l_info["binding"]
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["const EntryHandle_t entry"]
//::     pd_params += ["entry"]
//::   else:
//::     params += ["const int32_t index"]
//::     pd_params += ["index"]
//::   #endif
//::   read_params = [api_prefix + "lpf_spec_t &lpf_spec"] + params
//::   read_pd_params = pd_params + ["&pd_lpf_spec"]
//::   param_str = ", ".join(read_params)
//::   pd_param_str = ", ".join(read_pd_params)
//::   name = "lpf_read_" + lpf
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    p4_pd_lpf_spec pd_lpf_spec = {0};
    int status = ${pd_name}(${pd_param_str});
    if(status != 0) {
        InvalidLPFOperation iop;
        iop.code = status;
        throw iop;
    }
    lpf_spec_pd_to_thrift(lpf_spec, &pd_lpf_spec);
  }

//::   write_params = params + ["const " + api_prefix + "lpf_spec_t &lpf_spec"]
//::   write_pd_params = pd_params + ["&pd_lpf_spec"]
//::   param_str = ", ".join(write_params)
//::   pd_param_str = ", ".join(write_pd_params)
//::   name = "lpf_set_" + lpf
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    p4_pd_lpf_spec pd_lpf_spec;

    lpf_spec_thrift_to_pd(lpf_spec, &pd_lpf_spec);
    int status = ${pd_name}(${pd_param_str});
    if(status != 0) {
      InvalidLPFOperation iop;
      iop.code = status;
      throw iop;
    }
  }
//:: #endfor

//:: for wred, w_info in wred_info.items():
//::   binding = w_info["binding"]
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["const EntryHandle_t entry"]
//::     pd_params += ["entry"]
//::   else:
//::     params += ["const int32_t index"]
//::     pd_params += ["index"]
//::   #endif
//::   read_params = [api_prefix + "wred_spec_t &wred_spec"] + params
//::   read_pd_params = pd_params + ["&pd_wred_spec"]
//::   param_str = ", ".join(read_params)
//::   pd_param_str = ", ".join(read_pd_params)
//::   name = "wred_read_" + wred
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

    p4_pd_wred_spec_t pd_wred_spec = {0};
    int status = ${pd_name}(${pd_param_str});
    if(status != 0) {
        InvalidWREDOperation iop;
        iop.code = status;
        throw iop;
    }
    wred_spec_pd_to_thrift(wred_spec, &pd_wred_spec);
  }

//::   write_params = params + ["const " + api_prefix + "wred_spec_t &wred_spec"]
//::   write_pd_params = pd_params + ["&pd_wred_spec"]
//::   param_str = ", ".join(write_params)
//::   pd_param_str = ", ".join(write_pd_params)
//::   name = "wred_set_" + wred
//::   pd_name = pd_prefix + name
  void ${name}(${param_str}) {
    p4_pd_dev_target_t pd_dev_tgt;
    pd_dev_tgt.device_id = dev_tgt.dev_id;
    pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
    p4_pd_wred_spec_t pd_wred_spec;

    wred_spec_thrift_to_pd(wred_spec, &pd_wred_spec);
    int status = ${pd_name}(${pd_param_str});
    if(status != 0) {
      InvalidWREDOperation iop;
      iop.code = status;
      throw iop;
    }
  }
//:: #endfor

//:: if gen_perf_test_pd == 1:

void threads_init() {
    ${p4_pd_prefix}_threads_init();
}

void threads_start() {
    ${p4_pd_prefix}_threads_start();
}
void threads_stop() {
    ${p4_pd_prefix}_threads_stop();
}
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     params += ["int num_entries"]
//::     param_str = ", ".join(params)
//::     name = table + "_table_bulk_init"
//::     pd_name = pd_prefix + name
    void ${name}(${param_str}) {
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

//::     pd_params = ["sess_hdl", "pd_dev_tgt", "num_entries"]
        int status = ${pd_name}(${", ".join(pd_params)});
        if (status != 0) {
            InvalidTableOperation iop;
            iop.code = status;
            throw iop;
        }
    }
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if action_table_hdl: continue
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     for action in t_info["actions"]:
//::       a_info = action_info[action]
//::       has_action_spec = len(a_info["param_names"]) > 0
//::       params = ["const SessionHandle_t sess_hdl",
//::                 "const DevTarget_t &dev_tgt"]
//::       if has_match_spec:
//::         params += ["const " + api_prefix + table + "_match_spec_t &match_spec"]
//::       #endif
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["const int32_t priority"]
//::       #endif
//::       if has_action_spec:
//::         params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::       #endif
//::       if t_info["timeout"]:
//::         params += ["const int32_t ttl"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::       param_str = ", ".join(params)
//::       name = table + "_table_add_with_" + action + "_bulk_setup"
//::       pd_name = pd_prefix + name
    void ${name}(${param_str}) {

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);

//::       if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::         match_params = gen_match_params(t_info["match_fields"], field_info)
//::         for name, width in match_params:
//::           if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::           else:
        memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::           #endif
//::         #endfor

//::       #endif
//::       if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::         action_params = gen_action_params(a_info["param_names"],
//::                                           a_info["param_byte_widths"])
//::         for name, width in action_params:
//::           if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::           else:
        memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::           #endif
//::         #endfor

//::       #endif

//::       pd_params = ["sess_hdl", "pd_dev_tgt"]
//::       if has_match_spec:
//::         pd_params += ["&pd_match_spec"]
//::       #endif
//::       if match_type == "ternary" or match_type == "range":
//::         pd_params += ["priority"]
//::       #endif
//::       if has_action_spec:
//::         pd_params += ["&pd_action_spec"]
//::       #endif
//::       if t_info["timeout"]:
//::        pd_params += ["(uint32_t) ttl"]
//::       #endif
//::       # direct parameter specs
//::       for res_name, res_type, _ in table_direct_resources[table]:
//::         if res_type == "bytes_meter" or res_type == "packets_meter" or res_type == "lpf" or res_type == "wred":
        p4_pd_${res_type}_spec_t pd_${res_name}_spec;
        ${res_type}_spec_thrift_to_pd(${res_name}_spec, &pd_${res_name}_spec);
//::           pd_params += ["&pd_" + res_name + "_spec"]
//::         #endif
//::         if res_type == "register":
        auto pd_${res_name}_spec = register_${res_name}_spec_thrift_to_pd(${res_name}_spec);
//::           pd_params += ["&pd_" + res_name + "_spec"]
//::         #endif
//::       #endfor
//::       pd_param_str = ", ".join(pd_params)
        int status = ${pd_name}(${pd_param_str});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     #endfor
//::   #endfor
//
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     name = table + "_churn_thread_spawn"
//::     pd_name = p4_pd_prefix + table + "_churn_thread_spawn"
    void ${name}() {
        ${pd_name}();
    }
//::     for res_name, res_type, res_hdl in table_direct_resources[table]:
//::       if res_type == "counter":
//::         name = table + "_stats_ent_dump_thread_spawn"
//::         pd_name = p4_pd_prefix + table + "_stats_ent_dump_thread_spawn"
//::         params = ["const SessionHandle_t sess_hdl",
//::                   "const DevTarget_t &dev_tgt",
//::                   "const std::vector<EntryHandle_t> &entry_hdls"]
//::         param_str = ", ".join(params)
    void ${name}(${param_str}) {
        int num_entry_hdls = entry_hdls.size();
        p4_pd_entry_hdl_t *hdls = (p4_pd_entry_hdl_t *) calloc(num_entry_hdls, sizeof(EntryHandle_t));
        assert (hdls != NULL);
        auto iter = 0;
        for (auto entry_hdl : entry_hdls) {
            hdls[iter++] = entry_hdl;
        }
        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = i16_to_bf_pipe(dev_tgt.dev_pipe_id);
        ${pd_name}(sess_hdl, pd_dev_tgt, hdls, num_entry_hdls);
        free(hdls);
    }
//::       #endif
//::     #endfor
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     name = table + "_table_perf_test_execute"
//::     pd_name = pd_prefix + name
    int ${name}() {
        int rate = ${pd_name}();
        return rate;
    }
//::   #endfor
//:: #endif

//:: if parser_value_set["ingress"] or parser_value_set["egress"]:
//::   pvs_names = []
//::   for pvs in parser_value_set["ingress"]:
//::     pvs_names.append(pvs["pvs_name"])
//::   #endfor
//::   for pvs in parser_value_set["egress"]:
//::     if pvs["pvs_name"] not in pvs_names:
//::       pvs_names.append(pvs["pvs_name"])
//::     #endif
//::   #endfor
//::   for pvs_name in pvs_names:

//::     thrift_api_name = "pvs_" + pvs_name + "_set_property"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_set_property"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const int8_t dev_id, const pvs_property_t::type property, const pvs_property_value_t::type value, const pvs_gress_t::type gress"
    void ${thrift_api_name}(${thrift_api_params}) {
        int32_t handle = 0;
        p4_pd_pvs_prop_type_t pvs_property = (p4_pd_pvs_prop_type_t)property;
        p4_pd_pvs_prop_value_t property_value;
        p4_pd_pvs_prop_args_t property_args;
        property_value.value = (uint32_t)value;
        property_args.value = (uint32_t)gress;
//::     pd_api_params = "sess_hdl, dev_id, pvs_property, property_value, property_args"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_get_property"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_get_property"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const int8_t dev_id, const pvs_property_t::type property, const pvs_gress_t::type gress"
    pvs_property_value_t::type ${thrift_api_name}(${thrift_api_params}) {
        int32_t handle = 0;
        p4_pd_pvs_prop_type_t pvs_property = (p4_pd_pvs_prop_type_t)property;
        p4_pd_pvs_prop_value_t property_value;
        p4_pd_pvs_prop_args_t property_args;
        property_args.value = (uint32_t)gress;
        int status = ${pd_api_name}(sess_hdl, dev_id, pvs_property, &property_value, property_args);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }

        switch (pvs_property)
        {
          case PD_PVS_GRESS_SCOPE:
          {
            if (property_value.value == PD_PVS_SCOPE_ALL_GRESS) {
              return pvs_property_value_t::PVS_SCOPE_ALL_GRESS;
            }
            else {
              return pvs_property_value_t::PVS_SCOPE_SINGLE_GRESS;
            }
          }
          case PD_PVS_PIPE_SCOPE:
          {
            if (property_value.value == PD_PVS_SCOPE_ALL_PIPELINES) {
              return pvs_property_value_t::PVS_SCOPE_ALL_PIPELINES;
            }
            else {
              return pvs_property_value_t::PVS_SCOPE_SINGLE_PIPELINE;
            }
          }
          case PD_PVS_PARSER_SCOPE:
          {
            if (property_value.value == PD_PVS_SCOPE_ALL_PARSERS_IN_PIPE) {
              return pvs_property_value_t::PVS_SCOPE_ALL_PARSERS;
            }
            else {
              return pvs_property_value_t::PVS_SCOPE_SINGLE_PARSER;
            }
          }
          default:
            InvalidTableOperation iop;
            iop.code = PIPE_INVALID_ARG;
            throw iop;
        }
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_add"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_add"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const DevParserTarget_t &dev_prsr_tgt, " + "const int32_t parser_value, " + "const int32_t parser_value_mask"
    PvsHandle_t ${thrift_api_name}(${thrift_api_params}) {
        p4_pd_dev_parser_target_t pd_dev_prsr_tgt;
        pd_dev_prsr_tgt.device_id = dev_prsr_tgt.dev_id;
        pd_dev_prsr_tgt.dev_pipe_id = i16_to_bf_pipe(dev_prsr_tgt.dev_pipe_id);
        pd_dev_prsr_tgt.parser_id = dev_prsr_tgt.parser_id;
        pd_dev_prsr_tgt.gress_id = dev_prsr_tgt.gress_id == 0 ? PD_PVS_GRESS_INGRESS :
                                   dev_prsr_tgt.gress_id == 1 ? PD_PVS_GRESS_EGRESS :
                                   dev_prsr_tgt.gress_id == -1 ? PD_PVS_GRESS_ALL :
                                   static_cast<p4_pd_pvs_gress_en>(dev_prsr_tgt.gress_id);
        int32_t handle = 0;
//::     pd_api_params = "sess_hdl, pd_dev_prsr_tgt, " + "parser_value, " + "parser_value_mask, " + "(p4_pd_pvs_hdl_t*)&handle"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return (handle);
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_modify"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_modify"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const int8_t dev_id, " + "const PvsHandle_t pvs_entry_handle, " + "const int32_t parser_value, " + "const int32_t parser_value_mask"
    void ${thrift_api_name}(${thrift_api_params}) {
//::     pd_api_params = "sess_hdl, dev_id, " + "pvs_entry_handle, " + "parser_value, " + "parser_value_mask"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_delete"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_delete"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const int8_t dev_id, " + "PvsHandle_t pvs_entry_handle"
//::
    void ${thrift_api_name}(${thrift_api_params}) {
//::     pd_api_params = "sess_hdl, dev_id, " + "pvs_entry_handle"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_get"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_get"
//::     thrift_api_params = "PVSSpec_t &return_pvs_spec, const SessionHandle_t sess_hdl, const int8_t dev_id, " + "PvsHandle_t pvs_entry_handle"
//::
    void ${thrift_api_name}(${thrift_api_params}) {
        uint32_t parser_value;
        uint32_t parser_value_mask;
        int status = ${pd_api_name}(sess_hdl, dev_id, pvs_entry_handle, &parser_value, &parser_value_mask);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }

        return_pvs_spec.parser_value = parser_value;
        return_pvs_spec.parser_value_mask = parser_value_mask;
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_handle_get"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_handle_get"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const DevParserTarget_t &dev_prsr_tgt, " + "const int32_t parser_value, " + "const int32_t parser_value_mask"
    PvsHandle_t ${thrift_api_name}(${thrift_api_params}) {
        p4_pd_dev_parser_target_t pd_dev_prsr_tgt;
        pd_dev_prsr_tgt.device_id = dev_prsr_tgt.dev_id;
        pd_dev_prsr_tgt.dev_pipe_id = i16_to_bf_pipe(dev_prsr_tgt.dev_pipe_id);
        pd_dev_prsr_tgt.parser_id = dev_prsr_tgt.parser_id;
        pd_dev_prsr_tgt.gress_id = dev_prsr_tgt.gress_id == 0 ? PD_PVS_GRESS_INGRESS :
                                   dev_prsr_tgt.gress_id == 1 ? PD_PVS_GRESS_EGRESS :
                                   dev_prsr_tgt.gress_id == -1 ? PD_PVS_GRESS_ALL :
                                   static_cast<p4_pd_pvs_gress_en>(dev_prsr_tgt.gress_id);
        int32_t handle = 0;
//::     pd_api_params = "sess_hdl, pd_dev_prsr_tgt, " + "parser_value, " + "parser_value_mask, " + "(p4_pd_pvs_hdl_t*)&handle"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return handle;
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_get_first_entry_handle"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_get_first_entry_handle"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const DevParserTarget_t &dev_prsr_tgt"
    PvsHandle_t ${thrift_api_name}(${thrift_api_params}) {
        p4_pd_dev_parser_target_t pd_dev_prsr_tgt;
        pd_dev_prsr_tgt.device_id = dev_prsr_tgt.dev_id;
        pd_dev_prsr_tgt.dev_pipe_id = i16_to_bf_pipe(dev_prsr_tgt.dev_pipe_id);
        pd_dev_prsr_tgt.parser_id = dev_prsr_tgt.parser_id;
        pd_dev_prsr_tgt.gress_id = dev_prsr_tgt.gress_id == 0 ? PD_PVS_GRESS_INGRESS :
                                   dev_prsr_tgt.gress_id == 1 ? PD_PVS_GRESS_EGRESS :
                                   dev_prsr_tgt.gress_id == -1 ? PD_PVS_GRESS_ALL :
                                   static_cast<p4_pd_pvs_gress_en>(dev_prsr_tgt.gress_id);
        int32_t handle = 0;
//::     pd_api_params = "sess_hdl, pd_dev_prsr_tgt, (p4_pd_pvs_hdl_t*)&handle"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return handle;
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_get_next_entry_handles"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_get_next_entry_handles"
//::     thrift_api_params = "std::vector<int> &rv, const SessionHandle_t sess_hdl, const DevParserTarget_t &dev_prsr_tgt, const PvsHandle_t entry_handle, const int32_t n"
    void ${thrift_api_name}(${thrift_api_params}) {
        p4_pd_dev_parser_target_t pd_dev_prsr_tgt;
        pd_dev_prsr_tgt.device_id = dev_prsr_tgt.dev_id;
        pd_dev_prsr_tgt.dev_pipe_id = i16_to_bf_pipe(dev_prsr_tgt.dev_pipe_id);
        pd_dev_prsr_tgt.parser_id = dev_prsr_tgt.parser_id;
        pd_dev_prsr_tgt.gress_id = dev_prsr_tgt.gress_id == 0 ? PD_PVS_GRESS_INGRESS :
                                   dev_prsr_tgt.gress_id == 1 ? PD_PVS_GRESS_EGRESS :
                                   dev_prsr_tgt.gress_id == -1 ? PD_PVS_GRESS_ALL :
                                   static_cast<p4_pd_pvs_gress_en>(dev_prsr_tgt.gress_id);
        std::vector<unsigned int> next_entry_handles(n);
//::     pd_api_params = "sess_hdl, pd_dev_prsr_tgt, entry_handle, n, next_entry_handles.data()"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        for (auto &i : next_entry_handles) rv.push_back(i);
    }

//::     thrift_api_name = "pvs_" + pvs_name + "_entry_get_count"
//::     pd_api_name = p4_pd_prefix + pvs_name + "_entry_get_count"
//::     thrift_api_params = "const SessionHandle_t sess_hdl, const DevParserTarget_t &dev_prsr_tgt, const bool read_from_hw"
    int32_t ${thrift_api_name}(${thrift_api_params}) {
        p4_pd_dev_parser_target_t pd_dev_prsr_tgt;
        pd_dev_prsr_tgt.device_id = dev_prsr_tgt.dev_id;
        pd_dev_prsr_tgt.dev_pipe_id = i16_to_bf_pipe(dev_prsr_tgt.dev_pipe_id);
        pd_dev_prsr_tgt.parser_id = dev_prsr_tgt.parser_id;
        pd_dev_prsr_tgt.gress_id = dev_prsr_tgt.gress_id == 0 ? PD_PVS_GRESS_INGRESS :
                                   dev_prsr_tgt.gress_id == 1 ? PD_PVS_GRESS_EGRESS :
                                   dev_prsr_tgt.gress_id == -1 ? PD_PVS_GRESS_ALL :
                                   static_cast<p4_pd_pvs_gress_en>(dev_prsr_tgt.gress_id);
        uint32_t c = 0;
//::     pd_api_params = "sess_hdl, pd_dev_prsr_tgt, read_from_hw, &c"
        int status = ${pd_api_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return c;
    }

//::   #endfor
//:: #endif

//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const " + api_prefix + hash_calc_name + "_input_t::type input"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_set"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    void ${fn_name}(${param_str}) {
//::   pd_input_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_t"
        ${pd_input_type} pd_input = (${pd_input_type})input;
        int status = ${pd_name}(${pd_api_params}, pd_input);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   ret_type = api_prefix + hash_calc_name + "_input_t::type"
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_get"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    int32_t ${fn_name}(${param_str}) {
//::   pd_input_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_t"
        ${pd_input_type} pd_input;
        int status = ${pd_name}(${pd_api_params}, &pd_input);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return ((${ret_type})pd_input);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const " + api_prefix + hash_calc_name + "_algo_t::type algo"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_algorithm_set"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    void ${fn_name}(${param_str}) {
//::   pd_algo_type = "p4_pd_" + api_prefix + hash_calc_name + "_algo_t"
        ${pd_algo_type} pd_algo = (${pd_algo_type})algo;
        int status = ${pd_name}(${pd_api_params}, pd_algo);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   ret_type = api_prefix + hash_calc_name + "_algo_t::type"
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_algorithm_get"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    int32_t ${fn_name}(${param_str}) {
//::   pd_algo_type = "p4_pd_" + api_prefix + hash_calc_name + "_algo_t"
        ${pd_algo_type} pd_algo;
        int status = ${pd_name}(${pd_api_params}, &pd_algo);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return ((${ret_type})pd_algo);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const int64_t seed"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_seed_set"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id, seed"
    void ${fn_name}(${param_str}) {
        int status = ${pd_name}(${pd_api_params});
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_seed_get"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    int64_t ${fn_name}(${param_str}) {
        uint64_t seed = 0;
        int status = ${pd_name}(${pd_api_params}, &seed);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return ((int64_t)seed);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const " + api_prefix + hash_calc_name + "_input_t::type input"]
//::   params += ["const std::vector<" + api_prefix + hash_calc_name + "_input_field_attribute_t> &array_of_attrs"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_set"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    void ${fn_name}(${param_str}) {
//::   pd_input_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_t"
//::   pd_attr_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_field_attribute_t"
//::   pd_field_attr_type = "p4_pd_" + api_prefix + "input_field_attr_type_t"
        ${pd_input_type} pd_input = (${pd_input_type})input;
        ${pd_attr_type} *pd_attr = new ${pd_attr_type}[array_of_attrs.size()]();
        int i = 0;
        /* Copy over the info from the thrift struct to the PD struct */
        for (auto & item : array_of_attrs) {
          pd_attr[i].input_field.id = item.input_field;
          pd_attr[i].type = (${pd_field_attr_type})item.type;
          pd_attr[i].value.val = item.value;
          i++;
        }
        int status = ${pd_name}(${pd_api_params}, pd_input, i, pd_attr);
        delete[] pd_attr;
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const " + api_prefix + hash_calc_name + "_input_t::type input"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_count_get"
//::   pd_name = pd_prefix + fn_name
//::   pd_api_params = "sess_hdl, dev_id"
    int32_t ${fn_name}(${param_str}) {
//::   pd_input_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_t"
        ${pd_input_type} pd_input = (${pd_input_type})input;
        uint32_t attr_count = 0;
        int status = ${pd_name}(${pd_api_params}, pd_input, &attr_count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return attr_count;
    }

//::   params = ["std::vector<" + api_prefix + hash_calc_name + "_input_field_attribute_t> &array_of_attrs"]
//::   params += ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const " + api_prefix + hash_calc_name + "_input_t::type input"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_get"
//::   pd_name = pd_prefix + fn_name
//::   pd_count_name = pd_prefix + "hash_calc_" + hash_calc_name + "_input_field_attribute_count_get"
//::   pd_api_params = "sess_hdl, dev_id"
    void ${fn_name}(${param_str}) {
//::   pd_input_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_t"
//::   pd_attr_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_field_attribute_t"
//::   attr_type = api_prefix + hash_calc_name + "_input_field_attribute_t"
//::   field_attr_type = api_prefix + "input_field_attr_type_t::type"
        ${pd_input_type} pd_input = (${pd_input_type})input;
        uint32_t attr_count = 0, filled_attr_count = 0;
        int status = ${pd_count_name}(${pd_api_params}, pd_input, &attr_count);
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        ${pd_attr_type} *pd_array_of_attrs = new ${pd_attr_type}[attr_count]();
        status = ${pd_name}(${pd_api_params}, pd_input, attr_count,
                pd_array_of_attrs, &filled_attr_count);
        if(status != 0) {
          delete[] pd_array_of_attrs;
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        /* Copy over the info from the PD struct to the thrift struct */
        array_of_attrs = std::vector<${attr_type}>(filled_attr_count);
        for (int i=0; i < filled_attr_count; i++) {
          array_of_attrs[i].input_field = pd_array_of_attrs[i].input_field.id;
          array_of_attrs[i].type = (${field_attr_type})pd_array_of_attrs[i].type;
          array_of_attrs[i].value = pd_array_of_attrs[i].value.val;
        }
        delete[] pd_array_of_attrs;
    }

//::   params = ["std::vector<int8_t> &hash_val",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id"]
//::   params += ["const int32_t attr_count"]
//::   params += ["const std::vector<" + api_prefix + hash_calc_name + "_input_field_attribute_t> &array_of_attrs"]
//::   params += ["const std::vector<int32_t> &attr_sizes"]
//::   param_str = ", ".join(params)
//::   fn_name = "hash_calc_" + hash_calc_name + "_calculate_hash_value"
//::   pd_name = pd_prefix + fn_name
    void ${fn_name}(${param_str}) {
//::   pd_attr_type = "p4_pd_" + api_prefix + hash_calc_name + "_input_field_attribute_t"
        ${pd_attr_type} *pd_attr = new ${pd_attr_type}[attr_count]();
        uint32_t *pd_attr_sizes = new uint32_t[attr_count]();
        hash_val = std::vector<int8_t>(${dyn_hash_calc_info['width']});
        int64_t result = 0;
        int i = 0;
        /* Copy over the info from the thrift struct to the PD struct */
        for (auto & item : array_of_attrs) {
          pd_attr[i].input_field.id = item.input_field;
          if (attr_sizes[i] > 32) {
            pd_attr[i].value.stream = (uint8_t *)malloc((attr_sizes[i] + 7) / 8);
            memcpy(pd_attr[i].value.stream, item.stream.c_str(), (attr_sizes[i] + 7) / 8);
          } else {
            pd_attr[i].value.val = item.value;
          }
          pd_attr_sizes[i] = (uint32_t)attr_sizes[i];
          i++;
        }
        int status = ${pd_name}(sess_hdl, dev_id, attr_count, pd_attr, pd_attr_sizes, (uint8_t *)hash_val.data(), ${dyn_hash_calc_info['width']});
        delete[] pd_attr;
        delete[] pd_attr_sizes;
        if(status != 0) {
          InvalidTableOperation iop;
          iop.code = status;
          throw iop;
        }
        return;
    }

//:: #endfor

private:
    static inline bf_dev_pipe_t i16_to_bf_pipe(int16_t pipe) {
      return (pipe == -1) ? BF_DEV_PIPE_ALL : (bf_dev_pipe_t)pipe;
    }
};
//:: if gen_md_pd:
std::map<int, std::vector<${api_prefix}tbl_update_t> > ${p4_prefix}Handler::${queue_name};
//:: #endif
