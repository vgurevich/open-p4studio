/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

//:: # BFN Thrift RPC Input Template
# BFN Thrift RPC Input

include "res.thrift"

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
//::

namespace py p4_pd_rpc
namespace cpp p4_pd_rpc
namespace c_glib p4_pd_rpc

typedef i32 EntryHandle_t
typedef i32 MemberHandle_t
typedef i32 GroupHandle_t
typedef binary MacAddr_t
typedef binary IPv6_t
typedef i32 SnapshotHandle_t
typedef i32 PvsHandle_t

struct ${api_prefix}counter_value_t {
  1: required i64 packets;
  2: required i64 bytes;
}

struct ${api_prefix}packets_meter_spec_t {
  1: required i64 cir_pps;
  2: required i64 cburst_pkts;
  3: required i64 pir_pps;
  4: required i64 pburst_pkts;
  5: required bool color_aware;
  6: optional bool is_set = 1;
}

struct ${api_prefix}bytes_meter_spec_t {
  1: required i64 cir_kbps;
  2: required i64 cburst_kbits;
  3: required i64 pir_kbps;
  4: required i64 pburst_kbits;
  5: required bool color_aware;
  6: optional bool is_set = 1;
}

enum ${api_prefix}lpf_type {
  TYPE_RATE = 0,
  TYPE_SAMPLE = 1
}

struct ${api_prefix}lpf_spec_t {
  1: required bool gain_decay_separate_time_constant;
  2: required double gain_time_constant;
  3: required double decay_time_constant;
  4: required double time_constant;
  5: required i32 output_scale_down_factor;
  6: required ${api_prefix}lpf_type lpf_type;
  7: optional bool is_set = 1;
}

struct ${api_prefix}wred_spec_t {
  1: required double time_constant;
  2: required i32 red_min_threshold;
  3: required i32 red_max_threshold;
  4: required double max_probability;
  5: optional bool is_set = 1;
}


enum ${api_prefix}idle_time_mode {
  POLL_MODE = 0,
  NOTIFY_MODE = 1,
  INVALID_MODE = 2
}

enum ${api_prefix}idle_time_hit_state {
  ENTRY_IDLE = 0,
  ENTRY_ACTIVE = 1
}

struct ${api_prefix}idle_time_params_t {
  1: required ${api_prefix}idle_time_mode mode;
  2: optional i32 ttl_query_interval;
  3: optional i32 max_ttl;
  4: optional i32 min_ttl;
  5: optional i32 cookie;
}

struct ${api_prefix}idle_tmo_expired_t {
  1: required i32 dev_id;
  2: required EntryHandle_t entry;
  3: required i32 cookie;
}

struct ${api_prefix}sel_update_t {
  1: required res.SessionHandle_t  sess_hdl;
  2: required res.DevTarget_t      dev_tgt;
  3: required i32                  cookie;
  4: required i32                  grp_hdl;
  5: required i32                  mbr_hdl;
  6: required i32                  index;
  7: required bool                 is_add;
}

enum ${api_prefix}grp_mbr_state {
  MBR_ACTIVE = 0,
  MBR_INACTIVE = 1
}


//:: if gen_md_pd:
enum ${api_prefix}mat_update_type {
  MAT_UPDATE_ADD = 0,
  MAT_UPDATE_ADD_MULTI = 1,
  MAT_UPDATE_SET_DFLT = 2,
  MAT_UPDATE_CLR_DFLT = 3,
  MAT_UPDATE_DEL = 4,
  MAT_UPDATE_MOD = 5,
  MAT_UPDATE_MOV = 6,
  MAT_UPDATE_MOV_MULTI = 7
}

struct ${api_prefix}multi_index {
  1: required i32  base_index;
  2: required i32  index_count;
}
struct ${api_prefix}mat_update_set_dflt_params {
  1: required i32    entry_hdl;
  2: required i32    action_profile_mbr;
  3: required i32    action_index;
  4: required bool   action_profile_mbr_exists;
  5: required i32    sel_grp_hdl;
  6: required i32    selection_index;
  7: required i32    num_selector_indices;
  8: required bool   sel_grp_exists;
  9: required binary drv_data;
}
struct ${api_prefix}mat_update_clr_dflt_params {
  1: required i32    entry_hdl;
}
struct ${api_prefix}mat_update_add_params {
  1: required i32    entry_hdl;
  2: required i32    priority;
  3: required i32    entry_index;
  4: required i32    action_profile_mbr;
  5: required i32    action_index;
  6: required bool   action_profile_mbr_exists;
  7: required i32    sel_grp_hdl;
  8: required i32    selection_index;
  9: required i32    num_selector_indices;
 10: required bool   sel_grp_exists;
 11: required binary drv_data;
}
struct ${api_prefix}mat_update_add_multi_params {
  1: required i32    entry_hdl;
  2: required i32    priority;
  3: required i32    action_profile_mbr;
  4: required i32    action_index;
  5: required bool   action_profile_mbr_exists;
  6: required i32    sel_grp_hdl;
  7: required i32    selection_index;
  8: required i32    num_selector_indices;
  9: required bool   sel_grp_exists;
 10: required list<${api_prefix}multi_index> locations;
 11: required binary drv_data;
}
struct ${api_prefix}mat_update_mov_multi_params {
  1: required i32    entry_hdl;
  2: required i32    action_profile_mbr;
  3: required i32    action_index;
  4: required bool   action_profile_mbr_exists;
  5: required i32    sel_grp_hdl;
  6: required i32    selection_index;
  7: required i32    num_selector_indices;
  8: required bool   sel_grp_exists;
  9: required list<${api_prefix}multi_index> locations;
 10: required binary drv_data;
}
struct ${api_prefix}mat_update_del_params {
  1: required i32    entry_hdl;
}
struct ${api_prefix}mat_update_mod_params {
  1: required i32    entry_hdl;
  2: required i32    action_profile_mbr;
  3: required i32    action_index;
  4: required bool   action_profile_mbr_exists;
  5: required i32    sel_grp_hdl;
  6: required i32    selection_index;
  7: required i32    num_selector_indices;
  8: required bool   sel_grp_exists;
  9: required binary drv_data;
}
struct ${api_prefix}mat_update_mov_params {
  1: required i32    entry_hdl;
  2: required i32    entry_index;
  3: required i32    action_profile_mbr;
  4: required i32    action_index;
  5: required bool   action_profile_mbr_exists;
  6: required i32    sel_grp_hdl;
  7: required i32    selection_index;
  8: required i32    num_selector_indices;
  9: required bool   sel_grp_exists;
 10: required binary drv_data;
}

union ${api_prefix}mat_update_params {
  1: ${api_prefix}mat_update_set_dflt_params set_dflt
  2: ${api_prefix}mat_update_clr_dflt_params clr_dflt
  3: ${api_prefix}mat_update_add_params      add
  4: ${api_prefix}mat_update_del_params      remove
  5: ${api_prefix}mat_update_mod_params      mod
  6: ${api_prefix}mat_update_mov_params      mov
  7: ${api_prefix}mat_update_add_multi_params add_multi
  8: ${api_prefix}mat_update_mov_multi_params mov_multi
}

struct ${api_prefix}mat_tbl_update_t {
  1: required res.DevTarget_t              dev_tgt;
  2: required i32                          tbl_hdl;
  3: required ${api_prefix}mat_update_type update_type;
  4: required ${api_prefix}mat_update_params update_params;
}

enum ${api_prefix}adt_update_type {
  ADT_UPDATE_ADD = 0,
  ADT_UPDATE_DEL = 1,
  ADT_UPDATE_MOD = 2
}
struct ${api_prefix}adt_update_add_params {
  1: required i32 entry_hdl;
  2: required binary drv_data;
}
struct ${api_prefix}adt_update_del_params {
  1: required i32 entry_hdl;
}
struct ${api_prefix}adt_update_mod_params {
  1: required i32 entry_hdl;
  2: required binary drv_data;
}
union ${api_prefix}adt_update_params {
  1: ${api_prefix}adt_update_add_params      add
  2: ${api_prefix}adt_update_del_params      remove
  3: ${api_prefix}adt_update_mod_params      mod
}
struct ${api_prefix}adt_tbl_update_t {
  1: required res.DevTarget_t              dev_tgt;
  2: required i32                          tbl_hdl;
  3: required ${api_prefix}adt_update_type update_type;
  4: required ${api_prefix}adt_update_params update_params;
}

enum ${api_prefix}sel_update_type {
  SEL_UPDATE_GROUP_CREATE = 0,
  SEL_UPDATE_GROUP_DESTROY = 1,
  SEL_UPDATE_ADD = 2,
  SEL_UPDATE_DEL = 3,
  SEL_UPDATE_ACTIVATE = 4,
  SEL_UPDATE_DEACTIVATE = 5,
  SEL_UPDATE_SET_FALLBACK = 6,
  SEL_UPDATE_CLR_FALLBACK = 7
}
struct ${api_prefix}sel_update_group_create_params {
  1: required i32 group_hdl;
  2: required i32 num_indexes;
  3: required i32 max_members;
  4: required i32 base_logical_index;
  5: required list<${api_prefix}multi_index> locations;
}
struct ${api_prefix}sel_update_group_destroy_params {
  1: required i32 group_hdl;
}
struct ${api_prefix}sel_update_add_params {
  1: required i32 group_hdl;
  2: required i32 entry_hdl;
  3: required i32 entry_index;
  4: required i32 entry_subindex;
  5: required binary drv_data;
}
struct ${api_prefix}sel_update_del_params {
  1: required i32 group_hdl;
  2: required i32 entry_hdl;
  3: required i32 entry_index;
  4: required i32 entry_subindex;
}
struct ${api_prefix}sel_update_activate_params {
  1: required i32 group_hdl;
  2: required i32 entry_hdl;
  3: required i32 entry_index;
  4: required i32 entry_subindex;
}
struct ${api_prefix}sel_update_deactivate_params {
  1: required i32 group_hdl;
  2: required i32 entry_hdl;
  3: required i32 entry_index;
  4: required i32 entry_subindex;
}
struct ${api_prefix}sel_update_set_fallback_params {
  1: required i32 entry_hdl;
  2: required binary drv_data;
}
union ${api_prefix}sel_update_params {
  1: ${api_prefix}sel_update_group_create_params    grp_create
  2: ${api_prefix}sel_update_group_destroy_params   grp_destroy
  3: ${api_prefix}sel_update_add_params             add
  4: ${api_prefix}sel_update_del_params             remove
  5: ${api_prefix}sel_update_activate_params        activate
  6: ${api_prefix}sel_update_deactivate_params      deactivate
  7: ${api_prefix}sel_update_set_fallback_params    set_fallback
}
struct ${api_prefix}sel_tbl_update_t {
  1: required res.DevTarget_t              dev_tgt;
  2: required i32                          tbl_hdl;
  3: required ${api_prefix}sel_update_type update_type;
  4: required ${api_prefix}sel_update_params update_params;
}

enum ${api_prefix}tbl_update_type {
  MAT_UPDATE_TYPE = 0,
  ADT_UPDATE_TYPE = 1,
  SEL_UPDATE_TYPE = 2
}
union ${api_prefix}tbl_update_data {
  1: ${api_prefix}mat_tbl_update_t mat
  2: ${api_prefix}adt_tbl_update_t adt
  3: ${api_prefix}sel_tbl_update_t sel
}
struct ${api_prefix}tbl_update_t {
  1: required ${api_prefix}tbl_update_type update_type;
  2: required ${api_prefix}tbl_update_data update_data;
}

//:: #endif
enum tbl_property_t
{
   TBL_PROP_TBL_ENTRY_SCOPE = 1,
   TBL_PROP_TERN_TABLE_ENTRY_PLACEMENT = 2,
   TBL_PROP_DUPLICATE_ENTRY_CHECK = 3,
   TBL_PROP_IDLETIME_REPEATED_NOTIFICATION = 4
}

enum tbl_property_value_t
{
   ENTRY_SCOPE_ALL_PIPELINES=0,
   ENTRY_SCOPE_SINGLE_PIPELINE=1,
   ENTRY_SCOPE_USER_DEFINED=2,
   TERN_ENTRY_PLACEMENT_DRV_MANAGED=0,
   TERN_ENTRY_PLACEMENT_APP_MANAGED=1,
   DUPLICATE_ENTRY_CHECK_DISABLE=0,
   DUPLICATE_ENTRY_CHECK_ENABLE=1,
   IDLETIME_REPEATED_NOTIFICATION_DISABLE = 0,
   IDLETIME_REPEATED_NOTIFICATION_ENABLE = 1
}

struct tbl_property_value_args_t
{
  1: required tbl_property_value_t value;
  2: required i32                  scope_args;
}

enum pvs_gress_t
{
   PVS_GRESS_INGRESS = 0,
   PVS_GRESS_EGRESS = 1,
   PVS_GRESS_ALL = 0xff
}

enum pvs_property_t {
  PVS_PROP_NONE = 0,
  PVS_GRESS_SCOPE,
  PVS_PIPE_SCOPE,
  PVS_PARSER_SCOPE
}

enum pvs_property_value_t {
  PVS_SCOPE_ALL_GRESS = 0,
  PVS_SCOPE_SINGLE_GRESS = 1,
  PVS_SCOPE_ALL_PIPELINES = 0,
  PVS_SCOPE_SINGLE_PIPELINE = 1,
  PVS_SCOPE_ALL_PARSERS = 0,
  PVS_SCOPE_SINGLE_PARSER = 1
}

enum tbl_dbg_counter_type_t {
  TBL_DBG_CNTR_DISABLED = 0,
  TBL_DBG_CNTR_LOG_TBL_MISS,
  TBL_DBG_CNTR_LOG_TBL_HIT,
  TBL_DBG_CNTR_GW_TBL_MISS,
  TBL_DBG_CNTR_GW_TBL_HIT,
  TBL_DBG_CNTR_GW_TBL_INHIBIT,
  TBL_DBG_CNTR_MAX
}

struct PVSSpec_t {
  1: required i32 parser_value;
  2: required i32 parser_value_mask;
}

struct TblCntrInfo_t {
  1: required tbl_dbg_counter_type_t type;
  2: required i32 value;
}

struct TblDbgStageInfo_t {
  1: required i32 num_counters;
  2: required list<string> tbl_name;
  3: required list<tbl_dbg_counter_type_t> type;
  4: required list<i32> value;
}

# not very space efficient but convenient
struct ${api_prefix}counter_flags_t {
  1: required bool read_hw_sync;
}

struct ${api_prefix}register_flags_t {
  1: required bool read_hw_sync;
}

struct ${api_prefix}snapshot_trig_spec_t {
  1: required string field_name;
  2: required i64 field_value;
  3: required i64 field_mask;
}

struct ${api_prefix}snapshot_tbl_data_t {
  1: required bool hit;
  2: required bool inhibited;
  3: required bool executed;
  4: required i32 hit_entry_handle;
}

//:: field_lists = set()
//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
//::     if fl_name in field_lists:
//::       continue
//::     #endif
//::     field_lists.add(fl_name)
enum ${api_prefix}input_fields_${fl_name}_t {
//::     enum_pos = 0
//::     for f_info in fl_info['fields']:
//::       enum_name_upper = fl_name.upper() + "_" + f_info['name'].upper()
  ${enum_name_upper} = ${enum_pos},
//::       enum_pos += 1
//::     #endfor
}
//::   #endfor
//:: #endfor

enum ${api_prefix}input_field_attr_type_t {
  INPUT_FIELD_ATTR_TYPE_MASK,
  INPUT_FIELD_ATTR_TYPE_VALUE
}

enum ${api_prefix}input_field_attr_value_mask_t {
  INPUT_FIELD_EXCLUDED = 0,
  INPUT_FIELD_INCLUDED
}

//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():
enum ${api_prefix}${hash_calc_name}_input_t {
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
//::     enum_name_upper = p4_pd_prefix.upper() + hash_calc_name.upper() + "_INPUT_" + fl_name.upper()
  ${enum_name_upper},
//::   #endfor
}

union ${api_prefix}${hash_calc_name}_input_fields_union_t {
//::   index = 1
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
  ${index}: ${api_prefix}input_fields_${fl_name}_t ${fl_name}
//::     index += 1
//::   #endfor
  ${index}: i32 id
}

union ${api_prefix}${hash_calc_name}_input_field_attr_value_union_t {
  1: ${api_prefix}input_field_attr_value_mask_t mask
  2: i64 attr_val
}

struct ${api_prefix}${hash_calc_name}_input_field_attribute_t {
  1: required i32 input_field;
  2: required ${api_prefix}input_field_attr_type_t type;
  3: i32 value;
  4: binary stream;
}

enum ${api_prefix}${hash_calc_name}_algo_t {
//::   for alg_name, handle in dyn_hash_calc_info['algs']:
//::     enum_name_upper = p4_pd_prefix.upper() + hash_calc_name.upper() + "_ALGORITHM_" + alg_name.upper()
  ${enum_name_upper},
//::   #endfor
}
//:: #endfor


# Match structs

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
struct ${api_prefix}${table}_match_spec_t {
//::   id = 1
//::   for name, width in match_params:
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::   id += 1
//::   #endfor
}

//:: #endfor

# Match struct for Dynamic Key Mask Exm Table.

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
//::     continue
//::   #endif
//::   if not t_info["dynamic_match_key_masks"]:
//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
struct ${api_prefix}${table}_match_key_mask_spec_t {
//::   id = 1
//::   for name, width in match_params:
//::     name = name + '_mask'
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::   id += 1
//::   #endfor
}

//:: #endfor

# Action structs

//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
//::     continue
//::   #endif
//::   action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
struct ${api_prefix}${action}_action_spec_t {
//::   id = 1
//::   for name, width in action_params:
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::     id += 1
//::   #endfor
}

//:: #endfor
union ${api_prefix}action_specs_t {
//:: id = 1
//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
//::     continue
//::   #endif
  ${id}: ${api_prefix}${action}_action_spec_t ${api_prefix}${action};
//::   id += 1
//:: #endfor
}

struct ${api_prefix}action_desc_t {
  1: required string name;
  2: required ${api_prefix}action_specs_t data;
}

//:: if gen_hitless_ha_test_pd:
struct ${api_prefix}ha_reconc_report_t {
  1: required i32 num_entries_added;
  2: required i32 num_entries_deleted;
  3: required i32 num_entries_modified;
}
//:: #endif

# Register values

//:: for register, r_info in register_info.items():
//::   if r_info["layout"]:
struct ${api_prefix}${register}_value_t {
//::     id = 1
//::     for f_name, f_width in r_info["layout"]:
  ${id}: required ${get_thrift_reg_type(f_width)} ${f_name};
//::       id += 1
//::     #endfor
}

//::   #endif
//:: #endfor

# Entry Descriptions

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::
struct ${api_prefix}${table}_entry_desc_t {
//::   id = 1
//::   if has_match_spec:
  ${id}: required ${api_prefix}${table}_match_spec_t match_spec;
//::     id += 1
//::   #endif
  ${id}: required bool has_mbr_hdl;
//::     id += 1
  ${id}: required bool has_grp_hdl;
//::     id += 1
  ${id}: required MemberHandle_t selector_grp_hdl;
//::     id += 1
  ${id}: required MemberHandle_t action_mbr_hdl;
//::     id += 1
//::   if match_type == "ternary" or match_type == "range":
  ${id}: required i32 priority;
//::     id += 1
//::   #endif
//::   if not action_table_hdl:
  ${id}: required ${api_prefix}action_desc_t action_desc;
//::     id += 1
//::   else:
  ${id}: required list<MemberHandle_t> members;
//::     id += 1
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       member = indirect_res_name + "_index"
  ${id}: required i32 ${member};
//::     id+= 1
//::     #endfor
//::   #endif
}

//:: #endfor

//:: for lq in learn_quanta:
//:: rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
//:: rpc_entry_type = api_prefix + lq["name"] + "_digest_entry_t"
struct ${rpc_entry_type} {
//::   fields = lq["fields"]
//::   count = 1
//::   for field in fields:
//::     field_name = field["field_name"]
//::     bit_width = field_info[field_name]["bit_width"]
//::     byte_width = (bit_width + 7 ) // 8
//::     if byte_width > 4:
//::       field_definition = "list<byte> %s" % field_name
//::     else:
//::       field_definition = "%s %s" % (get_thrift_type(byte_width), field_name)
//::     #endif
  ${count}: required ${field_definition};
//::   count += 1
//::   #endfor
}

struct ${rpc_msg_type} {
  1: required res.DevTarget_t             dev_tgt;
  2: required list<${rpc_entry_type}> msg;
  3: required i64                     msg_ptr;
}
//:: #endfor


exception InvalidTableOperation {
 1:i32 code
}

exception InvalidLearnOperation {
 1:i32 code
}

exception InvalidDbgOperation {
 1:i32 code
}

exception InvalidSnapshotOperation {
 1:i32 code
}

exception InvalidCounterOperation {
 1:i32 code
}

exception InvalidRegisterOperation {
 1:i32 code
}

exception InvalidMeterOperation {
 1:i32 code
}

exception InvalidLPFOperation {
 1:i32 code
}

exception InvalidWREDOperation {
 1:i32 code
}


service ${p4_prefix} {

    # Idle time config

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not t_info["timeout"]: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             api_prefix + "idle_time_params_t params"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_idle_tmo_enable"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "i32 cookie"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_idle_register_tmo_cb"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_idle_tmo_get_expired"
    list<${api_prefix}idle_tmo_expired_t> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_idle_tmo_disable"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_idle_params_get"
    ${api_prefix}idle_time_params_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry",
//::             "i32 ttl"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_set_ttl"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_get_ttl"
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_reset_ttl"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_update_hit_state"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_get_hit_state"
    ${api_prefix}idle_time_hit_state ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//:: #endfor


//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0:
//::     continue
//::   #endif
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             api_prefix + table + "_match_spec_t match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["i32 priority"]
//::   #endif
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_match_spec_to_entry_hdl"
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor


    # Dynamic Key Mask Exm Table.
      # set API
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if len(t_info["match_fields"]) == 0:
//::    continue
//::   #endif
//::   if not t_info["dynamic_match_key_masks"]:
//::     continue
//::   #endif

      # Set API
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             api_prefix + table + "_match_key_mask_spec_t match_spec"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_match_key_mask_spec_set"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

      # Get API
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_match_key_mask_spec_get"
    ${api_prefix}${table}_match_key_mask_spec_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

      # reset API
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_match_key_mask_spec_reset"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

    # Table entry add functions

//::
//:: def get_direct_parameter_specs(direct_resources, prefix, register_info):
//::   specs = []
//::   for res_name, res_type, _ in direct_resources:
//::     if res_type == "bytes_meter":
//::       specs += [prefix + "bytes_meter_spec_t " + res_name + "_spec"]
//::     elif res_type == "packets_meter":
//::       specs += [prefix + "packets_meter_spec_t " + res_name + "_spec"]
//::     elif res_type == "register":
//::       specs += [register_info[res_name]["v_thrift_type"] + " " + res_name + "_spec"]
//::     elif res_type == "lpf":
//::       specs += [prefix + "lpf_spec_t " + res_name + "_spec"]
//::     elif res_type == "wred":
//::       specs += [prefix + "wred_spec_t " + res_name + "_spec"]
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
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt",
//::               api_prefix + table + "_match_spec_t match_spec"]
//::     if match_type == "ternary" or match_type == "range":
//::       params += ["i32 priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     if t_info["timeout"]:
//::       params += ["i32 ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_table_add_with_" + action
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endfor

    # Table entry modify functions
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
//::       params = ["res.SessionHandle_t sess_hdl"]
//::       if idx:
//::         name += "_by_match_spec"
//::         params += ["res.DevTarget_t dev_tgt",
//::                    api_prefix + table + "_match_spec_t match_spec"]
//::         if match_type == "ternary" or match_type == "range":
//::           params += ["i32 priority"]
//::         #endif
//::       else:
//::         params += ["byte dev_id",
//::                    "EntryHandle_t entry"]
//::       #endif
//::       if has_action_spec:
//::         params += [api_prefix + action + "_action_spec_t action_spec"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::       param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::       param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     #endfor
//::   #endfor
//:: #endfor

    # Table entry delete functions
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
# //::   if action_table_hdl: continue
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = table + "_table_delete"
//::     params = ["res.SessionHandle_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["res.DevTarget_t dev_tgt",
//::                  api_prefix + table + "_match_spec_t match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["i32 priority"]
//::       #endif
//::     else:
//::       params += ["byte dev_id",
//::                  "EntryHandle_t entry"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endfor

    # Table default entry get functions
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = table + "_table_get_default_entry_handle"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::   name = table + "_table_get_default_entry"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "bool read_from_hw"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    ${api_prefix}${table}_entry_desc_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

    # Table default entry clear functions
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
# //::   if action_table_hdl: continue
//::   name = table + "_table_reset_default_entry"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

//:: for table, t_info in table_info.items():
//::   name = table + "_get_entry_count"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
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
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::   if not select_hdl: continue
//::   name = act_prof + "_get_selector_group_count"
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::   name = act_prof + "_get_selector_group_member_count"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

    # Get first entry handle functions
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::
//::   name = table + "_get_first_entry_handle"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   name = table + "_get_next_entry_handles"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "EntryHandle_t entry_hdl",
//::             "i32 n"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    list<EntryHandle_t> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   # same for direct and indirect, the difference is in the *_entry_desc_t
//::   name = table + "_get_entry"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry_hdl",
//::             "bool read_from_hw"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    ${api_prefix}${table}_entry_desc_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   if gen_md_pd:
//::     name = table + "_get_entry_from_plcmt_data"
//::     params = ["binary drv_data"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    ${api_prefix}${table}_entry_desc_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = table + "_get_plcmt_data"
//::     params = ["res.SessionHandle_t sess_hdl", "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

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
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "MemberHandle_t mbr_hdl",
//::             "bool read_from_hw"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    ${api_prefix}action_desc_t ${name}(${param_str});
//::   if gen_md_pd:

//::     name = act_prof + "_get_member_from_plcmt_data"
//::     params = ["binary drv_data"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    ${api_prefix}action_desc_t ${name}(${param_str});

//::     name = act_prof + "_get_full_member_info_from_plcmt_data"
//::     params = ["byte dev_id",
//::               "MemberHandle_t mbr_hdl",
//::               "binary drv_data"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    ${api_prefix}action_desc_t ${name}(${param_str});

//::     name = act_prof + "_get_plcmt_data"
//::     params = ["res.SessionHandle_t sess_hdl", "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   #endif
# Get first/next entry handles for action profile and selector tables
//::   name = act_prof + "_get_first_member"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   name = act_prof + "_get_next_members"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "EntryHandle_t entry_hdl",
//::             "i32 n"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    list<i32> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   if not select_hdl: continue
//::   name = act_prof + "_get_first_group"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   name = act_prof + "_get_next_groups"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "EntryHandle_t entry_hdl",
//::             "i32 n"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    list<i32> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   name = act_prof + "_get_first_group_member"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t grp_hdl"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   name = act_prof + "_get_next_group_members"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t grp_hdl",
//::             "EntryHandle_t mbr_hdl",
//::             "i32 n"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    list<i32> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   if gen_md_pd:
//::     name = act_prof + "_get_word_llp_active_member_count"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt",
//::               "i32 word_index"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     name = act_prof + "_get_word_llp_active_members"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt",
//::               "i32 word_index",
//::               "i32 count"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    list<i32> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     name = act_prof + "_sel_get_plcmt_data"
//::     params = ["res.SessionHandle_t sess_hdl", "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endif
//:: #endfor

    # Table set default action functions

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   for action in t_info["actions"]:
//::     name = table + "_set_default_action_" + action
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     a_info = action_info[action]
//::     if not a_info['allowed_to_be_default_action'][table]: continue
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endfor

//:: if gen_exm_test_pd == 1:
    # Exact match entry activate functions
//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     name = table + "_activate_entry"
//::     if match_type != "exact":
//::       continue
//::     #endif
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "EntryHandle_t entry"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ",".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor

    #   Exact match entry deactivate functions
//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     name = table + "_deactivate_entry"
//::     if match_type != "exact":
//::       continue
//::     #endif
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "EntryHandle_t entry"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ",".join(param_list)
     void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endif
    
     # Table set/get property
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
# //::   if action_table_hdl: continue
//::   name = table + "_set_property"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "tbl_property_t property",
//::             "tbl_property_value_t value",
//::             "i32 prop_args"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   name = table + "_get_property"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "tbl_property_t property"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    tbl_property_value_args_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

    # INDIRECT ACTION DATA AND MATCH SELECT

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
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_add_member_with_" + action
//::
    MemberHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::		   	    "MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_modify_member_with_" + action
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_member"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   if not select_hdl: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "i32 max_grp_size"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_create_group"
    GroupHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_group"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_add_member_to_group"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_member_from_group"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr",
//::             api_prefix + "grp_mbr_state mbr_state"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_group_member_state_set"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_group_member_state_get"
    ${api_prefix}grp_mbr_state ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "list<byte> hash_val"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_group_member_get_from_hash"
    MemberHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_set_dynamic_action_selection_fallback_member"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_reset_dynamic_action_selection_fallback_member"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_sel_get_updates"
    list<${api_prefix}sel_update_t> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["byte dev_id",
//::             "i32 cookie"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_sel_track_updates"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             api_prefix + table + "_match_spec_t match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["i32 priority"]
//::   #endif
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   if t_info["timeout"]:
//::     params_wo += ["i32 ttl"]
//::   #endif
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["i32 " + r + "_index" for r in bound_indirect_res]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_add_entry"
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::   if not select_hdl: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   if t_info["timeout"]:
//::     params_w += ["i32 ttl"]
//::   #endif
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   params_w += ["i32 " + r + "_index" for r in bound_indirect_res]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_add_entry_with_selector"
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = table + "_modify_entry"
//::     params = ["res.SessionHandle_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["res.DevTarget_t dev_tgt",
//::                  api_prefix + table + "_match_spec_t match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["i32 priority"]
//::       #endif
//::     else:
//::       params += ["byte dev_id",
//::                  "EntryHandle_t entry"]
//::     #endif
//::     params_wo = params + ["MemberHandle_t mbr"]
//::     params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       bound_indirect_res += [indirect_res_name]
//::     #endfor
//::     params_wo += ["i32 " + r + "_index" for r in bound_indirect_res]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::     if not select_hdl: continue
//::     params_w = params + ["GroupHandle_t grp"]
//::     params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::     params_w += ["i32 " + r + "_index" for r in bound_indirect_res]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_modify_entry_with_selector"
//::     if idx:
//::       name += "_by_match_spec"
//::     #endif
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["i32 " + r + "_index" for r in bound_indirect_res]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_set_default_entry"
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::
//::   if not select_hdl: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::   params_w += ["i32 " + r + "_index" for r in bound_indirect_res]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_set_default_entry_with_selector"
    EntryHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endfor

//:: for lq in learn_quanta:
//::   rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
  void ${lq["name"]}_register(1: res.SessionHandle_t sess_hdl, 2: byte dev_id) throws (1:InvalidLearnOperation ouch),
  void ${lq["name"]}_deregister(1: res.SessionHandle_t sess_hdl, 2: byte dev_id) throws (1:InvalidLearnOperation ouch),
  ${rpc_msg_type} ${lq["name"]}_get_digest(1: res.SessionHandle_t sess_hdl) throws (1:InvalidLearnOperation ouch),
  void ${lq["name"]}_digest_notify_ack(1: res.SessionHandle_t sess_hdl, 2: i64 msg_ptr) throws (1:InvalidLearnOperation ouch),
//:: #endfor

//:: name = "set_learning_timeout"
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:byte dev_id, 3:i32 usecs) throws (1:InvalidLearnOperation ouch),

//:: name = "tbl_dbg_counter_type_set"
    void ${name}(1:res.DevTarget_t dev_tgt, 2:string tbl_name, 3:tbl_dbg_counter_type_t type) throws (1:InvalidDbgOperation ouch),

//:: name = "tbl_dbg_counter_get"
    TblCntrInfo_t ${name}(1:res.DevTarget_t dev_tgt, 2:string tbl_name) throws (1:InvalidDbgOperation ouch),

//:: name = "tbl_dbg_counter_clear"
    void ${name}(1:res.DevTarget_t dev_tgt, 2:string tbl_name) throws (1:InvalidDbgOperation ouch),

//:: name = "tbl_dbg_counter_type_stage_set"
    void ${name}(1:res.DevTarget_t dev_tgt, 2:byte stage, 3:tbl_dbg_counter_type_t type) throws (1:InvalidDbgOperation ouch),

//:: name = "tbl_dbg_counter_stage_get"
    TblDbgStageInfo_t ${name}(1:res.DevTarget_t dev_tgt, 2:byte stage) throws (1:InvalidDbgOperation ouch),

//:: name = "tbl_dbg_counter_stage_clear"
    void ${name}(1:res.DevTarget_t dev_tgt, 2: byte stage) throws (1:InvalidDbgOperation ouch),

//:: name = "snapshot_create"
    SnapshotHandle_t ${name}(1:res.DevTarget_t dev_tgt, 2:byte start_stage, 3:byte end_stage, 4:byte direction) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_delete"
    void ${name}(1:SnapshotHandle_t handle) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_state_set"
    void ${name}(1: SnapshotHandle_t handle, 2:i32 state, 3:i32 usecs) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_state_get"
    i32 ${name}(1:SnapshotHandle_t handle, 2:i16 pipe) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_timer_enable"
    void ${name}(1: SnapshotHandle_t handle, 2:byte disable) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_capture_trigger_set"
    void ${name}(1: SnapshotHandle_t handle,
                2:${api_prefix}snapshot_trig_spec_t trig_spec,
                3:${api_prefix}snapshot_trig_spec_t trig_spec2) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_capture_data_get"
    i64 ${name}(1: SnapshotHandle_t handle, 2:i16 pipe, 3:i16 stage_id, 4:string field_name) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_capture_tbl_data_get"
    ${api_prefix}snapshot_tbl_data_t ${name}(1: SnapshotHandle_t handle, 2:i16 pipe, 3:string table_name) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_capture_trigger_fields_clr"
    void ${name}(1:SnapshotHandle_t handle) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_field_in_scope"
    bool ${name}(1:res.DevTarget_t dev_tgt, 2:byte stage,
                 3:byte direction, 4:string field_name) throws (1:InvalidSnapshotOperation ouch),

//:: name = "snapshot_trigger_field_in_scope"
    bool ${name}(1:res.DevTarget_t dev_tgt, 2:byte stage,
                 3:byte direction, 4:string field_name) throws (1:InvalidSnapshotOperation ouch),

    # counters

//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "counter_read_" + counter
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry, 4:${api_prefix}counter_flags_t flags) throws (1:InvalidCounterOperation ouch),
//::     name = "counter_write_" + counter
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry, 4:${api_prefix}counter_value_t counter_value) throws (1:InvalidCounterOperation ouch),

//::   else:
//::     name = "counter_read_" + counter
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}counter_flags_t flags) throws (1:InvalidCounterOperation ouch),
//::     name = "counter_write_" + counter
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}counter_value_t counter_value) throws (1:InvalidCounterOperation ouch),

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "counter_hw_sync_" + counter
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:bool blocking) throws (1:InvalidCounterOperation ouch),
//:: #endfor

    # registers

//:: for register, r_info in register_info.items():
//::   binding = r_info["binding"]
//::   table_type = r_info["table_type"]
//::   if table_type == "fifo":
//::     name = "register_occupancy_" + register
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt) throws (1:InvalidRegisterOperation ouch),
//::     name = "register_reset_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt) throws (1:InvalidRegisterOperation ouch),
//::     if r_info['direction'] == "in":
//::       name = "register_dequeue_" + register
    list<${r_info["v_thrift_type"]}> ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 num_to_dequeue) throws (1:InvalidRegisterOperation ouch),
//::     #endif
//::     if r_info['direction'] == "out":
//::       name = "register_enqueue_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:list<${r_info["v_thrift_type"]}> register_values) throws (1:InvalidRegisterOperation ouch),
//::     #endif
//::   else:
//::     name = "register_hw_sync_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt) throws (1:InvalidRegisterOperation ouch),
//::     if binding[0] == "direct":
//::       name = "register_read_" + register
    list<${r_info["v_thrift_type"]}> ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry, 4:${api_prefix}register_flags_t flags) throws (1:InvalidRegisterOperation ouch),
//::       name = "register_write_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:byte dev_id, 3:EntryHandle_t entry, 4:${r_info["v_thrift_type"]} register_value) throws (1:InvalidRegisterOperation ouch),

//::     else:
//::       name = "register_read_" + register
    list<${r_info["v_thrift_type"]}> ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}register_flags_t flags) throws (1:InvalidRegisterOperation ouch),
//::       name = "register_write_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${r_info["v_thrift_type"]} register_value) throws (1:InvalidRegisterOperation ouch),
//::       name = "register_reset_all_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt) throws (1:InvalidRegisterOperation ouch),
//::       name = "register_range_reset_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt 3:i32 index 4:i32 count) throws (1:InvalidRegisterOperation ouch),
//::       name = "register_write_all_" + register
    void ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:${r_info["v_thrift_type"]} register_value) throws (1:InvalidRegisterOperation ouch),
//::   name = "register_range_read_" + register
    list<${r_info["v_thrift_type"]}> ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 index, 4:i32 count, 5:${api_prefix}register_flags_t flags) throws (1:InvalidRegisterOperation ouch),
//::     #endif
//::   #endif
//:: #endfor

//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   params = ["1:res.SessionHandle_t sess_hdl",
//::             "2:res.DevTarget_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["3:EntryHandle_t entry"]
//::   else:
//::     params += ["3:i32 index"]
//::   #endif
//::
//::   param_str = ", ".join(params)
//::   if type_ == "packets":
//::     spec = api_prefix + "packets_meter_spec_t"
//::   else:
//::     spec = api_prefix + "bytes_meter_spec_t"
//::   #endif
//::   name = "meter_read_" + meter
    ${spec} ${name}(${param_str}) throws (1:InvalidMeterOperation ouch),
//::   if type_ == "packets":
//::     params += ["4:" + api_prefix + "packets_meter_spec_t meter_spec"]
//::   else:
//::     params += ["4:" + api_prefix + "bytes_meter_spec_t meter_spec"]
//::   #endif
//::
//::   param_str = ", ".join(params)
//::   name = "meter_set_" + meter
    void ${name}(${param_str}) throws (1:InvalidMeterOperation ouch),
//::   name = "meter_bytecount_adjust_set_" + meter
//::   params = ["1:res.SessionHandle_t sess_hdl",
//::             "2:res.DevTarget_t dev_tgt",
//::             "3:i32 bytecount"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) throws (1:InvalidMeterOperation ouch),
//::   name = "meter_bytecount_adjust_get_" + meter
//::   params = ["1:res.SessionHandle_t sess_hdl",
//::             "2:res.DevTarget_t dev_tgt"]
//::   param_str = ", ".join(params)
    i32 ${name}(${param_str}) throws (1:InvalidMeterOperation ouch),
//:: #endfor

//:: for lpf, l_info in lpf_info.items():
//::   binding = l_info["binding"]
//::   params = ["1:res.SessionHandle_t sess_hdl",
//::             "2:res.DevTarget_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["3:EntryHandle_t entry"]
//::   else:
//::     params += ["3:i32 index"]
//::   #endif
//::
//::   param_str = ", ".join(params)
//::   spec = api_prefix + "lpf_spec_t"
//::   name = "lpf_read_" + lpf
    ${spec} ${name}(${param_str}) throws (1:InvalidLPFOperation ouch),
//::   params += ["4:" + api_prefix + "lpf_spec_t lpf_spec"]
//::
//::   param_str = ", ".join(params)
//::   name = "lpf_set_" + lpf
    void ${name}(${param_str}) throws (1:InvalidLPFOperation ouch),
//:: #endfor

//:: for wred, w_info in wred_info.items():
//::   binding = w_info["binding"]
//::   params = ["1:res.SessionHandle_t sess_hdl",
//::             "2:res.DevTarget_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["3:EntryHandle_t entry"]
//::   else:
//::     params += ["3:i32 index"]
//::   #endif
//::
//::   param_str = ", ".join(params)
//::   spec = api_prefix + "wred_spec_t"
//::   name = "wred_read_" + wred
    ${spec} ${name}(${param_str}) throws (1:InvalidWREDOperation ouch),
//::   params += ["4:" + api_prefix + "wred_spec_t wred_spec"]
//::
//::   param_str = ", ".join(params)
//::   name = "wred_set_" + wred
    void ${name}(${param_str}) throws (1:InvalidWREDOperation ouch),
//:: #endfor

//:: if gen_perf_test_pd == 1:
    void threads_init(),
    void threads_start(),
    void threads_stop(),
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     params += ["i32 num_entries"]
//::
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_table_bulk_init"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     name = table + "_table_perf_test_execute"
    i32 ${name}(),
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if action_table_hdl: continue
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     for action in t_info["actions"]:
//::       a_info = action_info[action]
//::       has_action_spec = len(a_info["param_names"]) > 0
//::       params = ["res.SessionHandle_t sess_hdl",
//::                 "res.DevTarget_t dev_tgt"]
//::       if has_match_spec:
//::         params += [api_prefix + table + "_match_spec_t match_spec"]
//::       #endif
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["i32 priority"]
//::       #endif
//::       if has_action_spec:
//::         params += [api_prefix + action + "_action_spec_t action_spec"]
//::       #endif
//::       if t_info["timeout"]:
//::         params += ["i32 ttl"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], api_prefix, register_info)
//::       param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::       param_str = ", ".join(param_list)
//::       name = table + "_table_add_with_" + action + "_bulk_setup"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     #endfor
//::   #endfor
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     name = table + "_churn_thread_spawn"
    void ${name} () throws (1:InvalidTableOperation ouch),
//::     for res_name, res_type, res_hdl in table_direct_resources[table]:
//::       if res_type == "counter":
//::         name = table + "_stats_ent_dump_thread_spawn"
//::         params = ["res.SessionHandle_t sess_hdl",
//::                   "res.DevTarget_t dev_tgt",
//::                   "list<EntryHandle_t> entry_hdls"]
//::       param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::       param_str = ", ".join(param_list)
    void ${name} (${param_str}) throws (1:InvalidTableOperation ouch),
//::       #endif
//::     #endfor
//::   #endfor
//:: #endif

//:: if gen_md_pd:
    # MAT, ADT and SEL Update Tracking

//::   params = ["byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = "get_tbl_updates"
    list<${api_prefix}tbl_update_t> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte v_dev_id",
//::             "byte p_dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = "program_all_updates"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   if gen_hitless_ha_test_pd:
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = "enable_callbacks_for_hitless_ha"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endif
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_register_mat_update_cb"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor

//::   action_profiles = set()
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if not action_table_hdl: continue
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::     if act_prof in action_profiles: continue
//::     action_profiles.add(act_prof)

//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_register_adt_update_cb"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     if select_hdl:
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_register_sel_update_cb"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::     #endif
//::   #endfor

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "list<" + api_prefix + "tbl_update_t> updates"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = "restore_virtual_dev_state"
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//:: #endif

//:: if parser_value_set["ingress"] or parser_value_set["egress"]:
    # Parser Value Set
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

//::     name = "pvs_"+ pvs_name + "_set_property"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "pvs_property_t property",
//::               "pvs_property_value_t value",
//::               "pvs_gress_t gress"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_get_property"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "pvs_property_t property",
//::               "pvs_gress_t gress"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    pvs_property_value_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_add"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevParserTarget_t dev_prsr_tgt",
//::               "i32  parser_value",
//::               "i32  parser_value_mask"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    PvsHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_modify"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "PvsHandle_t pvs_handle",
//::               "i32  parser_value",
//::               "i32  parser_value_mask"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_delete"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "PvsHandle_t pvs_handle"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    void ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_get"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "PvsHandle_t pvs_handle"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    PVSSpec_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_handle_get"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevParserTarget_t dev_prsr_tgt",
//::               "i32  parser_value",
//::               "i32  parser_value_mask"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    PvsHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_get_first_entry_handle"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevParserTarget_t dev_prsr_tgt"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    PvsHandle_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_get_next_entry_handles"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevParserTarget_t dev_prsr_tgt",
//::               "PvsHandle_t entry_handle",
//::               "i32 n"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    list<PvsHandle_t> ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::     name = "pvs_"+ pvs_name + "_entry_get_count"
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevParserTarget_t dev_prsr_tgt",
//::               "bool read_from_hw"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    i32 ${name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   #endfor
//:: #endif

//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += [api_prefix + hash_calc_name + "_input_t input"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_set"
    void ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   ret_type = api_prefix + hash_calc_name + "_input_t input"
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_get"
    i32 ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += [api_prefix + hash_calc_name + "_algo_t algo"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_algorithm_set"
    void ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   ret_type = api_prefix + hash_calc_name + "_algo_t algo"
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_algorithm_get"
    i32 ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "i64 seed"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_seed_set"
    void ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_seed_get"
    i64 ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += [api_prefix + hash_calc_name + "_input_t input"]
//::   params += ["list<" + api_prefix + hash_calc_name + "_input_field_attribute_t> array_of_attrs"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_set"
    void ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += [api_prefix + hash_calc_name + "_input_t input"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_count_get"
    i32 ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += [api_prefix + hash_calc_name + "_input_t input"]
//::   attr_type = api_prefix + hash_calc_name + "_input_field_attribute_t"
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_input_field_attribute_get"
    list<${attr_type}> ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id"]
//::   params += ["i32 attr_count"]
//::   params += ["list<" + api_prefix + hash_calc_name + "_input_field_attribute_t> array_of_attrs"]
//::   params += ["list<i32> attr_sizes"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   fn_name = "hash_calc_" + hash_calc_name + "_calculate_hash_value"
    list<byte> ${fn_name}(${param_str}) throws (1:InvalidTableOperation ouch),

//:: #endfor

//:: if gen_hitless_ha_test_pd:
//::   for table, t_info in table_info.items():
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_get_ha_reconciliation_report"
    ${api_prefix}ha_reconc_report_t ${name}(${param_str}) throws (1:InvalidTableOperation ouch),
//::   #endfor
//:: #endif
} 
